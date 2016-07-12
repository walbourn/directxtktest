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
    m_window(0),
    m_outputWidth(800),
    m_outputHeight(600),
    m_outputRotation(DXGI_MODE_ROTATION_IDENTITY),
    m_featureLevel(D3D_FEATURE_LEVEL_9_1),
    m_lastStr(nullptr)
{
    *m_lastStrBuff = 0;
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
    m_window = window;
    m_outputWidth = std::max( width, 1 );
    m_outputHeight = std::max( height, 1 );
    m_outputRotation = rotation;

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
            MessageBox(m_window, L"GamePad not acting like a singleton", L"GamePadTest", MB_ICONERROR);
#endif
        }

        auto state = GamePad::Get().GetState(0);
        state;
    }

    m_found.reset(new bool[GamePad::MAX_PLAYER_COUNT] );
    memset(m_found.get(), 0, sizeof(bool) * GamePad::MAX_PLAYER_COUNT);

    CreateDevice();

    CreateResources();
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

    Present();
}

// Helper method to clear the backbuffers
void Game::Clear()
{
    // Clear the views
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::CornflowerBlue);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), nullptr);

    CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewPort);
}

// Presents the backbuffer contents to the screen
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
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
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);
    m_outputRotation = rotation;

    CreateResources();
}

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
void Game::ValidateDevice()
{
    // The D3D Device is no longer valid if the default adapter changed since the device
    // was created or if the device has been removed.

    DXGI_ADAPTER_DESC previousDesc;
    {
        ComPtr<IDXGIDevice3> dxgiDevice;
        HRESULT hr = m_d3dDevice.As(&dxgiDevice);
        DX::ThrowIfFailed(hr);

        ComPtr<IDXGIAdapter> deviceAdapter;
        hr = dxgiDevice->GetAdapter(&deviceAdapter);
        DX::ThrowIfFailed(hr);

        ComPtr<IDXGIFactory2> dxgiFactory;
        hr = deviceAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
        DX::ThrowIfFailed(hr);

        ComPtr<IDXGIAdapter1> previousDefaultAdapter;
        hr = dxgiFactory->EnumAdapters1(0, previousDefaultAdapter.GetAddressOf());
        DX::ThrowIfFailed(hr);

        hr = previousDefaultAdapter->GetDesc(&previousDesc);
        DX::ThrowIfFailed(hr);
    }

    DXGI_ADAPTER_DESC currentDesc;
    {
        ComPtr<IDXGIFactory2> currentFactory;
        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&currentFactory));
        DX::ThrowIfFailed(hr);

        ComPtr<IDXGIAdapter1> currentDefaultAdapter;
        hr = currentFactory->EnumAdapters1(0, &currentDefaultAdapter);
        DX::ThrowIfFailed(hr);

        hr = currentDefaultAdapter->GetDesc(&currentDesc);
        DX::ThrowIfFailed(hr);
    }

    // If the adapter LUIDs don't match, or if the device reports that it has been removed,
    // a new D3D device must be created.

    HRESULT hr = m_d3dDevice->GetDeviceRemovedReason();
    if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart
        || previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart
        || FAILED(hr))
    {
        // Create a new device and swap chain.
        OnDeviceLost();
    }
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 1024;
    height = 768;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    // This flag adds support for surfaces with a different color channel ordering than the API default.
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels [] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dContext;
    HRESULT hr = D3D11CreateDevice(
        nullptr,                                // specify null to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,                                // leave as nullptr unless software device
        creationFlags,                          // optionally set debug and Direct2D compatibility flags
        featureLevels,                          // list of feature levels this app can support
        _countof(featureLevels),                // number of entries in above list
        D3D11_SDK_VERSION,                      // always set this to D3D11_SDK_VERSION
        d3dDevice.GetAddressOf(),               // returns the Direct3D device created
        &m_featureLevel,                        // returns feature level of device created
        d3dContext.GetAddressOf()               // returns the device immediate context
        );

    DX::ThrowIfFailed(hr);

    hr = d3dDevice.As(&m_d3dDevice);
    DX::ThrowIfFailed(hr);

    hr = d3dContext.As(&m_d3dContext);
    DX::ThrowIfFailed(hr);

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    hr = m_d3dDevice.As(&d3dDebug);
    if (SUCCEEDED(hr))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        hr = d3dDebug.As(&d3dInfoQueue);
        if (SUCCEEDED(hr))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide [] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    m_spriteBatch = std::make_unique<SpriteBatch>(m_d3dContext.Get());

    m_comicFont = std::make_unique<SpriteFont>(m_d3dDevice.Get(), L"comic.spritefont");

    m_ctrlFont = std::make_unique<SpriteFont>(m_d3dDevice.Get(), L"xboxController.spritefont");

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
        DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&desc, &initData, &tex));

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
        SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;

        DX::ThrowIfFailed(m_d3dDevice->CreateShaderResourceView(tex.Get(), &SRVDesc, m_defaultTex.ReleaseAndGetAddressOf()));
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews [] = { nullptr };
    m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
    m_renderTargetView.Reset();
    m_d3dContext->Flush();

    UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(2, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent( IID_PPV_ARGS( &dxgiFactory ) ) );

        ComPtr<IDXGIFactory2> dxgiFactory2;
        HRESULT hr = dxgiFactory.As(&dxgiFactory2);
        DX::ThrowIfFailed(hr);

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;

        // Create a SwapChain from a window.
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP) 
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        hr = dxgiFactory->CreateSwapChainForCoreWindow(m_d3dDevice.Get(),
            m_window, &swapChainDesc,
            nullptr, m_swapChain.GetAddressOf());
        DX::ThrowIfFailed(hr);
#else
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        hr = dxgiFactory2->CreateSwapChainForHwnd(
            m_d3dDevice.Get(), m_window, &swapChainDesc,
            &fsSwapChainDesc,
            nullptr, m_swapChain.ReleaseAndGetAddressOf());
        DX::ThrowIfFailed(hr);

        // This template does not support 'full-screen' mode and prevents the ALT+ENTER shortcut from working
        dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER);
#endif
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backBuffer));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));
    SetDebugObjectName(backBuffer.Get(), "BackBuffer");

    CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_spriteBatch->SetViewport(viewPort);

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP) 
    m_spriteBatch->SetRotation(m_outputRotation);
#endif
}

void Game::OnDeviceLost()
{
    m_spriteBatch.reset();
    m_comicFont.reset();
    m_ctrlFont.reset();

    m_defaultTex.Reset();

    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    CreateDevice();

    CreateResources();
}