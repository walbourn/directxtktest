//--------------------------------------------------------------------------------------
// File: RenderTexture.cpp
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

#include "pch.h"
#include "RenderTexture.h"

#include "DirectXHelpers.h"

#include <algorithm>
#include <stdio.h>
#include <stdexcept>

#include <wrl/client.h>

using namespace DirectX;
using namespace DX;

using Microsoft::WRL::ComPtr;

RenderTexture::RenderTexture(DXGI_FORMAT format) :
    m_format(format),
    m_width(0),
    m_height(0)
{
}

void RenderTexture::SetDevice(_In_ ID3D11Device* device)
{
    if (device == m_device.Get())
        return;

    if (m_device)
    {
        ReleaseDevice();
    }

    {
        UINT formatSupport = 0;
        if (FAILED(device->CheckFormatSupport(m_format, &formatSupport)))
        {
            throw std::exception("CheckFormatSupport");
        }

        UINT32 required = D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE | D3D11_FORMAT_SUPPORT_RENDER_TARGET;
        if ((formatSupport & required) != required)
        {
#ifdef _DEBUG
            char buff[128] = {};
            sprintf_s(buff, "RenderTexture: Device does not support the requested format (%u)!\n", m_format);
            OutputDebugStringA(buff);
#endif
            throw std::exception("RenderTexture");
        }
    }

    m_device = device;

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_fastSemantics = (device->GetCreationFlags() & D3D11_CREATE_DEVICE_IMMEDIATE_CONTEXT_FAST_SEMANTICS) != 0;
#endif
}


void RenderTexture::SizeResources(size_t width, size_t height)
{
    if (width == m_width && height == m_height)
        return;

    if (m_width > UINT32_MAX || m_height > UINT32_MAX)
    {
        throw std::out_of_range("Invalid width/height");
    }

    if (!m_device)
        return;

    m_width = m_height = 0;

    // Create a render target
    CD3D11_TEXTURE2D_DESC renderTargetDesc(
        m_format,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1, // The render target view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT,
        0,
        1
    );

    ThrowIfFailed(m_device->CreateTexture2D(
        &renderTargetDesc,
        nullptr,
        m_renderTarget.ReleaseAndGetAddressOf()
    ));

    SetDebugObjectName(m_renderTarget.Get(), "RenderTexture RT");

    // Create RTV.
    CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, m_format);

    ThrowIfFailed(m_device->CreateRenderTargetView(
        m_renderTarget.Get(),
        &renderTargetViewDesc,
        m_renderTargetView.ReleaseAndGetAddressOf()
    ));

    SetDebugObjectName(m_renderTargetView.Get(), "RenderTexture RTV");

    // Create SRV.
    CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc(D3D11_SRV_DIMENSION_TEXTURE2D, m_format);

    ThrowIfFailed(m_device->CreateShaderResourceView(
        m_renderTarget.Get(),
        &shaderResourceViewDesc,
        m_shaderResourceView.ReleaseAndGetAddressOf()
    ));

    SetDebugObjectName(m_shaderResourceView.Get(), "RenderTexture SRV");

    m_width = width;
    m_height = height;
}


void RenderTexture::ReleaseDevice()
{
    m_renderTargetView.Reset();
    m_shaderResourceView.Reset();
    m_renderTarget.Reset();

    m_device.Reset();

    m_width = m_height = 0;
}

#if defined(_XBOX_ONE) && defined(_TITLE)
void RenderTexture::EndScene(_In_ ID3D11DeviceContextX* context)
{
    if (m_fastSemantics)
    {
        context->FlushGpuCacheRange(
            D3D11_FLUSH_ENSURE_CB0_COHERENCY
            | D3D11_FLUSH_COLOR_BLOCK_INVALIDATE
            | D3D11_FLUSH_TEXTURE_L1_INVALIDATE
            | D3D11_FLUSH_TEXTURE_L2_INVALIDATE,
            nullptr, D3D11_FLUSH_GPU_CACHE_RANGE_ALL);
        context->DecompressResource(m_renderTarget.Get(), 0, nullptr,
            m_renderTarget.Get(), 0, nullptr,
            m_format, D3D11X_DECOMPRESS_PROPAGATE_COLOR_CLEAR);
    }
}
#endif

void RenderTexture::SetWindow(const RECT& output)
{
    // Determine the render target size in pixels.
    size_t width = std::max<size_t>(output.right - output.left, 1);
    size_t height = std::max<size_t>(output.bottom - output.top, 1);

    SizeResources(width, height);
}

