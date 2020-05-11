//--------------------------------------------------------------------------------------
// File: BasicAudioTest.cpp
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
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "Audio.h"

#include <stdio.h>

// C4619/4616 #pragma warning warnings
// C26451 Arithmetic overflow
#pragma warning(disable : 4619 4616 26451)

using namespace DirectX;

#define TEST_SINE_WAVE
#define TEST_LIMITER
#define TEST_SOUNDEFFECT
#define TEST_WAVEBANK
#define TEST_SHUTDOWN

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
    void dump_stats(_In_ AudioEngine* engine)
    {
        auto stats = engine->GetStatistics();

        printf("\nPlaying: %zu / %zu; Instances %zu; Voices %zu / %zu / %zu / %zu; %zu audio bytes\n",
            stats.playingOneShots, stats.playingInstances,
            stats.allocatedInstances, stats.allocatedVoices, stats.allocatedVoices3d,
            stats.allocatedVoicesOneShot, stats.allocatedVoicesIdle,
            stats.audioBytes);
    }


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


    //----------------------------------------------------------------------------------
    void GenerateSineWave(_Out_writes_(sampleRate) int16_t* data, int sampleRate, int frequency)
    {
        const double timeStep = 1.0 / double(sampleRate);
        const double freq = double(frequency);

        int16_t* ptr = data;
        double time = 0.0;
        for (int j = 0; j < sampleRate; ++j, ++ptr)
        {
            double angle = (2.0 * XM_PI * freq) * time;
            double factor = 0.5 * (sin(angle) + 1.0);
            *ptr = int16_t(32768 * factor);
            time += timeStep;
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

    //
    // Enumerate devices
    //

    auto enumList = AudioEngine::GetRendererDetails();

    if ( enumList.empty() )
    {
        printf("ERROR: Audio device enumeration results in no devices\n");
        return 1;
    }
    else
    {
        printf("INFO: Found %zu audio devices:\n", enumList.size() );
        for( auto it = enumList.cbegin(); it != enumList.cend(); ++it )
        {
            printf( "\t\"%ls\"\n", it->description.c_str() );
        }
    }

    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif
#ifdef TEST_LIMITER
    eflags = eflags | AudioEngine_UseMasteringLimiter;
#endif

    // First device test using explicit device ID
    {
        printf("\nTrying specific device and format\n\t\"%ls\"\n", enumList[0].description.c_str() );

        WAVEFORMATEX testwfx;
        testwfx.wFormatTag = WAVE_FORMAT_PCM;
        testwfx.nChannels = 1;
        testwfx.nSamplesPerSec = 22100;

        auto audEngine = std::make_unique<AudioEngine>(eflags, &testwfx, enumList[0].deviceId.c_str());

        {
            auto output = audEngine->GetOutputFormat();

            if ( output.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE
                 || output.Format.cbSize != ( sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) ) )
            {
                printf("ERROR: GetOutputFormat failed\n" );
                dump_wfx( reinterpret_cast<const WAVEFORMATEX*>( &output ) );
                return 1;
            }

            if ( output.Format.nChannels != testwfx.nChannels
                 || output.Format.nSamplesPerSec != testwfx.nSamplesPerSec )
            {
                printf("ERROR: Output format rate %u, channels %u was not expected\n", output.Format.nSamplesPerSec, output.Format.nChannels );
                dump_wfx( reinterpret_cast<const WAVEFORMATEX*>( &output ) );
                return 1;
            }

            if ( !audEngine->IsAudioDevicePresent() )
            {
                printf("ERROR: IsAudioDevicePresent returned unexpected value\n");
                return 1;
            }
        }

        printf("\tPASS\n");
    }

    // Use default for rest of tests
    printf("\nTrying default device\n");

    auto audEngine = std::make_unique<AudioEngine>(eflags);

    {
        auto output = audEngine->GetOutputFormat();

        if ( output.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE
             || output.Format.cbSize != ( sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) ) )
        {
            printf("ERROR: GetOutputFormat failed\n" );
            dump_wfx( reinterpret_cast<const WAVEFORMATEX*>( &output ) );
            return 1;
        }

        const char* speakerConfig;
        switch( output.dwChannelMask )
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

        printf("\tOutput format rate %u, channels %u, %s (%08X)\n", output.Format.nSamplesPerSec, output.Format.nChannels, speakerConfig, output.dwChannelMask );
    }

    if ( !audEngine->IsAudioDevicePresent() )
    {
        printf("ERROR: IsAudioDevicePresent returned unexpected value\n");
        return 1;
    }

    printf("\tPASS\n");

