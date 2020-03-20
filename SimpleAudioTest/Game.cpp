//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK - Simple Audio Test (UI-based BasicAudioTest)
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

#include "pch.h"
#include "Game.h"

//#define GAMMA_CORRECT_RENDERING
//#define USE_FAST_SEMANTICS

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

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
                static const GUID s_wfexBase = { 0x00000000, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 };

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
    
// Constructor.
Game::Game() noexcept(false) :
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
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_UNKNOWN, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_UNKNOWN, 2, D3D_FEATURE_LEVEL_9_3,
        DX::DeviceResources::c_Enable4K_Xbox
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(c_RenderFormat, DXGI_FORMAT_UNKNOWN);
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    m_deviceResources->RegisterDeviceNotify(this);
#endif

    m_console = std::make_unique<DX::TextConsole>();
}

Game::~Game()
{
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
    HWND window,
#else
    IUnknown* window,
#endif
    int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();

#if defined(_XBOX_ONE) && defined(_TITLE)
    UNREFERENCED_PARAMETER(rotation);
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    m_deviceResources->SetWindow(window);
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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
            for (auto it = enumList.cbegin(); it != enumList.cend(); ++it)
            {
                sprintf_s(buff, "\t\"%ls\"\n", it->description.c_str());
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

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#define MEDIA_PATH L"..\\BasicAudioTest\\"
#else
#define MEDIA_PATH
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

#if defined(USING_XAUDIO2_7_DIRECTX) || defined(USING_XAUDIO2_9)
    m_alarmXWMA = std::make_unique<SoundEffect>(m_audEngine.get(), MEDIA_PATH L"Alarm01_xwma.wav");
    m_console->Write(L"Alarm01_xwma.wav: ");
    dump_wfx(m_console.get(), m_alarmXWMA->GetFormat());
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_alarmXMA = std::make_unique<SoundEffect>(m_audEngine.get(), L"Alarm01_xma.wav");
    m_console->Write(L"Alarm01_xma.wav: ");
    dump_wfx(m_console.get(), m_alarmXMA->GetFormat());
#endif

    //--- XWB Wave Banks ---
    m_wbPCM = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"droid.xwb");
    m_console->WriteLine(L"droid.xwb");
    m_console->Format(L"    Index #8 (%zu bytes, %zu samples, %zu ms)\n",
        m_wbPCM->GetSampleSizeInBytes(8),
        m_wbPCM->GetSampleDuration(8),
        m_wbPCM->GetSampleDurationMS(8));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbPCM->GetFormat(8, wfx, 64));
    }

    m_wbADPCM = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"ADPCMdroid.xwb");
    m_console->WriteLine(L"ADPCMdroid.xwb");
    m_console->Format(L"    Index #8 (%zu bytes, %zu samples, %zu ms)\n",
        m_wbADPCM->GetSampleSizeInBytes(8),
        m_wbADPCM->GetSampleDuration(8),
        m_wbADPCM->GetSampleDurationMS(8));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbADPCM->GetFormat(8, wfx, 64));
    }

#if defined(USING_XAUDIO2_7_DIRECTX) || defined(USING_XAUDIO2_9)
    m_wbXWMA = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"xwmadroid.xwb");
    m_console->WriteLine(L"xwmadroid.xwb");
    m_console->Format(L"    Index #8 (%zu bytes, %zu samples, %zu ms)\n",
        m_wbXWMA->GetSampleSizeInBytes(8),
        m_wbXWMA->GetSampleDuration(8),
        m_wbXWMA->GetSampleDurationMS(8));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbXWMA->GetFormat(8, wfx, 64));
    }
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_wbXMA = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"xmadroid.xwb");
    m_console->WriteLine(L"xmadroid.xwb");
    m_console->Format(L"    Index #8 (%zu bytes, %zu samples, %zu ms)\n",
        m_wbXMA->GetSampleSizeInBytes(8),
        m_wbXMA->GetSampleDuration(8),
        m_wbXMA->GetSampleDurationMS(8));
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(&buff);
        dump_wfx(m_console.get(), m_wbXMA->GetFormat(8, wfx, 64));
    }
#endif
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
            m_wbPCM->Play(8);
        }
        if (m_gamepadButtons.dpadLeft == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"ADPCM Wavebank started");
            m_wbADPCM->Play(8);
        }

