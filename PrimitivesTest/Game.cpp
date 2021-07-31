//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK Geometric Primitives
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
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float rowtop = 4.f;
    constexpr float row0 = 2.7f;
    constexpr float row1 = 1.f;
    constexpr float row2 = -0.7f;
    constexpr float row3 = -2.5f;

    constexpr float col0 = -7.5f;
    constexpr float col1 = -5.75f;
    constexpr float col2 = -4.25f;
    constexpr float col3 = -2.7f;
    constexpr float col4 = -1.25f;
    constexpr float col5 = 0.f;
    constexpr float col6 = 1.25f;
    constexpr float col7 = 2.5f;
    constexpr float col8 = 4.25f;
    constexpr float col9 = 5.75f;
    constexpr float col10 = 7.5f;
}

Game::Game() noexcept(false) :
    m_instanceCount(0),
    m_spinning(true),
    m_pitch(0),
    m_yaw(0)
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
        c_RenderFormat, DXGI_FORMAT_D24_UNORM_S8_UINT, 2, D3D_FEATURE_LEVEL_9_3,
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

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_spinning = !m_spinning;
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

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
    {
        m_spinning = !m_spinning;
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

    XMMATRIX world;
    XMVECTOR quat;

    if (m_spinning)
    {
        world = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
        quat = XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
    }
    else
    {
        world = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0);
        quat = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, roll);
    }

    XMVECTORF32 red, green, blue, yellow, cyan, magenta, cornflower, lime, gray;
#ifdef GAMMA_CORRECT_RENDERING
    red.v = XMColorSRGBToRGB(Colors::Red);
    green.v = XMColorSRGBToRGB(Colors::Green);
    blue.v = XMColorSRGBToRGB(Colors::Blue);
    yellow.v = XMColorSRGBToRGB(Colors::Yellow);
    cyan.v = XMColorSRGBToRGB(Colors::Cyan);
    magenta.v = XMColorSRGBToRGB(Colors::Magenta);
    cornflower.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
    lime.v = XMColorSRGBToRGB(Colors::Lime);
    gray.v = XMColorSRGBToRGB(Colors::Gray);
#else
    red.v = Colors::Red;
    green.v = Colors::Green;
    blue.v = Colors::Blue;
    yellow.v = Colors::Yellow;
    cyan.v = Colors::Cyan;
    magenta.v = Colors::Magenta;
    cornflower.v = Colors::CornflowerBlue;
    lime.v = Colors::Lime;
    gray.v = Colors::Gray;
