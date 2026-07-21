//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectX Tool Kit - NPREffect
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"

#define GAMMA_CORRECT_RENDERING

// Build for LH vs. RH coords
//#define LH_COORDS

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
#ifdef GAMMA_CORRECT_RENDERING
    const XMVECTORF32 c_clearColor = { { { 0.127437726f, 0.300543845f, 0.846873462f, 1.f } } };
#else
    const XMVECTORF32 c_clearColor = Colors::CornflowerBlue;
#endif
    const XMVECTORF32 c_rimToonColor = { { { 0.6f, 0.8f, 1.0f } } };
    const XMVECTORF32 c_rimGoochColor = { { { 1.f, 1.f, 1.f, 1.f, } } };

    constexpr float row0 = 3.0f;
    constexpr float row1 = 1.5f;
    constexpr float row2 = 0.f;
    constexpr float row3 = -1.5f;
    constexpr float row4 = -3.0f;

    constexpr float col0 = -6.f;
    constexpr float col1 = -4.f;
    constexpr float col2 = -2.f;
    constexpr float col3 = 0.f;
    constexpr float col4 = 2.f;
    constexpr float col5 = 4.f;
    constexpr float col6 = 6.f;

    struct TestVertex
    {
        TestVertex(FXMVECTOR iposition, FXMVECTOR inormal, FXMVECTOR itextureCoordinate)
        {
            XMStoreFloat3(&this->position, iposition);
            XMStoreFloat3(&this->normal, inormal);
            XMStoreFloat2(&this->textureCoordinate, itextureCoordinate);
            XMStoreUByte4(&this->blendIndices, XMVectorSet(0, 1, 2, 3));

            float u = XMVectorGetX(itextureCoordinate) - 0.5f;
            float v = XMVectorGetY(itextureCoordinate) - 0.5f;

            float d = 1 - sqrtf(u * u + v * v) * 2;

            if (d < 0)
                d = 0;

            XMStoreFloat4(&this->blendWeight, XMVectorSet(d, 1 - d, u, v));

            color = 0xFFFF00FF;
        }

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 textureCoordinate;
        XMUBYTE4 blendIndices;
        XMFLOAT4 blendWeight;
        XMUBYTE4 color;

        static constexpr unsigned int InputElementCount = 6;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    const D3D11_INPUT_ELEMENT_DESC TestVertex::InputElements[] =
    {
        { "SV_Position",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",        0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };


    using VertexCollection = std::vector<TestVertex>;
    using IndexCollection = std::vector<uint16_t>;

    #include "../../Src/TeapotData.inc"

    // Tessellates the specified bezier patch.
    void TessellatePatch(VertexCollection& vertices, IndexCollection& indices, TeapotPatch const& patch, FXMVECTOR scale, bool isMirrored)
    {
        constexpr int tessellation = 16;

        XMVECTOR controlPoints[16];

        for (int i = 0; i < 16; i++)
        {
            controlPoints[i] = XMVectorMultiply(TeapotControlPoints[patch.indices[i]], scale);
        }

        Bezier::CreatePatchIndices(tessellation, isMirrored, [&](size_t index)
        {
            indices.push_back((uint16_t)(vertices.size() + index));
        });

        Bezier::CreatePatchVertices(controlPoints, tessellation, isMirrored, [&](FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate)
        {
            vertices.push_back(TestVertex(position, normal, textureCoordinate));
        });
    }

    // Creates a teapot primitive.
    UINT CreateTeapot(ID3D11Device* device, ID3D11Buffer** vertexBuffer, ID3D11Buffer** indexBuffer)
    {
        VertexCollection vertices;
        IndexCollection indices;

        XMVECTOR negateXZ = XMVectorMultiply(g_XMNegateX, g_XMNegateZ);

        for (size_t i = 0; i < sizeof(TeapotPatches) / sizeof(TeapotPatches[0]); i++)
        {
            TeapotPatch const& patch = TeapotPatches[i];

            TessellatePatch(vertices, indices, patch, g_XMOne, false);
            TessellatePatch(vertices, indices, patch, g_XMNegateX, true);

            if (patch.mirrorZ)
            {
                TessellatePatch(vertices, indices, patch, g_XMNegateZ, true);
                TessellatePatch(vertices, indices, patch, negateXZ, false);
            }
        }

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, vertices, D3D11_BIND_VERTEX_BUFFER, vertexBuffer)
        );

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, indexBuffer)
        );

        return static_cast<UINT>(indices.size());
    }
} // anonymous namespace

