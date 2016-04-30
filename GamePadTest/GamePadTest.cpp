//--------------------------------------------------------------------------------------
// File: GamePadTest.cpp
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

#include "GamePad.h"
#include "CommonStates.h"
#include "VertexTypes.h"
#include "DirectXColors.h"
#include "DDSTextureLoader.h"
#include "ScreenGrab.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"

#include "PlatformHelpers.h"

#include <stdio.h>

#include <wrl/client.h>

#include <wincodec.h>

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/ )
#include <wrl.h>
#pragma comment(lib,"RuntimeObject.lib")
#endif

using namespace DirectX;
using Microsoft::WRL::ComPtr;

std::unique_ptr<SpriteBatch> g_spriteBatch;
std::unique_ptr<GamePad> g_gamePad;

// Build for LH vs. RH coords
//#define LH_COORDS

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_ACTIVATEAPP:
            if (g_gamePad)
            {
                if (wParam)
                {
                    g_gamePad->Resume();
                }
                else
                {
                    g_gamePad->Suspend();
                }
            }
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow )
{
    HRESULT hr;

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/ )
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize))
        return 1;
#endif

    wchar_t *const className = L"TestWindowClass";

    WNDCLASSEX wndClass = {};

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = className;
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wndClass);

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    RECT client;
    GetClientRect(hwnd, &client);

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = client.right;
    swapChainDesc.BufferDesc.Height = client.bottom;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_10_0;
    
    DWORD d3dFlags = 0;
