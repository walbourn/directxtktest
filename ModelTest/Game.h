//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK Model
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------
#pragma once

#include "DirectXTKTest.h"
#include "StepTimer.h"

constexpr uint32_t c_testTimeout = 5000;

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
    const wchar_t* GetAppName() const { return L"ModelTest (DirectX 11)"; }
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

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

    DirectX::GamePad::ButtonStateTracker    m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker m_keyboardButtons;

    // DirectXTK Test Objects
#ifdef XBOX
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
#endif

    std::unique_ptr<DirectX::CommonStates>  m_states;

    std::shared_ptr<DirectX::EnvironmentMapEffect> m_effect;

    std::unique_ptr<DirectX::Model>         m_cup;
    std::unique_ptr<DirectX::Model>         m_cupInst;
    std::unique_ptr<DirectX::Model>         m_cupMesh;
    std::unique_ptr<DirectX::Model>         m_vbo;
    std::unique_ptr<DirectX::Model>         m_vbo2;
    std::unique_ptr<DirectX::Model>         m_teapot;
    std::unique_ptr<DirectX::Model>         m_gamelevel;
    std::unique_ptr<DirectX::Model>         m_ship;
    std::unique_ptr<DirectX::Model>         m_tiny;
    std::unique_ptr<DirectX::Model>         m_soldier;
    std::unique_ptr<DirectX::Model>         m_dwarf;
    std::unique_ptr<DirectX::Model>         m_lmap;
    std::unique_ptr<DirectX::Model>         m_nmap;

    std::unique_ptr<DirectX::EffectFactory> m_abstractFXFactory;
    std::unique_ptr<DirectX::EffectFactory> m_fxFactory;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_defaultTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubemap;

    DirectX::SimpleMath::Matrix             m_view;
    DirectX::SimpleMath::Matrix             m_projection;

    Microsoft::WRL::ComPtr<ID3D11Buffer>                            m_instancedVB;

    UINT                                                            m_instanceCount;
    std::unique_ptr<DirectX::XMFLOAT3X4[]>                          m_instanceTransforms;
    DirectX::ModelBone::TransformArray                              m_bones;

    bool m_spinning;
    float m_pitch;
    float m_yaw;
};
