//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK Model
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
#define NORMALMAPS

// Build for LH vs. RH coords
#define LH_COORDS

namespace
{
    constexpr float row0 = 2.f;
    constexpr float row1 = 0.f;
    constexpr float row2 = -2.f;
}

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

extern std::unique_ptr<Model> CreateModelFromOBJ(
    _In_ ID3D11Device* d3dDevice,
    _In_ ID3D11DeviceContext* context,
    _In_z_ const wchar_t* szFileName,
    _In_ IEffectFactory& fxFactory,
    bool enableInstacing,
    ModelLoaderFlags flags);


//--------------------------------------------------------------------------------------

// Note VS 2017 incorrectly returns 'false' for is_nothrow_move_constructible<Model>
static_assert(std::is_move_constructible<Model>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<Model>::value, "Move Assign.");

// VS 2017 on XDK incorrectly thinks it's not noexcept
static_assert(std::is_move_constructible<ModelMesh>::value, "Move Ctor.");
static_assert(std::is_move_assignable<ModelMesh>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<ModelMeshPart>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ModelMeshPart>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<ModelBone>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ModelBone>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<ModelBone::TransformArray>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<ModelBone::TransformArray>::value, "Move Assign.");

//--------------------------------------------------------------------------------------

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

    m_bones = ModelBone::MakeArray(IEffectSkinning::MaxBones);
    XMMATRIX id = XMMatrixIdentity();
    for (size_t j = 0; j < IEffectSkinning::MaxBones; ++j)
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

    auto time = static_cast<float>(m_timer.GetTotalSeconds());

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

    // Skinning settings
    float s = 1 + sin(time * 1.7f) * 0.5f;

    XMMATRIX scale = XMMatrixScaling(s, s, s);

    for (size_t j = 0; j < IEffectSkinning::MaxBones; ++j)
    {
        m_bones[j] = scale;
    }

    Clear();

    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

#ifdef LH_COORDS
    float fogstart = -5;
    float fogend = -8;
#else
    float fogstart = 5;
    float fogend = 8;
#endif

    //--- Draw Wavefront OBJ models --------------------------------------------------------
    XMMATRIX local = XMMatrixTranslation(1.5f, row0, 0.f);
    local = XMMatrixMultiply(world, local);
    m_cup->Draw(context, *m_states, local, m_view, m_projection);

        // Wireframe
    local = XMMatrixTranslation(3.f, row0, 0.f);
    local = XMMatrixMultiply(world, local);
    m_cup->Draw(context, *m_states, local, m_view, m_projection, true);

        // Custom settings
    local = XMMatrixTranslation(0.f, row0, 0.f);
    local = XMMatrixMultiply(world, local);
    m_cup->Draw(context, *m_states, local, m_view, m_projection, false, [&]()
    {
        ID3D11ShaderResourceView* srv = m_defaultTex.Get();
        context->PSSetShaderResources(0, 1, &srv);
    });

        // Lighting settings
    m_cup->UpdateEffects([&](IEffect* effect)
    {
        auto lights = dynamic_cast<IEffectLights*>(effect);
        if (lights)
        {
            XMVECTOR dir = XMVector3Rotate(g_XMOne, quat);
            lights->SetLightDirection(0, dir);
        }
    });
    local = XMMatrixTranslation(-1.5f, row0, 0.f);
    m_cup->Draw(context, *m_states, local, m_view, m_projection);

        // Per pixel lighting
    m_cup->UpdateEffects([&](IEffect* effect)
    {
        auto lights = dynamic_cast<IEffectLights*>(effect);
        if (lights)
        {
            lights->SetPerPixelLighting(true);
        }
    });
    local = XMMatrixTranslation(-3.f, row0, 0.f);
    m_cup->Draw(context, *m_states, local, m_view, m_projection);
    m_cup->UpdateEffects([&](IEffect* effect)
    {
        auto lights = dynamic_cast<IEffectLights*>(effect);
        if (lights)
        {
            lights->SetPerPixelLighting(false);
        }
    });

        // Fog settings
#ifdef GAMMA_CORRECT_RENDERING
    XMVECTORF32 fogColor;
    fogColor.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
#else
    XMVECTOR fogColor = Colors::CornflowerBlue;
