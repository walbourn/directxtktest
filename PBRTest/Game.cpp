//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK PBR Effect Test
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"
#include "vbo.h"

//#define USE_FAST_SEMANTICS

// Build for LH vs. RH coords
//#define LH_COORDS

// For UWP/PC, this tests using a linear F16 swapchain intead of HDR10
//#define TEST_HDR_LINEAR

extern void ExitGame() noexcept;

#ifdef XBOX
extern bool g_HDRMode;
#endif

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace
{
    // Linear colors for DirectXMath were not added until v3.17 in the Windows SDK (22621)
    const XMVECTORF32 c_clearColor = { { { 0.127437726f, 0.300543845f, 0.846873462f, 1.f } } };

    constexpr XMVECTORF32 c_BrightYellow = { { { 2.f, 2.f, 0.f, 1.f } } };

    constexpr float col0 = -4.25f;
    constexpr float col1 = -3.f;
    constexpr float col2 = -1.75f;
    constexpr float col3 = -.6f;
    constexpr float col4 = .6f;
    constexpr float col5 = 1.75f;
    constexpr float col6 = 3.f;
    constexpr float col7 = 4.25f;

    constexpr float row0 = 2.f;
    constexpr float row1 = 0.25f;
    constexpr float row2 = -1.1f;
    constexpr float row3 = -2.5f;

    void ReadVBO(_In_z_ const wchar_t* name, GeometricPrimitive::VertexCollection& vertices, GeometricPrimitive::IndexCollection& indices)
    {
        std::vector<uint8_t> blob;
        {
            std::ifstream inFile(name, std::ios::in | std::ios::binary | std::ios::ate);

            if (!inFile)
                throw std::runtime_error("ReadVBO");

            std::streampos len = inFile.tellg();
            if (!inFile)
                throw std::runtime_error("ReadVBO");

            if (len < static_cast<std::streampos>(sizeof(VBO::header_t)))
                throw std::runtime_error("ReadVBO");

            blob.resize(size_t(len));

            inFile.seekg(0, std::ios::beg);
            if (!inFile)
                throw std::runtime_error("ReadVBO");

            inFile.read(reinterpret_cast<char*>(blob.data()), len);
            if (!inFile)
                throw std::runtime_error("ReadVBO");

            inFile.close();
        }

        auto hdr = reinterpret_cast<const VBO::header_t*>(blob.data());

        if (!hdr->numIndices || !hdr->numVertices)
            throw std::runtime_error("ReadVBO");

        static_assert(sizeof(VertexPositionNormalTexture) == 32, "VBO vertex size mismatch");

        size_t vertSize = sizeof(VertexPositionNormalTexture) * hdr->numVertices;
        if (blob.size() < (vertSize + sizeof(VBO::header_t)))
            throw std::runtime_error("End of file");

        size_t indexSize = sizeof(uint16_t) * hdr->numIndices;
        if (blob.size() < (sizeof(VBO::header_t) + vertSize + indexSize))
            throw std::runtime_error("End of file");

        vertices.resize(hdr->numVertices);
        auto verts = reinterpret_cast<const VertexPositionNormalTexture*>(blob.data() + sizeof(VBO::header_t));
        memcpy_s(vertices.data(), vertices.size() * sizeof(VertexPositionNormalTexture), verts, vertSize);

        indices.resize(hdr->numIndices);
        auto tris = reinterpret_cast<const uint16_t*>(blob.data() + sizeof(VBO::header_t) + vertSize);
        memcpy_s(indices.data(), indices.size() * sizeof(uint16_t), tris, indexSize);
    }
}

