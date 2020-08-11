//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK DDSTextureLoader & WICTextureLoader
//
// Copyright (c) Microsoft Corporation. All rights reserved.
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
    const wchar_t* GetAppName() const { return L"LoadTest (DirectX 11)"; }
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void UnitTests(bool success);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>       m_gamePad;
    std::unique_ptr<DirectX::Keyboard>      m_keyboard;

    // DirectXTK Test Objects
#ifdef XBOX
    std::unique_ptr<DirectX::GraphicsMemory>            m_graphicsMemory;
#endif

    std::unique_ptr<DirectX::GeometricPrimitive>        m_cube;
    std::unique_ptr<DirectX::BasicEffect>               m_effect;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_earth;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_earth2;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_dxlogo;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_dxlogo2;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_win95;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_win95_2;

    Microsoft::WRL::ComPtr<ID3D11InputLayout>           m_layout;

    uint64_t m_frame;
};
