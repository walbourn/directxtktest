//--------------------------------------------------------------------------------------
// File: LoadTest.cpp
//
// Developer unit test for DirectXTK DDSTextureLoader & WICTextureLoader
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
#include "VertexTypes.h"
#include "DirectXColors.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "ScreenGrab.h"

#include "Shaders\Compiled\LoadTest_VS2D.inc"
#include "Shaders\Compiled\LoadTest_PS2D.inc"

#include <wrl/client.h>

#include <wincodec.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Build for LH vs. RH coords
//#define LH_COORDS

struct CBNeverChanges
{
    XMMATRIX mView;
};

struct CBChangeOnResize
{
    XMMATRIX mProjection;
};

struct CBChangesEveryFrame
{
    XMMATRIX mWorld;
    XMFLOAT4 vEyePosition;
    XMFLOAT4 vMeshColor;
};

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

    WNDCLASSEX wndClass = {};

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = className;
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wndClass);

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 640, 640, nullptr, nullptr, hInstance, nullptr);

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

    ComPtr<ID3D11VertexShader> vertexShader;
    if (FAILED(device->CreateVertexShader( LoadTest_VS2D, sizeof(LoadTest_VS2D), nullptr, &vertexShader )))
        return 1;

    ComPtr<ID3D11PixelShader> pixelShader;
    if (FAILED(device->CreatePixelShader( LoadTest_PS2D, sizeof(LoadTest_PS2D), nullptr, &pixelShader )))
        return 1;

    ComPtr<ID3D11InputLayout> vertexLayout;
    if (FAILED(device->CreateInputLayout( VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount,
                                          LoadTest_VS2D, sizeof(LoadTest_VS2D), &vertexLayout ) ) )
        return 1;

#ifdef LH_COORDS
    static const float hi = 1.f;
    static const float low = 0.f;
#else
    static const float hi = 0.f;
    static const float low = 1.f;
