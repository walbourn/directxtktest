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

static_assert(std::is_nothrow_move_constructible<GeometricPrimitive>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<GeometricPrimitive>::value, "Move Assign.");

bool Test04(ID3D11Device *device)
{
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    bool success = true;

    std::unique_ptr<DirectX::GeometricPrimitive> cube;
    try
    {
        cube = GeometricPrimitive::CreateCube(context.Get(), 1.f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating cube (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> box;
    try
    {
        box = GeometricPrimitive::CreateBox(context.Get(), XMFLOAT3(1.f / 2.f, 2.f / 2.f, 3.f / 2.f));
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating box (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> sphere;
    try
    {
        sphere = GeometricPrimitive::CreateSphere(context.Get(), 1.f, 16);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating sphere (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> geosphere;
    try
    {
        geosphere = GeometricPrimitive::CreateGeoSphere(context.Get(), 1.f, 3);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating geosphere (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> cylinder;
    try
    {
        cylinder = GeometricPrimitive::CreateCylinder(context.Get(), 1.f, 1.f, 32);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating cylinder (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> cone;
    try
    {
        cone = GeometricPrimitive::CreateCone(context.Get(), 1.f, 1.f, 32);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating cone (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> torus;
    try
    {
        torus = GeometricPrimitive::CreateTorus(context.Get(), 1.f, 0.333f, 32);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating torus (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> teapot;
    try
    {
        teapot = GeometricPrimitive::CreateTeapot(context.Get(), 1.f, 8);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating utah teapot (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> tetra;
    try
    {
        tetra = GeometricPrimitive::CreateTetrahedron(context.Get(), 0.75f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating tetrahedron (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> octa;
    try
    {
        octa = GeometricPrimitive::CreateOctahedron(context.Get(), 0.75f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating octahedron (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> dodec;
    try
    {
        dodec = GeometricPrimitive::CreateDodecahedron(context.Get(), 0.5f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating dodecahedron (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> iso;
    try
    {
        iso = GeometricPrimitive::CreateIcosahedron(context.Get(), 0.5f);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating icosadedron (except: %s)\n", e.what());
        success =  false;
    }

    std::unique_ptr<DirectX::GeometricPrimitive> customBox;
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

    std::unique_ptr<DirectX::GeometricPrimitive> customBox2;
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

    return success;
}
