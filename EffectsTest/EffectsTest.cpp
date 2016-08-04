//--------------------------------------------------------------------------------------
// File: EffectsTest.cpp
//
// Developer unit test for DirectXTK Effects
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

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Effects.h"
#include "CommonStates.h"
#include "DirectXColors.h"
#include "DirectXPackedVector.h"
#include "DDSTextureLoader.h"
#include "ScreenGrab.h"
#include "Bezier.h"

#include <vector>
#include <functional>

#include <wrl/client.h>

#include <wincodec.h>

using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

// Build for LH vs. RH coords
//#define LH_COORDS

struct TestVertex
{
    TestVertex(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate)
    {
        XMStoreFloat3(&this->position, position);
        XMStoreFloat3(&this->normal, normal);
        XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
        XMStoreFloat2(&this->textureCoordinate2, textureCoordinate * 3);
        XMStoreUByte4(&this->blendIndices, XMVectorSet(0, 1, 2, 3));

        XMStoreFloat3(&this->tangent, g_XMZero);

        float u = XMVectorGetX(textureCoordinate) - 0.5f;
        float v = XMVectorGetY(textureCoordinate) - 0.5f;

        float d = 1 - sqrt(u * u + v * v) * 2;

        if (d < 0)
            d = 0;

        XMStoreFloat4(&this->blendWeight, XMVectorSet(d, 1 - d, u, v));
    }

    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT3 tangent;
    XMFLOAT2 textureCoordinate;
    XMFLOAT2 textureCoordinate2;
    XMUBYTE4 blendIndices;
    XMFLOAT4 blendWeight;

    static const int InputElementCount = 7;
    static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
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
};


typedef std::vector<TestVertex> VertexCollection;
typedef std::vector<uint16_t> IndexCollection;


struct aligned_deleter { void operator()(void* p) { _aligned_free(p); } };


// Helper for computing tangents (see DirectXMesh <http://go.microsoft.com/fwlink/?LinkID=324981>)
void ComputeTangents(const IndexCollection& indices, VertexCollection& vertices)
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
static void CreateBuffer(_In_ ID3D11Device* device, T const& data, D3D11_BIND_FLAG bindFlags, _Out_ ID3D11Buffer** pBuffer)
{
    D3D11_BUFFER_DESC bufferDesc = {};

    bufferDesc.ByteWidth = (UINT)data.size() * sizeof(T::value_type);
    bufferDesc.BindFlags = bindFlags;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA dataDesc = {};

    dataDesc.pSysMem = data.data();

    HRESULT hr = device->CreateBuffer(&bufferDesc, &dataDesc, pBuffer);

    if (FAILED(hr))
        throw std::exception();
}


// Helper for creating a D3D input layout.
static void CreateInputLayout(_In_ ID3D11Device* device, IEffect* effect, _Out_ ID3D11InputLayout** pInputLayout)
{
    void const* shaderByteCode;
    size_t byteCodeLength;

    effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    HRESULT hr = device->CreateInputLayout(TestVertex::InputElements,
                                           TestVertex::InputElementCount,
                                           shaderByteCode, byteCodeLength,
                                           pInputLayout);

    if (FAILED(hr))
        throw std::exception();
}


namespace
{
    #include "../../Src/TeapotData.inc"
}


// Tessellates the specified bezier patch.
static void TessellatePatch(VertexCollection& vertices, IndexCollection& indices, TeapotPatch const& patch, FXMVECTOR scale, bool isMirrored)
{
    const int tessellation = 16;

    // Look up the 16 control points for this patch.
    XMVECTOR controlPoints[16];

    for (int i = 0; i < 16; i++)
    {
        controlPoints[i] = TeapotControlPoints[patch.indices[i]] * scale;
    }

    // Create the index data.
    Bezier::CreatePatchIndices(tessellation, isMirrored, [&](size_t index)
    {
        indices.push_back((uint16_t)(vertices.size() + index));
    });

    // Create the vertex data.
    Bezier::CreatePatchVertices(controlPoints, tessellation, isMirrored, [&](FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate)
    {
        vertices.push_back(TestVertex(position, normal, textureCoordinate));
    });
}


