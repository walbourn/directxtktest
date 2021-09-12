//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK PostProcess
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
    const wchar_t* GetAppName() const { return L"PostProcessTest (DirectX 11)"; }
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void ShaderTest();

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
#ifdef XBOX
    std::unique_ptr<DirectX::GraphicsMemory>            m_graphicsMemory;
#endif

    DirectX::SimpleMath::Matrix                         m_world;
    DirectX::SimpleMath::Matrix                         m_view;
    DirectX::SimpleMath::Matrix                         m_proj;

    std::unique_ptr<DirectX::IPostProcess>              m_abstractPostProcess;
    std::unique_ptr<DirectX::BasicPostProcess>          m_basicPostProcess;
    std::unique_ptr<DirectX::DualPostProcess>           m_dualPostProcess;
    std::unique_ptr<DirectX::ToneMapPostProcess>        m_toneMapPostProcess;
    int                                                 m_scene;

    std::unique_ptr<DirectX::SpriteBatch>               m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>                m_font;
    std::unique_ptr<DirectX::GeometricPrimitive>        m_shape;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_background;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_hdrTexture;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_sceneTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_sceneSRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_sceneRT;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_blur1Tex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_blur1SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_blur1RT;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_blur2Tex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_blur2SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_blur2RT;

    float                                               m_delay;
};
