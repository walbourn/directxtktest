//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK PostProcess
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
    const wchar_t* GetAppName() const { return L"PostProcessTest (DirectX 11)"; }

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
    std::unique_ptr<DirectX::GamePad>                   m_gamePad;
    std::unique_ptr<DirectX::Keyboard>                  m_keyboard;

    DirectX::GamePad::ButtonStateTracker                m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker             m_keyboardButtons;

    // DirectXTK Test Objects
#if defined(_XBOX_ONE) && defined(_TITLE)
    std::unique_ptr<DirectX::GraphicsMemory>            m_graphicsMemory;
#endif

    DirectX::SimpleMath::Matrix                         m_world;
    DirectX::SimpleMath::Matrix                         m_view;
    DirectX::SimpleMath::Matrix                         m_proj;

    std::unique_ptr<DirectX::BasicPostProcess>          m_basicPostProcess;
    std::unique_ptr<DirectX::DualPostProcess>           m_dualPostProcess;
    int                                                 m_scene;

    std::unique_ptr<DirectX::SpriteBatch>               m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>                m_font;
    std::unique_ptr<DirectX::GeometricPrimitive>        m_shape;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_background;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_sceneTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_sceneSRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_sceneRT;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_blur1Tex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_blur1SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_blur1RT;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_blur2Tex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_blur2SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_blur2RT;
};