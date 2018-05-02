//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK GamePad
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

#define GAMMA_CORRECT_RENDERING
#define USE_FAST_SEMANTICS

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
#include <Windows.UI.Core.h>
#endif

using namespace DirectX;

using Microsoft::WRL::ComPtr;

// Constructor.
Game::Game() :
    m_lastStr(nullptr)
{
    *m_lastStrBuff = 0;

    // 2D only rendering
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(GAMMA_CORRECT_RENDERING)
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
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
#if defined(_XBOX_ONE) && defined(_TITLE)
    UNREFERENCED_PARAMETER(rotation);
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    m_deviceResources->SetWindow(window);
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    m_deviceResources->SetWindow(window, width, height, rotation);
#else
    UNREFERENCED_PARAMETER(rotation);
    m_deviceResources->SetWindow(window, width, height);
#endif

    m_gamePad = std::make_unique<GamePad>();

    // Singleton test
    {
        bool thrown = false;

        try
        {
            std::unique_ptr<GamePad> gamePad2(new GamePad);
        }
        catch (...)
        {
            thrown = true;
        }

        if (!thrown)
        {
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
            MessageBox(window, L"GamePad not acting like a singleton", L"GamePadTest", MB_ICONERROR);
#else
            throw std::exception("GamePad not acting like a singleton");
#endif
        }

        auto state = GamePad::Get().GetState(0);
        state;
    }

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/ )

    m_ctrlChanged.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    m_userChanged.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    if (!m_ctrlChanged.IsValid() || !m_userChanged.IsValid())
    {
        throw std::exception("CreateEvent");
    }

    m_gamePad->RegisterEvents( m_ctrlChanged.Get(), m_userChanged.Get() );

#endif

    m_found.reset(new bool[GamePad::MAX_PLAYER_COUNT] );
    memset(m_found.get(), 0, sizeof(bool) * GamePad::MAX_PLAYER_COUNT);

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
    m_state.connected = false;

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/ )

    HANDLE events[2] = { m_ctrlChanged.Get(), m_userChanged.Get() };
    switch (WaitForMultipleObjects(_countof(events), events, FALSE, 0))
    {
    case WAIT_OBJECT_0:
        OutputDebugStringA("EVENT: Controller changed\n");
        break;
    case WAIT_OBJECT_0 + 1:
        OutputDebugStringA("EVENT: User changed\n");
        break;
    }

