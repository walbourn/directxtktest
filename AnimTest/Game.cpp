//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK Model animation (under development)
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

    auto time = static_cast<float>(m_timer.GetTotalSeconds());

    XMMATRIX world = XMMatrixRotationY(XM_PI);

    // Skinning settings
    float s = 1 + sin(time * 1.7f) * 0.5f;

    XMMATRIX scale = XMMatrixScaling(s, s, s);

    for (size_t j = 0; j < SkinnedEffect::MaxBones; ++j)
    {
        m_bones[j] = scale;
    }

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    XMMATRIX local = XMMatrixMultiply(XMMatrixScaling(0.3f, 0.3f, 0.3f), XMMatrixTranslation(0.f, row2, 0.f));
    local = XMMatrixMultiply(world, local);
    m_tank->Draw(context, *m_states, local, m_view, m_projection);

    m_teapot->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->ResetBoneTransforms();
    });
    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(-2.f, row0, 0.f));
    m_teapot->Draw(context, *m_states, local, m_view, m_projection);

    m_teapot->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->SetBoneTransforms(m_bones.get(), SkinnedEffect::MaxBones);
    });
    local = XMMatrixMultiply(XMMatrixScaling(0.01f, 0.01f, 0.01f), XMMatrixTranslation(-2.f, row1, 0.f));
    m_teapot->Draw(context, *m_states, local, m_view, m_projection);

    // Draw SDKMESH models
    m_soldier->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->ResetBoneTransforms();
    });
    local = XMMatrixMultiply(XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(2.f, row0, 0.f));
    local = XMMatrixMultiply(world, local);
    m_soldier->Draw(context, *m_states, local, m_view, m_projection);

    m_soldier->UpdateEffects([&](IEffect* effect)
    {
        auto skinnedEffect = dynamic_cast<IEffectSkinning*>(effect);
        if (skinnedEffect)
            skinnedEffect->SetBoneTransforms(m_bones.get(), SkinnedEffect::MaxBones);
    });
    local = XMMatrixMultiply(XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(2.f, row1, 0.f));
    local = XMMatrixMultiply(world, local);
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
    ModelLoaderFlags flags = ccw ? ModelLoader_CounterClockwise : ModelLoader_Clockwise;
    m_teapot = Model::CreateFromCMO(device, L"teapot.cmo", *m_fxFactory, flags | ModelLoader_IncludeSkeleton);

    DumpBones(m_teapot->bones, "teapot.cmo");

    // DirectX SDK Mesh
    flags = ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise;

    m_tank = Model::CreateFromSDKMESH(device, L"TankScene.sdkmesh", *m_fxFactory, flags | ModelLoader_IncludeBones);

    DumpBones(m_tank->bones, "TankScene.sdkmesh");

    m_soldier = Model::CreateFromSDKMESH(device, L"soldier.sdkmesh", *m_fxFactory, flags | ModelLoader_IncludeSkeleton);

    DumpBones(m_soldier->bones, "soldier.sdkmesh");
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
    XMMATRIX orient = XMLoadFloat4x4(&m_deviceResources->GetOrientationTransform3D());
    m_projection *= orient;
#endif
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_states.reset();

    m_teapot.reset();
    m_soldier.reset();
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
