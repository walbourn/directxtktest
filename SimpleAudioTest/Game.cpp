//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK - Simple Audio Test (UI-based BasicAudioTest)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"

//#define GAMMA_CORRECT_RENDERING
//#define USE_FAST_SEMANTICS

// Test Advanced Format (4Kn) streaming wave banks vs. DVD (2048) sector aligned
#define TEST_4KN

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;


namespace
{
    constexpr unsigned int WB_INMEMORY_ENTRY = 8;
    constexpr unsigned int WB_STREAM_ENTRY = 1;

    const wchar_t* STREAM_NAMES[] =
    {
        L"ADPCM",
#ifdef TEST_XWMA
        L"xWMA",
#endif
#ifdef TEST_XMA2
        L"XMA2",
#endif
    };
}

#pragma warning(push)
#pragma warning(disable : 4100)

SoundStreamInstance* Game::GetCurrentStream(unsigned int index)
{
    if (index >= std::size(STREAM_NAMES))
        return nullptr;

#ifdef TEST_XWMA
    if (index == 1)
    {
        if (!m_streamXWMA)
        {
            m_streamXWMA = m_wbstreamXWMA->CreateStreamInstance(WB_STREAM_ENTRY);
        }
        return m_streamXWMA.get();
    }
    else
#endif
#ifdef TEST_XMA2
    if (index == 2)
    {
        if (!m_streamXMA)
        {
            m_streamXMA = m_wbstreamXMA->CreateStreamInstance(WB_STREAM_ENTRY);
        }
        return m_streamXMA.get();
    }
    else
#endif
    if (!m_streamADPCM)
    {
        m_streamADPCM = m_wbstreamADPCM->CreateStreamInstance(WB_STREAM_ENTRY);
    }
    return m_streamADPCM.get();
}

#pragma warning(pop)

namespace
{
    void SetDeviceString(_In_ AudioEngine* engine, _Out_writes_(maxsize) wchar_t* deviceStr, size_t maxsize)
    {
        if (engine->IsAudioDevicePresent())
        {
            auto wfx = engine->GetOutputFormat();

            const char* speakerConfig;
            switch (wfx.dwChannelMask)
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

            swprintf_s(deviceStr, maxsize, L"Output format rate %u, channels %u, %hs (%08X)", wfx.Format.nSamplesPerSec, wfx.Format.nChannels, speakerConfig, wfx.dwChannelMask);
        }
        else
        {
            wcscpy_s(deviceStr, maxsize, L"No default audio device found, running in 'silent mode'");
        }
    }

    const wchar_t* GetFormatTagName(DWORD tag)
    {
        switch (tag)
        {
        case WAVE_FORMAT_PCM: return L"PCMi";
        case WAVE_FORMAT_IEEE_FLOAT: return L"PCMf";
        case WAVE_FORMAT_ADPCM: return L"ADPCM";
        case WAVE_FORMAT_WMAUDIO2: return L"WMAUDIO2";
        case WAVE_FORMAT_WMAUDIO3: return L"WMAUDIO3";
        case 0x166 /* WAVE_FORMAT_XMA2 */: return L"XMA2";
        default: return L"*Unknown*"; break;
        }
    }

