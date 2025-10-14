//--------------------------------------------------------------------------------------
// File: vertextypes.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "VertexTypes.h"

#include "DirectXHelpers.h"
#include "Effects.h"

#include <cstdio>
#include <type_traits>

#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

//--------------------------------------------------------------------------------------
// As of DirectXMath 3.13, these types are is_nothrow_copy/move_assignable

// VertexPosition
static_assert(std::is_nothrow_default_constructible<VertexPosition>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPosition>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPosition>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPosition>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPosition>::value, "Move Assign.");

// VertexPositionColor
static_assert(std::is_nothrow_default_constructible<VertexPositionColor>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionColor>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionColor>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionColor>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionColor>::value, "Move Assign.");

// VertexPositionTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionTexture>::value, "Move Assign.");

// VertexPositionDualTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionDualTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionDualTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionDualTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionDualTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionDualTexture>::value, "Move Assign.");

// VertexPositionNormal
static_assert(std::is_nothrow_default_constructible<VertexPositionNormal>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormal>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormal>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormal>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormal>::value, "Move Assign.");

// VertexPositionColorTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionColorTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionColorTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionColorTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionColorTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionColorTexture>::value, "Move Assign.");

// VertexPositionNormalColor
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalColor>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalColor>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalColor>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalColor>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalColor>::value, "Move Assign.");

// VertexPositionNormalTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalTexture>::value, "Move Assign.");

// VertexPositionNormalColorTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalColorTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalColorTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalColorTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalColorTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalColorTexture>::value, "Move Assign.");

// VertexPositionNormalTangentColorTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalTangentColorTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalTangentColorTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalTangentColorTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalTangentColorTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalTangentColorTexture>::value, "Move Assign.");

// VertexPositionNormalTangentColorTextureSkinning
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalTangentColorTextureSkinning>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalTangentColorTextureSkinning>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalTangentColorTextureSkinning>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalTangentColorTextureSkinning>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalTangentColorTextureSkinning>::value, "Move Assign.");

namespace
{
    template<class T>
    _Success_(return)
    inline bool TestVertexType(_In_ ID3D11Device *device, _In_ IEffect* effect)
    {
        static_assert(T::InputElementCount > 0, "element count must be non-zero");
        static_assert(T::InputElementCount <= 32 /* D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT */, "element count is too large");

        if (_stricmp(T::InputElements[0].SemanticName, "SV_Position") != 0)
        {
            return false;
        }

        for (size_t j = 0; j < T::InputElementCount; ++j)
        {
            if (T::InputElements[j].SemanticName == nullptr)
                return false;
        }

        ComPtr<ID3D11InputLayout> inputLayout;
        HRESULT hr = CreateInputLayoutFromEffect<T>(device, effect, inputLayout.GetAddressOf());
        if (FAILED(hr))
            return false;

        return true;
    }
}

