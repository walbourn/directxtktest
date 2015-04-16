//--------------------------------------------------------------------------------------
// File: Audio3DTest.cpp
//
// Developer unit test for DirectXTK for Audio - Positional
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

#include <Windows.h>
#include <Dbt.h>

#include "GeometricPrimitive.h"
#include "Effects.h"
#include "CommonStates.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "ScreenGrab.h"

#include "Audio.h"

#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#pragma warning(pop)

using namespace DirectX;

// Build for LH vs. RH coords
//#define LH_COORDS

// Build with Reverb enabled or not
#define USE_REVERB

static const X3DAUDIO_CONE Listener_DirectionalCone = { X3DAUDIO_PI*5.0f/6.0f, X3DAUDIO_PI*11.0f/6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f };

static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_LFE_CurvePoints[3] = { 0.0f, 1.0f, 0.25f, 0.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       Emitter_LFE_Curve          = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_LFE_CurvePoints[0], 3 };

static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_Reverb_CurvePoints[3] = { 0.0f, 0.5f, 0.75f, 1.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       Emitter_Reverb_Curve          = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_Reverb_CurvePoints[0], 3 };

void SetDeviceString( _In_ AudioEngine* engine, _Out_writes_(maxsize) wchar_t* deviceStr, size_t maxsize )
{
    if ( engine->IsAudioDevicePresent() )
    {
        auto wfx = engine->GetOutputFormat();

        const char* speakerConfig;
        switch( wfx.dwChannelMask )
        {
        case SPEAKER_MONO:              speakerConfig = "Mono"; break;
        case SPEAKER_STEREO:            speakerConfig = "Stereo"; break;
        case SPEAKER_2POINT1:           speakerConfig = "2.1"; break;
        case SPEAKER_SURROUND:          speakerConfig = "Surround"; break;
        case SPEAKER_QUAD:              speakerConfig = "Quad"; break;
        case SPEAKER_4POINT1:           speakerConfig = "4.1"; break;
        case SPEAKER_5POINT1:           speakerConfig = "5.1"; break;
        case SPEAKER_7POINT1:           speakerConfig = "7.1"; break;
        case SPEAKER_5POINT1_SURROUND:  speakerConfig = "Surround5.1"; break;
        case SPEAKER_7POINT1_SURROUND:  speakerConfig = "Surround7.1"; break;
        default:                        speakerConfig = "(unknown)"; break;
        }

        swprintf_s( deviceStr, maxsize, L"Output format rate %u, channels %u, %hs (%08X)", wfx.Format.nSamplesPerSec, wfx.Format.nChannels, speakerConfig, wfx.dwChannelMask );
    }
    else
    {
        wcscpy_s( deviceStr, maxsize, L"No default audio device found, running in 'silent mode'" );
    }
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

        case WM_DEVICECHANGE:
            if ( wParam == DBT_DEVICEARRIVAL )
            {
                auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>( lParam );
                if( pDev )
                {
                    if ( pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE )
                    {
                        auto pInter = reinterpret_cast<const PDEV_BROADCAST_DEVICEINTERFACE>( pDev );
                        if ( pInter->dbcc_classguid == KSCATEGORY_AUDIO )
                        {
#ifdef _DEBUG
                            OutputDebugStringA( "INFO: New audio device detected: " );
                            OutputDebugString( pInter->dbcc_name );
                            OutputDebugStringA( "\n" );
#endif
                            PostMessage( hwnd, WM_USER, 0, 0 );
                        }
                    }
                }
            }
            return 0;

        case WM_KEYDOWN:
            if ( !( HIWORD(lParam) & KF_REPEAT ) )
            {
                switch (wParam)
                {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;

                case '1':
                    {
                        auto audEngine = reinterpret_cast<AudioEngine*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
                        if ( audEngine ) 
                            audEngine->SetReverb( Reverb_Default );
                    }
                    break;

                case '2':
                    {
                        auto audEngine = reinterpret_cast<AudioEngine*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
                        if ( audEngine ) 
                            audEngine->SetReverb( Reverb_Bathroom );
                    }
                    break;

                case '3':
                    {
                        auto audEngine = reinterpret_cast<AudioEngine*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
                        if ( audEngine ) 
                            audEngine->SetReverb( Reverb_Cave );
                    }
                    break;

                case '0':
                    {
                        auto audEngine = reinterpret_cast<AudioEngine*>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
                        if ( audEngine ) 
                            audEngine->SetReverb( Reverb_Off );
                    }
                    break;
                }
            }
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}


int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow )
{
    HRESULT hr;

    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
    {
        MessageBoxA(NULL, "COM init failed", 0, 0);
        return 1;
    }

    wchar_t *const className = L"TestWindowClass";

    WNDCLASSEX wndClass = { 0 };

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = className;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wndClass);

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 1600, 720, NULL, NULL, hInstance, NULL);

    DEV_BROADCAST_DEVICEINTERFACE filter = {0};
    filter.dbcc_size = sizeof( filter );
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filter.dbcc_classguid = KSCATEGORY_AUDIO;

    HDEVNOTIFY hNewAudio = RegisterDeviceNotification( hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE );

    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;

#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

#ifdef USE_REVERB
    eflags = eflags | AudioEngine_EnvironmentalReverb | AudioEngine_ReverbUseFilters;
#endif

    std::unique_ptr<AudioEngine> audEngine( new AudioEngine( eflags ) );
    SetWindowLongPtr( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>( audEngine.get() ) );

    wchar_t deviceStr[ 256 ] = {0};
    SetDeviceString( audEngine.get(), deviceStr, 256 );

    std::unique_ptr<SoundEffect> soundEffect( new SoundEffect( audEngine.get(), L"heli.wav" ) );

    auto effect = soundEffect->CreateInstance( SoundEffectInstance_Use3D | SoundEffectInstance_ReverbUseFilters );
    if ( !effect )
    {
        MessageBoxA( hwnd, "Failed loading heli.wav", 0, 0 );
        return 1;
    }

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