    void dump_wfx(DX::TextConsole* console, const WAVEFORMATEX *wfx)
    {
        if (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            if (wfx->cbSize < (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
            {
                console->Format(L"    EXTENSIBLE, %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                    wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                console->Format(L"    ERROR: Invalid WAVE_FORMAT_EXTENSIBLE\n");
            }
            else
            {
                static const GUID s_wfexBase = { 0x00000000, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

                auto wfex = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wfx);

                if (memcmp(reinterpret_cast<const BYTE*>(&wfex->SubFormat) + sizeof(DWORD),
                    reinterpret_cast<const BYTE*>(&s_wfexBase) + sizeof(DWORD), sizeof(GUID) - sizeof(DWORD)) != 0)
                {
                    console->Format(L"    EXTENSIBLE, %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                        wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                    console->Format(L"    ERROR: Unknown EXTENSIBLE SubFormat {%8.8lX-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}\n",
                        wfex->SubFormat.Data1, wfex->SubFormat.Data2, wfex->SubFormat.Data3,
                        wfex->SubFormat.Data4[0], wfex->SubFormat.Data4[1], wfex->SubFormat.Data4[2], wfex->SubFormat.Data4[3],
                        wfex->SubFormat.Data4[4], wfex->SubFormat.Data4[5], wfex->SubFormat.Data4[6], wfex->SubFormat.Data4[7]);
                }
                else
                {
                    console->Format(L"    EXTENSIBLE %ls (%u), %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                        GetFormatTagName(wfex->SubFormat.Data1), wfex->SubFormat.Data1,
                        wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
                    console->Format(L"        %u samples per block, %u valid bps, %u channel mask",
                        wfex->Samples.wSamplesPerBlock, wfex->Samples.wValidBitsPerSample, wfex->dwChannelMask);
                }
            }
        }
        else
        {
            console->Format(L"    %ls (%u), %u channels, %u-bit, %u Hz, %u align, %u avg\n",
                GetFormatTagName(wfx->wFormatTag), wfx->wFormatTag,
                wfx->nChannels, wfx->wBitsPerSample, wfx->nSamplesPerSec, wfx->nBlockAlign, wfx->nAvgBytesPerSec);
        }
    }
}

//--------------------------------------------------------------------------------------

static_assert(std::is_nothrow_move_constructible<AudioEngine>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<AudioEngine>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<WaveBank>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<WaveBank>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<SoundEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SoundEffect>::value, "Move Assign.");

// VS 2017 on XDK incorrectly thinks it's not noexcept
static_assert(std::is_nothrow_move_constructible<AudioListener>::value, "Move Ctor.");
static_assert(std::is_move_assignable<AudioListener>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<AudioEmitter>::value, "Move Ctor.");
static_assert(std::is_move_assignable<AudioEmitter>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<SoundEffectInstance>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SoundEffectInstance>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<SoundStreamInstance>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SoundStreamInstance>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<DynamicSoundEffectInstance>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DynamicSoundEffectInstance>::value, "Move Assign.");

//--------------------------------------------------------------------------------------

// Constructor.
Game::Game() noexcept(false) :
    m_currentStream(0),
    m_critError(false),
    m_retrydefault(false),
    m_newAudio(false),
    m_deviceStr{},
    m_gamepadPresent(false)
{
#ifdef GAMMA_CORRECT_RENDERING
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif

    // 2D only rendering
#ifdef XBOX
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_UNKNOWN, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(UWP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_UNKNOWN, 2, D3D_FEATURE_LEVEL_9_3,
        DX::DeviceResources::c_Enable4K_Xbox
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(c_RenderFormat, DXGI_FORMAT_UNKNOWN);
#endif

#ifdef LOSTDEVICE
    m_deviceResources->RegisterDeviceNotify(this);
#endif

    m_console = std::make_unique<DX::TextConsole>();
}

Game::~Game()
{
    m_streamADPCM.reset();

#ifdef TEST_XWMA
    m_streamXWMA.reset();
#endif

#ifdef TEST_XMA2
    m_streamXMA.reset();
#endif

    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(
#ifdef COREWINDOW
    IUnknown* window,
#else
    HWND window,
#endif
    int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();

#ifdef XBOX
    UNREFERENCED_PARAMETER(rotation);
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    m_deviceResources->SetWindow(window);
#ifdef COREWINDOW
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#endif
#elif defined(UWP)
    m_deviceResources->SetWindow(window, width, height, rotation);
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#else
    UNREFERENCED_PARAMETER(rotation);
    m_deviceResources->SetWindow(window, width, height);
#endif

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Enumerate devices
    {
        auto enumList = AudioEngine::GetRendererDetails();

        if (enumList.empty())
        {
            OutputDebugStringA("\nERROR: Audio device enumeration results in no devices\n");
        }
        else
        {
            char buff[1024] = {};
            sprintf_s(buff,"\nINFO: Found %zu audio devices:\n", enumList.size());
            OutputDebugStringA(buff);
            for (const auto& it : enumList)
            {
                sprintf_s(buff, "\t\"%ls\"\n", it.description.c_str());
                OutputDebugStringA(buff);
            }
        }
    }

    // Initialize audio
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;

#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    eflags = eflags | AudioEngine_UseMasteringLimiter;

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    SetDeviceString(m_audEngine.get(), m_deviceStr, 256);

#ifdef PC
#define MEDIA_PATH L"..\\BasicAudioTest\\"
#define STREAM_MEDIA_PATH L"..\\StreamingAudioTest\\"
#else
#define MEDIA_PATH
#define STREAM_MEDIA_PATH
#endif

    //--- WAV files ---
    m_alarmPCM = std::make_unique<SoundEffect>(m_audEngine.get(), MEDIA_PATH L"Alarm01.wav");
    m_console->Write(L"Alarm01.wav:       ");
    dump_wfx(m_console.get(), m_alarmPCM->GetFormat());

    m_tadaPCM = std::make_unique<SoundEffect>(m_audEngine.get(), L"tada.wav");
    m_console->Write(L"Tada.wav:          ");
    dump_wfx(m_console.get(), m_tadaPCM->GetFormat());

    m_alarmADPCM = std::make_unique<SoundEffect>(m_audEngine.get(), MEDIA_PATH L"Alarm01_adpcm.wav");
    m_console->Write(L"Alarm01_adpcm.wav: ");
    dump_wfx(m_console.get(), m_alarmADPCM->GetFormat());

    m_alarmFLOAT = std::make_unique<SoundEffect>(m_audEngine.get(), MEDIA_PATH L"Alarm01_float.wav");
    m_console->Write(L"Alarm01_float.wav: ");
    dump_wfx(m_console.get(), m_alarmFLOAT->GetFormat());

#ifdef TEST_XWMA
    m_alarmXWMA = std::make_unique<SoundEffect>(m_audEngine.get(), MEDIA_PATH L"Alarm01_xwma.wav");
    m_console->Write(L"Alarm01_xwma.wav:  ");
    dump_wfx(m_console.get(), m_alarmXWMA->GetFormat());
#endif

#ifdef TEST_XMA2
    m_alarmXMA = std::make_unique<SoundEffect>(m_audEngine.get(), L"Alarm01_xma.wav");
    m_console->Write(L"Alarm01_xma.wav:   ");
    dump_wfx(m_console.get(), m_alarmXMA->GetFormat());
#endif

    //--- XWB Wave Banks ---
    m_wbPCM = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"droid.xwb");
    m_console->WriteLine(L"droid.xwb");
    m_console->Format(L"    Index #%u (%zu bytes, %zu samples, %zu ms)\n",
        WB_INMEMORY_ENTRY,
        m_wbPCM->GetSampleSizeInBytes(WB_INMEMORY_ENTRY),
        m_wbPCM->GetSampleDuration(WB_INMEMORY_ENTRY),
        m_wbPCM->GetSampleDurationMS(WB_INMEMORY_ENTRY));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbPCM->GetFormat(WB_INMEMORY_ENTRY, wfx, 64));
    }

    m_wbADPCM = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"ADPCMdroid.xwb");
    m_console->WriteLine(L"ADPCMdroid.xwb");
    m_console->Format(L"    Index #%u (%zu bytes, %zu samples, %zu ms)\n",
        WB_INMEMORY_ENTRY,
        m_wbADPCM->GetSampleSizeInBytes(WB_INMEMORY_ENTRY),
        m_wbADPCM->GetSampleDuration(WB_INMEMORY_ENTRY),
        m_wbADPCM->GetSampleDurationMS(WB_INMEMORY_ENTRY));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbADPCM->GetFormat(WB_INMEMORY_ENTRY, wfx, 64));
    }

#ifdef TEST_4KN
    m_wbstreamADPCM = std::make_unique<WaveBank>(m_audEngine.get(), STREAM_MEDIA_PATH L"WaveBankADPCM4Kn.xwb");
    m_console->WriteLine(L"WaveBankADPCM4Kn.xwb");
#else
    m_wbstreamADPCM = std::make_unique<WaveBank>(m_audEngine.get(), STREAM_MEDIA_PATH L"WaveBankADPCM.xwb");
    m_console->WriteLine(L"WaveBankADPCM.xwb");
#endif
    m_console->Format(L"    Index #%u (%zu bytes, %zu samples, %zu ms)\n",
        WB_STREAM_ENTRY,
        m_wbstreamADPCM->GetSampleSizeInBytes(WB_STREAM_ENTRY),
        m_wbstreamADPCM->GetSampleDuration(WB_STREAM_ENTRY),
        m_wbstreamADPCM->GetSampleDurationMS(WB_STREAM_ENTRY));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbstreamADPCM->GetFormat(WB_STREAM_ENTRY, wfx, 64));
    }

