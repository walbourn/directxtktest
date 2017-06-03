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

//#define GAMMA_CORRECT_RENDERING
//#define USE_FAST_SEMANTICS

// Build for LH vs. RH coords
//#define LH_COORDS

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const int MaxScene = 4;
}

// Constructor.
Game::Game() :
    m_scene(0)
{
#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, 2, true);
#elif defined(GAMMA_CORRECT_RENDERING)
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0);
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
void Game::Update(DX::StepTimer const& timer)
{
    auto pad = m_gamePad->GetState(0);
    auto kb = m_keyboard->GetState();
    if (kb.Escape || (pad.IsConnected() && pad.IsViewPressed()))
    {
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
        PostQuitMessage(0);
#else
        Windows::ApplicationModel::Core::CoreApplication::Exit();
#endif
    }

    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            ++m_scene;
            if (m_scene >= MaxScene)
                m_scene = 0;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    m_keyboardButtons.Update(kb);

    if ( m_keyboardButtons.IsKeyPressed(Keyboard::Space))
    {
        ++m_scene;
        if (m_scene >= MaxScene)
            m_scene = 0;
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

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources->Prepare();
#endif

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    m_spriteBatch->Begin();
    m_spriteBatch->Draw(m_background.Get(), m_deviceResources->GetOutputSize());
    m_spriteBatch->End();

    m_shape->Draw(m_world, m_view, m_proj, Colors::White, m_texture.Get());

    // Post process.
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    const wchar_t* descstr = nullptr;
    switch (m_scene)
    {
    case 0:
    default:
        descstr = L"Copy (passthrough)";
        m_basicPostProcess->Set(BasicPostProcess::Copy);
        break;

    case 1:
        descstr = L"Monochrome";
        m_basicPostProcess->Set(BasicPostProcess::Monochrome);
        break;

    case 2:
        descstr = L"Downscale 2x2";
        m_basicPostProcess->Set(BasicPostProcess::DownScale_2x2);
        break;

    case 3:
        descstr = L"Downscale 4x4";
        m_basicPostProcess->Set(BasicPostProcess::DownScale_4x4);
        break;
    }

    m_basicPostProcess->Process(context);

    // Draw UI.
    auto size = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(size.right, size.bottom);

    m_spriteBatch->Begin();
    m_font->DrawString(m_spriteBatch.get(), descstr, XMFLOAT2(float(safeRect.left), float(safeRect.bottom - m_font->GetLineSpacing())));
    m_spriteBatch->End();

    // Clear binding to avoid SDK debug warning
    ID3D11ShaderResourceView* nullsrv[] = { nullptr, nullptr };
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

    // Setup post processing
    m_basicPostProcess = std::make_unique<BasicPostProcess>(device);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();

    UINT width = size.right - size.left;
    UINT height = size.bottom - size.top;

    // Create additional render targets
    auto device = m_deviceResources->GetD3DDevice();

    CD3D11_TEXTURE2D_DESC sceneDesc(
        DXGI_FORMAT_R16G16B16A16_FLOAT, width, height,
        1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

    DX::ThrowIfFailed(device->CreateTexture2D(&sceneDesc, nullptr, m_sceneTex.GetAddressOf()));

    DX::ThrowIfFailed(device->CreateRenderTargetView(m_sceneTex.Get(), nullptr, m_sceneRT.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(device->CreateShaderResourceView(m_sceneTex.Get(), nullptr, m_sceneSRV.ReleaseAndGetAddressOf()));

    m_basicPostProcess->SetSourceTexture(m_sceneSRV.Get());

    // Setup matrices
    m_view = Matrix::CreateLookAt(Vector3(2.f, 2.f, 2.f),
        Vector3::Zero, Vector3::UnitY);

    m_proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f,
        float(size.right) / float(size.bottom), 0.1f, 10.f);
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnDeviceLost()
{
    m_spriteBatch.reset();
    m_font.reset();
    m_shape.reset();
    m_texture.Reset();
    m_background.Reset();

    m_sceneTex.Reset();
    m_sceneSRV.Reset();
    m_sceneRT.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
