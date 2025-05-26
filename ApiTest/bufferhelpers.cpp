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

using namespace DirectX;
using Microsoft::WRL::ComPtr;

bool Test01(ID3D11Device *device)
{
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
        if (FAILED(CreateStaticBuffer(device, s_vertexData, std::size(s_vertexData), sizeof(VertexPositionColor),
            D3D11_BIND_VERTEX_BUFFER, vb.GetAddressOf())))
        {
            printf("ERROR: Failed CreateStaticBuffer(1) test\n");
            success = false;
        }

        ComPtr<ID3D11Buffer> vb2;
        if (FAILED(CreateStaticBuffer(device, s_vertexData, std::size(s_vertexData),
            D3D11_BIND_VERTEX_BUFFER, vb2.GetAddressOf())))
        {
            printf("ERROR: Failed CreateStaticBuffer(2) test\n");
            success = false;
        }

        ComPtr<ID3D11Buffer> vb3;
        std::vector<VertexPositionColor> verts(s_vertexData, s_vertexData + std::size(s_vertexData));

        if (FAILED(CreateStaticBuffer(device, verts, D3D11_BIND_VERTEX_BUFFER, vb3.GetAddressOf())))
        {
            printf("ERROR: Failed CreateStaticBuffer(3) test\n");
            success = false;
        }

        if (device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
        {
            ComPtr<ID3D11Buffer> vb4;
            if (FAILED(CreateStaticBuffer(device, verts, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS,
                vb4.GetAddressOf())))
            {
                printf("ERROR: Failed CreateStaticBuffer(4) test\n");
                success = false;
            }
        }
    }

    return success;
}