#endif
    SimpleMath::Vector4 white = Colors::White.v;

    //--- Draw shapes ----------------------------------------------------------------------
    m_cube->Draw(world * XMMatrixTranslation(col0, row0, 0), m_view, m_projection);
    m_sphere->Draw(world * XMMatrixTranslation(col1, row0, 0), m_view, m_projection, red);
    m_geosphere->Draw(world * XMMatrixTranslation(col2, row0, 0), m_view, m_projection, green);
    m_cylinder->Draw(world * XMMatrixTranslation(col3, row0, 0), m_view, m_projection, lime);
    m_cone->Draw(world * XMMatrixTranslation(col4, row0, 0), m_view, m_projection, yellow);
    m_torus->Draw(world * XMMatrixTranslation(col5, row0, 0), m_view, m_projection, blue);
    m_teapot->Draw(world * XMMatrixTranslation(col6, row0, 0), m_view, m_projection, cornflower);
    m_tetra->Draw(world * XMMatrixTranslation(col7, row0, 0), m_view, m_projection, red);
    m_octa->Draw(world * XMMatrixTranslation(col8, row0, 0), m_view, m_projection, lime);
    m_dodec->Draw(world * XMMatrixTranslation(col9, row0, 0), m_view, m_projection, blue);
    m_iso->Draw(world * XMMatrixTranslation(col10, row0, 0), m_view, m_projection, cyan);
    m_box->Draw(world * XMMatrixTranslation(col8, row3, 0), m_view, m_projection, magenta);

    //--- Draw textured shapes -------------------------------------------------------------
    m_cube->Draw(world * XMMatrixTranslation(col0, row1, 0), m_view, m_projection, white, m_reftxt.Get());
    m_sphere->Draw(world * XMMatrixTranslation(col1, row1, 0), m_view, m_projection, red, m_reftxt.Get());
    m_geosphere->Draw(world * XMMatrixTranslation(col2, row1, 0), m_view, m_projection, green, m_reftxt.Get());
    m_cylinder->Draw(world * XMMatrixTranslation(col3, row1, 0), m_view, m_projection, lime, m_reftxt.Get());
    m_cone->Draw(world * XMMatrixTranslation(col4, row1, 0), m_view, m_projection, yellow, m_reftxt.Get());
    m_torus->Draw(world * XMMatrixTranslation(col5, row1, 0), m_view, m_projection, blue, m_reftxt.Get());
    m_teapot->Draw(world * XMMatrixTranslation(col6, row1, 0), m_view, m_projection, cornflower, m_reftxt.Get());
    m_tetra->Draw(world * XMMatrixTranslation(col7, row1, 0), m_view, m_projection, red, m_reftxt.Get());
    m_octa->Draw(world * XMMatrixTranslation(col8, row1, 0), m_view, m_projection, lime, m_reftxt.Get());
    m_dodec->Draw(world * XMMatrixTranslation(col9, row1, 0), m_view, m_projection, blue, m_reftxt.Get());
    m_iso->Draw(world * XMMatrixTranslation(col10, row1, 0), m_view, m_projection, cyan, m_reftxt.Get());
    m_box->Draw(world * XMMatrixTranslation(col9, row3, 0), m_view, m_projection, magenta, m_reftxt.Get());
    m_customBox->Draw(world * XMMatrixTranslation(col7, row3, 0), m_view, m_projection, white, m_reftxt.Get());

    //--- Draw shapes in wireframe ---------------------------------------------------------
    m_cube->Draw(world * XMMatrixTranslation(col0, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_sphere->Draw(world * XMMatrixTranslation(col1, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_geosphere->Draw(world * XMMatrixTranslation(col2, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_cylinder->Draw(world * XMMatrixTranslation(col3, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_cone->Draw(world * XMMatrixTranslation(col4, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_torus->Draw(world * XMMatrixTranslation(col5, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_teapot->Draw(world * XMMatrixTranslation(col6, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_tetra->Draw(world * XMMatrixTranslation(col7, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_octa->Draw(world * XMMatrixTranslation(col8, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_dodec->Draw(world * XMMatrixTranslation(col9, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_iso->Draw(world * XMMatrixTranslation(col10, row2, 0), m_view, m_projection, gray, nullptr, true);
    m_box->Draw(world * XMMatrixTranslation(col10, row3, 0), m_view, m_projection, gray, nullptr, true);

    //--- Draw shapes with alpha blending --------------------------------------------------
    m_cube->Draw(world * XMMatrixTranslation(col0, row3, 0), m_view, m_projection, white * alphaFade);
    m_cube->Draw(world * XMMatrixTranslation(col1, row3, 0), m_view, m_projection, white * alphaFade, m_cat.Get());

    //--- Draw shapes with custom device states --------------------------------------------
    m_cube->Draw(world * XMMatrixTranslation(col2, row3, 0), m_view, m_projection, white * alphaFade, m_cat.Get(), false, [&]()
    {
        context->OMSetBlendState(m_states->NonPremultiplied(), Colors::White, 0xFFFFFFFF);
    });

    //--- Draw shapes with custom effects --------------------------------------------------
    XMVECTOR dir = XMVector3Rotate(g_XMOne, quat);
    m_customEffect->EnableDefaultLighting();
    m_customEffect->SetFogEnabled(false);
    m_customEffect->SetLightDirection(0, dir);
    m_customEffect->SetWorld(XMMatrixTranslation(col3, row3, 0));
    m_cube->Draw(m_customEffect.get(), m_customIL.Get());

    m_customEffect->SetFogEnabled(true);
#ifdef LH_COORDS
    m_customEffect->SetFogStart(-6);
    m_customEffect->SetFogEnd(-8);
#else
    m_customEffect->SetFogStart(6);
    m_customEffect->SetFogEnd(8);
#endif
    m_customEffect->SetFogColor(cornflower);

    XMMATRIX fbworld = XMMatrixTranslation(0, 0, cos(time) * 2.f);
    m_customEffect->SetWorld(fbworld  * XMMatrixTranslation(col5, row3, 0));

    m_cube->Draw(m_customEffect.get(), m_customIL.Get());

    //--- Draw shapes using custom effects with instancing ---------------------------------
    m_instancedEffect->SetFogEnabled(true);
#ifdef LH_COORDS
    m_instancedEffect->SetFogStart(-6);
    m_instancedEffect->SetFogEnd(-8);
#else
    m_instancedEffect->SetFogStart(9);
    m_instancedEffect->SetFogEnd(10);
#endif
    m_instancedEffect->SetFogColor(cornflower);

    {
        {
            size_t j = 0;
            for (float x = -8.f; x <= 8.f; x += 3.f)
            {
                XMMATRIX m = world * XMMatrixTranslation(x, 0.f, cos(time + float(j) * XM_PIDIV4));
                XMStoreFloat3x4(&m_instanceTransforms[j], m);
                ++j;
            }

            assert(j == m_instanceCount);

            MapGuard map(context, m_instancedVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0);
            memcpy(map.pData, m_instanceTransforms.get(), j * sizeof(XMFLOAT3X4));
        }

        UINT stride = sizeof(XMFLOAT3X4);
        UINT offset = 0;
        context->IASetVertexBuffers(1, 1, m_instancedVB.GetAddressOf(), &stride, &offset);

        m_instancedEffect->SetWorld(XMMatrixTranslation(0.f, rowtop, 0.f));
        m_teapot->DrawInstanced(m_instancedEffect.get(), m_instancedIL.Get(), m_instanceCount);
    }

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
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
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
    width = 1600;
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

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"cat.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_cat.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"dx5_logo.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_dxlogo.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"reftexture.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_reftxt.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"normalMap.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, false,
        nullptr, m_normalMap.ReleaseAndGetAddressOf()));

#ifdef LH_COORDS
    bool rhcoords = false;
#else
    bool rhcoords = true;
#endif

    auto context = m_deviceResources->GetD3DDeviceContext();

    m_cube = GeometricPrimitive::CreateCube(context, 1.f, rhcoords);
    m_box = GeometricPrimitive::CreateBox(context, XMFLOAT3(1.f / 2.f, 2.f / 2.f, 3.f / 2.f), rhcoords);
    m_sphere = GeometricPrimitive::CreateSphere(context, 1.f, 16, rhcoords);
    m_geosphere = GeometricPrimitive::CreateGeoSphere(context, 1.f, 3, rhcoords);
    m_cylinder = GeometricPrimitive::CreateCylinder(context, 1.f, 1.f, 32, rhcoords);
    m_cone = GeometricPrimitive::CreateCone(context, 1.f, 1.f, 32, rhcoords);
    m_torus = GeometricPrimitive::CreateTorus(context, 1.f, 0.333f, 32, rhcoords);
    m_teapot = GeometricPrimitive::CreateTeapot(context, 1.f, 8, rhcoords);
    m_tetra = GeometricPrimitive::CreateTetrahedron(context, 0.75f, rhcoords);
    m_octa = GeometricPrimitive::CreateOctahedron(context, 0.75f, rhcoords);
    m_dodec = GeometricPrimitive::CreateDodecahedron(context, 0.5f, rhcoords);
    m_iso = GeometricPrimitive::CreateIcosahedron(context, 0.5f, rhcoords);

    {
        std::vector<GeometricPrimitive::VertexType> customVerts;
        std::vector<uint16_t> customIndices;
        GeometricPrimitive::CreateBox(customVerts, customIndices, XMFLOAT3(1.f / 2.f, 2.f / 2.f, 3.f / 2.f), rhcoords);

        assert(customVerts.size() == 24);
        assert(customIndices.size() == 36);

        for (auto it = customVerts.begin(); it != customVerts.end(); ++it)
        {
            it->textureCoordinate.x *= 5.f;
            it->textureCoordinate.y *= 5.f;
        }

        m_customBox = GeometricPrimitive::CreateCustom(context, customVerts, customIndices);
    }

    {
        // Ensure VertexType alias is consistent with alternative client usage
        std::vector<VertexPositionNormalTexture> customVerts;
        std::vector<uint16_t> customIndices;
        GeometricPrimitive::CreateBox(customVerts, customIndices, XMFLOAT3(1.f / 2.f, 2.f / 2.f, 3.f / 2.f), rhcoords);

        assert(customVerts.size() == 24);
        assert(customIndices.size() == 36);

        m_customBox2 = GeometricPrimitive::CreateCustom(context, customVerts, customIndices);
    }

    m_customEffect = std::make_unique<BasicEffect>(device);
    m_customEffect->EnableDefaultLighting();
    m_customEffect->SetTextureEnabled(true);
    m_customEffect->SetTexture(m_dxlogo.Get());
    m_customEffect->SetDiffuseColor(g_XMOne);

    m_cube->CreateInputLayout(m_customEffect.get(), m_customIL.ReleaseAndGetAddressOf());

    m_instancedEffect = std::make_unique <NormalMapEffect>(device);
    m_instancedEffect->EnableDefaultLighting();
    m_instancedEffect->SetTexture(m_dxlogo.Get());
    m_instancedEffect->SetNormalTexture(m_normalMap.Get());
    m_instancedEffect->SetInstancingEnabled(true);

    {
        static const D3D11_INPUT_ELEMENT_DESC s_InputElements[] =
        {
            // GeometricPrimitive::VertexType
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
            // XMFLOAT3X4
            { "InstMatrix",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "InstMatrix",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "InstMatrix",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

        CreateInputLayoutFromEffect(device, m_instancedEffect.get(),
            s_InputElements, std::size(s_InputElements),
            m_instancedIL.ReleaseAndGetAddressOf());

        // Create instance transforms.
        {
            size_t j = 0;
            for (float x = -8.f; x <= 8.f; x += 3.f)
            {
                ++j;
            }
            m_instanceCount = static_cast<UINT>(j);

            m_instanceTransforms = std::make_unique<XMFLOAT3X4[]>(j);

            constexpr XMFLOAT3X4 s_identity = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f };

            j = 0;
            for (float x = -8.f; x <= 8.f; x += 3.f)
            {
                m_instanceTransforms[j] = s_identity;
                m_instanceTransforms[j]._14 = x;
                ++j;
            }

            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = static_cast<UINT>(j * sizeof(XMFLOAT3X4));
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            D3D11_SUBRESOURCE_DATA initData = { m_instanceTransforms.get(), 0, 0 };

            DX::ThrowIfFailed(
                device->CreateBuffer(&desc, &initData, m_instancedVB.ReleaseAndGetAddressOf())
            );
        }
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { { { 0.f, 0.f, 9.f, 0.f } } };

    auto size = m_deviceResources->GetOutputSize();
    float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    m_view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 10);
#else
    m_view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 10);
#endif

#ifdef UWP
    {
        auto orient3d = m_deviceResources->GetOrientationTransform3D();
        XMMATRIX orient = XMLoadFloat4x4(&orient3d);
        m_projection *= orient;
    }
#endif

    m_customEffect->SetView(m_view);
    m_customEffect->SetProjection(m_projection);

    m_instancedEffect->SetView(m_view);
    m_instancedEffect->SetProjection(m_projection);
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_states.reset();

    m_cube.reset();
    m_box.reset();
    m_sphere.reset();
    m_geosphere.reset();
    m_cylinder.reset();
    m_cone.reset();
    m_torus.reset();
    m_teapot.reset();
    m_tetra.reset();
    m_octa.reset();
    m_dodec.reset();
    m_iso.reset();
    m_customBox.reset();
    m_customBox2.reset();

    m_customEffect.reset();
    m_instancedEffect.reset();

    m_customIL.Reset();
    m_instancedIL.Reset();

    m_cat.Reset();
    m_dxlogo.Reset();
    m_reftxt.Reset();
    m_normalMap.Reset();

    m_instancedVB.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
