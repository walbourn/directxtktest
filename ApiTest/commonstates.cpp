//--------------------------------------------------------------------------------------
// File: commonstates.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "CommonStates.h"

#include <cstdio>
#include <type_traits>

#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

static_assert(!std::is_copy_constructible<CommonStates>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<CommonStates>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<CommonStates>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<CommonStates>::value, "Move Assign.");

_Success_(return)
bool Test02(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    std::unique_ptr<CommonStates> states;
    try
    {
        states = std::make_unique<CommonStates>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        return false;
    }

    bool success = true;

    if (states->Opaque() == nullptr
        || states->AlphaBlend() == nullptr
        || states->Additive() == nullptr
        || states->NonPremultiplied() == nullptr)
    {
        printf("ERROR: Failed CommonStates blend state tests\n");
        success = false;
    }

    if (states->DepthNone() == nullptr
        || states->DepthDefault() == nullptr
        || states->DepthRead() == nullptr
        || states->DepthReverseZ() == nullptr
        || states->DepthReadReverseZ() == nullptr)
    {
        printf("ERROR: Failed CommonStates depth/stencil state tests\n");
        success = false;
    }

    if (states->CullNone() == nullptr
        || states->CullClockwise() == nullptr
        || states->CullCounterClockwise() == nullptr
        || states->Wireframe() == nullptr)
    {
        printf("ERROR: Failed CommonStates rasterizer state tests\n");
        success = false;
    }

    if (states->PointWrap() == nullptr
        || states->PointClamp() == nullptr
        || states->LinearWrap() == nullptr
        || states->LinearClamp() == nullptr
        || states->AnisotropicWrap() == nullptr
        || states->AnisotropicClamp() == nullptr)
    {
        printf("ERROR: Failed CommonStates sampler state tests\n");
        success = false;
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        ID3D11Device* nullDevice = nullptr;
        try
        {
            auto invalid = std::make_unique<CommonStates>(nullDevice);

            printf("ERROR: Failed to throw for null device\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}
