//-------------------------------------------------------------------------------------
// wav.cpp
//  
// Copyright (c) Microsoft Corporation.
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

#include "WAVFileReader.h"

#include <cstdio>
#include <stdexcept>

#include "SoundCommon.h"

#ifndef WAVE_FORMAT_XMA2
#define WAVE_FORMAT_XMA2 0x166
#endif

using namespace DirectX;

namespace
{
    struct TestMedia
    {
        uint32_t tag;
        uint32_t channels;
        uint32_t bits;
        uint32_t rate;
        uint32_t seek;
        uint32_t loop;
        const wchar_t *fname;
        uint8_t md5[16];
    };

    const TestMedia g_TestMedia[] =
    {
        // FormatTag | Channels | BitDepth | SampleRate | SeekCount | LoopLength | Filename | MD5Hash
        { WAVE_FORMAT_PCM, 1, 16, 44100, 0, 0, L"Audio3DTest\\heli.wav", {0xd6,0x08,0x6b,0xc3,0x82,0xcc,0x6f,0x83,0x35,0x09,0xf2,0xfe,0xe7,0x75,0x81,0xca} },
        { WAVE_FORMAT_PCM, 2, 16, 22050, 0, 0, L"BasicAudioTest\\Alarm01.wav", {0x3b,0xd7,0x66,0xb5,0x2b,0x02,0xef,0xff,0x5e,0xba,0xd4,0x77,0x5c,0x42,0x35,0x63} },
        { WAVE_FORMAT_ADPCM, 2, 4, 22052, 0, 0, L"BasicAudioTest\\Alarm01_adpcm.wav", {0xa9,0xff,0xc2,0x61,0x99,0xea,0x59,0x5c,0x50,0x54,0x6e,0x12,0x53,0xa3,0xf0,0x67} },
        { WAVE_FORMAT_IEEE_FLOAT, 2, 32, 22050, 0, 0, L"BasicAudioTest\\Alarm01_float.wav", {0xb1,0x22,0xcc,0xed,0x38,0xe5,0x74,0xa6,0x47,0x3a,0x80,0x8b,0x0e,0x68,0x4c,0xe8} },
        { WAVE_FORMAT_WMAUDIO2, 2, 16, 32000, 15, 0, L"BasicAudioTest\\Alarm01_xwma.wav", {0x45,0x3c,0x89,0x3c,0xa0,0x8e,0xcb,0xe5,0xac,0x00,0xe2,0xf0,0xb5,0x76,0x45,0x56} },
        { WAVE_FORMAT_ADPCM, 1, 4, 44099, 0, 1881344, L"BasicAudioTest\\electro_adpcm.wav", {0xb4,0x61,0x54,0xe8,0xf8,0x7f,0x63,0x87,0x7d,0x79,0xf3,0xb9,0x94,0xc7,0x33,0xf0} },
        { WAVE_FORMAT_IEEE_FLOAT, 1, 32, 44100, 0, 0, L"BasicAudioTest\\HipHoppy_float.wav", {0x1b,0xd1,0x46,0xf3,0xc7,0xc9,0x77,0x9f,0x4a,0x66,0x12,0x22,0xd9,0xb5,0x7b,0x09} },
        { WAVE_FORMAT_PCM, 1, 16, 44100, 0, 0, L"BasicAudioTest\\MusicMono.wav", {0x25,0xaf,0xc3,0x82,0x49,0x1c,0x21,0x36,0x23,0x56,0x3a,0xe7,0x45,0xae,0xda,0x43} },
        { WAVE_FORMAT_WMAUDIO2, 1, 16, 44100, 26, 0, L"BasicAudioTest\\musicmono_xwma.wav", {0x3c,0xfc,0x60,0xf2,0x02,0x8a,0x89,0xe3,0xce,0x08,0xb4,0x62,0xaf,0xab,0xe2,0xb3} },
        { WAVE_FORMAT_PCM, 6, 16, 44100, 0, 0, L"BasicAudioTest\\MusicSurround.wav", {0x9b,0xb7,0xbf,0x9d,0x6d,0xe5,0xda,0xef,0x6e,0x02,0x1a,0x28,0xe4,0xdd,0x54,0x42} },
        { WAVE_FORMAT_XMA2, 2, 16, 22050, 1, 0, L"SimpleAudioTest\\Alarm01_xma.wav", {0x49,0x4e,0x2a,0x0c,0x33,0xa8,0x85,0x16,0x42,0xc7,0x2c,0x45,0x11,0x96,0x47,0x4f} },
        { WAVE_FORMAT_PCM, 2, 8, 48000, 0, 0, L"SimpleAudioTest\\tada.wav", {0x36,0x00,0xd8,0x82,0xa7,0x51,0x75,0xd6,0x44,0x61,0x14,0x6a,0x99,0x71,0xee,0xe0} },
    };

#define printdigest(str,digest) printf( "%s:\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x\n", str, \
                                       digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7], \
                                       digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15] );

    const char* GetFormatTagName(WORD wFormatTag)
    {
        switch (wFormatTag)
        {
        case WAVE_FORMAT_PCM: return "PCM";
        case WAVE_FORMAT_ADPCM: return "MS ADPCM";
        case WAVE_FORMAT_EXTENSIBLE: return "EXTENSIBLE";
        case WAVE_FORMAT_IEEE_FLOAT: return "IEEE float";
        case WAVE_FORMAT_MPEGLAYER3: return "ISO/MPEG Layer3";
        case WAVE_FORMAT_DOLBY_AC3_SPDIF: return "Dolby Audio Codec 3 over S/PDIF";
        case WAVE_FORMAT_WMAUDIO2: return "Windows Media Audio";
        case WAVE_FORMAT_WMAUDIO3: return "Windows Media Audio Pro";
        case WAVE_FORMAT_WMASPDIF: return "Windows Media Audio over S/PDIF";
        case 0x165: /*WAVE_FORMAT_XMA*/ return "Xbox XMA";
        case 0x166: /*WAVE_FORMAT_XMA2*/ return "Xbox XMA2";
        default: return "*UNKNOWN*";
        }
    }

    const char *ChannelDesc(DWORD dwChannelMask)
    {
        switch (dwChannelMask)
        {
        case 0x00000004 /*SPEAKER_MONO*/: return "Mono";
        case 0x00000003 /* SPEAKER_STEREO */: return "Stereo";
        case 0x0000000B /* SPEAKER_2POINT1 */: return "2.1";
        case 0x00000107 /* SPEAKER_SURROUND */: return "Surround";
        case 0x00000033 /* SPEAKER_QUAD */: return "Quad";
        case 0x0000003B /* SPEAKER_4POINT1 */: return "4.1";
        case 0x0000003F /* SPEAKER_5POINT1 */: return "5.1";
        case 0x000000FF /* SPEAKER_7POINT1 */: return "7.1";
        case 0x0000060F /* SPEAKER_5POINT1_SURROUND */: return "Surround5.1";
        case 0x0000063F /* SPEAKER_7POINT1_SURROUND */: return "Surround7.1";
        default: return "Custom";
        }
    }

    void printwaveex(_In_ const WAVEFORMATEX* wfx)
    {
        if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE
            && (wfx->cbSize >= (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))))
        {
            auto wext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(&wfx);

            wprintf(L" (%hs %u channels, %u-bit, %lu Hz, CMask:%hs)", GetFormatTagName(wfx->wFormatTag), wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, ChannelDesc(wext->dwChannelMask));
        }
        else
        {
            wprintf(L" (%hs %u channels, %u-bit, %lu Hz)", GetFormatTagName(wfx->wFormatTag), wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec);
        }
    }
}

