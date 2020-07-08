//--------------------------------------------------------------------------------------
// File: Game.h
//
// Developer unit test for DirectXTK Effects
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
    final : public DX::IDeviceNotify
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
    const wchar_t* GetAppName() const { return L"EffectsTest (DirectX 11)"; }
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

            CreateTestInputLayout(device, this, &inputLayout);
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

    std::unique_ptr<DirectX::CommonStates>                      m_states;

    std::unique_ptr<DirectX::IEffect>                           m_abstractEffect;

    std::unique_ptr<EffectWithDecl<DirectX::BasicEffect>>       m_basicEffectUnlit;
    std::unique_ptr<EffectWithDecl<DirectX::BasicEffect>>       m_basicEffectUnlitVc;
    std::unique_ptr<EffectWithDecl<DirectX::BasicEffect>>       m_basicEffect;
    std::unique_ptr<EffectWithDecl<DirectX::BasicEffect>>       m_basicEffectNoSpecular;

    std::unique_ptr<EffectWithDecl<DirectX::SkinnedEffect>>     m_skinnedEffect;
    std::unique_ptr<EffectWithDecl<DirectX::SkinnedEffect>>     m_skinnedEffectNoSpecular;

    std::unique_ptr<EffectWithDecl<DirectX::EnvironmentMapEffect>>  m_envmap;
    std::unique_ptr<EffectWithDecl<DirectX::EnvironmentMapEffect>>  m_spheremap;
    std::unique_ptr<EffectWithDecl<DirectX::EnvironmentMapEffect>>  m_dparabolamap;

    std::unique_ptr<EffectWithDecl<DirectX::DualTextureEffect>> m_dualTexture;

    std::unique_ptr<EffectWithDecl<DirectX::AlphaTestEffect>>   m_alphaTest;

    std::unique_ptr<EffectWithDecl<DirectX::NormalMapEffect>>   m_normalMapEffect;
    std::unique_ptr<EffectWithDecl<DirectX::NormalMapEffect>>   m_normalMapEffectNoDiffuse;
    std::unique_ptr<EffectWithDecl<DirectX::NormalMapEffect>>   m_normalMapEffectNormalsOnly;
    std::unique_ptr<EffectWithDecl<DirectX::NormalMapEffect>>   m_normalMapEffectNoSpecular;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cat;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_opaqueCat;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubemap;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_envball;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_envdual;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_overlay;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_defaultTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_brickDiffuse;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_brickNormal;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_brickSpecular;

    Microsoft::WRL::ComPtr<ID3D11Buffer>    m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>    m_indexBuffer;
    
    DirectX::SimpleMath::Matrix             m_view;
    DirectX::SimpleMath::Matrix             m_projection;

    UINT                                    m_indexCount;

    static void CreateTestInputLayout(_In_ ID3D11Device* device, _In_ DirectX::IEffect* effect, _Out_ ID3D11InputLayout** pInputLayout);
};
