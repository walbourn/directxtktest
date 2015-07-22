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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include "Mouse.h"
#include "CommonStates.h"
#include "VertexTypes.h"
#include "DirectXColors.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "ScreenGrab.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "GeometricPrimitive.h"

#include <algorithm>
#include <stdio.h>

#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#pragma warning(pop)

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

using namespace DirectX;

static const XMVECTORF32 START_POSITION = { 0.f, -1.5f, 0.f, 0.f };
static const XMVECTORF32 ROOM_BOUNDS = { 8.f, 6.f, 12.f, 0.f };

std::unique_ptr<SpriteBatch> g_spriteBatch;
std::unique_ptr<Mouse> g_mouse;
std::unique_ptr<GeometricPrimitive> g_room;

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
        case WM_INPUT:
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
        case WM_MOUSEHOVER:
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
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wndClass);

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    RECT client;
    GetClientRect(hwnd, &client);


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

    D3D11_TEXTURE2D_DESC depthStencilDesc = { 0 };

    depthStencilDesc.Width = client.right;
    depthStencilDesc.Height = client.bottom;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;

    ComPtr<ID3D11Texture2D> depthStencilTexture;
    if (FAILED(device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilTexture)))
        return 1;

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    depthStencilViewDesc.Format = DXGI_FORMAT_UNKNOWN;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    ComPtr<ID3D11DepthStencilView> depthStencil;
    if (FAILED(device->CreateDepthStencilView(depthStencilTexture.Get(), &depthStencilViewDesc, &depthStencil)))
        return 1;

    CommonStates states(device.Get());

    g_spriteBatch.reset(new SpriteBatch(context.Get()));

    ComPtr<ID3D11ShaderResourceView> roomTex;
    if (FAILED(hr = CreateDDSTextureFromFile(device.Get(), L"texture.dds", nullptr, roomTex.GetAddressOf())))
        return 1;

    ComPtr<ID3D11ShaderResourceView> cursor;
    if (FAILED(hr = CreateWICTextureFromFile(device.Get(), L"arrow.png", nullptr, cursor.GetAddressOf())))
        return 1;

    g_room = GeometricPrimitive::CreateBox(context.Get(), XMFLOAT3(ROOM_BOUNDS[0], ROOM_BOUNDS[1], ROOM_BOUNDS[2]), false, true);

    XMVECTOR cameraPos = START_POSITION;

    static const XMVECTORF32 up = { 0.f, 1.f, 0.f, 0.f };

    XMMATRIX proj = XMMatrixPerspectiveFovRH(XMConvertToRadians(70.f), float(client.right) / float(client.bottom), 0.01f, 100.f);
    XMMATRIX world = XMMatrixIdentity();

    float pitch = 0;
    float yaw = 0;

    SpriteFont comic(device.Get(), L"comic.spritefont");

    g_mouse.reset( new Mouse );
    g_mouse->SetWindow(hwnd);

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
            MessageBox(hwnd, L"Mouse not acting like a singleton", L"MouseTest", MB_ICONERROR);
        }

        auto state = Mouse::Get().GetState();
        state;
    }

    bool quit = false;

    D3D11_VIEWPORT vp = { 0, 0, (float)client.right, (float)client.bottom, 0, 1 };

    context->RSSetViewports(1, &vp);

    context->OMSetRenderTargets(1, backBuffer.GetAddressOf(), depthStencil.Get());

    size_t frame = 0;

    Mouse::ButtonStateTracker tracker;

    const wchar_t * lastStr = nullptr;
    Mouse::Mode lastMode = Mouse::MODE_ABSOLUTE;
    while (!quit)
    {
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                quit = true;

            if (msg.message == WM_KEYDOWN && msg.wParam == VK_HOME)
                g_mouse->ResetScrollWheelValue();

            if (msg.message == WM_KEYDOWN && msg.wParam == VK_END)
            {
                if (lastMode == Mouse::MODE_ABSOLUTE)
                {
                    g_mouse->SetMode(Mouse::MODE_RELATIVE);
                }
                else
                {
                    g_mouse->SetMode(Mouse::MODE_ABSOLUTE);
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        context->ClearRenderTargetView(backBuffer.Get(), Colors::CornflowerBlue);
        context->ClearDepthStencilView(depthStencil.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

        float y = sinf(pitch);        // vertical
        float r = cosf(pitch);        // in the plane
        float z = r*cosf(yaw);        // fwd-back
        float x = r*sinf(yaw);        // left-right

        XMVECTOR lookAt = XMVectorAdd(cameraPos, XMVectorSet(x, y, z, 0.f));

        XMMATRIX view = XMMatrixLookAtRH(cameraPos, lookAt, up);

        g_room->Draw(world, view, proj, Colors::White, roomTex.Get());

        auto mouse = g_mouse->GetState();
        lastMode = mouse.positionMode;

        if (mouse.positionMode == Mouse::MODE_RELATIVE)
        {
            static const XMVECTORF32 ROTATION_GAIN = { 0.004, 0.004, 0.f, 0.f };
            XMVECTOR delta = XMVectorSet(float(mouse.x), float(mouse.y), 0.f, 0.f) * ROTATION_GAIN;

            pitch -= XMVectorGetY(delta);
            yaw -= XMVectorGetX(delta);

            // limit pitch to straight up or straight down
            float limit = XM_PI/ 2.0f - 0.01f;
            pitch = std::max(-limit, pitch);
            pitch = std::min(+limit, pitch);

            // keep longitude in sane range by wrapping
            if (yaw > XM_PI)
            {
                yaw -= XM_PI * 2.0f;
            }
            else if (yaw < -XM_PI)
            {
                yaw += XM_PI * 2.0f;
            }
        }

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

        if (tracker.leftButton == Mouse::ButtonStateTracker::ButtonState::PRESSED)
        {
            g_mouse->SetMode(Mouse::MODE_RELATIVE);
        }
        else if (tracker.leftButton == Mouse::ButtonStateTracker::ButtonState::RELEASED)
        {
            g_mouse->SetMode(Mouse::MODE_ABSOLUTE);
        }

        XMVECTOR xsize = comic.MeasureString( L"X" );

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

        comic.DrawString(g_spriteBatch.get(), (mouse.positionMode == Mouse::MODE_RELATIVE) ? L"Relative" : L"Absolute",
                         XMFLOAT2(50, 550), Colors::Blue);

        if ( lastStr )
        {
            comic.DrawString( g_spriteBatch.get(), lastStr, XMFLOAT2( 50, 600 ), Colors::Yellow );
        }

        if (mouse.positionMode == Mouse::MODE_ABSOLUTE)
        {
            g_spriteBatch->Draw(cursor.Get(), XMFLOAT2((float) mouse.x, (float) mouse.y));
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
