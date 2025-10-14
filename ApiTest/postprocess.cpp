//--------------------------------------------------------------------------------------
// File: postprocess.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "PostProcess.h"

#include <cstdio>
#include <type_traits>

#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

static_assert(!std::is_copy_constructible<BasicPostProcess>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<BasicPostProcess>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<BasicPostProcess>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<BasicPostProcess>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<DualPostProcess>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<DualPostProcess>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<DualPostProcess>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DualPostProcess>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<ToneMapPostProcess>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<ToneMapPostProcess>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<ToneMapPostProcess>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ToneMapPostProcess>::value, "Move Assign.");

_Success_(return)
bool Test06(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    bool success = true;

    {
        std::unique_ptr<BasicPostProcess> basic;
        try
        {
            basic = std::make_unique<BasicPostProcess>(device);
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed creating object (except: %s)\n", e.what());
            success = false;
        }

        #if 0
        for(size_t j = 0; j < BasicPostProcess::Effect_Max; ++j)
        {
            // TODO: basic->Process
        }
        #endif
    }

    {
        std::unique_ptr<DualPostProcess> dual;
        try
        {
            dual = std::make_unique<DualPostProcess>(device);
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed creating dual object (except: %s)\n", e.what());
            success = false;
        }

        #if 0
        for(size_t j = 0; j < 2; ++j)
        {
            // j ? DualPostProcess::BloomCombine : DualPostProcess::Merge);
            // TODO: dual->Process
        }
        #endif
    }

    {
        std::unique_ptr<ToneMapPostProcess> toneMap;
        try
        {
            toneMap = std::make_unique<ToneMapPostProcess>(device);
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed creating tonemap object (except: %s)\n", e.what());
            success = false;
        }

        #if 0
        for(size_t j = 0; j < ToneMapPostProcess::Operator_Max; ++j)
        {
            // TODO: toneMap->Process ToneMapPostProcess::SRGB
            // TODO: toneMap->Process ToneMapPostProcess::ST2084
        }
        #endif
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        ID3D11Device* nullDevice = nullptr;

        try
        {
            auto invalid = std::make_unique<BasicPostProcess>(nullDevice);

            printf("ERROR: Failed to throw invalid device [basic]\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto basic = std::make_unique<BasicPostProcess>(device);

            basic->SetEffect(BasicPostProcess::Effect_Max);

            printf("ERROR: Failed to throw invalid fx [basic]\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<DualPostProcess>(nullDevice);

            printf("ERROR: Failed to throw invalid device [dual]\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto dual = std::make_unique<DualPostProcess>(device);

            dual->SetEffect(DualPostProcess::Effect_Max);

            printf("ERROR: Failed to throw invalid fx [dual]\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<ToneMapPostProcess>(nullDevice);

            printf("ERROR: Failed to throw invalid device [tonemap]\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto tonemap = std::make_unique<ToneMapPostProcess>(device);

            tonemap->SetOperator(ToneMapPostProcess::Operator_Max);

            printf("ERROR: Failed to throw invalid operator [tonemap]\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto tonemap = std::make_unique<ToneMapPostProcess>(device);

            tonemap->SetTransferFunction(ToneMapPostProcess::TransferFunction_Max);

            printf("ERROR: Failed to throw invalid transfer func [tonemap]\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}
