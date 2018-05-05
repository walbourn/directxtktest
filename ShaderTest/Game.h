//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK - HLSL shader coverage
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

#include "RenderTexture.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game
#if !defined(_XBOX_ONE) || !defined(_TITLE)
    : public DX::IDeviceNotify
#endif
{
public:

    Game() noexcept(false);

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

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
    void OnWindowMoved();
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    void OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation);
#endif

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    void ValidateDevice();
#endif

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
    const wchar_t* GetAppName() const { return L"ShaderTest (DirectX 11)"; }
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
#if defined(_XBOX_ONE) && defined(_TITLE)
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
#endif

    template<typename T>
    class EffectWithDecl : public T
    {
    public:
        EffectWithDecl(ID3D11Device* device, std::function<void(T*)> setEffectParameters)
            : T(device)
        {
            setEffectParameters(this);

            CreateInputLayout(device, this, &inputLayout, &compressedInputLayout);
        }

        void Apply(ID3D11DeviceContext* context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, bool showCompressed)
        {
            SetMatrices(world, view, projection);

            auto ibasic = dynamic_cast<BasicEffect*>(this);
            if (ibasic)
                ibasic->SetBiasedVertexNormals(showCompressed);

            auto ienvmap = dynamic_cast<EnvironmentMapEffect*>(this);
            if (ienvmap)
                ienvmap->SetBiasedVertexNormals(showCompressed);

            auto inmap = dynamic_cast<NormalMapEffect*>(this);
            if (inmap)
                inmap->SetBiasedVertexNormals(showCompressed);

            auto ipbr = dynamic_cast<PBREffect*>(this);
            if (ipbr)
                ipbr->SetBiasedVertexNormals(showCompressed);

            auto iskin = dynamic_cast<SkinnedEffect*>(this);
            if (iskin)
                iskin->SetBiasedVertexNormals(showCompressed);

            auto idbg = dynamic_cast<DebugEffect*>(this);
            if (idbg)
                idbg->SetBiasedVertexNormals(true);

            T::Apply(context);

            context->IASetInputLayout((showCompressed) ? compressedInputLayout.Get() : inputLayout.Get());
        }

    private:
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> compressedInputLayout;
    };

    template<typename T>
    class DGSLEffectWithDecl : public T
    {
    public:
        DGSLEffectWithDecl(ID3D11Device* device, bool skinning, std::function<void(T*)> setEffectParameters)
            : T(device, nullptr, skinning)
        {
            setEffectParameters(this);

            CreateInputLayout(device, this, &inputLayout);
        }

        void Apply(ID3D11DeviceContext* context, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
        {
            SetMatrices(world, view, projection);

            T::Apply(context);

            context->IASetInputLayout(inputLayout.Get());
        }

    private:
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    };

    std::unique_ptr<DirectX::CommonStates>  m_states;

    std::vector<std::unique_ptr<EffectWithDecl<DirectX::BasicEffect>>> m_basic;
    std::vector<std::unique_ptr<EffectWithDecl<DirectX::SkinnedEffect>>> m_skinning;
    std::vector<std::unique_ptr<EffectWithDecl<DirectX::EnvironmentMapEffect>>> m_envmap;
    std::vector<std::unique_ptr<EffectWithDecl<DirectX::DualTextureEffect>>> m_dual;
    std::vector<std::unique_ptr<EffectWithDecl<DirectX::AlphaTestEffect>>> m_alphTest;
    std::vector<std::unique_ptr<EffectWithDecl<DirectX::NormalMapEffect>>> m_normalMap;
    std::vector<std::unique_ptr<EffectWithDecl<DirectX::PBREffect>>> m_pbr;
    std::vector<std::unique_ptr<EffectWithDecl<DirectX::DebugEffect>>> m_debug;
    std::vector<std::unique_ptr<DGSLEffectWithDecl<DirectX::DGSLEffect>>> m_dgsl;

    Microsoft::WRL::ComPtr<ID3D11Buffer>    m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>    m_compressedVB;
    Microsoft::WRL::ComPtr<ID3D11Buffer>    m_indexBuffer;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cat;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubemap;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_overlay;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_defaultTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_brickDiffuse;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_brickNormal;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_brickSpecular;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pbrAlbedo;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pbrNormal;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pbrRMA;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pbrEmissive;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_radianceIBL;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_irradianceIBL;

    std::unique_ptr<DX::RenderTexture>      m_velocityBuffer;

    DirectX::SimpleMath::Matrix             m_view;
    DirectX::SimpleMath::Matrix             m_projection;

    UINT                                    m_indexCount;

    bool                                    m_showCompressed;

    float                                   m_delay;
};