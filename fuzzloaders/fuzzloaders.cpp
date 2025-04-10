//--------------------------------------------------------------------------------------
// File: fuzzloaders.cpp
//
// Simple command-line tool for fuzz-testing the texture and sound loaders.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <Windows.h>

#include <d3d11_1.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <iterator>
#include <list>
#include <memory>
#include <tuple>
#include <vector>

#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "WaveBankReader.h"
#include "WAVFileReader.h"

#include <wrl\client.h>

#define TOOL_VERSION 0
#include "CmdLineHelpers.h"

using namespace Helpers;
using Microsoft::WRL::ComPtr;


namespace
{
    const wchar_t* g_ToolName = L"fuzzloaders";
    const wchar_t* g_Description = L"Microsoft (R) DirectX Tool Kit for DX11 File Fuzzing Harness";

#ifndef FUZZING_BUILD_MODE

    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    enum OPTIONS : uint32_t
    {
        OPT_RECURSIVE = 1,
        OPT_DDS,
        OPT_WAV,
        OPT_WIC,
        OPT_XWB,
        OPT_MAX
    };

    static_assert(OPT_MAX <= 32, "dwOptions is a unsigned int bitfield");

    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    const SValue<uint32_t> g_pOptions [] =
    {
        { L"r",         OPT_RECURSIVE },
        { L"dds",       OPT_DDS },
        { L"wav",       OPT_WAV },
        { L"wic",       OPT_WIC },
        { L"xwb",       OPT_XWB },
        { nullptr,      0 }
    };

    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    void PrintUsage()
    {
        PrintLogo(false, g_ToolName, g_Description);

        static const wchar_t* const s_usage =
            L"Usage: fuzzloaders <options> <files>\n"
            L"\n"
            L"   -r                  wildcard filename search is recursive\n"
            L"   -dds                force use of DDSTextureLoader\n"
            L"   -wav                force use of WAVFileReader\n"
            L"   -wic                force use of WICTextureLoader\n"
            L"   -xwb                force use of WaveBankReader\n";

        wprintf(L"%ls", s_usage);
    }

#endif // !FUZZING_BUILD_MODE

    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    HRESULT CreateDevice(ID3D11Device** pDev, ID3D11DeviceContext** pContext)
    {
        D3D_DRIVER_TYPE driverTypes [] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_NULL,
        };
        constexpr UINT numDriverTypes = static_cast<UINT>(std::size(driverTypes));

        HRESULT hr = E_FAIL;

        for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
        {
            hr = D3D11CreateDevice(nullptr,
                driverTypes[driverTypeIndex],
                nullptr,
                0 /*D3D11_CREATE_DEVICE_DEBUG*/,
                nullptr,
                0,
                D3D11_SDK_VERSION,
                pDev,
                nullptr,
                pContext);
            if (SUCCEEDED(hr))
                break;
        }

        return hr;
    }
}

//--------------------------------------------------------------------------------------
// Entry-point
//--------------------------------------------------------------------------------------
#ifndef FUZZING_BUILD_MODE

#ifdef _PREFAST_
#pragma prefast(disable : 28198, "Command-line tool, frees all memory on exit")
#endif

