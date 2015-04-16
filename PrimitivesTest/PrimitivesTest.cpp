//--------------------------------------------------------------------------------------
// File: PrimitiviesTest.cpp
//
// Developer unit test for DirectXTK Geometric Primitives
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

#include "GeometricPrimitive.h"
#include "Effects.h"
#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "ScreenGrab.h"

#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#pragma warning(pop)

using namespace DirectX;

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
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wndClass);

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 1600, 720, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    RECT client;
    GetClientRect(hwnd, &client);

    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGISwapChain* swapChain;
    ID3D11Texture2D* backBufferTexture;
    ID3D11RenderTargetView* backBuffer;
    ID3D11Texture2D* depthStencilTexture;
    ID3D11DepthStencilView* depthStencil;

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

    if (FAILED(hr = device->CreateRenderTargetView(backBufferTexture, &renderTargetViewDesc, &backBuffer)))
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

    if (FAILED(device->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilTexture)))
        return 1;

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    depthStencilViewDesc.Format = DXGI_FORMAT_UNKNOWN;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    if (FAILED(device->CreateDepthStencilView(depthStencilTexture, &depthStencilViewDesc, &depthStencil)))
        return 1;

    CommonStates states(device);

    ID3D11ShaderResourceView* cat;
    if (FAILED(CreateDDSTextureFromFile(device, L"cat.dds", nullptr, &cat)))
    {
        MessageBox(hwnd, L"Error loading cat.dds", 0, 0);
        return 1;
    }

    ID3D11ShaderResourceView* dxlogo;
    if (FAILED(CreateDDSTextureFromFile(device, L"dx5_logo.dds", nullptr, &dxlogo)))
    {
        MessageBox(hwnd, L"Error loading dx5_logo.dds", 0, 0);
        return 1;
    }

    ID3D11ShaderResourceView* reftxt;
    if (FAILED(CreateDDSTextureFromFile(device, L"reftexture.dds", nullptr, &reftxt)))
    {
        MessageBox(hwnd, L"Error loading reftexture.dds", 0, 0);
        return 1;
    }

#ifdef LH_COORDS
    bool rhcoords = false;
#else
    bool rhcoords = true;
#endif

    auto cube = GeometricPrimitive::CreateCube(context, 1.f, rhcoords );
    auto sphere = GeometricPrimitive::CreateSphere(context, 1.f, 16, rhcoords );
    auto geosphere = GeometricPrimitive::CreateGeoSphere(context, 1.f, 3, rhcoords );
    auto cylinder = GeometricPrimitive::CreateCylinder(context, 1.f, 1.f, 32, rhcoords );
    auto cone = GeometricPrimitive::CreateCone(context, 1.f, 1.f, 32, rhcoords );
    auto torus = GeometricPrimitive::CreateTorus(context, 1.f, 0.333f, 32, rhcoords );
    auto teapot = GeometricPrimitive::CreateTeapot(context, 1.f, 8, rhcoords );
    auto tetra = GeometricPrimitive::CreateTetrahedron(context, 0.75f, rhcoords );
    auto octa = GeometricPrimitive::CreateOctahedron(context, 0.75f, rhcoords );
    auto dodec = GeometricPrimitive::CreateDodecahedron(context, 0.5f, rhcoords );
    auto iso = GeometricPrimitive::CreateIcosahedron(context, 0.5f, rhcoords );

    ID3D11InputLayout* customIL = nullptr;
    std::unique_ptr<BasicEffect> customEffect( new BasicEffect( device ) );
    customEffect->EnableDefaultLighting();
    customEffect->SetTextureEnabled(true);
    customEffect->SetTexture(dxlogo);
    customEffect->SetDiffuseColor( g_XMOne );
    cube->CreateInputLayout( customEffect.get(), &customIL );

    bool quit = false;

    D3D11_VIEWPORT vp = { 0, 0, (float)client.right, (float)client.bottom, 0, 1 };

    context->RSSetViewports(1, &vp);

    context->OMSetRenderTargets(1, &backBuffer, depthStencil);

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    size_t frame = 0;

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

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        
        float time = (float)(counter.QuadPart - start.QuadPart) / (float)freq.QuadPart;

        context->ClearRenderTargetView(backBuffer, Colors::CornflowerBlue);
        context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

        float alphaFade = (sin(time * 2) + 1) / 2;

        if (alphaFade >= 1)
            alphaFade = 1 - FLT_EPSILON;

        float yaw = time * 0.4f;
        float pitch = time * 0.7f;
        float roll = time * 1.1f;

        XMVECTORF32 cameraPosition = { 0, 0, 7 };

        float aspect = (float)client.right / (float)client.bottom;

        XMMATRIX world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
        XMVECTOR quat = XMQuaternionRotationRollPitchYaw( pitch, yaw, roll );

