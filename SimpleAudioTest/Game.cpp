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

#include "FindMedia.h"

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
#ifdef GAMMA_CORRECT_RENDERING
    // Linear colors for DirectXMath were not added until v3.17 in the Windows SDK (22621)
    const XMVECTORF32 c_clearColor = { { { 0.127437726f, 0.300543845f, 0.846873462f, 1.f } } };
#else
    const XMVECTORF32 c_clearColor = Colors::CornflowerBlue;
#endif

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

            swprintf_s(deviceStr, maxsize, L"Output format rate %u, %u-bit, channels %u, %hs (%08X)",
                wfx.Format.nSamplesPerSec, wfx.Format.wBitsPerSample, wfx.Format.nChannels,
                speakerConfig, wfx.dwChannelMask);
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
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
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
        DX::DeviceResources::c_EnableQHD_Xbox
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

    static const wchar_t* s_searchFolders[] =
    {
        L"BasicAudioTest",
        L"StreamingAudioTest",
        nullptr
    };

    //--- WAV files ---
    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"Alarm01.wav", s_searchFolders);
    m_alarmPCM = std::make_unique<SoundEffect>(m_audEngine.get(), strFilePath);
    m_console->Write(L"Alarm01.wav:       ");
    dump_wfx(m_console.get(), m_alarmPCM->GetFormat());

    DX::FindMediaFile(strFilePath, MAX_PATH, L"tada.wav", s_searchFolders);
    m_tadaPCM = std::make_unique<SoundEffect>(m_audEngine.get(), strFilePath);
    m_console->Write(L"Tada.wav:          ");
    dump_wfx(m_console.get(), m_tadaPCM->GetFormat());

    DX::FindMediaFile(strFilePath, MAX_PATH, L"Alarm01_adpcm.wav", s_searchFolders);
    m_alarmADPCM = std::make_unique<SoundEffect>(m_audEngine.get(), strFilePath);
    m_console->Write(L"Alarm01_adpcm.wav: ");
    dump_wfx(m_console.get(), m_alarmADPCM->GetFormat());

    DX::FindMediaFile(strFilePath, MAX_PATH, L"Alarm01_float.wav", s_searchFolders);
    m_alarmFLOAT = std::make_unique<SoundEffect>(m_audEngine.get(), strFilePath);
    m_console->Write(L"Alarm01_float.wav: ");
    dump_wfx(m_console.get(), m_alarmFLOAT->GetFormat());

#ifdef TEST_XWMA
    DX::FindMediaFile(strFilePath, MAX_PATH, L"Alarm01_xwma.wav", s_searchFolders);
    m_alarmXWMA = std::make_unique<SoundEffect>(m_audEngine.get(), strFilePath);
    m_console->Write(L"Alarm01_xwma.wav:  ");
    dump_wfx(m_console.get(), m_alarmXWMA->GetFormat());
#endif

#ifdef TEST_XMA2
    m_alarmXMA = std::make_unique<SoundEffect>(m_audEngine.get(), L"Alarm01_xma.wav");
    m_console->Write(L"Alarm01_xma.wav:   ");
    dump_wfx(m_console.get(), m_alarmXMA->GetFormat());
#endif

    //--- XWB Wave Banks ---
    DX::FindMediaFile(strFilePath, MAX_PATH, L"droid.xwb", s_searchFolders);
    m_wbPCM = std::make_unique<WaveBank>(m_audEngine.get(), strFilePath);
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

    DX::FindMediaFile(strFilePath, MAX_PATH, L"ADPCMdroid.xwb", s_searchFolders);
    m_wbADPCM = std::make_unique<WaveBank>(m_audEngine.get(), strFilePath);
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
    const wchar_t* s_wbADPCMBank = L"WaveBankADPCM4Kn.xwb";
#else
    const wchar_t* s_wbADPCMBank = L"WaveBankADPCM.xwb";
