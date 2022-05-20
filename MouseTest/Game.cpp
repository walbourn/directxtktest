//--------------------------------------------------------------------------------------
// File: MouseTest.cpp
//
// Developer unit test for DirectXTK Mouse
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"

#include "FindMedia.h"

#define GAMMA_CORRECT_RENDERING
#define USE_FAST_SEMANTICS

// Enable to test using always relative mode and not using absolute
//#define TEST_LOCKED_RELATIVE

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr XMVECTORF32 START_POSITION = { { { 0.f, -1.5f, 0.f, 0.f } } };
    constexpr XMVECTORF32 ROOM_BOUNDS = { { { 8.f, 6.f, 12.f, 0.f } } };
}

static_assert(std::is_nothrow_move_constructible<Mouse>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<Mouse>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<Mouse::ButtonStateTracker>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<Mouse::ButtonStateTracker>::value, "Move Assign.");

// Constructor.
Game::Game() noexcept(false) :
    m_ms{},
    m_lastMode(Mouse::MODE_ABSOLUTE),
    m_pitch(0),
    m_yaw(0),
    m_lastStr(nullptr)
{
    m_cameraPos = START_POSITION.v;

#ifdef GAMMA_CORRECT_RENDERING
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
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
        c_RenderFormat, DXGI_FORMAT_D24_UNORM_S8_UINT, 2, D3D_FEATURE_LEVEL_9_3,
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
    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();

#ifdef XBOX
    UNREFERENCED_PARAMETER(rotation);
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    m_deviceResources->SetWindow(window);
#ifdef COREWINDOW
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
    m_mouse->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#endif
#elif defined(UWP)
    m_deviceResources->SetWindow(window, width, height, rotation);
    m_mouse->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#else
    UNREFERENCED_PARAMETER(rotation);
    m_deviceResources->SetWindow(window, width, height);
    m_mouse->SetWindow(window);
#endif

#ifdef TEST_LOCKED_RELATIVE
    m_mouse->SetMode(Mouse::MODE_RELATIVE);
#endif

#ifdef USING_COREWINDOW
    OutputDebugStringA("INFO: Using CoreWindow\n");
#elif defined(USING_GAMEINPUT)
    OutputDebugStringA("INFO: Using GameInput\n");
#else
    OutputDebugStringA("INFO: Using Win32 messages\n");
#endif

    // Singleton test
    {
        bool thrown = false;

        try
        {
            std::unique_ptr<Mouse> mouse2(new Mouse);
        }
        catch (...)
        {
            thrown = true;
        }

        if (!thrown)
        {
#ifdef PC
            MessageBoxW(window, L"Mouse not acting like a singleton", L"MouseTest", MB_ICONERROR);
#else
            throw std::runtime_error("Mouse not acting like a singleton");
#endif
        }
    }

    OutputDebugStringA(m_mouse->IsConnected() ? "INFO: Mouse is connected\n" : "INFO: No mouse found\n");
    OutputDebugStringA(m_mouse->IsVisible() ? "INFO: Mouse cursor is visible on startup\n" : "INFO: Mouse cursor NOT visible on startup\n");

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

    if (kb.Escape)
    {
        ExitGame();
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
    {
        const bool isvisible = m_mouse->IsVisible();
        m_mouse->SetVisible(!isvisible);

        OutputDebugStringA(m_mouse->IsVisible() ? "INFO: Mouse cursor is visible\n" : "INFO: Mouse cursor NOT visible\n");
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Home))
    {
        m_mouse->ResetScrollWheelValue();
    }
    else if (m_keyboardButtons.IsKeyPressed(Keyboard::End))
    {
#ifndef TEST_LOCKED_RELATIVE
        if (m_lastMode == Mouse::MODE_ABSOLUTE)
        {
            m_mouse->SetMode(Mouse::MODE_RELATIVE);
        }
        else
        {
            m_mouse->SetMode(Mouse::MODE_ABSOLUTE);
        }
#endif
    }

    auto mouse = m_mouse->GetState();
    m_lastMode = mouse.positionMode;

    if (mouse.positionMode == Mouse::MODE_RELATIVE)
    {
        const SimpleMath::Vector4 ROTATION_GAIN(0.004f, 0.004f, 0.f, 0.f);
        const XMVECTOR delta = SimpleMath::Vector4(float(mouse.x), float(mouse.y), 0.f, 0.f) * ROTATION_GAIN;

        m_pitch -= XMVectorGetY(delta);
        m_yaw -= XMVectorGetX(delta);

        // limit pitch to straight up or straight down
        float limit = XM_PI / 2.0f - 0.01f;
        m_pitch = std::max(-limit, m_pitch);
        m_pitch = std::min(+limit, m_pitch);

        // keep longitude in sane range by wrapping
        if (m_yaw > XM_PI)
        {
            m_yaw -= XM_PI * 2.0f;
        }
        else if (m_yaw < -XM_PI)
        {
            m_yaw += XM_PI * 2.0f;
        }
    }

    m_tracker.Update(mouse);

    using ButtonState = Mouse::ButtonStateTracker::ButtonState;

    if (m_tracker.leftButton == ButtonState::PRESSED)
        m_lastStr = L"LeftButton was pressed";
    else if (m_tracker.leftButton == ButtonState::RELEASED)
        m_lastStr = L"LeftButton was released";
    else if (m_tracker.rightButton == ButtonState::PRESSED)
        m_lastStr = L"RightButton was pressed";
    else if (m_tracker.rightButton == ButtonState::RELEASED)
        m_lastStr = L"RightButton was released";
    else if (m_tracker.middleButton == ButtonState::PRESSED)
        m_lastStr = L"MiddleButton was pressed";
    else if (m_tracker.middleButton == ButtonState::RELEASED)
        m_lastStr = L"MiddleButton was released";
    else if (m_tracker.xButton1 == ButtonState::PRESSED)
        m_lastStr = L"XButton1 was pressed";
    else if (m_tracker.xButton1 == ButtonState::RELEASED)
        m_lastStr = L"XButton1 was released";
    else if (m_tracker.xButton2 == ButtonState::PRESSED)
        m_lastStr = L"XButton2 was pressed";
    else if (m_tracker.xButton2 == ButtonState::RELEASED)
        m_lastStr = L"XButton2 was released";

#ifndef TEST_LOCKED_RELATIVE
    if (m_tracker.leftButton == ButtonState::PRESSED)
    {
        m_mouse->SetMode(Mouse::MODE_RELATIVE);
    }
    else if (m_tracker.leftButton == ButtonState::RELEASED)
    {
        m_mouse->SetMode(Mouse::MODE_ABSOLUTE);
    }
#endif

    m_ms = mouse;
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

    XMVECTORF32 red, blue, lightGray, yellow;
#ifdef GAMMA_CORRECT_RENDERING
    red.v = XMColorSRGBToRGB(Colors::Red);
    blue.v = XMColorSRGBToRGB(Colors::Blue);
    lightGray.v = XMColorSRGBToRGB(Colors::LightGray);
    yellow.v = XMColorSRGBToRGB(Colors::Yellow);
#else
    red.v = Colors::Red;
    blue.v = Colors::Blue;
    lightGray.v = Colors::LightGray;
    yellow.v = Colors::Yellow;
#endif

    const float y = sinf(m_pitch);      // vertical
    const float r = cosf(m_pitch);      // in the plane
    const float z = r * cosf(m_yaw);    // fwd-back
    const float x = r * sinf(m_yaw);    // left-right

    XMVECTOR lookAt = XMVectorAdd(m_cameraPos, XMVectorSet(x, y, z, 0.f));

    XMMATRIX view = XMMatrixLookAtRH(m_cameraPos, lookAt, Vector3::Up);

    m_room->Draw(Matrix::Identity, view, m_proj, Colors::White, m_roomTex.Get());

    const XMVECTOR xsize = m_comicFont->MeasureString(L"X");

    const float height = XMVectorGetY(xsize);

    m_spriteBatch->Begin();

    XMFLOAT2 pos(50, 50);

    // Buttons
    m_comicFont->DrawString(m_spriteBatch.get(), L"LeftButton", pos, m_ms.leftButton ? red : lightGray);
    pos.y += height * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"RightButton", pos, m_ms.rightButton ? red : lightGray);
    pos.y += height * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"MiddleButton", pos, m_ms.middleButton ? red : lightGray);
    pos.y += height * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"XButton1", pos, m_ms.xButton1 ? red : lightGray);
    pos.y += height * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"XButton2", pos, m_ms.xButton2 ? red : lightGray);

    // Scroll Wheel
    pos.y += height * 2;
    {
        wchar_t buff[16] = {};
        swprintf_s(buff, L"%d", m_ms.scrollWheelValue);
        m_comicFont->DrawString(m_spriteBatch.get(), buff, pos, Colors::Black);
    }

    m_comicFont->DrawString(m_spriteBatch.get(), (m_ms.positionMode == Mouse::MODE_RELATIVE) ? L"Relative" : L"Absolute",
        XMFLOAT2(50, 550), blue);

    if (m_lastStr)
    {
        m_comicFont->DrawString(m_spriteBatch.get(), m_lastStr, XMFLOAT2(50, 600), yellow);
    }

    if (m_ms.positionMode == Mouse::MODE_ABSOLUTE)
    {
        m_spriteBatch->Draw(m_cursor.Get(), XMFLOAT2((float)m_ms.x, (float)m_ms.y));
    }

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
    auto const viewport = m_deviceResources->GetScreenViewport();
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
    m_deviceResources->Suspend();
}