#endif
    m_cup->UpdateEffects([&](IEffect* effect)
    {
        auto lights = dynamic_cast<IEffectLights*>(effect);
        if (lights)
            lights->EnableDefaultLighting();

        auto fog = dynamic_cast<IEffectFog*>(effect);
        if (fog)
        {
            fog->SetFogEnabled(true);
            fog->SetFogStart(fogstart);
            fog->SetFogEnd(fogend);
            fog->SetFogColor(fogColor);
        }
    });
    local = XMMatrixTranslation(-4.f, row0, cos(time) * 2.f);
    m_cup->Draw(context, *m_states, local, m_view, m_projection);
    m_cup->UpdateEffects([&](IEffect* effect)
    {
        auto fog = dynamic_cast<IEffectFog*>(effect);
        if (fog)
            fog->SetFogEnabled(false);
    });

        // Custom drawing
    local = XMMatrixRotationX(cos(time)) * XMMatrixTranslation(-5.f, row0, cos(time) * 2.f);
    for (const auto& mit : m_cup->meshes)
    {
        auto mesh = mit.get();
        assert(mesh != 0);

        mesh->PrepareForRendering(context, *m_states.get());

        for (const auto& it : mesh->meshParts)
        {
            auto part = it.get();
            assert(part != 0);

            auto imatrices = dynamic_cast<IEffectMatrices*>(part->effect.get());
            if (imatrices) imatrices->SetWorld(local);

            part->Draw(context, part->effect.get(), part->inputLayout.Get());
        }
    }

        // Custom drawing using instancing
    if (device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_9_3)
    {
        {
            size_t j = 0;
            for (float y = -4.f; y <= 4.f; y += 1.f)
            {
                XMMATRIX m = world * XMMatrixTranslation(0.f, y, cos(time + float(j) * XM_PIDIV4));
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

        local = XMMatrixTranslation(6.f, 0, 0);
        for (const auto& mit : m_cupInst->meshes)
        {
            auto mesh = mit.get();
            assert(mesh != 0);

            mesh->PrepareForRendering(context, *m_states.get());

            for (const auto& it : mesh->meshParts)
            {
                auto part = it.get();
                assert(part != 0);

                auto imatrices = dynamic_cast<IEffectMatrices*>(part->effect.get());
                if (imatrices)
                {
                    imatrices->SetMatrices(local, m_view, m_projection);
                }

                part->DrawInstanced(context, part->effect.get(), part->inputLayout.Get(), m_instanceCount);
            }
        }
    }

    //--- Draw VBO models ------------------------------------------------------------------
    local = XMMatrixMultiply(XMMatrixScaling(0.25f, 0.25f, 0.25f), XMMatrixTranslation(4.5f, row0, 0.f));
    local = XMMatrixMultiply(world, local);
    m_vbo->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(0.25f, 0.25f, 0.25f), XMMatrixTranslation(4.5f, row2, 0.f));
    local = XMMatrixMultiply(world, local);
    m_vbo2->Draw(context, *m_states, local, m_view, m_projection);

    //--- Draw CMO models ------------------------------------------------------------------
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
    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(-3.5f, row1, 0.f));
    local = XMMatrixMultiply(world, local);
    m_teapot->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(0.1f, 0.1f, 0.1f), XMMatrixTranslation(0.f, row1, 0.f));
    local = XMMatrixMultiply(world, local);
    m_gamelevel->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(.2f, .2f, .2f), XMMatrixTranslation(0.f, row2, 0.f));
    local = XMMatrixMultiply(world, local);
    m_ship->Draw(context, *m_states, local, m_view, m_projection);

    //--- Draw SDKMESH models --------------------------------------------------------------
    local = XMMatrixMultiply(XMMatrixScaling(0.005f, 0.005f, 0.005f), XMMatrixTranslation(2.5f, row2, 0.f));
    local = XMMatrixMultiply(world, local);
    m_tiny->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixTranslation(-2.5f, row2, 0.f);
    local = XMMatrixMultiply(world, local);
    m_dwarf->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(-5.0f, row2, 0.f));
    local = XMMatrixMultiply(XMMatrixRotationRollPitchYaw(0, XM_PI, roll), local);
    m_lmap->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(0.05f, 0.05f, 0.05f), XMMatrixTranslation(-5.0f, row1, 0.f));
    local = XMMatrixMultiply(world, local);
    m_nmap->Draw(context, *m_states, local, m_view, m_projection);

    m_soldier->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->ResetBoneTransforms();
    });
    local = XMMatrixMultiply(XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(3.5f, row1, 0.f));
    local = XMMatrixMultiply(world, local);
    m_soldier->Draw(context, *m_states, local, m_view, m_projection);

    m_soldier->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->SetBoneTransforms(m_bones.get(), IEffectSkinning::MaxBones);
    });
    local = XMMatrixMultiply(XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(2.5f, row1, 0.f));
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

    m_abstractFXFactory = std::make_unique<EffectFactory>(device);

    m_fxFactory = std::make_unique<EffectFactory>(device);

