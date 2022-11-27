//--------------------------------------------------------------------------------------
// File: StreamingAudioTest.cpp
//
// Developer unit test for DirectXTK for Audio
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
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Audio.h"

#include <stdio.h>

// Test Advanced Format (4Kn) streaming wave banks vs. DVD (2048) sector aligned
#define TEST_4KN

#define TEST_PCM
#define TEST_ADPCM

#ifdef USING_XAUDIO2_9
#define TEST_XWMA
#endif

// C4619/4616 #pragma warning warnings
// C26451 Arithmetic overflow
#pragma warning(disable : 4619 4616 26451)

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
                printf("\tEXTENSIBLE, %u channels, %u-bit, %lu Hz, %u align, %lu avg\n",
                    wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                printf("\tERROR: Invalid WAVE_FORMAT_EXTENSIBLE\n");
            }
            else
            {
                static const GUID s_wfexBase = { 0x00000000, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

                auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);

                if (memcmp(reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                    reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD)) != 0)
                {
                    printf("\tEXTENSIBLE, %u channels, %u-bit, %lu Hz, %u align, %lu avg\n",
                        wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                    printf("\tERROR: Unknown EXTENSIBLE SubFormat {%8.8lX-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}\n",
                        wfex->SubFormat.Data1, wfex->SubFormat.Data2, wfex->SubFormat.Data3,
                        wfex->SubFormat.Data4[0], wfex->SubFormat.Data4[1], wfex->SubFormat.Data4[2], wfex->SubFormat.Data4[3],
                        wfex->SubFormat.Data4[4], wfex->SubFormat.Data4[5], wfex->SubFormat.Data4[6], wfex->SubFormat.Data4[7]);
                }
                else
                {
                    printf("\tEXTENSIBLE %s (%lu), %u channels, %u-bit, %lu Hz, %u align, %lu avg\n",
                        GetFormatTagName(wfex->SubFormat.Data1), wfex->SubFormat.Data1,
                        wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                    printf("\t\t%u samples per block, %u valid bps, %lu channel mask",
                        wfex->Samples.wSamplesPerBlock, wfex->Samples.wValidBitsPerSample, wfex->dwChannelMask);
                }
            }
        }
        else
        {
            printf("\t%s (%u), %u channels, %u-bit, %lu Hz, %u align, %lu avg\n",
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

#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

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

        printf("\tOutput format rate %lu, channels %u, %s (%08lX)\n", output.Format.nSamplesPerSec, output.Format.nChannels, speakerConfig, output.dwChannelMask);
    }

    if (!audEngine->IsAudioDevicePresent())
    {
        printf("ERROR: IsAudioDevicePresent returned unexpected value\n");
        return 1;
    }

#ifdef TEST_PCM
    { // PCM WaveBank

#ifdef TEST_4KN
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"WaveBank4Kn.xwb");
        printf("\n\nINFO: Loaded wavebank4kn.xwb\n");
        if (!wb->IsAdvancedFormat())
        {
            printf("\n\nERROR: Not a 4096 aligned wavebank!");
            return 1;
        }
#else
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"WaveBank.xwb");
        printf("\n\nINFO: Loaded wavebank.xwb\n");
        if (wb->IsAdvancedFormat())
        {
            printf("\n\nERROR: Not a 2048 aligned wavebank!");
            return 1;
        }