#ifdef TEST_XWMA
    m_wbXWMA = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"xwmadroid.xwb");
    m_console->WriteLine(L"xwmadroid.xwb");
    m_console->Format(L"    Index #%u (%zu bytes, %zu samples, %zu ms)\n",
        WB_INMEMORY_ENTRY,
        m_wbXWMA->GetSampleSizeInBytes(WB_INMEMORY_ENTRY),
        m_wbXWMA->GetSampleDuration(WB_INMEMORY_ENTRY),
        m_wbXWMA->GetSampleDurationMS(WB_INMEMORY_ENTRY));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbXWMA->GetFormat(WB_INMEMORY_ENTRY, wfx, 64));
    }

#ifdef TEST_4KN
    m_wbstreamXWMA = std::make_unique<WaveBank>(m_audEngine.get(), STREAM_MEDIA_PATH L"WaveBankxWMA4Kn.xwb");
    m_console->WriteLine(L"WaveBankxWMA4Kn.xwb");
#else
    m_wbstreamXWMA = std::make_unique<WaveBank>(m_audEngine.get(), STREAM_MEDIA_PATH L"WaveBankxWMA.xwb");
    m_console->WriteLine(L"WaveBankxWMA.xwb");
#endif
    m_console->Format(L"    Index #%u (%zu bytes, %zu samples, %zu ms)\n",
        WB_STREAM_ENTRY,
        m_wbstreamXWMA->GetSampleSizeInBytes(WB_STREAM_ENTRY),
        m_wbstreamXWMA->GetSampleDuration(WB_STREAM_ENTRY),
        m_wbstreamXWMA->GetSampleDurationMS(WB_STREAM_ENTRY));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbstreamXWMA->GetFormat(WB_STREAM_ENTRY, wfx, 64));
    }
