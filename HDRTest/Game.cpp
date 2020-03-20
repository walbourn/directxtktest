//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK PostProcess (HDR10/ToneMap)
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

//#define USE_FAST_SEMANTICS

// Build for LH vs. RH coords
//#define LH_COORDS

// For UWP/PC, this tests using a linear F16 swapchain intead of HDR10
//#define TEST_HDR_LINEAR

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const XMVECTORF32 c_BrightYellow = { 2.f, 2.f, 0.f, 1.f };

    const XMVECTORF32 c_DimWhite = { .5f, .5f, .5f, 1.f };
    const XMVECTORF32 c_BrightWhite = { 2.f, 2.f, 2.f, 1.f };
    const XMVECTORF32 c_VeryBrightWhite = { 4.f, 4.f, 4.f, 1.f };

    const float row0 = -2.f;

    const float col0 = -5.f;
    const float col1 = -3.5f;
    const float col2 = -1.f;
    const float col3 = 1.f;
    const float col4 = 3.5f;
    const float col5 = 5.f;
}

#if defined(_XBOX_ONE) && defined(_TITLE)
extern bool g_HDRMode;
#endif

// Constructor.
Game::Game() noexcept(false) :
    m_toneMapMode(ToneMapPostProcess::Reinhard)
{
#ifdef TEST_HDR_LINEAR
    const DXGI_FORMAT c_DisplayFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
#else
    const DXGI_FORMAT c_DisplayFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        | DX::DeviceResources::c_EnableHDR);
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_DisplayFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0,
        DX::DeviceResources::c_EnableHDR | DX::DeviceResources::c_Enable4K_Xbox);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_DisplayFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0,
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
    auto pad = m_gamePad->GetState(0);
    auto kb = m_keyboard->GetState();
    if (kb.Escape || (pad.IsConnected() && pad.IsViewPressed()))
    {
        ExitGame();
    }

    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            CycleToneMapOperator();
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    m_keyboardButtons.Update(kb);

    if (m_keyboardButtons.pressed.T)
    {
        CycleToneMapOperator();
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

    auto vp = m_deviceResources->GetOutputSize();
    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(vp.right - vp.left), UINT(vp.bottom - vp.top));

    long w = safeRect.right - safeRect.left;
    long h = safeRect.bottom - safeRect.top;

    m_batch->Begin();

    RECT r = { safeRect.left, safeRect.top,
        safeRect.left + ( w / 2 ),
        safeRect.top + (h / 2) };
    m_batch->Draw(m_hdrImage1.Get(), r);

    r = { safeRect.left + (w/2), safeRect.top,
        safeRect.left + (w / 2) + (w / 4),
        safeRect.top + (h / 4) };
    m_batch->Draw(m_hdrImage2.Get(), r, c_DimWhite);

    r = { safeRect.left + (w / 2) + (w / 4), safeRect.top,
        safeRect.left + (w / 2) + (w / 4) * 2,
        safeRect.top + (h / 4) };
    m_batch->Draw(m_hdrImage2.Get(), r);

    r = { safeRect.left + (w / 2), safeRect.top + (h/4),
        safeRect.left + (w / 2) + (w / 4),
        safeRect.top + (h / 4) * 2 };
    m_batch->Draw(m_hdrImage2.Get(), r, c_BrightWhite);

    r = { safeRect.left + (w / 2) + (w / 4), safeRect.top + (h / 4),
        safeRect.left + (w / 2) + (w / 4) * 2,
        safeRect.top + (h / 4) * 2 };
    m_batch->Draw(m_hdrImage2.Get(), r, c_VeryBrightWhite);

    m_batch->End();


    // Time-based animation
    float time = static_cast<float>(m_timer.GetTotalSeconds());

    float alphaFade = (sin(time * 2) + 1) / 2;

    if (alphaFade >= 1)
        alphaFade = 1 - FLT_EPSILON;

    float yaw = time * 0.4f;
    float pitch = time * 0.7f;
    float roll = time * 1.1f;

    XMMATRIX world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
    XMVECTOR quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);

    m_flatEffect->SetMatrices(world* XMMatrixTranslation(col0, row0, 0), m_view, m_projection);
    m_flatEffect->SetTexture(m_hdrImage1.Get());
    m_shape->Draw(m_flatEffect.get(), m_flatInputLayout.Get());

    m_flatEffect->SetMatrices(world* XMMatrixTranslation(col1, row0, 0), m_view, m_projection);
    m_flatEffect->SetTexture(m_hdrImage2.Get());
    m_shape->Draw(m_flatEffect.get(), m_flatInputLayout.Get());

    m_shape->Draw(world* XMMatrixTranslation(col2, row0, 0), m_view, m_projection, Colors::White, m_hdrImage1.Get());
    m_shape->Draw(world* XMMatrixTranslation(col3, row0, 0), m_view, m_projection, Colors::White, m_hdrImage2.Get());

    m_brightEffect->SetMatrices(world* XMMatrixTranslation(col4, row0, 0), m_view, m_projection);
    m_brightEffect->SetTexture(m_hdrImage1.Get());
    m_shape->Draw(m_brightEffect.get(), m_brightInputLayout.Get());

    m_brightEffect->SetMatrices(world* XMMatrixTranslation(col5, row0, 0), m_view, m_projection);
    m_brightEffect->SetTexture(m_hdrImage2.Get());
    m_shape->Draw(m_brightEffect.get(), m_brightInputLayout.Get());

    // Render HUD
    m_batch->Begin();

    const wchar_t* info = L"";