#ifdef _DEBUG
    d3dFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapChain;
    if (FAILED(hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, d3dFlags, &featureLevel, 1,
                                                  D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, nullptr, &context)))
        return 1;

    ComPtr<ID3D11Texture2D> backBufferTexture;
    if (FAILED(hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture)))
        return 1;

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = { DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2D };

    ComPtr<ID3D11RenderTargetView> backBuffer;
    if (FAILED(hr = device->CreateRenderTargetView(backBufferTexture.Get(), &renderTargetViewDesc, &backBuffer)))
        return 1;

    CommonStates states(device.Get());

    g_spriteBatch = std::make_unique<SpriteBatch>(context.Get());

    SpriteFont ctrlFont(device.Get(), L"xboxController.spritefont");

    SpriteFont comic(device.Get(), L"comic.spritefont");

    g_gamePad = std::make_unique<GamePad>();

    // Singleton test
    {
        bool thrown = false;

        try
        {
            std::unique_ptr<GamePad> gamePad2( new GamePad );
        }
        catch( ... )
        {
            thrown = true;
        }

        if ( !thrown )
        {
            MessageBox(hwnd, L"GamePad not acting like a singleton", L"GamePadTest", MB_ICONERROR);
        }

        auto state = GamePad::Get().GetState(0);
        state;
    }

    ComPtr<ID3D11ShaderResourceView> defaultTex;
    {
        static const uint32_t s_pixel = 0xffffffff;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        D3D11_TEXTURE2D_DESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        ComPtr<ID3D11Texture2D> tex;
        hr = device->CreateTexture2D(&desc, &initData, &tex);

        if (FAILED(hr))
        {
            return 1;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
        memset(&SRVDesc, 0, sizeof(SRVDesc));
        SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;

        hr = device->CreateShaderResourceView(tex.Get(), &SRVDesc, &defaultTex);
        if (FAILED(hr))
        {
            return 1;
        }
    }

    bool quit = false;

    D3D11_VIEWPORT vp = { 0, 0, (float)client.right, (float)client.bottom, 0, 1 };

    context->RSSetViewports(1, &vp);

    context->OMSetRenderTargets(1, backBuffer.GetAddressOf(), nullptr);

#ifdef LH_COORDS
    context->RSSetState( states.CullCounterClockwise() );
#else
    context->RSSetState( states.CullClockwise() );
#endif

    size_t frame = 0;

    std::unique_ptr<bool[]> found( new bool[ GamePad::MAX_PLAYER_COUNT ] );
    memset( found.get(), 0, sizeof(bool) * GamePad::MAX_PLAYER_COUNT );

    GamePad::ButtonStateTracker tracker;

    const wchar_t * lastStr = nullptr;

    while (!quit)
    {
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                quit = true;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        context->ClearRenderTargetView(backBuffer.Get(), Colors::CornflowerBlue);

        g_spriteBatch->Begin();

        GamePad::State state;
        state.connected = false;

        int player = -1;

        for (int j = 0; j < GamePad::MAX_PLAYER_COUNT; ++j)
        {
            XMVECTOR color = Colors::Black;
            auto state2 = g_gamePad->GetState(j);
            if ( state2.IsConnected() )
            {
                if ( !found[j] )
                {
                    found[j] = true;

                    auto caps = g_gamePad->GetCapabilities( j );
                    if ( caps.IsConnected() )
                    {
                        char buff[ 64 ];
                        sprintf_s(buff, "Player %d -> type %u, id %I64u\n", j, caps.gamepadType, caps.id);
                        OutputDebugStringA( buff );                        
                    }
                }

                color = Colors::White;

                if (player == -1)
                {
                    player = j;
                    state = state2;
                }
            }
            else if ( found[j] )
            {
                found[ j ] = false;

                char buff[ 32 ];
                sprintf_s( buff, "Player %d <- disconnected\n", j );
                OutputDebugStringA( buff );
            }

            ctrlFont.DrawString(g_spriteBatch.get(), L"$", XMFLOAT2(800.f, float( 50 + j * 150) ), color);
        }

        if (state.IsConnected())
        {
            tracker.Update( state );

            if ( tracker.a == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button A was pressed\n";
            else if ( tracker.a == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button A was released\n";
            else if ( tracker.b == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button B was pressed\n";
            else if ( tracker.b == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button B was released\n";
            else if ( tracker.x == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button X was pressed\n";
            else if ( tracker.x == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button X was released\n";
            else if ( tracker.y == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button Y was pressed\n";
            else if ( tracker.y == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button Y was released\n";
            else if ( tracker.leftStick == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button LeftStick was pressed\n";
            else if ( tracker.leftStick == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button LeftStick was released\n";
            else if ( tracker.rightStick == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button RightStick was pressed\n";
            else if ( tracker.rightStick == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button RightStick was released\n";
            else if ( tracker.leftShoulder == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button LeftShoulder was pressed\n";
            else if ( tracker.leftShoulder == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button LeftShoulder was released\n";
            else if ( tracker.rightShoulder == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button RightShoulder was pressed\n";
            else if ( tracker.rightShoulder == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button RightShoulder was released\n";
            else if ( tracker.view == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button BACK/VIEW was pressed\n";
            else if ( tracker.view == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button BACK/VIEW was released\n";
            else if ( tracker.menu == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button START/MENU was pressed\n";
            else if ( tracker.menu == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button START/MENU was released\n";
            else if ( tracker.dpadUp == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button DPAD UP was pressed\n";
            else if ( tracker.dpadUp == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button DPAD UP was released\n";
            else if ( tracker.dpadDown == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button DPAD DOWN was pressed\n";
            else if ( tracker.dpadDown == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button DPAD DOWN was released\n";
            else if ( tracker.dpadLeft == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button DPAD LEFT was pressed\n";
            else if ( tracker.dpadLeft == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button DPAD LEFT was released\n";
            else if ( tracker.dpadRight == GamePad::ButtonStateTracker::PRESSED )
                lastStr = L"Button DPAD RIGHT was pressed\n";
            else if ( tracker.dpadRight == GamePad::ButtonStateTracker::RELEASED )
                lastStr = L"Button DPAD RIGHT was released\n";

            assert(tracker.back == tracker.view);
            assert(tracker.start == tracker.menu);

            if ( lastStr )
            {
                comic.DrawString( g_spriteBatch.get(), lastStr, XMFLOAT2( 25.f, 650.f ), Colors::Yellow );
            }

            // X Y A B
            ctrlFont.DrawString(g_spriteBatch.get(), L"&", XMFLOAT2(325, 150), state.IsXPressed() ? Colors::White : Colors::Black);
            ctrlFont.DrawString(g_spriteBatch.get(), L"(", XMFLOAT2(400, 110), state.IsYPressed() ? Colors::White : Colors::Black);
            ctrlFont.DrawString(g_spriteBatch.get(), L"'", XMFLOAT2(400, 200), state.IsAPressed() ? Colors::White : Colors::Black);
            ctrlFont.DrawString(g_spriteBatch.get(), L")", XMFLOAT2(475, 150), state.IsBPressed() ? Colors::White : Colors::Black);

            // Left/Right sticks
            auto loc = XMFLOAT2(10, 110);
            loc.x -= state.IsLeftThumbStickLeft() ? 20.f : 0.f;
            loc.x += state.IsLeftThumbStickRight() ? 20.f : 0.f;
            loc.y -= state.IsLeftThumbStickUp() ? 20.f : 0.f;
            loc.y += state.IsLeftThumbStickDown() ? 20.f : 0.f;

            ctrlFont.DrawString(g_spriteBatch.get(), L" ", loc, state.IsLeftStickPressed() ? Colors::White : Colors::Black, 0, XMFLOAT2(0, 0));

            loc = XMFLOAT2(450, 300);
            loc.x -= state.IsRightThumbStickLeft() ? 20.f : 0.f;
            loc.x += state.IsRightThumbStickRight() ? 20.f : 0.f;
            loc.y -= state.IsRightThumbStickUp() ? 20.f : 0.f;
            loc.y += state.IsRightThumbStickDown() ? 20.f : 0.f;

            ctrlFont.DrawString(g_spriteBatch.get(), L"\"", loc, state.IsRightStickPressed() ? Colors::White : Colors::Black, 0, XMFLOAT2(0, 0));

            // DPad
            XMVECTOR color = Colors::Black;
            if (state.dpad.up || state.dpad.down || state.dpad.right || state.dpad.left)
                color = Colors::White;

            loc = XMFLOAT2(175, 300);
            loc.x -= state.IsDPadLeftPressed() ? 20.f : 0.f;
            loc.x += state.IsDPadRightPressed() ? 20.f : 0.f;
            loc.y -= state.IsDPadUpPressed() ? 20.f : 0.f;
            loc.y += state.IsDPadDownPressed() ? 20.f : 0.f;

            ctrlFont.DrawString(g_spriteBatch.get(), L"!", loc, color );

            // Back/Start (aka View/Menu)
            ctrlFont.DrawString(g_spriteBatch.get(), L"#", XMFLOAT2(175, 75), state.IsViewPressed() ? Colors::White : Colors::Black);

            ctrlFont.DrawString(g_spriteBatch.get(), L"%", XMFLOAT2(300, 75), state.IsMenuPressed() ? Colors::White : Colors::Black);

            // Triggers/Shoulders
            ctrlFont.DrawString(g_spriteBatch.get(), L"*", XMFLOAT2(500, 10), state.IsRightShoulderPressed() ? Colors::White : Colors::Black, 0, XMFLOAT2(0, 0), 0.5f);

            loc = XMFLOAT2(450, 10);
            loc.x += state.IsRightTriggerPressed() ? 5.f : 0.f;
            color = XMVectorSet(state.triggers.right, state.triggers.right, state.triggers.right, 1.f);
            ctrlFont.DrawString(g_spriteBatch.get(), L"+", loc, color, 0, XMFLOAT2(0, 0), 0.5f);

            loc = XMFLOAT2(130, 10);
            loc.x -= state.IsLeftTriggerPressed() ? 5.f : 0.f;
            color = XMVectorSet(state.triggers.left, state.triggers.left, state.triggers.left, 1.f);
            ctrlFont.DrawString(g_spriteBatch.get(), L",", loc, color, 0, XMFLOAT2(0, 0), 0.5f);

            ctrlFont.DrawString(g_spriteBatch.get(), L"-", XMFLOAT2(10, 10), state.IsLeftShoulderPressed() ? Colors::White : Colors::Black, 0, XMFLOAT2(0, 0), 0.5f);

            // Sticks
            RECT src = { 0, 0, 1, 1 };

            RECT rc;
            rc.top = 500;
            rc.left = 10;
            rc.bottom = 525;
            rc.right = rc.left + int( ( (state.thumbSticks.leftX + 1.f) / 2.f) * 275);

            g_spriteBatch->Draw(defaultTex.Get(), rc, &src);

            rc.top = 550;
            rc.bottom = 575;

            rc.right = rc.left + int( ((state.thumbSticks.leftY + 1.f) / 2.f) * 275);

            g_spriteBatch->Draw(defaultTex.Get(), rc, &src);

            rc.top = 500;
            rc.left = 325;
            rc.bottom = 525;
            rc.right = rc.left + int(((state.thumbSticks.rightX + 1.f) / 2.f) * 275);

            g_spriteBatch->Draw(defaultTex.Get(), rc, &src);

            rc.top = 550;
            rc.bottom = 575;

            rc.right = rc.left + int(((state.thumbSticks.rightY + 1.f) / 2.f) * 275);

            g_spriteBatch->Draw(defaultTex.Get(), rc, &src);

            g_gamePad->SetVibration(player, state.triggers.left, state.triggers.right);
        }
        else
        {
            lastStr = nullptr;
            tracker.Reset();
        }

        g_spriteBatch->End();

        swapChain->Present(1, 0);
        ++frame;

        if ( frame == 10 )
        {
            ComPtr<ID3D11Texture2D> backBufferTex;
            hr = swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&backBufferTex);
            if ( SUCCEEDED(hr) )
            {
                hr = SaveWICTextureToFile(context.Get(), backBufferTex.Get(), GUID_ContainerFormatBmp, L"SCREENSHOT.BMP");
                hr = SaveDDSTextureToFile(context.Get(), backBufferTex.Get(), L"SCREENSHOT.DDS" );
            }
        }
    }

    g_spriteBatch.reset();

    return 0;
}