#endif

        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);

        for (uint32_t j = 0; j < 3; ++j)
        {
            printf("\tIndex #%u (%zu bytes, %zu samples, %zu ms)\n",
                j, wb->GetSampleSizeInBytes(j), wb->GetSampleDuration(j), wb->GetSampleDurationMS(j));
            dump_wfx(wb->GetFormat(j, wfx, 64));
        }

        auto stream1 = wb->CreateStreamInstance(0u);

        size_t streamDur = wb->GetSampleDurationMS(0u);

        printf("Playing #0: ");
        stream1->Play();

        if (stream1->IsLooped())
        {
            printf("\nERROR: Play() should have put it into a non-looped mode\n");
            return 1;
        }

        ULONGLONG startTick = GetTickCount64();

        while (stream1->GetState() == PLAYING)
        {
            UPDATE

            printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 60000)
                break;
        }

        printf("<done>\n");

        ULONGLONG dur = GetTickCount64() - startTick;

        if (dur < streamDur)
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, streamDur);
            return 1;
        }
        else if (dur > (streamDur + 2000))
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, streamDur);
            return 1;
        }

        // Restart
        printf("\nPlaying stream instance for 2 seconds...\n");

        stream1->Play();

        if (stream1->IsLooped())
        {
            printf("\nERROR: Play() should have put it into a non-looped mode\n");
            return 1;
        }

        startTick = GetTickCount64();

        while (stream1->GetState() == PLAYING)
        {
            UPDATE

            printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 2000)
                break;
        }

        printf("\nRestarting...\n");
        stream1->Stop();
        stream1->Play();

        startTick = GetTickCount64();

        while (stream1->GetState() == PLAYING)
        {
            UPDATE

                printf(".");
            Sleep(200);
        }

        dur = GetTickCount64() - startTick;

        if (dur < streamDur)
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, streamDur);
            return 1;
        }
        else if (dur > (streamDur + 2000))
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, streamDur);
            return 1;
        }

        // Loop
        auto stream2 = wb->CreateStreamInstance(2u);

        printf("\n\nPlaying #2 (looped): ");
        stream2->Play(true);

        if (!stream2->IsLooped())
        {
            printf("\nERROR: Play(true) should have put it into a looped mode\n");
            return 1;
        }

        startTick = GetTickCount64();

        bool exitloop = true;
        while (stream2->GetState() == PLAYING)
        {
            UPDATE

                if (GetAsyncKeyState(VK_ESCAPE))
                {
                    while (GetAsyncKeyState(VK_ESCAPE))
                        Sleep(10);
                    break;
                }

            printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 100000)
            {
                printf("<timeout>\n");
                return 1;
            }
            else if (exitloop && (tick > startTick + 45000))
            {
                printf("<breaking loop>");
                stream2->Stop(false);
                exitloop = false;
            }
        }

        stream2->Stop();
        printf("<done>\n");

        // Additional tests
        auto stream3 = wb->CreateStreamInstance(1u);

        printf("\n\nPlaying #1 (looped): ");
        stream3->Play(true);

        printf("\nLooping for 15 seconds...\n");

        if (!stream3->IsLooped())
        {
            printf("\nERROR: Play(true) should have put it into a looped mode\n");
            return 1;
        }

        startTick = GetTickCount64();

        while (stream3->GetState() == PLAYING)
        {
            UPDATE

                printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 15000)
                break;
        }

        // Pause
        stream3->Pause();

        if (stream3->GetState() != PAUSED)
        {
            printf("\nERROR: Pause should have put it into a PAUSED state\n");
            return 1;
        }

        printf("\nPausing for 5 seconds...\n");

        startTick = GetTickCount64();

        for (;;)
        {
            UPDATE

                printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 5000)
                break;
        }

        // Resume
        stream3->Resume();

        printf("\nResuming for 5 seconds...\n");

        if (stream3->GetState() != PLAYING)
        {
            printf("\nERROR: Play should have put it into a PLAYING state\n");
            return 1;
        }

        startTick = GetTickCount64();

        while (stream3->GetState() == PLAYING)
        {
            UPDATE

                printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 5000)
                break;
        }

        // Restart loop
        stream3->Stop();
        stream3->Play(true);

        printf("\nRestarting for 10 seconds...\n");

        if (stream3->GetState() != PLAYING)
        {
            printf("\nERROR: Play should have put it into a PLAYING state\n");
            return 1;
        }

        startTick = GetTickCount64();

        while (stream3->GetState() == PLAYING)
        {
            UPDATE

                printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 10000)
                break;
        }

        // Exit loop
        stream3->Stop(false);

        printf("\nExiting loop...\n");

        if (stream3->IsLooped())
        {
            printf("\nERROR: Stop(false) should have put it into a non-looped mode\n");
            return 1;
        }

        while (stream3->GetState() == PLAYING)
        {
            UPDATE

                printf(".");
            Sleep(200);
        }

        // Volume
        stream3->Play(true);

        printf("\nScaling volume...\n");

        float acc = 0.f;
        float step = 1.f / 10.f;

        while (stream3->GetState() == PLAYING)
        {
            stream3->SetVolume(acc);
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if (acc > 1.f)
            {
                acc = 1.f;
                step = -step;
            }
            else if (acc < 0.f)
                break;

            UPDATE
        }

        stream3->SetVolume(1.f);

        // Pan
        printf("\nPanning...\n");

        acc = -1.f;
        step = 1.f / 10.f;

        while (stream3->GetState() == PLAYING)
        {
            stream3->SetPan(acc);
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if (acc > 1.f)
                break;

            UPDATE
        }

        stream3->SetPan(0.f);

        // Pitch test (mono)
        printf("\nPitch-shifting...\n");

        acc = -1.f;
        step = 1.f / 10.f;

        while (stream3->GetState() == PLAYING)
        {
            stream3->SetPitch(acc);
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if (acc > 1.f)
                break;

            UPDATE
        }

        stream3->SetPitch(0.f);
        stream3->Stop();
    }
#endif

#ifdef TEST_ADPCM
    {
        // ADPCM WaveBank
#ifdef TEST_4KN
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"WaveBankADPCM4Kn.xwb");
        printf("\n\nINFO: Loaded WaveBankADPCM4Kn.xwb\n");
        if (!wb->IsAdvancedFormat())
        {
            printf("\n\nERROR: Not a 4096 aligned wavebank!");
            return 1;
        }
#else
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"WaveBankADPCM.xwb");
        printf("\n\nINFO: Loaded WaveBankADPCM.xwb\n");
        if (wb->IsAdvancedFormat())
        {
            printf("\n\nERROR: Not a 2048 aligned wavebank!");
            return 1;
        }