#endif // xWMA

#ifdef TEST_XMA2
    m_wbXMA = std::make_unique<WaveBank>(m_audEngine.get(), L"xmadroid.xwb");
    m_console->WriteLine(L"xmadroid.xwb");
    m_console->Format(L"    Index #%u (%zu bytes, %zu samples, %zu ms)\n",
        WB_INMEMORY_ENTRY,
        m_wbXMA->GetSampleSizeInBytes(WB_INMEMORY_ENTRY),
        m_wbXMA->GetSampleDuration(WB_INMEMORY_ENTRY),
        m_wbXMA->GetSampleDurationMS(WB_INMEMORY_ENTRY));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbXMA->GetFormat(WB_INMEMORY_ENTRY, wfx, 64));
    }

#ifdef TEST_4KN
    m_wbstreamXMA = std::make_unique<WaveBank>(m_audEngine.get(), L"WaveBankXMA2_4Kn.xwb");
    m_console->WriteLine(L"WaveBankXMA2_4Kn.xwb");
#else
    m_wbstreamXMA = std::make_unique<WaveBank>(m_audEngine.get(), L"WaveBankXMA2.xwb");
    m_console->WriteLine(L"WaveBankXMA2.xwb");
#endif
    m_console->Format(L"    Index #%u (%zu bytes, %zu samples, %zu ms)\n",
        WB_STREAM_ENTRY,
        m_wbstreamXMA->GetSampleSizeInBytes(WB_STREAM_ENTRY),
        m_wbstreamXMA->GetSampleDuration(WB_STREAM_ENTRY),
        m_wbstreamXMA->GetSampleDurationMS(WB_STREAM_ENTRY));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbstreamXMA->GetFormat(WB_STREAM_ENTRY, wfx, 64));
    }