#if defined(_XBOX_ONE) && defined(_TITLE)
    switch (m_toneMapMode)
    {
    case ToneMapPostProcess::Saturate: info = (g_HDRMode) ? L"HDR10 (GameDVR: None)" : L"None"; break;
    case ToneMapPostProcess::Reinhard: info = (g_HDRMode) ? L"HDR10 (GameDVR: Reinhard)" : L"Reinhard"; break;
    case ToneMapPostProcess::ACESFilmic: info = (g_HDRMode) ? L"HDR10 (GameDVR: ACES Filmic)" : L"ACES Filmic"; break;
    }
#else
    switch (m_deviceResources->GetColorSpace())
    {
    default:
        switch (m_toneMapMode)
        {
        case ToneMapPostProcess::Saturate: info = L"None"; break;
        case ToneMapPostProcess::Reinhard: info = L"Reinhard"; break;
        case ToneMapPostProcess::ACESFilmic: info = L"ACES Filmic"; break;
        }
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        info = L"HDR10";
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
        info = L"Linear";
        break;
    }
#endif

    m_font->DrawString(m_batch.get(), info, XMFLOAT2(float(safeRect.right - (safeRect.right / 4)), float(safeRect.bottom - (h/16))), c_BrightYellow);

    m_batch->End();

    // Tonemap the frame.
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_hdrScene->EndScene(context);
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
    ID3D11RenderTargetView* renderTargets[2] = { m_deviceResources->GetRenderTargetView(), m_deviceResources->GetGameDVRRenderTargetView() };
    context->OMSetRenderTargets(2, renderTargets, nullptr);

    m_toneMap->SetOperator(static_cast<ToneMapPostProcess::Operator>(m_toneMapMode));
#else
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    switch (m_deviceResources->GetColorSpace())
    {
    default:
        m_toneMap->SetOperator(static_cast<ToneMapPostProcess::Operator>(m_toneMapMode));
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
    m_keyboardButtons.Reset();
    m_gamePadButtons.Reset();
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    auto context = m_deviceResources->GetD3DDeviceContext();
    context->Suspend(0);
#endif
}

void Game::OnResuming()
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    auto context = m_deviceResources->GetD3DDeviceContext();
    context->Resume();
#endif

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

    auto context = m_deviceResources->GetD3DDeviceContext();

#ifdef LH_COORDS
    m_shape = GeometricPrimitive::CreateCube(context, 1.f, false);
#else
    m_shape = GeometricPrimitive::CreateCube(context);
#endif

    m_flatEffect = std::make_unique<BasicEffect>(device);
    m_flatEffect->SetTextureEnabled(true);
    m_flatEffect->SetAmbientLightColor(Colors::White);
    m_flatEffect->DisableSpecular();
    m_shape->CreateInputLayout(m_flatEffect.get(), m_flatInputLayout.ReleaseAndGetAddressOf());

    m_brightEffect = std::make_unique<BasicEffect>(device);
    m_brightEffect->SetTextureEnabled(true);
    m_brightEffect->EnableDefaultLighting();
    m_brightEffect->SetLightDiffuseColor(0, Colors::White);
    m_brightEffect->SetLightDiffuseColor(1, c_VeryBrightWhite);
    m_brightEffect->SetLightDiffuseColor(2, Colors::White);
    m_shape->CreateInputLayout(m_brightEffect.get(), m_brightInputLayout.ReleaseAndGetAddressOf());

    m_batch = std::make_unique<SpriteBatch>(context);

    m_font = std::make_unique<SpriteFont>(device, L"comic.spritefont");

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"HDR_029_Sky_Cloudy_Ref.dds", nullptr, m_hdrImage1.ReleaseAndGetAddressOf())
    );

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"HDR_112_River_Road_2_Ref.dds", nullptr, m_hdrImage2.ReleaseAndGetAddressOf())
    );

    // Set the device for the HDR helper
    m_hdrScene->SetDevice(device);

    m_toneMap = std::make_unique<ToneMapPostProcess>(device);
    m_toneMap->SetOperator(static_cast<ToneMapPostProcess::Operator>(m_toneMapMode));
    m_toneMap->SetTransferFunction(ToneMapPostProcess::SRGB);

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_toneMap->SetMRTOutput(true);
#endif
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { 0, 0, 7 };

    auto size = m_deviceResources->GetOutputSize();
    float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    m_view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 10);
#else
    m_view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 10);
#endif

    // Set windows size for HDR.
    m_hdrScene->SetWindow(size);

    m_toneMap->SetHDRSourceTexture(m_hdrScene->GetShaderResourceView());

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    m_batch->SetRotation(m_deviceResources->GetRotation());
#endif

    m_batch->SetViewport(m_deviceResources->GetScreenViewport());
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnDeviceLost()
{
    m_batch.reset();
    m_font.reset();

    m_shape.reset();

    m_flatEffect.reset();
    m_brightEffect.reset();

    m_hdrImage1.Reset();
    m_hdrImage2.Reset();

    m_flatInputLayout.Reset();
    m_brightInputLayout.Reset();

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

void Game::CycleToneMapOperator()
{
#if !defined(_XBOX_ONE) || !defined(_TITLE)
    if (m_deviceResources->GetColorSpace() != DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
        return;
#endif

    m_toneMapMode += 1;

    if (m_toneMapMode >= ToneMapPostProcess::Operator_Max)
    {
        m_toneMapMode = ToneMapPostProcess::Saturate;
    }
}