// Constructor.
Game::Game() noexcept(false) :
    m_indexCount(0),
    m_indexCountCube(0),
    m_toneMapMode(ToneMapPostProcess::Reinhard),
    m_ibl(0),
    m_spinning(true),
    m_showDebug(false),
    m_debugMode(DebugEffect::Mode_Default),
    m_pitch(0),
    m_yaw(0)
{
#if defined(TEST_HDR_LINEAR) && !defined(XBOX)
    constexpr DXGI_FORMAT c_DisplayFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
#else
    constexpr DXGI_FORMAT c_DisplayFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
#endif

#ifdef XBOX
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_DisplayFormat, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        | DX::DeviceResources::c_EnableHDR);
#elif defined(UWP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_DisplayFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0,
        DX::DeviceResources::c_EnableHDR | DX::DeviceResources::c_Enable4K_Xbox | DX::DeviceResources::c_EnableQHD_Xbox);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_DisplayFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0,
        DX::DeviceResources::c_EnableHDR);
#endif

#ifdef LOSTDEVICE
    m_deviceResources->RegisterDeviceNotify(this);
#endif

    // Set up for HDR rendering.
    m_hdrScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
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
    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitGame();
        }

        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED
            || m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            ++m_ibl;
            if (m_ibl >= s_nIBL)
            {
                m_ibl = 0;
            }
        }
        else if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED
                 || m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            if (!m_ibl)
                m_ibl = s_nIBL - 1;
            else
                --m_ibl;
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_spinning = !m_spinning;
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            CycleDebug();
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            CycleToneMapOperator();
        }

        if (pad.IsLeftStickPressed())
        {
            m_spinning = false;
            m_yaw = m_pitch = 0.f;
        }
        else
        {
            m_yaw += pad.thumbSticks.leftX * 0.1f;
            m_pitch -= pad.thumbSticks.leftY * 0.1f;
        }
    }
    else
    {
        m_gamePadButtons.Reset();

        if (kb.A || kb.D)
        {
            m_spinning = false;
            m_yaw += (kb.D ? 0.1f : -0.1f);
        }

        if (kb.W || kb.S)
        {
            m_spinning = false;
            m_pitch += (kb.W ? 0.1f : -0.1f);
        }

        if (kb.Home)
        {
            m_spinning = false;
            m_yaw = m_pitch = 0.f;
        }
    }

    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }

    if (kb.Escape)
    {
        ExitGame();
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Enter) && !kb.LeftAlt && !kb.RightAlt)
    {
        ++m_ibl;
        if (m_ibl >= s_nIBL)
        {
            m_ibl = 0;
        }
    }
    else if (m_keyboardButtons.IsKeyPressed(Keyboard::Back))
    {
        if (!m_ibl)
            m_ibl = s_nIBL - 1;
        else
            --m_ibl;
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
    {
        m_spinning = !m_spinning;
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Tab))
    {
        CycleDebug();
    }

    if (m_keyboardButtons.pressed.T)
    {
        CycleToneMapOperator();
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

    const auto vp = m_deviceResources->GetOutputSize();
    const auto safeRect = Viewport::ComputeTitleSafeArea(UINT(vp.right - vp.left), UINT(vp.bottom - vp.top));

    // Time-based animation
    float time = static_cast<float>(m_timer.GetTotalSeconds());

    float alphaFade = (sin(time * 2) + 1) / 2;

    if (alphaFade >= 1)
        alphaFade = 1 - FLT_EPSILON;

    float yaw = time * 0.4f;
    float pitch = time * 0.7f;
    float roll = time * 1.1f;

    XMMATRIX world;

    if (m_spinning)
    {
        world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
    }
    else
    {
        world = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0);
    }

    // Setup for object drawing.
    UINT vertexStride = sizeof(GeometricPrimitive::VertexType);
    UINT vertexOffset = 0;

    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);

    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->OMSetBlendState(m_states->AlphaBlend(), Colors::White, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
#ifdef LH_COORDS
    context->RSSetState(m_states->CullClockwise());
#else
    context->RSSetState(m_states->CullCounterClockwise());
#endif

    ID3D11SamplerState* samplers[] =
    {
        m_states->AnisotropicClamp(),
        m_states->LinearWrap(),
    };

    context->PSSetSamplers(0, 2, samplers);

    if (m_showDebug)
    {
        //--- DebugEffect ------------------------------------------------------------------
        context->IASetInputLayout(m_inputLayoutDBG.Get());

        m_debug->SetMode(m_debugMode);
        m_debug->SetAlpha(1.f);
        m_debug->SetWorld(world * XMMatrixTranslation(col0, row0, 0));
        m_debug->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        m_debug->SetWorld(world * XMMatrixTranslation(col3, row0, 0));
        m_debug->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        m_debug->SetWorld(world * XMMatrixTranslation(col1, row0, 0));
        m_debug->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        m_debug->SetAlpha(alphaFade);
        m_debug->SetWorld(world * XMMatrixTranslation(col2, row0, 0));
        m_debug->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        context->IASetVertexBuffers(0, 1, m_vertexBufferCube.GetAddressOf(), &vertexStride, &vertexOffset);
        context->IASetIndexBuffer(m_indexBufferCube.Get(), DXGI_FORMAT_R16_UINT, 0);
        context->IASetInputLayout(m_inputLayoutCube.Get());
        m_debug->SetAlpha(1.f);
        m_debug->SetWorld(world * XMMatrixTranslation(col7, row0, 0));
        m_debug->Apply(context);
        context->DrawIndexed(m_indexCountCube, 0, 0);

        context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);
        context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        context->IASetInputLayout(m_inputLayoutPBR.Get());

        m_debug->SetAlpha(1.f);
        m_debug->SetWorld(world * XMMatrixTranslation(col4, row0, 0));
        m_debug->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        m_debug->SetAlpha(alphaFade);
        m_debug->SetWorld(world * XMMatrixTranslation(col5, row0, 0));
        m_debug->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);
    }
    else
    {
        //--- NormalMap --------------------------------------------------------------------
        context->IASetInputLayout(m_inputLayoutNM.Get());

        m_normalMapEffect->SetWorld(world * XMMatrixTranslation(col0, row0, 0));
        m_normalMapEffect->SetTexture(m_baseColor[0].Get());
        m_normalMapEffect->SetNormalTexture(m_normalMap[0].Get());
        m_normalMapEffect->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        m_normalMapEffect->SetWorld(world * XMMatrixTranslation(col3, row0, 0));
        m_normalMapEffect->SetTexture(m_baseColor[1].Get());
        m_normalMapEffect->SetNormalTexture(m_normalMap[1].Get());
        m_normalMapEffect->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        //--- PBREffect (basic) ------------------------------------------------------------
        context->IASetInputLayout(m_inputLayoutPBR.Get());

        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        m_radianceIBL[m_ibl]->GetDesc(&desc);

        m_pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL[m_ibl].Get());
        m_pbrCube->SetIBLTextures(m_radianceIBL[m_ibl].Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL[m_ibl].Get());

        m_pbr->SetAlpha(1.f);
        m_pbr->SetWorld(world * XMMatrixTranslation(col1, row0, 0));
        m_pbr->SetSurfaceTextures(m_baseColor[0].Get(), m_normalMap[0].Get(), m_rma[0].Get());
        m_pbr->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        m_pbr->SetAlpha(alphaFade);
        m_pbr->SetWorld(world * XMMatrixTranslation(col2, row0, 0));
        m_pbr->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        context->IASetVertexBuffers(0, 1, m_vertexBufferCube.GetAddressOf(), &vertexStride, &vertexOffset);
        context->IASetIndexBuffer(m_indexBufferCube.Get(), DXGI_FORMAT_R16_UINT, 0);
        context->IASetInputLayout(m_inputLayoutCube.Get());
        m_pbrCube->SetWorld(world * XMMatrixTranslation(col7, row0, 0));
        m_pbrCube->Apply(context);
        context->DrawIndexed(m_indexCountCube, 0, 0);

        //--- PBREffect (emissive) ---------------------------------------------------------
        context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);
        context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        context->IASetInputLayout(m_inputLayoutPBR.Get());

        m_pbr->SetEmissiveTexture(m_emissiveMap[1].Get());

        m_pbr->SetAlpha(1.f);
        m_pbr->SetWorld(world * XMMatrixTranslation(col4, row0, 0));
        m_pbr->SetSurfaceTextures(m_baseColor[1].Get(), m_normalMap[1].Get(), m_rma[1].Get());
        m_pbr->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);

        m_pbr->SetAlpha(alphaFade);
        m_pbr->SetWorld(world * XMMatrixTranslation(col5, row0, 0));
        m_pbr->Apply(context);
        context->DrawIndexed(m_indexCount, 0, 0);
    }

    //--- PBREffect (constant) -------------------------------------------------------------
    context->IASetInputLayout(m_inputLayoutPBR.Get());

    m_pbr->SetAlpha(1.f);
    m_pbr->SetSurfaceTextures(nullptr, nullptr, nullptr);
    m_pbr->SetEmissiveTexture(nullptr);
    m_pbr->SetWorld(world * XMMatrixTranslation(col0, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Blue);
    m_pbr->SetConstantMetallic(1.f);
    m_pbr->SetConstantRoughness(0.2f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col1, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Green);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col2, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Red);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col3, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Yellow);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col4, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Cyan);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col5, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Magenta);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col6, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::Black);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col7, row1, 0));
    m_pbr->SetConstantAlbedo(Colors::White);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Row 2
    m_pbr->SetAlpha(alphaFade);
    m_pbr->SetWorld(world * XMMatrixTranslation(col0, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Blue);
    m_pbr->SetConstantMetallic(0.f);
    m_pbr->SetConstantRoughness(0.2f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col1, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Green);
    m_pbr->SetConstantMetallic(0.25f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col2, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Red);
    m_pbr->SetConstantMetallic(0.5f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col3, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Yellow);
    m_pbr->SetConstantMetallic(0.75f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col4, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Cyan);
    m_pbr->SetConstantMetallic(1.f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col5, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Magenta);
    m_pbr->SetConstantMetallic(0.5f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col6, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::Black);
    m_pbr->SetConstantMetallic(0.75f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col7, row2, 0));
    m_pbr->SetConstantAlbedo(Colors::White);
    m_pbr->SetConstantMetallic(0.8f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Row3
    m_pbr->SetAlpha(1.f);
    m_pbr->SetWorld(world * XMMatrixTranslation(col0, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Blue);
    m_pbr->SetConstantMetallic(0.5f);
    m_pbr->SetConstantRoughness(0.0f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col1, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Green);
    m_pbr->SetConstantRoughness(0.25f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col2, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Red);
    m_pbr->SetConstantRoughness(0.5f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col3, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Yellow);
    m_pbr->SetConstantRoughness(0.75f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col4, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Cyan);
    m_pbr->SetConstantRoughness(1.0f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col5, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Magenta);
    m_pbr->SetConstantRoughness(0.2f);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col6, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::Black);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    m_pbr->SetWorld(world * XMMatrixTranslation(col7, row3, 0));
    m_pbr->SetConstantAlbedo(Colors::White);
    m_pbr->Apply(context);
    context->DrawIndexed(m_indexCount, 0, 0);

    // Render HUD
    m_batch->Begin();

    const wchar_t* info = L"";

#ifdef XBOX
    switch (m_toneMapMode)
    {
    case ToneMapPostProcess::Saturate: info = (g_HDRMode) ? L"HDR10 (GameDVR: None)" : L"None"; break;
    case ToneMapPostProcess::Reinhard: info = (g_HDRMode) ? L"HDR10 (GameDVR: Reinhard)" : L"Reinhard"; break;
    case ToneMapPostProcess::ACESFilmic: info = (g_HDRMode) ? L"HDR10 (GameDVR: ACES Filmic)" : L"ACES Filmic"; break;
    }
#else
    switch (m_deviceResources->GetColorSpace())
    {
    default:
        switch (m_toneMapMode)
        {
        case ToneMapPostProcess::Saturate: info = L"None"; break;
        case ToneMapPostProcess::Reinhard: info = L"Reinhard"; break;
        case ToneMapPostProcess::ACESFilmic: info = L"ACES Filmic"; break;
        }
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        info = L"HDR10";
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
        info = L"Linear";
        break;
    }
#endif

    long h = safeRect.bottom - safeRect.top;

    m_font->DrawString(m_batch.get(), info, XMFLOAT2(float(safeRect.right - (safeRect.right / 4)), float(safeRect.bottom - (h / 16))), c_BrightYellow);

    m_batch->End();

    // Tonemap the frame.
#ifdef XBOX
    m_hdrScene->EndScene(context);
#endif

#ifdef XBOX
    ID3D11RenderTargetView* renderTargets[2] = { m_deviceResources->GetRenderTargetView(), m_deviceResources->GetGameDVRRenderTargetView() };
    context->OMSetRenderTargets(2, renderTargets, nullptr);

    m_toneMap->SetOperator(static_cast<ToneMapPostProcess::Operator>(m_toneMapMode));
#else
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    switch (m_deviceResources->GetColorSpace())
    {
    default:
        m_toneMap->SetOperator(static_cast<ToneMapPostProcess::Operator>(m_toneMapMode));
        m_toneMap->SetTransferFunction((m_deviceResources->GetBackBufferFormat() == DXGI_FORMAT_R16G16B16A16_FLOAT) ? ToneMapPostProcess::Linear : ToneMapPostProcess::SRGB);
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        m_toneMap->SetOperator(ToneMapPostProcess::None);
        m_toneMap->SetTransferFunction(ToneMapPostProcess::ST2084);
        break;

    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
        m_toneMap->SetOperator(ToneMapPostProcess::None);
        m_toneMap->SetTransferFunction(ToneMapPostProcess::Linear);
        break;
    }
#endif

    m_toneMap->Process(context);

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);

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
    auto renderTarget = m_hdrScene->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, c_clearColor);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    const auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

void Game::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Game::OnResuming()
{
    m_deviceResources->Resume();

    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

#ifdef PC
void Game::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

#if defined(PC) || defined(UWP)
void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
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

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_batch = std::make_unique<SpriteBatch>(context);

    m_font = std::make_unique<SpriteFont>(device, L"comic.spritefont");

    m_states = std::make_unique<CommonStates>(device);

    m_hdrScene->SetDevice(device);

    m_normalMapEffect = std::make_unique<NormalMapEffect>(device);
    m_normalMapEffect->EnableDefaultLighting();

    m_pbr = std::make_unique<PBREffect>(device);
    m_pbr->EnableDefaultLighting();

    m_pbrCube = std::make_unique<PBREffect>(device);
    m_pbrCube->EnableDefaultLighting();

    m_debug = std::make_unique<DebugEffect>(device);

    m_toneMap = std::make_unique<ToneMapPostProcess>(device);
    m_toneMap->SetOperator(static_cast<ToneMapPostProcess::Operator>(m_toneMapMode));
    m_toneMap->SetTransferFunction(ToneMapPostProcess::SRGB);

#ifdef XBOX
    m_toneMap->SetMRTOutput(true);
#endif

    // Create test geometry
    {
        GeometricPrimitive::VertexCollection vertices;
        GeometricPrimitive::IndexCollection indices;

        GeometricPrimitive::CreateSphere(vertices, indices);

        // Create the D3D buffers.
        if (vertices.size() >= USHRT_MAX)
            throw std::out_of_range("Too many vertices for 16-bit index buffer");

        DX::ThrowIfFailed(
            CreateInputLayoutFromEffect<GeometricPrimitive::VertexType>(device, m_normalMapEffect.get(), m_inputLayoutNM.ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateInputLayoutFromEffect<GeometricPrimitive::VertexType>(device, m_pbr.get(), m_inputLayoutPBR.ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateInputLayoutFromEffect<GeometricPrimitive::VertexType>(device, m_debug.get(), m_inputLayoutDBG.ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, m_indexBuffer.ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, vertices, D3D11_BIND_VERTEX_BUFFER, m_vertexBuffer.ReleaseAndGetAddressOf())
        );

        // Record index count for draw
        m_indexCount = static_cast<UINT>(indices.size());
    }

    // Create cube geometry
    {
        GeometricPrimitive::VertexCollection vertices;
        GeometricPrimitive::IndexCollection indices;

        ReadVBO(L"BrokenCube.vbo", vertices, indices);

        // Create the D3D buffers.
        if (vertices.size() >= USHRT_MAX)
            throw std::out_of_range("Too many vertices for 16-bit index buffer");

        DX::ThrowIfFailed(
            CreateInputLayoutFromEffect<GeometricPrimitive::VertexType>(device, m_pbrCube.get(), m_inputLayoutCube.ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, indices, D3D11_BIND_INDEX_BUFFER, m_indexBufferCube.ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateStaticBuffer(device, vertices, D3D11_BIND_VERTEX_BUFFER, m_vertexBufferCube.ReleaseAndGetAddressOf())
        );

        // Record index count for draw
        m_indexCountCube = static_cast<UINT>(indices.size());
    }

    // Load textures
    static const wchar_t* s_albedoTextures[s_nMaterials] =
    {
        L"SphereMat_baseColor.png",
        L"Sphere2Mat_baseColor.png",
        L"BrokenCube_baseColor.png",
    };
    static const wchar_t* s_normalMapTextures[s_nMaterials] =
    {
        L"SphereMat_normal.png",
        L"Sphere2Mat_normal.png",
        L"BrokenCube_normal.png",
    };
    static const wchar_t* s_rmaTextures[s_nMaterials] =
    {
        L"SphereMat_occlusionRoughnessMetallic.png",
        L"Sphere2Mat_occlusionRoughnessMetallic.png",
        L"BrokenCube_occlusionRoughnessMetallic.png",
    };
    static const wchar_t* s_emissiveTextures[s_nMaterials] =
    {
        nullptr,
        L"Sphere2Mat_emissive.png",
        L"BrokenCube_emissive.png",
    };

    static_assert(std::size(s_albedoTextures) == std::size(s_normalMapTextures), "Material array mismatch");
    static_assert(std::size(s_albedoTextures) == std::size(s_rmaTextures), "Material array mismatch");
    static_assert(std::size(s_albedoTextures) == std::size(s_emissiveTextures), "Material array mismatch");

    for (size_t j = 0; j < s_nMaterials; ++j)
    {
        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, s_albedoTextures[j], nullptr, m_baseColor[j].ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, s_normalMapTextures[j], nullptr, m_normalMap[j].ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateWICTextureFromFile(device, s_rmaTextures[j], nullptr, m_rma[j].ReleaseAndGetAddressOf())
        );

        if (s_emissiveTextures[j])
        {
            DX::ThrowIfFailed(
                CreateWICTextureFromFile(device, s_emissiveTextures[j], nullptr, m_emissiveMap[j].ReleaseAndGetAddressOf())
            );
        }
    }

    m_pbrCube->SetSurfaceTextures(m_baseColor[2].Get(), m_normalMap[2].Get(), m_rma[2].Get());
    m_pbrCube->SetEmissiveTexture(m_emissiveMap[2].Get());

    static const wchar_t* s_radianceIBL[s_nIBL] =
    {
        L"Atrium_diffuseIBL.dds",
        L"Garage_diffuseIBL.dds",
        L"SunSubMixer_diffuseIBL.dds",
    };
    static const wchar_t* s_irradianceIBL[s_nIBL] =
    {
        L"Atrium_specularIBL.dds",
        L"Garage_specularIBL.dds",
        L"SunSubMixer_specularIBL.dds",
    };

    static_assert(std::size(s_radianceIBL) == std::size(s_irradianceIBL), "IBL array mismatch");

    for (size_t j = 0; j < s_nIBL; ++j)
    {
        DX::ThrowIfFailed(
            CreateDDSTextureFromFile(device, s_radianceIBL[j], nullptr, m_radianceIBL[j].ReleaseAndGetAddressOf())
        );

        DX::ThrowIfFailed(
            CreateDDSTextureFromFile(device, s_irradianceIBL[j], nullptr, m_irradianceIBL[j].ReleaseAndGetAddressOf())
        );
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { { { 0.f, 0.f, 6.f, 0.f } } };

    const auto size = m_deviceResources->GetOutputSize();
    float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    XMMATRIX view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    XMMATRIX projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 15);
#else
    XMMATRIX view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    XMMATRIX projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 15);
#endif

#ifdef UWP
    {
        auto orient3d = m_deviceResources->GetOrientationTransform3D();
        XMMATRIX orient = XMLoadFloat4x4(&orient3d);
        projection *= orient;
    }
#endif

    m_normalMapEffect->SetView(view);
    m_pbr->SetView(view);
    m_pbrCube->SetView(view);
    m_debug->SetView(view);

    m_normalMapEffect->SetProjection(projection);
    m_pbr->SetProjection(projection);
    m_pbrCube->SetProjection(projection);
    m_debug->SetProjection(projection);

    // Set windows size for HDR.
    m_hdrScene->SetWindow(size);

    m_toneMap->SetHDRSourceTexture(m_hdrScene->GetShaderResourceView());
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_batch.reset();
    m_font.reset();

    m_states.reset();

    for (size_t j = 0; j < s_nMaterials; ++j)
    {
        m_baseColor[j].Reset();
        m_normalMap[j].Reset();
        m_rma[j].Reset();
        m_emissiveMap[j].Reset();
    }

    for (size_t j = 0; j < s_nIBL; ++j)
    {
        m_radianceIBL[j].Reset();
        m_irradianceIBL[j].Reset();
    }

    m_normalMapEffect.reset();
    m_pbr.reset();
    m_pbrCube.reset();
    m_debug.reset();

    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
    m_inputLayoutNM.Reset();
    m_inputLayoutPBR.Reset();
    m_inputLayoutDBG.Reset();

    m_vertexBufferCube.Reset();
    m_indexBufferCube.Reset();
    m_inputLayoutCube.Reset();

    m_toneMap.reset();

    m_hdrScene->ReleaseDevice();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion

void Game::CycleDebug()
{
    if (!m_showDebug)
    {
        m_showDebug = true;
        m_debugMode = DebugEffect::Mode_Default;
    }
    else if (m_debugMode == DebugEffect::Mode_BiTangents)
    {
        m_showDebug = false;
    }
    else
    {
        m_debugMode = static_cast<DebugEffect::Mode>(m_debugMode + 1);

        switch (m_debugMode)
        {
        case DebugEffect::Mode_Normals: OutputDebugStringA("INFO: Showing normals\n"); break;
        case DebugEffect::Mode_Tangents: OutputDebugStringA("INFO: Showing tangents\n"); break;
        case DebugEffect::Mode_BiTangents: OutputDebugStringA("INFO: Showing bi-tangents\n"); break;
        case DebugEffect::Mode_Default: break;
        }
    }
}

void Game::CycleToneMapOperator()
{
#ifndef XBOX
    if (m_deviceResources->GetColorSpace() != DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
        return;
#endif

    m_toneMapMode += 1;

    if (m_toneMapMode >= static_cast<int>(ToneMapPostProcess::Operator_Max))
    {
        m_toneMapMode = ToneMapPostProcess::Saturate;
    }
}
