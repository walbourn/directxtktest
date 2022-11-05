//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK PostProcess (HDR10/ToneMap)
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------
#pragma once

#include "DirectXTKTest.h"
#include "StepTimer.h"

#include "RenderTexture.h"


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
    const wchar_t* GetAppName() const { return L"HDRTest (DirectX 11)"; }
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void CycleToneMapOperator();
    void CycleColorRotation();

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
    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
#endif

    // HDR resources
    std::unique_ptr<DirectX::ToneMapPostProcess>    m_toneMap;
    std::unique_ptr<DX::RenderTexture>              m_hdrScene;

    // Test resources.
    std::unique_ptr<DirectX::SpriteBatch>               m_batch;
    std::unique_ptr<DirectX::SpriteFont>                m_font;

    std::unique_ptr<DirectX::GeometricPrimitive>        m_shape;

    std::unique_ptr<DirectX::BasicEffect>               m_flatEffect;
    std::unique_ptr<DirectX::BasicEffect>               m_brightEffect;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_hdrImage1;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_hdrImage2;

    Microsoft::WRL::ComPtr<ID3D11InputLayout>           m_flatInputLayout;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>           m_brightInputLayout;

    DirectX::SimpleMath::Matrix                         m_view;
    DirectX::SimpleMath::Matrix                         m_projection;

    int                                                 m_toneMapMode;
    int                                                 m_hdr10Rotation;
};