_Use_decl_annotations_
void Game::CreateTestInputLayout(ID3D11Device* device, IEffect* effect, ID3D11InputLayout** pInputLayout)
{
    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<TestVertex>(device, effect, pInputLayout)
    );
}

//--------------------------------------------------------------------------------------

Game::Game() noexcept(false) :
    m_indexCount(0)
{
#ifdef GAMMA_CORRECT_RENDERING
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif

#ifdef XBOX
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
        );
#elif defined(UWP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0,
        DX::DeviceResources::c_Enable4K_Xbox | DX::DeviceResources::c_EnableQHD_Xbox
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(c_RenderFormat);
#endif

#ifdef LOSTDEVICE
    m_deviceResources->RegisterDeviceNotify(this);
#endif
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
    auto pad = m_gamePad->GetState(0);
    auto kb = m_keyboard->GetState();
    if (kb.Escape || (pad.IsConnected() && pad.IsViewPressed()))
    {
        ExitGame();
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

    XMMATRIX world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

    // Set state objects.
    context->OMSetBlendState(m_states->AlphaBlend(), Colors::White, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
#ifdef LH_COORDS
    context->RSSetState(m_states->CullClockwise());
#else
    context->RSSetState(m_states->CullCounterClockwise());
#endif

    auto sampler = m_states->LinearWrap();
    context->PSSetSamplers(0, 1, &sampler);

    // Set vertex and index buffers.
    UINT vertexStride = sizeof(TestVertex);
    UINT vertexOffset = 0;

    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //--- NPREffect: Cel shading -----------------------------------------------------------

    // Default cel shading (4 bands).
    m_celEffect->effect.SetCelShaderBands(4);
    m_celEffect->Apply(context, world * XMMatrixTranslation(col0, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading with 2 bands.
    m_celEffect->effect.SetCelShaderBands(2);
    m_celEffect->Apply(context, world * XMMatrixTranslation(col1, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading with 8 bands.
    m_celEffect->effect.SetCelShaderBands(8);
    m_celEffect->Apply(context, world * XMMatrixTranslation(col2, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading, no specular.
    m_celEffectNoSpecular->Apply(context, world * XMMatrixTranslation(col3, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading, no rim lighting.
    m_celEffectNoRim->Apply(context, world * XMMatrixTranslation(col4, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading with vertex color.
    m_celEffectVc->Apply(context, world * XMMatrixTranslation(col5, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading with 4 bands and texture.
    m_celEffectTx->effect.SetCelShaderBands(4);
    m_celEffectTx->Apply(context, world * XMMatrixTranslation(col0, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading with 2 bands and texture.
    m_celEffectTx->effect.SetCelShaderBands(2);
    m_celEffectTx->Apply(context, world * XMMatrixTranslation(col1, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading with 8 bands and texture.
    m_celEffectTx->effect.SetCelShaderBands(8);
    m_celEffectTx->Apply(context, world * XMMatrixTranslation(col2, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading with texture, no specular.
    m_celEffectTxNoSpecular->Apply(context, world * XMMatrixTranslation(col3, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading with texture, no rim lighting.
    m_celEffectTxNoRim->Apply(context, world * XMMatrixTranslation(col4, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading with vertex color and texture.
    m_celEffectTxVc->Apply(context, world * XMMatrixTranslation(col5, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Cel shading (4 bands) with alpha.
    m_celEffect->effect.SetCelShaderBands(4);
    m_celEffect->effect.SetAlpha(alphaFade);
    m_celEffect->Apply(context, world * XMMatrixTranslation(col1, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_celEffect->effect.SetAlpha(1.f);

    //--- NPREffect: Gooch shading ---------------------------------------------------------

    // Default Gooch shading.
    m_goochEffect->Apply(context, world * XMMatrixTranslation(col0, row2, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Gooch shading with custom cool/warm colors.
    m_goochEffectCustom->effect.SetGoochCoolColor(Colors::Red, 0.4f);
    m_goochEffectCustom->effect.SetGoochWarmColor(Colors::Green, 0.4f);
    m_goochEffectCustom->Apply(context, world * XMMatrixTranslation(col1, row2, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_goochEffectCustom->effect.SetGoochCoolColor(Colors::Black, 0.1f);
    m_goochEffectCustom->effect.SetGoochWarmColor(Colors::Blue, 0.1f);
    m_goochEffectCustom->Apply(context, world * XMMatrixTranslation(col2, row2, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Gooch shading, no specular.
    m_goochEffectNoSpecular->Apply(context, world * XMMatrixTranslation(col3, row2, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Gooch shading, no rim lighting.
    m_goochEffectNoRim->Apply(context, world * XMMatrixTranslation(col4, row2, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Gooch shading with vertex color.
    m_goochEffectVc->Apply(context, world * XMMatrixTranslation(col5, row2, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Gooch shading with texture.
    m_goochEffectTx->Apply(context, world * XMMatrixTranslation(col0, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Gooch shading with texture, no specular.
    m_goochEffectTxNoSpecular->Apply(context, world * XMMatrixTranslation(col3, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Gooch shading with texture, no rim lighting.
    m_goochEffectTxNoRim->Apply(context, world * XMMatrixTranslation(col4, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Gooch shading with vertex color.
    m_goochEffectTxVc->Apply(context, world * XMMatrixTranslation(col5, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Default Gooch shading with alpha.
    m_goochEffect->effect.SetAlpha(alphaFade);
    m_goochEffect->Apply(context, world * XMMatrixTranslation(col2, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_goochEffect->effect.SetAlpha(1.f);

    //--- NPREffect: MatCap shading --------------------------------------------------------

    // Default MatCap shading
    m_matcapEffect->effect.SetMatCap(m_matCapTxt1.Get());
    m_matcapEffect->Apply(context, world * XMMatrixTranslation(col0, row4, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_matcapEffect->effect.SetAlpha(alphaFade);
    m_matcapEffect->effect.SetMatCap(m_matCapTxt2.Get());
    m_matcapEffect->Apply(context, world * XMMatrixTranslation(col1, row4, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_matcapEffect->effect.SetAlpha(1.f);

    // Matcap shading with vertex color.
    m_matcapEffectVc->Apply(context, world * XMMatrixTranslation(col2, row4, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Matcap shading with texture.
    m_matcapEffectTx->effect.SetMatCap(m_matCapTxt1.Get());
    m_matcapEffectTx->Apply(context, world * XMMatrixTranslation(col3, row4, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_matcapEffectTx->effect.SetAlpha(alphaFade);
    m_matcapEffectTx->effect.SetMatCap(m_matCapTxt2.Get());
    m_matcapEffectTx->Apply(context, world * XMMatrixTranslation(col4, row4, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_matcapEffectTx->effect.SetAlpha(1.f);

    // Matcap shading with vertex color and texture.
    m_matcapEffectTxVc->Apply(context, world * XMMatrixTranslation(col5, row4, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    //--- SkinnedNPREffect -----------------------------------------------------------------

    // Skinned effect, identity transforms.
    XMMATRIX bones[4] =
    {
        XMMatrixIdentity(),
        XMMatrixIdentity(),
        XMMatrixScaling(0, 0, 0),
        XMMatrixScaling(0, 0, 0),
    };

    // Cel shading (4 bands) with skinning.
    m_skinnedCelEffect->effect.SetBoneTransforms(bones, 4);
    m_skinnedCelEffect->Apply(context, world * XMMatrixTranslation(col6, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Gooch effect with skinning.
    m_skinnedGoochEffect->effect.SetBoneTransforms(bones, 4);
    m_skinnedGoochEffect->Apply(context, world * XMMatrixTranslation(col6, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Matcap shading with skinning.
    m_skinnedMatcapEffect->effect.SetBoneTransforms(bones, 4);
    m_skinnedMatcapEffect->Apply(context, world * XMMatrixTranslation(col6, row2, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

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

    m_skinnedCelEffect->effect.SetBoneTransforms(bones, 4);
    m_skinnedCelEffect->Apply(context, world * XMMatrixTranslation(col6, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

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

    m_skinnedCelEffect->effect.SetBoneTransforms(bones, 4);
    m_skinnedCelEffect->Apply(context, world * XMMatrixTranslation(col6, row4, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Show the new frame.
    m_deviceResources->Present();

#ifdef XBOX
    m_graphicsMemory->Commit();
#endif
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, c_clearColor);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    const auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Game::OnResuming()
{
    m_deviceResources->Resume();

    m_timer.ResetElapsedTime();
}

#ifdef PC
void Game::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

#if defined(PC) || defined(UWP)
void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
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

    // Create test geometry.
    m_indexCount = CreateTeapot(device,
        m_vertexBuffer.ReleaseAndGetAddressOf(),
        m_indexBuffer.ReleaseAndGetAddressOf());

    XMVECTORF32 blue, red, green, grey;
#ifdef GAMMA_CORRECT_RENDERING
    blue.v = XMColorSRGBToRGB(Colors::Blue);
    red.v  = XMColorSRGBToRGB(Colors::Red);
    green.v = XMColorSRGBToRGB(Colors::Green);
    grey.v = XMColorSRGBToRGB(Colors::Gray);
    DDS_LOADER_FLAGS loadFlags = DDS_LOADER_FORCE_SRGB;
    WIC_LOADER_FLAGS wicLoadFlags = WIC_LOADER_FORCE_SRGB;
#else
    blue.v = Colors::Blue;
    red.v  = Colors::Red;
    green.v = Colors::Green;
    grey.v = Colors::Gray;
    DDS_LOADER_FLAGS loadFlags = DDS_LOADER_DEFAULT;
    WIC_LOADER_FLAGS wicLoadFlags = WIC_LOADER_DEFAULT;
#endif

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"reftexture.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, loadFlags,
        nullptr, m_reftxt.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, L"matcap_gold.png",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, wicLoadFlags,
        nullptr, m_matCapTxt1.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, L"matcap_ice.png",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, wicLoadFlags,
        nullptr, m_matCapTxt2.ReleaseAndGetAddressOf()));

    //--- Cel shading (Mode_Cel) -----------------------------------------------------------

    // Default cel shading.
    m_celEffect = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Cel);
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(blue);
        effect->SetRimLightingColor(c_rimToonColor);
        effect->SetCelShaderBands(4);
    });

    // Cel shading, no specular.
    m_celEffectNoSpecular = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Cel);
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(blue);
        effect->SetRimLightingColor(c_rimToonColor);
        effect->SetCelShaderBands(4);
        effect->DisableSpecular();
    });

    // Cel shading, no rim lighting.
    m_celEffectNoRim = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Cel);
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(blue);
        effect->SetCelShaderBands(4);
        effect->DisableRimLighting();
    });

    // Cel shading with vertex color.
    m_celEffectVc = std::make_unique<EffectWithDecl<NPREffect>>(device, [](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Cel);
        effect->EnableDefaultLighting();
        effect->SetRimLightingColor(c_rimToonColor);
        effect->SetCelShaderBands(4);
        effect->SetVertexColorEnabled(true);
    });

    // Cel shading with texture.
    m_celEffectTx = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Cel);
        effect->EnableDefaultLighting();
        effect->SetRimLightingColor(c_rimToonColor);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_reftxt.Get());
        effect->SetCelShaderBands(4);
    });

    // Cel shading with texture, no specular
    m_celEffectTxNoSpecular = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Cel);
        effect->EnableDefaultLighting();
        effect->SetRimLightingColor(c_rimToonColor);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_reftxt.Get());
        effect->SetCelShaderBands(4);
        effect->DisableSpecular();
    });

    // Cel shading with texture, no rim lighting
    m_celEffectTxNoRim = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Cel);
        effect->EnableDefaultLighting();
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_reftxt.Get());
        effect->SetCelShaderBands(4);
        effect->DisableRimLighting();
    });

    // Cel shading with vertex color and texture.
    m_celEffectTxVc = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Cel);
        effect->EnableDefaultLighting();
        effect->SetRimLightingColor(c_rimToonColor);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_reftxt.Get());
        effect->SetCelShaderBands(4);
        effect->SetVertexColorEnabled(true);
    });

    // Cel shading with skinning.
    m_skinnedCelEffect = std::make_unique<EffectWithDecl<SkinnedNPREffect>>(device, [=](SkinnedNPREffect* effect)
    {
        effect->SetMode(SkinnedNPREffect::Mode_Cel);
        effect->EnableDefaultLighting();
        effect->SetRimLightingColor(c_rimToonColor);
        effect->SetTexture(m_reftxt.Get());
        effect->SetCelShaderBands(4);
    });

    //--- Gooch shading (Mode_Gooch) -------------------------------------------------------

    // Default Gooch shading.
    m_goochEffect = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Gooch);
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(grey);
        effect->SetRimLightingColor(c_rimGoochColor);
    });

    // Gooch shading, no specular.
    m_goochEffectNoSpecular = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Gooch);
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(grey);
        effect->SetRimLightingColor(c_rimGoochColor);
        effect->DisableSpecular();
    });

    // Gooch shading, no rim lighting.
    m_goochEffectNoRim = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Gooch);
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(grey);
        effect->DisableRimLighting();
    });

    // Gooch shading with vertex color.
    m_goochEffectVc = std::make_unique<EffectWithDecl<NPREffect>>(device, [](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Gooch);
        effect->EnableDefaultLighting();
        effect->SetRimLightingColor(c_rimGoochColor);
        effect->SetVertexColorEnabled(true);
    });

    // Gooch shading with custom cool/warm colors.
    m_goochEffectCustom = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Gooch);
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(grey);
        effect->SetRimLightingColor(c_rimGoochColor);
    });

    // Gooch shading with texture.
    m_goochEffectTx = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Gooch);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_reftxt.Get());
        effect->SetDiffuseColor(grey);
        effect->SetRimLightingColor(c_rimGoochColor);
        effect->EnableDefaultLighting();
        effect->SetGoochCoolColor(red, 0.4f);
        effect->SetGoochWarmColor(green, 0.4f);
    });

    // Gooch shading with texture, no specular.
    m_goochEffectTxNoSpecular = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Gooch);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_reftxt.Get());
        effect->SetDiffuseColor(grey);
        effect->SetRimLightingColor(c_rimGoochColor);
        effect->EnableDefaultLighting();
        effect->SetGoochCoolColor(red, 0.4f);
        effect->SetGoochWarmColor(green, 0.4f);
        effect->DisableSpecular();
    });

    // Gooch shading with texture, no rim lighting.
    m_goochEffectTxNoRim = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Gooch);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_reftxt.Get());
        effect->SetDiffuseColor(grey);
        effect->EnableDefaultLighting();
        effect->SetGoochCoolColor(red, 0.4f);
        effect->SetGoochWarmColor(green, 0.4f);
        effect->DisableRimLighting();
    });

    // Gooch shading with vertex color and texture.
    m_goochEffectTxVc = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_Gooch);
        effect->SetTextureEnabled(true);
        effect->SetDiffuseColor(grey);
        effect->SetTexture(m_reftxt.Get());
        effect->EnableDefaultLighting();
        effect->SetRimLightingColor(c_rimGoochColor);
        effect->SetGoochCoolColor(red, 0.4f);
        effect->SetGoochWarmColor(green, 0.4f);
        effect->SetVertexColorEnabled(true);
    });

    // Gooch shading with skinning.
    m_skinnedGoochEffect = std::make_unique<EffectWithDecl<SkinnedNPREffect>>(device, [=](SkinnedNPREffect* effect)
    {
        effect->SetMode(SkinnedNPREffect::Mode_Gooch);
        effect->SetTexture(m_reftxt.Get());
        effect->SetDiffuseColor(grey);
        effect->SetRimLightingColor(c_rimGoochColor);
        effect->EnableDefaultLighting();
        effect->SetGoochCoolColor(red, 0.4f);
        effect->SetGoochWarmColor(green, 0.4f);
    });

    //--- MatCap shading (Mode_MatCap) -----------------------------------------------------

    // Default MatCap shading
    m_matcapEffect = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_MatCap);
        effect->SetMatCap(m_matCapTxt1.Get());
    });

    // Matcap shading with vertex color.
    m_matcapEffectVc = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_MatCap);
        effect->SetMatCap(m_matCapTxt1.Get());
        effect->SetVertexColorEnabled(true);
    });

    // Matcap shading with texture.
    m_matcapEffectTx = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_MatCap);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_reftxt.Get());
        effect->SetMatCap(m_matCapTxt1.Get());
    });

    // Matcap sahding with vertex color and texture.
    m_matcapEffectTxVc = std::make_unique<EffectWithDecl<NPREffect>>(device, [=](NPREffect* effect)
    {
        effect->SetMode(NPREffect::Mode_MatCap);
        effect->SetTextureEnabled(true);
        effect->SetTexture(m_reftxt.Get());
        effect->SetMatCap(m_matCapTxt1.Get());
        effect->SetVertexColorEnabled(true);
    });

    // MatCap shading with skinning.
    m_skinnedMatcapEffect = std::make_unique<EffectWithDecl<SkinnedNPREffect>>(device, [=](SkinnedNPREffect* effect)
    {
        effect->SetMode(SkinnedNPREffect::Mode_MatCap);
        effect->SetTexture(m_reftxt.Get());
        effect->SetMatCap(m_matCapTxt1.Get());
    });
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { { { 0.f, 0.f, 7.f, 0.f } } };

    const auto size = m_deviceResources->GetOutputSize();
    const float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    m_view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 15);
#else
    m_view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 15);
#endif

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
    m_celEffect.reset();
    m_celEffectNoSpecular.reset();
    m_celEffectNoRim.reset();
    m_celEffectVc.reset();
    m_celEffectTx.reset();
    m_celEffectTxNoSpecular.reset();
    m_celEffectTxNoRim.reset();
    m_celEffectTxVc.reset();
    m_skinnedCelEffect.reset();

    m_goochEffect.reset();
    m_goochEffectNoSpecular.reset();
    m_goochEffectNoRim.reset();
    m_goochEffectVc.reset();
    m_goochEffectCustom.reset();
    m_goochEffectTx.reset();
    m_goochEffectTxNoSpecular.reset();
    m_goochEffectTxNoRim.reset();
    m_goochEffectTxVc.reset();
    m_skinnedGoochEffect.reset();

    m_matcapEffect.reset();
    m_matcapEffectVc.reset();
    m_matcapEffectTx.reset();
    m_matcapEffectTxVc.reset();
    m_skinnedMatcapEffect.reset();

    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();

    m_reftxt.Reset();
    m_matCapTxt1.Reset();
    m_matCapTxt2.Reset();

    m_states.reset();

#ifdef XBOX
    m_graphicsMemory.reset();
#endif
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