int __cdecl wmain(_In_ int argc, _In_z_count_(argc) wchar_t* argv[])
{
    // Initialize COM (needed for WIC)
    HRESULT hr = hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        wprintf(L"Failed to initialize COM (%08X)\n", static_cast<unsigned int>(hr));
        return 1;
    }

    // Process command line
    uint32_t dwOptions = 0;
    std::list<SConversion> conversion;

    for (int iArg = 1; iArg < argc; iArg++)
    {
        PWSTR pArg = argv[iArg];

        if (('-' == pArg[0]) || ('/' == pArg[0]))
        {
            pArg++;
            PWSTR pValue;

            for (pValue = pArg; *pValue && (':' != *pValue); pValue++);

            if (*pValue)
                *pValue++ = 0;

            uint32_t dwOption = LookupByName(pArg, g_pOptions);

            if (!dwOption || (dwOptions & (1 << dwOption)))
            {
                PrintUsage();
                return 1;
            }

            dwOptions |= 1 << dwOption;

            switch (dwOption)
            {
            case OPT_DDS:
            case OPT_WAV:
            case OPT_WIC:
            case OPT_XWB:
                {
                    uint32_t mask = (1 << OPT_DDS)
                        | (1 << OPT_WAV)
                        | (1 << OPT_WIC)
                        | (1 << OPT_XWB);
                    mask &= ~(1 << dwOption);
                    if (dwOptions & mask)
                    {
                        wprintf(L"-dds, -wav, -wic, and -xwb are mutually exclusive options\n");
                        return 1;
                    }
                }
                break;

            default:
                break;
            }
        }
        else if (wcspbrk(pArg, L"?*") != nullptr)
        {
            size_t count = conversion.size();
            SearchForFiles(pArg, conversion, (dwOptions & (1 << OPT_RECURSIVE)) != 0, nullptr);
            if (conversion.size() <= count)
            {
                wprintf(L"No matching files found for %ls\n", pArg);
                return 1;
            }
        }
        else
        {
            SConversion conv = {};
            conv.szSrc = pArg;

            conversion.push_back(conv);
        }
    }

    if (conversion.empty())
    {
        wprintf(L"ERROR: Need at least 1 image file to fuzz\n\n");
        PrintUsage();
        return 0;
    }

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    hr = CreateDevice(device.GetAddressOf(), context.GetAddressOf());
    if (FAILED(hr))
    {
        wprintf(L"ERROR: Failed to create required Direct3D device to fuzz: %08X\n", static_cast<unsigned int>(hr));
        return 1;
    }

    for (auto& pConv : conversion)
    {
        wchar_t ext[_MAX_EXT];
        _wsplitpath_s(pConv.szSrc.c_str(), nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);
        const bool isdds = (_wcsicmp(ext, L".dds") == 0);
        const bool iswav = (_wcsicmp(ext, L".wav") == 0);
        const bool isxwb = (_wcsicmp(ext, L".xwb") == 0);

        bool usedds = false;
        bool usewav = false;
        bool usexwb = false;
        bool usewic = false;
        if (dwOptions & (1 << OPT_DDS))
        {
            usedds = true;
        }
        else if (dwOptions & (1 << OPT_WAV))
        {
            usewav = true;
        }
        else if (dwOptions & (1 << OPT_XWB))
        {
            usexwb = true;
        }
        else if (dwOptions & (1 << OPT_WIC))
        {
            usewic = true;
        }
        else
        {
            usedds = true;
            usewav = true;
            usexwb = true;
            usewic = true;
        }

        // Load source image
#ifdef _DEBUG
        OutputDebugStringW(pConv.szSrc.c_str());
        OutputDebugStringA("\n");
#endif

        ComPtr<ID3D11Resource> tex;
        if (usedds)
        {
            hr = DirectX::CreateDDSTextureFromFileEx(device.Get(), pConv.szSrc.c_str(), 0,
                D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE, 0,
                DirectX::DDS_LOADER_DEFAULT, tex.GetAddressOf(), nullptr, nullptr);
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                wprintf(L"ERROR: DDSTexture file not not found:\n%ls\n", pConv.szSrc.c_str());
                return 1;
            }
            else if (FAILED(hr)
                && hr != E_INVALIDARG
                && hr != HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
                && hr != E_OUTOFMEMORY
                && hr != HRESULT_FROM_WIN32(ERROR_HANDLE_EOF)
                && hr != HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
                && (hr != E_FAIL || (hr == E_FAIL && isdds)))
            {
#ifdef _DEBUG
                char buff[128] = {};
                sprintf_s(buff, "DDSTexture failed with %08X\n", static_cast<unsigned int>(hr));
                OutputDebugStringA(buff);
#endif
                wprintf(L"!");
            }
            else
            {
                wprintf(L"%ls", SUCCEEDED(hr) ? L"*" : L".");
            }
        }

        if (usewav)
        {
            std::unique_ptr<uint8_t[]> data;
            DirectX::WAVData result = {};
            hr = DirectX::LoadWAVAudioFromFileEx(pConv.szSrc.c_str(), data, result);
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                wprintf(L"ERROR: WAVAudio file not not found:\n%ls\n", pConv.szSrc.c_str());
                return 1;
            }
            else if (FAILED(hr)
                && hr != E_INVALIDARG
                && hr != HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
                && hr != E_OUTOFMEMORY
                && hr != HRESULT_FROM_WIN32(ERROR_HANDLE_EOF)
                && hr != HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
                && (hr != E_FAIL || (hr == E_FAIL && iswav)))
            {
#ifdef _DEBUG
                char buff[128] = {};
                sprintf_s(buff, "WAVAudio failed with %08X\n", static_cast<unsigned int>(hr));
                OutputDebugStringA(buff);
#endif
                wprintf(L"!");
            }
            else
            {
                wprintf(L"%ls", SUCCEEDED(hr) ? L"*" : L".");
            }
        }

        if (usexwb)
        {
            auto wb = std::make_unique<DirectX::WaveBankReader>();
            hr = wb->Open(pConv.szSrc.c_str());
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                wprintf(L"ERROR: XWBAudio file not not found:\n%ls\n", pConv.szSrc.c_str());
                return 1;
            }
            else if (FAILED(hr)
                && hr != E_INVALIDARG
                && hr != HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
                && hr != E_OUTOFMEMORY
                && hr != HRESULT_FROM_WIN32(ERROR_HANDLE_EOF)
                && hr != HRESULT_FROM_WIN32(ERROR_INVALID_DATA)
                && (hr != E_FAIL || (hr == E_FAIL && isxwb)))
            {
#ifdef _DEBUG
                char buff[128] = {};
                sprintf_s(buff, "XWBAudio failed with %08X\n", static_cast<unsigned int>(hr));
                OutputDebugStringA(buff);
#endif
                wprintf(L"!");
            }
            else if (SUCCEEDED(hr))
            {
                wprintf(L"w");
                wb->WaitOnPrepare();
                wprintf(L"\b*");
            }
            else
            {
                wprintf(L".");
            }
        }

        if (usewic)
        {
            hr = DirectX::CreateWICTextureFromFileEx(device.Get(), pConv.szSrc.c_str(), 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE, 0, DirectX::WIC_LOADER_DEFAULT, tex.GetAddressOf(), nullptr);
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                wprintf(L"ERROR: WICTexture file not found:\n%ls\n", pConv.szSrc.c_str());
                return 1;
            }
            else if (FAILED(hr)
                && hr != E_INVALIDARG
                && hr != HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
                && hr != WINCODEC_ERR_COMPONENTNOTFOUND
                && hr != E_OUTOFMEMORY
                && hr != WINCODEC_ERR_BADHEADER)
            {
#ifdef _DEBUG
                char buff[128] = {};
                sprintf_s(buff, "WICTexture failed with %08X\n", static_cast<unsigned int>(hr));
                OutputDebugStringA(buff);
#endif
                wprintf(L"!");
            }
            else
            {
                wprintf(L"%ls", SUCCEEDED(hr) ? L"*" : L".");
            }
        }
        fflush(stdout);
    }

    wprintf(L"\n*** FUZZING COMPLETE ***\n");

    return 0;
}