//-------------------------------------------------------------------------------------

extern HRESULT MD5Checksum( _In_reads_(dataSize) const uint8_t *data, size_t dataSize, _Out_bytecap_x_(16) uint8_t *digest );

//-------------------------------------------------------------------------------------
// 
bool Test01()
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

        std::unique_ptr<uint8_t[]> wavData;
        WAVData result = {};
        HRESULT hr = LoadWAVAudioFromFileEx(szPath, wavData, result);
        if ( FAILED(hr) )
        {
            success = false;
            printf( "Failed loading wav from file (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
        }
        else if (!result.wfx || !result.startAudio)
        {
            success = false;
            printf( "Bad metadata read from (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
        }
        else if (GetFormatTag(result.wfx) != g_TestMedia[index].tag
                 || result.wfx->nChannels != g_TestMedia[index].channels
                 || result.wfx->wBitsPerSample != g_TestMedia[index].bits
                 || result.wfx->nSamplesPerSec != g_TestMedia[index].rate
                 || result.seekCount != g_TestMedia[index].seek
                 || result.loopLength != g_TestMedia[index].loop)
        {
            success = false;
            printf( "Metadata error in wav file:\n%ls\n", szPath );
            printwaveex(result.wfx);
            if (result.seekCount > 0)
            {
                printf("\nSeekCount = %u\n", result.seekCount);
            }
            if (result.loopLength > 0)
            {
                printf("\nLoop = %u..%u\n", result.loopStart, result.loopLength);
            }
            printf("\n");
        }
        else
        {
            uint8_t digest[16];
            hr = MD5Checksum( wavData.get(), result.audioBytes, digest );
            if ( FAILED(hr) )
            {
                success = false;
                printf( "Failed computing MD5 checksum of wave data (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
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

        ++ncount;
    }

    printf("%zu files tested, %zu files passed ", ncount, npass );

    return success;
}
