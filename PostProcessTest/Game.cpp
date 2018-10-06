//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK PostProcess
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

#define USE_FAST_SEMANTICS

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const int MaxScene = 27;

    const DXGI_FORMAT c_sdrFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
    const DXGI_FORMAT c_hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
}

// Constructor.
Game::Game() noexcept(false) :
    m_scene(0)
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_sdrFormat, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics);
#endif
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_sdrFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0,
        DX::DeviceResources::c_Enable4K_Xbox);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_sdrFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0);
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    m_deviceResources->RegisterDeviceNotify(this);
#endif
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

    ShaderTest();
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

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED
            || m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            ++m_scene;
            if (m_scene >= MaxScene)
                m_scene = 0;
        }
        else if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED
                 || m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            --m_scene;
            if (m_scene < 0)
                m_scene = MaxScene - 1;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    m_keyboardButtons.Update(kb);

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
    {
        ++m_scene;
        if (m_scene >= MaxScene)
            m_scene = 0;
    }
    else if (m_keyboardButtons.IsKeyPressed(Keyboard::Back))
    {
        --m_scene;
        if (m_scene < 0)
            m_scene = MaxScene - 1;
    }

    float time = float(timer.GetTotalSeconds());

    m_world = Matrix::CreateRotationY(time);
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

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
    m_deviceResources->Prepare();
#endif

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    m_spriteBatch->Begin();
    m_spriteBatch->Draw(m_scene >= 16 ? m_hdrTexture.Get() : m_background.Get(), m_deviceResources->GetOutputSize());
    m_spriteBatch->End();

    m_shape->Draw(m_world, m_view, m_proj, Colors::White, m_texture.Get());

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
    context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

    // Post process.
#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
    context->DecompressResource(m_sceneTex.Get(), 0, nullptr,
        m_sceneTex.Get(), 0, nullptr,
        c_hdrFormat, D3D11X_DECOMPRESS_PROPAGATE_COLOR_CLEAR);