#endif

    static const VertexPositionNormalTexture vertices[] =
    {
        VertexPositionNormalTexture( XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT3( 0.0f, 1.0f, 0.0f), XMFLOAT2( hi, 0.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT3( 0.0f, 1.0f, 0.0f), XMFLOAT2( low, 0.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT3( 0.0f, 1.0f, 0.0f), XMFLOAT2( low, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT3( 0.0f, 1.0f, 0.0f), XMFLOAT2( hi, 1.0f ) ),

        VertexPositionNormalTexture( XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT3( 0.0f, -1.0f, 0.0f), XMFLOAT2( low, 0.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT3( 0.0f, -1.0f, 0.0f), XMFLOAT2( hi, 0.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT3( 0.0f, -1.0f, 0.0f), XMFLOAT2( hi, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT3( 0.0f, -1.0f, 0.0f), XMFLOAT2( low, 1.0f ) ),

        VertexPositionNormalTexture( XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT3( -1.0f, 0.0f, 0.0f), XMFLOAT2( low, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT3( -1.0f, 0.0f, 0.0f), XMFLOAT2( hi, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT3( -1.0f, 0.0f, 0.0f), XMFLOAT2( hi, 0.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT3( -1.0f, 0.0f, 0.0f), XMFLOAT2( low, 0.0f ) ),

        VertexPositionNormalTexture( XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT3( 1.0f, 0.0f, 0.0f), XMFLOAT2( hi, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT3( 1.0f, 0.0f, 0.0f), XMFLOAT2( low, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT3( 1.0f, 0.0f, 0.0f), XMFLOAT2( low, 0.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT3( 1.0f, 0.0f, 0.0f), XMFLOAT2( hi, 0.0f ) ),

        VertexPositionNormalTexture( XMFLOAT3( -1.0f, -1.0f, -1.0f ), XMFLOAT3( 0.0f, 0.0f, -1.0f), XMFLOAT2( low, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, -1.0f, -1.0f ), XMFLOAT3( 0.0f, 0.0f, -1.0f), XMFLOAT2( hi, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, 1.0f, -1.0f ), XMFLOAT3( 0.0f, 0.0f, -1.0f), XMFLOAT2( hi, 0.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( -1.0f, 1.0f, -1.0f ), XMFLOAT3( 0.0f, 0.0f, -1.0f), XMFLOAT2( low, 0.0f ) ),

        VertexPositionNormalTexture( XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT3( 0.0f, 0.0f, 1.0f), XMFLOAT2( hi, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, -1.0f, 1.0f ), XMFLOAT3( 0.0f, 0.0f, 1.0f), XMFLOAT2( low, 1.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( 1.0f, 1.0f, 1.0f ), XMFLOAT3( 0.0f, 0.0f, 1.0f), XMFLOAT2( low, 0.0f ) ),
        VertexPositionNormalTexture( XMFLOAT3( -1.0f, 1.0f, 1.0f ), XMFLOAT3( 0.0f, 0.0f, 1.0f), XMFLOAT2( hi, 0.0f ) ),
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( VertexPositionNormalTexture ) * 24;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = vertices;

    ComPtr<ID3D11Buffer> vertexBuffer;
    if (FAILED(device->CreateBuffer( &bd, &InitData, &vertexBuffer )))
        return 1;

    UINT stride = sizeof( VertexPositionNormalTexture );
    UINT offset = 0;

    static const WORD indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( WORD ) * 36;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;

    ComPtr<ID3D11Buffer> indexBuffer;
    if (FAILED(device->CreateBuffer( &bd, &InitData, &indexBuffer )))
        return 1;

    context->VSSetShader( vertexShader.Get(), nullptr, 0 );
    context->PSSetShader( pixelShader.Get(), nullptr, 0 );
    context->IASetInputLayout( vertexLayout.Get() );
    context->IASetVertexBuffers( 0, 1, vertexBuffer.GetAddressOf(), &stride, &offset );
    context->IASetIndexBuffer( indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 );
    context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChanges);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    ComPtr<ID3D11Buffer> cbNeverChanges;
    if (FAILED(device->CreateBuffer( &bd, nullptr, &cbNeverChanges )))
        return 1;
    
    bd.ByteWidth = sizeof(CBChangeOnResize);

    ComPtr<ID3D11Buffer> cbChangeOnResize;
    if (FAILED(device->CreateBuffer( &bd, nullptr, &cbChangeOnResize )))
        return 1;
    
    bd.ByteWidth = sizeof(CBChangesEveryFrame);

    ComPtr<ID3D11Buffer> cbChangesEveryFrame;
    hr = device->CreateBuffer( &bd, nullptr, &cbChangesEveryFrame );
    if( FAILED( hr ) )
        return hr;

    // Initialize matrices
    XMMATRIX world = XMMatrixIdentity();

    static const XMVECTORF32 eyePosition = { 0.0f, 3.0f, -6.0f, 0.0f };
    static const XMVECTORF32 At = { 0.0f, 1.0f, 0.0f, 0.0f };
    static const XMVECTORF32 Up = { 0.0f, 1.0f, 0.0f, 0.0f };

#ifdef LH_COORDS
    XMMATRIX view = XMMatrixLookAtLH( eyePosition, At, Up );
    XMMATRIX proj = XMMatrixPerspectiveFovLH( XM_PIDIV4, 1.f, 0.01f, 100.0f );
#else
    XMMATRIX view = XMMatrixLookAtRH( eyePosition, At, Up );
    XMMATRIX proj = XMMatrixPerspectiveFovRH( XM_PIDIV4, 1.f, 0.01f, 100.0f );
#endif

    CBNeverChanges cbNever;
    cbNever.mView = XMMatrixTranspose( view );
    context->UpdateSubresource( cbNeverChanges.Get(), 0, nullptr, &cbNever, 0, 0 );
    
    CBChangeOnResize cbChanges;
    cbChanges.mProjection = XMMatrixTranspose( proj );
    context->UpdateSubresource( cbChangeOnResize.Get(), 0, nullptr, &cbChanges, 0, 0 );

    // Alpha mode test
    DDS_ALPHA_MODE alphaMode;
    {
        ComPtr<ID3D11ShaderResourceView> tree;
        if (FAILED(CreateDDSTextureFromFile(device.Get(), L"tree02S_pmalpha.dds", nullptr, &tree, 0, &alphaMode)) || alphaMode != DDS_ALPHA_MODE_PREMULTIPLIED)
        {
            MessageBox(hwnd, L"Error loading tree02S_pmalpha.dds", L"LoadTest", MB_ICONERROR);
        }
    }

    // Various dimension loads
    {
        ComPtr<ID3D11ShaderResourceView> test;
        if (FAILED(CreateDDSTextureFromFile(device.Get(), L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds", nullptr, &test, 0 )))
        {
            MessageBox(hwnd, L"Error loading io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds", L"LoadTest", MB_ICONERROR);
        }
        else
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            test->GetDesc( &desc );
            
            if ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                 || desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURE1D
                 || desc.Texture1D.MipLevels != 1 )
            {
                MessageBox(hwnd, L"Error validating io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds", L"LoadTest", MB_ICONERROR);
            }
        }
    }

    {
        ComPtr<ID3D11ShaderResourceView> test;
        if (FAILED(CreateDDSTextureFromFile(device.Get(), L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds", nullptr, &test, 0 )))
        {
            MessageBox(hwnd, L"Error loading io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds", L"LoadTest", MB_ICONERROR);
        }
        else
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            test->GetDesc( &desc );
            
            if ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                 || desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURE1DARRAY
                 || desc.Texture1DArray.MipLevels != 1
                 || desc.Texture1DArray.ArraySize != 6 )
            {
                MessageBox(hwnd, L"Error validating io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds", L"LoadTest", MB_ICONERROR);
            }
        }
    }

    {
        ComPtr<ID3D11ShaderResourceView> test;
        if (FAILED(CreateDDSTextureFromFile(device.Get(), L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds", nullptr, &test, 0 )))
        {
            MessageBox(hwnd, L"Error loading io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds", L"LoadTest", MB_ICONERROR);
        }
        else
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            test->GetDesc( &desc );
            
            if ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                 || desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURE2DARRAY
                 || desc.Texture2DArray.MipLevels != 1
                 || desc.Texture2DArray.ArraySize != 6 )
            {
                MessageBox(hwnd, L"Error validating io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds", L"LoadTest", MB_ICONERROR);
            }
        }
    }

    {
        ComPtr<ID3D11ShaderResourceView> test;
        if (FAILED(CreateDDSTextureFromFile(device.Get(), L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds", nullptr, &test, 0 )))
        {
            MessageBox(hwnd, L"Error loading io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds", L"LoadTest", MB_ICONERROR);
        }
        else
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            test->GetDesc( &desc );
            
            if ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                 || desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURE3D
                 || desc.Texture3D.MipLevels != 1)
            {
                MessageBox(hwnd, L"Error validating io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds", L"LoadTest", MB_ICONERROR);
            }
        }
    }

    // Auto-gen mip various dimension loads
    {
        ComPtr<ID3D11ShaderResourceView> test;
        if (FAILED(CreateDDSTextureFromFile(device.Get(), context.Get(), L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds", nullptr, &test, 0 )))
        {
            MessageBox(hwnd, L"Error autogen loading io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds", L"LoadTest", MB_ICONERROR);
        }
        else
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            test->GetDesc( &desc );
            
            if ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                 || desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURE1D
                 || desc.Texture1D.MipLevels != 6 )
            {
                MessageBox(hwnd, L"Error validating autogen io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds", L"LoadTest", MB_ICONERROR);
            }
        }
    }

    {
        ComPtr<ID3D11ShaderResourceView> test;
        if (FAILED(CreateDDSTextureFromFile(device.Get(), context.Get(), L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds", nullptr, &test, 0 )))
        {
            MessageBox(hwnd, L"Error autogen loading io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds", L"LoadTest", MB_ICONERROR);
        }
        else
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            test->GetDesc( &desc );
            
            if ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                 || desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURE1DARRAY
                 || desc.Texture1DArray.MipLevels != 6
                 || desc.Texture1DArray.ArraySize != 6 )
            {
                MessageBox(hwnd, L"Error validating autogen io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds", L"LoadTest", MB_ICONERROR);
            }
        }
    }

    {
        ComPtr<ID3D11ShaderResourceView> test;
        if (FAILED(CreateDDSTextureFromFile(device.Get(), context.Get(), L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds", nullptr, &test, 0 )))
        {
            MessageBox(hwnd, L"Error autogen loading io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds", L"LoadTest", MB_ICONERROR);
        }
        else
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            test->GetDesc( &desc );
            
            if ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                 || desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURE2DARRAY
                 || desc.Texture2DArray.MipLevels != 8
                 || desc.Texture2DArray.ArraySize != 6 )
            {
                MessageBox(hwnd, L"Error validating autogen io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds", L"LoadTest", MB_ICONERROR);
            }
        }
    }

    {
        ComPtr<ID3D11ShaderResourceView> test;
        if (FAILED(CreateDDSTextureFromFile(device.Get(), context.Get(), L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds", nullptr, &test, 0 )))
        {
            MessageBox(hwnd, L"Error autogen loading io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds", L"LoadTest", MB_ICONERROR);
        }
        else
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            test->GetDesc( &desc );
            
            if ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                 || desc.ViewDimension != D3D_SRV_DIMENSION_TEXTURE3D
                 || desc.Texture3D.MipLevels != 8)
            {
                MessageBox(hwnd, L"Error validating autogen io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds", L"LoadTest", MB_ICONERROR);
            }
        }
    }

    // sRGB test
    {
        ComPtr<ID3D11ShaderResourceView> test;
        if (FAILED(CreateWICTextureFromFile(device.Get(), L"cup_small.jpg", nullptr, &test, 0 )))
        {
            MessageBox(hwnd, L"Error loading cup_small.jpg", L"LoadTest", MB_ICONERROR);
        }
        else
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            test->GetDesc( &desc );
            
            if ( desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB )
            {
                MessageBox(hwnd, L"Error validating cup_small.jpg", L"LoadTest", MB_ICONERROR);
            }
        }
    }


    // View textures
    ComPtr<ID3D11ShaderResourceView> earth;
    if (FAILED(CreateDDSTextureFromFile(device.Get(), L"earth_A2B10G10R10.dds", nullptr, &earth, 0, &alphaMode)) || alphaMode != DDS_ALPHA_MODE_UNKNOWN)
    {
        MessageBox(hwnd, L"Error loading earth_A2B10G10R10.dds", L"LoadTest", MB_ICONERROR);
        return 1;
    }

    ComPtr<ID3D11ShaderResourceView> earth2;
    if (FAILED(CreateDDSTextureFromFileEx(device.Get(), L"earth_A2B10G10R10.dds", 0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, true, nullptr, &earth2)))
    {
        MessageBox(hwnd, L"Error loading earth_A2B10G10R10.dds (2)", L"LoadTest", MB_ICONERROR);
        return 1;
    }

    ComPtr<ID3D11ShaderResourceView> dxlogo;
    if (FAILED(CreateDDSTextureFromFile( device.Get(), context.Get(), L"dx5_logo_autogen.dds", nullptr, &dxlogo )))
    {
        MessageBox(hwnd, L"Error loading dxlogo autogen.dds", L"LoadTest", MB_ICONERROR);
        return 1;
    }

    ComPtr<ID3D11ShaderResourceView> dxlogo2;
    if (FAILED(CreateDDSTextureFromFileEx( device.Get(), L"dx5_logo.dds", 0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, true, nullptr, &dxlogo2)))
    {
        MessageBox(hwnd, L"Error loading dxlogo.dds (2)", L"LoadTest", MB_ICONERROR);
        return 1;
    }

    ComPtr<ID3D11ShaderResourceView> win95;
    if (FAILED(CreateWICTextureFromFile(device.Get(), context.Get(), L"win95.bmp", nullptr, &win95)))
    {
        MessageBox(hwnd, L"Error loading win95.bmp", L"LoadTest", MB_ICONERROR);
        return 1;
    }

    ComPtr<ID3D11ShaderResourceView> win95_2;
    if (FAILED(CreateWICTextureFromFileEx(device.Get(), context.Get(), L"win95.bmp", 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true, nullptr, &win95_2)))
    {
        MessageBox(hwnd, L"Error loading win95.bmp (2)", L"LoadTest", MB_ICONERROR);
        return 1;
    }

    CommonStates states(device.Get());

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

        static float t = 0.0f;
        static uint64_t timeStart = 0;
        uint64_t timeCur = GetTickCount64();
        if( timeStart == 0 )
            timeStart = timeCur;
        t = ( timeCur - timeStart ) / 1000.0f;

        context->ClearRenderTargetView(backBuffer.Get(), Colors::CornflowerBlue);

        float dist = 10.f;

        // Cube 1
        world = XMMatrixRotationY( t ) * XMMatrixTranslation( 1.5f, -2.1f , (dist/2.f) + dist * sin(t) );

        CBChangesEveryFrame cb;
        cb.mWorld = XMMatrixTranspose( world );
        XMStoreFloat4( &cb.vEyePosition, eyePosition );
        XMStoreFloat4( &cb.vMeshColor, Colors::White );
        context->UpdateSubresource( cbChangesEveryFrame.Get(), 0, nullptr, &cb, 0, 0 );

        context->VSSetConstantBuffers( 0, 1, cbNeverChanges.GetAddressOf() );
        context->VSSetConstantBuffers( 1, 1, cbChangeOnResize.GetAddressOf() );
        context->VSSetConstantBuffers( 2, 1, cbChangesEveryFrame.GetAddressOf() );
        context->PSSetConstantBuffers( 2, 1, cbChangesEveryFrame.GetAddressOf() );

        ID3D11SamplerState* samplerState = states.LinearClamp();
        context->PSSetSamplers( 0, 1, &samplerState );

        context->PSSetShaderResources( 0, 1, earth.GetAddressOf() );
        context->DrawIndexed( 36, 0, 0 );

        // Cube 2
        world = XMMatrixRotationY( -t ) * XMMatrixTranslation( 1.5f, 0, (dist/2.f) + dist * sin(t) );

        cb.mWorld = XMMatrixTranspose( world );
        context->UpdateSubresource( cbChangesEveryFrame.Get(), 0, nullptr, &cb, 0, 0 );

        context->PSSetShaderResources( 0, 1, win95_2.GetAddressOf() );
        context->DrawIndexed( 36, 0, 0 );

        // Cube 3
        world = XMMatrixRotationY( t ) * XMMatrixTranslation( 1.5f, 2.1f, (dist/2.f) + dist * sin(t) );

        cb.mWorld = XMMatrixTranspose( world );
        context->UpdateSubresource( cbChangesEveryFrame.Get(), 0, nullptr, &cb, 0, 0 );

        context->PSSetShaderResources( 0, 1, win95.GetAddressOf() );
        context->DrawIndexed( 36, 0, 0 );

        // Cube 4
        world = XMMatrixRotationY( -t ) * XMMatrixTranslation( -1.5f, -2.1f, (dist/2.f) + dist * sin(t) );

        cb.mWorld = XMMatrixTranspose( world );
        context->UpdateSubresource( cbChangesEveryFrame.Get(), 0, nullptr, &cb, 0, 0 );

        context->PSSetShaderResources( 0, 1, earth2.GetAddressOf() );
        context->DrawIndexed( 36, 0, 0 );

        // Cube 5
        world = XMMatrixRotationY( t ) * XMMatrixTranslation( -1.5f, 0, (dist/2.f) + dist * sin(t) );

        cb.mWorld = XMMatrixTranspose( world );
        context->UpdateSubresource( cbChangesEveryFrame.Get(), 0, nullptr, &cb, 0, 0 );

        context->PSSetShaderResources( 0, 1, dxlogo2.GetAddressOf() );
        context->DrawIndexed( 36, 0, 0 );

        // Cube 6
        world = XMMatrixRotationY( -t ) * XMMatrixTranslation( -1.5f, 2.1f, (dist/2.f) + dist * sin(t) );

        cb.mWorld = XMMatrixTranspose( world );
        context->UpdateSubresource( cbChangesEveryFrame.Get(), 0, nullptr, &cb, 0, 0 );

        context->PSSetShaderResources( 0, 1, dxlogo.GetAddressOf() );
        context->DrawIndexed( 36, 0, 0 );

        swapChain->Present(1, 0);
        ++frame;

        if ( frame == 10 )
        {
            ComPtr<ID3D11Texture2D> backBufferTex;
            hr = swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&backBufferTex);
            if ( SUCCEEDED(hr) )
            {
                hr = SaveWICTextureToFile( context.Get(), backBufferTex.Get(), GUID_ContainerFormatPng, L"SCREENSHOT.PNG");
                hr = SaveWICTextureToFile( context.Get(), backBufferTex.Get(), GUID_ContainerFormatJpeg, L"SCREENSHOT.JPG" );
                hr = SaveWICTextureToFile( context.Get(), backBufferTex.Get(), GUID_ContainerFormatBmp, L"SCREENSHOT.BMP", &GUID_WICPixelFormat16bppBGR565 );
                hr = SaveWICTextureToFile( context.Get(), backBufferTex.Get(), GUID_ContainerFormatTiff, L"SCREENSHOT.TIF", nullptr,
                                            [&](IPropertyBag2* props)
                                            {
                                                PROPBAG2 options[2] = { 0, 0 };
                                                options[0].pstrName = L"CompressionQuality";
                                                options[1].pstrName = L"TiffCompressionMethod";

                                                VARIANT varValues[2];
                                                varValues[0].vt = VT_R4;
                                                varValues[0].fltVal = 0.75f;

                                                varValues[1].vt = VT_UI1;
                                                varValues[1].bVal = WICTiffCompressionNone;

                                                (void)props->Write( 2, options, varValues ); 
                                            });

                hr = SaveDDSTextureToFile( context.Get(), backBufferTex.Get(), L"SCREENSHOT.DDS" );
            }
        }
    }

    return 0;
}
