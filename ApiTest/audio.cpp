//--------------------------------------------------------------------------------------
// File: audio.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "Audio.h"

#include <d3d11.h>

#include <cstdio>
#include <iterator>

using namespace DirectX;

_Success_(return)
bool TestA01(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    // Create audio engine
    bool success = true;

    std::unique_ptr<AudioEngine> audioEngine;
    try
    {
        AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
        #ifdef _DEBUG
        eflags |= AudioEngine_Debug;
        #endif

        audioEngine = std::make_unique<AudioEngine>(eflags);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating audio engine (except: %s)\n", e.what());
        success = false;
    }

    if (!audioEngine)
        return success;

    // SoundEffect
    const static wchar_t* s_waveFiles[] =
    {
        L"BasicAudioTest\\Alarm01.wav",
        L"BasicAudioTest\\Alarm01_adpcm.wav",
        L"BasicAudioTest\\Alarm01_float.wav",

    #ifdef USING_XAUDIO2_9
        L"BasicAudioTest\\Alarm01_xwma.wav",
    #endif
    };

    for(size_t j=0; j < std::size(s_waveFiles); ++j)
    {
        std::unique_ptr<SoundEffect> soundEffect;
        try
        {
            soundEffect = std::make_unique<SoundEffect>(audioEngine.get(), s_waveFiles[j]);
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed creating sound effect [%ls] (except: %s)\n", s_waveFiles[j], e.what());
            success = false;
        }

        if (soundEffect)
        {
            std::unique_ptr<SoundEffectInstance> soundEffectInstance;
            try
            {
                soundEffectInstance = soundEffect->CreateInstance();
            }
            catch(const std::exception& e)
            {
                printf("ERROR: Failed creating sound effect instance [%ls] (except: %s)\n", s_waveFiles[j], e.what());
                success = false;
            }
        }
    }

    // DynamicSoundEffectInstance
    std::unique_ptr<DynamicSoundEffectInstance> effect;
    try
    {
        // PCM 44100 Hz, 16-bit, 1 channel
        effect = std::make_unique<DynamicSoundEffectInstance>( audioEngine.get(),
            [](DynamicSoundEffectInstance*)
            {
                // 'Buffer needed' event handler
            },
            44100, 1 );
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating dynamic sound effect instance (except: %s)\n", e.what());
        success = false;
    }

    // WavBank
    const static wchar_t* s_bankFiles[] =
    {
        L"BasicAudioTest\\droid.xwb",
        L"BasicAudioTest\\ADPCMdroid.xwb",

    #ifdef USING_XAUDIO2_9
        L"BasicAudioTest\\xwmadroid.xwb",
    #endif
    };

    for(size_t j=0; j < std::size(s_bankFiles); ++j)
    {
        std::unique_ptr<WaveBank> waveBank;
        try
        {
            waveBank = std::make_unique<WaveBank>(audioEngine.get(), s_bankFiles[j]);
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed creating wave bank [%ls] (except: %s)\n", s_bankFiles[j], e.what());
            success = false;
        }

        if (waveBank)
        {
            std::unique_ptr<SoundEffectInstance> soundEffectInstance;
            try
            {
                soundEffectInstance = waveBank->CreateInstance(1);
            }
            catch(const std::exception& e)
            {
                printf("ERROR: Failed creating sound effect instance [%ls] (except: %s)\n", s_bankFiles[j], e.what());
                success = false;
            }
        }
    }

    return success;
}
