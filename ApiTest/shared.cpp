//--------------------------------------------------------------------------------------
// File: shared.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "GamePad.h"
#include "Keyboard.h"
#include "Mouse.h"

#include <cstdio>

#include <d3d11_1.h>

using namespace DirectX;

_Success_(return)
bool Test16(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    std::unique_ptr<GamePad> pad;
    try
    {
        pad = std::make_unique<GamePad>();
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        return false;
    }

    return true;
}

_Success_(return)
bool Test17(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    std::unique_ptr<Keyboard> kb;
    try
    {
        kb = std::make_unique<Keyboard>();
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        return false;
    }

    return true;
}

_Success_(return)
bool Test18(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    std::unique_ptr<Mouse> mouse;
    try
    {
        mouse = std::make_unique<Mouse>();
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        return false;
    }

    return true;
}
