//-------------------------------------------------------------------------------------
// SimpleMathTestD3D11.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <d3d11.h>
#include "SimpleMath.h"

#include "SimpleMathTest.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

#ifdef __MINGW32__
struct CD3D11_VIEWPORT : public D3D11_VIEWPORT
{
    CD3D11_VIEWPORT() = default;
    explicit CD3D11_VIEWPORT( const D3D11_VIEWPORT& o ) :
        D3D11_VIEWPORT( o )
    {}
    explicit CD3D11_VIEWPORT(
        FLOAT topLeftX,
        FLOAT topLeftY,
        FLOAT width,
        FLOAT height,
        FLOAT minDepth = 0.f /*D3D11_MIN_DEPTH*/,
        FLOAT maxDepth = 1.f /*D3D11_MAX_DEPTH*/)
    {
        TopLeftX = topLeftX;
        TopLeftY = topLeftY;
        Width = width;
        Height = height;
        MinDepth = minDepth;
        MaxDepth = maxDepth;
    }
};

inline bool operator==( const D3D11_RECT& l, const D3D11_RECT& r )
{
    return l.left == r.left && l.top == r.top &&
        l.right == r.right && l.bottom == r.bottom;
}
inline bool operator!=( const D3D11_RECT& l, const D3D11_RECT& r )
{ return !( l == r ); }
#endif // __MINGW32_

int TestD3D11()
{
    bool success = true;

    // RECT
    Rectangle smallRect(50, 75, 100, 200);
    Rectangle bigRect(15, 32, 1920, 1080);

    {
        RECT smallRct = { 50, 75, 100 + 50, 200 + 75 };

        if (smallRct != smallRect)
        {
            printf("ERROR: RECT != small\n");
            success = false;
        }

        RECT bigRct = { 15, 32, 1920 + 15, 1080 + 32 };

        if (bigRct != bigRect)
        {
            printf("ERROR: RECT != big\n");
            success = false;
        }
    }

    // Viewport
    {
        CD3D11_VIEWPORT d3d11vp(23.f, 42.f, 666.f, 1234.f, 0.f, 1.f);
        Viewport vp(d3d11vp);

        if (!XMScalarNearEqual(vp.x, d3d11vp.TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d11vp.TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d11vp.Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d11vp.Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d11vp.MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d11vp.MaxDepth, EPSILON))
        {
            printf("ERROR: D3D11_VIEWPORT ctor\n");
            success = false;
        }
    }

    {
        CD3D11_VIEWPORT d3d11vp(23.f, 42.f, 666.f, 1234.f, 0.f, 1.f);
        Viewport vp;
        vp = d3d11vp;

        if (!XMScalarNearEqual(vp.x, d3d11vp.TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d11vp.TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d11vp.Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d11vp.Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d11vp.MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d11vp.MaxDepth, EPSILON))
        {
            printf("ERROR: D3D11_VIEWPORT =\n");
            success = false;
        }
    }

    Viewport vp(23.f, 42.f, 666.f, 1234.f);

    {
        D3D11_VIEWPORT d3d11vp = vp;

        if (!XMScalarNearEqual(vp.x, d3d11vp.TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d11vp.TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d11vp.Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d11vp.Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d11vp.MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d11vp.MaxDepth, EPSILON))
        {
            printf("ERROR: operator D3D11_VIEWPORT\n");
            success = false;
        }
    }

    {
        const D3D11_VIEWPORT* d3d11vp = vp.Get11();

        if (!XMScalarNearEqual(vp.x, d3d11vp->TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d11vp->TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d11vp->Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d11vp->Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d11vp->MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d11vp->MaxDepth, EPSILON))
        {
            printf("ERROR: Get11\n");
            success = false;
        }
    }

    return (success) ? 0 : 1;
}