#endif
    DX::FindMediaFile(strFilePath, MAX_PATH, s_wbADPCMBank, s_searchFolders);
    m_console->WriteLine(s_wbADPCMBank);
    m_wbstreamADPCM = std::make_unique<WaveBank>(m_audEngine.get(), strFilePath);
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
    DX::FindMediaFile(strFilePath, MAX_PATH, L"xwmadroid.xwb", s_searchFolders);
    m_wbXWMA = std::make_unique<WaveBank>(m_audEngine.get(), strFilePath);
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
    const wchar_t* s_wbXWMABank = L"WaveBankxWMA4Kn.xwb";
#else
    const wchar_t* s_wbXWMABank = L"WaveBankxWMA.xwb";
#endif
    DX::FindMediaFile(strFilePath, MAX_PATH, s_wbXWMABank, s_searchFolders);
    m_wbstreamXWMA = std::make_unique<WaveBank>(m_audEngine.get(), strFilePath);
    m_console->WriteLine(s_wbXWMABank);
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
    const wchar_t* s_wbXMA2Bank = L"WaveBankXMA2_4Kn.xwb";
#else
    const wchar_t* s_wbXMA2Bank = L"WaveBankXMA2.xwb";
#endif
    m_wbstreamXMA = std::make_unique<WaveBank>(m_audEngine.get(), s_wbXMA2Bank);
    m_console->WriteLine(s_wbXMA2Bank);
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

    UnitTests();
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

    const auto size = m_deviceResources->GetOutputSize();

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

    context->ClearRenderTargetView(renderTarget, c_clearColor);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    // Set the viewport.
    const auto viewport = m_deviceResources->GetScreenViewport();
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
    m_gamepadButtons.Reset();
    m_keyboardButtons.Reset();
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
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

#if defined(PC) || defined(UWP)
void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
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
    const auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);
    m_console->SetViewport(viewport);

    const auto size = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    float dy = m_comicFont->GetLineSpacing();

    safeRect.top += long(dy * 3);
    safeRect.bottom -= long(dy * 4);

    m_console->SetWindow(safeRect);

#ifdef UWP
    m_spriteBatch->SetRotation(m_deviceResources->GetRotation());
#endif
}

namespace DirectX
{
    // Internal function to validate
    bool __cdecl IsValid(_In_ const WAVEFORMATEX* wfx) noexcept;
}

