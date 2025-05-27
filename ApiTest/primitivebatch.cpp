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

static_assert(std::is_nothrow_move_constructible<PrimitiveBatch<VertexPositionColor>>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<PrimitiveBatch<VertexPositionColor>>::value, "Move Assign.");

_Success_(return)
bool Test07(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    using Vertex = VertexPositionColor;

    std::unique_ptr<PrimitiveBatch<Vertex>> batch;
    try
    {
        batch = std::make_unique<PrimitiveBatch<Vertex>>(context.Get());
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        return false;
    }

    return true;
}