// Creates a teapot primitive.
int CreateTeapot(ID3D11Device* device, ID3D11Buffer** vertexBuffer, ID3D11Buffer** indexBuffer)
{
    VertexCollection vertices;
    IndexCollection indices;

    for (int i = 0; i < sizeof(TeapotPatches) / sizeof(TeapotPatches[0]); i++)
    {
        TeapotPatch const& patch = TeapotPatches[i];

        // Because the teapot is symmetrical from left to right, we only store
        // data for one side, then tessellate each patch twice, mirroring in X.
        TessellatePatch(vertices, indices, patch, g_XMOne, false);
        TessellatePatch(vertices, indices, patch, g_XMNegateX, true);

        if (patch.mirrorZ)
        {
            // Some parts of the teapot (the body, lid, and rim, but not the
            // handle or spout) are also symmetrical from front to back, so
            // we tessellate them four times, mirroring in Z as well as X.
            TessellatePatch(vertices, indices, patch, g_XMNegateZ, true);
            TessellatePatch(vertices, indices, patch, g_XMNegateX * g_XMNegateZ, false);
        }
    }

    // Compute tangents
    ComputeTangents(indices, vertices);

    // Create the D3D buffers.
    CreateBuffer(device, vertices, D3D11_BIND_VERTEX_BUFFER, vertexBuffer);
    CreateBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, indexBuffer);

    return (int)indices.size();
}


template<typename T>
class EffectWithDecl : public T
{
public:
    EffectWithDecl(ID3D11Device* device, std::function<void(T*)> setEffectParameters)
      : T(device)
    {
        setEffectParameters(this);

        CreateInputLayout(device, this, &inputLayout);
    }


    void Apply(ID3D11DeviceContext* context, CXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
    {
        SetMatrices(world, view, projection);

        T::Apply(context);

        context->IASetInputLayout(inputLayout.Get());
    }


private:
    ComPtr<ID3D11InputLayout> inputLayout;
};


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}


int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow)
{
    if (!XMVerifyCPUSupport())
        return 1;

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    HRESULT hr;

    wchar_t *const className = L"TestWindowClass";

    WNDCLASSEX wndClass = {};

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = className;
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wndClass);

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    RECT client;
    GetClientRect(hwnd, &client);

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = client.right;
    swapChainDesc.BufferDesc.Height = client.bottom;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_10_0;

    DWORD d3dFlags = 0;