#ifdef TEST_SINE_WAVE

    //
    // SoundEffect/SoundEffectInstance
    //

    { // PCM Sine Wave
        size_t audioSize = 44100 * 2;
        auto wavData = std::make_unique<uint8_t[]>(audioSize + sizeof(WAVEFORMATEX));

        auto audioStart = wavData.get() + sizeof(WAVEFORMATEX);

        GenerateSineWave( reinterpret_cast<int16_t*>( audioStart ), 44100, 440 );
        
        auto wfx = reinterpret_cast<WAVEFORMATEX*>( wavData.get() );
        wfx->wFormatTag = WAVE_FORMAT_PCM;
        wfx->nChannels = 1;
        wfx->nSamplesPerSec = 44100;
        wfx->nAvgBytesPerSec = 2 * 44100;
        wfx->nBlockAlign = 2;
        wfx->wBitsPerSample = 16;
        wfx->cbSize = 0;
        dump_wfx( wfx );

        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), wavData, wfx, audioStart, audioSize);

        printf( "\n\nINFO: PCM-A440 sine wave (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), soundEffect->GetSampleDurationMS() );

        auto wfxRef = soundEffect->GetFormat();
        if ( !wfxRef
             || (memcmp( wfx, wfxRef, sizeof(WAVEFORMATEX) ) != 0 ) )
        {
            printf("ERROR: GetFormat() failed\n" );
            return 1;
        }

        auto effect = soundEffect->CreateInstance();

        printf("\nPlaying sound effect instance...\n" );

        effect->Play(true);

        printf("\nLooping for 5 seconds...\n");

        if( !effect->IsLooped() )
        {
            printf("\nERROR: Play(true) should have put it into a looped mode\n" );
            return 1;
        }

        ULONGLONG startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 5000 )
                break;
        }

        // Exit loop
        effect->Stop( false );

        printf("\nExiting loop...\n");

        if( effect->IsLooped() )
        {
            printf("\nERROR: Stop(false) should have put it into a non-looped mode\n" );
            return 1;
        }

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

#ifdef TEST_LIMITER

        printf("\n\nMastering volume limiter...\n");

        effect->Play( true );

        printf( "\nMin for 5 seconds...\n" );
        audEngine->SetMasteringLimit( FXMASTERINGLIMITER_MIN_RELEASE, FXMASTERINGLIMITER_MIN_LOUDNESS );

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 5000 )
                break;
        }

        printf( "\nMax for 5 seconds...\n" );
        audEngine->SetMasteringLimit( FXMASTERINGLIMITER_MAX_RELEASE, FXMASTERINGLIMITER_MAX_LOUDNESS );

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 5000 )
                break;
        }

        audEngine->SetMasteringLimit( FXMASTERINGLIMITER_DEFAULT_RELEASE, FXMASTERINGLIMITER_DEFAULT_LOUDNESS );

#endif // TEST_LIMITER

        printf("\n\nMastering volume test...\n");

        effect->Play( true );

        float acc = 0.f;
        float step = 1.f / 10.f;

        while ( effect->GetState() == PLAYING )
        {
            audEngine->SetMasterVolume( acc );
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if ( acc > 1.f )
            {
                acc = 1.f;
                step = -step;
            }
            else if ( acc < 0.f )
                break;

            UPDATE
        }

        audEngine->SetMasterVolume( 1.f  );
    };

#endif // TEST_SINE_WAVE

