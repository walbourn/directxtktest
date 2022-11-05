//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK DGSLEffect support
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"

//#define GAMMA_CORRECT_RENDERING
#define USE_FAST_SEMANTICS

// Build for LH vs. RH coords
//#define LH_COORDS

// Build FL 10.0 vs. 9.1
//#define FEATURE_LEVEL_9_X

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float row0 = 2.f;
    constexpr float row1 = 0.f;
    constexpr float row2 = -2.f;
}

//--------------------------------------------------------------------------------------

static_assert(std::is_nothrow_move_constructible<DGSLEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DGSLEffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<SkinnedDGSLEffect>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<SkinnedDGSLEffect>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<DGSLEffectFactory>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<DGSLEffectFactory>::value, "Move Assign.");

// VS 2017 on XDK incorrectly thinks it's not noexcept
static_assert(std::is_nothrow_move_constructible<DGSLEffectFactory::DGSLEffectInfo>::value, "Move Ctor.");
static_assert(std::is_move_assignable<DGSLEffectFactory::DGSLEffectInfo>::value, "Move Assign.");

//--------------------------------------------------------------------------------------

Game::Game() noexcept(false)
{
#ifdef GAMMA_CORRECT_RENDERING
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
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
#ifdef FEATURE_LEVEL_9_X
        c_RenderFormat, DXGI_FORMAT_D24_UNORM_S8_UINT, 2, D3D_FEATURE_LEVEL_9_3,
#else
        c_RenderFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0,
#endif
        DX::DeviceResources::c_Enable4K_Xbox | DX::DeviceResources::c_EnableQHD_Xbox
        );
#elif defined(FEATURE_LEVEL_9_X)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D24_UNORM_S8_UINT, 2, D3D_FEATURE_LEVEL_9_3
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D32_FLOAT, 2, D3D_FEATURE_LEVEL_10_0
        );
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

    m_bones = ModelBone::MakeArray(SkinnedDGSLEffect::MaxBones);
    XMMATRIX id = XMMatrixIdentity();
    for (size_t j = 0; j < SkinnedDGSLEffect::MaxBones; ++j)
    {
        m_bones[j] = id;
    }
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

    // Skinning settings
    float s = 1 + sin(time * 1.7f) * 0.5f;
    XMMATRIX scale = XMMatrixScaling(s, s, s);

    for (size_t j = 0; j < SkinnedDGSLEffect::MaxBones; ++j)
    {
        m_bones[j] = scale;
    }

    //--- Draw CMO models ------------------------------------------------------------------
    XMMATRIX local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(2.f, row0, 0.f));
    local = XMMatrixMultiply(world, local);
    m_teapotUnlit->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(2.f, row1, 0.f));
    local = XMMatrixMultiply(world, local);
    m_teapotLambert->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(2.f, row2, 0.f));
    local = XMMatrixMultiply(world, local);
    m_teapotPhong->Draw(context, *m_states, local, m_view, m_projection);

    // Effect Settings
    m_teapotTest->UpdateEffects([&](IEffect* effect)
    {
        auto dgsl = reinterpret_cast<DGSLEffect*>(effect);
#ifdef GAMMA_CORRECT_RENDERING
        XMVECTORF32 color;
        color.v = XMColorSRGBToRGB(Colors::Gray);
        dgsl->SetDiffuseColor(color);
#else
        dgsl->SetDiffuseColor(Colors::Gray);
#endif
        dgsl->DisableSpecular();
        dgsl->SetLightEnabled(0, true);
        dgsl->SetLightEnabled(1, true);
        dgsl->SetLightEnabled(2, true);
    });

    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(4.f, row0, 0.f));
    local = XMMatrixMultiply(world, local);
    m_teapotTest->Draw(context, *m_states, local, m_view, m_projection);

    m_teapotTest->UpdateEffects([&](IEffect* effect)
    {
        auto dgsl = reinterpret_cast<DGSLEffect*>(effect);
#ifdef GAMMA_CORRECT_RENDERING
        XMVECTORF32 color;
        color.v = XMColorSRGBToRGB(Colors::Red);
        dgsl->SetDiffuseColor(color);
        color.v = XMColorSRGBToRGB(Colors::White);
        dgsl->SetSpecularColor(color);
#else
        dgsl->SetDiffuseColor(Colors::Red);
        dgsl->SetSpecularColor(Colors::White);
#endif
        dgsl->SetSpecularPower(16);
        dgsl->SetLightEnabled(0, true);
        dgsl->SetLightEnabled(1, false);
        dgsl->SetLightEnabled(2, false);
    });

    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(4.f, row1, 0.f));
    local = XMMatrixMultiply(world, local);
    m_teapotTest->Draw(context, *m_states, local, m_view, m_projection);

    m_teapotTest->UpdateEffects([&](IEffect* effect)
    {
        auto dgsl = reinterpret_cast<DGSLEffect*>(effect);
#ifdef GAMMA_CORRECT_RENDERING
        XMVECTORF32 color;
        color.v = XMColorSRGBToRGB(Colors::Green);
        dgsl->SetDiffuseColor(color);
#else
        dgsl->SetDiffuseColor(Colors::Green);
#endif
        dgsl->SetLightEnabled(0, false);
        dgsl->SetLightEnabled(1, true);
        dgsl->SetLightEnabled(2, true);
    });

    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(4.f, row2, 0.f));
    local = XMMatrixMultiply(world, local);
    m_teapotTest->Draw(context, *m_states, local, m_view, m_projection);

    // Skinned models
    m_teapot->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->ResetBoneTransforms();
    });
    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(-2.f, row1, 0.f));
    local = XMMatrixMultiply(world, local);
    m_teapot->Draw(context, *m_states, local, m_view, m_projection);

    m_teapot->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->SetBoneTransforms(m_bones.get(), IEffectSkinning::MaxBones);
    });
    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(0.f, row0, 0.f));
    m_teapot->Draw(context, *m_states, local, m_view, m_projection);

    // General CMO models
    local = XMMatrixMultiply(XMMatrixScaling(0.1f, 0.1f, 0.1f), XMMatrixTranslation(0.f, row1, 0.f));
    local = XMMatrixMultiply(world, local);
    m_gamelevel->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(.2f, .2f, .2f), XMMatrixTranslation(-2.f, row2, 0.f));
    local = XMMatrixMultiply(world, local);
    m_ship->Draw(context, *m_states, local, m_view, m_projection);

    //--- Draw SDKMESH models --------------------------------------------------------------
    m_soldier->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
        {
            skinnedEffect->SetBoneTransforms(m_bones.get(), IEffectSkinning::MaxBones);
        }
    });

    local = XMMatrixMultiply(XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(-2.5f, row0, 0.f));
    m_soldier->Draw(context, *m_states, local, m_view, m_projection);

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
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
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
    auto const r = m_deviceResources->GetOutputSize();
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

    m_states = std::make_unique<CommonStates>(device);

    m_fx = std::make_unique<DGSLEffectFactory>(device);

