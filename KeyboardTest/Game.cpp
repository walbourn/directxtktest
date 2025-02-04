//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK Keyboard
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

    constexpr XMVECTORF32 START_POSITION = { { { 0.f, -1.5f, 0.f, 0.f } } };
    constexpr XMVECTORF32 ROOM_BOUNDS = { { { 8.f, 6.f, 12.f, 0.f } } };

    constexpr float MOVEMENT_GAIN = 0.07f;

    constexpr int c_oemLowOffset = 0xBA;
    constexpr const wchar_t* s_oemLow[] =
    {
        L"Semicolon",   // Oem 1
        L"Plus",
        L"Comma",
        L"Minus",
        L"Period",
        L"Question",    // Oem 2
        L"Tilde",       // Oem 3
    };
}

static_assert(std::is_nothrow_move_constructible<Keyboard>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<Keyboard>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<Keyboard::KeyboardStateTracker>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<Keyboard::KeyboardStateTracker>::value, "Move Assign.");

// Constructor.
Game::Game() noexcept(false) :
    m_kb{},
    m_lastStr(nullptr),
    m_lastStrBuff{}
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
            auto kb2 = std::make_unique<Keyboard>();
        }
        catch (...)
        {
            thrown = true;
        }

        if (!thrown)
        {
#ifdef PC
            MessageBoxW(window, L"Keyboard not acting like a singleton", L"KeyboardTest", MB_ICONERROR);
#else
            throw std::runtime_error("Keyboard not acting like a singleton");
#endif
        }
    }

    OutputDebugStringA(m_keyboard->IsConnected() ? "INFO: Keyboard is connected\n" : "INFO: No keyboard found\n");

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

    if (kb.Escape)
    {
        ExitGame();
    }

    if (kb.Home)
        m_keyboard->Reset();

    m_tracker.Update(kb);

    if (m_tracker.pressed.Q)
        m_lastStr = L"Q was pressed";
    else if (m_tracker.released.Q)
        m_lastStr = L"Q was released";
    else if (m_tracker.pressed.W)
        m_lastStr = L"W was pressed";
    else if (m_tracker.released.W)
        m_lastStr = L"W was released";
    else if (m_tracker.pressed.E)
        m_lastStr = L"E was pressed";
    else if (m_tracker.released.E)
        m_lastStr = L"E was released";
    else if (m_tracker.pressed.R)
        m_lastStr = L"R was pressed";
    else if (m_tracker.released.R)
        m_lastStr = L"R was released";
    else if (m_tracker.pressed.T)
        m_lastStr = L"T was pressed";
    else if (m_tracker.released.T)
        m_lastStr = L"T was released";
    else if (m_tracker.pressed.Y)
        m_lastStr = L"Y was pressed";
    else if (m_tracker.released.Y)
        m_lastStr = L"Y was released";
    else if (m_tracker.pressed.LeftShift)
        m_lastStr = L"LeftShift was pressed";
    else if (m_tracker.released.LeftShift)
        m_lastStr = L"LeftShift was released";
    else if (m_tracker.pressed.LeftAlt)
        m_lastStr = L"LeftAlt was pressed";
    else if (m_tracker.released.LeftAlt)
        m_lastStr = L"LeftAlt was released";
    else if (m_tracker.pressed.LeftControl)
        m_lastStr = L"LeftCtrl was pressed";
    else if (m_tracker.released.LeftControl)
        m_lastStr = L"LeftCtrl was released";
    else if (m_tracker.pressed.RightShift)
        m_lastStr = L"RightShift was pressed";
    else if (m_tracker.released.RightShift)
        m_lastStr = L"RightShift was released";
    else if (m_tracker.pressed.RightAlt)
        m_lastStr = L"RightAlt was pressed";
    else if (m_tracker.released.RightAlt)
        m_lastStr = L"RightAlt was released";
    else if (m_tracker.pressed.RightControl)
        m_lastStr = L"RightCtrl was pressed";
    else if (m_tracker.released.RightControl)
        m_lastStr = L"RightCtrl was released";
    else if (m_tracker.pressed.Space)
        m_lastStr = L"Space was pressed";
    else if (m_tracker.released.Space)
        m_lastStr = L"Space was released";
    else if (m_tracker.pressed.Enter)
        m_lastStr = L"Enter was pressed";
    else if (m_tracker.released.Enter)
        m_lastStr = L"Enter was released";
    else if (m_tracker.pressed.Back)
        m_lastStr = L"Back was pressed";
    else if (m_tracker.released.Back)
        m_lastStr = L"Back was released";
    else if (m_tracker.pressed.NumLock)
        m_lastStr = L"NumLock was pressed";
    else if (m_tracker.released.NumLock)
        m_lastStr = L"NumLock was released";
    else if (m_tracker.pressed.CapsLock)
        m_lastStr = L"CapsLock was pressed";
    else if (m_tracker.released.CapsLock)
        m_lastStr = L"CapsLock was released";
    else if (m_tracker.pressed.Scroll)
        m_lastStr = L"Scroll was pressed";
    else if (m_tracker.released.Scroll)
        m_lastStr = L"Scroll was released";
    else if (m_tracker.pressed.Up)
        m_lastStr = L"Up was pressed";
    else if (m_tracker.released.Up)
        m_lastStr = L"Up was released";
    else if (m_tracker.pressed.Down)
        m_lastStr = L"Down was presssed";
    else if (m_tracker.released.Down)
        m_lastStr = L"Down was released";
    else if (m_tracker.pressed.Left)
        m_lastStr = L"Left was pressed";
    else if (m_tracker.released.Left)
        m_lastStr = L"Left was released";
    else if (m_tracker.pressed.Right)
        m_lastStr = L"Right was pressed";
    else if (m_tracker.released.Right)
        m_lastStr = L"Right was released";
    else if (m_tracker.pressed.PageUp)
        m_lastStr = L"PageUp was pressed";
    else if (m_tracker.released.PageUp)
        m_lastStr = L"PageUp was released";
    else if (m_tracker.pressed.PageDown)
        m_lastStr = L"PageDown was pressed";
    else if (m_tracker.released.PageDown)
        m_lastStr = L"PageDown was released";

    for (int vk = VK_F1; vk <= VK_F10; ++vk)
    {
        if (m_tracker.IsKeyPressed(static_cast<DirectX::Keyboard::Keys>(vk)))
        {
            swprintf_s(m_lastStrBuff, L"F%d was pressed", vk - VK_F1 + 1);
            m_lastStr = m_lastStrBuff;
        }
        else if (m_tracker.IsKeyReleased(static_cast<DirectX::Keyboard::Keys>(vk)))
        {
            swprintf_s(m_lastStrBuff, L"F%d was released", vk - VK_F1 + 1);
            m_lastStr = m_lastStrBuff;
        }
    }

    for (int vk = 0x30; vk <= 0x39; ++vk)
    {
        if (m_tracker.IsKeyPressed(static_cast<DirectX::Keyboard::Keys>(vk)))
        {
            swprintf_s(m_lastStrBuff, L"%d was pressed", vk - 0x30);
            m_lastStr = m_lastStrBuff;
        }
        else if (m_tracker.IsKeyReleased(static_cast<DirectX::Keyboard::Keys>(vk)))
        {
            swprintf_s(m_lastStrBuff, L"%d was released", vk - 0x30);
            m_lastStr = m_lastStrBuff;
        }
    }

    for (int vk = 0x60; vk <= 0x69; ++vk)
    {
        if (m_tracker.IsKeyPressed(static_cast<DirectX::Keyboard::Keys>(vk)))
        {
            swprintf_s(m_lastStrBuff, L"Numkey %d was pressed", vk - 0x60);
            m_lastStr = m_lastStrBuff;
        }
        else if (m_tracker.IsKeyReleased(static_cast<DirectX::Keyboard::Keys>(vk)))
        {
            swprintf_s(m_lastStrBuff, L"Numkey %d was released", vk - 0x60);
            m_lastStr = m_lastStrBuff;
        }
    }

    for (int vk = 0xBA; vk <= 0xC0; ++vk)
    {
        if (m_tracker.IsKeyPressed(static_cast<DirectX::Keyboard::Keys>(vk)))
        {
            swprintf_s(m_lastStrBuff, L"Oem%ls was pressed", s_oemLow[vk - c_oemLowOffset]);
            m_lastStr = m_lastStrBuff;
        }
        else if (m_tracker.IsKeyReleased(static_cast<DirectX::Keyboard::Keys>(vk)))
        {
            swprintf_s(m_lastStrBuff, L"Oem%ls was released", s_oemLow[vk - c_oemLowOffset]);
            m_lastStr = m_lastStrBuff;
        }
    }

    Vector3 move = Vector3::Zero;

    if (kb.Up)
        move.y += 1.f;

    if (kb.Down)
        move.y -= 1.f;

    if (kb.Left)
        move.x -= 1.f;

    if (kb.Right)
        move.x += 1.f;

    if (kb.PageUp)
        move.z += 1.f;

    if (kb.PageDown)
        move.z -= 1.f;

    move *= MOVEMENT_GAIN;

    m_cameraPos += move;

    const Vector3 halfBound = (Vector3(ROOM_BOUNDS.v) / Vector3(2.f)) - Vector3(0.1f, 0.1f, 0.1f);

    m_cameraPos = Vector3::Min(m_cameraPos, halfBound);
    m_cameraPos = Vector3::Max(m_cameraPos, -halfBound);

    m_kb = kb;
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

    XMVECTORF32 red, lightGray, yellow;
