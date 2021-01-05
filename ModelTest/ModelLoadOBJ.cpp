//--------------------------------------------------------------------------------------
// File: ModelTestOBJ.cpp
//
// Code for loading a Model from a WaveFront OBJ file
//
// http://en.wikipedia.org/wiki/Wavefront_.obj_file
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"

#include <map>

#include "WaveFrontReader.h"

using namespace DirectX;

namespace
{
    inline XMFLOAT3 GetMaterialColor(float r, float g, float b, bool srgb)
    {
        if (srgb)
        {
            XMVECTOR v = XMVectorSet(r, g, b, 1.f);
            v = XMColorSRGBToRGB(v);

            XMFLOAT3 result;
            XMStoreFloat3(&result, v);
            return result;
        }
        else
        {
            return XMFLOAT3(r, g, b);
        }
    }

    //----------------------------------------------------------------------------------
    // Shared VB input element description
    INIT_ONCE g_InitOnce = INIT_ONCE_STATIC_INIT;
    std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> g_vbdecl;

    BOOL CALLBACK InitializeDecl(PINIT_ONCE initOnce, PVOID Parameter, PVOID *lpContext)
    {
        UNREFERENCED_PARAMETER(initOnce);
        UNREFERENCED_PARAMETER(Parameter);
        UNREFERENCED_PARAMETER(lpContext);
        g_vbdecl = std::make_shared<std::vector<D3D11_INPUT_ELEMENT_DESC>>(VertexPositionNormalTexture::InputElements,
            VertexPositionNormalTexture::InputElements + VertexPositionNormalTexture::InputElementCount);
        return TRUE;
    }
}


//--------------------------------------------------------------------------------------
std::unique_ptr<Model> CreateModelFromOBJ(
    _In_ ID3D11Device* d3dDevice,
    _In_ ID3D11DeviceContext* deviceContext,
    _In_z_ const wchar_t* szFileName,
    _In_ IEffectFactory& fxFactory,
    ModelLoaderFlags flags )
{
    if ( !InitOnceExecuteOnce( &g_InitOnce, InitializeDecl, nullptr, nullptr ) )
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "InitOnceExecuteOnce");

    auto obj = std::make_unique<WaveFrontReader<uint16_t>>();

    if ( FAILED( obj->Load( szFileName ) ) )
    {
        throw std::runtime_error("Failed loading WaveFront file");
    }

    if ( obj->vertices.empty() || obj->indices.empty() || obj->attributes.empty() || obj->materials.empty() )
    {
        throw std::runtime_error("Missing data in WaveFront file");
    }
    
    // Sort by attributes
    {
        struct Face
        {
            uint32_t attribute;
            uint16_t a;
            uint16_t b;
            uint16_t c;
        };

        std::vector<Face> faces;
        faces.reserve(obj->attributes.size());

        assert(obj->attributes.size() * 3 == obj->indices.size());

        for (size_t i = 0; i < obj->attributes.size(); ++i)
        {
            Face f;
            f.attribute = obj->attributes[i];
            f.a = obj->indices[i * 3];
            f.b = obj->indices[i * 3 + 1];
            f.c = obj->indices[i * 3 + 2];

            faces.push_back(f);
        }

        std::stable_sort(faces.begin(), faces.end(), [](const Face& a, const Face& b) -> bool
        {
            return (a.attribute < b.attribute);
        });

        obj->attributes.clear();
        obj->indices.clear();

        for (auto it = faces.cbegin(); it != faces.cend(); ++it)
        {
            obj->attributes.push_back(it->attribute);
            obj->indices.push_back(it->a);
            obj->indices.push_back(it->b);
            obj->indices.push_back(it->c);
        }
    }

    // Create Vertex Buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer> vb;
    DX::ThrowIfFailed(
        CreateStaticBuffer(d3dDevice, obj->vertices, D3D11_BIND_VERTEX_BUFFER, vb.GetAddressOf())
    );

    // Create Index Buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer> ib;
    DX::ThrowIfFailed(
        CreateStaticBuffer(d3dDevice, obj->indices, D3D11_BIND_INDEX_BUFFER, ib.GetAddressOf())
    );

    // Create mesh
    auto mesh = std::make_shared<ModelMesh>();
    mesh->name = szFileName;
    mesh->ccw = (flags & ModelLoader_CounterClockwise) != 0;
    mesh->pmalpha = (flags & ModelLoader_PremultipledAlpha) != 0;

    BoundingSphere::CreateFromPoints( mesh->boundingSphere, obj->vertices.size(), &obj->vertices[0].position, sizeof( VertexPositionNormalTexture ) );
    BoundingBox::CreateFromPoints( mesh->boundingBox, obj->vertices.size(), &obj->vertices[0].position, sizeof( VertexPositionNormalTexture ) );

    // Create a subset for each attribute/material
    uint32_t curmaterial = static_cast<uint32_t>( -1 );
    std::shared_ptr<IEffect> effect;
    bool alpha = false;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> il;

    size_t index = 0;
    size_t sindex = 0;
    size_t nindices = 0;
    for( auto it = obj->attributes.cbegin(); it != obj->attributes.cend(); ++it )
    {
        if ( *it != curmaterial )
        {
            auto mat = obj->materials[ *it ];

            alpha = ( mat.fAlpha < 1.f ) ? true : false;

            EffectFactory::EffectInfo info;
            info.name = mat.strName;
            info.alpha = mat.fAlpha;
            info.ambientColor = GetMaterialColor(mat.vAmbient.x, mat.vAmbient.y, mat.vAmbient.z, (flags & ModelLoader_MaterialColorsSRGB) != 0);
            info.diffuseColor = GetMaterialColor(mat.vDiffuse.x, mat.vDiffuse.y, mat.vDiffuse.z, (flags & ModelLoader_MaterialColorsSRGB) != 0);

            if ( mat.bSpecular )
            {
                info.specularPower = static_cast<float>(mat.nShininess);
                info.specularColor = mat.vSpecular;
            }

            info.diffuseTexture = mat.strTexture;

            effect = fxFactory.CreateEffect( info, deviceContext );

            // Create input layout from effect
            DX::ThrowIfFailed(
                CreateInputLayoutFromEffect<VertexPositionNormalTexture>(d3dDevice, effect.get(), il.ReleaseAndGetAddressOf())
            );

            curmaterial = *it;
        }

        nindices += 3;

        auto nit = it+1;
        if ( nit == obj->attributes.cend() || *nit != curmaterial )
        {
            auto part = new ModelMeshPart;

            part->indexCount = static_cast<uint32_t>( nindices );
            part->startIndex = static_cast<uint32_t>( sindex );
            part->vertexStride = sizeof( VertexPositionNormalTexture );
            part->inputLayout = il;
            part->indexBuffer = ib;
            part->vertexBuffer = vb;
            part->effect = effect;
            part->vbDecl = g_vbdecl;
            part->isAlpha = alpha;
            
            mesh->meshParts.emplace_back( part );

            nindices = 0;
            sindex = index + 3;
        }

        index += 3;
    }

    // Create model
    std::unique_ptr<Model> model(new Model());
    model->name = szFileName;
    model->meshes.push_back( mesh );
 
    return model;
}