void Game::UnitTests()
{
    bool success = true;
    OutputDebugStringA("*********** UNIT TESTS BEGIN ***************\n");

    // PCM
    {
        WAVEFORMATEX testwfx{};
        testwfx.wFormatTag = WAVE_FORMAT_PCM;
        testwfx.nChannels = 1;
        testwfx.nSamplesPerSec = 22100;

        WAVEFORMATEX testwfx2 = testwfx;
        testwfx2.wBitsPerSample = 8;
        testwfx2.nBlockAlign = 1;
        testwfx2.nAvgBytesPerSec = testwfx2.nSamplesPerSec * testwfx2.nBlockAlign;

        WAVEFORMATEX testwfx3 = testwfx;
        testwfx3.wBitsPerSample = 178;

        #pragma warning(push)
        #pragma warning(disable:6385 6387)
        if (IsValid(nullptr) || IsValid(&testwfx) || !IsValid(&testwfx2) || IsValid(&testwfx3))
        {
            OutputDebugStringA("ERROR: Failed waveformat valid A tests\n");
            success = false;
        }
        #pragma warning(pop)

        testwfx2 = testwfx;
        testwfx2.nChannels = XAUDIO2_MAX_AUDIO_CHANNELS  * 2;
        testwfx2.wBitsPerSample = 8;
        testwfx2.nBlockAlign = 1;
        testwfx2.nAvgBytesPerSec = testwfx2.nSamplesPerSec * testwfx2.nBlockAlign;

        testwfx3 = testwfx;
        testwfx3.nChannels = 2;
        testwfx3.nSamplesPerSec = XAUDIO2_MAX_SAMPLE_RATE * 2;
        testwfx3.wBitsPerSample = 8;
        testwfx3.nBlockAlign = 1;
        testwfx3.nAvgBytesPerSec = testwfx2.nSamplesPerSec * testwfx2.nBlockAlign;

        if (IsValid(&testwfx2) || IsValid(&testwfx3))
        {
            OutputDebugStringA("ERROR: Failed waveformat valid B tests\n");
            success = false;
        }
    }

    // ADPCM
    {
        const ADPCMCOEFSET s_coefs[7] = { { 256, 0 }, { 512, -256 } , { 0, 0 } , { 192, 64 } , { 240, 0 }, { 460, -208 }, { 392, -232 } };

        uint8_t bytes[sizeof(ADPCMWAVEFORMAT) + 32] = {};
        auto testwfx = reinterpret_cast<ADPCMWAVEFORMAT*>(bytes);

        uint8_t bytes2[sizeof(ADPCMWAVEFORMAT) + 32] = {};
        auto testwfx2 = reinterpret_cast<ADPCMWAVEFORMAT*>(bytes2);

        uint8_t bytes3[sizeof(ADPCMWAVEFORMAT) + 32] = {};
        auto testwfx3 = reinterpret_cast<ADPCMWAVEFORMAT*>(bytes3);

        testwfx->wfx.wFormatTag = WAVE_FORMAT_ADPCM;
        testwfx->wfx.nChannels = 1;
        testwfx->wfx.nSamplesPerSec = 22100;
        testwfx->wfx.wBitsPerSample = 4;
        testwfx->wfx.cbSize = 32;
        testwfx->wNumCoef = 7;
        memcpy(testwfx->aCoef, s_coefs, sizeof(s_coefs));
        testwfx->wSamplesPerBlock = 16;
        testwfx->wfx.nBlockAlign = static_cast<WORD>(7 * testwfx->wfx.nChannels
            + (testwfx->wSamplesPerBlock - 2) * 4 * testwfx->wfx.nChannels / 8);
        testwfx->wfx.nAvgBytesPerSec = static_cast<DWORD>(testwfx->wfx.nBlockAlign * testwfx->wfx.nSamplesPerSec / testwfx->wSamplesPerBlock);

        memcpy(bytes2, bytes, sizeof(bytes));
        testwfx2->wfx.nChannels = 4;

        memcpy(bytes3, bytes, sizeof(bytes));
        testwfx3->wfx.wBitsPerSample = 23;

        if (!IsValid(reinterpret_cast<const WAVEFORMATEX*>(testwfx))
            || IsValid(reinterpret_cast<const WAVEFORMATEX*>(testwfx2))
            || IsValid(reinterpret_cast<const WAVEFORMATEX*>(testwfx3)))
        {
            OutputDebugStringA("ERROR: Failed waveformat valid C tests\n");
            success = false;
        }
    }

    // Float PCM
    {
        PCMWAVEFORMAT testwfx{};
        testwfx.wf.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        testwfx.wBitsPerSample = 32;
        testwfx.wf.nChannels = 2;
        testwfx.wf.nSamplesPerSec = 22100;
        testwfx.wf.nBlockAlign = static_cast<WORD>(testwfx.wf.nChannels * 4);
        testwfx.wf.nAvgBytesPerSec = testwfx.wf.nBlockAlign * testwfx.wf.nSamplesPerSec;

        PCMWAVEFORMAT testwfx2 = testwfx;
        testwfx2.wBitsPerSample = 16;

        PCMWAVEFORMAT testwfx3 = testwfx;
        testwfx3.wf.nBlockAlign = 16;

        if (!IsValid(reinterpret_cast<const WAVEFORMATEX*>(&testwfx))
            || IsValid(reinterpret_cast<const WAVEFORMATEX*>(&testwfx2))
            || IsValid(reinterpret_cast<const WAVEFORMATEX*>(&testwfx3)))
        {
            OutputDebugStringA("ERROR: Failed waveformat valid D tests\n");
            success = false;
        }
    }

    // TODO - WAVE_FORMAT_EXTENSIBLE

    OutputDebugStringA(success ? "Passed\n" : "Failed\n");
    OutputDebugStringA("***********  UNIT TESTS END  ***************\n");

    if (!success)
    {
        throw std::runtime_error("Unit Tests Failed");
    }
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
