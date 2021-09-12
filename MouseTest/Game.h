//--------------------------------------------------------------------------------------
// File: MouseTest.h
//
// Developer unit test for DirectXTK Mouse
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

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
    const wchar_t* GetAppName() const { return L"MouseTest (DirectX 11)"; }
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                                       m_timer;

    // Input devices.
    std::unique_ptr<DirectX::Keyboard>                  m_keyboard;
    std::unique_ptr<DirectX::Mouse>                     m_mouse;

    DirectX::Keyboard::KeyboardStateTracker             m_keyboardButtons;
    DirectX::Mouse::ButtonStateTracker                  m_tracker;

    // DirectXTK Test Objects
#ifdef XBOX
    std::unique_ptr<DirectX::GraphicsMemory>            m_graphicsMemory;
#endif

    std::unique_ptr<DirectX::SpriteBatch>               m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>                m_comicFont;
    DirectX::Mouse::State                               m_ms;
    DirectX::Mouse::Mode                                m_lastMode;
    std::unique_ptr<DirectX::GeometricPrimitive>        m_room;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_roomTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_cursor;

    float                                               m_pitch;
    float                                               m_yaw;

    DirectX::SimpleMath::Vector3                        m_cameraPos;
    DirectX::SimpleMath::Matrix                         m_proj;

    const wchar_t *                                     m_lastStr;
};
