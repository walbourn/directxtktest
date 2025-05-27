//--------------------------------------------------------------------------------------
// File: directxhelpers.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "DirectXHelpers.h"

#include "Effects.h"
#include "VertexTypes.h"

#include <cmath>
#include <cstdio>
#include <iterator>
#include <random>

#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    inline bool CheckIsPowerOf2(size_t x) noexcept
    {
        if (!x)
            return false;

        return (ceil(log2(x)) == float(log2(x)));
    }

    inline uint64_t CheckAlignUp(uint64_t size, size_t alignment) noexcept
    {
        return ((size + alignment - 1) / alignment) * alignment;
    }

    inline uint64_t CheckAlignDown(uint64_t size, size_t alignment) noexcept
    {
        return (size / alignment) * alignment;
    }
}

_Success_(return)
bool Test03(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    bool success = true;

    std::random_device rd;
    std::default_random_engine generator(rd());

    // IsPowerOf2
    {
        for (size_t j = 0; j < 0x20000; ++j)
        {
            if (IsPowerOf2(j) != CheckIsPowerOf2(j))
            {
                printf("ERROR: Failed IsPowerOf2 tests\n");
                success = false;
            }
        }
    }

    // AlignUp/Down - uint32_t
    {
        std::uniform_int_distribution<uint32_t> dist(1, UINT16_MAX);
        for (size_t j = 1; j < 0x20000; j <<= 1)
        {
            if (!IsPowerOf2(j))
            {
                printf("ERROR: Failed IsPowerOf2 Align(32)\n");
                success = false;
            }

            for (size_t k = 0; k < 20000; k++)
            {
                uint32_t value = dist(generator);
                uint32_t up = AlignUp(value, j);
                uint32_t down = AlignDown(value, j);
                auto upCheck = static_cast<uint32_t>(CheckAlignUp(value, j));
                auto downCheck = static_cast<uint32_t>(CheckAlignDown(value, j));

                if (!up)
                {
                    printf("ERROR: Failed AlignUp(32) tests\n");
                    success = false;
                }
                else if (!down && value > j)
                {
                    printf("ERROR: Failed AlignDown(32) tests\n");
                    success = false;
                }
                else if (up < down)
                {
                    printf("ERROR: Failed Align(32) tests\n");
                    success = false;
                }
                else if (value > up)
                {
                    printf("ERROR: Failed AlignUp(32) tests\n");
                    success = false;
                }
                else if (value < down)
                {
                    printf("ERROR: Failed AlignDown(32) tests\n");
                    success = false;
                }
                else if (up != upCheck)
                {
                    printf("ERROR: Failed AlignUp(32) tests\n");
                    success = false;
                }
                else if (down != downCheck)
                {
                    printf("ERROR: Failed AlignDown(32) tests\n");
                    success = false;
                }
            }
        }
    }

    // AlignUp/Down - uint64_t
    {
        std::uniform_int_distribution<uint64_t> dist(1, UINT32_MAX);
        for (size_t j = 1; j < 0x20000; j <<= 1)
        {
            if (!IsPowerOf2(j))
            {
                printf("ERROR: Failed IsPowerOf2 Align(64)\n");
                success = false;
            }

            for (size_t k = 0; k < 20000; k++)
            {
                uint64_t value = dist(generator);
                uint64_t up = AlignUp(value, j);
                uint64_t down = AlignDown(value, j);
                uint64_t upCheck = CheckAlignUp(value, j);
                uint64_t downCheck = CheckAlignDown(value, j);

                if (!up)
                {
                    printf("ERROR: Failed AlignUp(64) tests\n");
                    success = false;
                }
                else if (!down && value > j)
                {
                    printf("ERROR: Failed AlignDown(64) tests\n");
                    success = false;
                }
                else if (up < down)
                {
                    printf("ERROR: Failed Align(64) tests\n");
                    success = false;
                }
                else if (value > up)
                {
                    printf("ERROR: Failed AlignUp(64) tests\n");
                    success = false;
                }
                else if (value < down)
                {
                    printf("ERROR: Failed AlignDown(64) tests\n");
                    success = false;
                }
                else if (up != upCheck)
                {
                    printf("ERROR: Failed AlignUp(64) tests\n");
                    success = false;
                }
                else if (down != downCheck)
                {
                    printf("ERROR: Failed AlignDown(64) tests\n");
                    success = false;
                }
            }
        }
    }

    // CreateInputLayoutFromEffect
    {
        static const D3D11_INPUT_ELEMENT_DESC s_inputElementDesc[2] =
        {
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA,  0 },
            { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA , 0 },
        };

        std::unique_ptr<BasicEffect> effect;
        try
        {
            effect = std::make_unique<BasicEffect>(device);
            effect->SetVertexColorEnabled(true);
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed creating required effect object (except: %s)\n", e.what());
            return false;
        }

        ComPtr<ID3D11InputLayout> il;
        HRESULT hr = CreateInputLayoutFromEffect(device,
            effect.get(),
            s_inputElementDesc, std::size(s_inputElementDesc),
            il.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: Failed CreateInputLayoutFromEffect(1) test (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }

        ComPtr<ID3D11InputLayout> il2;
        hr = CreateInputLayoutFromEffect<VertexPositionColor>(device,
            effect.get(),
            il2.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: Failed CreateInputLayoutFromEffect(2) test (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    return success;
}