#ifdef TEST_SOUNDEFFECT
    { // PCM .WAV
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"MusicMono.wav");

        size_t effectDur = soundEffect->GetSampleDurationMS();

        printf( "\n\nINFO: Loaded MusicMono.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), effectDur );

        dump_wfx( soundEffect->GetFormat() );
    
        auto effect = soundEffect->CreateInstance();

        // Standard
        printf("\nPlaying sound effect instance...\n" );

        effect->Play();

        if( effect->IsLooped() )
        {
            printf("\nERROR: Play() should have put it into a non-looped mode\n" );
            return 1;
        }

        ULONGLONG startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        ULONGLONG dur = GetTickCount64() - startTick;

        if ( dur < effectDur )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
            return 1;
        }
        else if ( dur > ( effectDur + 2000 ) )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
            return 1;
        }

        // Restart
        printf("\nPlaying sound effect instance for 2 seconds...\n" );

        effect->Play();

        if( effect->IsLooped() )
        {
            printf("\nERROR: Play() should have put it into a non-looped mode\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 2000 )
                break;
        }

        printf("\nRestarting...\n" );
        effect->Stop();
        effect->Play();

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        dur = GetTickCount64() - startTick;

        if ( dur < effectDur )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
            return 1;
        }
        else if ( dur > ( effectDur + 2000 ) )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
            return 1;
        }

        // Loop
        effect->Play(true);

        printf("\nLooping for 15 seconds...\n");

        if( !effect->IsLooped() )
        {
            printf("\nERROR: Play(true) should have put it into a looped mode\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 15000 )
                break;
        }

        // Pause
        effect->Pause();

        if ( effect->GetState() != PAUSED )
        {
            printf("\nERROR: Pause should have put it into a PAUSED state\n" );
            return 1;
        }

        printf("\nPausing for 5 seconds...\n");

        startTick = GetTickCount64();

        for(;;)
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 5000 )
                break;
        }

        // Resume
        effect->Resume();

        printf("\nResuming for 5 seconds...\n");

        if ( effect->GetState() != PLAYING )
        {
            printf("\nERROR: Play should have put it into a PLAYING state\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 5000 )
                break;
        }

        // Restart loop
        effect->Stop();
        effect->Play(true);

        printf("\nRestarting for 10 seconds...\n");

        if ( effect->GetState() != PLAYING )
        {
            printf("\nERROR: Play should have put it into a PLAYING state\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 10000 )
                break;
        }

        // Exit loop
        effect->Stop( false );

        printf("\nExiting loop...\n");

        if( effect->IsLooped() )
        {
            printf("\nERROR: Stop(false) should have put it into a non-looped mode\n" );
            return 1;
        }

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        // Volume
        effect->Play(true);

        printf("\nScaling volume...\n" );

        float acc = 0.f;
        float step = 1.f / 10.f;

        while ( effect->GetState() == PLAYING )
        {
            effect->SetVolume( acc );
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if ( acc > 1.f )
            {
                acc = 1.f;
                step = -step;
            }
            else if ( acc < 0.f )
                break;

            UPDATE
        }

        effect->SetVolume( 1.f );

        // Pan
        printf("\nPanning...\n" );

        acc = -1.f;
        step = 1.f / 10.f;

        while ( effect->GetState() == PLAYING )
        {
            effect->SetPan( acc );
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if ( acc > 1.f )
                break;

            UPDATE
        }

        effect->SetPan( 0.f );

        // Pitch test (mono)
        printf("\nPitch-shifting...\n" );

        acc = -1.f;
        step = 1.f / 10.f;

        while ( effect->GetState() == PLAYING )
        {
            effect->SetPitch( acc );
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if ( acc > 1.f )
                break;

            UPDATE
        }

        effect->SetPitch( 0.f );
        effect->Stop();

        dump_stats( audEngine.get() );

        // TrimVoicePool test
        auto stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 1
             || stats.allocatedInstances != 1 )
        {
            printf( "\nERROR: GetStatistics() failed\n" );
            return 1;
        }

        printf(" \nTrim voice pool...\n" );
        audEngine->TrimVoicePool();

        dump_stats( audEngine.get() );

        stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 0
             || stats.allocatedInstances != 1 )
        {
            printf( "\nERROR: TrimVoicePool() failed\n" );
            return 1;
        }
    }

    { // PCM .WAV (Stereo)
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"Alarm01.wav");

        size_t effectDur = soundEffect->GetSampleDurationMS();

        printf( "\n\nINFO: Loaded Alarm01.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), effectDur );

        dump_wfx( soundEffect->GetFormat() );
    
        auto effect = soundEffect->CreateInstance();

        // Standard
        printf("\nPlaying sound effect instance...\n" );

        effect->Play();

        if( effect->IsLooped() )
        {
            printf("\nERROR: Play() should have put it into a non-looped mode\n" );
            return 1;
        }

        ULONGLONG startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        ULONGLONG dur = GetTickCount64() - startTick;

        if ( dur < effectDur )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
            return 1;
        }
        else if ( dur > ( effectDur + 2000 ) )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
            return 1;
        }

        // Restart
        printf("\nPlaying sound effect instance for 2 seconds...\n" );

        effect->Play();

        if( effect->IsLooped() )
        {
            printf("\nERROR: Play() should have put it into a non-looped mode\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 2000 )
                break;
        }

        printf("\nRestarting...\n" );
        effect->Stop();
        effect->Play();

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        dur = GetTickCount64() - startTick;

        if ( dur < effectDur )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
            return 1;
        }
        else if ( dur > ( effectDur + 2000 ) )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
            return 1;
        }

        // Loop
        effect->Play(true);

        printf("\nLooping for 15 seconds...\n");

        if( !effect->IsLooped() )
        {
            printf("\nERROR: Play(true) should have put it into a looped mode\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 15000 )
                break;
        }

        // Pause
        effect->Pause();

        if ( effect->GetState() != PAUSED )
        {
            printf("\nERROR: Pause should have put it into a PAUSED state\n" );
            return 1;
        }

        printf("\nPausing for 5 seconds...\n");

        startTick = GetTickCount64();

        for(;;)
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 5000 )
                break;
        }

        // Resume
        effect->Resume();

        printf("\nResuming for 5 seconds...\n");

        if ( effect->GetState() != PLAYING )
        {
            printf("\nERROR: Play should have put it into a PLAYING state\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 5000 )
                break;
        }

        // Restart loop
        effect->Stop();
        effect->Play(true);

        printf("\nRestarting for 10 seconds...\n");

        if ( effect->GetState() != PLAYING )
        {
            printf("\nERROR: Play should have put it into a PLAYING state\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 10000 )
                break;
        }

        // Exit loop
        effect->Stop( false );

        printf("\nExiting loop...\n");

        if( effect->IsLooped() )
        {
            printf("\nERROR: Stop(false) should have put it into a non-looped mode\n" );
            return 1;
        }

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        // Volume
        effect->Play(true);

        printf("\nScaling volume...\n" );

        float acc = 0.f;
        float step = 1.f / 10.f;

        while ( effect->GetState() == PLAYING )
        {
            effect->SetVolume( acc );
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if ( acc > 1.f )
            {
                acc = 1.f;
                step = -step;
            }
            else if ( acc < 0.f )
                break;

            UPDATE
        }

        effect->SetVolume( 1.f );

        // Pan
        printf("\nPanning...\n" );

        acc = -1.f;
        step = 1.f / 10.f;

        while ( effect->GetState() == PLAYING )
        {
            effect->SetPan( acc );
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if ( acc > 1.f )
                break;

            UPDATE
        }

        effect->SetPan( 0.f );

        // Pitch test
        printf("\nPitch-shifting...\n" );

        acc = -1.f;
        step = 1.f / 10.f;

        while ( effect->GetState() == PLAYING )
        {
            effect->SetPitch( acc );
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if ( acc > 1.f )
                break;

            UPDATE
        }

        effect->SetPitch( 0.f );
        effect->Stop();

        dump_stats( audEngine.get() );

        // TrimVoicePool test
        auto stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 1
             || stats.allocatedInstances != 1 )
        {
            printf( "\nERROR: GetStatistics() failed\n" );
            return 1;
        }

        printf(" \nTrim voice pool...\n" );
        audEngine->TrimVoicePool();

        dump_stats( audEngine.get() );

        stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 0
             || stats.allocatedInstances != 1 )
        {
            printf( "\nERROR: TrimVoicePool() failed\n" );
            return 1;
        }
    }

    { // PCM .WAV (Multichannel)
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"MusicSurround.wav");

        size_t effectDur = soundEffect->GetSampleDurationMS();

        printf( "\n\nINFO: Loaded MusicSurround.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), effectDur );

        dump_wfx( soundEffect->GetFormat() );
    
        auto effect = soundEffect->CreateInstance();

        // Standard
        printf("\nPlaying sound effect instance...\n" );

        effect->Play();

        if( effect->IsLooped() )
        {
            printf("\nERROR: Play() should have put it into a non-looped mode\n" );
            return 1;
        }

        ULONGLONG startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        ULONGLONG dur = GetTickCount64() - startTick;

        if ( dur < effectDur )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
            return 1;
        }
        else if ( dur > ( effectDur + 2000 ) )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
            return 1;
        }

        // Restart
        printf("\nPlaying sound effect instance for 2 seconds...\n" );

        effect->Play();

        if( effect->IsLooped() )
        {
            printf("\nERROR: Play() should have put it into a non-looped mode\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 2000 )
                break;
        }

        printf("\nRestarting...\n" );
        effect->Stop();
        effect->Play();

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        dur = GetTickCount64() - startTick;

        if ( dur < effectDur )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
            return 1;
        }
        else if ( dur > ( effectDur + 2000 ) )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
            return 1;
        }

        // Loop
        effect->Play(true);

        printf("\nLooping for 15 seconds...\n");

        if( !effect->IsLooped() )
        {
            printf("\nERROR: Play(true) should have put it into a looped mode\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 15000 )
                break;
        }

        // Pause
        effect->Pause();

        if ( effect->GetState() != PAUSED )
        {
            printf("\nERROR: Pause should have put it into a PAUSED state\n" );
            return 1;
        }

        printf("\nPausing for 5 seconds...\n");

        startTick = GetTickCount64();

        for(;;)
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 5000 )
                break;
        }

        // Resume
        effect->Resume();

        printf("\nResuming for 5 seconds...\n");

        if ( effect->GetState() != PLAYING )
        {
            printf("\nERROR: Play should have put it into a PLAYING state\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 5000 )
                break;
        }

        // Restart loop
        effect->Stop();
        effect->Play(true);

        printf("\nRestarting for 10 seconds...\n");

        if ( effect->GetState() != PLAYING )
        {
            printf("\nERROR: Play should have put it into a PLAYING state\n" );
            return 1;
        }

        startTick = GetTickCount64();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);

            ULONGLONG tick = GetTickCount64();

            if ( tick > startTick + 10000 )
                break;
        }

        // Exit loop
        effect->Stop( false );

        printf("\nExiting loop...\n");

        if( effect->IsLooped() )
        {
            printf("\nERROR: Stop(false) should have put it into a non-looped mode\n" );
            return 1;
        }

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        // Volume
        effect->Play(true);

        printf("\nScaling volume...\n" );

        float acc = 0.f;
        float step = 1.f / 10.f;

        while ( effect->GetState() == PLAYING )
        {
            effect->SetVolume( acc );
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if ( acc > 1.f )
            {
                acc = 1.f;
                step = -step;
            }
            else if ( acc < 0.f )
                break;

            UPDATE
        }

        effect->SetVolume( 1.f );

        // Pan
        effect->SetPan( 0.f );

        // Pitch test
        printf("\nPitch-shifting...\n" );

        acc = -1.f;
        step = 1.f / 10.f;

        while ( effect->GetState() == PLAYING )
        {
            effect->SetPitch( acc );
            printf(".(%.1f)", acc);
            Sleep(500);
            acc += step;

            if ( acc > 1.f )
                break;

            UPDATE
        }

        effect->SetPitch( 0.f );
        effect->Stop();

        dump_stats( audEngine.get() );

        // TrimVoicePool test
        auto stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 1
             || stats.allocatedInstances != 1 )
        {
            printf( "\nERROR: GetStatistics() failed\n" );
            return 1;
        }

        printf(" \nTrim voice pool...\n" );
        audEngine->TrimVoicePool();

        dump_stats( audEngine.get() );

        stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 0
             || stats.allocatedInstances != 1 )
        {
            printf( "\nERROR: TrimVoicePool() failed\n" );
            return 1;
        }
    }

    { // PCM float .WAV
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"HipHoppy_float.wav");

        size_t effectDur = soundEffect->GetSampleDurationMS();

        printf( "\nINFO: Loaded HipHoppy_float.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), effectDur);

        dump_wfx( soundEffect->GetFormat() );
    
        auto effect = soundEffect->CreateInstance();

        // Standard
        printf("\nPlaying sound effect instance...\n" );

        ULONGLONG startTick = GetTickCount64();

        effect->Play();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        ULONGLONG dur = GetTickCount64() - startTick;

        if ( dur < effectDur )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
            return 1;
        }
        else if ( dur > ( effectDur + 1000 ) )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
            return 1;
        }

        dump_stats( audEngine.get() );
    }

    { // ADPCM .WAV
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"electro_adpcm.wav");

        size_t effectDur = soundEffect->GetSampleDurationMS();

        printf( "\nINFO: Loaded electro_adpcm.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), effectDur);

        dump_wfx( soundEffect->GetFormat() );
    
        auto effect = soundEffect->CreateInstance();

        // Standard
        printf("\nPlaying sound effect instance...\n" );

        ULONGLONG startTick = GetTickCount64();

        effect->Play();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        ULONGLONG dur = GetTickCount64() - startTick;

        if ( dur < effectDur )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
            return 1;
        }
        else if ( dur > ( effectDur + 1000 ) )
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
            return 1;
        }

        dump_stats( audEngine.get() );
    }

    #ifdef USING_XAUDIO2_9

    { // xWMA .WAV
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"musicmono_xwma.wav");

        size_t effectDur = soundEffect->GetSampleDurationMS();

        printf( "\nINFO: Loaded musicmono_xwma.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), effectDur );
    
        dump_wfx( soundEffect->GetFormat() );

        auto effect = soundEffect->CreateInstance();

        // Standard
        printf("\nPlaying sound effect instance...\n" );

        ULONGLONG startTick = GetTickCount64();

        effect->Play();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        ULONGLONG dur = GetTickCount64() - startTick;

        if (dur < effectDur)
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
            return 1;
        }
        else if (dur >(effectDur + 1000))
        {
            printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
            return 1;
        }

        dump_stats( audEngine.get() );
    }

