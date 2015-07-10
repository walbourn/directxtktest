//--------------------------------------------------------------------------------------
// File: KeyboardTest.cpp
//
// Developer unit test for DirectXTK Keyboard (Win32)
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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "Keyboard.h"
#include "CommonStates.h"
#include "VertexTypes.h"
#include "DirectXColors.h"
#include "DDSTextureLoader.h"
#include "ScreenGrab.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"

#include <stdio.h>

#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#pragma warning(pop)

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

using namespace DirectX;

std::unique_ptr<SpriteBatch> g_spriteBatch;
std::unique_ptr<Keyboard> g_keyboard;

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
            Keyboard::ProcessMessage(msg, wParam, lParam);
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            Keyboard::ProcessMessage(msg, wParam, lParam);
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}


int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow )
{
    HRESULT hr;

    wchar_t *const className = L"TestWindowClass";

    WNDCLASSEX wndClass = { 0 };

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = className;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wndClass);

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    RECT client;
    GetClientRect(hwnd, &client);

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11Texture2D> backBufferTexture;
    ComPtr<ID3D11RenderTargetView> backBuffer;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };

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

    if (FAILED(hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, d3dFlags, &featureLevel, 1, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &device, NULL, &context)))
        return 1;

    if (FAILED(hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTexture)))
        return 1;

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = { DXGI_FORMAT_UNKNOWN, D3D11_RTV_DIMENSION_TEXTURE2D };

    if (FAILED(hr = device->CreateRenderTargetView(backBufferTexture.Get(), &renderTargetViewDesc, &backBuffer)))
        return 1;

    CommonStates states(device.Get());

    g_spriteBatch.reset(new SpriteBatch(context.Get()));

    SpriteFont comic(device.Get(), L"comic.spritefont");

    g_keyboard.reset(new Keyboard);

    // Singleton test
    {
        bool thrown = false;

        try
        {
            std::unique_ptr<Keyboard> kb2(new Keyboard);
        }
        catch (...)
        {
            thrown = true;
        }

        if (!thrown)
        {
            MessageBox(hwnd, L"Keyboard not acting like a singleton", 0, 0);
        }

        auto state = Keyboard::Get().GetState();
        state;
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

    Keyboard::KeyboardStateTracker tracker;

    const wchar_t * lastStr = nullptr;
    wchar_t lastStrBuff[128] = { 0 };
    while (!quit)
    {
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                quit = true;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto kb = g_keyboard->GetState();

        if (kb.Escape)
            break;

        if (kb.Home)
            g_keyboard->Reset();

        tracker.Update(kb);

        if (tracker.pressed.Q)
            lastStr = L"Q was pressed";
        else if ( tracker.released.Q )
            lastStr = L"Q was released";
        else if (tracker.pressed.W)
            lastStr = L"W was pressed";
        else if ( tracker.released.W )
            lastStr = L"W was released";
        else if (tracker.pressed.E)
            lastStr = L"E was pressed";
        else if (tracker.released.E)
            lastStr = L"E was released";
        else if (tracker.pressed.R)
            lastStr = L"R was pressed";
        else if ( tracker.released.R )
            lastStr = L"R was released";
        else if (tracker.pressed.T)
            lastStr = L"T was pressed";
        else if ( tracker.released.T )
            lastStr = L"T was released";
        else if (tracker.pressed.Y)
            lastStr = L"Y was pressed";
        else if (tracker.released.Y)
            lastStr = L"Y was released";
        else if (tracker.pressed.LeftShift)
            lastStr = L"LeftShift was pressed";
        else if (tracker.released.LeftShift)
            lastStr = L"LeftShift was released";
        else if (tracker.pressed.LeftAlt)
            lastStr = L"LeftAlt was pressed";
        else if (tracker.released.LeftAlt)
            lastStr = L"LeftAlt was released";
        else if (tracker.pressed.LeftControl)
            lastStr = L"LeftCtrl was pressed";
        else if (tracker.released.LeftControl)
            lastStr = L"LeftCtrl was released";
        else if (tracker.pressed.RightShift)
            lastStr = L"RightShift was pressed";
        else if (tracker.released.RightShift)
            lastStr = L"RightShift was released";
        else if (tracker.pressed.RightAlt)
            lastStr = L"RightAlt was pressed";
        else if (tracker.released.RightAlt)
            lastStr = L"RightAlt was released";
        else if (tracker.pressed.RightControl)
            lastStr = L"RightCtrl was pressed";
        else if (tracker.released.RightControl)
            lastStr = L"RightCtrl was released";
        else if (tracker.pressed.Space)
            lastStr = L"Space was pressed";
        else if (tracker.released.Space)
            lastStr = L"Space was released";

        for (int vk = VK_F1; vk <= VK_F10; ++vk)
        {
            if (tracker.IsKeyPressed(static_cast<DirectX::Keyboard::Keys>(vk)))
            {
                wchar_t buff[5];
                swprintf_s(buff, L"F%d", vk - VK_F1 + 1);
                swprintf_s(lastStrBuff, L"%s was pressed", buff);
                lastStr = lastStrBuff;
            }
            else if (tracker.IsKeyReleased(static_cast<DirectX::Keyboard::Keys>(vk)))
            {
                wchar_t buff[5];
                swprintf_s(buff, L"F%d", vk - VK_F1 + 1);
                swprintf_s(lastStrBuff, L"%s was released", buff);
                lastStr = lastStrBuff;
            }
        }

        for (int vk = 0x30; vk <= 0x39; ++vk)
        {
            if (tracker.IsKeyPressed(static_cast<DirectX::Keyboard::Keys>(vk)))
            {
                wchar_t buff[3];
                swprintf_s(buff, L"%d", vk - 0x30);
                swprintf_s(lastStrBuff, L"%s was pressed", buff);
                lastStr = lastStrBuff;
            }
            else if (tracker.IsKeyReleased(static_cast<DirectX::Keyboard::Keys>(vk)))
            {
                wchar_t buff[3];
                swprintf_s(buff, L"%d", vk - 0x30);
                swprintf_s(lastStrBuff, L"%s was released", buff);
                lastStr = lastStrBuff;
            }
        }

        context->ClearRenderTargetView(backBuffer.Get(), Colors::CornflowerBlue);

        XMVECTOR xsize = comic.MeasureString( L"X" );

        float width = XMVectorGetX(xsize);
        float height = XMVectorGetY(xsize);

        g_spriteBatch->Begin();

        XMFLOAT2 pos(50, 50);

        // Row 0
        for (int vk = VK_F1; vk <= VK_F10; ++vk)
        {
            wchar_t buff[5];
            swprintf_s(buff, L"F%d", vk - VK_F1 + 1);
            comic.DrawString(g_spriteBatch.get(), buff, pos, kb.IsKeyDown( static_cast<DirectX::Keyboard::Keys>( vk ) ) ? Colors::Red : Colors::White);

            pos.x += width * 3;
        }

        // Row 1
        pos.x = 50;
        pos.y += height * 2;

        for (int vk = 0x30; vk <= 0x39; ++vk)
        {
            wchar_t buff[3];
            swprintf_s(buff, L"%d", vk - 0x30);
            comic.DrawString(g_spriteBatch.get(), buff, pos, kb.IsKeyDown( static_cast<DirectX::Keyboard::Keys>( vk ) ) ? Colors::Red : Colors::White);

            pos.x += width * 2;
        }

        // Row 2
        pos.x = 50;
        pos.y += height * 2;

        comic.DrawString(g_spriteBatch.get(), L"Q", pos, kb.Q ? Colors::Red : Colors::White);

        pos.x += width * 2;

        comic.DrawString(g_spriteBatch.get(), L"W", pos, kb.W ? Colors::Red : Colors::White);

        pos.x += width * 2;

        comic.DrawString(g_spriteBatch.get(), L"E", pos, kb.E ? Colors::Red : Colors::White);

        pos.x += width * 2;

        comic.DrawString(g_spriteBatch.get(), L"R", pos, kb.R ? Colors::Red : Colors::White);

        pos.x += width * 2;

        comic.DrawString(g_spriteBatch.get(), L"T", pos, kb.T ? Colors::Red : Colors::White);

        pos.x += width * 2;

        comic.DrawString(g_spriteBatch.get(), L"Y", pos, kb.Y ? Colors::Red : Colors::White);

        // Row 3
        pos.x = 50;
        pos.y += height * 2;

        comic.DrawString(g_spriteBatch.get(), L"LeftShift", pos, kb.LeftShift ? Colors::Red : Colors::White);

        pos.x += width * 10;

        comic.DrawString(g_spriteBatch.get(), L"RightShift", pos, kb.RightShift ? Colors::Red : Colors::White);

        // Row 4
        pos.x = 50;
        pos.y += height * 2;

        comic.DrawString(g_spriteBatch.get(), L"LeftCtrl", pos, kb.LeftControl ? Colors::Red : Colors::White);

        pos.x += width * 10;

        comic.DrawString(g_spriteBatch.get(), L"RightCtrl", pos, kb.RightControl ? Colors::Red : Colors::White);

        // Row 5
        pos.x = 50;
        pos.y += height * 2;

        comic.DrawString(g_spriteBatch.get(), L"LeftAlt", pos, kb.LeftAlt ? Colors::Red : Colors::White);

        pos.x += width * 10;

        comic.DrawString(g_spriteBatch.get(), L"RightAlt", pos, kb.RightAlt ? Colors::Red : Colors::White);

       // Row 6
        pos.x = 50;
        pos.y += height * 2;

        comic.DrawString(g_spriteBatch.get(), L"Space", pos, kb.Space ? Colors::Red : Colors::White);

        if ( lastStr )
        {
            comic.DrawString( g_spriteBatch.get(), lastStr, XMFLOAT2( 50, 650 ), Colors::Yellow );
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
                hr = SaveWICTextureToFile( context.Get(), backBufferTex.Get(), GUID_ContainerFormatBmp, L"SCREENSHOT.BMP" );
                hr = SaveDDSTextureToFile( context.Get(), backBufferTex.Get(), L"SCREENSHOT.DDS" );
            }
        }
    }

    g_spriteBatch.reset();

    return 0;
}