#ifdef GAMMA_CORRECT_RENDERING
    m_fxFactory->EnableForceSRGB(true);
    bool forceSRGB = true;
#else
    bool forceSRGB = false;
#endif

#ifndef NORMALMAPS
    m_fxFactory->EnableNormalMapEffect(false);
#endif

    auto context = m_deviceResources->GetD3DDeviceContext();

#ifdef LH_COORDS
    bool ccw = false;
#else
    bool ccw = true;
#endif

    // Wavefront OBJ
#ifdef GAMMA_CORRECT_RENDERING
    m_cup = CreateModelFromOBJ(device, context, L"cup._obj", *m_fxFactory, false, (ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise) | ModelLoader_MaterialColorsSRGB);
    m_cupInst = CreateModelFromOBJ(device, context, L"cup._obj", *m_fxFactory, true, (ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise) | ModelLoader_MaterialColorsSRGB);
#else
    m_cup = CreateModelFromOBJ(device, context, L"cup._obj", *m_fxFactory, false, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);
    m_cupInst = CreateModelFromOBJ(device, context, L"cup._obj", *m_fxFactory, true, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);
#endif

    // VBO
    m_vbo = Model::CreateFromVBO(device, L"player_ship_a.vbo", nullptr, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);

    m_effect = std::make_shared<EnvironmentMapEffect>(device);
    m_effect->EnableDefaultLighting();

        // Load test textures
    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"default.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB,
        nullptr, m_defaultTex.ReleaseAndGetAddressOf()));

    m_effect->SetTexture(m_defaultTex.Get());

    DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"cubemap.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, forceSRGB, 
        nullptr, m_cubemap.ReleaseAndGetAddressOf()));

    m_effect->SetEnvironmentMap(m_cubemap.Get());

    m_vbo2 = Model::CreateFromVBO(device, L"player_ship_a.vbo", m_effect, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);

    // Visual Studio CMO

    // TODO - forceSRGB behavior for material colors

    m_teapot = Model::CreateFromCMO(device, L"teapot.cmo", *m_fxFactory, ccw ? ModelLoader_CounterClockwise : ModelLoader_Clockwise);

    m_gamelevel = Model::CreateFromCMO(device, L"gamelevel.cmo", *m_fxFactory, ccw ? ModelLoader_CounterClockwise : ModelLoader_Clockwise);

    m_ship = Model::CreateFromCMO(device, L"25ab10e8-621a-47d4-a63d-f65a00bc1549_model.cmo", *m_fxFactory, ccw ? ModelLoader_CounterClockwise : ModelLoader_Clockwise);

    // DirectX SDK Mesh
    m_cupMesh = Model::CreateFromSDKMESH(device, L"cup.sdkmesh", *m_fxFactory, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);
    m_tiny = Model::CreateFromSDKMESH(device, L"tiny.sdkmesh", *m_fxFactory, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);
    m_soldier = Model::CreateFromSDKMESH(device, L"soldier.sdkmesh", *m_fxFactory, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);
    m_dwarf = Model::CreateFromSDKMESH(device, L"dwarf.sdkmesh", *m_fxFactory, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);
    m_lmap = Model::CreateFromSDKMESH(device, L"SimpleLightMap.sdkmesh", *m_fxFactory, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);
    m_nmap = Model::CreateFromSDKMESH(device, L"Helmet.sdkmesh", *m_fxFactory, ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise);

    // Create instance transforms.
    {
        size_t j = 0;
        for (float y = -4.f; y <= 4.f; y += 1.f)
        {
            ++j;
        }
        m_instanceCount = static_cast<UINT>(j);

        m_instanceTransforms = std::make_unique<XMFLOAT3X4[]>(j);

        constexpr XMFLOAT3X4 s_identity = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f };

        j = 0;
        for (float y = -4.f; y <= 4.f; y += 1.f)
        {
            m_instanceTransforms[j] = s_identity;
            m_instanceTransforms[j]._24 = y;
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
    m_effect.reset();

    m_cup.reset();
    m_cupInst.reset();
    m_cupMesh.reset();
    m_vbo.reset();
    m_vbo2.reset();
    m_teapot.reset();
    m_gamelevel.reset();
    m_ship.reset();
    m_tiny.reset();
    m_soldier.reset();
    m_dwarf.reset();
    m_lmap.reset();
    m_nmap.reset();

    m_instancedVB.Reset();

    m_abstractFXFactory.reset();
    m_fxFactory.reset();

    m_defaultTex.Reset();
    m_cubemap.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
