//--------------------------------------------------------------------------------------
// File: effects.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// https://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "Effects.h"

#include "BufferHelpers.h"
#include "DirectXHelpers.h"
#include "VertexTypes.h"

#include <cstdio>
#include <stdexcept>
#include <type_traits>

#include <wrl/client.h>

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

static_assert(!std::is_copy_constructible<BasicEffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<BasicEffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<BasicEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<BasicEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<AlphaTestEffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<AlphaTestEffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<AlphaTestEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<AlphaTestEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<DualTextureEffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<DualTextureEffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<DualTextureEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DualTextureEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<EnvironmentMapEffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<EnvironmentMapEffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<EnvironmentMapEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<EnvironmentMapEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<SkinnedEffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<SkinnedEffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<SkinnedEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SkinnedEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<NormalMapEffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<NormalMapEffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<NormalMapEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<NormalMapEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<SkinnedNormalMapEffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<SkinnedNormalMapEffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<SkinnedNormalMapEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SkinnedNormalMapEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<PBREffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<PBREffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<PBREffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<PBREffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<SkinnedPBREffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<SkinnedPBREffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<SkinnedPBREffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SkinnedPBREffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<DebugEffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<DebugEffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<DebugEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DebugEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<NPREffect>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<NPREffect>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<NPREffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<NPREffect>::value, "Move Assign.");

static_assert(std::is_nothrow_default_constructible<IEffectFactory::EffectInfo>::value, "Copy Ctor.");
static_assert(std::is_nothrow_copy_constructible<IEffectFactory::EffectInfo>::value, "Copy Ctor.");
static_assert(std::is_nothrow_copy_assignable<IEffectFactory::EffectInfo>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<IEffectFactory::EffectInfo>::value, "Move Ctor.");
static_assert(std::is_move_assignable<IEffectFactory::EffectInfo>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<EffectFactory>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<EffectFactory>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<EffectFactory>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<EffectFactory>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<PBREffectFactory>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<PBREffectFactory>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<PBREffectFactory>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<PBREffectFactory>::value, "Move Assign.");
static_assert(std::is_nothrow_move_constructible<DGSLEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DGSLEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<EffectFactory>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<EffectFactory>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<SkinnedDGSLEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SkinnedDGSLEffect>::value, "Move Assign.");

static_assert(!std::is_copy_constructible<DGSLEffectFactory>::value, "Copy Ctor.");
static_assert(!std::is_copy_assignable<DGSLEffectFactory>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<DGSLEffectFactory>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DGSLEffectFactory>::value, "Move Assign.");

static_assert(std::is_nothrow_default_constructible<DGSLEffectFactory::DGSLEffectInfo>::value, "Copy Ctor.");
static_assert(std::is_nothrow_copy_constructible<DGSLEffectFactory::DGSLEffectInfo>::value, "Copy Ctor.");
static_assert(std::is_nothrow_copy_assignable<DGSLEffectFactory::DGSLEffectInfo>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<DGSLEffectFactory::DGSLEffectInfo>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DGSLEffectFactory::DGSLEffectInfo>::value, "Move Assign.");

namespace
{
    struct TestVertex
    {
        TestVertex(FXMVECTOR iposition, FXMVECTOR inormal, FXMVECTOR itextureCoordinate)
        {
            XMStoreFloat3(&this->position, iposition);
            XMStoreFloat3(&this->normal, inormal);
            XMStoreFloat2(&this->textureCoordinate, itextureCoordinate);
            XMStoreFloat2(&this->textureCoordinate2, XMVectorScale(itextureCoordinate, 3));
            PackedVector::XMStoreUByte4(&this->blendIndices, XMVectorSet(0, 1, 2, 3));

            float u = XMVectorGetX(itextureCoordinate) - 0.5f;
            float v = XMVectorGetY(itextureCoordinate) - 0.5f;

            float d = 1 - sqrtf(u * u + v * v) * 2;

            if (d < 0)
                d = 0;

            XMStoreFloat4(&this->blendWeight, XMVectorSet(d, 1 - d, u, v));

            color = 0xFFFF00FF;
        }

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 textureCoordinate;
        XMFLOAT2 textureCoordinate2;
        PackedVector::XMUBYTE4 blendIndices;
        XMFLOAT4 blendWeight;
        PackedVector::XMUBYTE4 color;