#endif // xWMA

    // SoundEffect One-shots
    { // PCM .WAV one-shots
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"Alarm01.wav");
        printf( "\n\nINFO: Loaded Alarm01.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), soundEffect->GetSampleDurationMS() );

        dump_wfx( soundEffect->GetFormat() );

        printf("One-shots\n*");
        soundEffect->Play();
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play();

        Sleep (1000);
        UPDATE

        printf(".");
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play();

        for( int j=0; j < 5; ++j )
        {
            if ( j > 0 ) printf(".");
            Sleep (1000);
            UPDATE
        }

        printf(" *");
        soundEffect->Play();

        while (soundEffect->IsInUse())
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        dump_stats( audEngine.get() );

        // TrimVoicePool test
        auto stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 3
             || stats.allocatedInstances != 0
             || stats.allocatedVoicesOneShot != 3
             || stats.allocatedVoicesIdle != 2 )
        {
            printf( "\nERROR: GetStatistics() failed\n" );
            return 1;
        }

        printf(" \nTrim voice pool...\n" );
        audEngine->TrimVoicePool();

        dump_stats( audEngine.get() );

        stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 1
             || stats.allocatedInstances != 0
             || stats.allocatedVoicesOneShot != 1
             || stats.allocatedVoicesIdle != 0 )
        {
            printf( "\nERROR: TrimVoicePool() failed\n" );
            return 1;
        }

        // Let last one-shot voice move to free list...
        UPDATE

        dump_stats( audEngine.get() );

        stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 1
             || stats.allocatedInstances != 0
             || stats.allocatedVoicesOneShot != 1
             || stats.allocatedVoicesIdle != 1 )
        {
            printf( "\nERROR: Update() failed\n" );
            return 1;
        }

        audEngine->TrimVoicePool();

        dump_stats( audEngine.get() );

        stats = audEngine->GetStatistics();

        if ( stats.allocatedVoices != 0
             || stats.allocatedInstances != 0
             || stats.allocatedVoicesOneShot != 0
             || stats.allocatedVoicesIdle != 0 )
        {
            printf( "\nERROR: TrimVoicePool() failed\n" );
            return 1;
        }

        printf("\nOne-shots w/ volume\n*");
        soundEffect->Play( 0.25f, 0.f, 0.f );
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play( 0.25f, 0.f, 0.f );

        Sleep (1000);
        UPDATE

        printf(".");
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play( 0.25f, 0.f, 0.f );

        for( int j=0; j < 5; ++j )
        {
            if ( j > 0 ) printf(".");
            Sleep (1000);
            UPDATE
        }

        printf(" *");
        soundEffect->Play( 0.25f, 0.f, 0.f );

        while (soundEffect->IsInUse())
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        printf("\nOne-shots w/ pitch\n*");
        soundEffect->Play( 1.f, 0.5f, 0.f );
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play( 1.f, -0.5f, 0.f );

        Sleep (1000);
        UPDATE

        printf(".");
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play( 1.f, 1.f, 0.f );

        for( int j=0; j < 5; ++j )
        {
            if ( j > 0 ) printf(".");
            Sleep (1000);
            UPDATE
        }

        printf(" *");
        soundEffect->Play( 1.f, -1.f, 0.f );

        while (soundEffect->IsInUse())
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        printf("\nOne-shots w/ pan\n*");
        soundEffect->Play( 1.f, 0.f, -1.f );
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play( 1.f, 0.f, 1.f );

        Sleep (1000);
        UPDATE

        printf(".");
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play( 1.f, 0.f, 1.f );

        for( int j=0; j < 5; ++j )
        {
            if ( j > 0 ) printf(".");
            Sleep (1000);
            UPDATE
        }

        printf(" *");
        soundEffect->Play( 1.f, 0.f, 1.f );

        while (soundEffect->IsInUse())
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        dump_stats( audEngine.get() );
    }

    { // PCM float .WAV one-shots
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"Alarm01_float.wav");
        printf( "\n\nINFO: Loaded Alarm01_float.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), soundEffect->GetSampleDurationMS() );

        dump_wfx( soundEffect->GetFormat() );

        printf("One-shots\n*");
        soundEffect->Play();
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play();

        Sleep (1000);
        UPDATE

        printf(".");
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play();

        for( int j=0; j < 5; ++j )
        {
            if ( j > 0 ) printf(".");
            Sleep (1000);
            UPDATE
        }

        printf(" *");
        soundEffect->Play();

        while (soundEffect->IsInUse())
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        dump_stats( audEngine.get() );
    }

    { // ADPCM .WAV one-shots
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"Alarm01_adpcm.wav");
        printf( "\n\nINFO: Loaded Alarm01_adpcm.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), soundEffect->GetSampleDurationMS() );

        dump_wfx( soundEffect->GetFormat() );

        printf("One-shots\n*");
        soundEffect->Play();
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play();

        Sleep (1000);
        UPDATE

        printf(".");
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play();

        for( int j=0; j < 5; ++j )
        {
            if ( j > 0 ) printf(".");
            Sleep (1000);
            UPDATE
        }

        printf(" *");
        soundEffect->Play();

        while (soundEffect->IsInUse())
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        dump_stats( audEngine.get() );
    }

    #ifdef USING_XAUDIO2_9

    { // xWMA .WAV one-shots
        auto soundEffect = std::make_unique<SoundEffect>(audEngine.get(), L"Alarm01_xwma.wav");
        printf( "\n\nINFO: Loaded Alarm01_xwma.wav (%zu bytes, %zu samples, %zu ms)\n",
                soundEffect->GetSampleSizeInBytes(), soundEffect->GetSampleDuration(), soundEffect->GetSampleDurationMS() );

        dump_wfx( soundEffect->GetFormat() );

        printf("One-shots\n*");
        soundEffect->Play();
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play();

        Sleep (1000);
        UPDATE

        printf(".");
        Sleep (1000);
        UPDATE

        printf(" *");
        soundEffect->Play();

        for( int j=0; j < 5; ++j )
        {
            if ( j > 0 ) printf(".");
            Sleep (1000);
            UPDATE
        }

        printf(" *");
        soundEffect->Play();

        while (soundEffect->IsInUse())
        {
            UPDATE

            printf(".");
            Sleep(1000);
        }

        dump_stats( audEngine.get() );
    }

