//--------------------------------------------------------------------------------------
// File: sprites.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "SpriteBatch.h"
#include "SpriteFont.h"

#include <cstdio>
#include <iterator>
#include <type_traits>
#include <vector>

#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

static_assert(std::is_nothrow_move_constructible<SpriteBatch>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SpriteBatch>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<SpriteFont>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SpriteFont>::value, "Move Assign.");

// SpriteBatch
_Success_(return)
bool Test08(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    std::unique_ptr<SpriteBatch> batch;
    try
    {
        batch = std::make_unique<SpriteBatch>(context.Get());
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        return false;
    }

    return true;
}

// SpriteFont
_Success_(return)
bool Test09(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    static const wchar_t* s_fonts[] =
    {
        L"SpriteFontTest\\comic.spritefont",
        L"SpriteFontTest\\italic.spritefont",
        L"SpriteFontTest\\script.spritefont",
        L"SpriteFontTest\\nonproportional.spritefont",
        L"SpriteFontTest\\multicolored.spritefont",
        L"SpriteFontTest\\japanese.spritefont",
        L"SpriteFontTest\\xboxController.spritefont",
        L"SpriteFontTest\\xboxOneController.spritefont",
        L"SpriteFontTest\\consolas.spritefont",
    };

    std::vector<std::unique_ptr<SpriteFont>> fonts;

    bool success = true;

    for(size_t j = 0; j < std::size(s_fonts); j++)
    {
        try
        {
            auto font = std::make_unique<SpriteFont>(device, s_fonts[j]);
            fonts.emplace_back(std::move(font));
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed creating %ls object (except: %s)\n", s_fonts[j], e.what());
            success = false;
        }
    }

    for(size_t j = 0; j < std::size(s_fonts); ++j)
    {
        if (!fonts[j])
            continue;

    #ifndef NO_WCHAR_T
        if (fonts[j]->GetDefaultCharacter() != 0)
        {
            printf("FAILED: GetDefaultCharacter for %ls\n", s_fonts[j]);
            success = false;
        }
    #endif

        if (fonts[j]->GetLineSpacing() == 0)
        {
            printf("FAILED: GetLineSpacing for %ls\n", s_fonts[j]);
            success = false;
        }

        if (fonts[j]->ContainsCharacter(637))
        {
            printf("FAILED: ContainsCharacter for %ls\n", s_fonts[j]);
            success = false;
        }

        ComPtr<ID3D11ShaderResourceView> sheet;
        fonts[j]->GetSpriteSheet(sheet.GetAddressOf());
        if (!sheet)
        {
            printf("FAILED: GetSpriteSheet for %ls\n", s_fonts[j]);
            success = false;
        }
    }

    return success;
}
