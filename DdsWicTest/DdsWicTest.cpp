//-------------------------------------------------------------------------------------
// DdsWicTest.cpp
//
// Copyright (c) Microsoft Corporation.
//-------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <Windows.h>

#include <d3d11_1.h>
#include <wrl/client.h>

#include <cstdint>
#include <cstdio>
#include <iterator>
#include <memory>

//-------------------------------------------------------------------------------------
// Types and globals

using TestFN = bool (*)(ID3D11Device* pDevice);

struct TestInfo
{
    const char *name;
    TestFN func;
};

extern bool Test01(_In_ ID3D11Device* pDevice);
extern bool Test02(_In_ ID3D11Device* pDevice);

TestInfo g_Tests[] =
{
    { "DDSTextureLoader", Test01 },
//    { "WICTextureLoader", Test02 },
};

using Microsoft::WRL::ComPtr;

namespace
{
    HRESULT CreateDevice(_Outptr_ ID3D11Device** pDevice)
    {
        if (!pDevice)
            return E_INVALIDARG;

        *pDevice = nullptr;

        const D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
        };

        UINT createDeviceFlags = 0;
    #ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

        D3D_FEATURE_LEVEL fl;
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_NULL,
            nullptr, createDeviceFlags, featureLevels, static_cast<UINT>(std::size(featureLevels)),
            D3D11_SDK_VERSION, pDevice, &fl, nullptr);

        return hr;
    }
}

//-------------------------------------------------------------------------------------
bool RunTests(_In_ ID3D11Device* pDevice)
{
    size_t nPass = 0;
    size_t nFail = 0;

    for(size_t i=0; i < std::size(g_Tests); ++i)
    {
        printf("%s: ", g_Tests[i].name );

        if ( g_Tests[i].func(pDevice) )
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
    printf("*** DdsWicTest\n" );
    printf("**************************************************************\n");

    ComPtr<ID3D11Device> d3dDevice;
    HRESULT hr = CreateDevice(d3dDevice.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed to create required Direct3D device (%08X)\n", static_cast<unsigned int>(hr));
        return -1;
    }

    if ( !RunTests(d3dDevice.Get()) )
        return -1;

    return 0;
}


//-------------------------------------------------------------------------------------
#include <bcrypt.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status)          ((Status) >= 0)
#endif

struct bcrypthandle_closer { void operator()(BCRYPT_HASH_HANDLE h) { BCryptDestroyHash(h); } };

using ScopedHashHandle = std::unique_ptr<void, bcrypthandle_closer>;

#define MD5_DIGEST_LENGTH 16

HRESULT MD5Checksum( _In_reads_(dataSize) const uint8_t *data, size_t dataSize, _Out_bytecap_x_(16) uint8_t *digest )
{
    if ( !data || !dataSize || !digest )
        return E_INVALIDARG;

    memset( digest, 0, MD5_DIGEST_LENGTH );

    NTSTATUS status;

    // Ensure have the MD5 algorithm ready
    static BCRYPT_ALG_HANDLE s_algid = nullptr;
    if ( !s_algid )
    {
        status = BCryptOpenAlgorithmProvider( &s_algid, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER,  0 );
        if ( !NT_SUCCESS(status) )
            return HRESULT_FROM_NT(status);

        DWORD len = 0, res = 0;
        status = BCryptGetProperty( s_algid, BCRYPT_HASH_LENGTH, (PBYTE)&len, sizeof(DWORD), &res, 0 );
        if ( !NT_SUCCESS(status) || res != sizeof(DWORD) || len != MD5_DIGEST_LENGTH )
        {
            return E_FAIL;
        }
    }

    // Create hash object
    BCRYPT_HASH_HANDLE hobj;
    status = BCryptCreateHash( s_algid, &hobj, nullptr, 0, nullptr, 0, 0 );
    if ( !NT_SUCCESS(status) )
        return HRESULT_FROM_NT(status);

    ScopedHashHandle hash( hobj );

    status = BCryptHashData( hash.get(), (PBYTE)data, (ULONG)dataSize, 0 );
    if ( !NT_SUCCESS(status) )
        return HRESULT_FROM_NT(status);

    status = BCryptFinishHash( hash.get(), (PBYTE)digest, MD5_DIGEST_LENGTH, 0 );
    if ( !NT_SUCCESS(status) )
        return HRESULT_FROM_NT(status);

#ifdef _DEBUG
    char buff[1024] = ", { ";
    char tmp[16];

    for( size_t i=0; i < MD5_DIGEST_LENGTH; ++i )
    {
        sprintf_s( tmp, "0x%02x%s", digest[i], (i < (MD5_DIGEST_LENGTH-1)) ? "," : " } " );
        strcat_s( buff, tmp );
    }

    OutputDebugStringA( buff );
    OutputDebugStringA("\n");
#endif

    return S_OK;
}
