//--------------------------------------------------------------------------------------
// File: GamePadTest.cpp
//
// Developer unit test for DirectXTK Mouse
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
#include "GamePadTest.h"

using namespace DirectX;

using Microsoft::WRL::ComPtr;

// Constructor.
Game::Game() :
    m_lastStr(nullptr)
{
    *m_lastStrBuff = 0;

    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    IUnknown* window,
#else
    HWND window,
#endif
    int width, int height, DXGI_MODE_ROTATION rotation)
{
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
            throw std::exception("GamePad not acting like a singleton");
#else
            MessageBox(window, L"GamePad not acting like a singleton", L"GamePadTest", MB_ICONERROR);
#endif
        }

        auto state = GamePad::Get().GetState(0);
        state;
    }

    m_found.reset(new bool[GamePad::MAX_PLAYER_COUNT] );
    memset(m_found.get(), 0, sizeof(bool) * GamePad::MAX_PLAYER_COUNT);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

// Executes basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world
void Game::Update(DX::StepTimer const&)
{
    // TODO -
    int player = -1;

    m_state.connected = false;

    for (int j = 0; j < GamePad::MAX_PLAYER_COUNT; ++j)
    {
        XMVECTOR color = Colors::Black;
        auto state2 = m_gamePad->GetState(j);
        if (state2.IsConnected())
        {
            if (!m_found[j])
            {
                m_found[j] = true;

                auto caps = m_gamePad->GetCapabilities(j);
                if (caps.IsConnected())
                {
                    char buff[64];
                    sprintf_s(buff, "Player %d -> type %u, id %I64u\n", j, caps.gamepadType, caps.id);
                    OutputDebugStringA(buff);
                }
            }

            if (player == -1)
            {
                player = j;
                m_state = state2;
            }
        }
        else if (m_found[j])
        {
            m_found[j] = false;

            char buff[32];
            sprintf_s(buff, "Player %d <- disconnected\n", j);
            OutputDebugStringA(buff);
        }
    }

    if (m_state.IsConnected())
    {
        m_tracker.Update(m_state);

        if (m_tracker.a == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button A was pressed\n";
        else if (m_tracker.a == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button A was released\n";
        else if (m_tracker.b == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button B was pressed\n";
        else if (m_tracker.b == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button B was released\n";
        else if (m_tracker.x == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button X was pressed\n";
        else if (m_tracker.x == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button X was released\n";
        else if (m_tracker.y == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button Y was pressed\n";
        else if (m_tracker.y == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button Y was released\n";
        else if (m_tracker.leftStick == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button LeftStick was pressed\n";
        else if (m_tracker.leftStick == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button LeftStick was released\n";
        else if (m_tracker.rightStick == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button RightStick was pressed\n";
        else if (m_tracker.rightStick == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button RightStick was released\n";
        else if (m_tracker.leftShoulder == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button LeftShoulder was pressed\n";
        else if (m_tracker.leftShoulder == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button LeftShoulder was released\n";
        else if (m_tracker.rightShoulder == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button RightShoulder was pressed\n";
        else if (m_tracker.rightShoulder == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button RightShoulder was released\n";
        else if (m_tracker.view == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button BACK/VIEW was pressed\n";
        else if (m_tracker.view == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button BACK/VIEW was released\n";
        else if (m_tracker.menu == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button START/MENU was pressed\n";
        else if (m_tracker.menu == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button START/MENU was released\n";
        else if (m_tracker.dpadUp == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button DPAD UP was pressed\n";
        else if (m_tracker.dpadUp == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button DPAD UP was released\n";
        else if (m_tracker.dpadDown == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button DPAD DOWN was pressed\n";
        else if (m_tracker.dpadDown == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button DPAD DOWN was released\n";
        else if (m_tracker.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button DPAD LEFT was pressed\n";
        else if (m_tracker.dpadLeft == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button DPAD LEFT was released\n";
        else if (m_tracker.dpadRight == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button DPAD RIGHT was pressed\n";
        else if (m_tracker.dpadRight == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button DPAD RIGHT was released\n";
        else if (m_tracker.leftStickUp == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button LEFT STICK was pressed UP\n";
        else if (m_tracker.leftStickUp == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button LEFT STICK was released from UP\n";
        else if (m_tracker.leftStickDown == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button LEFT STICK was pressed DOWN\n";
        else if (m_tracker.leftStickDown == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button LEFT STICK was released from DOWN\n";
        else if (m_tracker.leftStickLeft == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button LEFT STICK was pressed LEFT\n";
        else if (m_tracker.leftStickLeft == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button LEFT STICK was released from LEFT\n";
        else if (m_tracker.leftStickRight == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button LEFT STICK was pressed RIGHT\n";
        else if (m_tracker.leftStickRight == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button LEFT STICK was released from RIGHT\n";
        else if (m_tracker.rightStickUp == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button RIGHT STICK was pressed UP\n";
        else if (m_tracker.rightStickUp == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button RIGHT STICK was released from UP\n";
        else if (m_tracker.rightStickDown == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button RIGHT STICK was pressed DOWN\n";
        else if (m_tracker.rightStickDown == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button RIGHT STICK was released from DOWN\n";
        else if (m_tracker.rightStickLeft == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button RIGHT STICK was pressed LEFT\n";
        else if (m_tracker.rightStickLeft == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button RIGHT STICK was released from LEFT\n";
        else if (m_tracker.rightStickRight == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button RIGHT STICK was pressed RIGHT\n";
        else if (m_tracker.rightStickRight == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button RIGHT STICK was released from RIGHT\n";
        else if (m_tracker.leftTrigger == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button LEFT TRIGGER was pressed\n";
        else if (m_tracker.leftTrigger == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button LEFT TRIGGER was released\n";
        else if (m_tracker.rightTrigger == GamePad::ButtonStateTracker::PRESSED)
            m_lastStr = L"Button RIGHT TRIGGER was pressed\n";
        else if (m_tracker.rightTrigger == GamePad::ButtonStateTracker::RELEASED)
            m_lastStr = L"Button RIGHT TRIGGER was released\n";

        assert(m_tracker.back == m_tracker.view);
        assert(m_tracker.start == m_tracker.menu);

        m_gamePad->SetVibration(player, m_state.triggers.left, m_state.triggers.right);
    }
    else
    {
        m_lastStr = nullptr;
        m_tracker.Reset();
    }
}

// Draws the scene
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
        return;

    Clear();

    m_spriteBatch->Begin();

    int player = -1;

    for (int j = 0; j < std::min(GamePad::MAX_PLAYER_COUNT, 4); ++j)
    {
        XMVECTOR color = m_found[j] ? Colors::White : Colors::Black;
        m_ctrlFont->DrawString(m_spriteBatch.get(), L"$", XMFLOAT2(800.f, float(50 + j * 150)), color);
    }

    if (m_lastStr)
    {
        m_comicFont->DrawString(m_spriteBatch.get(), m_lastStr, XMFLOAT2(25.f, 650.f), Colors::Yellow);
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
}

// Helper method to clear the backbuffers
void Game::Clear()
{
    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetBackBufferRenderTargetView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}

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

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
void Game::ValidateDevice()
{
    m_deviceResources->ValidateDevice();
}
#endif

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