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

// Note VS 2017 incorrectly returns 'false' for is_nothrow_move_constructible<Model>
static_assert(std::is_move_constructible<Model>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<Model>::value, "Move Assign.");

// VS 2017 on XDK incorrectly thinks it's not noexcept
static_assert(std::is_move_constructible<ModelMesh>::value, "Move Ctor.");
static_assert(std::is_move_assignable<ModelMesh>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<ModelMeshPart>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ModelMeshPart>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<ModelBone>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ModelBone>::value, "Move Assign.");

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
        sdkmesh = Model::CreateFromSDKMESH(device, L"ModelTest\\cup.sdkmesh", *fxFactory);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating model from SDKMESH (except: %s)\n", e.what());
        success = false;
    }

    return success;
}
