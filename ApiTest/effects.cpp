//--------------------------------------------------------------------------------------
// File: effects.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "Effects.h"

#include "BufferHelpers.h"
#include "DirectXHelpers.h"
#include "VertexTypes.h"

#include <cstdio>
#include <type_traits>

#include <wrl/client.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

static_assert(std::is_nothrow_move_constructible<BasicEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<BasicEffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<AlphaTestEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<AlphaTestEffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<DualTextureEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DualTextureEffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<EnvironmentMapEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<EnvironmentMapEffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<SkinnedEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SkinnedEffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<NormalMapEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<NormalMapEffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<SkinnedNormalMapEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SkinnedNormalMapEffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<PBREffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<PBREffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<SkinnedPBREffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SkinnedPBREffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<DebugEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DebugEffect>::value, "Move Assign.");

// VS 2017 on XDK incorrectly thinks it's not noexcept
static_assert(std::is_nothrow_move_constructible<IEffectFactory::EffectInfo>::value, "Move Ctor.");
static_assert(std::is_move_assignable<IEffectFactory::EffectInfo>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<EffectFactory>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<EffectFactory>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<PBREffectFactory>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<PBREffectFactory>::value, "Move Assign.");

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

_Success_(return)
bool Test00(_In_ ID3D11Device *device)
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

    return success;
}

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

    return success;
}

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

    return success;
}