        static constexpr unsigned int InputElementCount = 7;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };


    const D3D11_INPUT_ELEMENT_DESC TestVertex::InputElements[] =
    {
        { "SV_Position",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     1, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",        0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
}

// BasicEffect, AlphaTestEffect, DualTextureEffect, EnvironmentMapEffect, SkinnedEffect
_Success_(return)
bool Test05(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    bool success = true;

    // Create effect
    std::unique_ptr<BasicEffect> basic;
    try
    {
        basic = std::make_unique<BasicEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<AlphaTestEffect> alpha;
    try
    {
        alpha = std::make_unique<AlphaTestEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating alpha object (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<DualTextureEffect> dual;
    try
    {
        dual = std::make_unique<DualTextureEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating dual object (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<EnvironmentMapEffect> envmap;
    try
    {
        envmap = std::make_unique<EnvironmentMapEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating envmap object (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<SkinnedEffect> skin;
    try
    {
        skin = std::make_unique<SkinnedEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating skin object (except: %s)\n", e.what());
        success = false;
    }

    // Create input layouts
    ComPtr<ID3D11InputLayout> ilBasic;
    basic->SetTextureEnabled(true);
    basic->SetLightingEnabled(true);
    basic->SetVertexColorEnabled(true);
    HRESULT hr = CreateInputLayoutFromEffect<TestVertex>(device,
        basic.get(),
        ilBasic.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout for basic (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    ComPtr<ID3D11InputLayout> ilAlpha;
    alpha->SetVertexColorEnabled(true);
    hr = CreateInputLayoutFromEffect<TestVertex>(device,
        alpha.get(),
        ilAlpha.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout for alpha (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    ComPtr<ID3D11InputLayout> ilDual;
    dual->SetVertexColorEnabled(true);
    hr = CreateInputLayoutFromEffect<TestVertex>(device,
        dual.get(),
        ilDual.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout for dual (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    ComPtr<ID3D11InputLayout> ilEnvMap;
    hr = CreateInputLayoutFromEffect<VertexPositionNormalTexture>(device,
        envmap.get(),
        ilEnvMap.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout for envmap (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    ComPtr<ID3D11InputLayout> ilSkin;
    hr = CreateInputLayoutFromEffect<TestVertex>(device,
        skin.get(),
        ilSkin.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout for skin (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    // Apply
    ComPtr<ID3D11ShaderResourceView> defaultTex;
    {
        const uint32_t s_pixel = 0xffffffff;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            defaultTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    ComPtr<ID3D11ShaderResourceView> secondTex;
    {
        const uint32_t s_pixel = 0xf0f0f0f0;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            secondTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    try
    {
        context->IASetInputLayout(ilBasic.Get());

        basic->SetTexture(defaultTex.Get());

        for(int combos = 0; combos <= 0x3f; ++combos)
        {
            basic->SetLightingEnabled((combos & 0x1) ? true : false);
            basic->SetFogEnabled((combos & 0x2) ? true : false);
            basic->SetVertexColorEnabled((combos & 0x4) ? true : false);
            basic->SetTextureEnabled((combos & 0x8) ? true : false);
            basic->SetPerPixelLighting((combos & 0x10) ? true : false);
            basic->SetBiasedVertexNormals((combos & 0x20) ? true : false);
            basic->Apply(context.Get());
        }
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed applying basic (except: %s)\n", e.what());
        success = false;
    }

    try
    {
        context->IASetInputLayout(ilAlpha.Get());

        alpha->SetTexture(defaultTex.Get());

        for(int combos = 0; combos <= 0x7; ++combos)
        {
            alpha->SetFogEnabled((combos & 0x1) ? true : false);
            alpha->SetVertexColorEnabled((combos & 0x2) ? true : false);
            alpha->SetAlphaFunction((combos & 0x4) ? D3D11_COMPARISON_GREATER : D3D11_COMPARISON_EQUAL);
            alpha->Apply(context.Get());
        }
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed applying alpha (except: %s)\n", e.what());
        success = false;
    }

    try
    {
        context->IASetInputLayout(ilDual.Get());

        dual->SetTexture(defaultTex.Get());
        dual->SetTexture2(secondTex.Get());

        for(int combos = 0; combos <= 0x3; ++combos)
        {
            dual->SetFogEnabled((combos & 0x1) ? true : false);
            dual->SetVertexColorEnabled((combos & 0x2) ? true : false);
            dual->Apply(context.Get());
        }
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed applying dual (except: %s)\n", e.what());
        success = false;
    }

    for(size_t j = 0; j < 3; ++j)
    {
        try
        {
            context->IASetInputLayout(ilEnvMap.Get());

            envmap->SetMode(static_cast<EnvironmentMapEffect::Mapping>(j));
            envmap->SetTexture(defaultTex.Get());
            envmap->SetEnvironmentMap(secondTex.Get());

            // TODO: Mapping_Sphere, Mapping_DualParabola
            for(int combos = 0; combos <= 0x1F; ++combos)
            {
                envmap->SetFogEnabled((combos & 0x1) ? true : false);
                envmap->SetPerPixelLighting((combos & 0x2) ? true : false);
                envmap->SetBiasedVertexNormals((combos & 0x4) ? true : false);
                envmap->SetFresnelFactor((combos & 0x8) ? 1.f : 0.f);
                envmap->SetEnvironmentMapSpecular((combos & 0x10) ? g_XMOne : g_XMZero);
                envmap->Apply(context.Get());
            }
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed applying envmap (except: %s)\n", e.what());
            success = false;
        }
    }

    for(size_t j = 1; j <= 4; ++j)
    {
        if (j == 3)
        {
            // Weights have to be 1, 2, or 4.
            continue;
        }

        try
        {
            context->IASetInputLayout(ilSkin.Get());

            skin->SetWeightsPerVertex(static_cast<int>(j));
            skin->SetTexture(defaultTex.Get());

            for(int combos = 0; combos <= 0x7; ++combos)
            {
                skin->SetFogEnabled((combos & 0x1) ? true : false);
                skin->SetPerPixelLighting((combos & 0x2) ? true : false);
                skin->SetBiasedVertexNormals((combos & 0x4) ? true : false);
                skin->Apply(context.Get());
            }
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed applying skin (except: %s)\n", e.what());
            success = false;
        }
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        ID3D11Device* nullDevice = nullptr;
        try
        {
            auto invalid = std::make_unique<BasicEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<AlphaTestEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device alpha\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<DualTextureEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device dual\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<EnvironmentMapEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device envmap\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<SkinnedEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device skin\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<EffectFactory>(nullDevice);

            printf("ERROR: Failed to throw for null device for EffectFactory\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}

// NormalMapEffect, SkinnedNormalMapEffect, DebugEffect
_Success_(return)
bool Test11(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    bool success = true;

    // Create effect
    std::unique_ptr<NormalMapEffect> nmap;
    try
    {
        nmap = std::make_unique<NormalMapEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<SkinnedNormalMapEffect> skin;
    try
    {
        skin = std::make_unique<SkinnedNormalMapEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating skin object (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<DebugEffect> dbg;
    try
    {
        dbg = std::make_unique<DebugEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating debug object (except: %s)\n", e.what());
        success = false;
    }

    // Create input layouts
    ComPtr<ID3D11InputLayout> il;
    nmap->SetVertexColorEnabled(true);
    HRESULT hr = CreateInputLayoutFromEffect<TestVertex>(device,
        nmap.get(),
        il.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    ComPtr<ID3D11InputLayout> ilSkin;
    hr = CreateInputLayoutFromEffect<TestVertex>(device,
        skin.get(),
        ilSkin.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout for skin (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    ComPtr<ID3D11InputLayout> ilDebug;
    dbg->SetVertexColorEnabled(true);
    hr = CreateInputLayoutFromEffect<TestVertex>(device,
        dbg.get(),
        ilDebug.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout for debug (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    // Apply
    ComPtr<ID3D11ShaderResourceView> defaultTex;
    {
        const uint32_t s_pixel = 0xffffffff;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            defaultTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    ComPtr<ID3D11ShaderResourceView> normalTex;
    {
        const uint32_t s_pixel = 0xf0f0f0f0;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            normalTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    ComPtr<ID3D11ShaderResourceView> specularTex;
    {
        const uint32_t s_pixel = 0xabababff;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            specularTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    try
    {
        context->IASetInputLayout(il.Get());

        nmap->SetTexture(defaultTex.Get());
        nmap->SetNormalTexture(normalTex.Get());

        for(int combos = 0; combos <= 0xf; ++combos)
        {
            nmap->SetFogEnabled((combos & 0x1) ? true : false);
            nmap->SetVertexColorEnabled((combos & 0x2) ? true : false);
            nmap->SetBiasedVertexNormals((combos & 0x4) ? true : false);
            nmap->SetSpecularTexture((combos & 0x8) ? specularTex.Get() : nullptr);
            nmap->Apply(context.Get());
        }
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed applying nmap (except: %s)\n", e.what());
        success = false;
    }

    for(size_t j = 1; j <= 4; ++j)
    {
        if (j == 3)
        {
            // Weights have to be 1, 2, or 4.
            continue;
        }

        try
        {
            context->IASetInputLayout(ilSkin.Get());

            skin->SetWeightsPerVertex(static_cast<int>(j));
            skin->SetTexture(defaultTex.Get());
            skin->SetNormalTexture(normalTex.Get());

            for(int combos = 0; combos <= 0x7; ++combos)
            {
                skin->SetFogEnabled((combos & 0x1) ? true : false);
                skin->SetBiasedVertexNormals((combos & 0x2) ? true : false);
                skin->SetSpecularTexture((combos & 0x4) ? specularTex.Get() : nullptr);
                skin->Apply(context.Get());
            }
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed applying skin (except: %s)\n", e.what());
            success = false;
        }
    }

    for(size_t j = 0; j <= 3; ++j)
    {
        try
        {
            dbg->SetMode(static_cast<DebugEffect::Mode>(j));
            context->IASetInputLayout(ilDebug.Get());

            for(int combos = 0; combos <= 0x3; ++combos)
            {
                dbg->SetVertexColorEnabled((combos & 0x1) ? true : false);
                dbg->SetBiasedVertexNormals((combos & 0x2) ? true : false);
                dbg->Apply(context.Get());
            }
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed applying dbg (except: %s)\n", e.what());
            success = false;
        }
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        ID3D11Device* nullDevice = nullptr;
        try
        {
            auto invalid = std::make_unique<NormalMapEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<SkinnedNormalMapEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device skin\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<DebugEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device debug\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}

// PBREffect, SkinnedPBREffect, PBREffectFactory
_Success_(return)
bool Test12(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    bool success = true;

    // Create effect
    std::unique_ptr<PBREffect> pbr;
    try
    {
        pbr = std::make_unique<PBREffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<SkinnedPBREffect> skin;
    try
    {
        skin = std::make_unique<SkinnedPBREffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating skin object (except: %s)\n", e.what());
        success = false;
    }

    // Create input layouts
    ComPtr<ID3D11InputLayout> il;
    HRESULT hr = CreateInputLayoutFromEffect<VertexPositionNormalTexture>(device,
        pbr.get(),
        il.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    ComPtr<ID3D11InputLayout> ilSkin;
    hr = CreateInputLayoutFromEffect<TestVertex>(device,
        skin.get(),
        ilSkin.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout for skin (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    // Apply
    ComPtr<ID3D11ShaderResourceView> albetoTex;
    {
        const uint32_t s_pixel = 0xffffffff;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            albetoTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    ComPtr<ID3D11ShaderResourceView> normalTex;
    {
        const uint32_t s_pixel = 0xf0f0f0f0;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            normalTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    ComPtr<ID3D11ShaderResourceView> rmaTex;
    {
        const uint32_t s_pixel = 0xabababff;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            rmaTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    ComPtr<ID3D11ShaderResourceView> emissiveTex;
    {
        const uint32_t s_pixel = 0x11111111;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            emissiveTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    try
    {
        context->IASetInputLayout(il.Get());

        pbr->SetSurfaceTextures(albetoTex.Get(), normalTex.Get(), rmaTex.Get());

        for(int combos = 0; combos <= 0x3f; ++combos)
        {
            pbr->SetBiasedVertexNormals((combos & 0x1) ? true : false);
            pbr->SetEmissiveTexture((combos & 0x2) ? emissiveTex.Get() : nullptr);
            pbr->Apply(context.Get());
        }
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed applying pbr (except: %s)\n", e.what());
        success = false;
    }

    for(size_t j = 1; j <= 4; ++j)
    {
        if (j == 3)
        {
            // Weights have to be 1, 2, or 4.
            continue;
        }

        try
        {
            context->IASetInputLayout(ilSkin.Get());

            skin->SetWeightsPerVertex(static_cast<int>(j));
            skin->SetSurfaceTextures(albetoTex.Get(), normalTex.Get(), rmaTex.Get());

            for(int combos = 0; combos <= 0x3; ++combos)
            {
                skin->SetBiasedVertexNormals((combos & 0x1) ? true : false);
                skin->SetEmissiveTexture((combos & 0x2) ? emissiveTex.Get() : nullptr);
                skin->Apply(context.Get());
            }
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed applying skin (except: %s)\n", e.what());
            success = false;
        }
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        ID3D11Device* nullDevice = nullptr;
        try
        {
            auto invalid = std::make_unique<PBREffect>(nullDevice);

            printf("ERROR: Failed to throw for null device\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
        try
        {
            auto invalid = std::make_unique<SkinnedPBREffect>(nullDevice);

            printf("ERROR: Failed to throw for null device skin\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<PBREffectFactory>(nullDevice);

            printf("ERROR: Failed to throw for null device for PBREffectFactory\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}

// DGSLEffect, SkinnedDGSLEffect, DGSLEffectFactory
_Success_(return)
bool Test14(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    bool success = true;

    // Create effect
    std::unique_ptr<DGSLEffect> dgsl;
    try
    {
        dgsl = std::make_unique<DGSLEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        success = false;
    }

    std::unique_ptr<SkinnedDGSLEffect> skin;
    try
    {
        skin = std::make_unique<SkinnedDGSLEffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating skin object (except: %s)\n", e.what());
        success = false;
    }

    // Create input layouts
    ComPtr<ID3D11InputLayout> il;
    HRESULT hr = CreateInputLayoutFromEffect<VertexPositionNormalTangentColorTexture>(device,
        dgsl.get(),
        il.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    ComPtr<ID3D11InputLayout> ilSkin;
    hr = CreateInputLayoutFromEffect<VertexPositionNormalTangentColorTextureSkinning>(device,
        skin.get(),
        ilSkin.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout for skin (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    // Apply
    ComPtr<ID3D11ShaderResourceView> defaultTex;
    {
        const uint32_t s_pixel = 0xffffffff;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            defaultTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    try
    {
        context->IASetInputLayout(il.Get());

        dgsl->SetTexture(defaultTex.Get());

        for(int combos = 0; combos <= 0x7f; ++combos)
        {
            dgsl->SetLightingEnabled((combos & 0x1) ? true : false);
            dgsl->SetVertexColorEnabled((combos & 0x2) ? true : false);
            dgsl->SetTextureEnabled((combos & 0x4) ? true : false);
            dgsl->Apply(context.Get());
        }
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed applying dgsl (except: %s)\n", e.what());
        success = false;
    }

    for(size_t j = 1; j <= 4; ++j)
    {
        if (j == 3)
        {
            // Weights have to be 1, 2, or 4.
            continue;
        }

        try
        {
            context->IASetInputLayout(ilSkin.Get());

            skin->SetTexture(defaultTex.Get());
            skin->SetWeightsPerVertex(static_cast<int>(j));

            for(int combos = 0; combos <= 0x7f; ++combos)
            {
                skin->SetLightingEnabled((combos & 0x1) ? true : false);
                skin->SetVertexColorEnabled((combos & 0x2) ? true : false);
                skin->SetTextureEnabled((combos & 0x4) ? true : false);
                skin->Apply(context.Get());
            }
        }
        catch(const std::exception& e)
        {
            printf("ERROR: Failed applying skin (except: %s)\n", e.what());
            success = false;
        }
    }

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        ID3D11Device* nullDevice = nullptr;
        try
        {
            auto invalid = std::make_unique<DGSLEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<SkinnedDGSLEffect>(nullDevice);

            printf("ERROR: Failed to throw for null device skin\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }

        try
        {
            auto invalid = std::make_unique<DGSLEffectFactory>(nullDevice);

            printf("ERROR: Failed to throw for null device for DGSLEffectFactory\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}

// NPREffect
_Success_(return)
bool Test21(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    bool success = true;

    // Create effect
    std::unique_ptr<NPREffect> npr;
    try
    {
        npr = std::make_unique<NPREffect>(device);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed creating object (except: %s)\n", e.what());
        success = false;
    }

    // Create input layouts
    ComPtr<ID3D11InputLayout> il;
    npr->SetTextureEnabled(true);
    npr->SetVertexColorEnabled(true);
    HRESULT hr = CreateInputLayoutFromEffect<TestVertex>(device,
        npr.get(),
        il.GetAddressOf());
    if (FAILED(hr))
    {
        printf("ERROR: Failed creating input layout (%08X)\n", static_cast<unsigned int>(hr));
        success = false;
    }

    // Apply
    ComPtr<ID3D11ShaderResourceView> defaultTex;
    {
        const uint32_t s_pixel = 0xffffffff;

        D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

        hr = CreateTextureFromMemory(device, 1u, 1u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            nullptr,
            defaultTex.GetAddressOf());
        if (FAILED(hr))
        {
            printf("ERROR: failed to create needed test texture (%08X)\n", static_cast<unsigned int>(hr));
            success = false;
        }
    }

    // shader combos
    try
    {
        context->IASetInputLayout(il.Get());

        npr->SetTexture(defaultTex.Get());

        for(int combos = 0; combos <= 0x3f; ++combos)
        {
            npr->SetBiasedVertexNormals((combos & 0x1) ? true : false);
            npr->SetMode((combos & 0x2) ? NPREffect::Mode_Cel : NPREffect::Mode_Gooch);
            npr->Apply(context.Get());
        }
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed applying npr (except: %s)\n", e.what());
        success = false;
    }

    try
    {
        context->IASetInputLayout(il.Get());

        npr->SetTexture(defaultTex.Get());
        npr->SetMatCap(defaultTex.Get());

        for(int combos = 0; combos <= 0x1f; ++combos)
        {
            npr->SetBiasedVertexNormals((combos & 0x1) ? true : false);
            npr->SetMode(NPREffect::Mode_MatCap);
            npr->Apply(context.Get());
        }
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed applying matcap npr (except: %s)\n", e.what());
        success = false;
    }

    // camera settings
    try
    {
        XMMATRIX world = XMMatrixTranslation(1.f, 2.f, 3.f);
        npr->SetWorld(world);

        constexpr XMVECTORF32 pos = { { { 0.f, 0.f, -10.f, 0.f } } };
        XMMATRIX view = XMMatrixLookAtLH(pos, g_XMZero, g_XMIdentityR1);
        npr->SetView(view);

        XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.0f, 0.1f, 100.0f);
        npr->SetProjection(proj);

        npr->SetMatrices(world, view, proj);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed npr camera methods (except: %s)\n", e.what());
        success = false;
    }

    // light settings
    try
    {
        npr->SetLightDirection(0, g_XMIdentityR2);
        npr->SetLightDirection(1, g_XMIdentityR2);

        npr->EnableDefaultLighting();
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed npr light methods (except: %s)\n", e.what());
        success = false;
    }

    // material settings
    try
    {
        npr->SetDiffuseColor(Colors::Red);
        npr->SetSpecularColor(Colors::Blue);
        npr->SetRimLightingColor(Colors::Green);
        npr->SetAlpha(0.5f);
        npr->SetColorAndAlpha(Colors::White);

        npr->SetSpecularThreshold(0.5f, 0.003f);
        npr->DisableSpecular();

        npr->SetRimLightingPower(3.5f);
        npr->SetRimLightingIntensity(0.75f);
        npr->SetRimLightingRange(0.3f, 0.4f);
        npr->DisableRimLighting();

        npr->SetCelShaderBands(6);

        npr->SetGoochCoolColor(Colors::Blue, 0.1f);
        npr->SetGoochWarmColor(Colors::Red, 0.4);
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Failed npr materials methods (except: %s)\n", e.what());
        success = false;
    }

    // invalid args
    bool threw = false;
    try
    {
        npr->SetSpecularThreshold(2.f, 4.f);
    }
    catch(const std::invalid_argument&)
    {
        threw = true;
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Unexpected error for invalid specular threshold (except: %s)\n", e.what());
        success = false;
    }
    if (!threw)
    {
        printf("ERROR: Failed to throw for invalid specular threshold\n");
        success = false;
    }

    threw = false;
    try
    {
        npr->SetRimLightingIntensity(2.f);
    }
    catch(const std::invalid_argument&)
    {
        threw = true;
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Unexpected error for invalid rim lighting intensity (except: %s)\n", e.what());
        success = false;
    }
    if (!threw)
    {
        printf("ERROR: Failed to throw for invalid rim lighting intensity\n");
        success = false;
    }

    threw = false;
    try
    {
        npr->SetRimLightingRange(-1.f, -2.f);
    }
    catch(const std::invalid_argument&)
    {
        threw = true;
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Unexpected error for invalid rim lighting range (except: %s)\n", e.what());
        success = false;
    }
    if (!threw)
    {
        printf("ERROR: Failed to throw for invalid rim lighting range\n");
        success = false;
    }

    threw = false;
    try
    {
        npr->SetCelShaderBands(-1);
    }
    catch(const std::invalid_argument&)
    {
        threw = true;
    }
    catch(const std::exception& e)
    {
        printf("ERROR: Unexpected error for invalid cel shader bands (except: %s)\n", e.what());
        success = false;
    }
    if (!threw)
    {
        printf("ERROR: Failed to throw for invalid cel shader bands\n");
        success = false;
    }

    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        ID3D11Device* nullDevice = nullptr;
        try
        {
            auto invalid = std::make_unique<NPREffect>(nullDevice);

            printf("ERROR: Failed to throw for null device\n");
            success = false;
        }
        catch(const std::exception&)
        {
        }
    }
    #pragma warning(pop)

    return success;
}
