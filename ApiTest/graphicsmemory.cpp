//--------------------------------------------------------------------------------------
// File: graphicsmemory.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "GraphicsMemory.h"

#include <cstdio>
#include <type_traits>

using namespace DirectX;

static_assert(std::is_nothrow_move_constructible<GraphicsMemory>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<GraphicsMemory>::value, "Move Assign.");

using namespace DirectX;

bool Test04(ID3D11Device *device)
{
    std::unique_ptr<GraphicsMemory> graphicsMemory;
    try
    {
        graphicsMemory = std::make_unique<GraphicsMemory>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        return false;
    }

    return true;
}