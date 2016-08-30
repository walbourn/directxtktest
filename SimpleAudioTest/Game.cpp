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
}
    
// Constructor.
Game::Game() :
    m_critError(false),
    m_retrydefault(false),
    m_newAudio(false),
    m_gamepadPresent(false)
{
    *m_deviceStr = 0;

    // 2D only rendering
#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN, 2, true);
#elif defined(GAMMA_CORRECT_RENDERING)
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    m_deviceResources->RegisterDeviceNotify(this);
#endif
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

    m_alarmPCM = std::make_unique<SoundEffect>(m_audEngine.get(), MEDIA_PATH L"Alarm01.wav");
    m_alarmADPCM = std::make_unique<SoundEffect>(m_audEngine.get(), MEDIA_PATH L"Alarm01_adpcm.wav");
    m_alarmFLOAT = std::make_unique<SoundEffect>(m_audEngine.get(), MEDIA_PATH L"Alarm01_float.wav");

    m_wbPCM = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"droid.xwb");
    m_wbADPCM = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"ADPCMdroid.xwb");

#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
    m_alarmXWMA = std::make_unique<SoundEffect>(m_audEngine.get(), MEDIA_PATH L"Alarm01_xwma.wav");
    m_wbXWMA = std::make_unique<WaveBank>(m_audEngine.get(), MEDIA_PATH L"xwmadroid.xwb");
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
        if (pad.IsViewPressed())
        {
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
            PostQuitMessage(0);
#else
            Windows::ApplicationModel::Core::CoreApplication::Exit();
#endif
        }

        if (m_gamepadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_alarmPCM->Play();
        }
        if (m_gamepadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_alarmADPCM->Play();
        }
        if (m_gamepadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_alarmFLOAT->Play();
        }

        if (m_gamepadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            m_wbPCM->Play(8);
        }
        if (m_gamepadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            m_wbADPCM->Play(8);
        }

#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
        if (m_gamepadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_alarmXWMA->Play();
        }
        if (m_gamepadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            m_wbXWMA->Play(8);
        }
#endif

        if (m_gamepadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
        {
            m_audEngine->TrimVoicePool();
        }
    }
    else
    {
        if (m_keyboardButtons.IsKeyPressed(Keyboard::D1))
        {
            m_alarmPCM->Play();
        }
        if (m_keyboardButtons.IsKeyPressed(Keyboard::D3))
        {
            m_alarmADPCM->Play();
        }
        if (m_keyboardButtons.IsKeyPressed(Keyboard::D4))
        {
            m_alarmFLOAT->Play();
        }

        if (m_keyboardButtons.IsKeyPressed(Keyboard::Q))
        {
            m_wbPCM->Play(8);
        }
        if (m_keyboardButtons.IsKeyPressed(Keyboard::W))
        {
            m_wbADPCM->Play(8);
        }

#if defined(_XBOX_ONE) || (_WIN32_WINNT < _WIN32_WINNT_WIN8) || (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
        if (m_keyboardButtons.IsKeyPressed(Keyboard::D2))
        {
            m_alarmXWMA->Play();
        }
        if (m_keyboardButtons.IsKeyPressed(Keyboard::E))
        {
            m_wbXWMA->Play(8);
        }
#endif

        if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
        {
            m_audEngine->TrimVoicePool();
        }
    }

    if (kb.Escape)
    {
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
        PostQuitMessage(0);
#else
        Windows::ApplicationModel::Core::CoreApplication::Exit();
#endif
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

    auto stats = m_audEngine->GetStatistics();

    wchar_t statsStr[256] = {};
    swprintf_s(statsStr, L"Playing: %Iu / %Iu; Instances %Iu; Voices %Iu / %Iu / %Iu / %Iu; %Iu audio bytes",
        stats.playingOneShots, stats.playingInstances,
        stats.allocatedInstances, stats.allocatedVoices, stats.allocatedVoices3d,
        stats.allocatedVoicesOneShot, stats.allocatedVoicesIdle,
        stats.audioBytes);

    auto size = m_deviceResources->GetOutputSize();

    auto safeRect = Viewport::ComputeTitleSafeArea(size.right, size.bottom);

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
        help1 = L"Press A, B, X, or Y to trigger Alarm.wav";
        help2 = L"Press DPAD to trigger Wavebank entry #8";
        help3 = L"Press MENU/START to trim voices";
        help4 = L"Press VIEW/BACK to exit";
    }
    else
    {
        help1 = L"Press 1, 2, 3, or 4 to trigger Alarm.wav";
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
    m_audEngine->Suspend();
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_gamepadButtons.Reset();
    m_keyboardButtons.Reset();
    m_audEngine->Resume();
}

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
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    m_spriteBatch->SetRotation(m_deviceResources->GetRotation());
#endif
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnDeviceLost()
{
    m_spriteBatch.reset();
    m_comicFont.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
