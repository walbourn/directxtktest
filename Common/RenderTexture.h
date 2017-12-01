//--------------------------------------------------------------------------------------
// File: RenderTexture.h
//
// Helper for managing offscreen render targets
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once

namespace DX
{
    class RenderTexture
    {
    public:
        RenderTexture(DXGI_FORMAT format);

#if !defined(_MSC_VER) || _MSC_VER >= 1900
        RenderTexture(RenderTexture&&) = default;
        RenderTexture& operator= (RenderTexture&&) = default;
#endif

        RenderTexture(RenderTexture const&) = delete;
        RenderTexture& operator= (RenderTexture const&) = delete;

        void SetDevice(_In_ ID3D11Device* device);

        void SizeResources(size_t width, size_t height);

        void ReleaseDevice();

#if defined(_XBOX_ONE) && defined(_TITLE)
        void EndScene(_In_ ID3D11DeviceContextX* context);
#endif

        void SetWindow(const RECT& rect);

        ID3D11Texture2D*            GetRenderTarget() const { return m_renderTarget.Get(); }
        ID3D11RenderTargetView*	    GetRenderTargetView() const { return m_renderTargetView.Get(); }
        ID3D11ShaderResourceView*   GetShaderResourceView() const { return m_shaderResourceView.Get(); }

        DXGI_FORMAT GetFormat() const { return m_format; }

    private:
        Microsoft::WRL::ComPtr<ID3D11Device>                m_device;
        Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_renderTarget;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_shaderResourceView;

        DXGI_FORMAT                                         m_format;

        size_t                                              m_width;
        size_t                                              m_height;

#if defined(_XBOX_ONE) && defined(_TITLE)
        bool                                                m_fastSemantics;
#endif
    };
}