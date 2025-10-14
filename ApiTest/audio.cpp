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

    for(size_t index=0; index < std::size(s_waveFiles); ++index)
    {
        std::unique_ptr<SoundEffect> soundEffect;
        try
        {
            soundEffect = std::make_unique<SoundEffect>(audioEngine.get(), s_waveFiles[index]);
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed creating sound effect [%ls] (except: %s)\n", s_waveFiles[index], e.what());
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
                printf("ERROR: Failed creating sound effect instance [%ls] (except: %s)\n", s_waveFiles[index], e.what());
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

    for(size_t index=0; index < std::size(s_bankFiles); ++index)
    {
        std::unique_ptr<WaveBank> waveBank;
        try
        {
            waveBank = std::make_unique<WaveBank>(audioEngine.get(), s_bankFiles[index]);
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed creating wave bank [%ls] (except: %s)\n", s_bankFiles[index], e.what());
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
                printf("ERROR: Failed creating sound effect instance [%ls] (except: %s)\n", s_bankFiles[index], e.what());
                success = false;
            }

            if (!index)
            {
            #pragma warning(push)
            #pragma warning(disable:6385 6387)
                if (waveBank->GetFormat(0, nullptr, 0) != nullptr)
                {
                    printf("ERROR: Expected failure for null parameter for GetFormat\n");
                    success = false;
                }

                WAVEFORMATEX wfx = {};
                if (waveBank->GetFormat(INT32_MAX, &wfx, sizeof(wfx)) != nullptr)
                {
                    printf("ERROR: Expected failure for out of range index for GetFormat\n");
                    success = false;
                }

                if (waveBank->GetPrivateData(0, nullptr, 0))
                {
                    printf("ERROR: Expected failure for null parameter for GetPrivateData\n");
                    success = false;
                }

                uint8_t data[16] = {};
                if (waveBank->GetPrivateData(0, &data, 0))
                {
                    printf("ERROR: Expected failure for zero size for GetPrivateData\n");
                    success = false;
                }

                if (waveBank->GetPrivateData(INT32_MAX, &data, sizeof(data)))
                {
                    printf("ERROR: Expected failure for out of range index for GetPrivateData\n");
                    success = false;
                }
            #pragma warning(pop)
            }
        }
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        // SoundEffect
        try
        {
            auto invalid = std::make_unique<SoundEffect>(nullptr, nullptr);

            printf("ERROR: Failed to throw for null engine for SoundEffect\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<SoundEffect>(audioEngine.get(), nullptr);

            printf("ERROR: Failed to throw for null filename for SoundEffect\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<SoundEffect>(audioEngine.get(), L"TestFileNotExist.wav");

            printf("ERROR: Failed to throw for missing file for SoundEffect\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            std::unique_ptr<uint8_t[]> data;
            auto invalid = std::make_unique<SoundEffect>(nullptr, data, nullptr, nullptr, 0);

            printf("ERROR: Failed to throw for null engine for SoundEffect 2\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            std::unique_ptr<uint8_t[]> data;
            auto invalid = std::make_unique<SoundEffect>(audioEngine.get(), data, nullptr, nullptr, 0);

            printf("ERROR: Failed to throw for null parameters for SoundEffect 2\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            WAVEFORMATEX wfx = {};
            std::unique_ptr<uint8_t[]> data;
            auto invalid = std::make_unique<SoundEffect>(audioEngine.get(), data, &wfx, nullptr, 0);

            printf("ERROR: Failed to throw for null data parameter for SoundEffect 2\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            std::unique_ptr<uint8_t[]> data;
            auto invalid = std::make_unique<SoundEffect>(nullptr, data, nullptr, nullptr, 0, 0, 0);

            printf("ERROR: Failed to throw for null engine for SoundEffect 3\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            std::unique_ptr<uint8_t[]> data;
            auto invalid = std::make_unique<SoundEffect>(audioEngine.get(), data, nullptr, nullptr, 0, 0, 0);

            printf("ERROR: Failed to throw for null parameters for SoundEffect 3\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            WAVEFORMATEX wfx = {};
            std::unique_ptr<uint8_t[]> data;
            auto invalid = std::make_unique<SoundEffect>(audioEngine.get(), data, &wfx, nullptr, 0, 0, 0);

            printf("ERROR: Failed to throw for null data parameter for SoundEffect 3\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        // DynamicSoundEffectInstance
        try
        {
            auto invalid = std::make_unique<DynamicSoundEffectInstance>(nullptr, nullptr, 0, 0, 0);

            printf("ERROR: Failed to throw for null engine for DynamicSoundEffectInstance\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<DynamicSoundEffectInstance>(audioEngine.get(), nullptr, 0, 0, 0);

            printf("ERROR: Failed to throw for out of range sample rate DynamicSoundEffectInstance\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<DynamicSoundEffectInstance>(audioEngine.get(), nullptr, 44100, 0, 0);

            printf("ERROR: Failed to throw for out of range channel count DynamicSoundEffectInstance\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<DynamicSoundEffectInstance>(audioEngine.get(), nullptr, 44100, 1, 0);

            printf("ERROR: Failed to throw for out of range sample rate DynamicSoundEffectInstance\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<DynamicSoundEffectInstance>(audioEngine.get(), nullptr, 44100, 1, 16);

            printf("ERROR: Failed to throw for no callback DynamicSoundEffectInstance\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        // WaveBank
        try
        {
            auto invalid = std::make_unique<WaveBank>(nullptr, nullptr);

            printf("ERROR: Failed to throw for null engine for WaveBank\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<WaveBank>(audioEngine.get(), nullptr);

            printf("ERROR: Failed to throw for null filename for WaveBank\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<WaveBank>(audioEngine.get(), L"TestFileNotExist.xwb");

            printf("ERROR: Failed to throw for missing file for WaveBank\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}