#endif // XMA2
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    AudioRender();

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const&)
{
    auto pad = m_gamePad->GetState(0);

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);
    m_gamepadPresent = pad.IsConnected();
    if (m_gamepadPresent)
    {
        m_gamepadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitGame();
        }

        using ButtonState = GamePad::ButtonStateTracker::ButtonState;

        if (m_gamepadButtons.a == ButtonState::PRESSED)
        {
            if (pad.IsLeftTriggerPressed())
            {
                m_console->WriteLine(L"PCM tada started");
                m_tadaPCM->Play();
            }
            else
            {
                m_console->WriteLine(L"PCM alarm started");
                m_alarmPCM->Play();
            }
        }
        if (m_gamepadButtons.x == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"ADPCM alarm started");
            m_alarmADPCM->Play();
        }
        if (m_gamepadButtons.y == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"FLOAT32 alarm started");
            m_alarmFLOAT->Play();
        }

        if (m_gamepadButtons.dpadDown == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"PCM Wavebank started");
            m_wbPCM->Play(WB_INMEMORY_ENTRY);
        }
        if (m_gamepadButtons.dpadLeft == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"ADPCM Wavebank started");
            m_wbADPCM->Play(WB_INMEMORY_ENTRY);
        }

#ifdef TEST_XWMA
        if (m_gamepadButtons.b == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"xWMA alarm started");
            m_alarmXWMA->Play();
        }
        if (m_gamepadButtons.dpadRight == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"xWMA Wavebank started");
            m_wbXWMA->Play(WB_INMEMORY_ENTRY);
        }
#endif

#ifdef TEST_XMA2
        if (m_gamepadButtons.rightStick == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"XMA2 alarm started");
            m_alarmXMA->Play();
        }
        if (m_gamepadButtons.dpadUp == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"XMA2 Wavebank started");
            m_wbXMA->Play(WB_INMEMORY_ENTRY);
        }
#endif

        if (m_gamepadButtons.leftShoulder == ButtonState::PRESSED || m_gamepadButtons.rightShoulder == ButtonState::PRESSED)
        {
            UpdateCurrentStream(m_gamepadButtons.leftShoulder == ButtonState::PRESSED);
        }

        if (m_gamepadButtons.leftTrigger == ButtonState::PRESSED || m_gamepadButtons.rightTrigger == ButtonState::PRESSED)
        {
            CycleCurrentStream(m_gamepadButtons.rightTrigger == ButtonState::PRESSED);
        }

        if (m_gamepadButtons.menu == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"Voice pool trimmed");
            m_audEngine->TrimVoicePool();
        }
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::D1))
    {
        m_console->WriteLine(L"PCM alarm started");
        m_alarmPCM->Play();
    }
    if (m_keyboardButtons.IsKeyPressed(Keyboard::D3))
    {
        m_console->WriteLine(L"ADPCM alarm started");
        m_alarmADPCM->Play();
    }
    if (m_keyboardButtons.IsKeyPressed(Keyboard::D4))
    {
        m_console->WriteLine(L"FLOAT32 alarm started");
        m_alarmFLOAT->Play();
    }
    if (m_keyboardButtons.IsKeyPressed(Keyboard::D5))
    {
        m_console->WriteLine(L"PCM tada started");
        m_tadaPCM->Play();
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Q))
    {
        m_console->WriteLine(L"PCM Wavebank started");
        m_wbPCM->Play(WB_INMEMORY_ENTRY);
    }
    if (m_keyboardButtons.IsKeyPressed(Keyboard::W))
    {
        m_console->WriteLine(L"ADPCM Wavebank started");
        m_wbADPCM->Play(WB_INMEMORY_ENTRY);
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::A) || m_keyboardButtons.IsKeyPressed(Keyboard::S))
    {
        UpdateCurrentStream(m_keyboardButtons.IsKeyPressed(Keyboard::A));
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::OemOpenBrackets) || m_keyboardButtons.IsKeyReleased(Keyboard::OemCloseBrackets))
    {
        CycleCurrentStream(m_keyboardButtons.IsKeyReleased(Keyboard::OemCloseBrackets));
    }

#ifdef TEST_XWMA
    if (m_keyboardButtons.IsKeyPressed(Keyboard::D2))
    {
        m_console->WriteLine(L"xWMA alarm started");
        m_alarmXWMA->Play();
    }
    if (m_keyboardButtons.IsKeyPressed(Keyboard::E))
    {
        m_console->WriteLine(L"xWMA Wavebank started");
        m_wbXWMA->Play(WB_INMEMORY_ENTRY);
    }
#endif

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
    {
        m_console->WriteLine(L"Voice pool trimmed");
        m_audEngine->TrimVoicePool();
    }

    if (kb.Escape)
    {
        ExitGame();
    }
}

