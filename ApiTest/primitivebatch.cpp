//--------------------------------------------------------------------------------------
// File: primtitivebatch.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "PrimitiveBatch.h"
#include <VertexTypes.h>

#include <type_traits>

using namespace DirectX;

static_assert(std::is_nothrow_move_constructible<PrimitiveBatch<VertexPositionColor>>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<PrimitiveBatch<VertexPositionColor>>::value, "Move Assign.");