#ifdef LH_COORDS
        XMMATRIX view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
        XMMATRIX projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 10);
#else
        XMMATRIX view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
        XMMATRIX projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 10);
#endif

        customEffect->SetView( view );
        customEffect->SetProjection( projection );

        const float row0 = 2.7f;
        const float row1 = 1.f;
        const float row2 = -0.7f;
        const float row3 = -2.5f;

        const float col0 = -7.5f;
        const float col1 = -5.75f;
        const float col2 = -4.25f;
        const float col3 = -2.7f;
        const float col4 = -1.25f;
        const float col5 = 0.f;
        const float col6 = 1.25f;
        const float col7 = 2.5f;
        const float col8 = 4.25f;
        const float col9 = 5.75f;
        const float col10 = 7.5f;

        // Draw shapes.
        cube->Draw(world * XMMatrixTranslation(col0, row0, 0), view, projection);
        sphere->Draw(world * XMMatrixTranslation(col1, row0, 0), view, projection, Colors::Red);
        geosphere->Draw( world * XMMatrixTranslation(col2, row0, 0), view, projection, Colors::Green );
        cylinder->Draw(world * XMMatrixTranslation(col3, row0, 0), view, projection, Colors::Lime);
        cone->Draw(world * XMMatrixTranslation(col4, row0, 0), view, projection, Colors::Yellow);
        torus->Draw(world * XMMatrixTranslation(col5, row0, 0), view, projection, Colors::Blue);
        teapot->Draw(world * XMMatrixTranslation(col6, row0, 0), view, projection, Colors::CornflowerBlue);
        tetra->Draw(world * XMMatrixTranslation(col7, row0, 0), view, projection, Colors::Red);
        octa->Draw(world * XMMatrixTranslation(col8, row0, 0), view, projection, Colors::Lime);
        dodec->Draw(world * XMMatrixTranslation(col9, row0, 0), view, projection, Colors::Blue);
        iso->Draw(world * XMMatrixTranslation(col10, row0, 0), view, projection, Colors::Cyan);

        // Draw textured shapes.
        cube->Draw(world * XMMatrixTranslation(col0, row1, 0), view, projection, Colors::White, reftxt);
        sphere->Draw(world * XMMatrixTranslation(col1, row1, 0), view, projection, Colors::Yellow, reftxt);
        geosphere->Draw(world * XMMatrixTranslation(col2, row1, 0), view, projection, Colors::White, reftxt);
        cylinder->Draw(world * XMMatrixTranslation(col3, row1, 0), view, projection, Colors::Cyan * 1.2f, reftxt);
        cone->Draw(world * XMMatrixTranslation(col4, row1, 0), view, projection, Colors::Yellow * 1.2f, reftxt);
        torus->Draw(world * XMMatrixTranslation(col5, row1, 0), view, projection, Colors::LightPink, reftxt);
        teapot->Draw(world * XMMatrixTranslation(col6, row1, 0), view, projection, Colors::CornflowerBlue * 1.5, reftxt);
        tetra->Draw(world * XMMatrixTranslation(col7, row1, 0), view, projection, Colors::White, reftxt);
        octa->Draw(world * XMMatrixTranslation(col8, row1, 0), view, projection, Colors::White, reftxt);
        dodec->Draw(world * XMMatrixTranslation(col9, row1, 0), view, projection, Colors::White, reftxt);
        iso->Draw(world * XMMatrixTranslation(col10, row1, 0), view, projection, Colors::White, reftxt);

        // Draw shapes in wireframe.
        cube->Draw(world * XMMatrixTranslation(col0, row2, 0), view, projection, Colors::Gray, nullptr, true);
        sphere->Draw(world * XMMatrixTranslation(col1, row2, 0), view, projection, Colors::Gray, nullptr, true);
        geosphere->Draw(world * XMMatrixTranslation(col2, row2, 0), view, projection, Colors::Gray, nullptr, true);
        cylinder->Draw(world * XMMatrixTranslation(col3, row2, 0), view, projection, Colors::Gray, nullptr, true);
        cone->Draw(world * XMMatrixTranslation(col4, row2, 0), view, projection, Colors::Gray, nullptr, true);
        torus->Draw(world * XMMatrixTranslation(col5, row2, 0), view, projection, Colors::Gray, nullptr, true);
        teapot->Draw(world * XMMatrixTranslation(col6, row2, 0), view, projection, Colors::Gray, nullptr, true);
        tetra->Draw(world * XMMatrixTranslation(col7, row2, 0), view, projection, Colors::Gray, nullptr, true);
        octa->Draw(world * XMMatrixTranslation(col8, row2, 0), view, projection, Colors::Gray, nullptr, true);
        dodec->Draw(world * XMMatrixTranslation(col9, row2, 0), view, projection, Colors::Gray, nullptr, true);
        iso->Draw(world * XMMatrixTranslation(col10, row2, 0), view, projection, Colors::Gray, nullptr, true);

        // Draw shapes with alpha blending.
        cube->Draw(world * XMMatrixTranslation(col0, row3, 0), view, projection, Colors::White * alphaFade);
        cube->Draw(world * XMMatrixTranslation(col1, row3, 0), view, projection, Colors::White * alphaFade, cat);
        
        // Draw shapes with custom device states.
        cube->Draw(world * XMMatrixTranslation(col2, row3, 0), view, projection, Colors::White * alphaFade, cat, false, [&]()
        {
            context->OMSetBlendState(states.NonPremultiplied(), Colors::White, 0xFFFFFFFF);
        });

        // Draw shapes with custom effects.
        XMVECTOR dir = XMVector3Rotate( g_XMOne, quat );
        customEffect->EnableDefaultLighting();
        customEffect->SetFogEnabled(false);
        customEffect->SetLightDirection( 0, dir );
        customEffect->SetWorld( XMMatrixTranslation(col3, row3, 0) );
        cube->Draw( customEffect.get(), customIL );

        customEffect->SetFogEnabled(true);
