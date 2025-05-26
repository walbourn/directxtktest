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

static_assert(std::is_nothrow_move_constructible<SpriteBatch>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SpriteBatch>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<SpriteFont>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SpriteFont>::value, "Move Assign.");

// SpriteBatch
bool Test07(ID3D11Device *device)
{
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
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
bool Test08(ID3D11Device *device)
{
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
            return false;
        }
    }

    return true;
}
