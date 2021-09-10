//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK DGSLEffect support
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
    const wchar_t* GetAppName() const { return L"DGSLTest (DirectX 11)"; }
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

    // DirectXTK Test Objects
#ifdef XBOX
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
#endif

    std::unique_ptr<DirectX::CommonStates>      m_states;
    std::unique_ptr<DirectX::DGSLEffectFactory> m_fx;
    std::unique_ptr<DirectX::Model>             m_teapot;
    std::unique_ptr<DirectX::Model>             m_teapotUnlit;
    std::unique_ptr<DirectX::Model>             m_teapotLambert;
    std::unique_ptr<DirectX::Model>             m_teapotPhong;
    std::unique_ptr<DirectX::Model>             m_teapotTest;
    std::unique_ptr<DirectX::Model>             m_gamelevel;
    std::unique_ptr<DirectX::Model>             m_ship;
    std::unique_ptr<DirectX::Model>             m_soldier;

    DirectX::SimpleMath::Matrix                 m_view;
    DirectX::SimpleMath::Matrix                 m_projection;

    DirectX::ModelBone::TransformArray          m_bones;
};