#endif

    auto renderTarget = m_deviceResources->GetRenderTargetView();
    context->OMSetRenderTargets(1, &renderTarget, nullptr);
    m_basicPostProcess->SetSourceTexture(m_sceneSRV.Get());
    m_dualPostProcess->SetSourceTexture(m_sceneSRV.Get());
    m_toneMapPostProcess->SetHDRSourceTexture(m_sceneSRV.Get());
    m_toneMapPostProcess->SetExposure(0.f);

    const wchar_t* descstr = nullptr;
    switch (m_scene)
    {
    case 0:
    default:
        descstr = L"Copy (passthrough)";
        m_basicPostProcess->SetEffect(BasicPostProcess::Copy);
        m_basicPostProcess->Process(context);
        break;

    case 1:
        descstr = L"Monochrome";
        m_basicPostProcess->SetEffect(BasicPostProcess::Monochrome);
        m_basicPostProcess->Process(context);
        break;

    case 2:
        descstr = L"Sepia";
        m_basicPostProcess->SetEffect(BasicPostProcess::Sepia);
        m_basicPostProcess->Process(context);
        break;

    case 3:
        descstr = L"Downscale 2x2";
        m_basicPostProcess->SetEffect(BasicPostProcess::DownScale_2x2);
        m_basicPostProcess->Process(context);
        break;

    case 4:
        descstr = L"Downscale 4x4";
        m_basicPostProcess->SetEffect(BasicPostProcess::DownScale_4x4);
        m_basicPostProcess->Process(context);
        break;

    case 5:
        descstr = L"GaussianBlur 5x5";
        m_basicPostProcess->SetEffect(BasicPostProcess::GaussianBlur_5x5);
        m_basicPostProcess->SetGaussianParameter(1.f);
        m_basicPostProcess->Process(context);
        break;

    case 6:
        descstr = L"GaussianBlur 5x5 (2X)";
        m_basicPostProcess->SetEffect(BasicPostProcess::GaussianBlur_5x5);
        m_basicPostProcess->SetGaussianParameter(2.f);
        m_basicPostProcess->Process(context);
        break;

    case 7:
        descstr = L"BloomExtract";
        m_basicPostProcess->SetEffect(BasicPostProcess::BloomExtract);
        m_basicPostProcess->SetBloomExtractParameter(0.25f);
        m_basicPostProcess->Process(context);
        break;

    case 8:
        {
            descstr = L"BloomBlur (extract + horizontal)";
        
            // Pass 1 (scene -> blur1)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomExtract);
            m_basicPostProcess->SetBloomExtractParameter(0.25f);

            auto blurRT1 = m_blur1RT.Get();
            context->OMSetRenderTargets(1, &blurRT1, nullptr);

            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 2 (blur1 -> rt)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomBlur);
            m_basicPostProcess->SetBloomBlurParameters(true, 4.f, 1.f);

            context->OMSetRenderTargets(1, &renderTarget, nullptr);

            m_basicPostProcess->SetSourceTexture(m_blur1SRV.Get());
            m_basicPostProcess->Process(context);
        }
        break;

    case 9:
        {
            descstr = L"BloomBlur (extract + vertical)";
        
            // Pass 1 (scene -> blur1)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomExtract);
            m_basicPostProcess->SetBloomExtractParameter(0.25f);

            auto blurRT1 = m_blur1RT.Get();
            context->OMSetRenderTargets(1, &blurRT1, nullptr);

            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 2 (blur1 -> rt)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomBlur);
            m_basicPostProcess->SetBloomBlurParameters(false, 4.f, 1.f);

            context->OMSetRenderTargets(1, &renderTarget, nullptr);

            m_basicPostProcess->SetSourceTexture(m_blur1SRV.Get());
            m_basicPostProcess->Process(context);
        }
        break;

    case 10:
        {
            descstr = L"BloomBlur (extract + horz + vert)";

            // Pass 1 (scene -> blur1)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomExtract);
            m_basicPostProcess->SetBloomExtractParameter(0.25f);

            auto blurRT1 = m_blur1RT.Get();
            context->OMSetRenderTargets(1, &blurRT1, nullptr);

            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 2 (blur1 -> blur2)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomBlur);
            m_basicPostProcess->SetBloomBlurParameters(true, 4.f, 1.f);

            auto blurRT2 = m_blur2RT.Get();
            context->OMSetRenderTargets(1, &blurRT2, nullptr);

            m_basicPostProcess->SetSourceTexture(m_blur1SRV.Get());
            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 3 (blur2 -> rt)
            m_basicPostProcess->SetBloomBlurParameters(false, 4.f, 1.f);

            context->OMSetRenderTargets(1, &renderTarget, nullptr);

            m_basicPostProcess->SetSourceTexture(m_blur2SRV.Get());
            m_basicPostProcess->Process(context);
        }
        break;

    case 11:
        {
            descstr = L"Bloom";

            // Pass 1 (scene->blur1)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomExtract);
            m_basicPostProcess->SetBloomExtractParameter(0.25f);

            auto blurRT1 = m_blur1RT.Get();
            context->OMSetRenderTargets(1, &blurRT1, nullptr);

            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 2 (blur1 -> blur2)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomBlur);
            m_basicPostProcess->SetBloomBlurParameters(true, 4.f, 1.f);

            auto blurRT2 = m_blur2RT.Get();
            context->OMSetRenderTargets(1, &blurRT2, nullptr);

            m_basicPostProcess->SetSourceTexture(m_blur1SRV.Get());
            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 3 (blur2 -> blur1)
            m_basicPostProcess->SetBloomBlurParameters(false, 4.f, 1.f);

            ID3D11ShaderResourceView* nullsrv[] = { nullptr, nullptr };
            context->PSSetShaderResources(0, 2, nullsrv);

            context->OMSetRenderTargets(1, &blurRT1, nullptr);

            m_basicPostProcess->SetSourceTexture(m_blur2SRV.Get());
            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 4 (scene+blur1 -> rt)
            m_dualPostProcess->SetEffect(DualPostProcess::BloomCombine);
            m_dualPostProcess->SetBloomCombineParameters(1.25f, 1.f, 1.f, 1.f);

            context->OMSetRenderTargets(1, &renderTarget, nullptr);

            m_dualPostProcess->SetSourceTexture2(m_blur1SRV.Get());
            m_dualPostProcess->Process(context);
        }
        break;

    case 12:
        {
            descstr = L"Bloom (Saturated)";

            // Pass 1 (scene->blur1)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomExtract);
            m_basicPostProcess->SetBloomExtractParameter(0.25f);

            auto blurRT1 = m_blur1RT.Get();
            context->OMSetRenderTargets(1, &blurRT1, nullptr);

            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 2 (blur1 -> blur2)
            m_basicPostProcess->SetEffect(BasicPostProcess::BloomBlur);
            m_basicPostProcess->SetBloomBlurParameters(true, 4.f, 1.f);

            auto blurRT2 = m_blur2RT.Get();
            context->OMSetRenderTargets(1, &blurRT2, nullptr);

            m_basicPostProcess->SetSourceTexture(m_blur1SRV.Get());
            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 3 (blur2 -> blur1)
            m_basicPostProcess->SetBloomBlurParameters(false, 4.f, 1.f);

            ID3D11ShaderResourceView* nullsrv[] = { nullptr, nullptr };
            context->PSSetShaderResources(0, 2, nullsrv);

            context->OMSetRenderTargets(1, &blurRT1, nullptr);

            m_basicPostProcess->SetSourceTexture(m_blur2SRV.Get());
            m_basicPostProcess->Process(context);

#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
            context->FlushGpuCacheRange(D3D11_FLUSH_ENSURE_CB0_COHERENCY | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE, nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
#endif

            // Pass 4 (scene+blur1 -> rt)
            m_dualPostProcess->SetEffect(DualPostProcess::BloomCombine);
            m_dualPostProcess->SetBloomCombineParameters(2.f, 1.f, 2.f, 0.f);

            context->OMSetRenderTargets(1, &renderTarget, nullptr);

            m_dualPostProcess->SetSourceTexture2(m_blur1SRV.Get());
            m_dualPostProcess->Process(context);
        }
        break;

    case 13:
        descstr = L"Merge (90%/10%)";
        m_dualPostProcess->SetEffect(DualPostProcess::Merge);
        m_dualPostProcess->SetSourceTexture2(m_hdrTexture.Get());
        m_dualPostProcess->SetMergeParameters(0.9f, 0.1f);
        m_dualPostProcess->Process(context);
        break;

    case 14:
        descstr = L"Merge (50%/%50%)";
        m_dualPostProcess->SetEffect(DualPostProcess::Merge);
        m_dualPostProcess->SetSourceTexture2(m_hdrTexture.Get());
        m_dualPostProcess->SetMergeParameters(0.5f, 0.5f);
        m_dualPostProcess->Process(context);
        break;

    case 15:
        descstr = L"Merge (10%/%90%)";
        m_dualPostProcess->SetEffect(DualPostProcess::Merge);
        m_dualPostProcess->SetSourceTexture2(m_hdrTexture.Get());
        m_dualPostProcess->SetMergeParameters(0.1f, 0.9f);
        m_dualPostProcess->Process(context);
        break;

    case 16:
        descstr = L"ToneMap (None)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::None);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::Linear);
        m_toneMapPostProcess->Process(context);
        break;

    case 17:
        descstr = L"ToneMap (Saturate)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::Saturate);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::Linear);
        m_toneMapPostProcess->Process(context);
        break;

    case 18:
        descstr = L"ToneMap (Reinhard)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::Reinhard);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::Linear);
        m_toneMapPostProcess->Process(context);
        break;

    case 19:
        descstr = L"ToneMap (ACES Filmic)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::ACESFilmic);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::Linear);
        m_toneMapPostProcess->Process(context);
        break;

    case 20:
        descstr = L"ToneMap (SRGB)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::None);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::SRGB);
        m_toneMapPostProcess->Process(context);
        break;

    case 21:
        descstr = L"ToneMap (Saturate SRGB)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::Saturate);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::SRGB);
        m_toneMapPostProcess->Process(context);
        break;

    case 22:
        descstr = L"ToneMap (Reinhard SRGB)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::Reinhard);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::SRGB);
        m_toneMapPostProcess->Process(context);
        break;

    case 23:
        descstr = L"ToneMap (ACES Filmic SRGB)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::ACESFilmic);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::SRGB);
        m_toneMapPostProcess->Process(context);
        break;

    case 24:
        descstr = L"ToneMap (Saturate SRGB EV 2)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::Saturate);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::SRGB);
        m_toneMapPostProcess->SetExposure(2.f);
        m_toneMapPostProcess->Process(context);
        break;

    case 25:
        descstr = L"ToneMap (Reinhard SRGB EV 2)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::Reinhard);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::SRGB);
        m_toneMapPostProcess->SetExposure(2.f);
        m_toneMapPostProcess->Process(context);
        break;

    case 26:
        descstr = L"ToneMap (ACES Filmic SRGB, EV 2)";
        m_toneMapPostProcess->SetOperator(ToneMapPostProcess::ACESFilmic);
        m_toneMapPostProcess->SetTransferFunction(ToneMapPostProcess::SRGB);
        m_toneMapPostProcess->SetExposure(2.f);
        m_toneMapPostProcess->Process(context);
        break;

        // ST2084 scenarios and MRT are convered in the HDRTest
    }

    // Draw UI.
    auto size = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(size.right, size.bottom);

    m_spriteBatch->Begin();
    m_font->DrawString(m_spriteBatch.get(), descstr, XMFLOAT2(float(safeRect.left), float(safeRect.bottom - m_font->GetLineSpacing())));
    m_spriteBatch->End();

    // Clear binding to avoid SDK debug warning
    ID3D11ShaderResourceView* nullsrv[] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullsrv);

    // Show the new frame.