#ifdef _DEBUG
    d3dFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapChain;
    if (FAILED(hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, d3dFlags, &featureLevel, 1,
        D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, nullptr, &context)))
        return 1;

    ComPtr<ID3D11Texture2D> backBufferTexture;
    if (FAILED(hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture)))
        return 1;

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = { DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2D };

    ComPtr<ID3D11RenderTargetView> backBuffer;
    if (FAILED(hr = device->CreateRenderTargetView(backBufferTexture.Get(), &renderTargetViewDesc, &backBuffer)))
        return 1;

    D3D11_TEXTURE2D_DESC depthStencilDesc = {};

    depthStencilDesc.Width = client.right;
    depthStencilDesc.Height = client.bottom;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;

    ComPtr<ID3D11Texture2D> depthStencilTexture;
    if (FAILED(device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilTexture)))
        return 1;

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = DXGI_FORMAT_UNKNOWN;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    ComPtr<ID3D11DepthStencilView> depthStencil;
    if (FAILED(device->CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilViewDesc, &depthStencil)))
        return 1;

    CommonStates states(device.Get());

    // Load textures.
    ComPtr<ID3D11ShaderResourceView> cat;
    ComPtr<ID3D11ShaderResourceView> opaqueCat;
    ComPtr<ID3D11ShaderResourceView> cubemap;
    ComPtr<ID3D11ShaderResourceView> overlay;
    ComPtr<ID3D11ShaderResourceView> defaultTex;
    ComPtr<ID3D11ShaderResourceView> brickDiffuse;
    ComPtr<ID3D11ShaderResourceView> brickNormal;
    ComPtr<ID3D11ShaderResourceView> brickSpecular;

    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"cat.dds", nullptr, &cat)))
        MessageBox(hwnd, L"Error loading cat.dds", L"EffectsTest", MB_ICONERROR);

    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"opaqueCat.dds", nullptr, &opaqueCat)))
        MessageBox(hwnd, L"Error loading opaqueCat.dds", L"EffectsTest", MB_ICONERROR);

    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"cubemap.dds", nullptr, &cubemap)))
        MessageBox(hwnd, L"Error loading cubemap.dds", L"EffectsTest", MB_ICONERROR);

    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"overlay.dds", nullptr, &overlay)))
        MessageBox(hwnd, L"Error loading overlay.dds", L"EffectsTest", MB_ICONERROR);

    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"default.dds", nullptr, &defaultTex)))
        MessageBox(hwnd, L"Error loading default.dds", L"EffectsTest", MB_ICONERROR);

    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"spnza_bricks_a.DDS", nullptr, &brickDiffuse)))
        MessageBox(hwnd, L"Error loading spnza_bricks_a.dds", L"EffectsTest", MB_ICONERROR);

    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"spnza_bricks_a_normal.DDS", nullptr, &brickNormal)))
        MessageBox(hwnd, L"Error loading spnza_bricks_a_normal.dds", L"EffectsTest", MB_ICONERROR);

    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"spnza_bricks_a_specular.DDS", nullptr, &brickSpecular)))
        MessageBox(hwnd, L"Error loading spnza_bricks_a_specular.dds", L"EffectsTest", MB_ICONERROR);

    // Create test geometry.
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;

    int indexCount = CreateTeapot(device.Get(), &vertexBuffer, &indexBuffer);

    // Create the shaders.
    EffectWithDecl<BasicEffect> basicEffectUnlit(device.Get(), [](BasicEffect* effect)
    {
        effect->SetDiffuseColor(Colors::Blue);
    });

    EffectWithDecl<BasicEffect> basicEffect(device.Get(), [](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(Colors::Red);
    });

    EffectWithDecl<BasicEffect> basicEffectNoSpecular(device.Get(), [](BasicEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(Colors::Red);
        effect->DisableSpecular();
    });

    EffectWithDecl<SkinnedEffect> skinnedEffect(device.Get(), [&](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(opaqueCat.Get());
    });

    EffectWithDecl<SkinnedEffect> skinnedEffectNoSpecular(device.Get(), [&](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(opaqueCat.Get());
        effect->DisableSpecular();
    });

    EffectWithDecl<EnvironmentMapEffect> envmap(device.Get(), [&](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(opaqueCat.Get());
        effect->SetEnvironmentMap(cubemap.Get());
    });

    EffectWithDecl<DualTextureEffect> dualTexture(device.Get(), [&](DualTextureEffect* effect)
    {
        effect->SetTexture(opaqueCat.Get());
        effect->SetTexture2(overlay.Get());
    });

    EffectWithDecl<AlphaTestEffect> alphaTest(device.Get(), [&](AlphaTestEffect* effect)
    {
        effect->SetTexture(cat.Get());
    });

    EffectWithDecl<NormalMapEffect> normalMapEffect(device.Get(), [&](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(Colors::White);
        effect->SetTexture(brickDiffuse.Get());
        effect->SetNormalTexture(brickNormal.Get());
        effect->SetSpecularTexture(brickSpecular.Get());
    });

    EffectWithDecl<NormalMapEffect> normalMapEffectNoDiffuse(device.Get(), [&](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(Colors::White);
        effect->SetTexture(defaultTex.Get());
        effect->SetNormalTexture(brickNormal.Get());
        effect->SetSpecularTexture(brickSpecular.Get());
    });

    EffectWithDecl<NormalMapEffect> normalMapEffectNormalsOnly(device.Get(), [&](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->DisableSpecular();
        effect->SetDiffuseColor(Colors::White);
        effect->SetTexture(defaultTex.Get());
        effect->SetNormalTexture(brickNormal.Get());
    });

    EffectWithDecl<NormalMapEffect> normalMapEffectNoSpecular(device.Get(), [&](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->DisableSpecular();
        effect->SetDiffuseColor(Colors::White);
        effect->SetTexture(brickDiffuse.Get());
        effect->SetNormalTexture(brickNormal.Get());
    });

    bool quit = false;

    D3D11_VIEWPORT vp = { 0, 0, (float)client.right, (float)client.bottom, 0, 1 };

    context->RSSetViewports(1, &vp);

    context->OMSetRenderTargets(1, backBuffer.GetAddressOf(), depthStencil.Get());

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    size_t frame = 0;

    while (!quit)
    {
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                quit = true;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        
        float time = (float)(counter.QuadPart - start.QuadPart) / (float)freq.QuadPart;

        context->ClearRenderTargetView(backBuffer.Get(), Colors::CornflowerBlue);
        context->ClearDepthStencilView(depthStencil.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

        // Set state objects.
        context->OMSetBlendState(states.AlphaBlend(), Colors::White, 0xFFFFFFFF);
        context->OMSetDepthStencilState(states.DepthDefault(), 0);
#ifdef LH_COORDS
        context->RSSetState(states.CullClockwise());
#else
        context->RSSetState(states.CullCounterClockwise());
#endif

        ID3D11SamplerState* samplers[] =
        {
            states.LinearWrap(),
            states.LinearWrap(),
        };

        context->PSSetSamplers(0, 2, samplers);

        // Set the vertex and index buffer.
        UINT vertexStride = sizeof(TestVertex);
        UINT vertexOffset = 0;

        context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);

        context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Compute camera matrices.
        float alphaFade = (sin(time * 2) + 1) / 2;

        if (alphaFade >= 1)
            alphaFade = 1 - FLT_EPSILON;

        float yaw = time * 0.4f;
        float pitch = time * 0.7f;
        float roll = time * 1.1f;

        XMVECTORF32 cameraPosition = { 0, 0, 6 };

        float aspect = (float)client.right / (float)client.bottom;

        XMMATRIX world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
#ifdef LH_COORDS
        XMMATRIX view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
        XMMATRIX projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 15);
        const float fogstart = -6;
        const float fogend = -8;