void Game::UpdateCurrentStream(bool isplay)
{
    auto stream = GetCurrentStream(m_currentStream);
    if (stream)
    {
        if (isplay)
        {
            if (stream->GetState() == SoundState::PLAYING)
            {
                wchar_t buff[64] = {};
                swprintf_s(buff, L"%ls streaming wavebank already playing", STREAM_NAMES[m_currentStream]);
                m_console->WriteLine(buff);
            }
            else
            {
                wchar_t buff[64] = {};
                swprintf_s(buff, L"%ls streaming wavebank playing", STREAM_NAMES[m_currentStream]);
                m_console->WriteLine(buff);

                stream->Play(true);
            }
        }
        else
        {
            if (stream->GetState() == SoundState::STOPPED)
            {
                wchar_t buff[64] = {};
                swprintf_s(buff, L"%ls streaming wavebank already stopped", STREAM_NAMES[m_currentStream]);
                m_console->WriteLine(buff);
            }
            else
            {
                wchar_t buff[64] = {};
                swprintf_s(buff, L"%ls streaming wavebank stopping", STREAM_NAMES[m_currentStream]);
                m_console->WriteLine(buff);
                stream->Stop();
            }
        }
    }
    else
    {
        wchar_t buff[64] = {};
        swprintf_s(buff, L"%ls streaming wavebank entry not found", STREAM_NAMES[m_currentStream]);
        m_console->WriteLine(buff);
    }
}