_Success_(return)
bool Test10(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    bool success = true;

    std::unique_ptr<BasicEffect> effect;
    try
    {
        effect = std::make_unique<BasicEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating required effect object (except: %s)\n", e.what());
        return false;
    }

    // VertexPosition
    {
        VertexPosition verts = { XMFLOAT3(1.f, 2.f, .3f ) };

        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f)
        {
            printf("ERROR: VertexPosition data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPosition>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPosition tests\n");
        success = false;
    }

    // VertexPositionColor
    {
        VertexPositionColor verts = { XMFLOAT3(1.f, 2.f, .3f), XMFLOAT4(0.5f, 0.25f, 0.75f, 1.f) };

        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.color.x != 0.5f
            || verts.color.y != 0.25f
            || verts.color.z != 0.75f
            || verts.color.w != 1.f)
        {
            printf("ERROR: VertexPositionColor data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionColor>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionColor tests\n");
        success = false;
    }

    // VertexPositionTexture
    {
        VertexPositionTexture verts = { XMFLOAT3(1.f, 2.f, .3f), XMFLOAT2(0.5f, 0.25f) };

        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.textureCoordinate.x != 0.5f
            || verts.textureCoordinate.y != 0.25f)
        {
            printf("ERROR: VertexPositionTexture data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionTexture>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionTexture tests\n");
        success = false;
    }

    // VertexPositionDualTexture
    {
        VertexPositionDualTexture verts = { XMFLOAT3(1.f, 2.f, .3f), XMFLOAT2(0.5f, 0.25f), XMFLOAT2(0.75f, 0.125f) };

        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.textureCoordinate0.x != 0.5f
            || verts.textureCoordinate0.y != 0.25f
            || verts.textureCoordinate1.x != 0.75f
            || verts.textureCoordinate1.y != 0.125f)
        {
            printf("ERROR: VertexPositionDualTexture data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionDualTexture>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionDualTexture tests\n");
        success = false;
    }

    // VertexPositionNormal
    {
        VertexPositionNormal verts = { XMFLOAT3(1.f, 2.f, .3f), XMFLOAT3(0.5f, 0.25f, 0.75f) };

        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.normal.x != 0.5f
            || verts.normal.y != 0.25f
            || verts.normal.z != 0.75f)
        {
            printf("ERROR: VertexPositionNormal data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionNormal>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionNormal tests\n");
        success = false;
    }

    // VertexPositionColorTexture
    {
        VertexPositionColorTexture verts = { XMFLOAT3(1.f, 2.f, .3f), XMFLOAT4(0.5f, 0.25f, 0.75f, 1.f), XMFLOAT2(0.1f, 0.2f) };
        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.color.x != 0.5f
            || verts.color.y != 0.25f
            || verts.color.z != 0.75f
            || verts.color.w != 1.f
            || verts.textureCoordinate.x != 0.1f
            || verts.textureCoordinate.y != 0.2f)
        {
            printf("ERROR: VertexPositionColorTexture data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionColorTexture>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionColorTexture tests\n");
        success = false;
    }

    // VertexPositionNormalColor
    {
        VertexPositionNormalColor verts = { XMFLOAT3(1.f, 2.f, .3f), XMFLOAT3(0.5f, 0.25f, 0.75f), XMFLOAT4(0.1f, 0.2f, 0.3f, 0.4f) };
        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.normal.x != 0.5f
            || verts.normal.y != 0.25f
            || verts.normal.z != 0.75f
            || verts.color.x != 0.1f
            || verts.color.y != 0.2f
            || verts.color.z != 0.3f
            || verts.color.w != 0.4f)
        {
            printf("ERROR: VertexPositionNormalColor data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionNormalColor>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionNormalColor tests\n");
        success = false;
    }

    // VertexPositionNormalTexture
    {
        VertexPositionNormalTexture verts = { XMFLOAT3(1.f, 2.f, .3f), XMFLOAT3(0.5f, 0.25f, 0.75f), XMFLOAT2(0.1f, 0.2f) };
        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.normal.x != 0.5f
            || verts.normal.y != 0.25f
            || verts.normal.z != 0.75f
            || verts.textureCoordinate.x != 0.1f
            || verts.textureCoordinate.y != 0.2f)
        {
            printf("ERROR: VertexPositionNormalTexture data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionNormalTexture>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionNormalTexture tests\n");
        success = false;
    }

    // VertexPositionNormalColorTexture
    {
        VertexPositionNormalColorTexture verts = { XMFLOAT3(1.f, 2.f, .3f), XMFLOAT3(0.5f, 0.25f, 0.75f), XMFLOAT4(0.1f, 0.2f, 0.3f, 0.4f), XMFLOAT2(0.1f, 0.2f) };
        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.normal.x != 0.5f
            || verts.normal.y != 0.25f
            || verts.normal.z != 0.75f
            || verts.color.x != 0.1f
            || verts.color.y != 0.2f
            || verts.color.z != 0.3f
            || verts.color.w != 0.4f
            || verts.textureCoordinate.x != 0.1f
            || verts.textureCoordinate.y != 0.2f)
        {
            printf("ERROR: VertexPositionNormalColorTexture data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionNormalColorTexture>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionNormalColorTexture tests\n");
        success = false;
    }

    // VertexPositionNormalTangentColorTexture
    {
        VertexPositionNormalTangentColorTexture verts =
        {
            XMFLOAT3(1.f, 2.f, .3f), XMFLOAT3(0.5f, 0.25f, 0.75f), XMFLOAT4(0.1f, 0.2f, 0.3f, 0.4f),
            0xAABBCCDD, XMFLOAT2(0.1f, 0.2f)
        };

        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.normal.x != 0.5f
            || verts.normal.y != 0.25f
            || verts.normal.z != 0.75f
            || verts.tangent.x != 0.1f
            || verts.tangent.y != 0.2f
            || verts.tangent.z != 0.3f
            || verts.tangent.w != 0.4f
            || verts.color != 0xAABBCCDD
            || verts.textureCoordinate.x != 0.1f
            || verts.textureCoordinate.y != 0.2f)
        {
            printf("ERROR: VertexPositionNormalTangentColorTexture data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionNormalTangentColorTexture>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionNormalTangentColorTexture tests\n");
        success = false;
    }

    // VertexPositionNormalTangentColorTextureSkinning
    {
        VertexPositionNormalTangentColorTextureSkinning verts =
        {
            XMFLOAT3(1.f, 2.f, .3f), XMFLOAT3(0.5f, 0.25f, 0.75f), XMFLOAT4(0.1f, 0.2f, 0.3f, 0.4f),
            0xAABBCCDD, XMFLOAT2(0.1f, 0.2f), XMUINT4(1,2,3,4), XMFLOAT4(0.1f,0.2f,0.3f,0.4f)
        };

        if (verts.position.x != 1.f
            || verts.position.y != 2.f
            || verts.position.z != .3f
            || verts.normal.x != 0.5f
            || verts.normal.y != 0.25f
            || verts.normal.z != 0.75f
            || verts.tangent.x != 0.1f
            || verts.tangent.y != 0.2f
            || verts.tangent.z != 0.3f
            || verts.tangent.w != 0.4f
            || verts.color != 0xAABBCCDD
            || verts.textureCoordinate.x != 0.1f
            || verts.textureCoordinate.y != 0.2f
            || verts.indices != 0x04030201
            || verts.weights != 0x664C3319)
        {
            printf("ERROR: VertexPositionNormalTangentColorTextureSkinning data incorrect\n");
            success = false;
        }
    }

    if (!TestVertexType<VertexPositionNormalTangentColorTextureSkinning>(device, effect.get()))
    {
        printf("ERROR: Failed VertexPositionNormalTangentColorTextureSkinning tests\n");
        success = false;
    }

    return success;
}