#ifdef GAMMA_CORRECT_RENDERING
    m_fx->EnableForceSRGB(true);
#endif
    
#ifdef LH_COORDS
    bool ccw = false;
#else
    bool ccw = true;
#endif

    // Visual Studio CMO
    ModelLoaderFlags flags = ccw ? ModelLoader_CounterClockwise : ModelLoader_Clockwise;

    m_teapotUnlit = Model::CreateFromCMO(device, L"teapot_unlit.cmo", *m_fx, flags);
    m_teapotLambert = Model::CreateFromCMO(device, L"teapot_lambert.cmo", *m_fx, flags);
    m_teapotPhong = Model::CreateFromCMO(device, L"teapot_phong.cmo", *m_fx, flags);
    m_teapotTest = Model::CreateFromCMO(device, L"teapot_phong.cmo", *m_fx, flags);

    m_teapot = Model::CreateFromCMO(device, L"teapot.cmo", *m_fx, flags);

    m_gamelevel = Model::CreateFromCMO(device, L"gamelevel.cmo", *m_fx, flags);

    m_ship = Model::CreateFromCMO(device, L"25ab10e8-621a-47d4-a63d-f65a00bc1549_model.cmo", *m_fx, flags);

    // DirectX SDK Mesh
    flags = ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise;

    m_soldier = Model::CreateFromSDKMESH(device, L"soldier.sdkmesh", *m_fx, flags);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { { { 0.f, 0.f, 6.f, 0.f } } };

    auto const size = m_deviceResources->GetOutputSize();
    float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    m_view = XMMatrixLookAtLH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovLH(1, aspect, 1, 15);
#else
    m_view = XMMatrixLookAtRH(cameraPosition, g_XMZero, XMVectorSet(0, 1, 0, 0));
    m_projection = XMMatrixPerspectiveFovRH(1, aspect, 1, 15);
#endif
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_states.reset();
    m_fx.reset();
    m_teapot.reset();
    m_teapotUnlit.reset();
    m_teapotLambert.reset();
    m_teapotPhong.reset();
    m_teapotTest.reset();
    m_gamelevel.reset();
    m_ship.reset();
    m_soldier.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