#if defined(USING_XAUDIO2_7_DIRECTX) || defined(USING_XAUDIO2_9)
        if (m_gamepadButtons.b == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"xWMA alarm started");
            m_alarmXWMA->Play();
        }
        if (m_gamepadButtons.dpadRight == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"xWMA Wavebank started");
            m_wbXWMA->Play(8);
        }
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
        if (m_gamepadButtons.rightStick == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"XMA2 alarm started");
            m_alarmXMA->Play();
        }
        if (m_gamepadButtons.dpadUp == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"XMA2 Wavebank started");
            m_wbXMA->Play(8);
        }
#endif

        if (m_gamepadButtons.menu == ButtonState::PRESSED)
        {
            m_console->WriteLine(L"Voice pool trimmed");
            m_audEngine->TrimVoicePool();
        }
    }
    else
    {
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
            m_wbPCM->Play(8);
        }
        if (m_keyboardButtons.IsKeyPressed(Keyboard::W))
        {
            m_console->WriteLine(L"ADPCM Wavebank started");
            m_wbADPCM->Play(8);
        }

#if defined(USING_XAUDIO2_7_DIRECTX) || defined(USING_XAUDIO2_9)
        if (m_keyboardButtons.IsKeyPressed(Keyboard::D2))
        {
            m_console->WriteLine(L"xWMA alarm started");
            m_alarmXWMA->Play();
        }
        if (m_keyboardButtons.IsKeyPressed(Keyboard::E))
        {
            m_console->WriteLine(L"xWMA Wavebank started");
            m_wbXWMA->Play(8);
        }
#endif

        if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
        {
            m_console->WriteLine(L"Voice pool trimmed");
            m_audEngine->TrimVoicePool();
        }
    }

    if (kb.Escape)
    {
        ExitGame();
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

#if defined(_XBOX_ONE) && defined(_TITLE)
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
    swprintf_s(statsStr, L"Playing: %zu / %zu; Instances %zu; Voices %zu / %zu / %zu / %zu; %zu audio bytes",
        stats.playingOneShots, stats.playingInstances,
        stats.allocatedInstances, stats.allocatedVoices, stats.allocatedVoices3d,
        stats.allocatedVoicesOneShot, stats.allocatedVoicesIdle,
        stats.audioBytes);

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

    if (m_gamepadPresent)
    {
#if defined(_XBOX_ONE) && defined(_TITLE)
        help1 = L"Press A, B, X, Y, or RThumb to trigger Alarm.wav; LTrigger+A for Tada.wav";
#else
        help1 = L"Press A, B, X, or Y to trigger Alarm.wav; LTrigger+A for Tada.wav";
#endif
        help2 = L"Press DPAD to trigger Wavebank entry #8";
        help3 = L"Press MENU/START to trim voices";
        help4 = L"Press VIEW/BACK to exit";
    }
    else
    {
        help1 = L"Press 1, 2, 3, or 4 to trigger Alarm.wav; 5 for Tada.wav";
        help2 = L"Press Q, W, or E to trigger Wavebank entry #8";
        help3 = L"Press Space to trim voices";
        help4 = L"Press Esc to exit";
    }

    m_comicFont->DrawString(m_spriteBatch.get(), help1, pos, blue);
    pos.y += dy;

    m_comicFont->DrawString(m_spriteBatch.get(), help2, pos, blue);
    pos.y += dy;

    m_comicFont->DrawString(m_spriteBatch.get(), help3, pos, blue);
    pos.y += dy;

    m_comicFont->DrawString(m_spriteBatch.get(), help4, pos, blue);

    m_spriteBatch->End();

    // Show the new frame.
    m_deviceResources->Present();

#if defined(_XBOX_ONE) && defined(_TITLE)
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
#if defined(_XBOX_ONE) && defined(_TITLE)
    auto context = m_deviceResources->GetD3DDeviceContext();
    context->Suspend(0);
#endif

    m_audEngine->Suspend();
}

void Game::OnResuming()
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    auto context = m_deviceResources->GetD3DDeviceContext();
    context->Resume();
#endif

    m_timer.ResetElapsedTime();
    m_gamepadButtons.Reset();
    m_keyboardButtons.Reset();
    m_audEngine->Resume();
}

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation)
{
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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

#if defined(_XBOX_ONE) && defined(_TITLE)
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
    
    RECT size = m_deviceResources->GetOutputSize();

    RECT safeRect = Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));

    float dy = m_comicFont->GetLineSpacing();

    safeRect.top += long(dy * 3);
    safeRect.bottom -= long(dy * 4);

    m_console->SetWindow(safeRect);

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    m_spriteBatch->SetRotation(m_deviceResources->GetRotation());
#endif
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
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
