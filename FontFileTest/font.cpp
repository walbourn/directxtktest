//-------------------------------------------------------------------------------------
// font.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <Windows.h>

#ifdef __MINGW32__
#include <unknwn.h>
#endif

#include "BinaryReader.h"
#include "SpriteFont.h"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>

using namespace DirectX;

namespace
{
    struct TestMedia
    {
        uint32_t    glyphCount;
        float       lineSpacing;
        uint32_t    defaultChar;
        uint32_t    textureWidth;
        uint32_t    textureHeight;
        DXGI_FORMAT textureFormat;
        uint32_t    textureStride;
        uint32_t    textureRows;
        const wchar_t *fname;
    };

    const TestMedia g_TestMedia[] =
    {
        // GlyphCount | LineSpacing | DefaultChar | TexW | TexH | Format | Stride | Rows | Filename
        { 95, 42.7f, 0, 256, 172, DXGI_FORMAT_BC2_UNORM, 1024, 43, L"SpriteFontTest\\comic.spritefont" },
        { 332, 15.6f, 0, 256, 128, DXGI_FORMAT_BC2_UNORM, 1024, 32, L"SpriteFontTest\\consolas.spritefont" },
        { 95, 56.8f, 0, 256, 288, DXGI_FORMAT_BC2_UNORM, 1024, 72, L"SpriteFontTest\\italic.spritefont" },
        { 26, 36.0f, 0, 128, 132, DXGI_FORMAT_BC2_UNORM, 512, 33, L"SpriteFontTest\\japanese.spritefont" },
        { 95, 47.0f, 0, 256, 160, DXGI_FORMAT_R8G8B8A8_UNORM, 1024, 160, L"SpriteFontTest\\multicolored.spritefont" },
        { 37, 18.1f, 0, 64, 68, DXGI_FORMAT_BC2_UNORM, 256, 17, L"SpriteFontTest\\nonproportional.spritefont" },
        { 95, 38.0f, 0, 256, 128, DXGI_FORMAT_BC2_UNORM, 1024, 32, L"SpriteFontTest\\script.spritefont" },
        { 14, 186.0f, 0, 512, 504, DXGI_FORMAT_R8G8B8A8_UNORM, 2048, 504, L"SpriteFontTest\\xboxController.spritefont" },
        { 14, 102.0f, 0, 512, 276, DXGI_FORMAT_R8G8B8A8_UNORM, 2048, 276, L"SpriteFontTest\\XboxOneController.spritefont" },
    };