#endif // xWMA

#endif // TEST_SOUNDEFFECT

#ifdef TEST_WAVEBANK
    //
    // WaveBank/SoundEffectInstance
    //

    { // Compact WaveBank validation
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"compact.xwb");
        printf( "\n\nINFO: Loaded compact.xwb\n" );

        if ( wb->Find( "Test" ) != -1 )
        {
            printf( "ERROR: WaveBank::Find failed\n" );
            return 1;
        }

        if ( ( wb->Find( "Explo1" ) != 0 )
             || ( wb->Find( "Explo2" ) != 1 )
             || ( wb->Find( "Explo3" ) != 2 )
             || ( wb->Find( "Explo4" ) != 3 ) )
        {
            // compact.xwb contains friendly entry names
            printf( "ERROR: WaveBank::Find failed\n" );
            return 1;
        }

        char buff[64];
        auto wfx = reinterpret_cast<WAVEFORMATEX*>( &buff );

        for (unsigned int j = 0; j < 4; ++j)
        {
            size_t effectDur = wb->GetSampleDurationMS(j);
            printf("\n\tIndex #%u (%zu bytes, %zu samples, %zu ms)\n", j,
                wb->GetSampleSizeInBytes(j), wb->GetSampleDuration(j), effectDur);
            dump_wfx(wb->GetFormat(j, wfx, 64));

            auto bankEffect = wb->CreateInstance( j );
            if ( !bankEffect )
            {
                printf("ERROR: Failed to create effect instance\n");
                return 1;
            }

            printf("\nPlaying sound effect instance...\n" );

            ULONGLONG startTick = GetTickCount64();

            bankEffect->Play();

            while ( bankEffect->GetState() == PLAYING )
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            ULONGLONG dur = GetTickCount64() - startTick;

            if (dur < effectDur)
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
                return 1;
            }
            else if (dur >(effectDur + 1000))
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
                return 1;
            }
        }

        dump_stats( audEngine.get() );
    }

    { // WaveBank validation
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"wavebank.xwb");
        printf( "\n\nINFO: Loaded wavebank.xwb\n" );

        if ( wb->Find( "Test" ) != -1 )
        {
            printf( "ERROR: WaveBank::Find failed\n" );
            return 1;
        }

        if ( ( wb->Find( "Explo1" ) != 0 )
             || ( wb->Find( "Explo2" ) != 1 )
             || ( wb->Find( "Explo3" ) != 2 )
             || ( wb->Find( "Explo4" ) != 3 ) )
        {
            // wavebank.xwb contains friendly entry names
            printf( "ERROR: WaveBank::Find failed\n" );
            return 1;
        }

        char buff[64];
        auto wfx = reinterpret_cast<WAVEFORMATEX*>( &buff );

        for (unsigned int j = 0; j < 4; ++j)
        {
            size_t effectDur = wb->GetSampleDurationMS(j);
            printf("\n\tIndex #%u (%zu bytes, %zu samples, %zu ms)\n", j,
                wb->GetSampleSizeInBytes(j), wb->GetSampleDuration(j), effectDur);
            dump_wfx(wb->GetFormat(j, wfx, 64));

            auto bankEffect = wb->CreateInstance( j );
            if ( !bankEffect )
            {
                printf("ERROR: Failed to create effect instance\n");
                return 1;
            }

            printf("\nPlaying sound effect instance...\n" );

            ULONGLONG startTick = GetTickCount64();

            bankEffect->Play();

            while ( bankEffect->GetState() == PLAYING )
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            ULONGLONG dur = GetTickCount64() - startTick;

            if (dur < effectDur)
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
                return 1;
            }
            else if (dur >(effectDur + 1000))
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
                return 1;
            }
        }

        dump_stats( audEngine.get() );
    }

    { // PCM WaveBank
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"droid.xwb");
        printf( "\n\nINFO: Loaded droid.xwb\n" );

        if ( wb->Find( "Test" ) != -1 )
        {
            printf( "ERROR: WaveBank::Find failed\n" );
            return 1;
        }

        if ( ( wb->Find( "Rumble2" ) != 10 )
             || ( wb->Find( "XACTGameGroove3" ) != 11 )
             || ( wb->Find( "droid_destroyed3" ) != 8 ) )
        {
            // droid.xwb contains friendly entry names
            printf( "ERROR: WaveBank::Find failed\n" );
            return 1;
        }

        char buff[64];
        auto wfx = reinterpret_cast<WAVEFORMATEX*>( &buff );

        size_t effectDur = wb->GetSampleDurationMS(10);

        printf( "\tIndex #10 (%zu bytes, %zu samples, %zu ms)\n",
                wb->GetSampleSizeInBytes( 10 ), wb->GetSampleDuration( 10 ), effectDur );
        dump_wfx( wb->GetFormat( 10, wfx, 64) );

        printf( "\tIndex #11 (%zu bytes, %zu samples, %zu ms)\n",
                wb->GetSampleSizeInBytes( 11 ), wb->GetSampleDuration( 11 ), wb->GetSampleDurationMS( 11 ) );
        dump_wfx( wb->GetFormat( 11, wfx, 64) );

        printf( "\tIndex #8 (%zu bytes, %zu samples, %zu ms)\n",
                wb->GetSampleSizeInBytes( 8 ), wb->GetSampleDuration( 8 ), wb->GetSampleDurationMS( 8 ) );
        dump_wfx( wb->GetFormat( 8, wfx, 64) );

        {
            auto bankEffect = wb->CreateInstance( 10 );
            if ( !bankEffect )
            {
                printf("ERROR: Failed to create effect instance\n");
                return 1;
            }

            printf("\nPlaying sound effect instance...\n" );

            ULONGLONG startTick = GetTickCount64();

            bankEffect->Play();

            while ( bankEffect->GetState() == PLAYING )
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            ULONGLONG dur = GetTickCount64() - startTick;

            if (dur < effectDur)
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
                return 1;
            }
            else if (dur >(effectDur + 1000))
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
                return 1;
            }

            dump_stats( audEngine.get() );
        }

        // WaveBank One-shots
        {
            printf("\nOne-shots\n*");
            wb->Play( 11 );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8 );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8 );
            Sleep (1000);
            UPDATE

            printf(".");
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8 );

            while (wb->IsInUse())
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            dump_stats( audEngine.get() );
        }

        // WaveBank One-shots w/ volume
        {
            printf("\nOne-shots w/ volume\n*");
            wb->Play( 11, 0.25f, 0.f, 0.f );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8, 0.25f, 0.f, 0.f );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8, 0.25f, 0.f, 0.f );
            Sleep (1000);
            UPDATE

            printf(".");
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8, 0.25f, 0.f, 0.f );

            while (wb->IsInUse())
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            dump_stats( audEngine.get() );
        }

        // WaveBank One-shots w/ pitch
        {
            printf("\nOne-shots w/ pitch\n*");
            wb->Play( 11, 1.f, 0.5f, 0.f );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8, 1.f, -0.5f, 0.f );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8, 1.f, -1.f, 0.f );
            Sleep (1000);
            UPDATE

            printf(".");
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8, 1.f, 1.f, 0.f );

            while (wb->IsInUse())
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            dump_stats( audEngine.get() );
        }

        // WaveBank One-shots w/ pan
        {
            printf("\nOne-shots w/ pan\n*");
            wb->Play( 11, 1.f, 0.f, -1.f );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8, 1.f, 0.f, 1.f );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8, 1.f, 0.f, 1.f );
            Sleep (1000);
            UPDATE

            printf(".");
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8, 1.f, 0.f, 1.f );

            while (wb->IsInUse())
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            dump_stats( audEngine.get() );
        }
    }

    { // ADPCM WaveBank
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"adpcmdroid.xwb");
        printf( "\n\nINFO: Loaded adpcmdroid.xwb\n" );

        if ( ( wb->Find( "Rumble2" ) != -1 )
             || ( wb->Find( "XACTGameGroove3" ) != -1 )
             || ( wb->Find( "droid_destroyed3" ) != -1 ) )
        {
            // adpcmdroid.xwb does not contain friendly entry names
            printf( "ERROR: WaveBank::Find failed\n" );
            return 1;
        }

        char buff[64];
        auto wfx = reinterpret_cast<WAVEFORMATEX*>( &buff );

        size_t effectDur = wb->GetSampleDurationMS(10);

        printf( "\tIndex #10 (%zu bytes, %zu samples, %zu ms)\n",
                wb->GetSampleSizeInBytes( 10 ), wb->GetSampleDuration( 10 ), effectDur );
        dump_wfx( wb->GetFormat( 10, wfx, 64) );

        printf( "\tIndex #11 (%zu bytes, %zu samples, %zu ms)\n",
                wb->GetSampleSizeInBytes( 11 ), wb->GetSampleDuration( 11 ), wb->GetSampleDurationMS( 11 ) );
        dump_wfx( wb->GetFormat( 11, wfx, 64) );

        printf( "\tIndex #8 (%zu bytes, %zu samples, %zu ms)\n",
                wb->GetSampleSizeInBytes( 8 ), wb->GetSampleDuration( 8 ), wb->GetSampleDurationMS( 8 ) );
        dump_wfx( wb->GetFormat( 8, wfx, 64) );

        {
            auto bankEffect = wb->CreateInstance( 10 );
            if ( !bankEffect )
            {
                printf("ERROR: Failed to create effect instance\n");
                return 1;
            }

            printf("\nPlaying sound effect instance...\n" );

            ULONGLONG startTick = GetTickCount64();

            bankEffect->Play();

            while ( bankEffect->GetState() == PLAYING )
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            ULONGLONG dur = GetTickCount64() - startTick;

            if (dur < effectDur)
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
                return 1;
            }
            else if (dur >(effectDur + 1000))
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
                return 1;
            }

            dump_stats( audEngine.get() );
        }

        // WaveBank One-shots
        {
            printf("\nOne-shots\n*");
            wb->Play( 11 );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8 );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8 );
            Sleep (1000);
            UPDATE

            printf(".");
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8 );

            while (wb->IsInUse())
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            dump_stats( audEngine.get() );
        }
    }

