//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK SpriteBatch
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
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
#ifdef GAMMA_CORRECT_RENDERING
    // Linear colors for DirectXMath were not added until v3.17 in the Windows SDK (22621)
    const XMVECTORF32 c_clearColor = { { { 0.127437726f, 0.300543845f, 0.846873462f, 1.f } } };
#else
    const XMVECTORF32 c_clearColor = Colors::CornflowerBlue;
#endif

    inline float randf()
    {
        return (float)rand() / (float)RAND_MAX * 10000;
    }
}

Game::Game() noexcept(false)
{
#ifdef GAMMA_CORRECT_RENDERING
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif

    // 2D only rendering
#ifdef XBOX
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_UNKNOWN, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(UWP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_UNKNOWN, 2, D3D_FEATURE_LEVEL_9_3,
        DX::DeviceResources::c_Enable4K_Xbox | DX::DeviceResources::c_EnableQHD_Xbox
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(c_RenderFormat, DXGI_FORMAT_UNKNOWN);
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

    if (kb.Left || (pad.IsConnected() && pad.dpad.left))
    {
        m_spriteBatch->SetRotation(DXGI_MODE_ROTATION_ROTATE270);
        assert(m_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_ROTATE270);
    }
    else if (kb.Right || (pad.IsConnected() && pad.dpad.right))
    {
        m_spriteBatch->SetRotation(DXGI_MODE_ROTATION_ROTATE90);
        assert(m_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_ROTATE90);
    }
    else if (kb.Up || (pad.IsConnected() && pad.dpad.up))
    {
        m_spriteBatch->SetRotation(DXGI_MODE_ROTATION_IDENTITY);
        assert(m_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_IDENTITY);
    }
    else if (kb.Down || (pad.IsConnected() && pad.dpad.down))
    {
        m_spriteBatch->SetRotation(DXGI_MODE_ROTATION_ROTATE180);
        assert(m_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_ROTATE180);
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

    XMVECTORF32 red, blue, gray, lime, pink, lsgreen;
#ifdef GAMMA_CORRECT_RENDERING
    red.v = XMColorSRGBToRGB(Colors::Red);
    blue.v = XMColorSRGBToRGB(Colors::Blue);
    gray.v = XMColorSRGBToRGB(Colors::Gray);
    lime.v = XMColorSRGBToRGB(Colors::Lime);
    pink.v = XMColorSRGBToRGB(Colors::Pink);
    lsgreen.v = XMColorSRGBToRGB(Colors::LightSeaGreen);
#else
    red.v = Colors::Red;
    blue.v = Colors::Blue;
    gray.v = Colors::Gray;
    lime.v = Colors::Lime;
    pink.v = Colors::Pink;
    lsgreen.v = Colors::LightSeaGreen;
#endif

    float time = 60.f * static_cast<float>(m_timer.GetTotalSeconds());

    m_spriteBatch->Begin(SpriteSortMode_Deferred, m_states->NonPremultiplied());

    // Moving
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(900, 384.f + sinf(time / 60.f)*384.f), nullptr, Colors::White, 0.f, XMFLOAT2(128, 128), 1, SpriteEffects_None, 0);

    // Spinning.
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(200, 150), nullptr, Colors::White, time / 100, XMFLOAT2(128, 128), 1, SpriteEffects_None, 0);

    // Zero size source region.
    RECT src = { 128, 128, 128, 140 };
    RECT dest = { 400, 150, 450, 200 };

    m_spriteBatch->Draw(m_cat.Get(), dest, &src, Colors::White, time / 100, XMFLOAT2(0, 6), SpriteEffects_None, 0);

    // Differently scaled.
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(0, 0), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.5);

    RECT dest1 = { 0, 0, 256, 64 };
    Rectangle dest2 = { 0, 0, 64, 256 };

    m_spriteBatch->Draw(m_cat.Get(), dest1);
    m_spriteBatch->Draw(m_cat.Get(), dest2);

    // Mirroring.
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(300, 10), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.3f, SpriteEffects_None);
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(350, 10), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.3f, SpriteEffects_FlipHorizontally);
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(400, 10), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.3f, SpriteEffects_FlipVertically);
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(450, 10), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.3f, SpriteEffects_FlipBoth);

    // Sorting.
    m_spriteBatch->Draw(m_letterA.Get(), XMFLOAT2(10, 280), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.1f);
    m_spriteBatch->Draw(m_letterC.Get(), XMFLOAT2(15, 290), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.9f);
    m_spriteBatch->Draw(m_letterB.Get(), XMFLOAT2(15, 285), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.5f);

    m_spriteBatch->Draw(m_letterA.Get(), XMFLOAT2(50, 280), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.9f);
    m_spriteBatch->Draw(m_letterC.Get(), XMFLOAT2(55, 290), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.1f);
    m_spriteBatch->Draw(m_letterB.Get(), XMFLOAT2(55, 285), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.5f);

    RECT source = { 16, 32, 256, 192 };

    // Draw overloads specifying position, origin and scale as XMFLOAT2.
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(-40, 320), red);

    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(200, 320), nullptr, lime, time / 500, XMFLOAT2(32, 128), 0.5f, SpriteEffects_None, 0.5f);
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(300, 320), &source, lime, time / 500, XMFLOAT2(120, 80), 0.5f, SpriteEffects_None, 0.5f);

    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(350, 320), nullptr, blue, time / 500, XMFLOAT2(32, 128), XMFLOAT2(0.25f, 0.5f), SpriteEffects_None, 0.5f);
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(450, 320), &source, blue, time / 500, XMFLOAT2(120, 80), XMFLOAT2(0.5f, 0.25f), SpriteEffects_None, 0.5f);

    // Draw overloads specifying position, origin and scale via the first two components of an XMVECTOR.
    m_spriteBatch->Draw(m_cat.Get(), XMVectorSet(0, 450, randf(), randf()), pink);

    m_spriteBatch->Draw(m_cat.Get(), XMVectorSet(200, 450, randf(), randf()), nullptr, lime, time / 500, XMVectorSet(32, 128, randf(), randf()), 0.5f, SpriteEffects_None, 0.5f);
    m_spriteBatch->Draw(m_cat.Get(), XMVectorSet(300, 450, randf(), randf()), &source, lime, time / 500, XMVectorSet(120, 80, randf(), randf()), 0.5f, SpriteEffects_None, 0.5f);

    m_spriteBatch->Draw(m_cat.Get(), XMVectorSet(350, 450, randf(), randf()), nullptr, blue, time / 500, XMVectorSet(32, 128, randf(), randf()), XMVectorSet(0.25f, 0.5f, randf(), randf()), SpriteEffects_None, 0.5f);
    m_spriteBatch->Draw(m_cat.Get(), XMVectorSet(450, 450, randf(), randf()), &source, blue, time / 500, XMVectorSet(120, 80, randf(), randf()), XMVectorSet(0.5f, 0.25f, randf(), randf()), SpriteEffects_None, 0.5f);

    // Draw overloads specifying position as a RECT.
    RECT rc1 = { 500, 320, 600, 420 };
    RECT rc2 = { 550, 450, 650, 550 };
    RECT rc3 = { 550, 550, 650, 650 };

    m_spriteBatch->Draw(m_cat.Get(), rc1, gray);

    m_spriteBatch->Draw(m_cat.Get(), rc2, nullptr, lsgreen, time / 300, XMFLOAT2(128, 128), SpriteEffects_None, 0.5f);
    m_spriteBatch->Draw(m_cat.Get(), rc3, &source, lsgreen, time / 300, XMFLOAT2(128, 128), SpriteEffects_None, 0.5f);

    m_spriteBatch->End();

    // Test alt samplers
    RECT tileRect = { long(256), long(256), long(256 * 3), long(256 * 3) };

    m_spriteBatch->Begin(SpriteSortMode_Deferred, m_states->NonPremultiplied(), m_states->PointClamp());
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(1100.f, 100.f), nullptr, Colors::White, time / 50, XMFLOAT2(128, 128));
    m_spriteBatch->End();

    m_spriteBatch->Begin(SpriteSortMode_Deferred, m_states->NonPremultiplied(), m_states->AnisotropicClamp());
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(1100.f, 350.f), &tileRect, Colors::White, time / 50, XMFLOAT2(256, 256));
    m_spriteBatch->End();

    m_spriteBatch->Begin(SpriteSortMode_Deferred, m_states->NonPremultiplied(), m_states->AnisotropicWrap());
    m_spriteBatch->Draw(m_cat.Get(), XMFLOAT2(1100.f, 600.f), &tileRect, Colors::White, time / 50, XMFLOAT2(256, 256));
    m_spriteBatch->End();

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

    context->ClearRenderTargetView(renderTarget, c_clearColor);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    // Set the viewport.
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

    auto context = m_deviceResources->GetD3DDeviceContext();

    m_spriteBatch = std::make_unique<SpriteBatch>(context);

