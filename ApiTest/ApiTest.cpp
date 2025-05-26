//--------------------------------------------------------------------------------------
// File: ApiTest.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include <crtdbg.h>

#include <cstdio>
#include <exception>
#include <iterator>

#include "DirectXMath.h"

#include <d3d11_1.h>

#include <wrl/client.h>

//#define D3D_DEBUG

using Microsoft::WRL::ComPtr;

namespace
{
    //---------------------------------------------------------------------------------
    const D3D_DRIVER_TYPE g_driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
    };

    const D3D_FEATURE_LEVEL g_featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT CreateDevice(ID3D11Device** pDev, ID3D11DeviceContext** pContext)
    {
        HRESULT hr = E_FAIL;

        UINT createDeviceFlags = 0;
    #ifdef D3D_DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

        for (UINT driverTypeIndex = 0; driverTypeIndex < std::size(g_driverTypes); driverTypeIndex++)
        {
            hr = D3D11CreateDevice(nullptr, g_driverTypes[driverTypeIndex], nullptr, createDeviceFlags,
                g_featureLevels, static_cast<UINT>(std::size(g_featureLevels)),
                D3D11_SDK_VERSION, pDev, nullptr, pContext);
            if (SUCCEEDED(hr))
                break;
        }

        return hr;
    }
}

//-------------------------------------------------------------------------------------
// Types and globals

typedef bool (*TestFN)(ID3D11Device *device);

struct TestInfo
{
    const char *name;
    TestFN func;
};

extern bool Test01(ID3D11Device *device);
extern bool Test02(ID3D11Device *device);
extern bool Test03(ID3D11Device *device);
extern bool Test04(ID3D11Device *device);
extern bool Test05(ID3D11Device *device);
extern bool Test06(ID3D11Device *device);
extern bool Test07(ID3D11Device *device);
extern bool Test08(ID3D11Device *device);
extern bool Test09(ID3D11Device *device);

TestInfo g_Tests[] =
{
    { "BufferHelpers", Test01 },
    { "CommonStates", Test02 },
    { "DirectXHelpers", Test03 },
    { "GeometricPrimitive", Test04 },
    { "GraphicsMemory", Test05 },
    { "PrimitiveBatch", Test06 },
    { "SpriteBatch", Test07 },
    { "SpriteFont", Test08 },
    { "VertexTypes", Test09 },
};

//-------------------------------------------------------------------------------------
bool RunTests(ID3D11Device* device)
{
    if (!device)
        return false;

    size_t nPass = 0;
    size_t nFail = 0;

    for(size_t i=0; i < std::size(g_Tests); ++i)
    {
        printf("%s: ", g_Tests[i].name );

        bool passed = false;

        try
        {
            passed = g_Tests[i].func(device);
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed with a standard C++ exception: %s\n", e.what());
        }
        catch(...)
        {
            printf("ERROR: Failed with an unknown C++ exception\n");
        }

        if (passed)
        {
            ++nPass;
            printf("PASS\n");
        }
        else
        {
            ++nFail;
            printf("FAIL\n");
        }
    }

    printf("Ran %zu tests, %zu pass, %zu fail\n", nPass+nFail, nPass, nFail);

    return (nFail == 0);
}


//-------------------------------------------------------------------------------------
int __cdecl wmain()
{
    printf("**************************************************************\n");
    printf("*** DirectX tool Kit for DX 11 API Test\n" );
    printf("**************************************************************\n");

    if ( !DirectX::XMVerifyCPUSupport() )
    {
        printf("ERROR: XMVerifyCPUSupport fails on this system, not a supported platform\n");
        return -1;
    }

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    HRESULT hr = CreateDevice(device.GetAddressOf(), context.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Unable to create a Direct3D 11 device (%08X).\n", static_cast<unsigned int>(hr));
        return -1;
    }

#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    if ( !RunTests(device.Get()) )
        return -1;

    return 0;
}
