//--------------------------------------------------------------------------------------
// File: GamePadTest.h
//
// Developer unit test for DirectXTK Mouse
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

#include "pch.h"
#include "StepTimer.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop
class Game
{
public:

    Game();

    // Initialization and management
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    void Initialize(IUnknown* window, int width, int height, DXGI_MODE_ROTATION rotation);
#else
    void Initialize(HWND window, int width, int height, DXGI_MODE_ROTATION rotation);
#endif

    // Basic game loop
    void Tick();
    void Render();

    // Rendering helpers
    void Clear();
    void Present();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation);
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    void ValidateDevice();
#endif

    void OnHome();
    void OnEnd();

    // Properites
    void GetDefaultSize( int& width, int& height ) const;


private:

    void Update(DX::StepTimer const& timer);

    void CreateDevice();
    void CreateResources();
    
    void OnDeviceLost();

    // Application state
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    IUnknown*                                       m_window;
#else
    HWND                                            m_window;
#endif
    int                                             m_outputWidth;
    int                                             m_outputHeight;
    DXGI_MODE_ROTATION                              m_outputRotation;

    // Direct3D Objects
    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext;

    // Rendering resources
    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;

    // Game state
    DX::StepTimer                                   m_timer;

    // Test content
    std::unique_ptr<DirectX::SpriteBatch>               m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>                m_comicFont;
    std::unique_ptr<DirectX::SpriteFont>                m_ctrlFont;
    std::unique_ptr<DirectX::GamePad>                   m_gamePad;
    DirectX::GamePad::ButtonStateTracker                m_tracker;
    DirectX::GamePad::State                             m_state;
    std::unique_ptr<bool[]>                             m_found;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_defaultTex;

    const wchar_t *                                 m_lastStr;
    wchar_t                                         m_lastStrBuff[128];
};