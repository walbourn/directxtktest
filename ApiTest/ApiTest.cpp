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

    HRESULT CreateDevice(_Outptr_ ID3D11Device** pDev, _Outptr_ ID3D11DeviceContext** pContext)
    {
        if (pDev)
            *pDev = nullptr;
        if (pContext)
            *pContext = nullptr;

        if (!pDev)
            return E_INVALIDARG;

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

typedef _Success_(return) bool (*TestFN)(_In_ ID3D11Device *device);

struct TestInfo
{
    const char *name;
    TestFN func;
};

extern _Success_(return) bool Test00(_In_ ID3D11Device *device);
extern _Success_(return) bool Test01(_In_ ID3D11Device *device);
extern _Success_(return) bool Test02(_In_ ID3D11Device *device);
extern _Success_(return) bool Test03(_In_ ID3D11Device *device);
extern _Success_(return) bool Test04(_In_ ID3D11Device *device);
extern _Success_(return) bool Test05(_In_ ID3D11Device *device);
extern _Success_(return) bool Test06(_In_ ID3D11Device *device);
extern _Success_(return) bool Test07(_In_ ID3D11Device *device);
extern _Success_(return) bool Test08(_In_ ID3D11Device *device);
extern _Success_(return) bool Test09(_In_ ID3D11Device *device);
extern _Success_(return) bool Test10(_In_ ID3D11Device *device);
extern _Success_(return) bool Test11(_In_ ID3D11Device *device);
extern _Success_(return) bool Test12(_In_ ID3D11Device *device);
extern _Success_(return) bool Test13(_In_ ID3D11Device *device);
extern _Success_(return) bool Test14(_In_ ID3D11Device *device);
extern _Success_(return) bool Test15(_In_ ID3D11Device *device);
extern _Success_(return) bool Test16(_In_ ID3D11Device *device);
extern _Success_(return) bool Test17(_In_ ID3D11Device *device);
extern _Success_(return) bool Test18(_In_ ID3D11Device *device);
extern _Success_(return) bool Test19(_In_ ID3D11Device *device);

#ifdef TEST_AUDIO
extern _Success_(return) bool TestA01(_In_ ID3D11Device *device);
#endif

const TestInfo g_Tests[] =
{
    { "GraphicsMemory", Test00 },
    { "BasicEffects", Test05 },
    { "BufferHelpers", Test01 },
    { "CommonStates", Test02 },
    { "DirectXHelpers", Test03 },
    { "GeometricPrimitive", Test04 },
    { "PostProcess", Test06 },
    { "PrimitiveBatch", Test07 },
    { "SpriteBatch", Test08 },
    { "SpriteFont", Test09 },
    { "VertexTypes", Test10 },
    { "DGSLEffect", Test14 },
    { "NormalMapEffect", Test11 },
    { "PBREffect", Test12 },
    { "Model", Test13 },
    { "DGSLModel", Test15 },
    { "GamePad", Test16 },
    { "Keyboard", Test17 },
    { "Mouse", Test18 },
    { "LoaderHelpers (internal)", Test19 },

#ifdef TEST_AUDIO
    { "Audio", TestA01 },
#endif
};

//-------------------------------------------------------------------------------------
_Success_(return)
bool RunTests(_In_ ID3D11Device* device)
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

    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    if (FAILED(hr))
    {
        printf("ERROR: Unable to initialize COM (%08X).\n", static_cast<unsigned int>(hr));
        return -1;
    }

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    hr = CreateDevice(device.GetAddressOf(), context.GetAddressOf());
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
