//--------------------------------------------------------------------------------------
// File: SpriteFontTest.cpp
//
// Developer unit test for DirectXTK SpriteFont
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


#include "SpriteFont.h"
#include "DirectXColors.h"
#include "ScreenGrab.h"

#include <wincodec.h>

#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

std::unique_ptr<SpriteBatch> g_spriteBatch;

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
                    g_spriteBatch->SetRotation( DXGI_MODE_ROTATION_ROTATE270 );
                break;

            case VK_RIGHT:
                if ( g_spriteBatch )
                    g_spriteBatch->SetRotation( DXGI_MODE_ROTATION_ROTATE90 );
                break;

            case VK_UP:
                if ( g_spriteBatch )
                    g_spriteBatch->SetRotation( DXGI_MODE_ROTATION_IDENTITY );
                break;

            case VK_DOWN:
                if ( g_spriteBatch )
                    g_spriteBatch->SetRotation( DXGI_MODE_ROTATION_ROTATE180 );
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

    UINT width = 1024;
    UINT height = 768;

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, hInstance, nullptr);

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

    g_spriteBatch.reset( new SpriteBatch(context.Get()) );

    SpriteFont comicFont(device.Get(), L"comic.spritefont");
    SpriteFont italicFont(device.Get(), L"italic.spritefont");
    SpriteFont scriptFont(device.Get(), L"script.spritefont");
    SpriteFont nonproportionalFont(device.Get(), L"nonproportional.spritefont");
    SpriteFont multicoloredFont(device.Get(), L"multicolored.spritefont");
    SpriteFont japaneseFont(device.Get(), L"japanese.spritefont");
    SpriteFont ctrlFont(device.Get(), L"xboxController.spritefont");
    SpriteFont consolasFont(device.Get(), L"consolas.spritefont");

    if ( comicFont.GetDefaultCharacter() != 0 )
    {
        MessageBox( hwnd, L"GetDefaultCharacter failed", L"SpriteFontTest", MB_OK | MB_ICONERROR );
    }

    // ContainsCharacter tests
    if ( comicFont.ContainsCharacter( 27 ) 
         || !comicFont.ContainsCharacter( '-' ) )
    {
        MessageBox( hwnd, L"ContainsCharacter failed", L"SpriteFontTest", MB_OK | MB_ICONERROR );
    }

    // FindGlyph/GetSpriteSheet tests
    {
        auto g = comicFont.FindGlyph( '-' );
        if ( g->Character != '-' || g->XOffset != 6 || g->YOffset != 24 )
        {
            MessageBox( hwnd, L"FindGlyph failed", L"SpriteFontTest", MB_OK | MB_ICONERROR );
        }

        ComPtr<ID3D11ShaderResourceView> sheet;
        comicFont.GetSpriteSheet( sheet.GetAddressOf() );
        if ( !sheet )
        {
            MessageBox( hwnd, L"GetSpriteSheet failed", L"SpriteFontTest", MB_OK | MB_ICONERROR );
        }
    }

    // DefaultCharacter tests
    comicFont.SetDefaultCharacter('-');
    if ( comicFont.GetDefaultCharacter() != '-' )
    {
        MessageBox( hwnd, L"Get/SetDefaultCharacter failed", L"SpriteFontTest", MB_OK | MB_ICONERROR );
    }

    // Linespacing tests
    float s=ctrlFont.GetLineSpacing();
    if ( s != 186.f )
    {
        MessageBox( hwnd, L"GetLineSpacing failed", L"SpriteFontTest", MB_OK | MB_ICONERROR );
    }
    ctrlFont.SetLineSpacing(256.f);
    s=ctrlFont.GetLineSpacing();
    if ( s != 256.f )
    {
        MessageBox( hwnd, L"Get/SetLineSpacing failed", L"SpriteFontTest", MB_OK | MB_ICONERROR );
    }
    ctrlFont.SetLineSpacing(186.f);

    bool quit = false;

    D3D11_VIEWPORT vp = { 0, 0, (float)client.right, (float)client.bottom, 0, 1 };

    context->RSSetViewports(1, &vp);

    context->OMSetRenderTargets(1, backBuffer.GetAddressOf(), nullptr);

    ComPtr<ID3D11RasterizerState> scissorState;
    CD3D11_RASTERIZER_DESC rsDesc(D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE, 0, 0.f, 0.f, TRUE, TRUE, TRUE, FALSE);
    if (FAILED(device->CreateRasterizerState(&rsDesc, &scissorState)))
        return 1;

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

        g_spriteBatch->Begin();

        comicFont.DrawString(g_spriteBatch.get(), L"Hello, world!", XMFLOAT2(0, 0));
        italicFont.DrawString(g_spriteBatch.get(), L"This text is in italics.\nIs it well spaced?", XMFLOAT2(220, 0));
        scriptFont.DrawString(g_spriteBatch.get(), L"Script font, yo...", XMFLOAT2(0, 50));
        
        SpriteEffects flip = (SpriteEffects)((int)(time / 100) & 3);
        multicoloredFont.DrawString(g_spriteBatch.get(), L"OMG it's full of stars!", XMFLOAT2(610, 130), Colors::White, XM_PIDIV2, XMFLOAT2(0, 0), 1, flip);
        
        comicFont.DrawString(g_spriteBatch.get(), L"This is a larger block\nof text using a\nfont scaled to a\nsmaller size.\nSome c\x1234ha\x1543rac\x2453te\x1634r\x1563s are not in the font, but should show up as hyphens.", XMFLOAT2(10, 90), Colors::Black, 0, XMFLOAT2(0, 0), 0.5f);

        wchar_t tmp[256] = { 0 };
        swprintf_s(tmp, L"%Iu frames", frame );
        
        nonproportionalFont.DrawString(g_spriteBatch.get(), tmp, XMFLOAT2(201, 130), Colors::Black);
        nonproportionalFont.DrawString(g_spriteBatch.get(), tmp, XMFLOAT2(200, 131), Colors::Black);
        nonproportionalFont.DrawString(g_spriteBatch.get(), tmp, XMFLOAT2(200, 130), Colors::Red);
        
        float scale = sin(time / 100) + 1;
        auto spinText = L"Spinning\nlike a cat";
        auto size = comicFont.MeasureString(spinText);
        comicFont.DrawString(g_spriteBatch.get(), spinText, XMVectorSet(150, 350, 0, 0), Colors::Blue, time / 60, size / 2, scale);
        
        auto mirrorText = L"It's a\nmirror...";
        auto mirrorSize = comicFont.MeasureString(mirrorText);
        comicFont.DrawString(g_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), Colors::Black, 0, mirrorSize * XMVectorSet(0, 1, 0, 0), 1, SpriteEffects_None);
        comicFont.DrawString(g_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), Colors::Gray, 0, mirrorSize * XMVectorSet(1, 1, 0, 0), 1, SpriteEffects_FlipHorizontally);
        comicFont.DrawString(g_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), Colors::Gray, 0, mirrorSize * XMVectorSet(0, 0, 0, 0), 1, SpriteEffects_FlipVertically);
        comicFont.DrawString(g_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), Colors::DarkGray, 0, mirrorSize * XMVectorSet(1, 0, 0, 0), 1, SpriteEffects_FlipBoth);

        japaneseFont.DrawString(g_spriteBatch.get(), L"\x79C1\x306F\x65E5\x672C\x8A9E\x304C\x8A71\x305B\x306A\x3044\x306E\x3067\x3001\n\x79C1\x306F\x3053\x308C\x304C\x4F55\x3092\x610F\x5473\x3059\x308B\x306E\x304B\x308F\x304B\x308A\x307E\x305B\x3093", XMFLOAT2(10, 512));

        {
            char ascii[256] = {0};
            int i = 0;
            for (size_t j = 32; j < 256; ++j)
            {
                if (j == L'\n' || j == L'\r' || j == L'\t')
                    continue;

                if (j > 0 && (j % 128) == 0)
                {
                    ascii[i++] = L'\n';
                    ascii[i++] = L'\n';
                }

                ascii[i++] = static_cast<char>(j + 1);
            }

            wchar_t unicode[256] = { 0 };
            if (!MultiByteToWideChar(437, MB_PRECOMPOSED, ascii, i, unicode, 256))
                wcscpy_s(unicode, L"<ERROR!>\n");

            consolasFont.DrawString(g_spriteBatch.get(), unicode, XMFLOAT2(10, 600), Colors::Cyan);
        }

        ctrlFont.DrawString(g_spriteBatch.get(), L" !\"\n#$%\n&'()\n*+,-", XMFLOAT2(650, 130), Colors::White, 0.f, XMFLOAT2(0.f, 0.f), 0.5f);

        UINT w, h;

        switch( g_spriteBatch->GetRotation() )
        {
        case DXGI_MODE_ROTATION_ROTATE90:
        case DXGI_MODE_ROTATION_ROTATE270:
            w = height;
            h = width;
            break;

        default:
            w = width;
            h = height;
            break;
        }

        for( UINT x = 0; x < w; x += 100 )
        {
            swprintf_s( tmp, L"%u\n", x );
            nonproportionalFont.DrawString( g_spriteBatch.get(), tmp, XMFLOAT2( float(x), float( h - 100 ) ), Colors::Yellow );
        }

        for( UINT y = 0; y < h; y += 100 )
        {
            swprintf_s( tmp, L"%u\n", y );
            nonproportionalFont.DrawString( g_spriteBatch.get(), tmp, XMFLOAT2( float( w - 100 ), float(y) ), Colors::Red );
        }

        g_spriteBatch->End();

        g_spriteBatch->Begin(SpriteSortMode_Deferred, nullptr, nullptr, nullptr, scissorState.Get(), [&]()
        {
            CD3D11_RECT r(640, 20, 740, 38);
            context->RSSetScissorRects(1, &r);
        });

        comicFont.DrawString(g_spriteBatch.get(), L"Clipping!", XMFLOAT2(640, 0), Colors::DarkGreen);

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
