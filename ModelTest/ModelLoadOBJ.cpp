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

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <windows.h>

#include "DirectXHelpers.h"
#include "Effects.h"
#include "Model.h"
#include "VertexTypes.h"

#include <algorithm>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "PlatformHelpers.h"

using namespace DirectX;

class WaveFrontObj
{
public:
    WaveFrontObj() {}

    HRESULT Load( _In_z_ const wchar_t* szFileName );
    HRESULT LoadMTL( _In_z_ const wchar_t* szFileName );

    void SortByAttributes();

    struct Material
    {
        XMFLOAT3 vAmbient;
        XMFLOAT3 vDiffuse;
        XMFLOAT3 vSpecular;

        uint32_t nShininess;
        float fAlpha;

        bool bSpecular;

        wchar_t strName[MAX_PATH];
        wchar_t strTexture[MAX_PATH];

        Material() :
            vAmbient( 0.2f, 0.2f, 0.2f ),
            vDiffuse( 0.8f, 0.8f, 0.8f ),
            vSpecular(  1.0f, 1.0f, 1.0f ),
            nShininess( 0 ),
            fAlpha( 1.f ),
            bSpecular( false )
            { memset(strName, 0, MAX_PATH); memset(strTexture, 0, MAX_PATH); } 
    };

    std::vector<VertexPositionNormalTexture>    vertices;
    std::vector<uint16_t>                       indices;
    std::vector<uint32_t>                       attributes;
    std::vector<Material>                       materials;

private:
    typedef std::unordered_multimap<UINT, UINT> VertexCache;

    DWORD AddVertex( UINT hash, VertexPositionNormalTexture* pVertex, VertexCache& cache );
};


//--------------------------------------------------------------------------------------
// Helper for creating a D3D vertex or index buffer.
template<typename T>
static void CreateBuffer(_In_ ID3D11Device* device, T const& data, D3D11_BIND_FLAG bindFlags, _Out_ ID3D11Buffer** pBuffer)
{
    D3D11_BUFFER_DESC bufferDesc = {};

    bufferDesc.ByteWidth = (UINT)data.size() * sizeof(T::value_type);
    bufferDesc.BindFlags = bindFlags;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA dataDesc = {};

    dataDesc.pSysMem = data.data();

    ThrowIfFailed(
        device->CreateBuffer(&bufferDesc, &dataDesc, pBuffer)
    );

    SetDebugObjectName(*pBuffer, "ModelOBJ");
}

// Helper for creating a D3D input layout.
static void CreateInputLayout(_In_ ID3D11Device* device, IEffect* effect, _Out_ ID3D11InputLayout** pInputLayout)
{
    void const* shaderByteCode;
    size_t byteCodeLength;

    effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    ThrowIfFailed(
        device->CreateInputLayout(VertexPositionNormalTexture::InputElements,
                                  VertexPositionNormalTexture::InputElementCount,
                                  shaderByteCode, byteCodeLength,
                                  pInputLayout)
    );

    SetDebugObjectName(*pInputLayout, "ModelOBJ");
}


//--------------------------------------------------------------------------------------
// Shared VB input element description
static INIT_ONCE g_InitOnce = INIT_ONCE_STATIC_INIT;
static std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>> g_vbdecl;

static BOOL CALLBACK InitializeDecl( PINIT_ONCE initOnce, PVOID Parameter, PVOID *lpContext )
{
    UNREFERENCED_PARAMETER( initOnce );
    UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( lpContext );
    g_vbdecl = std::make_shared<std::vector<D3D11_INPUT_ELEMENT_DESC>>( VertexPositionNormalTexture::InputElements,
                                    VertexPositionNormalTexture::InputElements + VertexPositionNormalTexture::InputElementCount );
    return TRUE;
}


