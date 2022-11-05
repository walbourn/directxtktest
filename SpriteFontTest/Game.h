//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK SpriteFont
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------
#pragma once

#include "DirectXTKTest.h"
#include "StepTimer.h"


// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game
#ifdef LOSTDEVICE
    final : public DX::IDeviceNotify
#endif
{
public:

    Game() noexcept(false);
    ~Game() = default;

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

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
    const wchar_t* GetAppName() const { return L"SpriteFontTest (DirectX 11)"; }
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void UnitTests();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>       m_gamePad;
    std::unique_ptr<DirectX::Keyboard>      m_keyboard;

    DirectX::GamePad::ButtonStateTracker    m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker m_keyboardButtons;

    // DirectXTK Test Objects
#ifdef XBOX
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
#endif

    std::unique_ptr<DirectX::SpriteBatch>   m_spriteBatch;

    std::unique_ptr<DirectX::SpriteFont>    m_comicFont;
    std::unique_ptr<DirectX::SpriteFont>    m_italicFont;
    std::unique_ptr<DirectX::SpriteFont>    m_scriptFont;
    std::unique_ptr<DirectX::SpriteFont>    m_nonproportionalFont;
    std::unique_ptr<DirectX::SpriteFont>    m_multicoloredFont;
    std::unique_ptr<DirectX::SpriteFont>    m_japaneseFont;
    std::unique_ptr<DirectX::SpriteFont>    m_ctrlFont;
    std::unique_ptr<DirectX::SpriteFont>    m_ctrlOneFont;
    std::unique_ptr<DirectX::SpriteFont>    m_consolasFont;

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_scissorState;

    uint64_t m_frame;

    bool                                    m_showUTF8;
    float                                   m_delay;
};