#ifdef GAMMA_CORRECT_RENDERING
    DDS_LOADER_FLAGS loadFlags = DDS_LOADER_FORCE_SRGB;
#else
    DDS_LOADER_FLAGS loadFlags = DDS_LOADER_DEFAULT;
#endif

    // Load textures.
    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"cat.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, loadFlags,
        nullptr, m_cat.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"a.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, loadFlags,
        nullptr, m_letterA.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"b.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, loadFlags,
        nullptr, m_letterB.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"c.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, loadFlags,
        nullptr, m_letterC.ReleaseAndGetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    const auto viewport = m_deviceResources->GetScreenViewport();

    m_spriteBatch->SetViewport(viewport);

#ifdef XBOX
    if (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_Enable4K_UHD)
    {
        // Scale sprite batch rendering when running 4k
        static const D3D11_VIEWPORT s_vp1080 = { 0.f, 0.f, 1920.f, 1080.f, D3D11_MIN_DEPTH, D3D11_MAX_DEPTH };
        m_spriteBatch->SetViewport(s_vp1080);
    }
#elif defined(UWP)
    if (m_deviceResources->GetDeviceOptions() & (DX::DeviceResources::c_Enable4K_Xbox | DX::DeviceResources::c_EnableQHD_Xbox))
    {
        // Scale sprite batch rendering when running 4k or 1440p
        static const D3D11_VIEWPORT s_vp1080 = { 0.f, 0.f, 1920.f, 1080.f, D3D11_MIN_DEPTH, D3D11_MAX_DEPTH };
        m_spriteBatch->SetViewport(s_vp1080);
    }

    auto rotation = m_deviceResources->GetRotation();
    m_spriteBatch->SetRotation(rotation);
#endif
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_states.reset();
    m_spriteBatch.reset();

    m_cat.Reset();
    m_letterA.Reset();
    m_letterB.Reset();
    m_letterC.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