void Game::CycleCurrentStream(bool increment)
{
    auto stream = GetCurrentStream(m_currentStream);

    bool wasplaying = false;
    if (stream)
    {
        wasplaying = (stream->GetState() == SoundState::PLAYING);
        stream->Stop();
    }

    constexpr auto nStreams = static_cast<unsigned int>(std::size(STREAM_NAMES));

    if (increment)
    {
        m_currentStream = (m_currentStream + 1) % nStreams;
    }
    else
    {
        m_currentStream = (m_currentStream + nStreams - 1) % nStreams;
    }

    if (wasplaying)
    {
        UpdateCurrentStream(true);
    }
    else
    {
        wchar_t buff[64] = {};
        swprintf_s(buff, L"Current stream is %ls", STREAM_NAMES[m_currentStream]);
        m_console->WriteLine(buff);
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

#ifdef XBOX
    m_deviceResources->Prepare();
#endif

    XMVECTORF32 red, blue;
#ifdef GAMMA_CORRECT_RENDERING
    red.v = XMColorSRGBToRGB(Colors::Red);
    blue.v = XMColorSRGBToRGB(Colors::Blue);
#else
    red.v = Colors::Red;
    blue.v = Colors::Blue;
#endif

    Clear();

    m_console->Render();

    auto stats = m_audEngine->GetStatistics();

    wchar_t statsStr[256] = {};
    swprintf_s(statsStr, L"Playing: %zu / %zu; Instances %zu; Voices %zu / %zu / %zu / %zu; %zu audio bytes; %zu stream bytes",
        stats.playingOneShots, stats.playingInstances,
        stats.allocatedInstances, stats.allocatedVoices, stats.allocatedVoices3d,
        stats.allocatedVoicesOneShot, stats.allocatedVoicesIdle,
        stats.audioBytes, stats.streamingBytes);

    auto size = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    m_spriteBatch->Begin();

    float dy = m_comicFont->GetLineSpacing();
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    m_comicFont->DrawString(m_spriteBatch.get(), m_deviceStr, pos);

    pos.y += dy;

    m_comicFont->DrawString(m_spriteBatch.get(), statsStr, pos);

    if (m_critError)
    {
        pos.y += dy;

        m_comicFont->DrawString(m_spriteBatch.get(), L"ERROR: Critical Error detected", pos, red);
    }

    pos.y = float(safeRect.bottom) - dy * 4;

    const wchar_t* help1 = nullptr;
    const wchar_t* help2 = nullptr;
    const wchar_t* help3 = nullptr;
    const wchar_t* help4 = nullptr;
    const wchar_t* help5 = nullptr;

    if (m_gamepadPresent)
    {
#ifdef TEST_XMA2
        help1 = L"Press A, B, X, Y, or RThumb to trigger Alarm.wav; LTrigger+A for Tada.wav";
#else
        help1 = L"Press A, B, X, or Y to trigger Alarm.wav; LTrigger+A for Tada.wav";
#endif
        help2 = L"Press DPAD to trigger in-memory Wavebank entry";
        help3 = L"Press RSB/LSB to start/stop streaming WaveBank entry; LT/RT to cycle stream bank";
        help4 = L"Press MENU/START to trim voices";
        help5 = L"Press VIEW/BACK to exit";
    }
    else
    {
        help1 = L"Press 1, 2, 3, or 4 to trigger Alarm.wav; 5 for Tada.wav";
        help2 = L"Press Q, W, or E to trigger Wavebank entry";
        help3 = L"Press A/S to start/stop streaming WaveBank entry; [] to cycle stream bank";
        help4 = L"Press Space to trim voices";
        help5 = L"Press Esc to exit";
    }

    m_comicFont->DrawString(m_spriteBatch.get(), help1, pos, blue);
    pos.y += dy;

    m_comicFont->DrawString(m_spriteBatch.get(), help2, pos, blue);
    pos.y += dy;

    m_comicFont->DrawString(m_spriteBatch.get(), help3, pos, blue);
    pos.y += dy;

    m_comicFont->DrawString(m_spriteBatch.get(), help4, pos, blue);
    pos.y += dy;

    m_comicFont->DrawString(m_spriteBatch.get(), help5, pos, blue);

    m_spriteBatch->End();

    // Show the new frame.
    m_deviceResources->Present();

#ifdef XBOX
    m_graphicsMemory->Commit();
#endif
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();

    XMVECTORF32 color;
#ifdef GAMMA_CORRECT_RENDERING
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
#else
    color.v = Colors::CornflowerBlue;
#endif
    context->ClearRenderTargetView(renderTarget, color);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}

void Game::AudioRender()
{
    if (m_retrydefault)
    {
        // We try to reset the audio using the default device
        m_retrydefault = false;

        if (m_audEngine->Reset())
        {
            // It worked, so we reset our sound
            m_critError = false;
            SetDeviceString(m_audEngine.get(), m_deviceStr, 256);
        }
    }

    if (!m_audEngine->Update())
    {
        // We are running in silent mode
        if (m_audEngine->IsCriticalError())
        {
            if (!m_critError)
            {
                // First will retry using the default audio device
                m_retrydefault = true;
                m_critError = true;
            }
        }

        if (m_newAudio)
        {
            // There's new audio and we are in silent mode, so retry default
            m_retrydefault = true;
        }
    }
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
    m_deviceResources->Suspend();

    m_audEngine->Suspend();
}

void Game::OnResuming()
{
    m_deviceResources->Resume();

    m_timer.ResetElapsedTime();
    m_gamepadButtons.Reset();
    m_keyboardButtons.Reset();
    m_audEngine->Resume();
}

#ifdef PC
void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

#ifndef XBOX
void Game::OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation)
{
#ifdef UWP
    if (!m_deviceResources->WindowSizeChanged(width, height, rotation))
        return;
#else
    UNREFERENCED_PARAMETER(rotation);
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;
#endif

    CreateWindowSizeDependentResources();
}
#endif

#ifdef UWP
void Game::ValidateDevice()
{
    m_deviceResources->ValidateDevice();
}
#endif

void Game::OnAudioDeviceChange()
{
    m_newAudio = true;
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

#ifdef XBOX
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device, m_deviceResources->GetBackBufferCount());
#endif

    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    m_comicFont = std::make_unique<SpriteFont>(device, L"comic.spritefont");

    m_console->RestoreDevice(context, L"courier_16.spritefont");
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);
    m_console->SetViewport(viewport);

    RECT size = m_deviceResources->GetOutputSize();

    RECT safeRect = Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    float dy = m_comicFont->GetLineSpacing();

    safeRect.top += long(dy * 3);
    safeRect.bottom -= long(dy * 4);

    m_console->SetWindow(safeRect);

#ifdef UWP
    m_spriteBatch->SetRotation(m_deviceResources->GetRotation());
#endif
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_spriteBatch.reset();
    m_comicFont.reset();

    m_console->ReleaseDevice();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
