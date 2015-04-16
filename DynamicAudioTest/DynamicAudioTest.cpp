//--------------------------------------------------------------------------------------
// File: DynamicAudioTest.cpp
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

#include "Audio.h"

#include <stdio.h>

using namespace DirectX;

#define TEST_DYNAMICSOUNDEFFECT


//--------------------------------------------------------------------------------------
#pragma warning(push)
#pragma warning( disable: 4005 )
#include <wrl.h>
#pragma warning(pop)

#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
#pragma comment(lib,"mfplat.lib")
#elif (_WIN32_WINNT < _WIN32_WINNT_WIN7 )
#error This code needs _WIN32_WINNT set to 0x0601 or higher. It is compatible with Windows Vista with KB 2117917 installed
#else
// The magic incantation needed to get down-level MF Source Reader support...
#define MF_SDK_VERSION 0x0001  
#define MF_API_VERSION 0x0070 
#define MF_VERSION (MF_SDK_VERSION << 16 | MF_API_VERSION)  
#pragma comment(lib,"mfplat_vista.lib")
#endif

#include <initguid.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>

#pragma comment(lib,"mfreadwrite.lib")


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


//--------------------------------------------------------------------------------------
const char* GetFormatTagName( DWORD tag )
{
    switch( tag )
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

void dump_wfx( const WAVEFORMATEX *wfx )
{
    if ( wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE )
    {
        if ( wfx->cbSize < ( sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) ) )
        {
            printf( "\tEXTENSIBLE, %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                    wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec );
            printf( "\tERROR: Invalid WAVE_FORMAT_EXTENSIBLE\n" );
        }
        else
        {
            static const GUID s_wfexBase = {0x00000000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};

            auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>( wfx );

            if ( memcmp( reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                         reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD) ) != 0 )
            {
                printf( "\tEXTENSIBLE, %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                        wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec );
                printf( "\tERROR: Unknown EXTENSIBLE SubFormat {%8.8lX-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}\n",
                        wfex->SubFormat.Data1, wfex->SubFormat.Data2, wfex->SubFormat.Data3,
                        wfex->SubFormat.Data4[0], wfex->SubFormat.Data4[1], wfex->SubFormat.Data4[2], wfex->SubFormat.Data4[3], 
                        wfex->SubFormat.Data4[4], wfex->SubFormat.Data4[5], wfex->SubFormat.Data4[6], wfex->SubFormat.Data4[7] );
            }
            else
            {
                printf( "\tEXTENSIBLE %s (%u), %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                        GetFormatTagName( wfex->SubFormat.Data1 ), wfex->SubFormat.Data1, 
                        wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec );
                printf( "\t\t%u samples per block, %u valid bps, %u channel mask",
                        wfex->Samples.wSamplesPerBlock, wfex->Samples.wValidBitsPerSample, wfex->dwChannelMask );
            }
        }
    }
    else
    {
        printf( "\t%s (%u), %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                GetFormatTagName( wfx->wFormatTag ), wfx->wFormatTag,
                wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec );
    }
}


//--------------------------------------------------------------------------------------
void GenerateSineWave( _Out_writes_(sampleRate) int16_t* data, int sampleRate, int frequency )
{
    const double timeStep = 1.0 / double(sampleRate);
    const double freq = double(frequency);

    int16_t* ptr = data;
    double time = 0.0;
    for( int j = 0; j < sampleRate; ++j, ++ptr )
    {
        double angle = ( 2.0 * XM_PI * freq ) * time;
        double factor = 0.5 * ( sin(angle) + 1.0 );
        *ptr = int16_t( 32768 * factor );
        time += timeStep;
    }
}


//--------------------------------------------------------------------------------------
HRESULT CreateMFReader(_In_z_ const WCHAR* mediaFile, _Outptr_ IMFSourceReader ** reader, _Out_ WAVEFORMATEX* wfx, _In_ size_t maxwfx)
{
    using namespace Microsoft::WRL;

    if (!mediaFile || !reader || !wfx)
        return E_INVALIDARG;

    HRESULT hr;
    ComPtr<IMFAttributes> attr;
#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
    hr = MFCreateAttributes(attr.GetAddressOf(), 2);
    if (FAILED(hr))
        return hr;

    hr = attr->SetUINT32(MF_LOW_LATENCY, TRUE);
#else
    hr = MFCreateAttributes(attr.GetAddressOf(), 1);
#endif
    if (FAILED(hr))
        return hr;

    hr = MFCreateSourceReaderFromURL(mediaFile, attr.Get(), reader);
    if (FAILED(hr))
        return hr;

    //
    // Make the output from Media Foundation PCM so XAudio2 can consume it
    //

    ComPtr<IMFMediaType> mediaType;
    hr = MFCreateMediaType(mediaType.GetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (FAILED(hr))
        return hr;

    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    if (FAILED(hr))
        return hr;

    hr = (*reader)->SetCurrentMediaType( DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, mediaType.Get());
    if (FAILED(hr))
        return hr;

    //
    // Get the wave format
    //

    ComPtr<IMFMediaType> outputMediaType;
    hr = (*reader)->GetCurrentMediaType( DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), outputMediaType.GetAddressOf());
    if (FAILED(hr))
        return hr;

    UINT32 waveFormatSize = 0;
    WAVEFORMATEX* waveFormat = nullptr;
    hr = MFCreateWaveFormatExFromMFMediaType(outputMediaType.Get(), &waveFormat, &waveFormatSize);
    if (FAILED(hr))
        return hr;

    memcpy_s(wfx, maxwfx, waveFormat, waveFormatSize);
    CoTaskMemFree(waveFormat);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
int __cdecl main()
{
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

    std::unique_ptr<AudioEngine> audEngine( new AudioEngine( eflags ) );

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

    //
    // DynamicSoundEffectInstance constructor
    //

#ifdef TEST_DYNAMICSOUNDEFFECT

    printf("\nCreating dynamic sound effect instance in various formats\n");

    const size_t FMT_CASES = 6;

    for( size_t j = 0; j < FMT_CASES; ++j )
    {
        const static uint32_t s_rate[FMT_CASES] = { 44100, 22050, 22050, 48000, XAUDIO2_MIN_SAMPLE_RATE, XAUDIO2_MAX_SAMPLE_RATE };
        const static uint32_t s_channels[FMT_CASES] = { 1, 2, 6, 4, 1, 8 };
        const static uint32_t s_bits[FMT_CASES] = { 16, 8, 16, 8, 8, 16 };

        std::unique_ptr<DynamicSoundEffectInstance> effect( new DynamicSoundEffectInstance( audEngine.get(),
                    nullptr, s_rate[j], s_channels[j], s_bits[j] ) );

        auto wfx = effect->GetFormat();
        dump_wfx(wfx);
        if ( wfx->wFormatTag != WAVE_FORMAT_PCM
             || wfx->cbSize != 0
             || wfx->nChannels != s_channels[j]
             || wfx->wBitsPerSample != s_bits[j]
             || wfx->nSamplesPerSec != s_rate[j]
             || wfx->nBlockAlign != ( ( s_bits[j] * s_channels[j] ) / 8 )
             || wfx->nAvgBytesPerSec != ( s_rate[j] * wfx->nBlockAlign ) )
        {
            printf("ERROR: GetFormat failed\n" );
            return 1;
        }

        // Force voice allocation by playing...
        effect->Play();

        if ( effect->GetState() != PLAYING )
        {
            printf("ERROR: Play failed to put it into PLAYING state\n");
            return 1;
        }

        if ( effect->GetPendingBufferCount() > 0 )
        {
            printf("ERROR: Invalid pending buffer count\n");
            return 1;
        }

        if ( effect->GetSampleSizeInBytes( 1000 ) != wfx->nAvgBytesPerSec )
        {
            printf("ERROR: GetSampleSizeInBytes failed\n" );
            return 1;
        }

        if ( effect->GetSampleDuration( wfx->nAvgBytesPerSec ) !=  wfx->nSamplesPerSec )
        {
            printf("ERROR: GetSampleDuration failed\n" );
            return 1;
        }

        if ( effect->GetSampleDurationMS( wfx->nAvgBytesPerSec ) != 1000 )
        {
            printf("ERROR: GetSampleDurationMS failed\n" );
            return 1;
        }
    }

    printf("\tPASS\n");

    //
    // Event test
    //

    printf( "\n\nEvent test\n" );

    {
        uint32_t buffNeededCount = 0;

        std::unique_ptr<DynamicSoundEffectInstance> effect( new DynamicSoundEffectInstance( audEngine.get(),
                        [&buffNeededCount](DynamicSoundEffectInstance*){ InterlockedIncrement( &buffNeededCount ); }, 44100, 1, 16 ) );

        if ( buffNeededCount > 0 )
        {
            printf("ERROR: Unexpected call to bufferNeeded event\n");
            return 1;
        }

        UPDATE

        if ( buffNeededCount > 0 )
        {
            printf("ERROR: Unexpected call to bufferNeeded event\n");
            return 1;
        }

        effect->Play();

        if ( effect->GetState() != PLAYING )
        {
            printf("ERROR: Play failed to put it into PLAYING state\n");
            return 1;
        }

        UPDATE

        if ( buffNeededCount < 1 )
        {
            printf("ERROR: Missing call to bufferNeeded event\n");
            return 1;
        }
    }

    printf("\tPASS\n");

    //
    // Procedural sound test
    //

    printf( "\n\nProcedural sound test\n" );

    {
        std::vector<uint8_t> audioBytes;
        audioBytes.resize( 44100 * 2 );

        GenerateSineWave( reinterpret_cast<int16_t*>( &audioBytes.front() ), 44100, 440 );

        uint32_t buffNeededCount = 0;

        std::unique_ptr<DynamicSoundEffectInstance> effect( new DynamicSoundEffectInstance( audEngine.get(),
                        [&audioBytes, &buffNeededCount](DynamicSoundEffectInstance* effect)
                        {
                            InterlockedIncrement( &buffNeededCount );

                            int count = effect->GetPendingBufferCount();

                            while( count < 3 )
                            {
                                effect->SubmitBuffer( &audioBytes.front(), audioBytes.size() );
                                ++count;
                            }

                        }, 44100, 1, 16 ) );

        effect->Play();

        if ( buffNeededCount > 0)
        {
            printf("ERROR: Unexpected call to bufferNeeded event\n");
            return 1;
        }

        UPDATE

        if (buffNeededCount < 1)
        {
            printf("ERROR: Missing call to bufferNeeded event\n");
            return 1;
        }

        if ( effect->GetState() != PLAYING )
        {
            printf("ERROR: Play failed to put it into PLAYING state\n");
            return 1;
        }

        printf("\nRunning for 5 seconds...\n");

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

        if ( buffNeededCount < 3 )
        {
            printf( "ERROR: Event callback was not called enough times (%u)\n", buffNeededCount );
            return 1;
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

        // Volume
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
    }

    printf("\n\tPASS\n");

#endif // TEST_DYNAMICSOUNDEFFECT

    //
    // Streaming implementation test
    //

    printf("\n\nStreaming test\n");

    {
        using namespace Microsoft::WRL;
        #define MAX_BUFFER_COUNT 12

        if ( FAILED( MFStartup(MF_VERSION) ) )
        {
            printf("\nERROR: Failed starting Media Foundation\n");
            return 1;
        }

        ComPtr<IMFSourceReader> reader;
        WAVEFORMATEX wfx;
        if (FAILED(CreateMFReader(L"becky.wma", reader.GetAddressOf(), &wfx, sizeof(wfx))))
        {
            printf("\nERROR: Failed creating MFSourceReader for becky.wma\n");
            return 1;
        }

        PROPVARIANT var;
        HRESULT hr = reader->GetPresentationAttribute( DWORD(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &var);
        uint64_t effectDur = 0;

        if (SUCCEEDED(hr))
        {
            if (var.vt == VT_UI8)
            {
                // MF returns things in 100 ns units...
                effectDur = var.uhVal.QuadPart / 10000;
            }
            PropVariantClear(&var);
        }

        printf("\n\nINFO: becky.wma (%I64u ms)\n", effectDur );
        dump_wfx( &wfx );

        if ( wfx.wFormatTag != WAVE_FORMAT_PCM )
        {
            printf("\nERROR: MFSourceReader not configured to return PCM data\n");
            return 1;
        }

        uint32_t currentStreamBuffer = 0;
        size_t bufferSize[MAX_BUFFER_COUNT] = { 0 };
        std::unique_ptr<uint8_t[]> buffers[MAX_BUFFER_COUNT];

        bool endofstream = false;

        std::unique_ptr<DynamicSoundEffectInstance> effect( new DynamicSoundEffectInstance( audEngine.get(),
            [&reader, &currentStreamBuffer, &bufferSize, &buffers, &endofstream](DynamicSoundEffectInstance* effect)
            {
                using namespace Microsoft::WRL;

                if (endofstream)
                    return;

                while (effect->GetPendingBufferCount() < MAX_BUFFER_COUNT)
                {
                    DWORD dwStreamIndex, dwStreamFlags;
                    LONGLONG llTimestamp;
                    ComPtr<IMFSample> sample;
                    if (FAILED(reader->ReadSample( DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, &dwStreamIndex, &dwStreamFlags, &llTimestamp, sample.GetAddressOf())))
                        throw std::exception("ReadSample");

                    if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
                    {
                        endofstream = true;
                        break;
                    }
                    {
                        ComPtr<IMFMediaBuffer> mediaBuffer;
                        if (FAILED(sample->ConvertToContiguousBuffer(mediaBuffer.GetAddressOf())))
                            throw std::exception("ConvertToContiguousBuffer");

                        BYTE* audioData = nullptr;
                        DWORD sampleBufferLength = 0;

                        if (SUCCEEDED(mediaBuffer->Lock(&audioData, nullptr, &sampleBufferLength)))
                        {
                            if (bufferSize[currentStreamBuffer] < sampleBufferLength)
                            {
                                buffers[currentStreamBuffer].reset(new uint8_t[sampleBufferLength]);
                                bufferSize[currentStreamBuffer] = sampleBufferLength;
                            }

                            memcpy_s(buffers[currentStreamBuffer].get(), sampleBufferLength, audioData, sampleBufferLength);

                            (void) mediaBuffer->Unlock();

                            effect->SubmitBuffer(buffers[currentStreamBuffer].get(), sampleBufferLength);

                            ++currentStreamBuffer;
                            currentStreamBuffer %= MAX_BUFFER_COUNT;
                        }
                        else
                            throw std::exception("Lock");
                    }
                }
            }, wfx.nSamplesPerSec, wfx.nChannels, wfx.wBitsPerSample ) );

        ULONGLONG startTick = GetTickCount64();

        effect->Play();

        while ( effect->GetState() == PLAYING )
        {
            UPDATE

            if ( endofstream && !effect->GetPendingBufferCount() )
                break;

            printf(".");
            Sleep(1000);
        }

        ULONGLONG dur = GetTickCount64() - startTick;

        if (dur < effectDur)
        {
            printf("\nERROR: Play() time (%I64u) was unexpectedly short (%I64u)\n", dur, effectDur);
            return 1;
        }
        else if (dur >(effectDur + 5000))
        {
            printf("\nERROR: Play() time (%I64u) was unexpectedly long (%I64u)\n", dur, effectDur);
            return 1;
        }

        printf("\n\tPASS\n");
    }

    printf( "\nDONE\n");

    //
    // Cleanup Audio
    //
    audEngine.reset();
    CoUninitialize();

    return 0;
}
