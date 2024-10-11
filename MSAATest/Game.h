//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK MSAATest
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------
#pragma once

#include "DirectXTKTest.h"
#include "StepTimer.h"
#include "MSAAHelper.h"

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
    void OnActivated() {}
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
    const wchar_t* GetAppName() const { return L"MSAATest (DirectX 11)"; }
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
    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
#endif

    std::unique_ptr<DirectX::BasicEffect>           m_effect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_inputLayout;

    std::unique_ptr<DirectX::CommonStates>          m_states;

    using Vertex = DirectX::VertexPositionTexture;
    std::unique_ptr<DirectX::PrimitiveBatch<Vertex>> m_batch;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState>    m_rstate;

    // MSAA resources
    std::unique_ptr<DX::MSAAHelper>         m_msaaHelper2;
    std::unique_ptr<DX::MSAAHelper>         m_msaaHelper4;
    std::unique_ptr<DX::MSAAHelper>         m_msaaHelper8;

    enum State
    {
        NOMSAA,
        MSAA2X,
        MSAA4X,
        MSAA8X,
        COUNT,
    };

    int     m_state;
    uint64_t m_frame;
    float    m_delay;
};
