//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK - HLSL shader coverage
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"

#define GAMMA_CORRECT_RENDERING
#define USE_FAST_SEMANTICS

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::PackedVector; 
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float SWAP_TIME = 1.f;
    constexpr float INTERACTIVE_TIME = 10.f;

    constexpr float ortho_width = 6.f;
    constexpr float ortho_height = 8.f;

    struct TestVertex
    {
        TestVertex(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate, uint32_t color)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
            XMStoreFloat2(&this->textureCoordinate2, XMVectorScale(textureCoordinate, 3));
            XMStoreUByte4(&this->blendIndices, g_XMZero);

            XMStoreFloat3(&this->tangent, g_XMZero);

            XMStoreFloat4(&this->blendWeight, g_XMIdentityR0);

            XMVECTOR clr = XMLoadUByteN4(reinterpret_cast<XMUBYTEN4*>(&color));
            XMStoreFloat4(&this->color, clr);
        }

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
        XMFLOAT2 textureCoordinate;
        XMFLOAT2 textureCoordinate2;
        XMUBYTE4 blendIndices;
        XMFLOAT4 blendWeight;
        XMFLOAT4 color;

        static constexpr unsigned int InputElementCount = 8;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];

        static constexpr unsigned int InstancedInputElementCount = 11;
        static const D3D11_INPUT_ELEMENT_DESC InstancedInputElements[InstancedInputElementCount];
    };

    const D3D11_INPUT_ELEMENT_DESC TestVertex::InputElements[] =
    {
        { "SV_Position",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     1, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    const D3D11_INPUT_ELEMENT_DESC TestVertex::InstancedInputElements[] =
    {
        { "SV_Position",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD",     1, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "InstMatrix",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",   1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",   2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };

    using VertexCollection = std::vector<TestVertex>;
    using IndexCollection = std::vector<uint16_t>;

    struct aligned_deleter { void operator()(void* p) noexcept { _aligned_free(p); } };

    // Helper for computing tangents (see DirectXMesh <http://go.microsoft.com/fwlink/?LinkID=324981>)
    void ComputeTangents(const IndexCollection& indices, VertexCollection& vertices)
    {
        static const float EPSILON = 0.0001f;
        static const XMVECTORF32 s_flips = { { { 1.f, -1.f, -1.f, 1.f } } };

        size_t nFaces = indices.size() / 3;
        size_t nVerts = vertices.size();

        std::unique_ptr<XMVECTOR[], aligned_deleter> temp(reinterpret_cast<XMVECTOR*>(_aligned_malloc(sizeof(XMVECTOR) * nVerts * 2, 16)));

        memset(temp.get(), 0, sizeof(XMVECTOR) * nVerts * 2);

        XMVECTOR* tangent1 = temp.get();
        XMVECTOR* tangent2 = temp.get() + nVerts;

        for (size_t face = 0; face < nFaces; ++face)
        {
            uint16_t i0 = indices[face * 3];
            uint16_t i1 = indices[face * 3 + 1];
            uint16_t i2 = indices[face * 3 + 2];

            if (i0 >= nVerts
                || i1 >= nVerts
                || i2 >= nVerts)
            {
                throw std::out_of_range("ComputeTangents");
            }

            XMVECTOR t0 = XMLoadFloat2(&vertices[i0].textureCoordinate);
            XMVECTOR t1 = XMLoadFloat2(&vertices[i1].textureCoordinate);
            XMVECTOR t2 = XMLoadFloat2(&vertices[i2].textureCoordinate);

            XMVECTOR s = XMVectorMergeXY(XMVectorSubtract(t1, t0), XMVectorSubtract(t2, t0));

            XMFLOAT4A tmp;
            XMStoreFloat4A(&tmp, s);

            float d = tmp.x * tmp.w - tmp.z * tmp.y;
            d = (fabsf(d) <= EPSILON) ? 1.f : (1.f / d);
            s = XMVectorScale(s, d);
            s = XMVectorMultiply(s, s_flips);

            XMMATRIX m0;
            m0.r[0] = XMVectorPermute<3, 2, 6, 7>(s, g_XMZero);
            m0.r[1] = XMVectorPermute<1, 0, 4, 5>(s, g_XMZero);
            m0.r[2] = m0.r[3] = g_XMZero;

            XMVECTOR p0 = XMLoadFloat3(&vertices[i0].position);
            XMVECTOR p1 = XMLoadFloat3(&vertices[i1].position);
            XMVECTOR p2 = XMLoadFloat3(&vertices[i2].position);

            XMMATRIX m1;
            m1.r[0] = XMVectorSubtract(p1, p0);
            m1.r[1] = XMVectorSubtract(p2, p0);
            m1.r[2] = m1.r[3] = g_XMZero;

            XMMATRIX uv = XMMatrixMultiply(m0, m1);

            tangent1[i0] = XMVectorAdd(tangent1[i0], uv.r[0]);
            tangent1[i1] = XMVectorAdd(tangent1[i1], uv.r[0]);
            tangent1[i2] = XMVectorAdd(tangent1[i2], uv.r[0]);

            tangent2[i0] = XMVectorAdd(tangent2[i0], uv.r[1]);
            tangent2[i1] = XMVectorAdd(tangent2[i1], uv.r[1]);
            tangent2[i2] = XMVectorAdd(tangent2[i2], uv.r[1]);
        }

        for (size_t j = 0; j < nVerts; ++j)
        {
            // Gram-Schmidt orthonormalization
            XMVECTOR b0 = XMLoadFloat3(&vertices[j].normal);
            b0 = XMVector3Normalize(b0);

            XMVECTOR tan1 = tangent1[j];
            XMVECTOR b1 = XMVectorSubtract(tan1, XMVectorMultiply(XMVector3Dot(b0, tan1), b0));
            b1 = XMVector3Normalize(b1);

            XMVECTOR tan2 = tangent2[j];
            XMVECTOR b2 = XMVectorSubtract(XMVectorSubtract(tan2, XMVectorMultiply(XMVector3Dot(b0, tan2), b0)), XMVectorMultiply(XMVector3Dot(b1, tan2), b1));
            b2 = XMVector3Normalize(b2);

            // handle degenerate vectors
            float len1 = XMVectorGetX(XMVector3Length(b1));
            float len2 = XMVectorGetY(XMVector3Length(b2));

            if ((len1 <= EPSILON) || (len2 <= EPSILON))
            {
                if (len1 > 0.5f)
                {
                    // Reset bi-tangent from tangent and normal
                    b2 = XMVector3Cross(b0, b1);
                }
                else if (len2 > 0.5f)
                {
                    // Reset tangent from bi-tangent and normal
                    b1 = XMVector3Cross(b2, b0);
                }
                else
                {
                    // Reset both tangent and bi-tangent from normal
                    XMVECTOR axis;

                    float d0 = fabs(XMVectorGetX(XMVector3Dot(g_XMIdentityR0, b0)));
                    float d1 = fabs(XMVectorGetX(XMVector3Dot(g_XMIdentityR1, b0)));
                    float d2 = fabs(XMVectorGetX(XMVector3Dot(g_XMIdentityR2, b0)));
                    if (d0 < d1)
                    {
                        axis = (d0 < d2) ? g_XMIdentityR0 : g_XMIdentityR2;
                    }
                    else if (d1 < d2)
                    {
                        axis = g_XMIdentityR1;
                    }
                    else
                    {
                        axis = g_XMIdentityR2;
                    }

                    b1 = XMVector3Cross(b0, axis);
                    b2 = XMVector3Cross(b0, b1);
                }
            }

            XMStoreFloat3(&vertices[j].tangent, b1);
        }
    }


    struct TestCompressedVertex
    {
        TestCompressedVertex(const TestVertex& bn)
        {
            position = bn.position;
            blendIndices = bn.blendIndices;

            XMVECTOR v = XMLoadFloat3(&bn.normal);
            v = XMVectorMultiplyAdd(v, g_XMOneHalf, g_XMOneHalf);
            XMStoreFloat3PK(&this->normal, v);

            v = XMLoadFloat3(&bn.tangent);
            v = XMVectorMultiplyAdd(v, g_XMOneHalf, g_XMOneHalf);
            XMStoreFloat3PK(&this->tangent, v);

            v = XMLoadFloat2(&bn.textureCoordinate);
            XMStoreHalf2(&this->textureCoordinate, v);

            v = XMLoadFloat2(&bn.textureCoordinate2);
            XMStoreHalf2(&this->textureCoordinate2, v);

            v = XMLoadFloat4(&bn.blendWeight);
            XMStoreUByteN4(&this->blendWeight, v);

            v = XMLoadFloat4(&bn.color);
            XMStoreColor(&this->color, v);
        }

        XMFLOAT3 position;
        XMFLOAT3PK normal;
        XMFLOAT3PK tangent;
        XMHALF2 textureCoordinate;
        XMHALF2 textureCoordinate2;
        XMUBYTE4 blendIndices;
        XMUBYTEN4 blendWeight;
        XMCOLOR color;

        static constexpr unsigned int InputElementCount = 8;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];

        static constexpr unsigned int InstancedInputElementCount = 11;
        static const D3D11_INPUT_ELEMENT_DESC InstancedInputElements[InstancedInputElementCount];
    };


    const D3D11_INPUT_ELEMENT_DESC TestCompressedVertex::InputElements[] =
    {
        { "SV_Position",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R11G11B10_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",      0, DXGI_FORMAT_R11G11B10_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R16G16_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     1, DXGI_FORMAT_R16G16_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",        0, DXGI_FORMAT_B8G8R8A8_UNORM,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    const D3D11_INPUT_ELEMENT_DESC TestCompressedVertex::InstancedInputElements[] =
    {
        { "SV_Position",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NORMAL",       0, DXGI_FORMAT_R11G11B10_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TANGENT",      0, DXGI_FORMAT_R11G11B10_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R16G16_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD",     1, DXGI_FORMAT_R16G16_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "COLOR",        0, DXGI_FORMAT_B8G8R8A8_UNORM,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "InstMatrix",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",   1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",   2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };

    // Creates a cube primitive.
    UINT CreateCube(
        _In_ ID3D11Device* device,
        _Outptr_ ID3D11Buffer** vertexBuffer,
        _Outptr_ ID3D11Buffer** compressedVertexBuffer,
        _Outptr_ ID3D11Buffer** indexBuffer)
    {
        if (vertexBuffer)
        {
            *vertexBuffer = nullptr;
        }
        if (compressedVertexBuffer)
        {
            *compressedVertexBuffer = nullptr;
        }
        if (indexBuffer)
        {
            *indexBuffer = nullptr;
        }

        VertexCollection vertices;
        IndexCollection indices;

        // A box has six faces, each one pointing in a different direction.
        constexpr int FaceCount = 6;

        static const XMVECTORF32 faceNormals[FaceCount] =
        {
            { { {  0,  0,  1, 0 } } },
            { { {  0,  0, -1, 0 } }  },
            { { {  1,  0,  0, 0 } }  },
            { { { -1,  0,  0, 0 } }  },
            { { {  0,  1,  0, 0 } }  },
            { { {  0, -1,  0, 0 } }  },
        };

        static const XMVECTORF32 textureCoordinates[4] =
        {
            { { { 1, 0, 0, 0 } } },
            { { { 1, 1, 0, 0 } } },
            { { { 0, 1, 0, 0 } } },
            { { { 0, 0, 0, 0 } } },
        };

        static uint32_t colors[FaceCount]
        {
            0xFF0000FF,
            0xFF00FF00,
            0xFFFF0000,
            0xFFFF00FF,
            0xFFFFFF00,
            0xFF00FFFF,
        };

        const Vector4 tsize(0.25f, 0.25f, 0.25f, 0.f);

        // Create each face in turn.
        for (size_t i = 0; i < FaceCount; i++)
        {
            Vector4 normal = faceNormals[i].v;

            // Get two vectors perpendicular both to the face normal and to each other.
            XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

            Vector4 side1 = XMVector3Cross(normal, basis);
            Vector4 side2 = XMVector3Cross(normal, side1);

            // Six indices (two triangles) per face.
            size_t vbase = vertices.size();
            indices.push_back(uint16_t(vbase + 0));
            indices.push_back(uint16_t(vbase + 1));
            indices.push_back(uint16_t(vbase + 2));

            indices.push_back(uint16_t(vbase + 0));
            indices.push_back(uint16_t(vbase + 2));
            indices.push_back(uint16_t(vbase + 3));

            // Four vertices per face.
            vertices.push_back(TestVertex((normal - side1 - side2) * tsize, normal, textureCoordinates[0], colors[i]));
            vertices.push_back(TestVertex((normal - side1 + side2) * tsize, normal, textureCoordinates[1], colors[i]));
            vertices.push_back(TestVertex((normal + side1 + side2) * tsize, normal, textureCoordinates[2], colors[i]));
            vertices.push_back(TestVertex((normal + side1 - side2) * tsize, normal, textureCoordinates[3], colors[i]));
        }

        // Compute tangents
        ComputeTangents(indices, vertices);

        // Create the D3D buffers.
        DX::ThrowIfFailed(
            CreateStaticBuffer(device, vertices, D3D11_BIND_VERTEX_BUFFER, vertexBuffer)
        );

        assert(vertexBuffer != nullptr && *vertexBuffer != nullptr);
        _Analysis_assume_(vertexBuffer != nullptr && *vertexBuffer != nullptr);

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, indexBuffer)
        );

        assert(indexBuffer != nullptr && *indexBuffer != nullptr);
        _Analysis_assume_(indexBuffer != nullptr && *indexBuffer != nullptr);

        // Create the compressed version
        std::vector<TestCompressedVertex> cvertices;
        cvertices.reserve(vertices.size());
        for (auto&i : vertices)
        {
            TestCompressedVertex cv(i);
            cvertices.emplace_back(std::move(cv));
        }

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, cvertices, D3D11_BIND_VERTEX_BUFFER, compressedVertexBuffer)
        );

        assert(compressedVertexBuffer != nullptr && *compressedVertexBuffer != nullptr);
        _Analysis_assume_(compressedVertexBuffer != nullptr && *compressedVertexBuffer != nullptr);

        return static_cast<UINT>(indices.size());
    }
}


// Helper for creating a D3D input layout.
_Use_decl_annotations_
void Game::CreateTestInputLayout(
    ID3D11Device* device,
    IEffect* effect,
    ID3D11InputLayout** pInputLayout,
    ID3D11InputLayout** pCompresedInputLayout,
    ID3D11InputLayout** pInstancedLayout,
    ID3D11InputLayout** pCompresedInstancedLayout)
{
    auto ibasic = dynamic_cast<BasicEffect*>(effect);
    auto ienvmap = dynamic_cast<EnvironmentMapEffect*>(effect);
    auto inmap = dynamic_cast<NormalMapEffect*>(effect);
    auto ipbr = dynamic_cast<PBREffect*>(effect);
    auto iskin = dynamic_cast<SkinnedEffect*>(effect);
    auto idbg = dynamic_cast<DebugEffect*>(effect);

    void const* shaderByteCode = nullptr;
    size_t byteCodeLength = 0;

    if (pInputLayout)
    {
        *pInputLayout = nullptr;

        if (ibasic)
        {
            ibasic->SetBiasedVertexNormals(false);
        }
        else if (ienvmap)
        {
            ienvmap->SetBiasedVertexNormals(false);
        }
        else if (inmap)
        {
            inmap->SetBiasedVertexNormals(false);
            inmap->SetInstancingEnabled(false);
        }
        else if (ipbr)
        {
            ipbr->SetBiasedVertexNormals(false);
            ipbr->SetInstancingEnabled(false);
        }
        else if (iskin)
        {
            iskin->SetBiasedVertexNormals(false);
        }
        else if (idbg)
        {
            idbg->SetBiasedVertexNormals(false);
            idbg->SetInstancingEnabled(false);
        }

        effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        HRESULT hr = device->CreateInputLayout(TestVertex::InputElements,
            TestVertex::InputElementCount,
            shaderByteCode, byteCodeLength,
            pInputLayout);
        DX::ThrowIfFailed(hr);

        assert(pInputLayout != 0 && *pInputLayout != 0);
        _Analysis_assume_(pInputLayout != 0 && *pInputLayout != 0);
    }

    if (pCompresedInputLayout)
    {
        *pCompresedInputLayout = nullptr;

        if (ibasic)
        {
            ibasic->SetBiasedVertexNormals(true);
        }
        else if (ienvmap)
        {
            ienvmap->SetBiasedVertexNormals(true);
        }
        else if (inmap)
        {
            inmap->SetBiasedVertexNormals(true);
        }
        else if (ipbr)
        {
            ipbr->SetBiasedVertexNormals(true);
        }
        else if (iskin)
        {
            iskin->SetBiasedVertexNormals(true);
        }
        else if (idbg)
        {
            idbg->SetBiasedVertexNormals(true);
        }

        effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        HRESULT hr = device->CreateInputLayout(TestCompressedVertex::InputElements,
            TestCompressedVertex::InputElementCount,
            shaderByteCode, byteCodeLength,
            pCompresedInputLayout);
        DX::ThrowIfFailed(hr);

        assert(pCompresedInputLayout != 0 && *pCompresedInputLayout != 0);
        _Analysis_assume_(pCompresedInputLayout != 0 && *pCompresedInputLayout != 0);
    }

    if (pInstancedLayout)
    {
        *pInstancedLayout = nullptr;

        if (inmap)
        {
            inmap->SetBiasedVertexNormals(false);
            inmap->SetInstancingEnabled(true);
        }
        else if (ipbr)
        {
            ipbr->SetBiasedVertexNormals(false);
            ipbr->SetInstancingEnabled(true);
        }
        else if (idbg)
        {
            idbg->SetBiasedVertexNormals(false);
            idbg->SetInstancingEnabled(true);
        }

        effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        HRESULT hr = device->CreateInputLayout(TestVertex::InstancedInputElements,
            TestVertex::InstancedInputElementCount,
            shaderByteCode, byteCodeLength,
            pInstancedLayout);
        DX::ThrowIfFailed(hr);

        assert(pInstancedLayout != 0 && *pInstancedLayout != 0);
        _Analysis_assume_(pInstancedLayout != 0 && *pInstancedLayout != 0);
    }

    if (pCompresedInstancedLayout)
    {
        *pCompresedInstancedLayout = nullptr;

        if (inmap)
        {
            inmap->SetBiasedVertexNormals(true);
            inmap->SetInstancingEnabled(true);
        }
        else if (ipbr)
        {
            ipbr->SetBiasedVertexNormals(true);
            ipbr->SetInstancingEnabled(true);
        }
        else if (idbg)
        {
            idbg->SetBiasedVertexNormals(true);
            idbg->SetInstancingEnabled(true);
        }

        effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        HRESULT hr = device->CreateInputLayout(TestCompressedVertex::InstancedInputElements,
            TestCompressedVertex::InstancedInputElementCount,
            shaderByteCode, byteCodeLength,
            pCompresedInstancedLayout);
        DX::ThrowIfFailed(hr);

        assert(pCompresedInstancedLayout != 0 && *pCompresedInstancedLayout != 0);
        _Analysis_assume_(pCompresedInstancedLayout != 0 && *pCompresedInstancedLayout != 0);
    }
}

Game::Game() noexcept(false) :
    m_indexCount(0),
    m_instanceCount(0),
    m_renderMode(Render_Normal),
    m_delay(0)
{
#ifdef GAMMA_CORRECT_RENDERING
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif

#ifdef XBOX
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(UWP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0,
        DX::DeviceResources::c_Enable4K_Xbox
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(c_RenderFormat);
#endif

#ifdef LOSTDEVICE
    m_deviceResources->RegisterDeviceNotify(this);
#endif

    // Used for PBREffect velocity buffer
    m_velocityBuffer = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R32_UINT);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(
#ifdef COREWINDOW
    IUnknown* window,
#else
    HWND window,
#endif
    int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();

#ifdef XBOX
    UNREFERENCED_PARAMETER(rotation);
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    m_deviceResources->SetWindow(window);
#ifdef COREWINDOW
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#endif
#elif defined(UWP)
    m_deviceResources->SetWindow(window, width, height, rotation);
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#else
    UNREFERENCED_PARAMETER(rotation);
    m_deviceResources->SetWindow(window, width, height);
#endif

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_delay = SWAP_TIME;
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    auto pad = m_gamePad->GetState(0);
    auto kb = m_keyboard->GetState();

    if (kb.Escape || (pad.IsConnected() && pad.IsViewPressed()))
    {
        ExitGame();
    }

    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    m_keyboardButtons.Update(kb);

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Space) || (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED))
    {
        CycleRenderMode();

        m_delay = INTERACTIVE_TIME;
    }
    else if (!kb.Space && !(pad.IsConnected() && pad.IsYPressed()))
    {
        m_delay -= static_cast<float>(timer.GetElapsedSeconds());

        if (m_delay <= 0.f)
        {
            CycleRenderMode();
            m_delay = SWAP_TIME;
        }
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

#ifdef XBOX
    m_deviceResources->Prepare();
#endif

    // Time-based animation
    float time = static_cast<float>(m_timer.GetTotalSeconds());

    float yaw = time * 0.4f;
    float pitch = time * 0.7f;
    float roll = time * 1.1f;

    auto context = m_deviceResources->GetD3DDeviceContext();

    XMMATRIX world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

    {
        size_t j = 0;
        for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
        {
            XMMATRIX m = world * XMMatrixTranslation(x, 0.f, 0.f);
            XMStoreFloat3x4(&m_instanceTransforms[j], m);
            ++j;
        }

        assert(j == m_instanceCount);

        MapGuard map(context, m_instancedVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0);
        memcpy(map.pData, m_instanceTransforms.get(), j * sizeof(XMFLOAT3X4));
    }

    Clear();

    // Set state objects.
    context->OMSetBlendState(m_states->AlphaBlend(), Colors::White, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
    context->RSSetState(m_states->CullNone());

    ID3D11SamplerState* samplers[] =
    {
        m_states->LinearWrap(),
        m_states->LinearWrap(),
    };

    context->PSSetSamplers(0, 2, samplers);

    // Set the vertex and index buffers.
    switch (m_renderMode)
    {
    case Render_Compressed:
    {
        UINT vertexStride = sizeof(TestCompressedVertex);
        UINT vertexOffset = 0;

        context->IASetVertexBuffers(0, 1, m_compressedVB.GetAddressOf(), &vertexStride, &vertexOffset);
    }
    break;

    case Render_Instanced:
    {
        UINT vertexStride[2] = { sizeof(TestVertex), sizeof(XMFLOAT3X4) };
        UINT vertexOffset[2] = {};
        ID3D11Buffer* vertexBuffers[2] = { m_vertexBuffer.Get(), m_instancedVB.Get() };

        context->IASetVertexBuffers(0, 2, vertexBuffers, vertexStride, vertexOffset);
    }
    break;

    case Render_CompressedInstanced:
    {
        UINT vertexStride[2] = { sizeof(TestCompressedVertex), sizeof(XMFLOAT3X4) };
        UINT vertexOffset[2] = {};
        ID3D11Buffer* vertexBuffers[2] = { m_compressedVB.Get(), m_instancedVB.Get() };

        context->IASetVertexBuffers(0, 2, vertexBuffers, vertexStride, vertexOffset);
    }
    break;

    default:
    {
        UINT vertexStride = sizeof(TestVertex);
        UINT vertexOffset = 0;

        context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);
    }
    break;
    }

    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    float y = ortho_height - 0.5f;

    if (m_renderMode == Render_Instanced || m_renderMode == Render_CompressedInstanced)
    {
        bool showCompressed = (m_renderMode == Render_CompressedInstanced);

        // NormalMapEffect (instanced)
        {
            auto it = m_normalMapInstanced.begin();
            assert(it != m_normalMapInstanced.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                (*it)->Apply(context, XMMatrixTranslation(0, y, -1.f), m_view, m_projection, showCompressed);

                context->DrawIndexedInstanced(m_indexCount, m_instanceCount, 0, 0, 0);

                ++it;
                if (it == m_normalMapInstanced.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_normalMapInstanced.cend());

            y -= 1.f;
        }

        // PBREffect (instanced)
        if (m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
        {
            auto it = m_pbrInstanced.begin();
            assert(it != m_pbrInstanced.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                (*it)->Apply(context, XMMatrixTranslation(0, y, -1.f), m_view, m_projection, showCompressed);

                context->DrawIndexedInstanced(m_indexCount, m_instanceCount, 0, 0, 0);

                ++it;
                if (it == m_pbrInstanced.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_pbrInstanced.cend());

            y -= 1.f;
        }

        // DebugEffect (instanced)
        {
            auto it = m_debugInstanced.begin();
            assert(it != m_debugInstanced.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                (*it)->Apply(context, XMMatrixTranslation(0, y, -1.f), m_view, m_projection, showCompressed);

                context->DrawIndexedInstanced(m_indexCount, m_instanceCount, 0, 0, 0);

                ++it;
                if (it == m_debugInstanced.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_debugInstanced.cend());

            y -= 1.f;
        }
    }
    else
    {
        bool showCompressed = (m_renderMode == Render_Compressed);

        // BasicEffect
        {
            auto it = m_basic.begin();
            assert(it != m_basic.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_basic.cend())
                        break;
                }

                if (it == m_basic.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_basic.cend());

            y -= 1.f;
        }

        // SkinnedEffect
        {
            auto it = m_skinning.begin();
            assert(it != m_skinning.end());

            XMMATRIX bones[4] =
            {
                XMMatrixIdentity(),
                XMMatrixIdentity(),
                XMMatrixIdentity(),
                XMMatrixIdentity(),
            };

            for (; y > -ortho_height; y -= 1.f)
            {
                for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                {
                    (*it)->SetBoneTransforms(bones, 4);
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_skinning.cend())
                        break;
                }

                if (it == m_skinning.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_skinning.cend());

            y -= 1.f;
        }

        // EnvironmentMapEffect
        {
            auto it = m_envmap.begin();
            assert(it != m_envmap.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_envmap.cend())
                        break;
                }

                if (it == m_envmap.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_envmap.cend());

            y -= 1.f;
        }

        // DualTextureEffect
        {
            auto it = m_dual.begin();
            assert(it != m_dual.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_dual.cend())
                        break;
                }

                if (it == m_dual.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_dual.cend());

            y -= 1.f;
        }

        // AlphaTestEffect
        {
            auto it = m_alphTest.begin();
            assert(it != m_alphTest.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_alphTest.cend())
                        break;
                }

                if (it == m_alphTest.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_alphTest.cend());

            y -= 1.f;
        }

        // NormalMapEffect
        float lastx = 0.f;
        {
            auto it = m_normalMap.begin();
            assert(it != m_normalMap.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                float x;
                for (x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_normalMap.cend())
                        break;
                }
                lastx = x;

                if (it == m_normalMap.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_normalMap.cend());

            // SkinnedNormalMapEffect should be on same line...
        }

        // SkinnedNormalMapEffect
        {
            auto it = m_skinningNormalMap.begin();
            assert(it != m_skinningNormalMap.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                for (float x = lastx + 1.f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_skinningNormalMap.cend())
                        break;
                }

                if (it == m_skinningNormalMap.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_skinningNormalMap.cend());

            y -= 1.f;
        }

        // PBREffect
        if (m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
        {
            context->OMSetBlendState(m_states->Opaque(), Colors::White, 0xFFFFFFFF);

            ID3D11RenderTargetView* views[] = { m_deviceResources->GetRenderTargetView(), m_velocityBuffer->GetRenderTargetView() };
            context->OMSetRenderTargets(2, views, m_deviceResources->GetDepthStencilView());

            {
                auto it = m_pbr.begin();
                assert(it != m_pbr.end());

                for (; y > -ortho_height; y -= 1.f)
                {
                    for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                    {
                        (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                        context->DrawIndexed(m_indexCount, 0, 0);

                        ++it;
                        if (it == m_pbr.cend())
                            break;
                    }

                    if (it == m_pbr.cend())
                        break;
                }

                // Make sure we drew all the effects
                assert(it == m_pbr.cend());

                y -= 1.f;
            }

            views[1] = nullptr;
            context->OMSetRenderTargets(2, views, m_deviceResources->GetDepthStencilView());

            context->OMSetBlendState(m_states->AlphaBlend(), Colors::White, 0xFFFFFFFF);
        }

        // SkinnedPBREffect
        if (m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
        {
            auto it = m_skinningPbr.begin();
            assert(it != m_skinningPbr.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_skinningPbr.cend())
                        break;
                }

                if (it == m_skinningPbr.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_skinningPbr.cend());

            y -= 1.f;
        }

        // DebugEffect
        {
            auto it = m_debug.begin();
            assert(it != m_debug.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection, showCompressed);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_debug.cend())
                        break;
                }

                if (it == m_debug.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_debug.cend());

            y -= 1.f;
        }

        // DGSLEffect
        if (!showCompressed)
        {
            auto it = m_dgsl.begin();
            assert(it != m_dgsl.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                float x;
                for (x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_dgsl.cend())
                        break;
                }
                lastx = x;

                if (it == m_dgsl.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_dgsl.cend());

            // SkinnedDGSLEffect should be on same line...
        }

        // SkinnedDGSLEffect
        if (!showCompressed)
        {
            auto it = m_dgslSkinned.begin();
            assert(it != m_dgslSkinned.end());

            for (; y > -ortho_height; y -= 1.f)
            {
                for (float x = lastx + 1.f; x < ortho_width; x += 1.f)
                {
                    (*it)->Apply(context, world * XMMatrixTranslation(x, y, -1.f), m_view, m_projection);
                    context->DrawIndexed(m_indexCount, 0, 0);

                    ++it;
                    if (it == m_dgslSkinned.cend())
                        break;
                }

                if (it == m_dgslSkinned.cend())
                    break;
            }

            // Make sure we drew all the effects
            assert(it == m_dgslSkinned.cend());

            y -= 1.f;
        }
    }

    // Show the new frame.
    m_deviceResources->Present();

#ifdef XBOX
    m_graphicsMemory->Commit();
#endif
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    XMVECTORF32 color;
#ifdef GAMMA_CORRECT_RENDERING
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
#else
    color.v = Colors::CornflowerBlue;
#endif
    context->ClearRenderTargetView(renderTarget, color);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Game::OnResuming()
{
    m_deviceResources->Resume();

    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

#ifdef PC
void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

#ifndef XBOX
void Game::OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation)
{
#ifdef UWP
    if (!m_deviceResources->WindowSizeChanged(width, height, rotation))
        return;
#else
    UNREFERENCED_PARAMETER(rotation);
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;
#endif

    CreateWindowSizeDependentResources();
}
#endif

#ifdef UWP
void Game::ValidateDevice()
{
    m_deviceResources->ValidateDevice();
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef XBOX
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device, m_deviceResources->GetBackBufferCount());
#endif

    m_states = std::make_unique<CommonStates>(device);

#ifdef GAMMA_CORRECT_RENDERING
    bool forceSRGB = true;
#else
    bool forceSRGB = false;
#endif

    // Load textures.
    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"cat.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_cat.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"cubemap.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_cubemap.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, L"spheremap.bmp",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_DEFAULT,
        nullptr, m_envball.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"dualparabola.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_envdual.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"overlay.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_overlay.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"default.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_defaultTex.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"spnza_bricks_a.DDS",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_brickDiffuse.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"spnza_bricks_a_normal.DDS",
        nullptr, m_brickNormal.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"spnza_bricks_a_specular.DDS",
        nullptr, m_brickSpecular.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"Sphere2Mat_baseColor.png",
        nullptr, m_pbrAlbedo.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"Sphere2Mat_normal.png",
        nullptr, m_pbrNormal.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"Sphere2Mat_occlusionRoughnessMetallic.png",
        nullptr, m_pbrRMA.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"Sphere2Mat_emissive.png",
        nullptr, m_pbrEmissive.ReleaseAndGetAddressOf()));

    if (m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
    {
        // These are BC6H, so they need to be FL 11 +
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"Atrium_diffuseIBL.dds",
            nullptr, m_radianceIBL.ReleaseAndGetAddressOf()));

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"Atrium_specularIBL.dds",
            nullptr, m_irradianceIBL.ReleaseAndGetAddressOf()));
    }

    // Create test geometry.
    m_indexCount = CreateCube(device,
        m_vertexBuffer.ReleaseAndGetAddressOf(),
        m_compressedVB.ReleaseAndGetAddressOf(),
        m_indexBuffer.ReleaseAndGetAddressOf());

    // Create instance transforms.
    {
        size_t j = 0;
        for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
        {
            ++j;
        }
        m_instanceCount = static_cast<UINT>(j);

        m_instanceTransforms = std::make_unique<XMFLOAT3X4[]>(j);

        constexpr XMFLOAT3X4 s_identity = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f };

        j = 0;
        for (float x = -ortho_width + 0.5f; x < ortho_width; x += 1.f)
        {
            m_instanceTransforms[j] = s_identity;
            m_instanceTransforms[j]._14 = x;
            ++j;
        }

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = static_cast<UINT>(j * sizeof(XMFLOAT3X4));
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA initData = { m_instanceTransforms.get(), 0, 0 };

        DX::ThrowIfFailed(
            device->CreateBuffer(&desc, &initData, m_instancedVB.ReleaseAndGetAddressOf())
        );
    }

    //--- BasicEffect ----------------------------------------------------------------------

    // BasicEffect (no texture)
    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect*)
    {
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // BasicEffect (textured)
    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetVertexColorEnabled(true);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetVertexColorEnabled(true);
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // BasicEffect (vertex lighting)
    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetVertexColorEnabled(true);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetVertexColorEnabled(true);
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetVertexColorEnabled(true);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetVertexColorEnabled(true);
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // BasicEffect (per pixel light)
    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetVertexColorEnabled(true);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetVertexColorEnabled(true);
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetVertexColorEnabled(true);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetVertexColorEnabled(true);
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // BasicEffect (one light vertex lighting)
    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetLightingEnabled(true);
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetLightingEnabled(true);
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetLightingEnabled(true);
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetVertexColorEnabled(true);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetLightingEnabled(true);
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetVertexColorEnabled(true);
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetLightingEnabled(true);
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetLightingEnabled(true);
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetLightingEnabled(true);
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetVertexColorEnabled(true);
    }));

    m_basic.emplace_back(std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        effect->SetLightingEnabled(true);
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetVertexColorEnabled(true);
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    //--- SkinnedEFfect --------------------------------------------------------------------

    // SkinnedEFfect (vertex lighting)
    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetWeightsPerVertex(2);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetWeightsPerVertex(2);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetWeightsPerVertex(1);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetWeightsPerVertex(1);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // SkinnedEFfect (one light vertex lighting)
    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetWeightsPerVertex(2);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetWeightsPerVertex(2);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetWeightsPerVertex(1);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        effect->SetWeightsPerVertex(1);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // SkinnedEFfect (per pixel lighting)
    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetWeightsPerVertex(2);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetWeightsPerVertex(2);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetWeightsPerVertex(1);
        effect->SetTexture(m_defaultTex.Get());
    }));

    m_skinning.emplace_back(std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [=](SkinnedEffect* effect)
    {
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetWeightsPerVertex(1);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    //--- EnvironmentMapEffect -------------------------------------------------------------

    // EnvironmentMapEffect (fresnel)
    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // EnvironmentMapEffect (no fresnel)
    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // EnvironmentMapEffect (specular)
    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // EnvironmentMapEffect (fresnel + specular)
    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // EnvironmentMapEffect (one light vertex lighting)
    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetLightEnabled(0, true);
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // EnvironmentMapEffect (per pixel lighting)
    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_Cube);
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_Cube);
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_Cube);
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_Cube);
        effect->SetPerPixelLighting(true);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Blue);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // EnvironmentMapEffect sphere mapping (per pixel lighting only)
    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_Sphere);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Green);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envball.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_Sphere);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Green);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envball.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_Sphere);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Green);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envball.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_Sphere);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Green);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envball.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // EnvironmentMapEffect dual parabolic mapping (per pixel lighting only)
    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_DualParabola);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Red);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envdual.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_DualParabola);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Red);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envdual.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_DualParabola);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Red);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envdual.Get());
    }));

    m_envmap.emplace_back(std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [=](EnvironmentMapEffect* effect)
    {
        effect->SetMode(EnvironmentMapEffect::Mapping_DualParabola);
        effect->EnableDefaultLighting();
        effect->SetEnvironmentMapSpecular(Colors::Red);
        effect->SetFresnelFactor(0.f);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envdual.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    //--- DualTextureEFfect ----------------------------------------------------------------

    m_dual.emplace_back(std::make_unique<EffectWithDecl<DualTextureEffect>>(device, [=](DualTextureEffect* effect)
    {
        effect->SetTexture(m_defaultTex.Get());
        effect->SetTexture2(m_overlay.Get());
    }));

    m_dual.emplace_back(std::make_unique<EffectWithDecl<DualTextureEffect>>(device, [=](DualTextureEffect* effect)
    {
        effect->SetTexture(m_defaultTex.Get());
        effect->SetTexture2(m_overlay.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_dual.emplace_back(std::make_unique<EffectWithDecl<DualTextureEffect>>(device, [=](DualTextureEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetTexture2(m_overlay.Get());
    }));

    m_dual.emplace_back(std::make_unique<EffectWithDecl<DualTextureEffect>>(device, [=](DualTextureEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetTexture2(m_overlay.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    //--- AlphaTestEffect ------------------------------------------------------------------

    // AlphaTestEffect (lt/gt)
    m_alphTest.emplace_back(std::make_unique<EffectWithDecl<AlphaTestEffect>>(device, [=](AlphaTestEffect* effect)
    {
        effect->SetTexture(m_cat.Get());
    }));

    m_alphTest.emplace_back(std::make_unique<EffectWithDecl<AlphaTestEffect>>(device, [=](AlphaTestEffect* effect)
    {
        effect->SetTexture(m_cat.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_alphTest.emplace_back(std::make_unique<EffectWithDecl<AlphaTestEffect>>(device, [=](AlphaTestEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_cat.Get());
    }));

    m_alphTest.emplace_back(std::make_unique<EffectWithDecl<AlphaTestEffect>>(device, [=](AlphaTestEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_cat.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    // AlphaTestEffect (eg/ne)
    m_alphTest.emplace_back(std::make_unique<EffectWithDecl<AlphaTestEffect>>(device, [=](AlphaTestEffect* effect)
    {
        effect->SetAlphaFunction(D3D11_COMPARISON_NOT_EQUAL);
        effect->SetTexture(m_cat.Get());
    }));

    m_alphTest.emplace_back(std::make_unique<EffectWithDecl<AlphaTestEffect>>(device, [=](AlphaTestEffect* effect)
    {
        effect->SetAlphaFunction(D3D11_COMPARISON_NOT_EQUAL);
        effect->SetTexture(m_cat.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_alphTest.emplace_back(std::make_unique<EffectWithDecl<AlphaTestEffect>>(device, [=](AlphaTestEffect* effect)
    {
        effect->SetAlphaFunction(D3D11_COMPARISON_NOT_EQUAL);
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_cat.Get());
    }));

    m_alphTest.emplace_back(std::make_unique<EffectWithDecl<AlphaTestEffect>>(device, [=](AlphaTestEffect* effect)
    {
        effect->SetAlphaFunction(D3D11_COMPARISON_NOT_EQUAL);
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_cat.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    //--- NormalMapEffect ------------------------------------------------------------------

    // NormalMapEffect (no specular)
    m_normalMap.emplace_back(std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
    }));

    m_normalMap.emplace_back(std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_normalMapInstanced.emplace_back(std::make_unique<InstancedEffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
    }));

    // NormalMapEffect (specular)
    m_normalMap.emplace_back(std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
        effect->SetSpecularTexture(m_brickSpecular.Get());
    }));

    m_normalMap.emplace_back(std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
        effect->SetSpecularTexture(m_brickSpecular.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_normalMapInstanced.emplace_back(std::make_unique<InstancedEffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
        effect->SetSpecularTexture(m_brickSpecular.Get());
    }));

    // NormalMapEffect (vertex color)
    m_normalMap.emplace_back(std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
    }));

    m_normalMap.emplace_back(std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_normalMap.emplace_back(std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
        effect->SetSpecularTexture(m_brickSpecular.Get());
    }));

    m_normalMap.emplace_back(std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
        effect->SetSpecularTexture(m_brickSpecular.Get());
        effect->SetFogEnabled(true);
        effect->SetFogColor(Colors::Black);
    }));

    m_normalMapInstanced.emplace_back(std::make_unique<InstancedEffectWithDecl<NormalMapEffect>>(device, [=](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetVertexColorEnabled(true);
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
    }));

    //--- SkinnedNormalMapEffect -----------------------------------------------------------

    // SkinnedNormalMapEffect (no specular)
    m_skinningNormalMap.emplace_back(std::make_unique<EffectWithDecl<SkinnedNormalMapEffect>>(device, [=](SkinnedNormalMapEffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetTexture(m_brickDiffuse.Get());
            effect->SetNormalTexture(m_brickNormal.Get());
        }));

    m_skinningNormalMap.emplace_back(std::make_unique<EffectWithDecl<SkinnedNormalMapEffect>>(device, [=](SkinnedNormalMapEffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetTexture(m_brickDiffuse.Get());
            effect->SetNormalTexture(m_brickNormal.Get());
            effect->SetFogEnabled(true);
            effect->SetFogColor(Colors::Black);
        }));

    // SkinnedNormalMapEffect (specular)
    m_skinningNormalMap.emplace_back(std::make_unique<EffectWithDecl<SkinnedNormalMapEffect>>(device, [=](SkinnedNormalMapEffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetTexture(m_brickDiffuse.Get());
            effect->SetNormalTexture(m_brickNormal.Get());
            effect->SetSpecularTexture(m_brickSpecular.Get());
        }));

    m_skinningNormalMap.emplace_back(std::make_unique<EffectWithDecl<SkinnedNormalMapEffect>>(device, [=](SkinnedNormalMapEffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetTexture(m_brickDiffuse.Get());
            effect->SetNormalTexture(m_brickNormal.Get());
            effect->SetSpecularTexture(m_brickSpecular.Get());
            effect->SetFogEnabled(true);
            effect->SetFogColor(Colors::Black);
        }));

    //--- PBREffect ------------------------------------------------------------------------
    if (m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        m_radianceIBL->GetDesc(&desc);

        m_pbr.emplace_back(std::make_unique<EffectWithDecl<PBREffect>>(device, [=](PBREffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetConstantAlbedo(Colors::Cyan);
            effect->SetConstantMetallic(0.5f);
            effect->SetConstantRoughness(0.75f);
            effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
        }));

        m_pbrInstanced.emplace_back(std::make_unique<InstancedEffectWithDecl<PBREffect>>(device, [=](PBREffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetConstantAlbedo(Colors::Cyan);
            effect->SetConstantMetallic(0.5f);
            effect->SetConstantRoughness(0.75f);
            effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
        }));

        // PBREffect (textured)
        m_pbr.emplace_back(std::make_unique<EffectWithDecl<PBREffect>>(device, [=](PBREffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetSurfaceTextures(m_pbrAlbedo.Get(), m_pbrNormal.Get(), m_pbrRMA.Get());
            effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
        }));

        m_pbrInstanced.emplace_back(std::make_unique<InstancedEffectWithDecl<PBREffect>>(device, [=](PBREffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetSurfaceTextures(m_pbrAlbedo.Get(), m_pbrNormal.Get(), m_pbrRMA.Get());
            effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
        }));

        // PBREffect (emissive)
        m_pbr.emplace_back(std::make_unique<EffectWithDecl<PBREffect>>(device, [=](PBREffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetSurfaceTextures(m_pbrAlbedo.Get(), m_pbrNormal.Get(), m_pbrRMA.Get());
            effect->SetEmissiveTexture(m_pbrEmissive.Get());
            effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
        }));

        m_pbrInstanced.emplace_back(std::make_unique<InstancedEffectWithDecl<PBREffect>>(device, [=](PBREffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetSurfaceTextures(m_pbrAlbedo.Get(), m_pbrNormal.Get(), m_pbrRMA.Get());
            effect->SetEmissiveTexture(m_pbrEmissive.Get());
            effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
        }));

        // PBREffect (velocity)
        m_pbr.emplace_back(std::make_unique<EffectWithDecl<PBREffect>>(device, [=](PBREffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetSurfaceTextures(m_pbrAlbedo.Get(), m_pbrNormal.Get(), m_pbrRMA.Get());
            effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
            effect->SetVelocityGeneration(true);
        }));

        // PBREffect (velocity + emissive)
        m_pbr.emplace_back(std::make_unique<EffectWithDecl<PBREffect>>(device, [=](PBREffect* effect)
        {
            effect->EnableDefaultLighting();
            effect->SetSurfaceTextures(m_pbrAlbedo.Get(), m_pbrNormal.Get(), m_pbrRMA.Get());
            effect->SetEmissiveTexture(m_pbrEmissive.Get());
            effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
            effect->SetVelocityGeneration(true);
        }));
    }

    //--- SkinnedPBREffect -----------------------------------------------------------------
    if (m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        m_radianceIBL->GetDesc(&desc);

        m_skinningPbr.emplace_back(std::make_unique<EffectWithDecl<SkinnedPBREffect>>(device, [=](SkinnedPBREffect* effect)
            {
                effect->EnableDefaultLighting();
                effect->SetConstantAlbedo(Colors::Cyan);
                effect->SetConstantMetallic(0.5f);
                effect->SetConstantRoughness(0.75f);
                effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
            }));

        // SkinnedPBREffect (textured)
        m_skinningPbr.emplace_back(std::make_unique<EffectWithDecl<SkinnedPBREffect>>(device, [=](SkinnedPBREffect* effect)
            {
                effect->EnableDefaultLighting();
                effect->SetSurfaceTextures(m_pbrAlbedo.Get(), m_pbrNormal.Get(), m_pbrRMA.Get());
                effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
            }));

        // SkinnedPBREffect (emissive)
        m_skinningPbr.emplace_back(std::make_unique<EffectWithDecl<SkinnedPBREffect>>(device, [=](SkinnedPBREffect* effect)
            {
                effect->EnableDefaultLighting();
                effect->SetSurfaceTextures(m_pbrAlbedo.Get(), m_pbrNormal.Get(), m_pbrRMA.Get());
                effect->SetEmissiveTexture(m_pbrEmissive.Get());
                effect->SetIBLTextures(m_radianceIBL.Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL.Get());
            }));
    }

    //--- DebugEffect ----------------------------------------------------------------------
    m_debug.emplace_back(std::make_unique<EffectWithDecl<DebugEffect>>(device, [=](DebugEffect*)
    {
    }));

    m_debugInstanced.emplace_back(std::make_unique<InstancedEffectWithDecl<DebugEffect>>(device, [=](DebugEffect*)
    {
    }));

    m_debug.emplace_back(std::make_unique<EffectWithDecl<DebugEffect>>(device, [=](DebugEffect* effect)
    {
        effect->SetMode(DebugEffect::Mode_Normals);
    }));

    m_debug.emplace_back(std::make_unique<EffectWithDecl<DebugEffect>>(device, [=](DebugEffect* effect)
    {
        effect->SetMode(DebugEffect::Mode_Tangents);
    }));

    m_debug.emplace_back(std::make_unique<EffectWithDecl<DebugEffect>>(device, [=](DebugEffect* effect)
    {
        effect->SetMode(DebugEffect::Mode_BiTangents);
    }));

    m_debug.emplace_back(std::make_unique<EffectWithDecl<DebugEffect>>(device, [=](DebugEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
    }));

    m_debugInstanced.emplace_back(std::make_unique<InstancedEffectWithDecl<DebugEffect>>(device, [=](DebugEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
    }));

    m_debug.emplace_back(std::make_unique<EffectWithDecl<DebugEffect>>(device, [=](DebugEffect* effect)
    {
        effect->SetMode(DebugEffect::Mode_Normals);
        effect->SetVertexColorEnabled(true);
    }));

    m_debug.emplace_back(std::make_unique<EffectWithDecl<DebugEffect>>(device, [=](DebugEffect* effect)
    {
        effect->SetMode(DebugEffect::Mode_Tangents);
        effect->SetVertexColorEnabled(true);
    }));

    m_debug.emplace_back(std::make_unique<EffectWithDecl<DebugEffect>>(device, [=](DebugEffect* effect)
    {
        effect->SetMode(DebugEffect::Mode_BiTangents);
        effect->SetVertexColorEnabled(true);
    }));

    //--- DGSLEffect -----------------------------------------------------------------------

    // DGSLEffect
    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect*)
    {
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->EnableDefaultLighting();
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetSpecularColor(Colors::Blue);
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->EnableDefaultLighting();
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->EnableDefaultLighting();
        effect->SetSpecularColor(Colors::Blue);
    }));


    // DGSLEffect (textured)
    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_cat.Get());
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_cat.Get());
        effect->EnableDefaultLighting();
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_cat.Get());
        effect->EnableDefaultLighting();
        effect->SetSpecularColor(Colors::Blue);
    }));

    // DGSLEffect (alpha discard)
    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetAlphaDiscardEnable(true);
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetAlphaDiscardEnable(true);
        effect->EnableDefaultLighting();
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetAlphaDiscardEnable(true);
        effect->EnableDefaultLighting();
        effect->SetSpecularColor(Colors::Blue);
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetAlphaDiscardEnable(true);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_cat.Get());
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetAlphaDiscardEnable(true);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_cat.Get());
        effect->EnableDefaultLighting();
    }));

    m_dgsl.emplace_back(std::make_unique<DGSLEffectWithDecl<DGSLEffect>>(device, [=](DGSLEffect* effect)
    {
        effect->SetAlphaDiscardEnable(true);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_cat.Get());
        effect->EnableDefaultLighting();
        effect->SetSpecularColor(Colors::Blue);
    }));

    // DGSLEffect (skinning)
    m_dgslSkinned.emplace_back(std::make_unique<DGSLEffectWithDecl<SkinnedDGSLEffect>>(device, [=](SkinnedDGSLEffect* effect)
    {
        effect->SetWeightsPerVertex(4);
    }));

    m_dgslSkinned.emplace_back(std::make_unique<DGSLEffectWithDecl<SkinnedDGSLEffect>>(device, [=](SkinnedDGSLEffect* effect)
    {
        effect->SetWeightsPerVertex(2);
    }));

    m_dgslSkinned.emplace_back(std::make_unique<DGSLEffectWithDecl<SkinnedDGSLEffect>>(device, [=](SkinnedDGSLEffect* effect)
    {
        effect->SetWeightsPerVertex(1);
    }));

    m_dgslSkinned.emplace_back(std::make_unique<DGSLEffectWithDecl<SkinnedDGSLEffect>>(device, [=](SkinnedDGSLEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->SetWeightsPerVertex(4);
    }));

    m_dgslSkinned.emplace_back(std::make_unique<DGSLEffectWithDecl<SkinnedDGSLEffect>>(device, [=](SkinnedDGSLEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->SetWeightsPerVertex(2);
    }));

    m_dgslSkinned.emplace_back(std::make_unique<DGSLEffectWithDecl<SkinnedDGSLEffect>>(device, [=](SkinnedDGSLEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
        effect->SetWeightsPerVertex(1);
    }));

    if (m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
    {
        m_velocityBuffer->SetDevice(device);
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    if (m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
    {
        auto size = m_deviceResources->GetOutputSize();
        m_velocityBuffer->SetWindow(size);
    }

    m_projection = XMMatrixOrthographicRH(ortho_width * 2.f, ortho_height * 2.f, 0.1f, 10.f);

#ifdef UWP
    {
        auto orient3d = m_deviceResources->GetOrientationTransform3D();
        XMMATRIX orient = XMLoadFloat4x4(&orient3d);
        m_projection *= orient;
    }
#endif
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_states.reset();

    m_basic.clear();
    m_skinning.clear();
    m_envmap.clear();
    m_dual.clear();
    m_alphTest.clear();
    m_normalMap.clear();
    m_skinningNormalMap.clear();
    m_pbr.clear();
    m_skinningPbr.clear();
    m_debug.clear();
    m_dgsl.clear();
    m_dgslSkinned.clear();

    m_normalMapInstanced.clear();
    m_pbrInstanced.clear();
    m_debugInstanced.clear();

    m_cat.Reset();
    m_cubemap.Reset();
    m_envball.Reset();
    m_envdual.Reset();
    m_overlay.Reset();
    m_defaultTex.Reset();
    m_brickDiffuse.Reset();
    m_brickNormal.Reset();
    m_brickSpecular.Reset();
    m_pbrAlbedo.Reset();
    m_pbrNormal.Reset();
    m_pbrRMA.Reset();
    m_pbrEmissive.Reset();
    m_radianceIBL.Reset();
    m_irradianceIBL.Reset();

    m_vertexBuffer.Reset();
    m_instancedVB.Reset();
    m_compressedVB.Reset();
    m_indexBuffer.Reset();

    m_velocityBuffer->ReleaseDevice();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion

void Game::CycleRenderMode()
{
    m_renderMode += 1;

    if (m_renderMode >= Render_Max)
    {
        m_renderMode = Render_Normal;
    }
}
