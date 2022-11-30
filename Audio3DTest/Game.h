//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK for Audio - Positional
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------
#pragma once

#include "DirectXTKTest.h"
#include "StepTimer.h"

constexpr uint32_t c_testTimeout = 10000;

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

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

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
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();

#ifdef PC
    void OnWindowMoved();
#endif

#if defined(PC) || defined(UWP)
    void OnDisplayChange();
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
    const wchar_t* GetAppName() const { return L"Audio3DTest (DirectX 11)"; }
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

    // DirectXTK Test Objects
#ifdef XBOX
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
#endif

    std::unique_ptr<DirectX::AudioEngine>           m_audEngine;
    std::unique_ptr<DirectX::SoundEffect>           m_soundEffect;
    std::unique_ptr<DirectX::SoundEffectInstance>   m_effect;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_sphere;
    std::unique_ptr<DirectX::SpriteBatch>           m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_comicFont;

    DirectX::AudioListener m_listener;
    DirectX::AudioEmitter  m_emitter;

    DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Matrix m_projection;

    DirectX::SimpleMath::Matrix m_listenerMatrix;
    DirectX::SimpleMath::Matrix m_emitterMatrix;

    bool m_critError;
    bool m_retrydefault;
    bool m_newAudio;

    wchar_t m_deviceStr[256];
};