#else
        XMMATRIX view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
        XMMATRIX projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 15);
        const float fogstart = 6;
        const float fogend = 8;
#endif

        // Simple unlit teapot.
        basicEffectUnlit.Apply(context.Get(), world * XMMatrixTranslation(-4, 2.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // Unlit with alpha fading.
        basicEffectUnlit.SetAlpha(alphaFade);
        basicEffectUnlit.Apply(context.Get(), world * XMMatrixTranslation(-4, 1.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        basicEffectUnlit.SetAlpha(1);

        // Unlit with fog.
        basicEffectUnlit.SetFogEnabled(true);
        basicEffectUnlit.SetFogStart(fogstart);
        basicEffectUnlit.SetFogEnd(fogend);
        basicEffectUnlit.SetFogColor(Colors::Gray);
        basicEffectUnlit.Apply(context.Get(), world * XMMatrixTranslation(-4, 0.5f, 2 - alphaFade * 6), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        basicEffectUnlit.SetFogEnabled(false);

        // Simple lit teapot.
        basicEffect.Apply(context.Get(), world * XMMatrixTranslation(-3, 2.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // Simple lit teapot, no specular
        basicEffectNoSpecular.Apply(context.Get(), world * XMMatrixTranslation(-2, 0, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // Simple lit with alpha fading.
        basicEffect.SetAlpha(alphaFade);
        basicEffect.Apply(context.Get(), world * XMMatrixTranslation(-3, 1.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        basicEffect.SetAlpha(1);

        // Simple lit alpha fading, no specular.
        basicEffectNoSpecular.SetAlpha(alphaFade);
        basicEffectNoSpecular.Apply(context.Get(), world * XMMatrixTranslation(-1, 0, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        basicEffectNoSpecular.SetAlpha(1);

        // Simple lit with fog.
        basicEffect.SetFogEnabled(true);
        basicEffect.SetFogStart(fogstart);
        basicEffect.SetFogEnd(fogend);
        basicEffect.SetFogColor(Colors::Gray);
        basicEffect.Apply(context.Get(), world * XMMatrixTranslation(-3, 0.5f, 2 - alphaFade * 6), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        basicEffect.SetFogEnabled(false);

        basicEffect.SetLightEnabled(1, false);
        basicEffect.SetLightEnabled(2, false);

        {
            // Light only from above.
            basicEffect.SetLightDirection(0, XMVectorSet(0, -1, 0, 0));
            basicEffect.Apply(context.Get(), world * XMMatrixTranslation(-2, 2.5f, 0), view, projection);
            context->DrawIndexed(indexCount, 0, 0);

            // Light only from the left.
            basicEffect.SetLightDirection(0, XMVectorSet(1, 0, 0, 0));
            basicEffect.Apply(context.Get(), world * XMMatrixTranslation(-1, 2.5f, 0), view, projection);
            context->DrawIndexed(indexCount, 0, 0);

            // Light only from straight in front.
            basicEffect.SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
            basicEffect.Apply(context.Get(), world * XMMatrixTranslation(0, 2.5f, 0), view, projection);
            context->DrawIndexed(indexCount, 0, 0);
        }

        basicEffect.EnableDefaultLighting();

        // Non uniform scaling.
        basicEffect.Apply(context.Get(), XMMatrixScaling(1, 2, 0.25f) * world * XMMatrixTranslation(1, 2.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        basicEffect.SetPerPixelLighting(true);

        {
            basicEffect.SetLightEnabled(1, false);
            basicEffect.SetLightEnabled(2, false);

            {
                // Light only from above + per pixel lighting.
                basicEffect.SetLightDirection(0, XMVectorSet(0, -1, 0, 0));
                basicEffect.Apply(context.Get(), world * XMMatrixTranslation(-2, 1.5f, 0), view, projection);
                context->DrawIndexed(indexCount, 0, 0);

                // Light only from the left + per pixel lighting.
                basicEffect.SetLightDirection(0, XMVectorSet(1, 0, 0, 0));
                basicEffect.Apply(context.Get(), world * XMMatrixTranslation(-1, 1.5f, 0), view, projection);
                context->DrawIndexed(indexCount, 0, 0);

                // Light only from straight in front + per pixel lighting.
                basicEffect.SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
                basicEffect.Apply(context.Get(), world * XMMatrixTranslation(0, 1.5f, 0), view, projection);
                context->DrawIndexed(indexCount, 0, 0);
            }

            basicEffect.EnableDefaultLighting();

            // Non uniform scaling + per pixel lighting.
            basicEffect.Apply(context.Get(), XMMatrixScaling(1, 2, 0.25f) * world * XMMatrixTranslation(1, 1.5f, 0), view, projection);
            context->DrawIndexed(indexCount, 0, 0);
        }

        basicEffect.SetPerPixelLighting(false);

        // Skinned effect, identity transforms.
        XMMATRIX bones[4] =
        {
            XMMatrixIdentity(),
            XMMatrixIdentity(),
            XMMatrixScaling(0, 0, 0),
            XMMatrixScaling(0, 0, 0),
        };

        skinnedEffect.SetBoneTransforms(bones, 4);
        skinnedEffect.Apply(context.Get(), world, view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        skinnedEffectNoSpecular.SetBoneTransforms(bones, 4);
        skinnedEffectNoSpecular.Apply(context.Get(), world * XMMatrixTranslation(1, 0, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // Skinned effect, variable scaling transforms.
        float scales[4] =
        {
            1 + sin(time * 1.7f) * 0.5f,
            1 + sin(time * 2.3f) * 0.5f,
            0,
            0,
        };

        for (int i = 0; i < 4; i++)
        {
            bones[i] = XMMatrixScaling(scales[i], scales[i], scales[i]);
        }

        skinnedEffect.SetBoneTransforms(bones, 4);
        skinnedEffect.Apply(context.Get(), world * XMMatrixTranslation(-1, -2, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // Skinned effect, different variable scaling transforms.
        float scales2[4] =
        {
            1,
            1,
            sin(time * 2.3f) * 0.5f,
            sin(time * 3.1f) * 0.5f,
        };

        for (int i = 0; i < 4; i++)
        {
            bones[i] = XMMatrixScaling(scales2[i], scales2[i], scales2[i]);
        }

        skinnedEffect.SetBoneTransforms(bones, 4);
        skinnedEffect.Apply(context.Get(), world * XMMatrixTranslation(-3, -2, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // Environment map effect.
        envmap.Apply(context.Get(), world * XMMatrixTranslation(2, 2.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // Environment map with alpha fading.
        envmap.SetAlpha(alphaFade);
        envmap.Apply(context.Get(), world * XMMatrixTranslation(2, 1.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        envmap.SetAlpha(1);

        // Environment map with fog.
        envmap.SetFogEnabled(true);
        envmap.SetFogStart(fogstart);
        envmap.SetFogEnd(fogend);
        envmap.SetFogColor(Colors::Gray);
        envmap.Apply(context.Get(), world * XMMatrixTranslation(2, 0.5f, 2 - alphaFade * 6), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        envmap.SetFogEnabled(false);

        // Environment map, animating the fresnel factor.
        envmap.SetFresnelFactor(alphaFade * 3);
        envmap.Apply(context.Get(), world * XMMatrixTranslation(2, -0.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        envmap.SetFresnelFactor(1);

        // Environment map, animating the amount, with no fresnel.
        envmap.SetEnvironmentMapAmount(alphaFade);
        envmap.SetFresnelFactor(0);
        envmap.Apply(context.Get(), world * XMMatrixTranslation(2, -1.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        envmap.SetEnvironmentMapAmount(1);
        envmap.SetFresnelFactor(1);

        // Environment map, animating the amount.
        envmap.SetEnvironmentMapAmount(alphaFade);
        envmap.Apply(context.Get(), world * XMMatrixTranslation(2, -2.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        envmap.SetEnvironmentMapAmount(1);

        // Environment map, with animating specular
        envmap.SetEnvironmentMapSpecular(Colors::Blue * alphaFade);
        envmap.Apply(context.Get(), world * XMMatrixTranslation(1, -1.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        envmap.SetEnvironmentMapSpecular(Colors::Black);

        // Environment map, with single light vertex color
        envmap.SetLightEnabled(1, false);
        envmap.SetLightEnabled(2, false);
        envmap.Apply(context.Get(), world * XMMatrixTranslation(1, -2.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        envmap.SetPerPixelLighting(true);

        {
            envmap.SetLightEnabled(1, false);
            envmap.SetLightEnabled(2, false);

            {
                // Light only from above + per pixel lighting, animating the fresnel factor.
                envmap.SetLightDirection(0, XMVectorSet(0, -1, 0, 0));
                envmap.SetFresnelFactor(alphaFade * 3);
                envmap.Apply(context.Get(), world * XMMatrixTranslation(3, -0.5f, 0), view, projection);
                context->DrawIndexed(indexCount, 0, 0);
                envmap.SetFresnelFactor(1);

                // Light only from the left + per pixel lighting, animating the amount, with no fresnel.
                envmap.SetEnvironmentMapAmount(alphaFade);
                envmap.SetFresnelFactor(0);
                envmap.SetLightDirection(0, XMVectorSet(1, 0, 0, 0));
                envmap.Apply(context.Get(), world * XMMatrixTranslation(3, -1.5f, 0), view, projection);
                context->DrawIndexed(indexCount, 0, 0);
                envmap.SetEnvironmentMapAmount(1);
                envmap.SetFresnelFactor(1);

                // Light only from straight in front + per pixel lighting with fog.
                envmap.SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
                envmap.SetFogEnabled(true);
                envmap.SetFogStart(fogstart);
                envmap.SetFogEnd(fogend);
                envmap.SetFogColor(Colors::Gray);
                envmap.Apply(context.Get(), world * XMMatrixTranslation(3, -2.5f, 2 - alphaFade * 6), view, projection);
                context->DrawIndexed(indexCount, 0, 0);
                envmap.SetFogEnabled(false);
            }

            envmap.EnableDefaultLighting();
        }

        envmap.SetPerPixelLighting(false);

        // Dual texture effect.
        dualTexture.Apply(context.Get(), world * XMMatrixTranslation(3, 2.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // Dual texture with alpha fading.
        dualTexture.SetAlpha(alphaFade);
        dualTexture.Apply(context.Get(), world * XMMatrixTranslation(3, 1.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        dualTexture.SetAlpha(1);

        // Dual texture with fog.
        dualTexture.SetFogEnabled(true);
        dualTexture.SetFogStart(fogstart);
        dualTexture.SetFogEnd(fogend);
        dualTexture.SetFogColor(Colors::Gray);
        dualTexture.Apply(context.Get(), world * XMMatrixTranslation(3, 0.5f, 2 - alphaFade * 6), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        dualTexture.SetFogEnabled(false);

        context->OMSetBlendState(states.Opaque(), Colors::White, 0xFFFFFFFF);

        {
            // Alpha test, > 0.
            alphaTest.SetAlphaFunction(D3D11_COMPARISON_GREATER);
            alphaTest.SetReferenceAlpha(0);
            alphaTest.Apply(context.Get(), world * XMMatrixTranslation(4, 2.5f, 0), view, projection);
            context->DrawIndexed(indexCount, 0, 0);

            // Alpha test, > 128.
            alphaTest.SetAlphaFunction(D3D11_COMPARISON_GREATER);
            alphaTest.SetReferenceAlpha(128);
            alphaTest.Apply(context.Get(), world * XMMatrixTranslation(4, 1.5f, 0), view, projection);
            context->DrawIndexed(indexCount, 0, 0);

            // Alpha test with fog.
            alphaTest.SetFogEnabled(true);
            alphaTest.SetFogStart(fogstart);
            alphaTest.SetFogEnd(fogend);
            alphaTest.SetFogColor(Colors::Red);
            alphaTest.Apply(context.Get(), world * XMMatrixTranslation(4, 0.5f, 2 - alphaFade * 6), view, projection);
            context->DrawIndexed(indexCount, 0, 0);
            alphaTest.SetFogEnabled(false);

            // Alpha test, < animating value.
            alphaTest.SetAlphaFunction(D3D11_COMPARISON_LESS);
            alphaTest.SetReferenceAlpha(1 + (int)(alphaFade * 254));
            alphaTest.Apply(context.Get(), world * XMMatrixTranslation(4, -0.5f, 0), view, projection);
            context->DrawIndexed(indexCount, 0, 0);

            // Alpha test, = 255.
            alphaTest.SetAlphaFunction(D3D11_COMPARISON_EQUAL);
            alphaTest.SetReferenceAlpha(255);
            alphaTest.Apply(context.Get(), world * XMMatrixTranslation(4, -1.5f, 0), view, projection);
            context->DrawIndexed(indexCount, 0, 0);

            // Alpha test, != 0.
            alphaTest.SetAlphaFunction(D3D11_COMPARISON_NOT_EQUAL);
            alphaTest.SetReferenceAlpha(0);
            alphaTest.Apply(context.Get(), world * XMMatrixTranslation(4, -2.5f, 0), view, projection);
            context->DrawIndexed(indexCount, 0, 0);
        }

        context->OMSetBlendState(states.AlphaBlend(), Colors::White, 0xFFFFFFFF);

        // NormalMapEffect
        normalMapEffect.Apply(context.Get(), world * XMMatrixTranslation(5, 2.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // NormalMapEffect no spec
        normalMapEffectNoSpecular.Apply(context.Get(), world * XMMatrixTranslation(5, 1.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // NormalMap with fog.
        normalMapEffect.SetFogEnabled(true);
        normalMapEffect.SetFogStart(fogstart);
        normalMapEffect.SetFogEnd(fogend);
        normalMapEffect.SetFogColor(Colors::Gray);
        normalMapEffect.Apply(context.Get(), world * XMMatrixTranslation(5, 0.5f, 2 - alphaFade * 6), view, projection);
        context->DrawIndexed(indexCount, 0, 0);
        normalMapEffect.SetFogEnabled(false);

        // NormalMap with default diffuse
        normalMapEffectNoDiffuse.Apply(context.Get(), world * XMMatrixTranslation(5, -.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        // NormalMap with default diffuse no spec
        normalMapEffectNormalsOnly.Apply(context.Get(), world * XMMatrixTranslation(5, -1.5f, 0), view, projection);
        context->DrawIndexed(indexCount, 0, 0);

        swapChain->Present(1, 0);
        ++frame;

        if ( frame == 10 )
        {
            ComPtr<ID3D11Texture2D> backBufferTex;
            hr = swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&backBufferTex);
            if ( SUCCEEDED(hr) )
            {
                hr = SaveWICTextureToFile( context.Get(), backBufferTex.Get(), GUID_ContainerFormatBmp, L"SCREENSHOT.BMP" );
                hr = SaveDDSTextureToFile( context.Get(), backBufferTex.Get(), L"SCREENSHOT.DDS" );
            }
        }

        time++;
    }

    return 0;
}