void Game::OnResuming()
{
    m_deviceResources->Resume();

    m_tracker.Reset();
    m_keyboardButtons.Reset();
    m_timer.ResetElapsedTime();
}

#ifdef PC
void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
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

    auto context = m_deviceResources->GetD3DDeviceContext();

    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"comic.spritefont");
    m_comicFont = std::make_unique<SpriteFont>(device, strFilePath);

    m_room = GeometricPrimitive::CreateBox(context, XMFLOAT3(ROOM_BOUNDS[0], ROOM_BOUNDS[1], ROOM_BOUNDS[2]), false, true);

#ifdef GAMMA_CORRECT_RENDERING
    bool forceSRGB = true;
#else
    bool forceSRGB = false;
#endif

    DX::FindMediaFile(strFilePath, MAX_PATH, L"texture.dds");
    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, strFilePath, 0,
        D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
        forceSRGB, nullptr, m_roomTex.GetAddressOf()));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"arrow.png");
    DX::ThrowIfFailed(CreateWICTextureFromFile(device, strFilePath, nullptr, m_cursor.GetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
#ifdef XBOX
    if (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_Enable4K_UHD)
    {
#ifdef GAMEINPUT
        Mouse::SetResolution(true);
#else
        Mouse::SetDpi(192.);
#endif
    }
#elif defined(UWP)
    if (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_Enable4K_Xbox)
    {
        Mouse::SetDpi(192.f);
    }
    else if (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableQHD_Xbox)
    {
        Mouse::SetDpi(128.f);
    }
#endif

    auto const size = m_deviceResources->GetOutputSize();
    m_proj = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(70.f), float(size.right) / float(size.bottom), 0.01f, 100.f);

    auto const viewPort = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewPort);

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

    m_spriteBatch->SetRotation(m_deviceResources->GetRotation());
#endif
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_room.reset();
    m_spriteBatch.reset();
    m_comicFont.reset();

    m_roomTex.Reset();
    m_cursor.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
