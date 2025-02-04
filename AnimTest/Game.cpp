//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK Model animation
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
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

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
#ifdef GAMMA_CORRECT_RENDERING
    // Linear colors for DirectXMath were not added until v3.17 in the Windows SDK (22621)
    const XMVECTORF32 c_clearColor = { { { 0.127437726f, 0.300543845f, 0.846873462f, 1.f } } };
#else
    const XMVECTORF32 c_clearColor = Colors::CornflowerBlue;
#endif

    constexpr float row0 = 2.f;
    constexpr float row1 = 0.f;
    constexpr float row2 = -2.f;

    void DumpBones(const ModelBone::Collection& bones, _In_z_ const char* name)
    {
        char buff[128] = {};
        if (bones.empty())
        {
            sprintf_s(buff, "ERROR: %s is missing model bones!\n", name);
            OutputDebugStringA(buff);
        }
        else
        {
            sprintf_s(buff, "%s: contains %zu bones\n", name, bones.size());
            OutputDebugStringA(buff);

            for (auto it : bones)
            {
                sprintf_s(buff, "\t'%ls' (%s | %s | %s)\n",
                    it.name.c_str(),
                    (it.childIndex != ModelBone::c_Invalid) ? "has children" : "no children",
                    (it.siblingIndex != ModelBone::c_Invalid) ? "has sibling" : "no siblings",
                    (it.parentIndex != ModelBone::c_Invalid) ? "has parent" : "no parent");
                OutputDebugStringA(buff);
            }
        }
    }
}

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
        c_RenderFormat, DXGI_FORMAT_D24_UNORM_S8_UINT, 2, D3D_FEATURE_LEVEL_9_3,
        DX::DeviceResources::c_Enable4K_Xbox | DX::DeviceResources::c_EnableQHD_Xbox
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
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    auto kb = m_keyboard->GetState();
    if (kb.Escape || (pad.IsConnected() && pad.IsViewPressed()))
    {
        ExitGame();
    }

    m_soldierAnim.Update(elapsedTime);
    m_teapotAnim.Update(elapsedTime);
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

    XMMATRIX world = XMMatrixRotationY(XM_PI);

    // Skinning settings
    float s = 1 + sin(time * 1.7f) * 0.5f;

    XMMATRIX scale = XMMatrixScaling(s, s, s);

    for (size_t j = 0; j < IEffectSkinning::MaxBones; ++j)
    {
        m_bones[j] = scale;
    }

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    // Tank scene (rigid-body animation)
    XMMATRIX local = XMMatrixMultiply(XMMatrixScaling(0.3f, 0.3f, 0.3f), XMMatrixTranslation(0.f, row2, 0.f));
    local = XMMatrixMultiply(world, local);

    constexpr uint32_t rootBone = 0;
    uint32_t tankBone = ModelBone::c_Invalid;
    uint32_t barricadeBone = ModelBone::c_Invalid;
    uint32_t nbones = 0;
    {
        for (auto it : m_tank->bones)
        {
            if (_wcsicmp(L"tank_geo", it.name.c_str()) == 0)
            {
                tankBone = nbones;
            }
            else if (_wcsicmp(L"barricade_geo", it.name.c_str()) == 0)
            {
                barricadeBone = nbones;
            }

            ++nbones;
        }

        assert(nbones == m_tank->bones.size());
    }

    m_tank->boneMatrices[rootBone] = XMMatrixRotationY((time / 10.f) * XM_PI);
    m_tank->boneMatrices[tankBone] = XMMatrixRotationY(time * XM_PI);

    m_tank->boneMatrices[barricadeBone] = XMMatrixTranslation(0.f, cosf(time) * 2.f, 0.f);

    auto bones = ModelBone::MakeArray(nbones);
    m_tank->CopyAbsoluteBoneTransformsTo(nbones, bones.get());
        // For SDKMESH rigid-body, the matrix data is the local position to use.

    m_tank->Draw(context, *m_states, nbones, bones.get(), local, m_view, m_projection);

    // Teapot (direct-mapped bones)
    m_teapot->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->ResetBoneTransforms();
    });
    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(-2.f, row0, 0.f));
    m_teapot->Draw(context, *m_states, local, m_view, m_projection);

    nbones = static_cast<uint32_t>(m_teapot->bones.size());
    bones = ModelBone::MakeArray(nbones);
    m_teapotAnim.Apply(*m_teapot, m_teapot->bones.size(), bones.get());

    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(-2.f, row1, 0.f));
    m_teapot->DrawSkinned(context, *m_states,
        nbones, bones.get(),
        local, m_view, m_projection);

    // Draw SDKMESH models (bone influences)
    m_soldier->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->ResetBoneTransforms();
    });
    local = XMMatrixMultiply(XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(2.f, row0, 0.f));
    local = XMMatrixMultiply(world, local);
    m_soldier->Draw(context, *m_states, local, m_view, m_projection);

    m_soldierDiff->UpdateEffects([&](IEffect* effect)
        {
            auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
            if (skinnedEffect)
                skinnedEffect->ResetBoneTransforms();
        });
    local = XMMatrixMultiply(XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(4.f, row0, 0.f));
    local = XMMatrixMultiply(world, local);
    m_soldierDiff->Draw(context, *m_states, local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(2.f, row1, 0.f));
    local = XMMatrixMultiply(XMMatrixRotationY(XM_PI), local);
    local = XMMatrixMultiply(world, local);

    nbones = static_cast<uint32_t>(m_soldier->bones.size());
    bones = ModelBone::MakeArray(nbones);
    m_soldierAnim.Apply(*m_soldier, m_soldier->bones.size(), bones.get());

    m_soldier->DrawSkinned(context, *m_states,
        nbones, bones.get(),
        local, m_view, m_projection);

    local = XMMatrixMultiply(XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(4.f, row1, 0.f));
    local = XMMatrixMultiply(XMMatrixRotationY(XM_PI), local);
    local = XMMatrixMultiply(world, local);

    m_soldierDiff->DrawSkinned(context, *m_states,
        nbones, bones.get(),
        local, m_view, m_projection);

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
    m_states = std::make_unique<CommonStates>(device);

    m_fxFactory = std::make_unique<EffectFactory>(device);

