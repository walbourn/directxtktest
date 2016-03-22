//--------------------------------------------------------------------------------------
// File: SpriteBatchTest.cpp
//
// Developer unit test for DirectXTK SpriteBatch
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

#include "SpriteBatch.h"
#include "CommonStates.h"
#include "DirectXColors.h"
#include "DDSTextureLoader.h"
#include "ScreenGrab.h"

#include <wrl\client.h>

#include <wincodec.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

std::unique_ptr<SpriteBatch> g_spriteBatch;

float randf()
{
    return (float)rand() / (float)RAND_MAX * 10000;
}


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

        case WM_KEYDOWN:
            switch( wParam )
            {
            case VK_LEFT:
                if ( g_spriteBatch )
                {
                    g_spriteBatch->SetRotation( DXGI_MODE_ROTATION_ROTATE270 );
                    assert( g_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_ROTATE270 );
                }
                break;

            case VK_RIGHT:
                if ( g_spriteBatch )
                {
                    g_spriteBatch->SetRotation( DXGI_MODE_ROTATION_ROTATE90 );
                    assert( g_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_ROTATE90 );
                }
                break;

            case VK_UP:
                if ( g_spriteBatch )
                {
                    g_spriteBatch->SetRotation( DXGI_MODE_ROTATION_IDENTITY );
                    assert( g_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_IDENTITY );
                }
                break;

            case VK_DOWN:
                if ( g_spriteBatch )
                {
                    g_spriteBatch->SetRotation( DXGI_MODE_ROTATION_ROTATE180 );
                    assert( g_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_ROTATE180 );
                }
                break;

            }
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
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

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
                               1024, 768, nullptr, nullptr, hInstance, nullptr);

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

    ComPtr<ID3D11ShaderResourceView> cat;
    ComPtr<ID3D11Resource> catTexture;
    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"cat.dds", &catTexture, &cat)))
    {
        MessageBox(hwnd, L"Error loading cat.dds", L"SpriteBatchTest", MB_ICONERROR);
        return 1;
    }

    ComPtr<ID3D11ShaderResourceView> letterA;
    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"a.dds", nullptr, &letterA)))
    {
        MessageBox(hwnd, L"Error loading a.dds", L"SpriteBatchTest", MB_ICONERROR);
        return 1;
    }

    ComPtr<ID3D11ShaderResourceView> letterB;
    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"b.dds", nullptr, &letterB)))
    {
        MessageBox(hwnd, L"Error loading b.dds", L"SpriteBatchTest", MB_ICONERROR);
        return 1;
    }

    ComPtr<ID3D11ShaderResourceView> letterC;
    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"c.dds", nullptr, &letterC)))
    {
        MessageBox(hwnd, L"Error loading c.dds", L"SpriteBatchTest", MB_ICONERROR);
        return 1;
    }

    CommonStates states(device.Get());

    g_spriteBatch.reset( new SpriteBatch(context.Get()) );

    bool quit = false;

    D3D11_VIEWPORT vp = { 0, 0, (float)client.right, (float)client.bottom, 0, 1 };

    context->RSSetViewports(1, &vp);

    g_spriteBatch->SetViewport( vp );

    context->OMSetRenderTargets(1, backBuffer.GetAddressOf(), nullptr);

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    size_t frame = 0;

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

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        
        float time = 60 * (float)(counter.QuadPart - start.QuadPart) / (float)freq.QuadPart;

        context->ClearRenderTargetView(backBuffer.Get(), Colors::CornflowerBlue);

        g_spriteBatch->Begin(SpriteSortMode_Deferred, states.NonPremultiplied());

        // Moving
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(900,384.f + sinf(time/60.f)*384.f), nullptr, Colors::White, 0.f, XMFLOAT2(128, 128), 1, SpriteEffects_None, 0);

        // Spinning.
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(200, 150), nullptr, Colors::White, time / 100, XMFLOAT2(128, 128), 1, SpriteEffects_None, 0);

        // Zero size source region.
        RECT src = { 128, 128, 128, 140 };
        RECT dest = { 400, 150, 450, 200 };

        g_spriteBatch->Draw(cat.Get(), dest, &src, Colors::White, time / 100, XMFLOAT2(0, 6), SpriteEffects_None, 0);

        // Differently scaled.
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(0, 0), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.5);

        RECT dest1 = { 0, 0, 256, 64 };
        RECT dest2 = { 0, 0, 64, 256 };

        g_spriteBatch->Draw(cat.Get(), dest1);
        g_spriteBatch->Draw(cat.Get(), dest2);

        // Mirroring.
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(300, 10), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.3f, SpriteEffects_None);
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(350, 10), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.3f, SpriteEffects_FlipHorizontally);
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(400, 10), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.3f, SpriteEffects_FlipVertically);
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(450, 10), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 0.3f, SpriteEffects_FlipBoth);

        // Sorting.
        g_spriteBatch->Draw(letterA.Get(), XMFLOAT2(10, 280), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.1f);
        g_spriteBatch->Draw(letterC.Get(), XMFLOAT2(15, 290), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.9f);
        g_spriteBatch->Draw(letterB.Get(), XMFLOAT2(15, 285), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.5f);

        g_spriteBatch->Draw(letterA.Get(), XMFLOAT2(50, 280), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.9f);
        g_spriteBatch->Draw(letterC.Get(), XMFLOAT2(55, 290), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.1f);
        g_spriteBatch->Draw(letterB.Get(), XMFLOAT2(55, 285), nullptr, Colors::White, 0, XMFLOAT2(0, 0), 1, SpriteEffects_None, 0.5f);

        RECT source = { 16, 32, 256, 192 };

        // Draw overloads specifying position, origin and scale as XMFLOAT2.
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(-40, 320), Colors::Red);
        
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(200, 320), nullptr, Colors::Lime, time / 500, XMFLOAT2(32, 128), 0.5f, SpriteEffects_None, 0.5f);
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(300, 320), &source, Colors::Lime, time / 500, XMFLOAT2(120, 80), 0.5f, SpriteEffects_None, 0.5f);
        
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(350, 320), nullptr, Colors::Blue, time / 500, XMFLOAT2(32, 128), XMFLOAT2(0.25f, 0.5f), SpriteEffects_None, 0.5f);
        g_spriteBatch->Draw(cat.Get(), XMFLOAT2(450, 320), &source, Colors::Blue, time / 500, XMFLOAT2(120, 80), XMFLOAT2(0.5f, 0.25f), SpriteEffects_None, 0.5f);

        // Draw overloads specifying position, origin and scale via the first two components of an XMVECTOR.
        g_spriteBatch->Draw(cat.Get(), XMVectorSet(0, 450, randf(), randf()), Colors::Pink);
        
        g_spriteBatch->Draw(cat.Get(), XMVectorSet(200, 450, randf(), randf()), nullptr, Colors::Lime, time / 500, XMVectorSet(32, 128, randf(), randf()), 0.5f, SpriteEffects_None, 0.5f);
        g_spriteBatch->Draw(cat.Get(), XMVectorSet(300, 450, randf(), randf()), &source, Colors::Lime, time / 500, XMVectorSet(120, 80, randf(), randf()), 0.5f, SpriteEffects_None, 0.5f);
        
        g_spriteBatch->Draw(cat.Get(), XMVectorSet(350, 450, randf(), randf()), nullptr, Colors::Blue, time / 500, XMVectorSet(32, 128, randf(), randf()), XMVectorSet(0.25f, 0.5f, randf(), randf()), SpriteEffects_None, 0.5f);
        g_spriteBatch->Draw(cat.Get(), XMVectorSet(450, 450, randf(), randf()), &source, Colors::Blue, time / 500, XMVectorSet(120, 80, randf(), randf()), XMVectorSet(0.5f, 0.25f, randf(), randf()), SpriteEffects_None, 0.5f);

        // Draw overloads specifying position as a RECT.
        RECT rc1 = { 500, 320, 600, 420 };
        RECT rc2 = { 550, 450, 650, 550 };
        RECT rc3 = { 550, 550, 650, 650 };

        g_spriteBatch->Draw(cat.Get(), rc1, Colors::Gray);
        
        g_spriteBatch->Draw(cat.Get(), rc2, nullptr, Colors::LightSeaGreen, time / 300, XMFLOAT2(128, 128), SpriteEffects_None, 0.5f);
        g_spriteBatch->Draw(cat.Get(), rc3, &source, Colors::LightSeaGreen, time / 300, XMFLOAT2(128, 128), SpriteEffects_None, 0.5f);

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
