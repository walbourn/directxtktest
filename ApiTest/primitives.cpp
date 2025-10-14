//--------------------------------------------------------------------------------------
// File: primitives.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "GeometricPrimitive.h"

#include <cstdio>
#include <type_traits>

#include <wrl/client.h>

using namespace DirectX;

static_assert(!std::is_copy_constructible<GeometricPrimitive>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<GeometricPrimitive>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<GeometricPrimitive>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<GeometricPrimitive>::value, "Move Assign.");

_Success_(return)
bool Test04(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    bool success = true;

    std::unique_ptr<GeometricPrimitive> cube;
    try
    {
        cube = GeometricPrimitive::CreateCube(context.Get(), 1.f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating cube (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> box;
    try
    {
        box = GeometricPrimitive::CreateBox(context.Get(), XMFLOAT3(1.f / 2.f, 2.f / 2.f, 3.f / 2.f));
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating box (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> sphere;
    try
    {
        sphere = GeometricPrimitive::CreateSphere(context.Get(), 1.f, 16);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating sphere (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> geosphere;
    try
    {
        geosphere = GeometricPrimitive::CreateGeoSphere(context.Get(), 1.f, 3);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating geosphere (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> cylinder;
    try
    {
        cylinder = GeometricPrimitive::CreateCylinder(context.Get(), 1.f, 1.f, 32);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating cylinder (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> cone;
    try
    {
        cone = GeometricPrimitive::CreateCone(context.Get(), 1.f, 1.f, 32);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating cone (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> torus;
    try
    {
        torus = GeometricPrimitive::CreateTorus(context.Get(), 1.f, 0.333f, 32);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating torus (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> teapot;
    try
    {
        teapot = GeometricPrimitive::CreateTeapot(context.Get(), 1.f, 8);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating utah teapot (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> tetra;
    try
    {
        tetra = GeometricPrimitive::CreateTetrahedron(context.Get(), 0.75f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating tetrahedron (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> octa;
    try
    {
        octa = GeometricPrimitive::CreateOctahedron(context.Get(), 0.75f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating octahedron (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> dodec;
    try
    {
        dodec = GeometricPrimitive::CreateDodecahedron(context.Get(), 0.5f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating dodecahedron (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> iso;
    try
    {
        iso = GeometricPrimitive::CreateIcosahedron(context.Get(), 0.5f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating icosadedron (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> customBox;
    try
    {
        GeometricPrimitive::VertexCollection customVerts;
        GeometricPrimitive::IndexCollection customIndices;
        GeometricPrimitive::CreateBox(customVerts, customIndices, XMFLOAT3(1.f / 2.f, 2.f / 2.f, 3.f / 2.f));

        assert(customVerts.size() == 24);
        assert(customIndices.size() == 36);

        for (auto& it : customVerts)
        {
            it.textureCoordinate.x *= 5.f;
            it.textureCoordinate.y *= 5.f;
        }

        customBox = GeometricPrimitive::CreateCustom(context.Get(), customVerts, customIndices);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating custom box (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<GeometricPrimitive> customBox2;
    try
    {
        // Ensure VertexType alias is consistent with alternative client usage
        GeometricPrimitive::VertexCollection customVerts;
        GeometricPrimitive::IndexCollection customIndices;
        GeometricPrimitive::CreateBox(customVerts, customIndices, XMFLOAT3(1.f / 2.f, 2.f / 2.f, 3.f / 2.f));

        assert(customVerts.size() == 24);
        assert(customIndices.size() == 36);

        customBox2 = GeometricPrimitive::CreateCustom(context.Get(), customVerts, customIndices);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating custom (except: %s)\n", e.what());
        success =  false;
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        ID3D11DeviceContext* nullContext = nullptr;
        try
        {
            auto invalid = GeometricPrimitive::CreateCube(nullContext, 1.f);

            printf("ERROR: Failed to throw for null context\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            GeometricPrimitive::VertexCollection customVerts;
            GeometricPrimitive::IndexCollection customIndices;
            GeometricPrimitive::CreateCustom(context.Get(), customVerts, customIndices);

            printf("ERROR: Failed to throw no verts/indices\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            GeometricPrimitive::VertexCollection customVerts;
            customVerts.resize(1);

            GeometricPrimitive::IndexCollection customIndices;
            customIndices.resize(1);

            GeometricPrimitive::CreateCustom(context.Get(), customVerts, customIndices);

            printf("ERROR: Failed to throw need multiple of 3 indices\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            GeometricPrimitive::VertexCollection customVerts;
            customVerts.resize(1);

            GeometricPrimitive::IndexCollection customIndices;
            customIndices.resize(3);
            customIndices.push_back(0);
            customIndices.push_back(1);
            customIndices.push_back(2);

            GeometricPrimitive::CreateCustom(context.Get(), customVerts, customIndices);

            printf("ERROR: Failed to throw out of range index indices\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            GeometricPrimitive::VertexCollection customVerts;
            customVerts.resize(1);

            GeometricPrimitive::IndexCollection customIndices;
            customIndices.resize(INT32_MAX);

            GeometricPrimitive::CreateCustom(context.Get(), customVerts, customIndices);

            printf("ERROR: Failed to throw too many indices\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

    #if defined(_M_X64) || defined(_M_ARM64) || defined(__amd64__) || defined(__aarch64__)
        try
        {
            GeometricPrimitive::VertexCollection customVerts;
            customVerts.resize(UINT32_MAX + 1);

            GeometricPrimitive::IndexCollection customIndices;
            customIndices.resize(3);

            GeometricPrimitive::CreateCustom(context.Get(), customVerts, customIndices);

            printf("ERROR: Failed to throw too many verts\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    #endif
    }
    #pragma warning(pop)

    return success;
}