    const char* GetFormatName(DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_BC2_UNORM: return "BC2_UNORM";
        case DXGI_FORMAT_BC2_UNORM_SRGB: return "BC2_UNORM_SRGB";
        case DXGI_FORMAT_B8G8R8A8_UNORM: return "B8G8R8A8_UNORM";
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return "B8G8R8A8_UNORM_SRGB";
        case DXGI_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8_UNORM";
        case DXGI_FORMAT_R8_UNORM: return "R8_UNORM";
        default: return "*UNKNOWN*";
        }
    }

    bool TestSpriteFontParsing(BinaryReader& reader, size_t index, const wchar_t* szPath) noexcept(false)
    {
        // Needs to match parsing logic in SpriteFont::Impl ctor

        // Validate magic header "DXTKfont"
        static const char spriteFontMagic[] = "DXTKfont";
        for (const char* magic = spriteFontMagic; *magic; magic++)
        {
            if (reader.Read<uint8_t>() != *magic)
            {
                printf( "Invalid magic header in spritefont file:\n%ls\n", szPath );
                return false;
            }
        }

        // Read glyph data
        auto glyphCount = reader.Read<uint32_t>();
        auto glyphData = reader.ReadArray<SpriteFont::Glyph>(glyphCount);

        if (glyphCount != g_TestMedia[index].glyphCount)
        {
            printf( "Glyph count mismatch in spritefont file (expected %u, got %u):\n%ls\n",
                g_TestMedia[index].glyphCount, glyphCount, szPath );
            return false;
        }

        // Verify glyphs are sorted by character code
        if (glyphCount > 1)
        {
            for (uint32_t i = 1; i < glyphCount; ++i)
            {
                if (glyphData[i].Character <= glyphData[i-1].Character)
                {
                    printf( "Glyphs not sorted at index %u (char %u <= %u):\n%ls\n",
                        i, glyphData[i].Character, glyphData[i-1].Character, szPath );
                    return false;
                }
            }
        }

        // Read font properties
        auto lineSpacing = reader.Read<float>();
        auto defaultChar = reader.Read<uint32_t>();

        // Allow small epsilon for floating-point comparison
        float diff = lineSpacing - g_TestMedia[index].lineSpacing;
        if (diff < -0.1f || diff > 0.1f)
        {
            printf( "Line spacing mismatch (expected %.1f, got %.1f):\n%ls\n",
                g_TestMedia[index].lineSpacing, lineSpacing, szPath );
            return false;
        }

        if (defaultChar != g_TestMedia[index].defaultChar)
        {
            printf( "Default character mismatch (expected %u, got %u):\n%ls\n",
                g_TestMedia[index].defaultChar, defaultChar, szPath );
            return false;
        }

        // Read texture metadata
        auto textureWidth = reader.Read<uint32_t>();
        auto textureHeight = reader.Read<uint32_t>();
        auto textureFormat = reader.Read<DXGI_FORMAT>();
        auto textureStride = reader.Read<uint32_t>();
        auto textureRows = reader.Read<uint32_t>();

        if (textureWidth != g_TestMedia[index].textureWidth
            || textureHeight != g_TestMedia[index].textureHeight)
        {
            printf( "Texture dimensions mismatch (expected %ux%u, got %ux%u):\n%ls\n",
                g_TestMedia[index].textureWidth, g_TestMedia[index].textureHeight,
            textureWidth, textureHeight, szPath );
            return false;
        }

        if (textureFormat != g_TestMedia[index].textureFormat)
        {
            printf( "Texture format mismatch (expected %s, got %s):\n%ls\n",
                GetFormatName(g_TestMedia[index].textureFormat),
                GetFormatName(textureFormat), szPath );
            return false;
        }

        if (textureStride != g_TestMedia[index].textureStride
            || textureRows != g_TestMedia[index].textureRows)
        {
            printf( "Texture layout mismatch (expected stride %u rows %u, got stride %u rows %u):\n%ls\n",
                g_TestMedia[index].textureStride, g_TestMedia[index].textureRows,
                textureStride, textureRows, szPath );
            return false;
        }

        // Validate texture data is present
        const uint64_t dataSize = uint64_t(textureStride) * uint64_t(textureRows);
        if (dataSize > UINT32_MAX)
        {
            printf( "Texture data size overflow:\n%ls\n", szPath );
            return false;
        }
        else
        {
            auto textureData = reader.ReadArray<uint8_t>(static_cast<size_t>(dataSize));
            if (!textureData)
            {
                printf( "Failed reading texture data:\n%ls\n", szPath );
                return false;
            }
        }

        return true;
    }
}

