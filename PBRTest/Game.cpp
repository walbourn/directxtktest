//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK ?
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"
#include "vbo.h"

#pragma warning(disable : 4238)

//#define USE_FAST_SEMANTICS

// Build for LH vs. RH coords
//#define LH_COORDS

// For UWP/PC, this tests using a linear F16 swapchain intead of HDR10
//#define TEST_HDR_LINEAR

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const float col0 = -4.25f;
    const float col1 = -3.f;
    const float col2 = -1.75f;
    const float col3 = -.6f;
    const float col4 = .6f;
    const float col5 = 1.75f;
    const float col6 = 3.f;
    const float col7 = 4.25f;

    const float row0 = 2.f;
    const float row1 = 0.25f;
    const float row2 = -1.1f;
    const float row3 = -2.5f;

    struct TestVertex
    {
        TestVertex() = default;

        TestVertex(XMFLOAT3 const& position,
            XMFLOAT3 const& normal,
            XMFLOAT2 const& textureCoordinate,
            XMFLOAT3 const& tangent)
            : position(position),
            normal(normal),
            textureCoordinate(textureCoordinate),
            tangent(tangent)
        { }

        TestVertex(FXMVECTOR position,
            FXMVECTOR normal,
            CXMVECTOR textureCoordinate,
            FXMVECTOR tangent)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
            XMStoreFloat3(&this->tangent, tangent);
        }

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 textureCoordinate;
        XMFLOAT3 tangent;

        static const int InputElementCount = 4;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

    const D3D11_INPUT_ELEMENT_DESC TestVertex::InputElements[] =
    {
        { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    static_assert(sizeof(TestVertex) == 44, "Vertex struct/layout mismatch");

    struct aligned_deleter { void operator()(void* p) { _aligned_free(p); } };

    // Helper for computing tangents (see DirectXMesh <http://go.microsoft.com/fwlink/?LinkID=324981>)
    void ComputeTangents(const std::vector<uint16_t>& indices, std::vector<TestVertex>& vertices)
    {
        static const float EPSILON = 0.0001f;
        static const XMVECTORF32 s_flips = { 1.f, -1.f, -1.f, 1.f };

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
                throw std::exception("ComputeTangents");
            }

            XMVECTOR t0 = XMLoadFloat2(&vertices[i0].textureCoordinate);
            XMVECTOR t1 = XMLoadFloat2(&vertices[i1].textureCoordinate);
            XMVECTOR t2 = XMLoadFloat2(&vertices[i2].textureCoordinate);

            XMVECTOR s = XMVectorMergeXY(t1 - t0, t2 - t0);

            XMFLOAT4A tmp;
            XMStoreFloat4A(&tmp, s);

            float d = tmp.x * tmp.w - tmp.z * tmp.y;
            d = (fabsf(d) <= EPSILON) ? 1.f : (1.f / d);
            s *= d;
            s = XMVectorMultiply(s, s_flips);

            XMMATRIX m0;
            m0.r[0] = XMVectorPermute<3, 2, 6, 7>(s, g_XMZero);
            m0.r[1] = XMVectorPermute<1, 0, 4, 5>(s, g_XMZero);
            m0.r[2] = m0.r[3] = g_XMZero;

            XMVECTOR p0 = XMLoadFloat3(&vertices[i0].position);
            XMVECTOR p1 = XMLoadFloat3(&vertices[i1].position);
            XMVECTOR p2 = XMLoadFloat3(&vertices[i2].position);

            XMMATRIX m1;
            m1.r[0] = p1 - p0;
            m1.r[1] = p2 - p0;
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
            XMVECTOR b1 = tan1 - XMVector3Dot(b0, tan1) * b0;
            b1 = XMVector3Normalize(b1);

            XMVECTOR tan2 = tangent2[j];
            XMVECTOR b2 = tan2 - XMVector3Dot(b0, tan2) * b0 - XMVector3Dot(b1, tan2) * b1;
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

    // Helper for creating a D3D vertex or index buffer.
    template<typename T>
    void CreateBuffer(_In_ ID3D11Device* device, T const& data, D3D11_BIND_FLAG bindFlags, _Out_ ID3D11Buffer** pBuffer)
    {
        D3D11_BUFFER_DESC bufferDesc = {};

        bufferDesc.ByteWidth = (UINT)data.size() * sizeof(T::value_type);
        bufferDesc.BindFlags = bindFlags;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;

        D3D11_SUBRESOURCE_DATA dataDesc = {};

        dataDesc.pSysMem = data.data();

        HRESULT hr = device->CreateBuffer(&bufferDesc, &dataDesc, pBuffer);
        DX::ThrowIfFailed(hr);

        assert(pBuffer != 0);
        _Analysis_assume_(pBuffer != 0);
    }

    // Helper for creating a D3D input layout.
    void CreateInputLayout(_In_ ID3D11Device* device, IEffect* effect, _Out_ ID3D11InputLayout** pInputLayout)
    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        HRESULT hr = device->CreateInputLayout(TestVertex::InputElements,
            TestVertex::InputElementCount,
            shaderByteCode, byteCodeLength,
            pInputLayout);
        DX::ThrowIfFailed(hr);

        assert(pInputLayout != 0);
        _Analysis_assume_(pInputLayout != 0);
    }

    void ReadVBO(_In_z_ const wchar_t* name, std::vector<VertexPositionNormalTexture>& vertices, std::vector<uint16_t>& indices)
    {
        std::vector<uint8_t> blob;
        {
            std::ifstream inFile(name, std::ios::in | std::ios::binary | std::ios::ate);

            if (!inFile)
                throw std::exception("ReadVBO");

            std::streampos len = inFile.tellg();
            if (!inFile)
                throw std::exception("ReadVBO");

            if (len < sizeof(VBO::header_t))
                throw std::exception("ReadVBO");

            blob.resize(size_t(len));

            inFile.seekg(0, std::ios::beg);
            if (!inFile)
                throw std::exception("ReadVBO");

            inFile.read(reinterpret_cast<char*>(blob.data()), len);
            if (!inFile)
                throw std::exception("ReadVBO");

            inFile.close();
        }

        auto hdr = reinterpret_cast<const VBO::header_t*>(blob.data());

        if (!hdr->numIndices || !hdr->numIndices)
            throw std::exception("ReadVBO");

        static_assert(sizeof(VertexPositionNormalTexture) == 32, "VBO vertex size mismatch");

        size_t vertSize = sizeof(VertexPositionNormalTexture) * hdr->numVertices;
        if (blob.size() < (vertSize + sizeof(VBO::header_t)))
            throw std::exception("End of file");

        size_t indexSize = sizeof(uint16_t) * hdr->numIndices;
        if (blob.size() < (sizeof(VBO::header_t) + vertSize + indexSize))
            throw std::exception("End of file");

        vertices.resize(hdr->numVertices);
        auto verts = reinterpret_cast<const VertexPositionNormalTexture*>(blob.data() + sizeof(VBO::header_t));
        memcpy_s(vertices.data(), vertices.size() * sizeof(VertexPositionNormalTexture), verts, vertSize);

        indices.resize(hdr->numIndices);
        auto tris = reinterpret_cast<const uint16_t*>(blob.data() + sizeof(VBO::header_t) + vertSize);
        memcpy_s(indices.data(), indices.size() * sizeof(uint16_t), tris, indexSize);
    }
}

