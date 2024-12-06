//-------------------------------------------------------------------------------------
// xwb.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#define NODRAWTEXT
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <Windows.h>

#include "WaveBankReader.h"

#include <cstdio>
#include <stdexcept>
#include <tuple>

using namespace DirectX;

namespace
{
    struct TestMedia
    {
        bool streaming;
        uint32_t entries;
        uint32_t audioBytes;
        const wchar_t *fname;
        uint8_t md5[16];
    };

    const TestMedia g_TestMedia[] =
    {
        // Type | Entries | BankAudioBytes | Filename | MD5Hash
        { false, 14, 5628004, L"BasicAudioTest\\ADPCMdroid.xwb", {0xd9,0xc3,0xa6,0x2d,0xe0,0x08,0xd0,0x17,0x4b,0x26,0x01,0xb7,0x86,0xa4,0xa2,0xbd} },
        { false, 4, 3164688, L"BasicAudioTest\\compact.xwb", {0xce,0xed,0x17,0x05,0xad,0x3c,0xfb,0x43,0x4b,0x1b,0x2d,0x3e,0x8a,0x84,0xbb,0x9c} },
        { false, 14, 20582244, L"BasicAudioTest\\droid.xwb", {0xa4,0x53,0x90,0x3a,0xcc,0x7a,0x7a,0x8d,0xd7,0x8e,0x6d,0x7b,0xe7,0x49,0xb6,0x64} },
        { false, 4, 3164688, L"BasicAudioTest\\wavebank.xwb", {0xce,0xed,0x17,0x05,0xad,0x3c,0xfb,0x43,0x4b,0x1b,0x2d,0x3e,0x8a,0x84,0xbb,0x9c} },
        { false, 14, 820656, L"BasicAudioTest\\xwmadroid.xwb", {0x8a,0x43,0xb6,0xe4,0x59,0x22,0x70,0x11,0x0d,0x09,0x06,0x1c,0x4b,0x0f,0x97,0x16} },
        { true, 3, 9545728, L"StreamingAudioTest\\WaveBank.xwb", {0x52,0x48,0x91,0x66,0xa4,0xc0,0xce,0x8e,0x7f,0x05,0x9d,0xcb,0xfc,0x3b,0x1c,0x75} },
        { true, 3, 9547776, L"StreamingAudioTest\\WaveBank4Kn.xwb", {0x52,0x48,0x91,0x66,0xa4,0xc0,0xce,0x8e,0x7f,0x05,0x9d,0xcb,0xfc,0x3b,0x1c,0x75} },
        { true, 3, 2613248, L"StreamingAudioTest\\WaveBankADPCM.xwb", {0x38,0x10,0x45,0x4f,0x43,0xdc,0xfe,0xb3,0xaa,0xe2,0xdf,0x4e,0x38,0x77,0x48,0xbf} },
        { true, 3, 2617344, L"StreamingAudioTest\\WaveBankADPCM4Kn.xwb", {0x38,0x10,0x45,0x4f,0x43,0xdc,0xfe,0xb3,0xaa,0xe2,0xdf,0x4e,0x38,0x77,0x48,0xbf} },
        { true, 3, 1492992, L"StreamingAudioTest\\WaveBankXMA2.xwb", {0xe0,0xaa,0x31,0x9b,0x5a,0x83,0x90,0x99,0x96,0xbf,0x04,0x18,0xa4,0x98,0xea,0xfa} },
        { true, 3, 1495040, L"StreamingAudioTest\\WaveBankXMA2_4Kn.xwb", {0xe0,0xaa,0x31,0x9b,0x5a,0x83,0x90,0x99,0x96,0xbf,0x04,0x18,0xa4,0x98,0xea,0xfa} },
        { true, 3, 663552, L"StreamingAudioTest\\WaveBankxWMA.xwb", {0x75,0x3f,0x19,0xa5,0x28,0x07,0xd5,0xda,0xe9,0x83,0xa0,0xc2,0x18,0x11,0xaf,0xbd} },
        { true, 3, 667648, L"StreamingAudioTest\\WaveBankxWMA4Kn.xwb", {0x75,0x3f,0x19,0xa5,0x28,0x07,0xd5,0xda,0xe9,0x83,0xa0,0xc2,0x18,0x11,0xaf,0xbd} },
    };

#define printdigest(str,digest) printf( "%s:\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x\n", str, \
                                       digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7], \
                                       digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15] );

    template<typename T>
    inline T AlignUp(T size, size_t alignment) noexcept
    {
        if (alignment > 0)
        {
            auto mask = static_cast<T>(alignment - 1);
            return (size + mask) & ~mask;
        }
        return size;
    }
}

//-------------------------------------------------------------------------------------

extern HRESULT MD5Checksum( _In_reads_(dataSize) const uint8_t *data, size_t dataSize, _Out_bytecap_x_(16) uint8_t *digest );