#ifdef USING_XAUDIO2_9

    { // xWMV WaveBank
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"xwmadroid.xwb");
        printf( "\n\nINFO: Loaded xwmadroid.xwb\n" );

        char buff[64];
        auto wfx = reinterpret_cast<WAVEFORMATEX*>( &buff );

        size_t effectDur = wb->GetSampleDurationMS(10);

        printf( "\tIndex #10 (%zu bytes, %zu samples, %zu ms)\n",
                wb->GetSampleSizeInBytes( 10 ), wb->GetSampleDuration( 10 ), effectDur );
        dump_wfx( wb->GetFormat( 10, wfx, 64) );

        printf( "\tIndex #11 (%zu bytes, %zu samples, %zu ms)\n",
                wb->GetSampleSizeInBytes( 11 ), wb->GetSampleDuration( 11 ), wb->GetSampleDurationMS( 11 ) );
        dump_wfx( wb->GetFormat( 11, wfx, 64) );

        printf( "\tIndex #8 (%zu bytes, %zu samples, %zu ms)\n",
                wb->GetSampleSizeInBytes( 8 ), wb->GetSampleDuration( 8 ), wb->GetSampleDurationMS( 8 ) );
        dump_wfx( wb->GetFormat( 8, wfx, 64) );

        {
            auto bankEffect = wb->CreateInstance( 10 );
            if ( !bankEffect )
            {
                printf("ERROR: Failed to create effect instance\n");
                return 1;
            }

            printf("\nPlaying sound effect instance...\n" );

            ULONGLONG startTick = GetTickCount64();

            bankEffect->Play();

            while ( bankEffect->GetState() == PLAYING )
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            ULONGLONG dur = GetTickCount64() - startTick;

            if ( dur < effectDur )
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly short (%zu)\n", dur, effectDur);
                return 1;
            }
            else if ( dur > ( effectDur + 1000 ) )
            {
                printf("\nERROR: Play() time (%llu) was unexpectedly long (%zu)\n", dur, effectDur);
                return 1;
            }

            dump_stats( audEngine.get() );
        }

        // WaveBank One-shots
        {
            printf("\nOne-shots\n*");
            wb->Play( 11 );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8 );
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8 );
            Sleep (1000);
            UPDATE

            printf(".");
            Sleep (1000);
            UPDATE

            printf(" *");
            wb->Play( 8 );

            while (wb->IsInUse())
            {
                UPDATE

                printf(".");
                Sleep(1000);
            }

            dump_stats( audEngine.get() );
        }
    }