#else // FUZZING_BUILD_MODE


//--------------------------------------------------------------------------------------
// Libfuzzer entry-point
//--------------------------------------------------------------------------------------
BOOL WINAPI InitializeDevice(PINIT_ONCE, PVOID, PVOID *idevice) noexcept
{
    ComPtr<ID3D11DeviceContext> context;
    HRESULT hr = CreateDevice(reinterpret_cast<ID3D11Device**>(idevice), context.GetAddressOf());
    if (SUCCEEDED(hr))
    {
        return TRUE;
    }

    return FALSE;
}


extern "C" __declspec(dllexport) int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static INIT_ONCE s_initOnce = INIT_ONCE_STATIC_INIT;

    ID3D11Device* device = nullptr;
    if (!InitOnceExecuteOnce(
        &s_initOnce,
        InitializeDevice,
        nullptr,
        reinterpret_cast<LPVOID*>(&device)))
    {
        return 0;
    }

    // Memory version
    {
        ComPtr<ID3D11Resource> tex;
        std::ignore = DirectX::CreateDDSTextureFromMemoryEx(device, data, size, 0,
                D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE, 0,
                DirectX::DDS_LOADER_DEFAULT, tex.GetAddressOf(), nullptr, nullptr);
    }

    // Disk version
    wchar_t tempFileName[MAX_PATH] = {};
    wchar_t tempPath[MAX_PATH] = {};

    if (!GetTempPathW(MAX_PATH, tempPath))
        return 0;

    if (!GetTempFileNameW(tempPath, L"fuzz", 0, tempFileName))
        return 0;

    {
        ScopedHandle hFile(safe_handle(CreateFileW(tempFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, nullptr)));
        if (!hFile)
            return 0;

        DWORD bytesWritten = 0;
        if (!WriteFile(hFile.get(), data, static_cast<DWORD>(size), &bytesWritten, nullptr))
            return 0;
    }

    {
        ComPtr<ID3D11Resource> tex;
        std::ignore = DirectX::CreateDDSTextureFromFileEx(device, tempFileName, 0,
                D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE, 0,
                DirectX::DDS_LOADER_DEFAULT, tex.GetAddressOf(), nullptr, nullptr);
    }

    {
        std::unique_ptr<uint8_t[]> wavData;
        DirectX::WAVData result = {};
        std::ignore = DirectX::LoadWAVAudioFromFileEx(tempFileName, wavData, result);
    }

    {
        auto wb = std::make_unique<DirectX::WaveBankReader>();
        std::ignore = wb->Open(tempFileName);
    }

    return 0;
}

#endif // FUZZING_BUILD_MODE