//-------------------------------------------------------------------------------------
//
bool Test02()
{
    bool success = true;

    size_t ncount = 0;
    size_t npass = 0;

    for( size_t index=0; index < std::size(g_TestMedia); ++index )
    {
        wchar_t szPath[MAX_PATH] = {};
        DWORD ret = ExpandEnvironmentStringsW(g_TestMedia[index].fname, szPath, MAX_PATH);
        if ( !ret || ret > MAX_PATH )
        {
            printf( "ERROR: ExpandEnvironmentStrings FAILED\n" );
            return false;
        }

#ifdef _DEBUG
        OutputDebugString(szPath);
        OutputDebugStringA("\n");
#endif

        auto wb = std::make_unique<DirectX::WaveBankReader>();
        HRESULT hr = wb->Open(szPath);
        if ( FAILED(hr) )
        {
            success = false;
            printf( "Failed loading wavebank from file (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
        }
        else
        {
            wb->WaitOnPrepare();
            if (wb->Count() != g_TestMedia[index].entries
                || wb->IsStreamingBank() != g_TestMedia[index].streaming
                || wb->BankAudioSize() != g_TestMedia[index].audioBytes)
            {
                success = false;
                printf( "Metadata error in wavebank file:\n%ls\n%u entries   %u audioBytes\n", szPath, wb->Count(), wb->BankAudioSize() );
            }
            else if (wb->IsStreamingBank())
            {
                WaveBankReader::Metadata metadata;
                hr = wb->GetMetadata(0, metadata);
                if ( FAILED(hr) )
                {
                    success = false;
                    printf( "Failed get wave metadata for entry 0 (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
                }
                else if (!metadata.duration || !metadata.offsetBytes || !metadata.lengthBytes)
                {
                    success = false;
                    printf( "Metadata error in wavebank entry:\n%ls\n%u duration  %u offset  %u length\n", szPath,
                        metadata.duration, metadata.offsetBytes, metadata.lengthBytes);
                }
                else
                {
                    size_t memSize = AlignUp(metadata.lengthBytes, 4096);
                    auto wavData = std::make_unique<uint8_t[]>(memSize);

                    HANDLE async = wb->GetAsyncHandle();

                    OVERLAPPED request = {};
                    request.hEvent = CreateEventExW(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
                    if (!request.hEvent)
                    {
                        printf("Fatal error: CreateEventEx failed (HRESULT %08X)\n", static_cast<unsigned int>(HRESULT_FROM_WIN32(GetLastError())));
                        return false;
                    }
                    request.Offset = metadata.offsetBytes;

                    bool pass = true;
                    uint32_t readLength = AlignUp(metadata.lengthBytes, 4096);
                    if (!ReadFile(async, wavData.get(), readLength, nullptr, &request))
                    {
                        const DWORD error = GetLastError();
                        if (error != ERROR_IO_PENDING)
                        {
                            success = false;
                            pass = false;
                            printf( "ERROR: Async read failed %08X:\n%ls\n%u duration  %u offset  %u length\n",
                                static_cast<unsigned int>(HRESULT_FROM_WIN32(error)),
                                szPath,
                                metadata.duration, metadata.offsetBytes, metadata.lengthBytes);
                        }
                    }

                    if (pass)
                    {
                        std::ignore = WaitForSingleObject(request.hEvent, INFINITE);

                        DWORD cb = 0;
                        const BOOL result = GetOverlappedResultEx(async, &request, &cb, 0, FALSE);
                        if (!result)
                        {
                            success = false;
                            pass = false;
                            const DWORD error = GetLastError();
                            printf("ERROR: Async read failed %08X\n", static_cast<unsigned int>(HRESULT_FROM_WIN32(error)));
                        }
                    }

                    if (pass)
                    {
                        uint8_t digest[16];
                        hr = MD5Checksum( wavData.get(), metadata.lengthBytes, digest );
                        if ( FAILED(hr) )
                        {
                            success = false;
                            pass = false;
                            printf( "Failed computing MD5 checksum of wavebank (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
                        }
                        else if ( memcmp( digest, g_TestMedia[index].md5, 16 ) != 0 )
                        {
                            success = false;
                            pass = false;
                            printf( "Failed comparing MD5 checksum:\n%ls\n", szPath );
                            printdigest( "computed", digest );
                            printdigest( "expected", g_TestMedia[index].md5 );
                        }
                    }

                    CloseHandle(request.hEvent);

                    if (pass)
                        ++npass;
                }
            }
            else
            {
                const uint8_t* wavData = nullptr;
                uint32_t audioBytes = 0;
                hr = wb->GetWaveData(0, &wavData, audioBytes);
                if ( FAILED(hr) )
                {
                    success = false;
                    printf( "Failed get wave data for entry 0 (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
                }
                else
                {
                    uint8_t digest[16];
                    hr = MD5Checksum( wavData, audioBytes, digest );
                    if ( FAILED(hr) )
                    {
                        success = false;
                        printf( "Failed computing MD5 checksum of wavebank (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
                    }
                    else if ( memcmp( digest, g_TestMedia[index].md5, 16 ) != 0 )
                    {
                        success = false;
                        printf( "Failed comparing MD5 checksum:\n%ls\n", szPath );
                        printdigest( "computed", digest );
                        printdigest( "expected", g_TestMedia[index].md5 );
                    }
                    else
                        ++npass;
                }
            }
        }

        ++ncount;
    }

    printf("%zu files tested, %zu files passed ", ncount, npass );

    return success;
}
