//--------------------------------------------------------------------------------------
// File: bufferhelpers.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "BufferHelpers.h"

#include "VertexTypes.h"

#include <cstdio>
#include <iterator>
#include <vector>

#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    const VertexPositionColor s_vertexData[3] =
    {
        { XMFLOAT3{ 0.0f,   0.5f,  0.5f }, XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f } },  // Top / Red
        { XMFLOAT3{ 0.5f,  -0.5f,  0.5f }, XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f } },  // Right / Green
        { XMFLOAT3{ -0.5f, -0.5f,  0.5f }, XMFLOAT4{ 0.0f, 0.0f, 1.0f, 1.0f } }   // Left / Blue
    };

    const uint32_t s_pixels[16] = {
        0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff,
        0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff,
    };
}

_Success_(return)
bool Test01(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    bool success = true;

    // CreateStaticBuffer
    {
        ComPtr<ID3D11Buffer> vb;
        HRESULT hr = CreateStaticBuffer(device,
            s_vertexData, std::size(s_vertexData), sizeof(VertexPositionColor),
            D3D11_BIND_VERTEX_BUFFER,
            vb.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: Failed CreateStaticBuffer(1) test (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        ComPtr<ID3D11Buffer> vb2;
        hr = CreateStaticBuffer(device,
            s_vertexData, std::size(s_vertexData),
            D3D11_BIND_VERTEX_BUFFER,
            vb2.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: Failed CreateStaticBuffer(2) test (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        ComPtr<ID3D11Buffer> vb3;
        std::vector<VertexPositionColor> verts(s_vertexData, s_vertexData + std::size(s_vertexData));

        hr = CreateStaticBuffer(device,
            verts,
            D3D11_BIND_VERTEX_BUFFER,
            vb3.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: Failed CreateStaticBuffer(3) test (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        if (device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
        {
            ComPtr<ID3D11Buffer> vb4;
            hr = CreateStaticBuffer(device,
                verts,
                D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS,
                vb4.GetAddressOf());
            if (FAILED(hr))
            {
                printf("ERROR: Failed CreateStaticBuffer(4) test (%08X)\n", static_cast<unsigned int>(hr));
                success = false;
            }
        }
    }

    // CreateTextureFromMemory
    {
        ComPtr<ID3D11Texture1D> res1;
        D3D11_SUBRESOURCE_DATA initData = { s_pixels, 0, 0 };
        HRESULT hr = CreateTextureFromMemory(device, 4u, DXGI_FORMAT_B8G8R8A8_UNORM, initData, res1.GetAddressOf(), nullptr);
        if (FAILED(hr))
        {
            printf("ERROR: Failed CreateTextureFromMemory 1D test (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        ComPtr<ID3D11Texture2D> res2;
        initData = { s_pixels, sizeof(uint32_t) * 8, 0 };
        hr = CreateTextureFromMemory(device, 8u, 2u, DXGI_FORMAT_B8G8R8A8_UNORM, initData, res2.GetAddressOf(), nullptr);
        if (FAILED(hr))
        {
            printf("ERROR: Failed CreateTextureFromMemory 2D test (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        ComPtr<ID3D11Texture3D> res3;
        initData = { s_pixels, sizeof(uint32_t) * 2, sizeof(uint32_t) * 4 };

        hr = CreateTextureFromMemory(device, 2u, 2u, 4, DXGI_FORMAT_B8G8R8A8_UNORM, initData, res3.GetAddressOf(), nullptr);
        if (FAILED(hr))
        {
            printf("ERROR: Failed CreateTextureFromMemory 3D test (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        ComPtr<ID3D11Buffer> res;
        ID3D11Device* nullDevice = nullptr;

        // CreateStaticBuffer
        HRESULT hr = CreateStaticBuffer(device, nullptr, 0, 0, 0u, nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateStaticBuffer - expected failure for null buffer (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateStaticBuffer(nullDevice, nullptr, 0, 0, 0u, res.ReleaseAndGetAddressOf());
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateStaticBuffer - expected failure for null device (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateStaticBuffer(device, s_vertexData, 0, 0, 0u, res.ReleaseAndGetAddressOf());
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateStaticBuffer - expected failure for zero length (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateStaticBuffer(device, s_vertexData, UINT32_MAX, INT32_MAX, 0u, res.ReleaseAndGetAddressOf());
        if (hr != HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
        {
            printf("ERROR: CreateUploadBuffer - expected failure for too large bytes (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        // CreateTextureFromMemory 1D
        ComPtr<ID3D11Texture1D> res1;
        D3D11_SUBRESOURCE_DATA initData = { s_pixels, 0, 0 };
        hr = CreateTextureFromMemory(device, 0, DXGI_FORMAT_UNKNOWN, initData, nullptr, nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateTextureFromMemory 1D - expected failure for null buffer (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateTextureFromMemory(nullDevice, 0, DXGI_FORMAT_UNKNOWN, initData, res1.ReleaseAndGetAddressOf(), nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateTextureFromMemory 1D - expected failure for null device (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateTextureFromMemory(device, 0, DXGI_FORMAT_UNKNOWN, initData, res1.ReleaseAndGetAddressOf(), nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateTextureFromMemory 1D - expected failure for zero dimensions (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateTextureFromMemory(device, UINT32_MAX, DXGI_FORMAT_UNKNOWN, initData, res1.ReleaseAndGetAddressOf(), nullptr);
        if (hr != HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
        {
            printf("ERROR: CreateTextureFromMemory 1D - expected failure for too large bytes (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        // CreateTextureFromMemory 2D
        ComPtr<ID3D11Texture2D> res2;
        initData = { s_pixels, sizeof(uint32_t) * 8, 0 };
        hr = CreateTextureFromMemory(device, 0, 0, DXGI_FORMAT_UNKNOWN, initData, nullptr, nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateTextureFromMemory 2D - expected failure for null buffer (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateTextureFromMemory(nullDevice, 0, 0, DXGI_FORMAT_UNKNOWN, initData, res2.ReleaseAndGetAddressOf(), nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateTextureFromMemory 2D - expected failure for null device (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateTextureFromMemory(device, 0, 0, DXGI_FORMAT_UNKNOWN, initData, res2.ReleaseAndGetAddressOf(), nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateTextureFromMemory 2D - expected failure for zero dimensions (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateTextureFromMemory(device, UINT32_MAX, UINT32_MAX, DXGI_FORMAT_UNKNOWN, initData, res2.ReleaseAndGetAddressOf(), nullptr);
        if (hr != HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
        {
            printf("ERROR: CreateTextureFromMemory 2D - expected failure for too large bytes (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        // CreateTextureFromMemory 3D
        ComPtr<ID3D11Texture3D> res3;
        initData = { s_pixels, sizeof(uint32_t) * 2, sizeof(uint32_t) * 4 };
        ID3D11Texture3D** nullTex3D = nullptr;
        hr = CreateTextureFromMemory(device, 0, 0, 0, DXGI_FORMAT_UNKNOWN, initData, nullTex3D, nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateTextureFromMemory 3D - expected failure for null buffer (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateTextureFromMemory(nullDevice, 0, 0, 0, DXGI_FORMAT_UNKNOWN, initData, res3.ReleaseAndGetAddressOf(), nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateTextureFromMemory 3D - expected failure for null device (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateTextureFromMemory(device, 0, 0, 0, DXGI_FORMAT_UNKNOWN, initData, res3.ReleaseAndGetAddressOf(), nullptr);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: CreateTextureFromMemory 3D - expected failure for zero dimensions (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = CreateTextureFromMemory(device, UINT32_MAX, UINT32_MAX, UINT32_MAX, DXGI_FORMAT_UNKNOWN, initData, res3.ReleaseAndGetAddressOf(), nullptr);
        if (hr != HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED))
        {
            printf("ERROR: CreateTextureFromMemory 3D - expected failure for too large bytes (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }
    #pragma warning(pop)

    return success;
}
