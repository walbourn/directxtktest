//--------------------------------------------------------------------------------------
// File: ModelTest.cpp
//
// Developer unit test for DirectXTK Model
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

#include "CommonStates.h"
#include "Effects.h"
#include "Model.h"
#include "DirectXColors.h"
#include "DirectXPackedVector.h"
#include "DDSTextureLoader.h"
#include "ScreenGrab.h"

#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#pragma warning(pop)

using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

// Build for LH vs. RH coords
#define LH_COORDS

struct aligned_deleter { void operator()(void* p) { _aligned_free(p); } };

extern std::unique_ptr<Model> CreateModelFromOBJ( _In_ ID3D11Device* d3dDevice, _In_ ID3D11DeviceContext* context, _In_z_ const wchar_t* szFileName,
                                                  _In_ IEffectFactory& fxFactory, bool ccw = true, bool pmalpha = false );

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

    ComPtr<ID3D11ShaderResourceView> defaultTex;
    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"default.dds", nullptr, defaultTex.GetAddressOf())))
        MessageBox(hwnd, L"Error loading default.dds", L"ModelTest", MB_ICONERROR);

    ComPtr<ID3D11ShaderResourceView> cubeMap;
    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"cubemap.dds", nullptr, cubeMap.GetAddressOf())))
        MessageBox(hwnd, L"Error loading cubemap.dds", L"ModelTest", MB_ICONERROR);

    EffectFactory fx(device.Get());

#ifdef LH_COORDS
    bool ccw = false;
#else
    bool ccw = true;
#endif

    std::unique_ptr<XMMATRIX[], aligned_deleter> bones(
        reinterpret_cast<XMMATRIX*>( _aligned_malloc( sizeof(XMMATRIX) * SkinnedEffect::MaxBones, 16 ) ) );

    XMMATRIX id = XMMatrixIdentity();
    for( size_t j=0; j < SkinnedEffect::MaxBones; ++j )
    {
        bones[ j ] = id;
    }

    // Wavefront OBJ
    auto cup = CreateModelFromOBJ( device.Get(), context.Get(), L"cup._obj", fx, !ccw );

    // VBO
    auto vbo = Model::CreateFromVBO(device.Get(), L"player_ship_a.vbo", nullptr, !ccw);

    auto effect = std::make_shared<EnvironmentMapEffect>(device.Get());
    effect->EnableDefaultLighting();
    effect->SetTexture(defaultTex.Get());
    effect->SetEnvironmentMap(cubeMap.Get());

    auto vbo2 = Model::CreateFromVBO(device.Get(), L"player_ship_a.vbo", effect, !ccw);

    // VS 2012 CMO
    auto teapot = Model::CreateFromCMO( device.Get(), L"teapot.cmo", fx, ccw );

    auto gamelevel = Model::CreateFromCMO( device.Get(), L"gamelevel.cmo", fx, ccw );

    auto ship = Model::CreateFromCMO( device.Get(), L"25ab10e8-621a-47d4-a63d-f65a00bc1549_model.cmo", fx, ccw );

    // DirectX SDK Mesh
    auto tiny = Model::CreateFromSDKMESH( device.Get(), L"tiny.sdkmesh", fx, !ccw );
    auto soldier = Model::CreateFromSDKMESH( device.Get(), L"soldier.sdkmesh", fx, !ccw );
    auto dwarf = Model::CreateFromSDKMESH( device.Get(), L"dwarf.sdkmesh", fx, !ccw );
    auto lmap = Model::CreateFromSDKMESH( device.Get(), L"SimpleLightMap.sdkmesh", fx, !ccw );

    bool quit = false;

    D3D11_VIEWPORT vp = { 0, 0, (float)client.right, (float)client.bottom, 0, 1 };

    context->RSSetViewports(1, &vp);

    context->OMSetRenderTargets(1, backBuffer.GetAddressOf(), depthStencil.Get());

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    size_t frame = 0;

    context->OMSetDepthStencilState( states.DepthDefault(), 0 );
    context->OMSetBlendState( states.Opaque(), nullptr, 0xFFFFFFFF );

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
        
        float time = (float)(counter.QuadPart - start.QuadPart) / (float)freq.QuadPart;

        context->ClearRenderTargetView(backBuffer.Get(), Colors::CornflowerBlue);
        context->ClearDepthStencilView(depthStencil.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

        // Compute camera matrices.
        float alphaFade = (sin(time * 2) + 1) / 2;

        if (alphaFade >= 1)
            alphaFade = 1 - FLT_EPSILON;

        float yaw = time * 0.4f;
        float pitch = time * 0.7f;
        float roll = time * 1.1f;

        XMVECTORF32 cameraPosition = { 0, 0, 6 };

        float aspect = (float)client.right / (float)client.bottom;

        XMMATRIX world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
        XMVECTOR quat = XMQuaternionRotationRollPitchYaw( pitch, yaw, roll );

#ifdef LH_COORDS
        XMMATRIX view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
        XMMATRIX projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 15);
