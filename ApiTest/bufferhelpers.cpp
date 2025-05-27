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

_Success_(return)
bool Test01(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    bool success = true;

    // CreateStaticBuffer
    {
        static const VertexPositionColor s_vertexData[3] =
        {
            { XMFLOAT3{ 0.0f,   0.5f,  0.5f }, XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f } },  // Top / Red
            { XMFLOAT3{ 0.5f,  -0.5f,  0.5f }, XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f } },  // Right / Green
            { XMFLOAT3{ -0.5f, -0.5f,  0.5f }, XMFLOAT4{ 0.0f, 0.0f, 1.0f, 1.0f } }   // Left / Blue
        };

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

    return success;
}
