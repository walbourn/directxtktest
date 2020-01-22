//--------------------------------------------------------------------------------------
// File: StreamingAudioTest.cpp
//
// Developer unit test for DirectXTK for Audio
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

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Audio.h"

#include <stdio.h>

using namespace DirectX;

//--------------------------------------------------------------------------------------
#define UPDATE \
    if (!audEngine->Update()) \
    { \
        if (audEngine->IsCriticalError()) \
        { \
            printf("ERROR: Update failed with critical error\n"); \
        } \
        else \
        { \
            printf("ERROR: Unexpected result from audEngine::Update\n"); \
        } \
        return 1; \
    }

namespace
{
    //----------------------------------------------------------------------------------
    const char* GetFormatTagName(DWORD tag)
    {
        switch (tag)
        {
        case WAVE_FORMAT_PCM: return "PCMi";
        case WAVE_FORMAT_IEEE_FLOAT: return "PCMf";
        case WAVE_FORMAT_ADPCM: return "ADPCM";
        case WAVE_FORMAT_WMAUDIO2: return "WMAUDIO2";
        case WAVE_FORMAT_WMAUDIO3: return "WMAUDIO3";
        case 0x166 /* WAVE_FORMAT_XMA2 */: return "XMA2";
        default: return "*Unknown*"; break;
        }
    }

    void dump_wfx(const WAVEFORMATEX* wfx)
    {
        if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            if (wfx->cbSize < (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
            {
                printf("\tEXTENSIBLE, %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                    wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                printf("\tERROR: Invalid WAVE_FORMAT_EXTENSIBLE\n");
            }
            else
            {
                static const GUID s_wfexBase = { 0x00000000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 };

                auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);

                if (memcmp(reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                    reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD)) != 0)
                {
                    printf("\tEXTENSIBLE, %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                        wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                    printf("\tERROR: Unknown EXTENSIBLE SubFormat {%8.8lX-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}\n",
                        wfex->SubFormat.Data1, wfex->SubFormat.Data2, wfex->SubFormat.Data3,
                        wfex->SubFormat.Data4[0], wfex->SubFormat.Data4[1], wfex->SubFormat.Data4[2], wfex->SubFormat.Data4[3],
                        wfex->SubFormat.Data4[4], wfex->SubFormat.Data4[5], wfex->SubFormat.Data4[6], wfex->SubFormat.Data4[7]);
                }
                else
                {
                    printf("\tEXTENSIBLE %s (%u), %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                        GetFormatTagName(wfex->SubFormat.Data1), wfex->SubFormat.Data1,
                        wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                    printf("\t\t%u samples per block, %u valid bps, %u channel mask",
                        wfex->Samples.wSamplesPerBlock, wfex->Samples.wValidBitsPerSample, wfex->dwChannelMask);
                }
            }
        }
        else
        {
            printf("\t%s (%u), %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                GetFormatTagName(wfx->wFormatTag), wfx->wFormatTag,
                wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
        }
    }
}


//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
int __cdecl main()
{
    if (!XMVerifyCPUSupport())
    {
        printf("ERROR: CPU failed DirectXMath init\n");
        return 1;
    }

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    //
    // Initialize XAudio2
    //
    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
    {
        printf("ERROR: COM init failed\n");
        return 1;
    }

    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    auto audEngine = std::make_unique<AudioEngine>(eflags);

    {
        auto output = audEngine->GetOutputFormat();

        if (output.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE
            || output.Format.cbSize != (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
        {
            printf("ERROR: GetOutputFormat failed\n");
            dump_wfx(reinterpret_cast<const WAVEFORMATEX*>(&output));
            return 1;
        }

        const char* speakerConfig;
        switch (output.dwChannelMask)
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

        printf("\tOutput format rate %u, channels %u, %s (%08X)\n", output.Format.nSamplesPerSec, output.Format.nChannels, speakerConfig, output.dwChannelMask);
    }

    if (!audEngine->IsAudioDevicePresent())
    {
        printf("ERROR: IsAudioDevicePresent returned unexpected value\n");
        return 1;
    }

    { // PCM WaveBank
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"WaveBank.xwb");
        printf("\n\nINFO: Loaded wavebank.xwb\n");

        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);

        for (uint32_t j = 0; j < 3; ++j)
        {
            printf("\tIndex #%zu (%zu bytes, %zu samples, %zu ms)\n",
                j, wb->GetSampleSizeInBytes(j), wb->GetSampleDuration(j), wb->GetSampleDurationMS(j));
            dump_wfx(wb->GetFormat(j, wfx, 64));
        }


        auto stream1 = wb->CreateStreamInstance(0u);

        // TODO -
    }

    printf("\tPASS\n");
}