#endif // xWMA

#endif // TEST_WAVEBANK

    UPDATE

    // TrimVoicePool test
    printf(" \nFinal trim voice pool...\n" );
    audEngine->TrimVoicePool();

    dump_stats( audEngine.get() );

    auto stats = audEngine->GetStatistics();

    if ( stats.allocatedVoices != 0
            || stats.allocatedInstances != 0
            || stats.allocatedVoicesOneShot != 0
            || stats.allocatedVoicesIdle != 0 )
    {
        printf( "\nERROR: TrimVoicePool() failed\n" );
        return 1;
    }


    printf( "\nDONE\n");

    //
    // Forced Shutdown
    //

#ifdef TEST_SHUTDOWN

    {
        auto music = std::make_unique<SoundEffect>(audEngine.get(), L"MusicMono.wav");
        auto alarm = std::make_unique<SoundEffect>(audEngine.get(), L"Alarm01.wav");
        auto wb = std::make_unique<WaveBank>(audEngine.get(), L"droid.xwb");

        printf("Force shutdown test...\n");
        auto loop = music->CreateInstance();

        loop->Play( true );

        printf(" *");
        alarm->Play();
        wb->Play( 8 );
        Sleep(500);

        printf(" *");
        alarm->Play();
        wb->Play( 8 );
        Sleep(500);

        printf(" *");
        alarm->Play();
        wb->Play( 8 );

        printf(".");
        Sleep (1000);
        UPDATE
   
        audEngine.reset();
    }

#endif // TEST_SHUTDOWN

    //
    // Cleanup Audio
    //
    audEngine.reset();
    CoUninitialize();

    return 0; 
}
