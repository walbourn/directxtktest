//--------------------------------------------------------------------------------------
// File: loaderhelpers.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP

#include <dxgiformat.h>
#include <wincodec.h>
#include <wrl/client.h>

#include "LoaderHelpers.h"

#include <algorithm>
#include <cstdio>
#include <iterator>

using namespace DirectX;
using namespace DirectX::LoaderHelpers;

namespace
{
    const UINT DXGI_START = 1;
    const UINT DXGI_END = 115;
}

_Success_(return)
bool Test19(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    bool success = true;

    static const DXGI_FORMAT s_bcFmts[] =
    {
        DXGI_FORMAT_BC1_TYPELESS, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB,
        DXGI_FORMAT_BC2_TYPELESS, DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB,
        DXGI_FORMAT_BC3_TYPELESS, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB,
        DXGI_FORMAT_BC4_TYPELESS, DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM,
        DXGI_FORMAT_BC5_TYPELESS, DXGI_FORMAT_BC5_UNORM, DXGI_FORMAT_BC5_SNORM,
        DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16,
        DXGI_FORMAT_BC7_TYPELESS, DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB
    };

    static const DXGI_FORMAT s_hasSRGB[] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM,
        DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC7_UNORM,
    };

    static const DXGI_FORMAT s_hasLinear[] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
        DXGI_FORMAT_BC1_UNORM_SRGB, DXGI_FORMAT_BC2_UNORM_SRGB, DXGI_FORMAT_BC3_UNORM_SRGB, DXGI_FORMAT_BC7_UNORM_SRGB,
    };

    static const DXGI_FORMAT s_typeless[] =
    {
        DXGI_FORMAT_R32G32B32A32_TYPELESS, DXGI_FORMAT_R32G32B32_TYPELESS, DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R32G32_TYPELESS,
        DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R32_TYPELESS,
        DXGI_FORMAT_R10G10B10A2_TYPELESS, DXGI_FORMAT_R8G8_TYPELESS, DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R8_TYPELESS,
        DXGI_FORMAT_BC1_TYPELESS, DXGI_FORMAT_BC2_TYPELESS, DXGI_FORMAT_BC3_TYPELESS, DXGI_FORMAT_BC4_TYPELESS, DXGI_FORMAT_BC5_TYPELESS,
        DXGI_FORMAT_B8G8R8A8_TYPELESS, DXGI_FORMAT_B8G8R8X8_TYPELESS, DXGI_FORMAT_BC7_TYPELESS,
    };


    for (UINT f = DXGI_START; f <= DXGI_END; ++f )
    {
        auto df = static_cast<DXGI_FORMAT>(f);

        bool isCompressed = std::find(std::begin(s_bcFmts), std::end(s_bcFmts), df) != std::end(s_bcFmts);
        bool isTypeless = std::find(std::begin(s_typeless), std::end(s_typeless), df) != std::end(s_typeless);
        bool hasSRGB = std::find(std::begin(s_hasSRGB), std::end(s_hasSRGB), df) != std::end(s_hasSRGB);
        bool hasLinear = std::find(std::begin(s_hasLinear), std::end(s_hasLinear), df) != std::end(s_hasLinear);

        if (BitsPerPixel(df) == 0)
        {
            printf("ERROR: BitsPerPixel failed on DXGI Format %u\n", f);
            success = false;
        }

        if (IsCompressed(df) != isCompressed)
        {
            printf("ERROR: IsCompressed failed on DXGI Format %u\n", f);
            success = false;
        }

        if (hasSRGB)
        {
            if ( MakeSRGB( df ) == df )
            {
                printf("ERROR: MakeSRGB failed on DXGI Format %u\n", f);
                success = false;
            }
        }
        else if ( MakeSRGB( df ) != df )
        {
            printf("ERROR: MakeSRGB failed on DXGI Format %u\n", f);
            success = false;
        }

        if (hasLinear)
        {
            if ( MakeLinear( df ) == df )
            {
                printf("ERROR: MakeLinear failed on DXGI Format %u\n", f);
                success = false;
            }
        }
        else if ( MakeLinear( df ) != df )
        {
            printf("ERROR: MakeLinear failed on DXGI Format %u\n", f);
            success = false;
        }

        if (isTypeless)
        {
            if ( EnsureNotTypeless( df ) == df )
            {
                printf("ERROR: EnsureNotTypeless failed on DXGI Format %u\n", f);
                success = false;
            }
        }
        else if ( EnsureNotTypeless( df ) != df )
        {
            printf("ERROR: EnsureNotTypeless failed on DXGI Format %u\n", f);
            success = false;
        }
    }

    UINT targetx = 100;
    UINT targety = 100;
    FitPowerOf2( 1024, 768, targetx, targety, 65535 );
    if ( (targetx != 63) || (targety != 63) )
    {
        printf("ERROR: FitPowerOf2 failed on 100x100 -> %ux%u\n", targetx, targety);
        success = false;
    }

    targetx = 200;
    targety = 100;
    FitPowerOf2( 1024, 768, targetx, targety, 65535 );
    if ( (targetx != 127) || (targety != 127) )
    {
        printf("ERROR: FitPowerOf2 failed on 200x100 -> %ux%u\n", targetx, targety);
        success = false;
    }

    targetx = 100;
    targety = 200;
    FitPowerOf2( 768, 1024, targetx, targety, 65535 );
    if ( (targetx != 127) || (targety != 127) )
    {
        printf("ERROR: FitPowerOf2 failed on 100x200 -> %ux%u\n", targetx, targety);
        success = false;
    }

    return success;
}