#ifdef LH_COORDS
        customEffect->SetFogStart(-6);
        customEffect->SetFogEnd(-8);
#else
        customEffect->SetFogStart(6);
        customEffect->SetFogEnd(8);
#endif
        customEffect->SetFogColor(Colors::CornflowerBlue);

        XMMATRIX fbworld = XMMatrixTranslation( 0, 0, cos(time) * 2.f );
        customEffect->SetWorld(fbworld  * XMMatrixTranslation(col5, row3, 0) );

        cube->Draw( customEffect.get(), customIL );

        swapChain->Present(1, 0);
        ++frame;

        if ( frame == 10 )
        {
            ID3D11Texture2D* backBufferTex = nullptr;
            hr = swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&backBufferTex);
            if ( SUCCEEDED(hr) )
            {
                hr = SaveWICTextureToFile( context, backBufferTex, GUID_ContainerFormatBmp, L"SCREENSHOT.BMP" );
                hr = SaveDDSTextureToFile( context, backBufferTex, L"SCREENSHOT.DDS" );
                backBufferTex->Release();
            }
        }

        time++;
    }

    customIL->Release();

    cat->Release();
    dxlogo->Release();
    reftxt->Release();
    depthStencilTexture->Release();
    depthStencil->Release();
    backBuffer->Release();
    backBufferTexture->Release();
    swapChain->Release();
    context->Release();
    device->Release();

    return 0;
}