#endif

        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);

        for (uint32_t j = 0; j < 3; ++j)
        {
            printf("\tIndex #%u (%zu bytes, %zu samples, %zu ms)\n",
                j, wb->GetSampleSizeInBytes(j), wb->GetSampleDuration(j), wb->GetSampleDurationMS(j));
            dump_wfx(wb->GetFormat(j, wfx, 64));
        }

        auto stream1 = wb->CreateStreamInstance(0u);

        printf("Playing #0: ");
        stream1->Play();

        ULONGLONG startTick = GetTickCount64();

        while (stream1->GetState() == PLAYING)
        {
            UPDATE

                if (GetAsyncKeyState(VK_ESCAPE))
                {
                    while (GetAsyncKeyState(VK_ESCAPE))
                        Sleep(10);
                    break;
                }

            printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 60000)
            {
                printf("<timeout>\n");
                return 1;
            }
        }

        stream1->Stop();
        printf("<done>\n");

        auto stream2 = wb->CreateStreamInstance(2u);

        printf("\n\nPlaying #2 (looped): ");
        stream2->Play(true);

        startTick = GetTickCount64();

        bool exitloop = true;
        while (stream2->GetState() == PLAYING)
        {
            UPDATE

                if (GetAsyncKeyState(VK_ESCAPE))
                {
                    while (GetAsyncKeyState(VK_ESCAPE))
                        Sleep(10);
                    break;
                }

            printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 100000)
            {
                printf("<timeout>\n");
                return 1;
            }
            else if (exitloop && (tick > startTick + 45000))
            {
                printf("<breaking loop>");
                stream2->Stop(false);
                exitloop = false;
            }
        }

        stream2->Stop();
        printf("<done>\n");

        auto stream3 = wb->CreateStreamInstance(1u);

        printf("\n\nPlaying #1: ");
        stream3->Play();

        startTick = GetTickCount64();

        while (stream3->GetState() == PLAYING)
        {
            UPDATE

                if (GetAsyncKeyState(VK_ESCAPE))
                {
                    while (GetAsyncKeyState(VK_ESCAPE))
                        Sleep(10);
                    break;
                }

            printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 100000)
            {
                printf("<timeout>\n");
                return 1;
            }
        }

        stream3->Stop();
        printf("<done>\n");
    }
#endif

#ifdef TEST_XWMA
    {
        // xWMA WaveBank
#ifdef TEST_4KN
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"WaveBankxWMA4Kn.xwb");
        printf("\n\nINFO: Loaded WaveBankxWMA4Kn.xwb\n");
        if (!wb->IsAdvancedFormat())
        {
            printf("\n\nERROR: Not a 4096 aligned wavebank!");
            return 1;
        }
#else
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"WaveBankxWMA.xwb");
        printf("\n\nINFO: Loaded WaveBankxWMA.xwb\n");
        if (wb->IsAdvancedFormat())
        {
            printf("\n\nERROR: Not a 2048 aligned wavebank!");
            return 1;
        }
#endif

        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);

        for (uint32_t j = 0; j < 3; ++j)
        {
            printf("\tIndex #%u (%zu bytes, %zu samples, %zu ms)\n",
                j, wb->GetSampleSizeInBytes(j), wb->GetSampleDuration(j), wb->GetSampleDurationMS(j));
            dump_wfx(wb->GetFormat(j, wfx, 64));
        }

        auto stream1 = wb->CreateStreamInstance(0u);

        printf("Playing #0: ");
        stream1->Play();

        ULONGLONG startTick = GetTickCount64();

        while (stream1->GetState() == PLAYING)
        {
            UPDATE

                if (GetAsyncKeyState(VK_ESCAPE))
                {
                    while (GetAsyncKeyState(VK_ESCAPE))
                        Sleep(10);
                    break;
                }

            printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 60000)
            {
                printf("<timeout>\n");
                return 1;
            }
        }

        stream1->Stop();
        printf("<done>\n");

        auto stream2 = wb->CreateStreamInstance(2u);

        printf("\n\nPlaying #2 (looped): ");
        stream2->Play(true);

        startTick = GetTickCount64();

        bool exitloop = true;
        while (stream2->GetState() == PLAYING)
        {
            UPDATE

                if (GetAsyncKeyState(VK_ESCAPE))
                {
                    while (GetAsyncKeyState(VK_ESCAPE))
                        Sleep(10);
                    break;
                }

            printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 100000)
            {
                printf("<timeout>\n");
                return 1;
            }
            else if (exitloop && (tick > startTick + 45000))
            {
                printf("<breaking loop>");
                stream2->Stop(false);
                exitloop = false;
            }
        }

        stream2->Stop();
        printf("<done>\n");

        auto stream3 = wb->CreateStreamInstance(1u);

        printf("\n\nPlaying #1: ");
        stream3->Play();

        startTick = GetTickCount64();

        while (stream3->GetState() == PLAYING)
        {
            UPDATE

                if (GetAsyncKeyState(VK_ESCAPE))
                {
                    while (GetAsyncKeyState(VK_ESCAPE))
                        Sleep(10);
                    break;
                }

            printf(".");
            Sleep(200);

            ULONGLONG tick = GetTickCount64();

            if (tick > startTick + 80000)
            {
                printf("<timeout>\n");
                return 1;
            }
        }

        stream3->Stop();
        printf("<done>\n");
    }
#endif

    return 0;
}
