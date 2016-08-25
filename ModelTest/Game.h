//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK Model
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------
#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include "DeviceResourcesXDK.h"
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#include "DeviceResourcesUWP.h"
#else
#include "DeviceResourcesPC.h"
#endif
#include "StepTimer.h"

#include "PlatformHelpers.h"


// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game
#if !defined(_XBOX_ONE) || !defined(_TITLE)
    : public DX::IDeviceNotify
#endif
{
public:

    Game();

    // Initialization and management
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
    void Initialize(HWND window, int width, int height, DXGI_MODE_ROTATION rotation);
#else
    void Initialize(IUnknown* window, int width, int height, DXGI_MODE_ROTATION rotation);
#endif

    // Basic game loop
    void Tick();
    void Render();

    // Rendering helpers
    void Clear();

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;
#endif

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    void OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation);
#endif

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    void ValidateDevice();
#endif

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
    const wchar_t* GetAppName() const { return L"ModelTest (DirectX 11)"; }

private:

    void Update(DX::StepTimer const& timer);

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
#if defined(_XBOX_ONE) && defined(_TITLE)
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
#endif

    std::unique_ptr<DirectX::CommonStates>  m_states;

    std::shared_ptr<DirectX::EnvironmentMapEffect> m_effect;

    std::unique_ptr<DirectX::Model>         m_cup;
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

    std::unique_ptr<DirectX::EffectFactory> m_fxFactory;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_defaultTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubemap;

    DirectX::SimpleMath::Matrix             m_view;
    DirectX::SimpleMath::Matrix             m_projection;

    std::unique_ptr<DirectX::XMMATRIX[], DirectX::aligned_deleter> m_bones;

};
