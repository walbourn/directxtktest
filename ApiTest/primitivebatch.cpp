//--------------------------------------------------------------------------------------
// File: primitivebatch.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "PrimitiveBatch.h"

#include "VertexTypes.h"

#include <cstdio>
#include <type_traits>

#include <wrl/client.h>

using namespace DirectX;

using Vertex = DirectX::VertexPositionColor;

static_assert(!std::is_copy_constructible<PrimitiveBatch<Vertex>>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<PrimitiveBatch<Vertex>>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<PrimitiveBatch<Vertex>>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<PrimitiveBatch<Vertex>>::value, "Move Assign.");

_Success_(return)
bool Test07(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    bool success = true;

    using Vertex = VertexPositionColor;

    std::unique_ptr<PrimitiveBatch<Vertex>> batch;
    try
    {
        batch = std::make_unique<PrimitiveBatch<Vertex>>(context.Get());
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        success = false;
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        try
        {
            ID3D11DeviceContext* nullContext = nullptr;
            auto invalid = std::make_unique<PrimitiveBatch<Vertex>>(nullContext, 0, 0);

            printf("ERROR: Failed to throw on null device context\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<PrimitiveBatch<Vertex>>(context.Get(), 0, 0);

            printf("ERROR: Failed to throw on zero max verts\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            struct BigVertex
            {
                char buffer[4096];
            };

            auto invalid = std::make_unique<PrimitiveBatch<BigVertex>>(context.Get(), 4096 * 3, 4096);

            printf("ERROR: Failed to throw on too big vert\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<PrimitiveBatch<Vertex>>(context.Get(), INT32_MAX, 4096);

            printf("ERROR: Failed to throw on too many indices\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<PrimitiveBatch<Vertex>>(context.Get(), 4096 * 3, INT32_MAX);

            printf("ERROR: Failed to throw on too many verts\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}
