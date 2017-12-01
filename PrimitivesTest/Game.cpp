//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK Geometric Primitives
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
#include "Game.h"

#pragma warning(disable : 4238)

#define GAMMA_CORRECT_RENDERING
#define USE_FAST_SEMANTICS

// Build for LH vs. RH coords
//#define LH_COORDS

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const float row0 = 2.7f;
    const float row1 = 1.f;
    const float row2 = -0.7f;
    const float row3 = -2.5f;

    const float col0 = -7.5f;
    const float col1 = -5.75f;
    const float col2 = -4.25f;
    const float col3 = -2.7f;
    const float col4 = -1.25f;
    const float col5 = 0.f;
    const float col6 = 1.25f;
    const float col7 = 2.5f;
    const float col8 = 4.25f;
    const float col9 = 5.75f;
    const float col10 = 7.5f;
}

Game::Game() :
    m_spinning(true),
    m_pitch(0),
    m_yaw(0)
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(GAMMA_CORRECT_RENDERING)
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>();
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    m_deviceResources->RegisterDeviceNotify(this);
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
    HWND window,
#else
    IUnknown* window,
#endif
    int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();

#if defined(_XBOX_ONE) && defined(_TITLE)
    UNREFERENCED_PARAMETER(rotation);
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    m_deviceResources->SetWindow(window);
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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

#if defined(_XBOX_ONE) && defined(_TITLE)
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
    m_cube->Draw(world * XMMatrixTranslation(col0, row1, 0), m_view, m_projection, Colors::White, m_reftxt.Get());
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
    m_customBox->Draw(world * XMMatrixTranslation(col7, row3, 0), m_view, m_projection, Colors::White, m_reftxt.Get());

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
    m_cube->Draw(world * XMMatrixTranslation(col0, row3, 0), m_view, m_projection, Colors::White * alphaFade);
    m_cube->Draw(world * XMMatrixTranslation(col1, row3, 0), m_view, m_projection, Colors::White * alphaFade, m_cat.Get());

    //--- Draw shapes with custom device states --------------------------------------------
    m_cube->Draw(world * XMMatrixTranslation(col2, row3, 0), m_view, m_projection, Colors::White * alphaFade, m_cat.Get(), false, [&]()
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

    // Show the new frame.
    m_deviceResources->Present();

#if defined(_XBOX_ONE) && defined(_TITLE)
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
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
}

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnWindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation)
{
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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

#if defined(_XBOX_ONE) && defined(_TITLE)
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
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { 0, 0, 9 };

    auto size = m_deviceResources->GetOutputSize();
    float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    m_view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 10);
#else
    m_view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 10);
#endif

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    XMMATRIX orient = XMLoadFloat4x4(&m_deviceResources->GetOrientationTransform3D());
    m_projection *= orient;
#endif

    m_customEffect->SetView(m_view);
    m_customEffect->SetProjection(m_projection);
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
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

    m_customIL.Reset();
    m_cat.Reset();
    m_dxlogo.Reset();
    m_reftxt.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