#ifdef LH_COORDS
    bool rhcoords = false;
#else
    bool rhcoords = true;
#endif

    auto sphere = GeometricPrimitive::CreateSphere(context, 1.f, 16, rhcoords );

    bool quit = false;

    D3D11_VIEWPORT vp = { 0, 0, (float)client.right, (float)client.bottom, 0, 1 };

    context->RSSetViewports(1, &vp);

    context->OMSetRenderTargets(1, &backBuffer, depthStencil);

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    LARGE_INTEGER start;
    QueryPerformanceCounter(&start);

    size_t frame = 0;

    effect->Play(true);

    AudioListener listener;
    listener.pCone = const_cast<X3DAUDIO_CONE*>( &Listener_DirectionalCone );

    AudioEmitter emitter;

    X3DAUDIO_CONE cone;
    memset( &cone, 0, sizeof(cone) );
    cone.OuterVolume = 1.f;
    cone.OuterLPF = 1.f;
    cone.OuterReverb = 1.f;

    emitter.SetPosition( XMFLOAT3( 10.f, 0.f, 0.f ) );
    emitter.pLFECurve = const_cast<X3DAUDIO_DISTANCE_CURVE*>( &Emitter_LFE_Curve );
    emitter.pReverbCurve = const_cast<X3DAUDIO_DISTANCE_CURVE*>( &Emitter_Reverb_Curve );
    emitter.CurveDistanceScaler = 14.f;
    emitter.pCone = &cone;

    float lastTime = 0.f;

    SpriteBatch spriteBatch(context);

    SpriteFont spriteFont(device, L"comic.spritefont");

    audEngine->SetReverb( Reverb_Default );

    bool critError = false;
    bool retrydefault = false;

    while (!quit)
    {
        if ( retrydefault )
        {
            // We try to reset the audio using the default device
            retrydefault = false;

            if( audEngine->Reset() )
            {
                // It worked, so we reset our sound
                critError = false;
                SetDeviceString( audEngine.get(), deviceStr, 256 );
                effect->Play(true);
            }
        }

        MSG msg;
        bool newaudio = false;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                quit = true;

            if (msg.message == WM_USER)
                newaudio = true;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        
        float time = (float)(counter.QuadPart - start.QuadPart) / (float)freq.QuadPart;

        context->ClearRenderTargetView(backBuffer, Colors::CornflowerBlue);
        context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

        XMVECTORF32 cameraPosition = { 0, 14, 0 };

        XMMATRIX listenerMatrix = XMMatrixTranslation( 0, 0, rhcoords ? 7.f : -7.f );

        float posx = cos(time) * 10.f;
        float posz = sin(time) * 5.f;
        XMMATRIX emitterMatrix = XMMatrixTranslation( posx, 0, rhcoords ? posz : -posz );

        float aspect = (float)client.right / (float)client.bottom;

#ifdef LH_COORDS
        XMMATRIX view = XMMatrixLookAtLH(cameraPosition, listenerMatrix.r[3], XMVectorSet(0, 0, 1, 0));
        XMMATRIX projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 100);
#else
        XMMATRIX view = XMMatrixLookAtRH(cameraPosition, listenerMatrix.r[3], XMVectorSet(0, 0, -1, 0));
        XMMATRIX projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 100);
#endif

        listener.SetPosition( listenerMatrix.r[3] );

        sphere->Draw(listenerMatrix, view, projection, Colors::Yellow);

        sphere->Draw(emitterMatrix, view, projection, Colors::Red);

        float dt = time - lastTime;
        lastTime = time;

        emitter.Update( emitterMatrix.r[3], g_XMIdentityR1, dt );

        effect->Apply3D( listener, emitter );

        if ( !audEngine->Update() )
        {
            // We are running in silent mode
            if ( audEngine->IsCriticalError() )
            {
                if ( !critError )
                {
                    // First will retry using the default audio device
                    retrydefault = true;
                    critError = true;
                }
            }

            if ( newaudio )
            {
                // There's new audio and we are in silent mode, so retry default
                retrydefault = true;
            }
        }

        auto stats = audEngine->GetStatistics();

        wchar_t statsStr[ 256 ];
        swprintf_s( statsStr, L"Playing: %Iu / %Iu; Instances %Iu; Voices %Iu / %Iu / %Iu / %Iu; %Iu audio bytes",
                                stats.playingOneShots, stats.playingInstances,
                                stats.allocatedInstances, stats.allocatedVoices, stats.allocatedVoices3d,
                                stats.allocatedVoicesOneShot, stats.allocatedVoicesIdle,
                                stats.audioBytes );

        spriteBatch.Begin();

        spriteFont.DrawString(&spriteBatch, deviceStr, XMFLOAT2(0, 0));
        spriteFont.DrawString(&spriteBatch, statsStr, XMFLOAT2(0, 36));

        if ( critError )
        {
            spriteFont.DrawString(&spriteBatch, L"ERROR: Critical Error detected", XMFLOAT2(0, 72), Colors::Red );
        }

        spriteBatch.End();

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

    effect->Stop();

    SetWindowLongPtr( hwnd, GWLP_USERDATA, NULL );
    audEngine.reset();

    depthStencilTexture->Release();
    depthStencil->Release();
    backBuffer->Release();
    backBufferTexture->Release();
    swapChain->Release();
    context->Release();
    device->Release();

    UnregisterDeviceNotification( hNewAudio );

    return 0;
}
