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

#include <cassert>
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
extern bool Test03(_In_ ID3D11Device* pDevice);
extern bool Test04(_In_ ID3D11Device* pDevice);
extern bool Test05(_In_ ID3D11Device* pDevice);
extern bool Test06(_In_ ID3D11Device* pDevice);

TestInfo g_Tests[] =
{
    { "DDSTextureLoader (File)", Test01 },
    { "DDSTextureLoader (Memory)", Test02 },
    { "WICTextureLoader (File)", Test03 },
    { "WICTextureLoader (Memory)", Test04 },
    { "ScreenGrab (DDS)", Test05 },
    { "ScreenGrab (WIC)", Test06 },
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

    HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
    if (FAILED(hr))
    {
        printf("ERROR: Failed to initialize COM (%08X)\n", static_cast<unsigned int>(hr));
        return -1;
    }

    ComPtr<ID3D11Device> d3dDevice;
    hr = CreateDevice(d3dDevice.GetAddressOf());
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


//-------------------------------------------------------------------------------------
using Blob = std::unique_ptr<uint8_t[]>;

namespace
{
    struct handle_closer { void operator()(HANDLE h) noexcept { assert(h != INVALID_HANDLE_VALUE); if (h) CloseHandle(h); } };

    using ScopedHandle = std::unique_ptr<void, handle_closer>;

    inline HANDLE safe_handle(HANDLE h) noexcept { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }
}

HRESULT LoadBlobFromFile(_In_z_ const wchar_t* szFile, Blob& blob, size_t& blobSize)
{
    if (!szFile)
        return E_INVALIDARG;

    blob.reset();
    blobSize = 0;

    ScopedHandle hFile(safe_handle(CreateFile(
        szFile,
        GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN, nullptr)));
    if (!hFile)
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // Get the file size
    LARGE_INTEGER fileSize = {};
    if (!GetFileSizeEx( hFile.get(), &fileSize))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // File is too big for 32-bit allocation, so reject read (4 GB should be plenty large enough for our test images)
    if (fileSize.HighPart > 0)
    {
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);
    }

    // Need at least 1 byte of data
    if (!fileSize.LowPart)
    {
        return E_FAIL;
    }

    // Create blob memory
    blob = std::make_unique<uint8_t[]>(fileSize.LowPart);
    blobSize = fileSize.LowPart;

    // Load entire file into blob memory
    DWORD bytesRead = 0;
    if (!ReadFile(hFile.get(), blob.get(), static_cast<DWORD>(blobSize), &bytesRead, nullptr) )
    {
        blob.reset();
        blobSize = 0;
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // Verify we got the whole blob loaded
    if ( bytesRead != blobSize )
    {
        blob.reset();
        blobSize = 0;
        return E_FAIL;
    }

    return S_OK;
}