//--------------------------------------------------------------------------------------
std::unique_ptr<Model> CreateModelFromOBJ( _In_ ID3D11Device* d3dDevice, _In_ ID3D11DeviceContext* deviceContext, _In_z_ const wchar_t* szFileName, _In_ IEffectFactory& fxFactory, bool ccw, bool pmalpha )
{
    if ( !InitOnceExecuteOnce( &g_InitOnce, InitializeDecl, nullptr, nullptr ) )
        throw std::exception("One-time initialization failed");

    std::unique_ptr<WaveFrontObj> obj( new WaveFrontObj() );

    if ( FAILED( obj->Load( szFileName ) ) )
    {
        throw std::exception("Failed loading WaveFront file");
    }

    if ( obj->vertices.empty() || obj->indices.empty() || obj->attributes.empty() || obj->materials.empty() )
    {
        throw std::exception("Missing data in WaveFront file");
    }
    
    obj->SortByAttributes();

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


//--------------------------------------------------------------------------------------
HRESULT WaveFrontObj::Load( _In_z_ const wchar_t* szFileName )
{
    static const size_t MAX_POLY = 16;

    std::wifstream InFile( szFileName );
    if( !InFile )
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

    std::vector<XMFLOAT3>   positions;
    std::vector<XMFLOAT3>   normals;
    std::vector<XMFLOAT2>   texCoords;

    VertexCache  vertexCache;

    Material defmat;
    wcscpy_s( defmat.strName, L"default" );
    materials.push_back( defmat );

    uint32_t curSubset = 0;

    wchar_t strCommand[256] = {};
    wchar_t strMaterialFilename[MAX_PATH] = {};
    for( ;; )
    {
        InFile >> strCommand;
        if( !InFile )
            break;

        if( 0 == wcscmp( strCommand, L"#" ) )
        {
            // Comment
        }
        else if( 0 == wcscmp( strCommand, L"v" ) )
        {
            // Vertex Position
            float x, y, z;
            InFile >> x >> y >> z;
            positions.push_back( XMFLOAT3( x, y, z ) );
        }
        else if( 0 == wcscmp( strCommand, L"vt" ) )
        {
            // Vertex TexCoord
            float u, v;
            InFile >> u >> v;
            texCoords.push_back( XMFLOAT2( u, v ) );
        }
        else if( 0 == wcscmp( strCommand, L"vn" ) )
        {
            // Vertex Normal
            float x, y, z;
            InFile >> x >> y >> z;
            normals.push_back( XMFLOAT3( x, y, z ) );
        }
        else if( 0 == wcscmp( strCommand, L"f" ) )
        {
            // Face
            UINT iPosition, iTexCoord, iNormal;
            VertexPositionNormalTexture vertex;

            DWORD faceIndex[ MAX_POLY ];
            size_t iFace = 0;
            for(;;)
            {
                if ( iFace >= MAX_POLY )
                {
                    // Too many polygon verts for the reader
                    return E_FAIL;
                }

                memset( &vertex, 0, sizeof( vertex ) );

                // OBJ format uses 1-based arrays
                InFile >> iPosition;
                if ( iPosition > positions.size() )
                    return E_FAIL;

                vertex.position = positions[ iPosition - 1 ];

                if( '/' == InFile.peek() )
                {
                    InFile.ignore();

                    if( '/' != InFile.peek() )
                    {
                        // Optional texture coordinate
                        InFile >> iTexCoord;
                        if ( iTexCoord > texCoords.size() )
                            return E_FAIL;

                        vertex.textureCoordinate = texCoords[ iTexCoord - 1 ];
                    }

                    if( '/' == InFile.peek() )
                    {
                        InFile.ignore();

                        // Optional vertex normal
                        InFile >> iNormal;
                        if ( iNormal > normals.size() )
                            return E_FAIL;

                        vertex.normal = normals[ iNormal - 1 ];
                    }
                }

                // If a duplicate vertex doesn't exist, add this vertex to the Vertices
                // list. Store the index in the Indices array. The Vertices and Indices
                // lists will eventually become the Vertex Buffer and Index Buffer for
                // the mesh.
                DWORD index = AddVertex( iPosition, &vertex, vertexCache );
                if ( index == (DWORD)-1 )
                   return E_OUTOFMEMORY;

                if ( index >= 0xFFFF )
                {
                    // Too many indices for 16-bit IB!
                    return E_FAIL;
                }

                faceIndex[ iFace ] = index;
                ++iFace;
   
                // Check for more face data or end of the face statement
                bool faceEnd = false;
                for(;;)
                {
                    wchar_t p = InFile.peek();
                    
                    if ( '\n' == p || !InFile )
                    {
                        faceEnd = true;
                        break;
                    }
                    else if ( isdigit( p ) )
                        break;

                    InFile.ignore();
                }

                if ( faceEnd )
                    break;
            }

            if ( iFace < 3 )
            {
                // Need at least 3 points to form a triangle
                return E_FAIL;
            }

            // Convert polygons to triangles
            DWORD i0 = faceIndex[0];
            DWORD i1 = faceIndex[1];

            for( size_t j = 2; j < iFace; ++ j )
            {
                DWORD index = faceIndex[ j ];
                indices.push_back( static_cast<uint16_t>( i0 ) );
                indices.push_back( static_cast<uint16_t>( i1 ) );
                indices.push_back( static_cast<uint16_t>( index ) );

                attributes.push_back( curSubset );

                i1 = index;
            }

            assert( attributes.size()*3 == indices.size() );
        }
        else if( 0 == wcscmp( strCommand, L"mtllib" ) )
        {
            // Material library
            InFile >> strMaterialFilename;
        }
        else if( 0 == wcscmp( strCommand, L"usemtl" ) )
        {
            // Material
            wchar_t strName[MAX_PATH] = {};
            InFile >> strName;

            bool bFound = false;
            uint32_t count = 0;
            for( auto it = materials.cbegin(); it != materials.cend(); ++it, ++count )
            {
                if( 0 == wcscmp( it->strName, strName ) )
                {
                    bFound = true;
                    curSubset = count;
                    break;
                }
            }

            if( !bFound )
            {
                Material mat;
                curSubset = static_cast<uint32_t>( materials.size() );
                wcscpy_s( mat.strName, MAX_PATH - 1, strName );
                materials.push_back( mat );
            }
        }
        else
        {
            // Unimplemented or unrecognized command
            OutputDebugStringW( strCommand );
        }

        InFile.ignore( 1000, '\n' );
    }

    // Cleanup
    InFile.close();

    // If an associated material file was found, read that in as well.
    if( *strMaterialFilename )
    {
        wchar_t ext[_MAX_EXT];
        wchar_t fname[_MAX_FNAME];
        _wsplitpath_s( strMaterialFilename, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, ext, _MAX_EXT );

        wchar_t drive[_MAX_DRIVE];
        wchar_t dir[_MAX_DIR];
        _wsplitpath_s( szFileName, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0 );

        wchar_t szPath[ MAX_PATH ];
        _wmakepath_s( szPath, MAX_PATH, drive, dir, fname, ext );

        HRESULT hr = LoadMTL( szPath );
        if ( FAILED(hr) )
            return hr;
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
HRESULT WaveFrontObj::LoadMTL( _In_z_ const wchar_t* szFileName )
{
    // Assumes MTL is in CWD along with OBJ
    std::wifstream InFile( szFileName );
    if( !InFile )
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

    auto curMaterial = materials.end();

    wchar_t strCommand[256] = {};
    for( ;; )
    {
        InFile >> strCommand;
        if( !InFile )
            break;

        if( 0 == wcscmp( strCommand, L"newmtl" ) )
        {
            // Switching active materials
            wchar_t strName[MAX_PATH] = {};
            InFile >> strName;

            curMaterial = materials.end();
            for( auto it = materials.begin(); it != materials.end(); ++it )
            {
                if( 0 == wcscmp( it->strName, strName ) )
                {
                    curMaterial = it;
                    break;
                }
            }
        }

        // The rest of the commands rely on an active material
        if( curMaterial == materials.end() )
            continue;

        if( 0 == wcscmp( strCommand, L"#" ) )
        {
            // Comment
        }
        else if( 0 == wcscmp( strCommand, L"Ka" ) )
        {
            // Ambient color
            float r, g, b;
            InFile >> r >> g >> b;
            curMaterial->vAmbient = XMFLOAT3( r, g, b );
        }
        else if( 0 == wcscmp( strCommand, L"Kd" ) )
        {
            // Diffuse color
            float r, g, b;
            InFile >> r >> g >> b;
            curMaterial->vDiffuse = XMFLOAT3( r, g, b );
        }
        else if( 0 == wcscmp( strCommand, L"Ks" ) )
        {
            // Specular color
            float r, g, b;
            InFile >> r >> g >> b;
            curMaterial->vSpecular = XMFLOAT3( r, g, b );
        }
        else if( 0 == wcscmp( strCommand, L"d" ) ||
                 0 == wcscmp( strCommand, L"Tr" ) )
        {
            // Alpha
            InFile >> curMaterial->fAlpha;
        }
        else if( 0 == wcscmp( strCommand, L"Ns" ) )
        {
            // Shininess
            int nShininess;
            InFile >> nShininess;
            curMaterial->nShininess = nShininess;
        }
        else if( 0 == wcscmp( strCommand, L"illum" ) )
        {
            // Specular on/off
            int illumination;
            InFile >> illumination;
            curMaterial->bSpecular = ( illumination == 2 );
        }
        else if( 0 == wcscmp( strCommand, L"map_Kd" ) )
        {
            // Texture
            InFile >> curMaterial->strTexture;
        }
        else
        {
            // Unimplemented or unrecognized command
        }

        InFile.ignore( 1000, L'\n' );
    }

    InFile.close();

    return S_OK;
}


//--------------------------------------------------------------------------------------
DWORD WaveFrontObj::AddVertex( UINT hash, VertexPositionNormalTexture* pVertex, VertexCache& cache )
{
    auto f = cache.equal_range( hash );

    for( auto it = f.first; it != f.second; ++it )
    {
        auto& tv = vertices[ it->second ];

        if ( 0 == memcmp( pVertex, &tv, sizeof(VertexPositionNormalTexture) ) )
        {
            return it->second;
        }
    }

    DWORD index = static_cast<UINT>( vertices.size() );
    vertices.push_back( *pVertex );

    VertexCache::value_type entry( hash, index );
    cache.insert( entry );
    return index;
}


//--------------------------------------------------------------------------------------
void WaveFrontObj::SortByAttributes()
{
    if ( attributes.empty() || indices.empty() )
        return;

    struct Face
    {
        uint32_t attribute;
        uint16_t a;
        uint16_t b;
        uint16_t c;
    };

    std::vector<Face> faces;
    faces.reserve( attributes.size() );

    assert( attributes.size()*3 == indices.size() );

    for( size_t i = 0; i < attributes.size(); ++i )
    {
        Face f;
        f.attribute = attributes[ i ];
        f.a = indices[ i*3 ];
        f.b = indices[ i*3 + 1 ];
        f.c = indices[ i*3 + 2 ];

        faces.push_back( f );
    }
    
    std::stable_sort( faces.begin(), faces.end(), [](const Face& a, const Face& b ) -> bool
                                            {
                                                return (a.attribute < b.attribute);
                                            });
    
    attributes.clear();
    indices.clear();

    for( auto it = faces.cbegin(); it != faces.cend(); ++it )
    {
        attributes.push_back( it->attribute );
        indices.push_back( it->a );
        indices.push_back( it->b );
        indices.push_back( it->c );
    }
}