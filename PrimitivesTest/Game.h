//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK Geometric Primitives
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------
#pragma once

#include "DirectXTKTest.h"
#include "StepTimer.h"

constexpr uint32_t c_testTimeout = 15000;

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
    const wchar_t* GetAppName() const { return L"PrimitivesTest (DirectX 11)"; }
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

    std::unique_ptr<DirectX::CommonStates>          m_states;

    std::unique_ptr<DirectX::GeometricPrimitive>    m_cube;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_box;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_sphere;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_geosphere;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_cylinder;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_cone;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_torus;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_teapot;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_tetra;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_octa;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_dodec;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_iso;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_customBox;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_customBox2;

    std::unique_ptr<DirectX::BasicEffect>           m_customEffect;
    std::unique_ptr<DirectX::NormalMapEffect>       m_instancedEffect;

    Microsoft::WRL::ComPtr<ID3D11InputLayout>           m_customIL;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>           m_instancedIL;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_cat;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_dxlogo;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_reftxt;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_normalMap;

    DirectX::SimpleMath::Matrix                         m_view;
    DirectX::SimpleMath::Matrix                         m_projection;

    Microsoft::WRL::ComPtr<ID3D11Buffer>                m_instancedVB;

    UINT                                                m_instanceCount;
    std::unique_ptr<DirectX::XMFLOAT3X4[]>              m_instanceTransforms;

    bool m_spinning;
    float m_pitch;
    float m_yaw;
};