#endif

    for (int j = 0; j < GamePad::MAX_PLAYER_COUNT; ++j)
    {
        auto state2 = m_gamePad->GetState(j);
        auto caps = m_gamePad->GetCapabilities(j);

        assert(state2.IsConnected() == caps.IsConnected());

        if (state2.IsConnected())
        {
            if (!m_found[j])
            {
                m_found[j] = true;

                if (caps.IsConnected())
                {
#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
                    if (!caps.id.empty())
                    {
                        using namespace Microsoft::WRL;
                        using namespace Microsoft::WRL::Wrappers;
                        using namespace ABI::Windows::Foundation;
                        using namespace ABI::Windows::System;

                        ComPtr<IUserStatics> statics;
                        DX::ThrowIfFailed(GetActivationFactory(HStringReference(RuntimeClass_Windows_System_User).Get(), statics.GetAddressOf()));

                        ComPtr<IUser> user;
                        HString str;
                        str.Set(caps.id.c_str(), static_cast<unsigned int>(caps.id.length()));
                        HRESULT hr = statics->GetFromId(str.Get(), user.GetAddressOf());
                        if (SUCCEEDED(hr))
                        {
                            UserType userType = UserType_RemoteUser;
                            DX::ThrowIfFailed(user->get_Type(&userType));

                            char buff[1024] = {};
                            sprintf_s(buff, "Player %d -> connected (type %u, id \"%ls\" (user found))\n", j, caps.gamepadType, caps.id.c_str());
                            OutputDebugStringA(buff);
                        }
                        else
                        {
                            char buff[1024] = {};
                            sprintf_s(buff, "Player %d -> connected (type %u, id \"%ls\" (user fail %08X))\n", j, caps.gamepadType, caps.id.c_str(), hr);
                            OutputDebugStringA(buff);
                        }
                    }
                    else
                    {
                        char buff[64] = {};
                        sprintf_s(buff, "Player %d -> connected (type %u, id is empty!)\n", j, caps.gamepadType);
                        OutputDebugStringA(buff);
                    }
#else
                    char buff[64] = {};
                    sprintf_s(buff, "Player %d -> connected (type %u, id %llu)\n", j, caps.gamepadType, caps.id);
                    OutputDebugStringA(buff);
#endif
                }
            }
        }
        else
        {
            if (m_found[j])
            {
                m_found[j] = false;

                char buff[32];
                sprintf_s(buff, "Player %d <- disconnected\n", j);
                OutputDebugStringA(buff);
            }
        }
    }

    m_state = m_gamePad->GetState(-1);

    if (m_state.IsConnected())
    {
        m_tracker.Update(m_state);

        using ButtonState = GamePad::ButtonStateTracker::ButtonState;

        if (m_tracker.a == ButtonState::PRESSED)
            m_lastStr = L"Button A was pressed\n";
        else if (m_tracker.a == ButtonState::RELEASED)
            m_lastStr = L"Button A was released\n";
        else if (m_tracker.b == ButtonState::PRESSED)
            m_lastStr = L"Button B was pressed\n";
        else if (m_tracker.b == ButtonState::RELEASED)
            m_lastStr = L"Button B was released\n";
        else if (m_tracker.x == ButtonState::PRESSED)
            m_lastStr = L"Button X was pressed\n";
        else if (m_tracker.x == ButtonState::RELEASED)
            m_lastStr = L"Button X was released\n";
        else if (m_tracker.y == ButtonState::PRESSED)
            m_lastStr = L"Button Y was pressed\n";
        else if (m_tracker.y == ButtonState::RELEASED)
            m_lastStr = L"Button Y was released\n";
        else if (m_tracker.leftStick == ButtonState::PRESSED)
            m_lastStr = L"Button LeftStick was pressed\n";
        else if (m_tracker.leftStick == ButtonState::RELEASED)
            m_lastStr = L"Button LeftStick was released\n";
        else if (m_tracker.rightStick == ButtonState::PRESSED)
            m_lastStr = L"Button RightStick was pressed\n";
        else if (m_tracker.rightStick == ButtonState::RELEASED)
            m_lastStr = L"Button RightStick was released\n";
        else if (m_tracker.leftShoulder == ButtonState::PRESSED)
            m_lastStr = L"Button LeftShoulder was pressed\n";
        else if (m_tracker.leftShoulder == ButtonState::RELEASED)
            m_lastStr = L"Button LeftShoulder was released\n";
        else if (m_tracker.rightShoulder == ButtonState::PRESSED)
            m_lastStr = L"Button RightShoulder was pressed\n";
        else if (m_tracker.rightShoulder == ButtonState::RELEASED)
            m_lastStr = L"Button RightShoulder was released\n";
        else if (m_tracker.view == ButtonState::PRESSED)
            m_lastStr = L"Button BACK/VIEW was pressed\n";
        else if (m_tracker.view == ButtonState::RELEASED)
            m_lastStr = L"Button BACK/VIEW was released\n";
        else if (m_tracker.menu == ButtonState::PRESSED)
            m_lastStr = L"Button START/MENU was pressed\n";
        else if (m_tracker.menu == ButtonState::RELEASED)
            m_lastStr = L"Button START/MENU was released\n";
        else if (m_tracker.dpadUp == ButtonState::PRESSED)
            m_lastStr = L"Button DPAD UP was pressed\n";
        else if (m_tracker.dpadUp == ButtonState::RELEASED)
            m_lastStr = L"Button DPAD UP was released\n";
        else if (m_tracker.dpadDown == ButtonState::PRESSED)
            m_lastStr = L"Button DPAD DOWN was pressed\n";
        else if (m_tracker.dpadDown == ButtonState::RELEASED)
            m_lastStr = L"Button DPAD DOWN was released\n";
        else if (m_tracker.dpadLeft == ButtonState::PRESSED)
            m_lastStr = L"Button DPAD LEFT was pressed\n";
        else if (m_tracker.dpadLeft == ButtonState::RELEASED)
            m_lastStr = L"Button DPAD LEFT was released\n";
        else if (m_tracker.dpadRight == ButtonState::PRESSED)
            m_lastStr = L"Button DPAD RIGHT was pressed\n";
        else if (m_tracker.dpadRight == ButtonState::RELEASED)
            m_lastStr = L"Button DPAD RIGHT was released\n";
        else if (m_tracker.leftStickUp == ButtonState::PRESSED)
            m_lastStr = L"Button LEFT STICK was pressed UP\n";
        else if (m_tracker.leftStickUp == ButtonState::RELEASED)
            m_lastStr = L"Button LEFT STICK was released from UP\n";
        else if (m_tracker.leftStickDown == ButtonState::PRESSED)
            m_lastStr = L"Button LEFT STICK was pressed DOWN\n";
        else if (m_tracker.leftStickDown == ButtonState::RELEASED)
            m_lastStr = L"Button LEFT STICK was released from DOWN\n";
        else if (m_tracker.leftStickLeft == ButtonState::PRESSED)
            m_lastStr = L"Button LEFT STICK was pressed LEFT\n";
        else if (m_tracker.leftStickLeft == ButtonState::RELEASED)
            m_lastStr = L"Button LEFT STICK was released from LEFT\n";
        else if (m_tracker.leftStickRight == ButtonState::PRESSED)
            m_lastStr = L"Button LEFT STICK was pressed RIGHT\n";
        else if (m_tracker.leftStickRight == ButtonState::RELEASED)
            m_lastStr = L"Button LEFT STICK was released from RIGHT\n";
        else if (m_tracker.rightStickUp == ButtonState::PRESSED)
            m_lastStr = L"Button RIGHT STICK was pressed UP\n";
        else if (m_tracker.rightStickUp == ButtonState::RELEASED)
            m_lastStr = L"Button RIGHT STICK was released from UP\n";
        else if (m_tracker.rightStickDown == ButtonState::PRESSED)
            m_lastStr = L"Button RIGHT STICK was pressed DOWN\n";
        else if (m_tracker.rightStickDown == ButtonState::RELEASED)
            m_lastStr = L"Button RIGHT STICK was released from DOWN\n";
        else if (m_tracker.rightStickLeft == ButtonState::PRESSED)
            m_lastStr = L"Button RIGHT STICK was pressed LEFT\n";
        else if (m_tracker.rightStickLeft == ButtonState::RELEASED)
            m_lastStr = L"Button RIGHT STICK was released from LEFT\n";
        else if (m_tracker.rightStickRight == ButtonState::PRESSED)
            m_lastStr = L"Button RIGHT STICK was pressed RIGHT\n";
        else if (m_tracker.rightStickRight == ButtonState::RELEASED)
            m_lastStr = L"Button RIGHT STICK was released from RIGHT\n";
        else if (m_tracker.leftTrigger == ButtonState::PRESSED)
            m_lastStr = L"Button LEFT TRIGGER was pressed\n";
        else if (m_tracker.leftTrigger == ButtonState::RELEASED)
            m_lastStr = L"Button LEFT TRIGGER was released\n";
        else if (m_tracker.rightTrigger == ButtonState::PRESSED)
            m_lastStr = L"Button RIGHT TRIGGER was pressed\n";
        else if (m_tracker.rightTrigger == ButtonState::RELEASED)
            m_lastStr = L"Button RIGHT TRIGGER was released\n";

        assert(m_tracker.back == m_tracker.view);
        assert(m_tracker.start == m_tracker.menu);

        m_gamePad->SetVibration(-1, m_state.triggers.left, m_state.triggers.right);
    }
    else
    {
        memset(&m_state, 0, sizeof(GamePad::State));

        m_lastStr = nullptr;
        m_tracker.Reset();
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

    XMVECTORF32 yellow;
#ifdef GAMMA_CORRECT_RENDERING
    yellow.v = XMColorSRGBToRGB(Colors::Yellow);
#else
    yellow.v = Colors::Yellow;
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources->Prepare();
#endif

    Clear();

    m_spriteBatch->Begin();

    for (int j = 0; j < std::min(GamePad::MAX_PLAYER_COUNT, 4); ++j)
    {
        XMVECTOR color = m_found[j] ? Colors::White : Colors::Black;
        m_ctrlFont->DrawString(m_spriteBatch.get(), L"$", XMFLOAT2(800.f, float(50 + j * 150)), color);
    }

    if (m_lastStr)
    {
        m_comicFont->DrawString(m_spriteBatch.get(), m_lastStr, XMFLOAT2(25.f, 650.f), yellow);
    }

    // X Y A B
    m_ctrlFont->DrawString(m_spriteBatch.get(), L"&", XMFLOAT2(325, 150), m_state.IsXPressed() ? Colors::White : Colors::Black);
    m_ctrlFont->DrawString(m_spriteBatch.get(), L"(", XMFLOAT2(400, 110), m_state.IsYPressed() ? Colors::White : Colors::Black);
    m_ctrlFont->DrawString(m_spriteBatch.get(), L"'", XMFLOAT2(400, 200), m_state.IsAPressed() ? Colors::White : Colors::Black);
    m_ctrlFont->DrawString(m_spriteBatch.get(), L")", XMFLOAT2(475, 150), m_state.IsBPressed() ? Colors::White : Colors::Black);

    // Left/Right sticks
    auto loc = XMFLOAT2(10, 110);
    loc.x -= m_state.IsLeftThumbStickLeft() ? 20.f : 0.f;
    loc.x += m_state.IsLeftThumbStickRight() ? 20.f : 0.f;
    loc.y -= m_state.IsLeftThumbStickUp() ? 20.f : 0.f;
    loc.y += m_state.IsLeftThumbStickDown() ? 20.f : 0.f;

    m_ctrlFont->DrawString(m_spriteBatch.get(), L" ", loc, m_state.IsLeftStickPressed() ? Colors::White : Colors::Black, 0, XMFLOAT2(0, 0));

    loc = XMFLOAT2(450, 300);
    loc.x -= m_state.IsRightThumbStickLeft() ? 20.f : 0.f;
    loc.x += m_state.IsRightThumbStickRight() ? 20.f : 0.f;
    loc.y -= m_state.IsRightThumbStickUp() ? 20.f : 0.f;
    loc.y += m_state.IsRightThumbStickDown() ? 20.f : 0.f;

    m_ctrlFont->DrawString(m_spriteBatch.get(), L"\"", loc, m_state.IsRightStickPressed() ? Colors::White : Colors::Black, 0, XMFLOAT2(0, 0));

    // DPad
    XMVECTOR color = Colors::Black;
    if (m_state.dpad.up || m_state.dpad.down || m_state.dpad.right || m_state.dpad.left)
        color = Colors::White;

    loc = XMFLOAT2(175, 300);
    loc.x -= m_state.IsDPadLeftPressed() ? 20.f : 0.f;
    loc.x += m_state.IsDPadRightPressed() ? 20.f : 0.f;
    loc.y -= m_state.IsDPadUpPressed() ? 20.f : 0.f;
    loc.y += m_state.IsDPadDownPressed() ? 20.f : 0.f;

    m_ctrlFont->DrawString(m_spriteBatch.get(), L"!", loc, color);

    // Back/Start (aka View/Menu)
    m_ctrlFont->DrawString(m_spriteBatch.get(), L"#", XMFLOAT2(175, 75), m_state.IsViewPressed() ? Colors::White : Colors::Black);
    assert(m_state.IsViewPressed() == m_state.IsBackPressed());
    assert(m_state.buttons.back == m_state.buttons.view);

    m_ctrlFont->DrawString(m_spriteBatch.get(), L"%", XMFLOAT2(300, 75), m_state.IsMenuPressed() ? Colors::White : Colors::Black);
    assert(m_state.IsMenuPressed() == m_state.IsStartPressed());
    assert(m_state.buttons.start == m_state.buttons.menu);

    // Triggers/Shoulders
    m_ctrlFont->DrawString(m_spriteBatch.get(), L"*", XMFLOAT2(500, 10), m_state.IsRightShoulderPressed() ? Colors::White : Colors::Black, 0, XMFLOAT2(0, 0), 0.5f);

    loc = XMFLOAT2(450, 10);
    loc.x += m_state.IsRightTriggerPressed() ? 5.f : 0.f;
    color = XMVectorSet(m_state.triggers.right, m_state.triggers.right, m_state.triggers.right, 1.f);
    m_ctrlFont->DrawString(m_spriteBatch.get(), L"+", loc, color, 0, XMFLOAT2(0, 0), 0.5f);

    loc = XMFLOAT2(130, 10);
    loc.x -= m_state.IsLeftTriggerPressed() ? 5.f : 0.f;
    color = XMVectorSet(m_state.triggers.left, m_state.triggers.left, m_state.triggers.left, 1.f);
    m_ctrlFont->DrawString(m_spriteBatch.get(), L",", loc, color, 0, XMFLOAT2(0, 0), 0.5f);

    m_ctrlFont->DrawString(m_spriteBatch.get(), L"-", XMFLOAT2(10, 10), m_state.IsLeftShoulderPressed() ? Colors::White : Colors::Black, 0, XMFLOAT2(0, 0), 0.5f);

    // Sticks
    RECT src = { 0, 0, 1, 1 };

    RECT rc;
    rc.top = 500;
    rc.left = 10;
    rc.bottom = 525;
    rc.right = rc.left + int(((m_state.thumbSticks.leftX + 1.f) / 2.f) * 275);

    m_spriteBatch->Draw(m_defaultTex.Get(), rc, &src);

    rc.top = 550;
    rc.bottom = 575;

    rc.right = rc.left + int(((m_state.thumbSticks.leftY + 1.f) / 2.f) * 275);

    m_spriteBatch->Draw(m_defaultTex.Get(), rc, &src);

    rc.top = 500;
    rc.left = 325;
    rc.bottom = 525;
    rc.right = rc.left + int(((m_state.thumbSticks.rightX + 1.f) / 2.f) * 275);

    m_spriteBatch->Draw(m_defaultTex.Get(), rc, &src);

    rc.top = 550;
    rc.bottom = 575;

    rc.right = rc.left + int(((m_state.thumbSticks.rightY + 1.f) / 2.f) * 275);

    m_spriteBatch->Draw(m_defaultTex.Get(), rc, &src);

    m_spriteBatch->End();

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
    auto renderTarget = m_deviceResources->GetRenderTargetView();

    XMVECTORF32 color;
#ifdef GAMMA_CORRECT_RENDERING
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
#else
    color.v = Colors::CornflowerBlue;
#endif
    context->ClearRenderTargetView(renderTarget, color);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

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
    m_tracker.Reset();
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
#pragma endregion

#pragma region Direct3D Resources
// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 1024;
    height = 768;
}

// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device, m_deviceResources->GetBackBufferCount());
#endif

    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    m_comicFont = std::make_unique<SpriteFont>(device, L"comic.spritefont");

    m_ctrlFont = std::make_unique<SpriteFont>(device, L"xboxController.spritefont");

    {
        static const uint32_t s_pixel = 0xffffffff;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        ComPtr<ID3D11Texture2D> tex;
        DX::ThrowIfFailed(device->CreateTexture2D(&desc, &initData, &tex));

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;

        DX::ThrowIfFailed(device->CreateShaderResourceView(tex.Get(), &SRVDesc, m_defaultTex.ReleaseAndGetAddressOf()));
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto viewPort = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewPort);

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP) 
    m_spriteBatch->SetRotation(m_deviceResources->GetRotation());
#endif
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnDeviceLost()
{
    m_spriteBatch.reset();
    m_comicFont.reset();
    m_ctrlFont.reset();

    m_defaultTex.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion