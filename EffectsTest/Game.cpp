//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK Effects
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"

#define GAMMA_CORRECT_RENDERING
#define USE_FAST_SEMANTICS

// Build for LH vs. RH coords
//#define LH_COORDS

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float rowA = -2.f;
    constexpr float row0 = 2.5f;
    constexpr float row1 = 1.5f;
    constexpr float row2 = 0.5f;
    constexpr float row3 = 0.f;
    constexpr float row4 = -0.5f;
    constexpr float row5 = -1.5f;
    constexpr float row6 = -2.5f;

    constexpr float colA = -5.f;
    constexpr float col0 = -4.f;
    constexpr float col1 = -3.f;
    constexpr float col2 = -2.f;
    constexpr float col3 = -1.f;
    constexpr float col4 = 0.f;
    constexpr float col5 = 1.f;
    constexpr float col6 = 2.f;
    constexpr float col7 = 3.f;
    constexpr float col8 = 4.f;
    constexpr float col9 = 5.f;

    struct TestVertex
    {
        TestVertex(FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate)
        {
            XMStoreFloat3(&this->position, position);
            XMStoreFloat3(&this->normal, normal);
            XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
            XMStoreFloat2(&this->textureCoordinate2, XMVectorScale(textureCoordinate, 3));
            XMStoreUByte4(&this->blendIndices, XMVectorSet(0, 1, 2, 3));

            float u = XMVectorGetX(textureCoordinate) - 0.5f;
            float v = XMVectorGetY(textureCoordinate) - 0.5f;

            float d = 1 - sqrt(u * u + v * v) * 2;

            if (d < 0)
                d = 0;

            XMStoreFloat4(&this->blendWeight, XMVectorSet(d, 1 - d, u, v));

            color = 0xFFFF00FF;
        }

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT2 textureCoordinate;
        XMFLOAT2 textureCoordinate2;
        XMUBYTE4 blendIndices;
        XMFLOAT4 blendWeight;
        XMUBYTE4 color;

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


    using VertexCollection = std::vector<TestVertex>;
    using IndexCollection = std::vector<uint16_t>;

    #include "../../Src/TeapotData.inc"

    // Tessellates the specified bezier patch.
    void TessellatePatch(VertexCollection& vertices, IndexCollection& indices, TeapotPatch const& patch, FXMVECTOR scale, bool isMirrored)
    {
        constexpr int tessellation = 16;

        // Look up the 16 control points for this patch.
        XMVECTOR controlPoints[16];

        for (int i = 0; i < 16; i++)
        {
            controlPoints[i] = XMVectorMultiply(TeapotControlPoints[patch.indices[i]], scale);
        }

        // Create the index data.
        Bezier::CreatePatchIndices(tessellation, isMirrored, [&](size_t index)
        {
            indices.push_back((uint16_t)(vertices.size() + index));
        });

        // Create the vertex data.
        Bezier::CreatePatchVertices(controlPoints, tessellation, isMirrored, [&](FXMVECTOR position, FXMVECTOR normal, FXMVECTOR textureCoordinate)
        {
            vertices.push_back(TestVertex(position, normal, textureCoordinate));
        });
    }


    // Creates a teapot primitive.
    UINT CreateTeapot(ID3D11Device* device, ID3D11Buffer** vertexBuffer, ID3D11Buffer** indexBuffer)
    {
        VertexCollection vertices;
        IndexCollection indices;

        XMVECTOR negateXZ = XMVectorMultiply(g_XMNegateX, g_XMNegateZ);

        for (size_t i = 0; i < sizeof(TeapotPatches) / sizeof(TeapotPatches[0]); i++)
        {
            TeapotPatch const& patch = TeapotPatches[i];

            // Because the teapot is symmetrical from left to right, we only store
            // data for one side, then tessellate each patch twice, mirroring in X.
            TessellatePatch(vertices, indices, patch, g_XMOne, false);
            TessellatePatch(vertices, indices, patch, g_XMNegateX, true);

            if (patch.mirrorZ)
            {
                // Some parts of the teapot (the body, lid, and rim, but not the
                // handle or spout) are also symmetrical from front to back, so
                // we tessellate them four times, mirroring in Z as well as X.
                TessellatePatch(vertices, indices, patch, g_XMNegateZ, true);
                TessellatePatch(vertices, indices, patch, negateXZ, false);
            }
        }

        // Create the D3D buffers.
        DX::ThrowIfFailed(
            CreateStaticBuffer(device, vertices, D3D11_BIND_VERTEX_BUFFER, vertexBuffer)
        );

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, indexBuffer)
        );

        return static_cast<UINT>(indices.size());
    }
} // anonymous namespace

_Use_decl_annotations_
void Game::CreateTestInputLayout(ID3D11Device* device, IEffect* effect, ID3D11InputLayout** pInputLayout)
{
    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<TestVertex>(device, effect, pInputLayout)
    );
}

Game::Game() noexcept(false) :
    m_indexCount(0)
{
#ifdef GAMMA_CORRECT_RENDERING
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif

#ifdef XBOX
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(UWP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0,
        DX::DeviceResources::c_Enable4K_Xbox
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(c_RenderFormat);
#endif

#ifdef LOSTDEVICE
    m_deviceResources->RegisterDeviceNotify(this);
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(
#ifdef COREWINDOW
    IUnknown* window,
#else
    HWND window,
#endif
    int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();

#ifdef XBOX
    UNREFERENCED_PARAMETER(rotation);
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    m_deviceResources->SetWindow(window);
#ifdef COREWINDOW
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#endif
#elif defined(UWP)
    m_deviceResources->SetWindow(window, width, height, rotation);
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#else
    UNREFERENCED_PARAMETER(rotation);
    m_deviceResources->SetWindow(window, width, height);
#endif

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const&)
{
    auto pad = m_gamePad->GetState(0);
    auto kb = m_keyboard->GetState();
    if (kb.Escape || (pad.IsConnected() && pad.IsViewPressed()))
    {
        ExitGame();
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

#ifdef XBOX
    m_deviceResources->Prepare();
#endif

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    // Time-based animation
    float time = static_cast<float>(m_timer.GetTotalSeconds());

    float alphaFade = (sin(time * 2) + 1) / 2;

    if (alphaFade >= 1)
        alphaFade = 1 - FLT_EPSILON;

    float yaw = time * 0.4f;
    float pitch = time * 0.7f;
    float roll = time * 1.1f;

    XMMATRIX world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

    // Set state objects.
    context->OMSetBlendState(m_states->AlphaBlend(), Colors::White, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
#ifdef LH_COORDS
    context->RSSetState(m_states->CullClockwise());
#else
    context->RSSetState(m_states->CullCounterClockwise());
#endif

    ID3D11SamplerState* samplers[] =
    {
        m_states->LinearWrap(),
        m_states->LinearWrap(),
    };

    context->PSSetSamplers(0, 2, samplers);

    // Set the vertex and index buffer.
    UINT vertexStride = sizeof(TestVertex);
    UINT vertexOffset = 0;

    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);

    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Determine fog distance.
#ifdef LH_COORDS
    const float fogstart = -6;
    const float fogend = -8;
#else
    const float fogstart = 6;
    const float fogend = 8;
#endif

    Vector4 red, blue, gray;
#ifdef GAMMA_CORRECT_RENDERING
    red = XMColorSRGBToRGB(Colors::Red);
    blue = XMColorSRGBToRGB(Colors::Blue);
    gray = XMColorSRGBToRGB(Colors::Gray);
#else
    red = Colors::Red;
    blue = Colors::Blue;
    gray = Colors::Gray;
#endif

    // Abstract effect
    m_abstractEffect->Apply(context);

    //--- BasicEFfect ----------------------------------------------------------------------

    // Simple unlit teapot.
    m_basicEffectUnlit->Apply(context, world * XMMatrixTranslation(col0, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Unlit with alpha fading.
    m_basicEffectUnlit->SetAlpha(alphaFade);
    m_basicEffectUnlit->Apply(context, world * XMMatrixTranslation(col0, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_basicEffectUnlit->SetAlpha(1);

    // Unlit with fog.
    m_basicEffectUnlit->SetFogEnabled(true);
    m_basicEffectUnlit->SetFogStart(fogstart);
    m_basicEffectUnlit->SetFogEnd(fogend);
    m_basicEffectUnlit->SetFogColor(gray);
    m_basicEffectUnlit->Apply(context, world * XMMatrixTranslation(col0, row2, 2 - alphaFade * 6), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_basicEffectUnlit->SetFogEnabled(false);

    // Simple unlit teapot with vertex colors.
    m_basicEffectUnlitVc->Apply(context, world * XMMatrixTranslation(colA, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Simple unlit teapot with alpha fading with vertex colors.
    m_basicEffectUnlitVc->SetAlpha(alphaFade);
    m_basicEffectUnlitVc->Apply(context, world * XMMatrixTranslation(colA, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_basicEffectUnlitVc->SetAlpha(1);

    // Unlit with fog with vertex colors.
    m_basicEffectUnlitVc->SetFogEnabled(true);
    m_basicEffectUnlitVc->SetFogStart(fogstart);
    m_basicEffectUnlitVc->SetFogEnd(fogend);
    m_basicEffectUnlitVc->SetFogColor(gray);
    m_basicEffectUnlitVc->Apply(context, world * XMMatrixTranslation(colA, row2, 2 - alphaFade * 6), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_basicEffectUnlitVc->SetFogEnabled(false);

    // Simple lit teapot.
    m_basicEffect->Apply(context, world * XMMatrixTranslation(col1, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Simple lit teapot, no specular
    m_basicEffectNoSpecular->Apply(context, world * XMMatrixTranslation(col2, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Simple lit with alpha fading.
    m_basicEffect->SetAlpha(alphaFade);
    m_basicEffect->Apply(context, world * XMMatrixTranslation(col1, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_basicEffect->SetAlpha(1);

    // Simple lit alpha fading, no specular.
    m_basicEffectNoSpecular->SetAlpha(alphaFade);
    m_basicEffectNoSpecular->Apply(context, world * XMMatrixTranslation(col3, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_basicEffectNoSpecular->SetAlpha(1);

    // Simple lit with fog.
    m_basicEffect->SetFogEnabled(true);
    m_basicEffect->SetFogStart(fogstart);
    m_basicEffect->SetFogEnd(fogend);
    m_basicEffect->SetFogColor(gray);
    m_basicEffect->Apply(context, world * XMMatrixTranslation(col1, row2, 2 - alphaFade * 6), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_basicEffect->SetFogEnabled(false);

    m_basicEffect->SetLightEnabled(1, false);
    m_basicEffect->SetLightEnabled(2, false);

    {
        // Light only from above.
        m_basicEffect->SetLightDirection(0, XMVectorSet(0, -1, 0, 0));
        m_basicEffect->Apply(context, world * XMMatrixTranslation(col2, row0, 0), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);

        // Light only from the left.
        m_basicEffect->SetLightDirection(0, XMVectorSet(1, 0, 0, 0));
        m_basicEffect->Apply(context, world * XMMatrixTranslation(col3, row0, 0), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);

        // Light only from straight in front.
        m_basicEffect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
        m_basicEffect->Apply(context, world * XMMatrixTranslation(col4, row0, 0), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);
    }

    m_basicEffect->EnableDefaultLighting();

    // Non uniform scaling.
    m_basicEffect->Apply(context, XMMatrixScaling(1, 2, 0.25f) * world * XMMatrixTranslation(col5, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_basicEffect->SetPerPixelLighting(true);

    {
        m_basicEffect->SetLightEnabled(1, false);
        m_basicEffect->SetLightEnabled(2, false);

        {
            // Light only from above + per pixel lighting.
            m_basicEffect->SetLightDirection(0, XMVectorSet(0, -1, 0, 0));
            m_basicEffect->Apply(context, world * XMMatrixTranslation(col2, row1, 0), m_view, m_projection);
            context->DrawIndexed(m_indexCount, 0, 0);

            // Light only from the left + per pixel lighting.
            m_basicEffect->SetLightDirection(0, XMVectorSet(1, 0, 0, 0));
            m_basicEffect->Apply(context, world * XMMatrixTranslation(col3, row1, 0), m_view, m_projection);
            context->DrawIndexed(m_indexCount, 0, 0);

            // Light only from straight in front + per pixel lighting.
            m_basicEffect->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
            m_basicEffect->Apply(context, world * XMMatrixTranslation(col4, row1, 0), m_view, m_projection);
            context->DrawIndexed(m_indexCount, 0, 0);
        }

        m_basicEffect->EnableDefaultLighting();

        // Non uniform scaling + per pixel lighting.
        m_basicEffect->Apply(context, XMMatrixScaling(1, 2, 0.25f) * world * XMMatrixTranslation(col5, row1, 0), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);
    }

    m_basicEffect->SetPerPixelLighting(false);

    //--- SkinnedEFfect --------------------------------------------------------------------

    // Skinned effect, identity transforms.
    XMMATRIX bones[4] =
    {
        XMMatrixIdentity(),
        XMMatrixIdentity(),
        XMMatrixScaling(0, 0, 0),
        XMMatrixScaling(0, 0, 0),
    };

    m_skinnedEffect->SetBoneTransforms(bones, 4);
    m_skinnedEffect->Apply(context, world, m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_skinnedEffectNoSpecular->SetBoneTransforms(bones, 4);
    m_skinnedEffectNoSpecular->Apply(context, world * XMMatrixTranslation(col5, row3, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Skinned effect with fog.
    m_skinnedEffect->SetBoneTransforms(bones, 4);
    m_skinnedEffect->SetFogEnabled(true);
    m_skinnedEffect->SetFogStart(fogstart);
    m_skinnedEffect->SetFogEnd(fogend);
    m_skinnedEffect->SetFogColor(gray);
    m_skinnedEffect->Apply(context, world * XMMatrixTranslation(colA, rowA, 2 - alphaFade * 6), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_skinnedEffect->SetFogEnabled(false);

    m_skinnedEffect->SetPerPixelLighting(true);

    m_skinnedEffect->SetBoneTransforms(bones, 4);
    m_skinnedEffect->SetFogEnabled(true);
    m_skinnedEffect->SetFogStart(fogstart);
    m_skinnedEffect->SetFogEnd(fogend);
    m_skinnedEffect->SetFogColor(gray);
    m_skinnedEffect->Apply(context, world * XMMatrixTranslation(col0, rowA, 2 - alphaFade * 6), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_skinnedEffect->SetFogEnabled(false);

    m_skinnedEffect->SetPerPixelLighting(false);

    // Skinned effect, variable scaling transforms.
    float scales[4] =
    {
        1 + sin(time * 1.7f) * 0.5f,
        1 + sin(time * 2.3f) * 0.5f,
        0,
        0,
    };

    for (int i = 0; i < 4; i++)
    {
        bones[i] = XMMatrixScaling(scales[i], scales[i], scales[i]);
    }

    m_skinnedEffect->SetBoneTransforms(bones, 4);
    m_skinnedEffect->Apply(context, world * XMMatrixTranslation(col3, rowA, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Skinned effect, different variable scaling transforms.
    float scales2[4] =
    {
        1,
        1,
        sin(time * 2.3f) * 0.5f,
        sin(time * 3.1f) * 0.5f,
    };

    for (int i = 0; i < 4; i++)
    {
        bones[i] = XMMatrixScaling(scales2[i], scales2[i], scales2[i]);
    }

    m_skinnedEffect->SetPerPixelLighting(true);
    m_skinnedEffect->SetBoneTransforms(bones, 4);
    m_skinnedEffect->Apply(context, world * XMMatrixTranslation(col1, rowA, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_skinnedEffect->SetPerPixelLighting(false);

    //--- EnvironmentMapEffect -------------------------------------------------------------

    // Environment map effect.
    m_envmap->Apply(context, world * XMMatrixTranslation(col6, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Environment map with alpha fading.
    m_envmap->SetAlpha(alphaFade);
    m_envmap->Apply(context, world * XMMatrixTranslation(col6, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_envmap->SetAlpha(1);

    // Environment map with fog.
    m_envmap->SetFogEnabled(true);
    m_envmap->SetFogStart(fogstart);
    m_envmap->SetFogEnd(fogend);
    m_envmap->SetFogColor(gray);
    m_envmap->Apply(context, world * XMMatrixTranslation(col6, row2, 2 - alphaFade * 6), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_envmap->SetFogEnabled(false);

    // Environment map, animating the fresnel factor.
    m_envmap->SetFresnelFactor(alphaFade * 3);
    m_envmap->Apply(context, world * XMMatrixTranslation(col6, row4, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_envmap->SetFresnelFactor(1);

    // Environment map, animating the amount, with no fresnel.
    m_envmap->SetEnvironmentMapAmount(alphaFade);
    m_envmap->SetFresnelFactor(0);
    m_envmap->Apply(context, world * XMMatrixTranslation(col6, row5, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_envmap->SetEnvironmentMapAmount(1);
    m_envmap->SetFresnelFactor(1);

    // Environment map, animating the amount.
    m_envmap->SetEnvironmentMapAmount(alphaFade);
    m_envmap->Apply(context, world * XMMatrixTranslation(col6, row6, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_envmap->SetEnvironmentMapAmount(1);

    // Environment map, with animating specular
    m_envmap->SetEnvironmentMapSpecular(blue * alphaFade);
    m_envmap->Apply(context, world * XMMatrixTranslation(col5, row5, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_envmap->SetEnvironmentMapSpecular(Colors::Black);

    // Environment map, with single light vertex color
    m_envmap->SetLightEnabled(1, false);
    m_envmap->SetLightEnabled(2, false);
    m_envmap->Apply(context, world * XMMatrixTranslation(col5, row6, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_envmap->SetPerPixelLighting(true);

    {
        m_envmap->SetLightEnabled(1, false);
        m_envmap->SetLightEnabled(2, false);

        {
            // Light only from above + per pixel lighting, animating the fresnel factor.
            m_envmap->SetLightDirection(0, XMVectorSet(0, -1, 0, 0));
            m_envmap->SetFresnelFactor(alphaFade * 3);
            m_envmap->Apply(context, world * XMMatrixTranslation(col7, row4, 0), m_view, m_projection);
            context->DrawIndexed(m_indexCount, 0, 0);
            m_envmap->SetFresnelFactor(1);

            // Light only from the left + per pixel lighting, animating the amount, with no fresnel.
            m_envmap->SetEnvironmentMapAmount(alphaFade);
            m_envmap->SetFresnelFactor(0);
            m_envmap->SetLightDirection(0, XMVectorSet(1, 0, 0, 0));
            m_envmap->Apply(context, world * XMMatrixTranslation(col7, row5, 0), m_view, m_projection);
            context->DrawIndexed(m_indexCount, 0, 0);
            m_envmap->SetEnvironmentMapAmount(1);
            m_envmap->SetFresnelFactor(1);

            // Light only from straight in front + per pixel lighting with fog.
            m_envmap->SetLightDirection(0, XMVectorSet(0, 0, -1, 0));
            m_envmap->SetFogEnabled(true);
            m_envmap->SetFogStart(fogstart);
            m_envmap->SetFogEnd(fogend);
            m_envmap->SetFogColor(gray);
            m_envmap->Apply(context, world * XMMatrixTranslation(col7, row6, 2 - alphaFade * 6), m_view, m_projection);
            context->DrawIndexed(m_indexCount, 0, 0);
            m_envmap->SetFogEnabled(false);
        }

        m_envmap->EnableDefaultLighting();
    }

    m_envmap->SetPerPixelLighting(false);

    m_spheremap->Apply(context, world * XMMatrixTranslation(col4, row5, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_dparabolamap->Apply(context, world * XMMatrixTranslation(col4, row6, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    //--- DualTextureEFfect ----------------------------------------------------------------

    // Dual texture effect.
    m_dualTexture->Apply(context, world * XMMatrixTranslation(col7, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Dual texture with alpha fading.
    m_dualTexture->SetAlpha(alphaFade);
    m_dualTexture->Apply(context, world * XMMatrixTranslation(col7, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_dualTexture->SetAlpha(1);

    // Dual texture with fog.
    m_dualTexture->SetFogEnabled(true);
    m_dualTexture->SetFogStart(fogstart);
    m_dualTexture->SetFogEnd(fogend);
    m_dualTexture->SetFogColor(gray);
    m_dualTexture->Apply(context, world * XMMatrixTranslation(col7, row2, 2 - alphaFade * 6), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_dualTexture->SetFogEnabled(false);

    //--- AlphaTestEFfect ------------------------------------------------------------------

    context->OMSetBlendState(m_states->Opaque(), Colors::White, 0xFFFFFFFF);

    {
        // Alpha test, > 0.
        m_alphaTest->SetAlphaFunction(D3D11_COMPARISON_GREATER);
        m_alphaTest->SetReferenceAlpha(0);
        m_alphaTest->Apply(context, world * XMMatrixTranslation(col8, row0, 0), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);

        // Alpha test, > 128.
        m_alphaTest->SetAlphaFunction(D3D11_COMPARISON_GREATER);
        m_alphaTest->SetReferenceAlpha(128);
        m_alphaTest->Apply(context, world * XMMatrixTranslation(col8, row1, 0), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);

        // Alpha test with fog.
        m_alphaTest->SetFogEnabled(true);
        m_alphaTest->SetFogStart(fogstart);
        m_alphaTest->SetFogEnd(fogend);
        m_alphaTest->SetFogColor(red);
        m_alphaTest->Apply(context, world * XMMatrixTranslation(col8, row2, 2 - alphaFade * 6), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);
        m_alphaTest->SetFogEnabled(false);

        // Alpha test, < animating value.
        m_alphaTest->SetAlphaFunction(D3D11_COMPARISON_LESS);
        m_alphaTest->SetReferenceAlpha(1 + (int)(alphaFade * 254));
        m_alphaTest->Apply(context, world * XMMatrixTranslation(col8, row4, 0), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);

        // Alpha test, = 255.
        m_alphaTest->SetAlphaFunction(D3D11_COMPARISON_EQUAL);
        m_alphaTest->SetReferenceAlpha(255);
        m_alphaTest->Apply(context, world * XMMatrixTranslation(col8, row5, 0), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);

        // Alpha test, != 0.
        m_alphaTest->SetAlphaFunction(D3D11_COMPARISON_NOT_EQUAL);
        m_alphaTest->SetReferenceAlpha(0);
        m_alphaTest->Apply(context, world * XMMatrixTranslation(col8, row6, 0), m_view, m_projection);
        context->DrawIndexed(m_indexCount, 0, 0);
    }

    context->OMSetBlendState(m_states->AlphaBlend(), Colors::White, 0xFFFFFFFF);

    //--- NormalMapEffect ------------------------------------------------------------------

    // NormalMapEffect
    m_normalMapEffect->Apply(context, world * XMMatrixTranslation(col9, row0, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // NormalMapEffect no spec
    m_normalMapEffectNoSpecular->Apply(context, world * XMMatrixTranslation(col9, row1, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // NormalMap with fog.
    m_normalMapEffect->SetFogEnabled(true);
    m_normalMapEffect->SetFogStart(fogstart);
    m_normalMapEffect->SetFogEnd(fogend);
    m_normalMapEffect->SetFogColor(gray);
    m_normalMapEffect->Apply(context, world * XMMatrixTranslation(col9, row2, 2 - alphaFade * 6), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);
    m_normalMapEffect->SetFogEnabled(false);

    // NormalMap with default diffuse
    m_normalMapEffectNoDiffuse->Apply(context, world * XMMatrixTranslation(col9, row4, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // NormalMap with default diffuse no spec
    m_normalMapEffectNormalsOnly->Apply(context, world * XMMatrixTranslation(col9, row5, 0), m_view, m_projection);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Show the new frame.
    m_deviceResources->Present();

#ifdef XBOX
    m_graphicsMemory->Commit();
#endif
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    XMVECTORF32 color;
#ifdef GAMMA_CORRECT_RENDERING
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
#else
    color.v = Colors::CornflowerBlue;
#endif
    context->ClearRenderTargetView(renderTarget, color);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Game::OnResuming()
{
    m_deviceResources->Resume();

    m_timer.ResetElapsedTime();
}

#ifdef PC
void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

#ifndef XBOX
void Game::OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation)
{
#ifdef UWP
    if (!m_deviceResources->WindowSizeChanged(width, height, rotation))
        return;
#else
    UNREFERENCED_PARAMETER(rotation);
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;
#endif

    CreateWindowSizeDependentResources();
}
#endif

#ifdef UWP
void Game::ValidateDevice()
{
    m_deviceResources->ValidateDevice();
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef XBOX
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device, m_deviceResources->GetBackBufferCount());
#endif

    m_states = std::make_unique<CommonStates>(device);

#ifdef GAMMA_CORRECT_RENDERING
    bool forceSRGB = true;
#else
    bool forceSRGB = false;
#endif

    // Load textures.
    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"cat.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_cat.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"opaqueCat.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_opaqueCat.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"cubemap.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_cubemap.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, L"spheremap.bmp",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_DEFAULT,
        nullptr, m_envball.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"dualparabola.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_envdual.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"overlay.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_overlay.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"default.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_defaultTex.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"spnza_bricks_a.DDS",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_brickDiffuse.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"spnza_bricks_a_normal.DDS",
        nullptr, m_brickNormal.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"spnza_bricks_a_specular.DDS",
        nullptr, m_brickSpecular.ReleaseAndGetAddressOf()));

    // Create test geometry.
    m_indexCount = CreateTeapot(device,
        m_vertexBuffer.ReleaseAndGetAddressOf(),
        m_indexBuffer.ReleaseAndGetAddressOf());

    // Create the shaders.
    m_basicEffectUnlit = std::make_unique<EffectWithDecl<BasicEffect>>(device, [](BasicEffect* effect)
    {
        XMVECTORF32 blue;
#ifdef GAMMA_CORRECT_RENDERING
        blue.v = XMColorSRGBToRGB(Colors::Blue);
#else
        blue.v = Colors::Blue;
#endif
        effect->SetDiffuseColor(blue);
    });

    m_basicEffectUnlitVc = std::make_unique<EffectWithDecl<BasicEffect>>(device, [](BasicEffect* effect)
    {
        effect->SetVertexColorEnabled(true);
    });

    m_basicEffect = std::make_unique<EffectWithDecl<BasicEffect>>(device, [=](BasicEffect* effect)
    {
        XMVECTORF32 red;
#ifdef GAMMA_CORRECT_RENDERING
        red.v = XMColorSRGBToRGB(Colors::Red);
#else
        red.v = Colors::Red;
#endif
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(red);
    });

    m_basicEffectNoSpecular = std::make_unique<EffectWithDecl<BasicEffect>>(device, [](BasicEffect* effect)
    {
        XMVECTORF32 red;
#ifdef GAMMA_CORRECT_RENDERING
        red.v = XMColorSRGBToRGB(Colors::Red);
#else
        red.v = Colors::Red;
#endif
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(red);
        effect->DisableSpecular();
    });

    m_skinnedEffect = std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [&](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_opaqueCat.Get());
    });

    m_skinnedEffectNoSpecular = std::make_unique<EffectWithDecl<SkinnedEffect>>(device, [&](SkinnedEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_opaqueCat.Get());
        effect->DisableSpecular();
    });

    m_envmap = std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [&](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetTexture(m_opaqueCat.Get());
        effect->SetEnvironmentMap(m_cubemap.Get());
    });

    m_spheremap = std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [&](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetFresnelFactor(0.f);
        effect->SetEnvironmentMapAmount(1.0f);
        effect->SetMode(EnvironmentMapEffect::Mapping_Sphere);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envball.Get());
    });

    m_dparabolamap = std::make_unique<EffectWithDecl<EnvironmentMapEffect>>(device, [&](EnvironmentMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetFresnelFactor(0.f);
        effect->SetEnvironmentMapAmount(1.0f);
        effect->SetMode(EnvironmentMapEffect::Mapping_DualParabola);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetEnvironmentMap(m_envdual.Get());
    });

    m_dualTexture = std::make_unique<EffectWithDecl<DualTextureEffect>>(device, [&](DualTextureEffect* effect)
    {
        effect->SetTexture(m_opaqueCat.Get());
        effect->SetTexture2(m_overlay.Get());
    });

    m_alphaTest = std::make_unique<EffectWithDecl<AlphaTestEffect>>(device, [&](AlphaTestEffect* effect)
    {
        effect->SetTexture(m_cat.Get());
    });

    m_normalMapEffect = std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [&](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(Colors::White);
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
        effect->SetSpecularTexture(m_brickSpecular.Get());
    });

    m_normalMapEffectNoDiffuse = std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [&](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->SetDiffuseColor(Colors::White);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
        effect->SetSpecularTexture(m_brickSpecular.Get());
    });

    m_normalMapEffectNormalsOnly = std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [&](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->DisableSpecular();
        effect->SetDiffuseColor(Colors::White);
        effect->SetTexture(m_defaultTex.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
    });

    m_normalMapEffectNoSpecular = std::make_unique<EffectWithDecl<NormalMapEffect>>(device, [&](NormalMapEffect* effect)
    {
        effect->EnableDefaultLighting();
        effect->DisableSpecular();
        effect->SetDiffuseColor(Colors::White);
        effect->SetTexture(m_brickDiffuse.Get());
        effect->SetNormalTexture(m_brickNormal.Get());
    });

    // Abstract
    m_abstractEffect = std::make_unique<BasicEffect>(device);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { { { 0.f, 0.f, 6.f, 0.f } } };

    auto size = m_deviceResources->GetOutputSize();
    float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    m_view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 15);
#else
    m_view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 15);
#endif

#ifdef UWP
    {
        auto orient3d = m_deviceResources->GetOrientationTransform3D();
        XMMATRIX orient = XMLoadFloat4x4(&orient3d);
        m_projection *= orient;
    }
#endif
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_states.reset();

    m_abstractEffect.reset();

    m_basicEffectUnlit.reset();
    m_basicEffectUnlitVc.reset();
    m_basicEffect.reset();
    m_basicEffectNoSpecular.reset();

    m_skinnedEffect.reset();
    m_skinnedEffectNoSpecular.reset();

    m_envmap.reset();
    m_spheremap.reset();
    m_dparabolamap.reset();

    m_dualTexture.reset();

    m_alphaTest.reset();

    m_normalMapEffect.reset();
    m_normalMapEffectNoDiffuse.reset();
    m_normalMapEffectNormalsOnly.reset();
    m_normalMapEffectNoSpecular.reset();

    m_cat.Reset();
    m_opaqueCat.Reset();
    m_cubemap.Reset();
    m_envball.Reset();
    m_envdual.Reset();
    m_overlay.Reset();
    m_defaultTex.Reset();
    m_brickDiffuse.Reset();
    m_brickNormal.Reset();
    m_brickSpecular.Reset();

    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
