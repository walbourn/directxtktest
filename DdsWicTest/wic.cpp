//-------------------------------------------------------------------------------------
// wic.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#define NODRAWTEXT
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <Windows.h>
#include <wrl/client.h>

#include "WICTextureLoader.h"

#include <cstdio>
#include <cwchar>
#include <stdexcept>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

#define DXTEX_MEDIA_PATH L"%DIRECTXTEX_MEDIA_PATH%\\"

namespace
{
    struct TestMedia
    {
        uint32_t width;
        uint32_t height;
        DXGI_FORMAT format;
        const wchar_t *fname;
        uint8_t md5[16];
    };

    const TestMedia g_TestMedia[] =
    {
        // Width | Height | Format | Filename | MD5Hash
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, L"EffectsTest\\spheremap.bmp", {} },
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, L"LoadTest\\win95.bmp", {} },
        { 512, 256, DXGI_FORMAT_R8G8B8A8_UNORM, L"PostProcessTest\\earth.bmp", {} },

        { 1280, 1024, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, L"LoadTest\\testpattern.png", {} },
        { 13, 20, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB , L"MouseTest\\arrow.png", {} },
        { 2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\BrokenCube_baseColor.png", {} },
        { 2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\BrokenCube_emissive.png", {} },
        { 2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\BrokenCube_normal.png", {} },
        { 2048, 2048, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\BrokenCube_occlusionRoughnessMetallic.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\Sphere2Mat_baseColor.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\Sphere2Mat_emissive.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\Sphere2Mat_normal.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\Sphere2Mat_occlusionRoughnessMetallic.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\SphereMat_baseColor.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\SphereMat_normal.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"PBRTest\\SphereMat_occlusionRoughnessMetallic.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"ShaderTest\\Sphere2Mat_baseColor.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"ShaderTest\\Sphere2Mat_emissive.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"ShaderTest\\Sphere2Mat_normal.png", {} },
        { 1024, 1024, DXGI_FORMAT_R8G8B8A8_UNORM, L"ShaderTest\\Sphere2Mat_occlusionRoughnessMetallic.png", {} },
        { 464, 338, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, L"SpriteFontTest\\multicolored.png", {} },
        { 1456, 104, DXGI_FORMAT_B8G8R8A8_UNORM, L"SpriteFontTest\\xboxOneControllerSpriteFont.PNG", {} },

        { 512, 683, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, L"LoadTest\\cup_small.jpg", {} },
        { 512, 256, DXGI_FORMAT_R8G8B8A8_UNORM, L"ModelTest\\cup.jpg", {} },
        { 800, 480, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, L"PostProcessTest\\sunset.jpg", {} },

        { 1512, 359, DXGI_FORMAT_R8_UNORM, L"LoadTest\\text.tif", {} },

        // DirectXTex test corpus (optional)
        { 200, 200, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"fishingboat.jpg", {} },
        { 200, 200, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"lena.jpg", {} },
        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"baboon.tiff", {} },
        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"f16.tiff", {} },
        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"peppers.tiff", {} },
        { 1024, 1024, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"pentagon.tiff", {} },

        { 512, 512, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"cameraman.tif", {} },
        { 512, 512, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"house.tif", {} },
        { 512, 512, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"jetplane.tif", {} },
        { 512, 512, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"lake.tif", {} },
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"lena_color_256.tif", {} },
        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"lena_color_512.tif", {} },
        { 256, 256, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"lena_gray_256.tif", {} },
        { 512, 512, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"lena_gray_512.tif", {} },
        { 512, 512, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"livingroom.tif", {} },
        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"mandril_color.tif", {} },
        { 512, 512, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"mandril_gray.tif", {} },
        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"peppers_color.tif", {} },
        { 512, 512, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"peppers_gray.tif", {} },
        { 512, 512, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"pirate.tif", {} },
        { 512, 512, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"walkbridge.tif", {} },
        { 512, 512, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"woman_blonde.tif", {} },
        { 512, 512, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"woman_darkhair.tif", {} },

        // PNG Test Suite sample files
        { 32, 32, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"BASN0G01.PNG", {} },
        { 32, 32, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"BASN0G02.PNG", {} },
        { 32, 32, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"BASN0G04.PNG", {} },
        { 32, 32, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"BASN0G08.PNG", {} },
        { 32, 32, DXGI_FORMAT_R16_UNORM, DXTEX_MEDIA_PATH L"BASN0G16.PNG", {} },
        { 32, 32, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"BASN2C08.PNG", {} },
        { 32, 32, DXGI_FORMAT_R16G16B16A16_UNORM, DXTEX_MEDIA_PATH L"BASN2C16.PNG", {} },
        { 32, 32, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"BASN3P01.PNG", {} },
        { 32, 32, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"BASN3P02.PNG", {} },
        { 32, 32, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"BASN3P04.PNG", {} },
        { 32, 32, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"BASN3P08.PNG", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"BASN4A08.PNG", {} },
        { 32, 32, DXGI_FORMAT_R16G16B16A16_UNORM, DXTEX_MEDIA_PATH L"BASN4A16.PNG", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"BASN6A08.PNG", {} },
        { 32, 32, DXGI_FORMAT_R16G16B16A16_UNORM, DXTEX_MEDIA_PATH L"BASN6A16.PNG", {} },

        // Windows BMP
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"tex1.bmp", {} },
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"tex2.bmp", {} },
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"tex3.bmp", {} },
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"tex4.bmp", {} },
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"tex5.bmp", {} },
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"tex6.bmp", {} },
        { 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"tex7.bmp", {} },
        { 32, 32, DXGI_FORMAT_B5G5R5A1_UNORM, DXTEX_MEDIA_PATH L"grad4d_a1r5g5b5.bmp", {} },
        { 32, 32, DXGI_FORMAT_B5G5R5A1_UNORM, DXTEX_MEDIA_PATH L"grad4d_a1r5g5b5_flip.bmp", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"grad4d_a4r4g4b4.bmp", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"grad4d_a4r4g4b4_flip.bmp", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8X8_UNORM /*not a V5 header*/, DXTEX_MEDIA_PATH L"grad4d_a8r8g8b8.bmp", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8X8_UNORM /*not a V5 header*/, DXTEX_MEDIA_PATH L"grad4d_a8r8g8b8_flip.bmp", {} },
        { 32, 32, DXGI_FORMAT_B5G6R5_UNORM, DXTEX_MEDIA_PATH L"grad4d_r5g6b5.bmp", {} },
        { 32, 32, DXGI_FORMAT_B5G6R5_UNORM, DXTEX_MEDIA_PATH L"grad4d_r5g6b5_flip.bmp", {} },
        { 32, 32, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"grad4d_r8g8b8.bmp", {} },
        { 32, 32, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"grad4d_r8g8b8_flip.bmp", {} },
        { 32, 32, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"grad4d_r8g8b8os2.bmp", {} },
        { 32, 32, DXGI_FORMAT_B5G5R5A1_UNORM, DXTEX_MEDIA_PATH L"grad4d_x1r5g5b5.bmp", {} },
        { 32, 32, DXGI_FORMAT_B5G5R5A1_UNORM, DXTEX_MEDIA_PATH L"grad4d_x1r5g5b5_flip.bmp", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"grad4d_x4r4g4b4.bmp", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"grad4d_x4r4g4b4_flip.bmp", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"grad4d_x8r8g8b8.bmp", {} },
        { 32, 32, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"grad4d_x8r8g8b8_flip.bmp", {} },

        // BMP Test Suite
        { 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-1x1.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-320x240-color.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-320x240-overlappingcolor.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-320x240.bmp", {} },
        { 321, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-321x240.bmp", {} },
        { 322, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-322x240.bmp", {} },
        { 323, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-323x240.bmp", {} },
        { 324, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-324x240.bmp", {} },
        { 325, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-325x240.bmp", {} },
        { 326, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-326x240.bmp", {} },
        { 327, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-327x240.bmp", {} },
        { 328, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-328x240.bmp", {} },
        { 329, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-329x240.bmp", {} },
        { 330, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-330x240.bmp", {} },
        { 331, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-331x240.bmp", {} },
        { 332, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-332x240.bmp", {} },
        { 333, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-333x240.bmp", {} },
        { 334, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-334x240.bmp", {} },
        { 335, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-335x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"1bpp-topdown-320x240.bmp", {} },
        { 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-1x1.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-320x240.bmp", {} },
        { 321, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-321x240.bmp", {} },
        { 322, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-322x240.bmp", {} },
        { 323, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-323x240.bmp", {} },
        { 324, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-324x240.bmp", {} },
        { 325, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-325x240.bmp", {} },
        { 326, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-326x240.bmp", {} },
        { 327, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-327x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"4bpp-topdown-320x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rle4-absolute-320x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rle4-alternate-320x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rle4-delta-320x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rle4-encoded-320x240.bmp", {} },
        { 16384 /* resized to 16k */, 1, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rle8-64000x1.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rle8-absolute-320x240.bmp", {} },
        { 160, 120, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rle8-blank-160x120.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rle8-delta-320x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rle8-encoded-320x240.bmp", {} },
        { 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"8bpp-1x1.bmp", {} },
        { 1, 16384 /* resized to 16k */, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"8bpp-1x64000.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"8bpp-320x240.bmp", {} },
        { 321, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"8bpp-321x240.bmp", {} },
        { 322, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"8bpp-322x240.bmp", {} },
        { 323, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"8bpp-323x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"8bpp-colorsimportant-two.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"8bpp-colorsused-zero.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"8bpp-topdown-320x240.bmp", {} },
        { 1, 1, DXGI_FORMAT_B5G5R5A1_UNORM, DXTEX_MEDIA_PATH L"555-1x1.bmp", {} },
        { 320, 240, DXGI_FORMAT_B5G5R5A1_UNORM, DXTEX_MEDIA_PATH L"555-320x240.bmp", {} },
        { 321, 240, DXGI_FORMAT_B5G5R5A1_UNORM, DXTEX_MEDIA_PATH L"555-321x240.bmp", {} },
        { 1, 1, DXGI_FORMAT_B5G6R5_UNORM, DXTEX_MEDIA_PATH L"565-1x1.bmp", {} },
        { 320, 240, DXGI_FORMAT_B5G6R5_UNORM, DXTEX_MEDIA_PATH L"565-320x240-topdown.bmp", {} },
        { 320, 240, DXGI_FORMAT_B5G6R5_UNORM, DXTEX_MEDIA_PATH L"565-320x240.bmp", {} },
        { 321, 240, DXGI_FORMAT_B5G6R5_UNORM, DXTEX_MEDIA_PATH L"565-321x240-topdown.bmp", {} },
        { 321, 240, DXGI_FORMAT_B5G6R5_UNORM, DXTEX_MEDIA_PATH L"565-321x240.bmp", {} },
        { 322, 240, DXGI_FORMAT_B5G6R5_UNORM, DXTEX_MEDIA_PATH L"565-322x240-topdown.bmp", {} },
        { 322, 240, DXGI_FORMAT_B5G6R5_UNORM, DXTEX_MEDIA_PATH L"565-322x240.bmp", {} },
        { 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"24bpp-1x1.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"24bpp-320x240.bmp", {} },
        { 321, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"24bpp-321x240.bmp", {} },
        { 322, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"24bpp-322x240.bmp", {} },
        { 323, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"24bpp-323x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"24bpp-imagesize-zero.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"24bpp-topdown-320x240.bmp", {} },
        { 1, 1, DXGI_FORMAT_B8G8R8X8_UNORM /*not a V5 header*/, DXTEX_MEDIA_PATH L"32bpp-1x1.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM /*not a V5 header*/, DXTEX_MEDIA_PATH L"32bpp-320x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM /*not a V5 header*/, DXTEX_MEDIA_PATH L"32bpp-101110-320x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM /*not a V5 header*/, DXTEX_MEDIA_PATH L"32bpp-888-optimalpalette-320x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM /*not a V5 header*/, DXTEX_MEDIA_PATH L"32bpp-optimalpalette-320x240.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8X8_UNORM /*not a V5 header*/, DXTEX_MEDIA_PATH L"32bpp-topdown-320x240.bmp", {} },
        { 1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"32bpp-1x1v5.bmp", {} },
        { 320, 240, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"32bpp-320x240v5.bmp", {} },
        { 320, 240, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"spaces in  filename.bmp", {} },

        // LibTiff test images
        // caspian.tif, dscf0013.tif, off_l16.tif, off_luv24.tif, off_luv32.tif, and ycbcr-cat.tif are not supported by WIC
        { 800, 607, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"cramps-tile.tif", {} },
        { 800, 607, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"cramps.tif", {} },
        { 1728, 1082, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"fax2d.tif", {} },
        { 1728, 1103, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"g3test.tif", {} },
        { 256, 192, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"jello.tif", {} },
        { 664, 813, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"jim___ah.tif", {} },
        { 277, 339, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"jim___cg.tif", {} },
        { 277, 339, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"jim___dg.tif", {} },
        { 277, 339, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"jim___gg.tif", {} },
        { 158, 118, DXGI_FORMAT_R16_UNORM, DXTEX_MEDIA_PATH L"ladoga.tif", {} },
        { 601, 81, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"oxford.tif", {} },
        { 640, 480, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"pc260001.tif", {} },
        { 512, 384, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"quad-jpeg.tif", {} },
        { 512, 384, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"quad-lzw.tif", {} },
        { 512, 384, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"quad-tile.tif", {} },
        { 160, 160, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"smallliz.tif", {} },
        { 256, 200, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"strike.tif", {} },
        { 234, 213, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"zackthecat.tif", {} },

        // Kodak Lossless True Color Image Suite
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim01.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim02.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim03.png", {} },
        { 512, 768, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim04.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim05.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim06.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim07.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim08.png", {} },
        { 512, 768, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim09.png", {} },
        { 512, 768, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim10.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim11.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim12.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim13.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim14.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim15.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim16.png", {} },
        { 512, 768, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim17.png", {} },
        { 512, 768, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim18.png", {} },
        { 512, 768, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim19.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim20.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim21.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim22.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim23.png", {} },
        { 768, 512, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"kodim24.png", {} },

        // JPEG
        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"rocks.jpg", {} },
        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"wall.jpg", {} },
        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"wood.jpg", {} },
        { 512, 768, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"memorial.jpg", {} },

        // Direct2D Test Images
        { 300, 227, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"32bppRGBAI.png", {} },
        { 300, 227, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"32bppRGBAN.png", {} },
        { 4096, 4096, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"PNG-interlaced-compression9-24bit-4096x4096.png", {} },
        { 203, 203, DXGI_FORMAT_B8G8R8A8_UNORM, DXTEX_MEDIA_PATH L"transparent_clock.png", {} },

        { 564, 749, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"Bad5.bmp", {} },
        { 640, 480, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"COUGAR.BMP", {} },
        { 200, 200, DXGI_FORMAT_B8G8R8X8_UNORM, DXTEX_MEDIA_PATH L"rgb32table888td.bmp", {} },

        { 1024, 768, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"Dock.jpg", {} },
        { 640, 480, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"image.127839287370267572.jpg", {} },
        { 500, 500, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"progressivehuffman.jpg", {} },

        { 73, 43, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"flower-minisblack-06.tif", {} },
        { 760, 399, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"H3_06.TIF", {} },

        { 1920, 1200, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"abstract-test-pattern.jpg", {} },
        { 256, 224, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"deltae_base.png", {} },
        { 768, 576, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"Grayscale_Staircase.png", {} },

        { 512, 512, DXGI_FORMAT_R8G8B8A8_UNORM, DXTEX_MEDIA_PATH L"Omega-SwanNebula-M17.png", {} },

        // WIC test suite
        { 550, 481, DXGI_FORMAT_R32G32B32A32_FLOAT, DXTEX_MEDIA_PATH L"Desert 128bpp BGRA.tif", {} },
        { 550, 481, DXGI_FORMAT_R32G32B32A32_FLOAT, DXTEX_MEDIA_PATH L"Desert 128bpp RGBA - converted to pRGBA.tif", {} },
        // TODO - GUID_WICPixelFormat32bppPBGRA (tiff), GUID_WICPixelFormat64bppPRGBA (tiff)

        // sRGB test cases
        { 64, 24, DXGI_FORMAT_R8_UNORM, DXTEX_MEDIA_PATH L"p99sr.png", {} },
        { 976, 800, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"chip_lagrange_o3_400_sigm_6p5x50.png", {} },
        { 800, 600, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"livingrobot-rear.tiff", {} },
        { 2048, 2048, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"tex4.png", {} },
        { 16384 /* resized to 16k */, 8192, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"earthdiffuse.png", {} },
        { 16384, 8192, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"earthdiffuseTexture.png", {} },
        { 8192, 4096, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"callisto.png", {} },

        // Additional TIF test cases
        { 2048, 1024, DXGI_FORMAT_R16G16B16A16_UNORM, DXTEX_MEDIA_PATH L"SnowPano_4k_Ref.TIF", {} }, // 48bppRGB

        #if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/) || defined(_WIN7_PLATFORM_UPDATE)
        { 1024, 1024, DXGI_FORMAT_R32G32B32_FLOAT, DXTEX_MEDIA_PATH L"ramps_vdm_rel.TIF", {} },
        { 768, 512, DXGI_FORMAT_R32G32B32_FLOAT, DXTEX_MEDIA_PATH L"96bpp_RGB_FP.TIF", {} },
        #endif

        #ifdef _M_X64
        // Very large images
        { 16384, 16384, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXTEX_MEDIA_PATH L"earth16kby16k.png", {} },
        #endif
    };

    void printdesc(const D3D11_TEXTURE2D_DESC & desc)
    {
        // For WIC, MipLevels=ArraySize=1 and MiscFlags=0
        printf("%ux%u format %u\n", desc.Width, desc.Height, desc.Format);
    }
}

//-------------------------------------------------------------------------------------

extern HRESULT MD5Checksum( _In_reads_(dataSize) const uint8_t *data, size_t dataSize, _Out_bytecap_x_(16) uint8_t *digest );

//-------------------------------------------------------------------------------------
//
bool Test02(_In_ ID3D11Device* pDevice)
{
    bool success = true;

    size_t ncount = 0;
    size_t npass = 0;

    bool skipped = false;

    for( size_t index=0; index < std::size(g_TestMedia); ++index )
    {
        wchar_t szPath[MAX_PATH] = {};
        DWORD ret = ExpandEnvironmentStringsW(g_TestMedia[index].fname, szPath, MAX_PATH);
        if ( !ret || ret > MAX_PATH )
        {
            printf( "ERROR: ExpandEnvironmentStrings FAILED\n" );
            return false;
        }

#ifdef _DEBUG
        OutputDebugString(szPath);
        OutputDebugStringA("\n");
#endif

        ComPtr<ID3D11Resource> res;
        HRESULT hr = CreateWICTextureFromFileEx(
            pDevice,
            szPath,
            0,
            D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0,
            WIC_LOADER_DEFAULT,
            res.GetAddressOf(), nullptr);
        if ( FAILED(hr) )
        {
            if (((hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) || (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)))
                && wcsstr(g_TestMedia[index].fname, DXTEX_MEDIA_PATH) != nullptr)
            {
                // DIRECTX_TEX_MEDIA test cases are optional
                skipped = true;
                continue;
            }

            success = false;
            printf( "ERROR: Failed loading WIC from file (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
        }
        else if (!res.Get())
        {
            success = false;
            printf( "ERROR: Failed to return resource (HRESULT %08X):\n%ls\n", static_cast<unsigned int>(hr), szPath );
        }
        else
        {
            bool pass = false;

            D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
            res->GetType(&dimension);

            if (dimension != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
            {
                success = false;
                printf( "ERROR: Unexpected resource dimension (%u..3)\n%ls\n", dimension, szPath );
            }

            ComPtr<ID3D11Texture2D> tex;
            hr = res.As(&tex);
            if (SUCCEEDED(hr))
            {
                D3D11_TEXTURE2D_DESC desc = {};
                tex->GetDesc(&desc);

                if (desc.Width != g_TestMedia[index].width
                    || desc.Height != g_TestMedia[index].height
                    || desc.MipLevels != 1
                    || desc.ArraySize != 1
                    || desc.Format != g_TestMedia[index].format)
                {
                    success = false;
                    printf( "ERROR: Unexpected resource metadata\n%ls\n", szPath );
                    printdesc(desc);
                    printf("...\n");

                    const D3D11_TEXTURE2D_DESC expected = {
                        g_TestMedia[index].width, g_TestMedia[index].height,
                        1,
                        1,
                        g_TestMedia[index].format, {},
                        D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0 };
                    printdesc(expected);
                }
                else
                {
                    // TODO: md5?
                    pass = true;
                }
            }

            if (FAILED(hr))
            {
                success = false;
                printf( "ERROR: Failed to obtain 2D texture desc (%08X)\n%ls\n", static_cast<unsigned int>(hr), szPath );
            }

            if (pass)
                ++npass;
        }

        ++ncount;
    }

    if (skipped)
    {
        printf("\nSkipped DIRECTX_TEX_MEDIA cases...\n");
    }

    printf("%zu files tested, %zu files passed ", ncount, npass );

    return success;
}