// Constructor.
Game::Game() :
    m_ibl(0),
    m_spinning(true),
    m_pitch(0),
    m_yaw(0)
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        | DX::DeviceResources::c_EnableHDR);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(
#if defined(TEST_HDR_LINEAR)
        DXGI_FORMAT_R16G16B16A16_FLOAT,
#else
        DXGI_FORMAT_R10G10B10A2_UNORM,
#endif
        DXGI_FORMAT_D32_FLOAT, 2,
        D3D_FEATURE_LEVEL_10_0,
        DX::DeviceResources::c_EnableHDR);
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    m_deviceResources->RegisterDeviceNotify(this);
#endif

    // Set up for HDR rendering.
    m_hdrScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
    HWND window,
#else
    IUnknown* window,
#endif
    int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();

#if defined(_XBOX_ONE) && defined(_TITLE)
    UNREFERENCED_PARAMETER(rotation);
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    m_deviceResources->SetWindow(window);
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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
void Game::Update(DX::StepTimer const&)
{
    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitGame();
        }

        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED
            || m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            ++m_ibl;
            if (m_ibl >= s_nIBL)
            {
                m_ibl = 0;
            }
        }
        else if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED
                 || m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            if (!m_ibl)
                m_ibl = s_nIBL - 1;
            else
                --m_ibl;
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_spinning = !m_spinning;
        }

        if (pad.IsLeftStickPressed())
        {
            m_spinning = false;
            m_yaw = m_pitch = 0.f;
        }
        else
        {
            m_yaw += pad.thumbSticks.leftX * 0.1f;
            m_pitch -= pad.thumbSticks.leftY * 0.1f;
        }
    }
    else
    {
        m_gamePadButtons.Reset();

        if (kb.A || kb.D)
        {
            m_spinning = false;
            m_yaw += (kb.D ? 0.1f : -0.1f);
        }

        if (kb.W || kb.S)
        {
            m_spinning = false;
            m_pitch += (kb.W ? 0.1f : -0.1f);
        }

        if (kb.Home)
        {
            m_spinning = false;
            m_yaw = m_pitch = 0.f;
        }
    }

    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }

    if (kb.Escape)
    {
        ExitGame();
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Enter) && !kb.LeftAlt && !kb.RightAlt)
    {
        ++m_ibl;
        if (m_ibl >= s_nIBL)
        {
            m_ibl = 0;
        }
    }
    else if (m_keyboardButtons.IsKeyPressed(Keyboard::Back))
    {
        if (!m_ibl)
            m_ibl = s_nIBL - 1;
        else
            --m_ibl;
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
    {
        m_spinning = !m_spinning;
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

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources->Prepare();
#endif

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    // Time-based animation
    float time = static_cast<float>(m_timer.GetTotalSeconds());

    float alphaFade = (sin(time * 2) + 1) / 2;

    if (alphaFade >= 1)
        alphaFade = 1 - FLT_EPSILON;

    float yaw = time * 0.4f;
    float pitch = time * 0.7f;
    float roll = time * 1.1f;

    XMMATRIX world;
    
    if (m_spinning)
    {
        world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
    }
    else
    {
        world = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0);
    }

    // Setup for object drawing.
    UINT vertexStride = sizeof(TestVertex);
    UINT vertexOffset = 0;

    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);

    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->OMSetBlendState(m_states->AlphaBlend(), Colors::White, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
#ifdef LH_COORDS
    context->RSSetState(m_states->CullClockwise());
#else
    context->RSSetState(m_states->CullCounterClockwise());
#endif

    ID3D11SamplerState* samplers[] =
    {
        m_states->AnisotropicClamp(),
        m_states->LinearWrap(),
    };

    context->PSSetSamplers(0, 2, samplers);

    //--- NormalMap ------------------------------------------------------------------------
    context->IASetInputLayout(m_inputLayoutNM.Get());

    m_normalMapEffect->SetWorld(world * XMMatrixTranslation(col0, row0, 0));
    m_normalMapEffect->SetTexture(m_baseColor[0].Get());
    m_normalMapEffect->SetNormalTexture(m_normalMap[0].Get());
    m_normalMapEffect->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_normalMapEffect->SetWorld(world * XMMatrixTranslation(col3, row0, 0));
    m_normalMapEffect->SetTexture(m_baseColor[1].Get());
    m_normalMapEffect->SetNormalTexture(m_normalMap[1].Get());
    m_normalMapEffect->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    //--- PBREffect (basic) ----------------------------------------------------------------
    context->IASetInputLayout(m_inputLayoutPBR.Get());

    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    m_radianceIBL[m_ibl]->GetDesc(&desc);

    m_pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), desc.TextureCube.MipLevels, m_irradianceIBL[m_ibl].Get());
    m_pbrCube->SetIBLTextures(m_radianceIBL[m_ibl].Get(), desc.TextureCube.MipLevels, m_irradianceIBL[m_ibl].Get());

    m_pbr->SetAlpha(1.f);
    m_pbr->SetWorld(world * XMMatrixTranslation(col1, row0, 0));
    m_pbr->SetSurfaceTextures(m_baseColor[0].Get(), m_normalMap[0].Get(), m_rma[0].Get());
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetAlpha(alphaFade);
    m_pbr->SetWorld(world * XMMatrixTranslation(col2, row0, 0));
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    context->IASetVertexBuffers(0, 1, m_vertexBufferCube.GetAddressOf(), &vertexStride, &vertexOffset);
    context->IASetIndexBuffer(m_indexBufferCube.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetInputLayout(m_inputLayoutCube.Get());
    m_pbrCube->SetWorld(world * XMMatrixTranslation(col7, row0, 0));
    m_pbrCube->Apply(context);
    context->DrawIndexed(m_indexCountCube, 0, 0);

    //--- PBREffect (emissive) -------------------------------------------------------------
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetInputLayout(m_inputLayoutPBR.Get());

    m_pbr->SetEmissiveTexture(m_emissiveMap[1].Get());

    m_pbr->SetAlpha(1.f);
    m_pbr->SetWorld(world * XMMatrixTranslation(col4, row0, 0));
    m_pbr->SetSurfaceTextures(m_baseColor[1].Get(), m_normalMap[1].Get(), m_rma[1].Get());
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetAlpha(alphaFade);
    m_pbr->SetWorld(world * XMMatrixTranslation(col5, row0, 0));
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    //--- PBREffect (constant) -------------------------------------------------------------   

    m_pbr->SetAlpha(1.f);
    m_pbr->SetSurfaceTextures(nullptr, nullptr, nullptr);
    m_pbr->SetEmissiveTexture(nullptr);
    m_pbr->SetWorld(world * XMMatrixTranslation(col0, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Blue);
    m_pbr->SetConstantMetallic(1.f);
    m_pbr->SetConstantRoughness(0.2f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col1, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Green);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col2, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Red);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col3, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Yellow);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col4, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Cyan);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col5, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Magenta);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col6, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Black);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col7, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::White);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Row 2
    m_pbr->SetAlpha(alphaFade);
    m_pbr->SetWorld(world * XMMatrixTranslation(col0, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Blue);
    m_pbr->SetConstantMetallic(0.f);
    m_pbr->SetConstantRoughness(0.2f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col1, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Green);
    m_pbr->SetConstantMetallic(0.25f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col2, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Red);
    m_pbr->SetConstantMetallic(0.5f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col3, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Yellow);
    m_pbr->SetConstantMetallic(0.75f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col4, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Cyan);
    m_pbr->SetConstantMetallic(1.f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col5, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Magenta);
    m_pbr->SetConstantMetallic(0.5f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col6, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Black);
    m_pbr->SetConstantMetallic(0.75f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col7, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::White);
    m_pbr->SetConstantMetallic(0.8f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Row3
    m_pbr->SetAlpha(1.f);
    m_pbr->SetWorld(world * XMMatrixTranslation(col0, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Blue);
    m_pbr->SetConstantMetallic(0.5f);
    m_pbr->SetConstantRoughness(0.0f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col1, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Green);
    m_pbr->SetConstantRoughness(0.25f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col2, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Red);
    m_pbr->SetConstantRoughness(0.5f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col3, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Yellow);
    m_pbr->SetConstantRoughness(0.75f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col4, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Cyan);
    m_pbr->SetConstantRoughness(1.0f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col5, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Magenta);
    m_pbr->SetConstantRoughness(0.2f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col6, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Black);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col7, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::White);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Tonemap the frame.
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_hdrScene->EndScene(context);
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
    ID3D11RenderTargetView* renderTargets[2] = { m_deviceResources->GetRenderTargetView(), m_deviceResources->GetGameDVRRenderTargetView() };
    context->OMSetRenderTargets(2, renderTargets, nullptr);
#else
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    switch (m_deviceResources->GetColorSpace())
    {
    default:
        m_toneMap->SetOperator(ToneMapPostProcess::ACESFilmic);
        m_toneMap->SetTransferFunction((m_deviceResources->GetBackBufferFormat() == DXGI_FORMAT_R16G16B16A16_FLOAT) ? ToneMapPostProcess::Linear : ToneMapPostProcess::SRGB);
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        m_toneMap->SetOperator(ToneMapPostProcess::None);
        m_toneMap->SetTransferFunction(ToneMapPostProcess::ST2084);
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
        m_toneMap->SetOperator(ToneMapPostProcess::None);
        m_toneMap->SetTransferFunction(ToneMapPostProcess::Linear);
        break;
    }
#endif

    m_toneMap->Process(context);

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);

    // Show the new frame.
    m_deviceResources->Present();

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_graphicsMemory->Commit();
#endif
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_hdrScene->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    XMVECTORF32 color;
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
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
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation)
{
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device, m_deviceResources->GetBackBufferCount());
#endif

    m_states = std::make_unique<CommonStates>(device);

    m_hdrScene->SetDevice(device);

    m_normalMapEffect = std::make_unique<NormalMapEffect>(device);
    m_normalMapEffect->EnableDefaultLighting();

    m_pbr = std::make_unique<PBREffect>(device);
    m_pbr->EnableDefaultLighting();

    m_pbrCube = std::make_unique<PBREffect>(device);
    m_pbrCube->EnableDefaultLighting();

    m_toneMap = std::make_unique<ToneMapPostProcess>(device);
    m_toneMap->SetOperator(ToneMapPostProcess::ACESFilmic);
    m_toneMap->SetTransferFunction(ToneMapPostProcess::SRGB);

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_toneMap->SetMRTOutput(true);
#endif

    // Create test geometry with tangent frame
    {
        std::vector<GeometricPrimitive::VertexType> origVerts;
        std::vector<uint16_t> indices;

        GeometricPrimitive::CreateSphere(origVerts, indices);

        std::vector<TestVertex> vertices;
        vertices.reserve(origVerts.size());

        for (auto it = origVerts.cbegin(); it != origVerts.cend(); ++it)
        {
            TestVertex v;
            v.position = it->position;
            v.normal = it->normal;
            v.textureCoordinate = it->textureCoordinate;
            vertices.emplace_back(v);
        }

        assert(origVerts.size() == vertices.size());

        ComputeTangents(indices, vertices);

        // Create the D3D buffers.
        if (vertices.size() >= USHRT_MAX)
            throw std::exception("Too many vertices for 16-bit index buffer");

        CreateInputLayout(device, m_normalMapEffect.get(), m_inputLayoutNM.ReleaseAndGetAddressOf());

        CreateInputLayout(device, m_pbr.get(), m_inputLayoutPBR.ReleaseAndGetAddressOf());
        
        CreateBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, m_indexBuffer.ReleaseAndGetAddressOf());

        CreateBuffer(device, vertices, D3D11_BIND_VERTEX_BUFFER, m_vertexBuffer.ReleaseAndGetAddressOf());

        // Record index count for draw
        m_indexCount = static_cast<UINT>(indices.size());
    }

    // Create cube geometry with tangent frame
    {
        std::vector<GeometricPrimitive::VertexType> origVerts;
        std::vector<uint16_t> indices;

        ReadVBO(L"BrokenCube.vbo", origVerts, indices);

        std::vector<TestVertex> vertices;
        vertices.reserve(origVerts.size());

        for (auto it = origVerts.cbegin(); it != origVerts.cend(); ++it)
        {
            TestVertex v;
            v.position = it->position;
            v.normal = it->normal;
            v.textureCoordinate = it->textureCoordinate;
            vertices.emplace_back(v);
        }

        assert(origVerts.size() == vertices.size());

        ComputeTangents(indices, vertices);

        // Create the D3D buffers.
        if (vertices.size() >= USHRT_MAX)
            throw std::exception("Too many vertices for 16-bit index buffer");

        CreateInputLayout(device, m_pbrCube.get(), m_inputLayoutCube.ReleaseAndGetAddressOf());

        CreateBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, m_indexBufferCube.ReleaseAndGetAddressOf());

        CreateBuffer(device, vertices, D3D11_BIND_VERTEX_BUFFER, m_vertexBufferCube.ReleaseAndGetAddressOf());

        // Record index count for draw
        m_indexCountCube = static_cast<UINT>(indices.size());
    }

    // Load textures
    static const wchar_t* s_albetoTextures[s_nMaterials] =
    {
        L"SphereMat_baseColor.png",
        L"Sphere2Mat_baseColor.png",
        L"BrokenCube_baseColor.png",
    };
    static const wchar_t* s_normalMapTextures[s_nMaterials] =
    {
        L"SphereMat_normal.png",
        L"Sphere2Mat_normal.png",
        L"BrokenCube_normal.png",
    };
    static const wchar_t* s_rmaTextures[s_nMaterials] =
    {
        L"SphereMat_occlusionRoughnessMetallic.png",
        L"Sphere2Mat_occlusionRoughnessMetallic.png",
        L"BrokenCube_occlusionRoughnessMetallic.png",
    };
    static const wchar_t* s_emissiveTextures[s_nMaterials] =
    {
        nullptr,
        L"Sphere2Mat_emissive.png",
        L"BrokenCube_emissive.png",
    };

    static_assert(_countof(s_albetoTextures) == _countof(s_normalMapTextures), "Material array mismatch");
    static_assert(_countof(s_albetoTextures) == _countof(s_rmaTextures), "Material array mismatch");
    static_assert(_countof(s_albetoTextures) == _countof(s_emissiveTextures), "Material array mismatch");

    for (size_t j = 0; j < s_nMaterials; ++j)
    {
        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, s_albetoTextures[j], nullptr, m_baseColor[j].ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, s_normalMapTextures[j], nullptr, m_normalMap[j].ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, s_rmaTextures[j], nullptr, m_rma[j].ReleaseAndGetAddressOf())
        );

        if (s_emissiveTextures[j])
        {
            DX::ThrowIfFailed(
                CreateWICTextureFromFile(device, s_emissiveTextures[j], nullptr, m_emissiveMap[j].ReleaseAndGetAddressOf())
            );
        }
    }

    m_pbrCube->SetSurfaceTextures(m_baseColor[2].Get(), m_normalMap[2].Get(), m_rma[2].Get());
    m_pbrCube->SetEmissiveTexture(m_emissiveMap[2].Get());

    static const wchar_t* s_radianceIBL[s_nIBL] =
    {
        L"Atrium_diffuseIBL.dds",
        L"Garage_diffuseIBL.dds",
        L"SunSubMixer_diffuseIBL.dds",
    };
    static const wchar_t* s_irradianceIBL[s_nIBL] =
    {
        L"Atrium_specularIBL.dds",
        L"Garage_specularIBL.dds",
        L"SunSubMixer_specularIBL.dds",
    };

    static_assert(_countof(s_radianceIBL) == _countof(s_irradianceIBL), "IBL array mismatch");

    for (size_t j = 0; j < s_nIBL; ++j)
    {
        DX::ThrowIfFailed(
            CreateDDSTextureFromFile(device, s_radianceIBL[j], nullptr, m_radianceIBL[j].ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateDDSTextureFromFile(device, s_irradianceIBL[j], nullptr, m_irradianceIBL[j].ReleaseAndGetAddressOf())
        );
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { 0, 0, 6 };

    auto size = m_deviceResources->GetOutputSize();
    float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    XMMATRIX view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    XMMATRIX projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 15);
#else
    XMMATRIX view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    XMMATRIX projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 15);
