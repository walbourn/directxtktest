//--------------------------------------------------------------------------------------
// File: MouseTest.cpp
//
// Developer unit test for DirectXTK Mouse (Win32)
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Mouse.h"
#include "CommonStates.h"
#include "VertexTypes.h"
#include "DirectXColors.h"
#include "WICTextureLoader.h"
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
std::unique_ptr<Mouse> g_mouse;

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

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            Mouse::ProcessMessage(msg, wParam, lParam);
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

    ComPtr<ID3D11ShaderResourceView> cursor;
    if (FAILED(hr = CreateWICTextureFromFile(device.Get(), L"arrow.png", nullptr, cursor.GetAddressOf())))
        return 1;

    SpriteFont comic(device.Get(), L"comic.spritefont");

    g_mouse.reset( new Mouse );

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

    Mouse::ButtonStateTracker tracker;

    const wchar_t * lastStr = nullptr;
    wchar_t lastStrBuff[128] = { 0 };
    while (!quit)
    {
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                quit = true;

            if (msg.message == WM_KEYDOWN && msg.wParam == VK_HOME)
                g_mouse->ResetScrollWheelValue();

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto mouse = g_mouse->GetState();

        tracker.Update(mouse);

        if (tracker.leftButton == Mouse::ButtonStateTracker::ButtonState::PRESSED)
            lastStr = L"LeftButton was pressed";
        else if (tracker.leftButton == Mouse::ButtonStateTracker::ButtonState::RELEASED)
            lastStr = L"LeftButton was released";
        else if (tracker.rightButton == Mouse::ButtonStateTracker::ButtonState::PRESSED)
            lastStr = L"RightButton was pressed";
        else if (tracker.rightButton == Mouse::ButtonStateTracker::ButtonState::RELEASED)
            lastStr = L"RightButton was released";
        else if (tracker.middleButton == Mouse::ButtonStateTracker::ButtonState::PRESSED)
            lastStr = L"MiddleButton was pressed";
        else if (tracker.middleButton == Mouse::ButtonStateTracker::ButtonState::RELEASED)
            lastStr = L"MiddleButton was released";
        else if (tracker.xButton1 == Mouse::ButtonStateTracker::ButtonState::PRESSED)
            lastStr = L"XButton1 was pressed";
        else if (tracker.xButton1 == Mouse::ButtonStateTracker::ButtonState::RELEASED)
            lastStr = L"XButton1 was released";
        else if (tracker.xButton2 == Mouse::ButtonStateTracker::ButtonState::PRESSED)
            lastStr = L"XButton2 was pressed";
        else if (tracker.xButton2 == Mouse::ButtonStateTracker::ButtonState::RELEASED)
            lastStr = L"XButton2 was released";

        context->ClearRenderTargetView(backBuffer.Get(), Colors::CornflowerBlue);

        XMVECTOR xsize = comic.MeasureString( L"X" );

        float width = XMVectorGetX(xsize);
        float height = XMVectorGetY(xsize);

        g_spriteBatch->Begin();

        XMFLOAT2 pos(50, 50);

        // Buttons
        comic.DrawString(g_spriteBatch.get(), L"LeftButton", pos, mouse.leftButton ? Colors::Red : Colors::White);
        pos.y += height * 2;

        comic.DrawString(g_spriteBatch.get(), L"RightButton", pos, mouse.rightButton ? Colors::Red : Colors::White);
        pos.y += height * 2;

        comic.DrawString(g_spriteBatch.get(), L"MiddleButton", pos, mouse.middleButton ? Colors::Red : Colors::White);
        pos.y += height * 2;

        comic.DrawString(g_spriteBatch.get(), L"XButton1", pos, mouse.xButton1 ? Colors::Red : Colors::White);
        pos.y += height * 2;

        comic.DrawString(g_spriteBatch.get(), L"XButton2", pos, mouse.xButton2 ? Colors::Red : Colors::White);

        // Scroll Wheel
        pos.y += height * 2;
        {
            wchar_t buff[16];
            swprintf_s(buff, L"%d", mouse.scrollWheelValue);
            comic.DrawString(g_spriteBatch.get(), buff, pos, Colors::White);
        }

        if ( lastStr )
        {
            comic.DrawString( g_spriteBatch.get(), lastStr, XMFLOAT2( 50, 650 ), Colors::Yellow );
        }

        g_spriteBatch->Draw(cursor.Get(), XMFLOAT2( (float)mouse.x, (float)mouse.y));

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
