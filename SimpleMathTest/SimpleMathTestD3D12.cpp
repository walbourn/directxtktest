//-------------------------------------------------------------------------------------
// SimpleMathTestD3D12.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//  
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <d3d12.h>
#include "SimpleMath.h"

#pragma warning(push)
#pragma warning(disable : 26495)
#include "d3dx12.h"
#pragma warning(pop)

#include "SimpleMathTest.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

int TestD3D12()
{
    // Viewport
    bool success = true;

    {
        CD3DX12_VIEWPORT d3d12vp(23.f, 42.f, 666.f, 1234.f, 0.f, 1.f);
        Viewport vp(d3d12vp);

        if (!XMScalarNearEqual(vp.x, d3d12vp.TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d12vp.TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d12vp.Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d12vp.Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d12vp.MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d12vp.MaxDepth, EPSILON))
        {
            printf("ERROR: D3D12_VIEWPORT ctor\n");
            success = false;
        }
    }

    {
        CD3DX12_VIEWPORT d3d12vp(23.f, 42.f, 666.f, 1234.f, 0.f, 1.f);
        Viewport vp;
        vp = d3d12vp;

        if (!XMScalarNearEqual(vp.x, d3d12vp.TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d12vp.TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d12vp.Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d12vp.Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d12vp.MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d12vp.MaxDepth, EPSILON))
        {
            printf("ERROR: D3D12_VIEWPORT =\n");
            success = false;
        }
    }

    Viewport vp(23.f, 42.f, 666.f, 1234.f);
    {
        D3D12_VIEWPORT d3d12vp = vp;

        if (!XMScalarNearEqual(vp.x, d3d12vp.TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d12vp.TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d12vp.Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d12vp.Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d12vp.MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d12vp.MaxDepth, EPSILON))
        {
            printf("ERROR: operator D3D12_VIEWPORT\n");
            success = false;
        }
    }

    {
        const D3D12_VIEWPORT* d3d12vp = vp.Get12();

        if (!XMScalarNearEqual(vp.x, d3d12vp->TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d12vp->TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d12vp->Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d12vp->Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d12vp->MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d12vp->MaxDepth, EPSILON))
        {
            printf("ERROR: Get\n");
            success = false;
        }
    }

    return (success) ? 0 : 1;
}
