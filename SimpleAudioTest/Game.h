//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK - Simple Audio Test (UI-based BasicAudioTest)
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------
#pragma once

#include "DirectXTKTest.h"
#include "StepTimer.h"
#include "TextConsole.h"

#ifdef USING_XAUDIO2_9
#define TEST_XWMA
#endif

#ifdef XBOX
#define TEST_XMA2
#endif

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game
#ifdef LOSTDEVICE
    final : public DX::IDeviceNotify
#endif
{
public:

    Game() noexcept(false);
    ~Game();

    // Initialization and management
#ifdef COREWINDOW
    void Initialize(IUnknown* window, int width, int height, DXGI_MODE_ROTATION rotation);
#else
    void Initialize(HWND window, int width, int height, DXGI_MODE_ROTATION rotation);
#endif

    // Basic game loop
    void Tick();

#ifdef LOSTDEVICE
    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;
#endif

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();

#ifdef PC
    void OnWindowMoved();
#endif

#ifndef XBOX
    void OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation);
#endif

#ifdef UWP
    void ValidateDevice();
#endif

    void OnAudioDeviceChange();

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
    const wchar_t* GetAppName() const { return L"SimpleAudioTest (DirectX 11)"; }
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void AudioRender();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>       m_gamePad;
    std::unique_ptr<DirectX::Keyboard>      m_keyboard;
    DirectX::Keyboard::KeyboardStateTracker m_keyboardButtons;
    DirectX::GamePad::ButtonStateTracker    m_gamepadButtons;

    // DirectXTK Test Objects
#ifdef XBOX
    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
#endif

    std::unique_ptr<DirectX::AudioEngine>   m_audEngine;
    std::unique_ptr<DirectX::SpriteBatch>   m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>    m_comicFont;

    std::unique_ptr<DirectX::SoundEffect>           m_alarmPCM;
    std::unique_ptr<DirectX::SoundEffect>           m_tadaPCM;
    std::unique_ptr<DirectX::SoundEffect>           m_alarmADPCM;
    std::unique_ptr<DirectX::SoundEffect>           m_alarmFLOAT;

    std::unique_ptr<DirectX::SoundStreamInstance>   m_streamADPCM;

#ifdef TEST_XWMA
    std::unique_ptr<DirectX::SoundEffect>           m_alarmXWMA;
    std::unique_ptr<DirectX::WaveBank>              m_wbXWMA;
    std::unique_ptr<DirectX::WaveBank>              m_wbstreamXWMA;
    std::unique_ptr<DirectX::SoundStreamInstance>   m_streamXWMA;
#endif

#ifdef TEST_XMA2
    std::unique_ptr<DirectX::SoundEffect>           m_alarmXMA;
    std::unique_ptr<DirectX::WaveBank>              m_wbXMA;
    std::unique_ptr<DirectX::WaveBank>              m_wbstreamXMA;
    std::unique_ptr<DirectX::SoundStreamInstance>   m_streamXMA;
#endif

    std::unique_ptr<DirectX::WaveBank>      m_wbPCM;
    std::unique_ptr<DirectX::WaveBank>      m_wbADPCM;
    std::unique_ptr<DirectX::WaveBank>      m_wbstreamADPCM;

    std::unique_ptr<DX::TextConsole>        m_console;

    unsigned int m_currentStream;

    bool m_critError;
    bool m_retrydefault;
    bool m_newAudio;

    wchar_t m_deviceStr[256];

    bool m_gamepadPresent;

    DirectX::SoundStreamInstance* GetCurrentStream(unsigned int);
    void UpdateCurrentStream(bool isplay);
    void CycleCurrentStream(bool increment);
};
