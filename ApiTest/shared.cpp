//--------------------------------------------------------------------------------------
// File: shared.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include <unknwn.h>

#include "BinaryReader.h"
#include "GamePad.h"
#include "Keyboard.h"
#include "Mouse.h"

#include <cstdio>
#include <type_traits>

#include <d3d11_1.h>

using namespace DirectX;

static_assert(!std::is_copy_constructible<BinaryReader>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<BinaryReader>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<BinaryReader>::value, "Move Ctor.");
static_assert(std::is_move_assignable<BinaryReader>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<GamePad>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<GamePad>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<GamePad>::value, "Move Ctor.");
static_assert(std::is_move_assignable<GamePad>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<Keyboard>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<Keyboard>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<Keyboard>::value, "Move Ctor.");
static_assert(std::is_move_assignable<Keyboard>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<Mouse>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<Mouse>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<Mouse>::value, "Move Ctor.");
static_assert(std::is_move_assignable<Mouse>::value, "Move Assign.");

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


#ifndef DIRECTX_TOOLKIT_IMPORT

_Success_(return)
bool Test20(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    bool success = true;

    {
        std::unique_ptr<uint8_t[]> data;
        size_t size = 0;
        HRESULT hr = BinaryReader::ReadEntireFile(L"SpriteFontTest\\xboxController.spritefont", data, &size);
        if (FAILED(hr) || (size != 1032680))
        {
            printf("ERROR: Failed to load test file %zu .. 1032680 (HRESULT: %08X)\n", size, static_cast<unsigned int>(hr));
            success = false;
        }
    }

    try
    {
        auto reader = std::make_unique<BinaryReader>(L"SpriteFontTest\\xboxController.spritefont");
    }
    catch(const std::exception& e)
    {
        printf("ERROR: BinaryReader fname ctor failed (except: %s)\n", e.what());
        success = false;
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        std::unique_ptr<uint8_t[]> data;
        size_t size = 0;
        HRESULT hr = BinaryReader::ReadEntireFile(nullptr, data, &size);
        if (hr != E_INVALIDARG)
        {
            printf("ERROR: Expected failure for null filename (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        hr = BinaryReader::ReadEntireFile(L"TestFileNotExist.dat", data, &size);
        if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            printf("ERROR: Expected failure for missing file (HRESULT: %08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        try
        {
            auto reader = std::make_unique<BinaryReader>(nullptr);

            printf("ERROR: Failed to throw for null filename\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto reader = std::make_unique<BinaryReader>(L"TestFileNotExist.dat");

            printf("ERROR: Failed to throw for missing file\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }

    return success;
}

#endif // !DIRECTX_TOOLKIT_IMPORT