#endif

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    XMMATRIX orient = XMLoadFloat4x4(&m_deviceResources->GetOrientationTransform3D());
    projection *= orient;
#endif

    m_normalMapEffect->SetView(view);
    m_pbr->SetView(view);
    m_pbrCube->SetView(view);

    m_normalMapEffect->SetProjection(projection);
    m_pbr->SetProjection(projection);
    m_pbrCube->SetProjection(projection);

    // Set windows size for HDR.
    m_hdrScene->SetWindow(size);

    m_toneMap->SetHDRSourceTexture(m_hdrScene->GetShaderResourceView());
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnDeviceLost()
{
    m_states.reset();

    for (size_t j = 0; j < s_nMaterials; ++j)
    {
        m_baseColor[j].Reset();
        m_normalMap[j].Reset();
        m_rma[j].Reset();
        m_emissiveMap[j].Reset();
    }

    for (size_t j = 0; j < s_nIBL; ++j)
    {
        m_radianceIBL[j].Reset();
        m_irradianceIBL[j].Reset();
    }

    m_normalMapEffect.reset();
    m_pbr.reset();
    m_pbrCube.reset();

    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
    m_inputLayoutNM.Reset();
    m_inputLayoutPBR.Reset();

    m_vertexBufferCube.Reset();
    m_indexBufferCube.Reset();
    m_inputLayoutCube.Reset();

    m_toneMap.reset();

    m_hdrScene->ReleaseDevice();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
