//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK PBR Model Test
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

namespace
{
    const float row0 = 2.f;
    const float row1 = 0.f;
    const float row2 = -2.f;
}

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

// Constructor.
Game::Game() noexcept(false) :
    m_ibl(0),
    m_spinning(true),
    m_pitch(0),
    m_yaw(0)
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

    auto time = static_cast<float>(m_timer.GetTotalSeconds());

    float alphaFade = (sin(time * 2) + 1) / 2;

    if (alphaFade >= 1)
        alphaFade = 1 - FLT_EPSILON;

    float yaw = time * 0.4f;
    float pitch = time * 0.7f;
    float roll = time * 1.1f;

    XMMATRIX world;
    XMVECTOR quat;

    if (m_spinning)
    {
        world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
        quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
    }
    else
    {
        world = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0);
        quat = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, roll);
    }

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    //--- Set PBR lighting sources ---
    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    m_radianceIBL[m_ibl]->GetDesc(&desc);

    m_cube->UpdateEffects([&](IEffect* effect)
    {
        auto pbr = dynamic_cast<PBREffect*>(effect);
        if (pbr)
        {
            pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), desc.TextureCube.MipLevels, m_irradianceIBL[m_ibl].Get());
            pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), desc.TextureCube.MipLevels, m_irradianceIBL[m_ibl].Get());
        }
    });

    //--- Draw SDKMESH models ---
    XMMATRIX local = XMMatrixTranslation(1.5f, row0, 0.f);
    local = XMMatrixMultiply(world, local);
    m_cube->Draw(context, *m_states, local, m_view, m_projection);

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

    m_hdrScene->SetDevice(device);

    m_toneMap = std::make_unique<ToneMapPostProcess>(device);
    m_toneMap->SetOperator(ToneMapPostProcess::ACESFilmic);
    m_toneMap->SetTransferFunction(ToneMapPostProcess::SRGB);

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_toneMap->SetMRTOutput(true);
#endif

    m_states = std::make_unique<CommonStates>(device);

    m_fxFactory = std::make_unique<PBREffectFactory>(device);

#ifdef LH_COORDS
    bool ccw = false;
#else
    bool ccw = true;
#endif

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#define IBL_PATH L"..\\PBRTest\\"
#else
#define IBL_PATH
#endif

    static const wchar_t* s_radianceIBL[s_nIBL] =
    {
        IBL_PATH L"Atrium_diffuseIBL.dds",
        IBL_PATH L"Garage_diffuseIBL.dds",
        IBL_PATH L"SunSubMixer_diffuseIBL.dds",
    };
    static const wchar_t* s_irradianceIBL[s_nIBL] =
    {
        IBL_PATH L"Atrium_specularIBL.dds",
        IBL_PATH L"Garage_specularIBL.dds",
        IBL_PATH L"SunSubMixer_specularIBL.dds",
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

    // DirectX SDK Mesh
    m_cube = Model::CreateFromSDKMESH(device, L"BrokenCube.sdkmesh", *m_fxFactory, !ccw);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { 0, 0, 6 };

    auto size = m_deviceResources->GetOutputSize();
    float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    m_view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 15);
#else
    m_view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 15);
#endif

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    {
        auto orient3d = m_deviceResources->GetOrientationTransform3D();
        XMMATRIX orient = XMLoadFloat4x4(&orient3d);
        m_projection *= orient;
    }
#endif

    // Set windows size for HDR.
    m_hdrScene->SetWindow(size);

    m_toneMap->SetHDRSourceTexture(m_hdrScene->GetShaderResourceView());
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnDeviceLost()
{
    m_states.reset();

    for (size_t j = 0; j < s_nIBL; ++j)
    {
        m_radianceIBL[j].Reset();
        m_irradianceIBL[j].Reset();
    }

    m_cube.reset();

    m_fxFactory.reset();

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
