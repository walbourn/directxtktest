//--------------------------------------------------------------------------------------
// File: model.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "Model.h"

#include "Effects.h"

#include <cstdio>
#include <iterator>

#include <wrl/client.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

static_assert(std::is_copy_constructible<Model>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<Model>::value, "Copy Assign.");
static_assert(std::is_move_constructible<Model>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<Model>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<ModelMesh>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<ModelMesh>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<ModelMesh>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ModelMesh>::value, "Move Assign.");

static_assert(std::is_copy_constructible<ModelMeshPart>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<ModelMeshPart>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<ModelMeshPart>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ModelMeshPart>::value, "Move Assign.");

static_assert(std::is_copy_constructible<ModelBone>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<ModelBone>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<ModelBone>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ModelBone>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<ModelBone::TransformArray>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<ModelBone::TransformArray>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<ModelBone::TransformArray>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ModelBone::TransformArray>::value, "Move Assign.");

_Success_(return)
bool Test13(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    std::unique_ptr<EffectFactory> fxFactory;
    try
    {
        fxFactory = std::make_unique<EffectFactory>(device);

        fxFactory->SetDirectory(L"ModelTest");
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating effects factory object (except: %s)\n", e.what());
        return false;
    }

    // Create model
    bool success = true;

    std::unique_ptr<Model> vbo;
    try
    {
        // Calls the memory version
        vbo = Model::CreateFromVBO(device, L"ModelTest\\player_ship_a.vbo", nullptr);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating model from VBO (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<Model> cmo;
    try
    {
        // Calls the memory version
        cmo = Model::CreateFromCMO(device, L"ModelTest\\teapot.cmo", *fxFactory);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating model from CMO (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<Model> sdkmesh;
    try
    {
        // Calls the memory version
        sdkmesh = Model::CreateFromSDKMESH(device, L"ModelTest\\cup.sdkmesh", *fxFactory);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating model from SDKMESH (except: %s)\n", e.what());
        success = false;
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        try
        {
            ID3D11Device* nullDevice = nullptr;
            auto invalid = std::make_unique<EffectFactory>(nullDevice);

            printf("ERROR: Failed to throw for null device for effect factory\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        // VBO
        try
        {
            ID3D11Device* nullDevice = nullptr;
            const wchar_t* nullFilename = nullptr;
            auto invalid = Model::CreateFromVBO(nullDevice, nullFilename, nullptr);

            printf("ERROR: Failed to throw for null device for VBO\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            const wchar_t* nullFilename = nullptr;
            auto invalid = Model::CreateFromVBO(device, nullFilename, nullptr);

            printf("ERROR: Failed to throw for null filename for VBO\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = Model::CreateFromVBO(device, L"TestFileNotExist.vbo", nullptr);

            printf("ERROR: Failed to throw for missing file for VBO\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            ID3D11Device* nullDevice = nullptr;
            const uint8_t* ptr = nullptr;
            auto invalid = Model::CreateFromVBO(nullDevice, ptr, 0, nullptr);

            printf("ERROR: Failed to throw for null device for VBO memory\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        // CMO
        try
        {
            ID3D11Device* nullDevice = nullptr;
            const wchar_t* nullFilename = nullptr;
            auto invalid = Model::CreateFromCMO(nullDevice, nullFilename, *fxFactory);

            printf("ERROR: Failed to throw for null device for CMO\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            const wchar_t* nullFilename = nullptr;
            auto invalid = Model::CreateFromCMO(device, nullFilename, *fxFactory);

            printf("ERROR: Failed to throw for null filename for CMO\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = Model::CreateFromCMO(device, L"TestFileNotExist.CMO", *fxFactory);

            printf("ERROR: Failed to throw for missing file for CMO\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            ID3D11Device* nullDevice = nullptr;
            const uint8_t* ptr = nullptr;
            auto invalid = Model::CreateFromCMO(nullDevice, ptr, 0, *fxFactory);

            printf("ERROR: Failed to throw for null device for CMO memory\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        // SDKMESH
        try
        {
            ID3D11Device* nullDevice = nullptr;
            const wchar_t* nullFilename = nullptr;
            auto invalid = Model::CreateFromSDKMESH(nullDevice, nullFilename, *fxFactory);

            printf("ERROR: Failed to throw for null device for SDKMESH\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            const wchar_t* nullFilename = nullptr;
            auto invalid = Model::CreateFromSDKMESH(device, nullFilename, *fxFactory);

            printf("ERROR: Failed to throw for null filename for SDKMESH\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = Model::CreateFromSDKMESH(device, L"TestFileNotExist.SDKMESH", *fxFactory);

            printf("ERROR: Failed to throw for missing file for SDKMESH\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            ID3D11Device* nullDevice = nullptr;
            const uint8_t* ptr = nullptr;
            auto invalid = Model::CreateFromSDKMESH(nullDevice, ptr, 0, *fxFactory);

            printf("ERROR: Failed to throw for null device for SDKMESH memory\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}

_Success_(return)
bool Test15(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    std::unique_ptr<DGSLEffectFactory> fxFactory;

    try
    {
        fxFactory = std::make_unique<DGSLEffectFactory>(device);

        fxFactory->SetDirectory(L"DGSLTest");
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating effects factory object (except: %s)\n", e.what());
        return false;
    }

    // Create model
    bool success = true;

    std::unique_ptr<Model> unlit;
    try
    {
        unlit = Model::CreateFromCMO(device, L"DGSLTest\\teapot_unlit.cmo", *fxFactory);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating model from CMO (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<Model> lambert;
    try
    {
        lambert = Model::CreateFromCMO(device, L"DGSLTest\\teapot_lambert.cmo", *fxFactory);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating model from CMO 2 (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<Model> phong;
    try
    {
        phong = Model::CreateFromCMO(device, L"DGSLTest\\teapot_phong.cmo", *fxFactory);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating model from CMO 3 (except: %s)\n", e.what());
        success = false;
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        try
        {
            ID3D11Device* nullDevice = nullptr;
            auto invalid = std::make_unique<DGSLEffectFactory>(nullDevice);

            printf("ERROR: Failed to throw for null device for effect factory\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}