#ifdef GAMMA_CORRECT_RENDERING
    red.v = XMColorSRGBToRGB(Colors::Red);
    lightGray.v = XMColorSRGBToRGB(Colors::LightGray);
    yellow.v = XMColorSRGBToRGB(Colors::Yellow);
#else
    red.v = Colors::Red;
    lightGray.v = Colors::LightGray;
    yellow.v = Colors::Yellow;
#endif

    const XMVECTOR lookAt = XMVectorAdd(m_cameraPos, Vector3::Backward);

    const XMMATRIX view = XMMatrixLookAtRH(m_cameraPos, lookAt, Vector3::Up);

    m_room->Draw(Matrix::Identity, view, m_proj, Colors::White, m_roomTex.Get());

    const XMVECTOR xsize = m_comicFont->MeasureString(L"X");

    const float width = XMVectorGetX(xsize);
    const float height = XMVectorGetY(xsize);
    const float linespace = height * 1.5f;

    m_spriteBatch->Begin();

    XMFLOAT2 pos(50, 50);

    // Row 0
    for (int vk = VK_F1; vk <= VK_F10; ++vk)
    {
        wchar_t buff[5] = {};
        swprintf_s(buff, L"F%d", vk - VK_F1 + 1);
        m_comicFont->DrawString(m_spriteBatch.get(), buff, pos, m_kb.IsKeyDown(static_cast<DirectX::Keyboard::Keys>(vk)) ? red : lightGray);

        pos.x += width * 3;
    }

    // Row 1
    pos.x = 50;
    pos.y += linespace;

    for (int vk = 0x30; vk <= 0x39; ++vk)
    {
        wchar_t buff[3] = {};
        swprintf_s(buff, L"%d", vk - 0x30);
        m_comicFont->DrawString(m_spriteBatch.get(), buff, pos, m_kb.IsKeyDown(static_cast<DirectX::Keyboard::Keys>(vk)) ? red : lightGray);

        pos.x += width * 2;
    }

    pos.x += width * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"Q", pos, m_kb.Q ? red : lightGray);

    pos.x += width * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"W", pos, m_kb.W ? red : lightGray);

    pos.x += width * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"E", pos, m_kb.E ? red : lightGray);

    pos.x += width * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"R", pos, m_kb.R ? red : lightGray);

    pos.x += width * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"T", pos, m_kb.T ? red : lightGray);

    pos.x += width * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"Y", pos, m_kb.Y ? red : lightGray);

    // Row 2
    pos.x = 50;
    pos.y += linespace;

    m_comicFont->DrawString(m_spriteBatch.get(), L"LeftShift", pos, m_kb.LeftShift ? red : lightGray);

    pos.x += width * 10;

    m_comicFont->DrawString(m_spriteBatch.get(), L"RightShift", pos, m_kb.RightShift ? red : lightGray);

    pos.x += width * 10;

    m_comicFont->DrawString(m_spriteBatch.get(), L"NumLock", pos, m_kb.NumLock ? red : lightGray);

    pos.x += width * 10;

    for (int vk = 0x67; vk <= 0x69; ++vk)
    {
        wchar_t buff[3] = {};
        swprintf_s(buff, L"%d", vk - 0x60);
        m_comicFont->DrawString(m_spriteBatch.get(), buff, pos, m_kb.IsKeyDown(static_cast<DirectX::Keyboard::Keys>(vk)) ? red : lightGray);

        pos.x += width * 2;
    }

    // Row 3
    pos.x = 50;
    pos.y += linespace;

    m_comicFont->DrawString(m_spriteBatch.get(), L"LeftCtrl", pos, m_kb.LeftControl ? red : lightGray);

    pos.x += width * 10;

    m_comicFont->DrawString(m_spriteBatch.get(), L"RightCtrl", pos, m_kb.RightControl ? red : lightGray);

    pos.x += width * 10;

    m_comicFont->DrawString(m_spriteBatch.get(), L"CapsLock", pos, m_kb.CapsLock ? red : lightGray);

    pos.x += width * 10;

    for (int vk = 0x64; vk <= 0x66; ++vk)
    {
        wchar_t buff[3] = {};
        swprintf_s(buff, L"%d", vk - 0x60);
        m_comicFont->DrawString(m_spriteBatch.get(), buff, pos, m_kb.IsKeyDown(static_cast<DirectX::Keyboard::Keys>(vk)) ? red : lightGray);

        pos.x += width * 2;
    }

    // Row 4
    pos.x = 50;
    pos.y += linespace;

    m_comicFont->DrawString(m_spriteBatch.get(), L"LeftAlt", pos, m_kb.LeftAlt ? red : lightGray);

    pos.x += width * 10;

    m_comicFont->DrawString(m_spriteBatch.get(), L"RightAlt", pos, m_kb.RightAlt ? red : lightGray);

    pos.x += width * 10;

    m_comicFont->DrawString(m_spriteBatch.get(), L"Scroll", pos, m_kb.Scroll ? red : lightGray);

    pos.x += width * 10;

    for (int vk = 0x61; vk <= 0x63; ++vk)
    {
        wchar_t buff[3] = {};
        swprintf_s(buff, L"%d", vk - 0x60);
        m_comicFont->DrawString(m_spriteBatch.get(), buff, pos, m_kb.IsKeyDown(static_cast<DirectX::Keyboard::Keys>(vk)) ? red : lightGray);

        pos.x += width * 2;
    }

    // Row 5
    pos.x = 50;
    pos.y += linespace;

    m_comicFont->DrawString(m_spriteBatch.get(), L"Space", pos, m_kb.Space ? red : lightGray);

    pos.x += width * 10;

    m_comicFont->DrawString(m_spriteBatch.get(), L"Enter", pos, m_kb.Enter ? red : lightGray);

    pos.x += width * 10;

    m_comicFont->DrawString(m_spriteBatch.get(), L"Back", pos, m_kb.Back ? red : lightGray);

    pos.x += width * 10;

    m_comicFont->DrawString(m_spriteBatch.get(), "0", pos, m_kb.IsKeyDown(DirectX::Keyboard::Keys::NumPad0) ? red : lightGray);

    // Row 6
    pos.x = 50;
    pos.y += linespace;

    for (int vk = 0xBA; vk <= 0xC0; ++vk)
    {
        wchar_t buff[16] = {};
        swprintf_s(buff, L"Oem%ls", s_oemLow[vk - c_oemLowOffset]);

        m_comicFont->DrawString(m_spriteBatch.get(), buff, pos, m_kb.IsKeyDown(static_cast<DirectX::Keyboard::Keys>(vk)) ? red : lightGray);

        if (vk == 0xbd)
        {
            pos.x = 50;
            pos.y += height;
        }
        else
        {
            pos.x += width * 10;
        }
    }

    // Row 7
    pos.x = 50;
    pos.y += height * 2;

    if (m_lastStr)
    {
        m_comicFont->DrawString(m_spriteBatch.get(), m_lastStr, pos, yellow);
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

    context->ClearRenderTargetView(renderTarget, c_clearColor);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    const auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    m_tracker.Reset();
}

void Game::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Game::OnResuming()
{
    m_deviceResources->Resume();

    m_tracker.Reset();
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

    auto context = m_deviceResources->GetD3DDeviceContext();

    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"comic.spritefont");
    m_comicFont = std::make_unique<SpriteFont>(device, strFilePath);

    m_room = GeometricPrimitive::CreateBox(context, XMFLOAT3(ROOM_BOUNDS[0], ROOM_BOUNDS[1], ROOM_BOUNDS[2]), false, true);

#ifdef GAMMA_CORRECT_RENDERING
    DDS_LOADER_FLAGS loadFlags = DDS_LOADER_FORCE_SRGB;
#else
    DDS_LOADER_FLAGS loadFlags = DDS_LOADER_DEFAULT;
#endif

    DX::FindMediaFile(strFilePath, MAX_PATH, L"texture.dds");
    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, strFilePath, 0,
        D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
        loadFlags, nullptr, m_roomTex.GetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    const auto size = m_deviceResources->GetOutputSize();
    m_proj = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(70.f), float(size.right) / float(size.bottom), 0.01f, 100.f);

    const auto viewPort = m_deviceResources->GetScreenViewport();
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
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
