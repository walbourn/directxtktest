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
    //----------------------------------------------------------------------------------
    // Helper for creating a D3D vertex or index buffer.
    template<typename T>
    void CreateBuffer(_In_ ID3D11Device* device, T const& data, D3D11_BIND_FLAG bindFlags, _Out_ ID3D11Buffer** pBuffer)
    {
        D3D11_BUFFER_DESC bufferDesc = {};

        bufferDesc.ByteWidth = (UINT)data.size() * sizeof(T::value_type);
        bufferDesc.BindFlags = bindFlags;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;

        D3D11_SUBRESOURCE_DATA dataDesc = {};

        dataDesc.pSysMem = data.data();

        DX::ThrowIfFailed(
            device->CreateBuffer(&bufferDesc, &dataDesc, pBuffer)
        );

        assert(pBuffer != 0 && *pBuffer != 0);
        _Analysis_assume_(pBuffer != 0 && *pBuffer != 0);

        SetDebugObjectName(*pBuffer, "ModelOBJ");
    }

    // Helper for creating a D3D input layout.
    void CreateInputLayout(_In_ ID3D11Device* device, IEffect* effect, _Out_ ID3D11InputLayout** pInputLayout)
    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        DX::ThrowIfFailed(
            device->CreateInputLayout(VertexPositionNormalTexture::InputElements,
                VertexPositionNormalTexture::InputElementCount,
                shaderByteCode, byteCodeLength,
                pInputLayout)
        );

        assert(pInputLayout != 0 && *pInputLayout != 0);
        _Analysis_assume_(pInputLayout != 0 && *pInputLayout != 0);

        SetDebugObjectName(*pInputLayout, "ModelOBJ");
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
std::unique_ptr<Model> CreateModelFromOBJ( _In_ ID3D11Device* d3dDevice, _In_ ID3D11DeviceContext* deviceContext, _In_z_ const wchar_t* szFileName, _In_ IEffectFactory& fxFactory, bool ccw, bool pmalpha )
{
    if ( !InitOnceExecuteOnce( &g_InitOnce, InitializeDecl, nullptr, nullptr ) )
        throw std::exception("One-time initialization failed");

    auto obj = std::make_unique<WaveFrontReader<uint16_t>>();

    if ( FAILED( obj->Load( szFileName ) ) )
    {
        throw std::exception("Failed loading WaveFront file");
    }

    if ( obj->vertices.empty() || obj->indices.empty() || obj->attributes.empty() || obj->materials.empty() )
    {
        throw std::exception("Missing data in WaveFront file");
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
    CreateBuffer( d3dDevice, obj->vertices, D3D11_BIND_VERTEX_BUFFER, &vb );

    // Create Index Buffer
    Microsoft::WRL::ComPtr<ID3D11Buffer> ib;
    CreateBuffer( d3dDevice, obj->indices, D3D11_BIND_INDEX_BUFFER, &ib );

    // Create mesh
    auto mesh = std::make_shared<ModelMesh>();
    mesh->name = szFileName;
    mesh->ccw = ccw;
    mesh->pmalpha = pmalpha;

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
            info.ambientColor = mat.vAmbient;
            info.diffuseColor = mat.vDiffuse;

            if ( mat.bSpecular )
            {
                info.specularPower = static_cast<float>(mat.nShininess);
                info.specularColor = mat.vSpecular;
            }

            info.diffuseTexture = mat.strTexture;

            effect = fxFactory.CreateEffect( info, deviceContext );

            // Create input layout from effect
            CreateInputLayout( d3dDevice, effect.get(), &il );

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