#else
        XMMATRIX view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
        XMMATRIX projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 15);
#endif

        const float row0 = 2.f;
        const float row1 = 0.f; 
        const float row2 = -2.f;

        // Draw Wavefront OBJ models
        XMMATRIX local = XMMatrixTranslation( 1.5f, row0, 0.f );
        local = XMMatrixMultiply( world, local );
        cup->Draw( context.Get(), states, local, view, projection );

            // Wireframe
        local = XMMatrixTranslation( 3.f, row0, 0.f );
        local = XMMatrixMultiply( world, local );
        cup->Draw( context.Get(), states, local, view, projection, true );

            // Custom settings
        local = XMMatrixTranslation( 0.f, row0, 0.f );
        local = XMMatrixMultiply( world, local );
        cup->Draw( context.Get(), states, local, view, projection, false, [&]()
        {
            ID3D11ShaderResourceView* srv = nullptr;
            context->PSSetShaderResources( 0, 1, &srv );
        });

            // Lighting settings
        cup->UpdateEffects([&](IEffect* effect)
        {
            auto lights = dynamic_cast<IEffectLights*>(effect);
            if ( lights )
            {
                XMVECTOR dir = XMVector3Rotate( g_XMOne, quat );
                lights->SetLightDirection( 0, dir );
            }
        });
        local = XMMatrixTranslation( -1.5f, row0, 0.f );
        cup->Draw( context.Get(), states, local, view, projection );

            // Fog settings
        cup->UpdateEffects([&](IEffect* effect)
        {
            auto lights = dynamic_cast<IEffectLights*>(effect);
            if ( lights )
                lights->EnableDefaultLighting();
        });
        cup->UpdateEffects([&](IEffect* effect)
        {
            auto fog = dynamic_cast<IEffectFog*>(effect);
            if ( fog )
            {
                fog->SetFogEnabled(true);
#ifdef LH_COORDS
                fog->SetFogStart(-5);
                fog->SetFogEnd(-8);
#else
                fog->SetFogStart(6);
                fog->SetFogEnd(8);
#endif
                fog->SetFogColor(Colors::CornflowerBlue);
            }
        });
        local = XMMatrixTranslation( -3.f, row0, cos(time) * 2.f );
        cup->Draw( context.Get(), states, local, view, projection );

        cup->UpdateEffects([&](IEffect* effect)
        {
            auto fog = dynamic_cast<IEffectFog*>(effect);
            if ( fog )
                fog->SetFogEnabled(false);
        });

        // Draw VBO models
        local = XMMatrixMultiply(XMMatrixScaling(0.25f, 0.25f, 0.25f), XMMatrixTranslation(4.5f, row0, 0.f));
        local = XMMatrixMultiply(world, local);
        vbo->Draw(context.Get(), states, local, view, projection);

        local = XMMatrixMultiply(XMMatrixScaling(0.25f, 0.25f, 0.25f), XMMatrixTranslation(4.5f, row2, 0.f));
        local = XMMatrixMultiply(world, local);
        vbo2->Draw(context.Get(), states, local, view, projection);

        // Skinning settings
        float s = 1 + sin(time * 1.7f) * 0.5f;

        XMMATRIX scale = XMMatrixScaling(s,s,s);

        for (size_t j=0; j < SkinnedEffect::MaxBones; ++j )
        {
            bones[ j ] = scale;
        }

        // Draw CMO models
        teapot->UpdateEffects([&](IEffect* effect)
        {
            auto skinnedEffect = dynamic_cast<IEffectSkinning*>( effect );
            if ( skinnedEffect )
                skinnedEffect->ResetBoneTransforms();
        });
        local = XMMatrixMultiply( XMMatrixScaling( 0.01f, 0.01f, 0.01f ), XMMatrixTranslation( -2.f, row1, 0.f ) );
        local = XMMatrixMultiply( world, local );
        teapot->Draw( context.Get(), states, local, view, projection );

        teapot->UpdateEffects([&](IEffect* effect)
        {
            auto skinnedEffect = dynamic_cast<IEffectSkinning*>( effect );
            if ( skinnedEffect )
                skinnedEffect->SetBoneTransforms(bones.get(), SkinnedEffect::MaxBones);
        });
        local = XMMatrixMultiply( XMMatrixScaling( 0.01f, 0.01f, 0.01f ), XMMatrixTranslation( -3.5f, row1, 0.f ) );
        local = XMMatrixMultiply( world, local );
        teapot->Draw( context.Get(), states, local, view, projection );

        local = XMMatrixMultiply( XMMatrixScaling( 0.1f, 0.1f, 0.1f ), XMMatrixTranslation( 0.f, row1, 0.f ) );
        local = XMMatrixMultiply( world, local );
        gamelevel->Draw( context.Get(), states, local, view, projection );

        local = XMMatrixMultiply( XMMatrixScaling( .2f, .2f, .2f ), XMMatrixTranslation( 0.f, row2, 0.f ) );
        local = XMMatrixMultiply( world, local );
        ship->Draw( context.Get(), states, local, view, projection );

        // Draw SDKMESH models
        local = XMMatrixMultiply( XMMatrixScaling( 0.005f, 0.005f, 0.005f ), XMMatrixTranslation( 2.5f, row2, 0.f ) );
        local = XMMatrixMultiply( world, local );
        tiny->Draw( context.Get(), states, local, view, projection );

        local = XMMatrixTranslation(-2.5f, row2, 0.f );
        local = XMMatrixMultiply( world, local );
        dwarf->Draw( context.Get(), states, local, view, projection );

        local = XMMatrixMultiply( XMMatrixScaling( 0.01f, 0.01f, 0.01f ), XMMatrixTranslation( -5.0f, row2, 0.f ) );
        local = XMMatrixMultiply( XMMatrixRotationRollPitchYaw(0, XM_PI, roll), local );
        lmap->Draw( context.Get(), states, local, view, projection );

        soldier->UpdateEffects([&](IEffect* effect)
        {
            auto skinnedEffect = dynamic_cast<IEffectSkinning*>( effect );
            if ( skinnedEffect )
                skinnedEffect->ResetBoneTransforms();
        });
        local = XMMatrixMultiply( XMMatrixScaling( 2.f, 2.f, 2.f ), XMMatrixTranslation( 3.5f, row1, 0.f ) );
        local = XMMatrixMultiply( world, local );
        soldier->Draw( context.Get(), states, local, view, projection );

        soldier->UpdateEffects([&](IEffect* effect)
        {
            auto skinnedEffect = dynamic_cast<IEffectSkinning*>( effect );
            if ( skinnedEffect )
                skinnedEffect->SetBoneTransforms(bones.get(), SkinnedEffect::MaxBones);
        });
        local = XMMatrixMultiply( XMMatrixScaling( 2.f, 2.f, 2.f ), XMMatrixTranslation( 2.5f, row1, 0.f ) );
        soldier->Draw( context.Get(), states, local, view, projection );

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

        time++;
    }

    cup.reset(nullptr);

    return 0;
}