#ifdef GAMMA_CORRECT_RENDERING
    m_fxFactory->EnableForceSRGB(true);
#endif

#ifdef LH_COORDS
    bool ccw = false;
#else
    bool ccw = true;
#endif

    // Visual Studio CMO
    ModelLoaderFlags flags = (ccw ? ModelLoader_CounterClockwise : ModelLoader_Clockwise)
        | ModelLoader_IncludeBones;

    size_t animsOffset;
    m_teapot = Model::CreateFromCMO(device, L"teapot.cmo", *m_fxFactory, flags, &animsOffset);

    DumpBones(m_teapot->bones, "teapot.cmo");

    if (!animsOffset)
    {
        OutputDebugStringA("ERROR: 'teapot.cmo' - No animation clips found in file!\n");
    }
    else
    {
        DX::ThrowIfFailed(m_teapotAnim.Load(L"teapot.cmo", animsOffset, L"Take 001"));

        OutputDebugStringA("'teapot.cmo' contains animation clips.\n");
    }

    // DirectX SDK Mesh
    flags = (ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise)
        | ModelLoader_IncludeBones;

    m_tank = Model::CreateFromSDKMESH(device, L"TankScene.sdkmesh", *m_fxFactory, flags);

    DumpBones(m_tank->bones, "TankScene.sdkmesh");

    m_soldier = Model::CreateFromSDKMESH(device, L"soldier.sdkmesh", *m_fxFactory, flags);

    DumpBones(m_soldier->bones, "soldier.sdkmesh");

    DX::ThrowIfFailed(m_soldierAnim.Load(L"soldier.sdkmesh_anim"));

    if (!m_soldierAnim.Bind(*m_soldier))
    {
        OutputDebugStringA("ERROR: Bind of soldier to animation failed to find any matching bones!\n");
    }

    m_fxFactory->EnableNormalMapEffect(false);
    m_soldierDiff = Model::CreateFromSDKMESH(device, L"soldier.sdkmesh", *m_fxFactory, flags);

    m_teapotAnim.Bind(*m_teapot);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 cameraPosition = { { { 0.f, 0.f, 6.f, 0.f } } };

    const auto size = m_deviceResources->GetOutputSize();
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

    m_teapot.reset();
    m_soldier.reset();
    m_soldierDiff.reset();
    m_tank.reset();

    m_fxFactory.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
