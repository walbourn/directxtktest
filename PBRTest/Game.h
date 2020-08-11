//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK PBR Effect Test
//
// Copyright (c) Microsoft Corporation. All rights reserved.
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
    const wchar_t* GetAppName() const { return L"PBRTest (DirectX 11)"; }
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void CycleDebug();
    void CycleToneMapOperator();

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
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::CommonStates>      m_states;

    // HDR resources
    std::unique_ptr<DirectX::ToneMapPostProcess>    m_toneMap;
    std::unique_ptr<DX::RenderTexture>              m_hdrScene;

    // Test geometry
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayoutNM;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayoutPBR;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayoutDBG;
    UINT                                        m_indexCount;

    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBufferCube;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBufferCube;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayoutCube;
    UINT                                        m_indexCountCube;

    // Test materials
    std::unique_ptr<DirectX::NormalMapEffect>       m_normalMapEffect;
    std::unique_ptr<DirectX::PBREffect>             m_pbr;
    std::unique_ptr<DirectX::PBREffect>             m_pbrCube;
    std::unique_ptr<DirectX::DebugEffect>           m_debug;

    static const size_t s_nMaterials = 3;
    static const size_t s_nIBL = 3;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_baseColor[s_nMaterials];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_normalMap[s_nMaterials];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_rma[s_nMaterials];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_emissiveMap[s_nMaterials];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_radianceIBL[s_nIBL];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_irradianceIBL[s_nIBL];

    int m_toneMapMode;
    uint32_t m_ibl;
    bool m_spinning;
    bool m_showDebug;
    DirectX::DebugEffect::Mode m_debugMode;
    float m_pitch;
    float m_yaw;
};