#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
    m_deviceResources->Present(0);
#else
    m_deviceResources->Present();
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_graphicsMemory->Commit();
#endif
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the scene views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_sceneRT.Get();
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

    m_spriteBatch->SetViewport(viewport);
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
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_timer.ResetElapsedTime();
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

    // Create scene objects
    auto context = m_deviceResources->GetD3DDeviceContext();

    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    m_font = std::make_unique<SpriteFont>(device, L"comic.spritefont");

    m_shape = GeometricPrimitive::CreateTeapot(context);

    m_world = Matrix::Identity;

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"earth.bmp", nullptr, m_texture.ReleaseAndGetAddressOf())
        );

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, L"sunset.jpg", nullptr, m_background.ReleaseAndGetAddressOf())
    );

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"HDR_029_Sky_Cloudy_Ref.DDS", nullptr, m_hdrTexture.ReleaseAndGetAddressOf())
    );

    // Setup post processing
    m_abstractPostProcess = std::make_unique<BasicPostProcess>(device);

    m_basicPostProcess = std::make_unique<BasicPostProcess>(device);

    m_dualPostProcess = std::make_unique<DualPostProcess>(device);

    m_toneMapPostProcess = std::make_unique<ToneMapPostProcess>(device);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();

    UINT width = size.right - size.left;
    UINT height = size.bottom - size.top;

    // Create scene render target
    auto device = m_deviceResources->GetD3DDevice();

    CD3D11_TEXTURE2D_DESC sceneDesc(
        c_hdrFormat, width, height,
        1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

    DX::ThrowIfFailed(device->CreateTexture2D(&sceneDesc, nullptr, m_sceneTex.GetAddressOf()));

    DX::ThrowIfFailed(device->CreateShaderResourceView(m_sceneTex.Get(), nullptr, m_sceneSRV.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(device->CreateRenderTargetView(m_sceneTex.Get(), nullptr, m_sceneRT.ReleaseAndGetAddressOf()));

    // Create additional render targets
    CD3D11_TEXTURE2D_DESC blurDesc(
        m_deviceResources->GetBackBufferFormat(), width, height,
        1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

    DX::ThrowIfFailed(device->CreateTexture2D(&blurDesc, nullptr, m_blur1Tex.GetAddressOf()));

    DX::ThrowIfFailed(device->CreateShaderResourceView(m_blur1Tex.Get(), nullptr, m_blur1SRV.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(device->CreateRenderTargetView(m_blur1Tex.Get(), nullptr, m_blur1RT.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(device->CreateTexture2D(&blurDesc, nullptr, m_blur2Tex.GetAddressOf()));

    DX::ThrowIfFailed(device->CreateShaderResourceView(m_blur2Tex.Get(), nullptr, m_blur2SRV.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(device->CreateRenderTargetView(m_blur2Tex.Get(), nullptr, m_blur2RT.ReleaseAndGetAddressOf()));

    // Setup matrices
    m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f),
        Vector3::Zero, Vector3::UnitY);

    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(size.right) / float(size.bottom), 0.1f, 10.f);
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnDeviceLost()
{
    m_abstractPostProcess.reset();
    m_basicPostProcess.reset();
    m_dualPostProcess.reset();
    m_toneMapPostProcess.reset();

    m_spriteBatch.reset();
    m_font.reset();
    m_shape.reset();
    m_texture.Reset();
    m_background.Reset();
    m_hdrTexture.Reset();

    m_sceneTex.Reset();
    m_sceneSRV.Reset();
    m_sceneRT.Reset();

    m_blur1Tex.Reset();
    m_blur1SRV.Reset();
    m_blur1RT.Reset();

    m_blur2Tex.Reset();
    m_blur2SRV.Reset();
    m_blur2RT.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion


// Attempt to bind every possible shader combination for validation
void Game::ShaderTest()
{
    assert(m_sceneTex != 0 && m_sceneRT != 0);
    assert(m_hdrTexture != 0);
    assert(m_background != 0);

    auto context = m_deviceResources->GetD3DDeviceContext();

    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    auto renderTarget = m_sceneRT.Get();
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    // Basic
    m_basicPostProcess->SetSourceTexture(m_hdrTexture.Get());

    for (int j = 0; j < static_cast<int>(BasicPostProcess::Effect_Max); ++j)
    {
        m_basicPostProcess->SetEffect( static_cast<BasicPostProcess::Effect>(j));
        m_basicPostProcess->Process(context);
    }

    // Dual
    m_dualPostProcess->SetSourceTexture(m_hdrTexture.Get());
    m_dualPostProcess->SetSourceTexture2(m_background.Get());

    for (int j = 0; j < static_cast<int>(DualPostProcess::Effect_Max); ++j)
    {
        m_dualPostProcess->SetEffect(static_cast<DualPostProcess::Effect>(j));
        m_dualPostProcess->Process(context);
    }

    // ToneMap
    m_toneMapPostProcess->SetHDRSourceTexture(m_hdrTexture.Get());
    for (int op = 0; op < static_cast<int>(ToneMapPostProcess::Operator_Max); ++op)
    {
        for (int tf = 0; tf < static_cast<int>(ToneMapPostProcess::TransferFunction_Max); ++tf)
        {
            m_toneMapPostProcess->SetOperator(static_cast<ToneMapPostProcess::Operator>(op));
            m_toneMapPostProcess->SetTransferFunction(static_cast<ToneMapPostProcess::TransferFunction>(tf));
            m_toneMapPostProcess->Process(context);
        }
    }

#if defined(_XBOX_ONE) && defined(_TITLE)
    ComPtr<ID3D11Texture2D> tex;
    ComPtr<ID3D11RenderTargetView> rtv;
    {
        auto size = m_deviceResources->GetOutputSize();

        UINT width = size.right - size.left;
        UINT height = size.bottom - size.top;

        CD3D11_TEXTURE2D_DESC desc(
            m_deviceResources->GetBackBufferFormat(), width, height,
            1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

        auto device = m_deviceResources->GetD3DDevice();

        DX::ThrowIfFailed(device->CreateTexture2D(&desc, nullptr, tex.GetAddressOf()));

        DX::ThrowIfFailed(device->CreateRenderTargetView(tex.Get(), nullptr, rtv.GetAddressOf()));

        m_toneMapPostProcess->SetMRTOutput(true);

        ID3D11RenderTargetView* rtvs[2] = { m_sceneRT.Get(), rtv.Get() };
        context->OMSetRenderTargets(2, rtvs, nullptr);

        for (int op = 0; op < static_cast<int>(ToneMapPostProcess::Operator_Max); ++op)
        {
            for (int tf = 0; tf < static_cast<int>(ToneMapPostProcess::TransferFunction_Max); ++tf)
            {
                m_toneMapPostProcess->SetOperator(static_cast<ToneMapPostProcess::Operator>(op));
                m_toneMapPostProcess->SetTransferFunction(static_cast<ToneMapPostProcess::TransferFunction>(tf));
                m_toneMapPostProcess->Process(context);
            }
        }

        m_toneMapPostProcess->SetMRTOutput(false);
    }
#endif

    context->ClearState();
}