//-------------------------------------------------------------------------------------
//
bool Test01()
{
    bool success = true;

    size_t ncount = 0;
    size_t npass = 0;

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

        bool pass = true;

        // From file
        try
        {
            BinaryReader reader(szPath);

            if (!TestSpriteFontParsing(reader, index, szPath))
            {
                success = false;
                pass = false;
            }
        }
        catch (const std::exception& e)
        {
            success = false;
            pass = false;
            printf( "C++ Exception loading spritefont from file (except: %s):\n%ls\n", e.what(), szPath );
        }
        catch (...)
        {
            success = false;
            pass = false;
            printf( "Unknown C++ Exception loading spritefont from file:\n%ls\n", szPath );
        }

        // From memory
        std::vector<uint8_t> rawData;
        {
            std::ifstream inFile(szPath, std::ios::in | std::ios::binary | std::ios::ate);
            if (inFile)
            {
                std::streamsize size = inFile.tellg();
                inFile.seekg(0, std::ios::beg);
                rawData.resize(static_cast<size_t>(size));
                if (!inFile.read(reinterpret_cast<char*>(rawData.data()), size))
                {
                    rawData.clear();
                }
            }

            if (!rawData.empty())
            {
                try
                {
                    BinaryReader reader(rawData.data(), rawData.size());

                    if (!TestSpriteFontParsing(reader, index, szPath))
                    {
                        success = false;
                        pass = false;
                    }
                }
                catch (const std::exception& e)
                {
                    success = false;
                    pass = false;
                    printf( "C++ Exception parsing spritefont file (except: %s):\n%ls\n", e.what(), szPath );
                }
                catch (...)
                {
                    success = false;
                    pass = false;
                    printf( "Unknown C++ Exception parsing spritefont file:\n%ls\n", szPath );
                }
            }
        }

        if (pass)
            ++npass;
        ++ncount;
    }

    printf("%zu files tested, %zu files passed ", ncount, npass );

    // Invalid argument tests using BinaryReader
    {
        // Empty data
        try
        {
            const uint8_t emptyData[] = { 0 };
            BinaryReader reader(emptyData, 0);
            // Attempting to read from empty data should throw
            bool threw = false;
            try
            {
                reader.Read<uint8_t>();
            }
            catch (const std::runtime_error&)
            {
                threw = true;
            }
            if (!threw)
            {
                printf("\nERROR: Expected exception reading from empty BinaryReader\n");
                success = false;
            }
        }
        catch (const std::exception& e)
        {
            printf("\nERROR: Unexpected exception for empty data test (except: %s)\n", e.what());
            success = false;
        }
        catch (...)
        {
            printf("\nERROR: Unknown C++ Exception for empty data test\n");
            success = false;
        }

        // Invalid magic header
        try
        {
            const uint8_t badMagic[] = { 'N', 'O', 'T', 'f', 'o', 'n', 't', '!', 0, 0, 0, 0 };
            BinaryReader reader(badMagic, sizeof(badMagic));
            auto firstByte = reader.Read<uint8_t>();
            if (firstByte == 'D')
            {
                printf("\nERROR: Bad magic data matched expected header\n");
                success = false;
            }
        }
        catch (const std::exception& e)
        {
            printf("\nERROR: Unexpected exception for bad magic test (except: %s)\n", e.what());
            success = false;
        }
        catch (...)
        {
            printf("\nERROR: Unknown C++ Exception for bad magic test\n");
            success = false;
        }

        // Truncated file (valid magic but no glyph data)
        try
        {
            const uint8_t truncatedData[] = { 'D', 'X', 'T', 'K', 'f', 'o', 'n', 't' };
            BinaryReader reader(truncatedData, sizeof(truncatedData));

            // Validate magic passes
            static const char spriteFontMagic[] = "DXTKfont";
            bool magicOk = true;
            for (const char* magic = spriteFontMagic; *magic; magic++)
            {
                if (reader.Read<uint8_t>() != *magic)
                {
                    magicOk = false;
                    break;
                }
            }

            if (!magicOk)
            {
                printf("\nERROR: Valid magic not recognized in truncated data test\n");
                success = false;
            }
            else
            {
                // Reading past end should throw
                bool threw = false;
                try
                {
                    reader.Read<uint32_t>();
                }
                catch (const std::runtime_error&)
                {
                    threw = true;
                }
                if (!threw)
                {
                    printf("\nERROR: Expected exception reading past end of truncated spritefont\n");
                    success = false;
                }
            }
        }
        catch (const std::exception& e)
        {
            printf("\nERROR: Unexpected exception for truncated file test (except: %s)\n", e.what());
            success = false;
        }
        catch (...)
        {
            printf("\nERROR: Unknown C++ Exception for truncated file test\n");
            success = false;
        }
    }

    return success;
}
