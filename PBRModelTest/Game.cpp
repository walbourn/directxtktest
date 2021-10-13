//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK PBR Model Test
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"

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
    const XMVECTORF32 c_BrightYellow = { { { 2.f, 2.f, 0.f, 1.f } } };

    constexpr float rowtop = 6.f;
    constexpr float row0 = 1.5f;
    constexpr float row1 = 0.f;
    constexpr float row2 = -1.5f;
}

// Constructor.
Game::Game() noexcept(false) :
    m_instanceCount(0),
    m_toneMapMode(ToneMapPostProcess::Reinhard),
    m_ibl(0),
    m_spinning(true),
    m_pitch(0),
    m_yaw(0)
{
#if defined(TEST_HDR_LINEAR) && !defined(XBOX)
    const DXGI_FORMAT c_DisplayFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
#else
    const DXGI_FORMAT c_DisplayFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
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
        DX::DeviceResources::c_EnableHDR | DX::DeviceResources::c_Enable4K_Xbox);
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
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

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

    if (m_keyboardButtons.pressed.T)
    {
        CycleToneMapOperator();
    }

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

    float alphaFade = (sin(time * 2) + 1) / 2;

    if (alphaFade >= 1)
        alphaFade = 1 - FLT_EPSILON;

    float yaw = time * 1.4f;

    XMMATRIX world;
    XMVECTOR quat;

    if (m_spinning)
    {
        world = XMMatrixRotationRollPitchYaw(0, yaw, 0);
        quat = XMQuaternionRotationRollPitchYaw(0, yaw, 0);
    }
    else
    {
        world = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0);
        quat = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0);
    }

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    auto vp = m_deviceResources->GetOutputSize();
    auto safeRect = Viewport::ComputeTitleSafeArea(UINT(vp.right - vp.left), UINT(vp.bottom - vp.top));

    //--- Set PBR lighting sources ---
    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
    m_radianceIBL[m_ibl]->GetDesc(&desc);

    m_cube->UpdateEffects([&](IEffect* effect)
    {
        auto pbr = dynamic_cast<PBREffect*>(effect);
        if (pbr)
        {
            pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL[m_ibl].Get());
        }
    });

    m_sphere->UpdateEffects([&](IEffect* effect)
    {
        auto pbr = dynamic_cast<PBREffect*>(effect);
        if (pbr)
        {
            pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL[m_ibl].Get());
        }
    });

    m_sphere2->UpdateEffects([&](IEffect* effect)
    {
        auto pbr = dynamic_cast<PBREffect*>(effect);
        if (pbr)
        {
            pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL[m_ibl].Get());
        }
    });

    m_robot->UpdateEffects([&](IEffect* effect)
    {
        auto pbr = dynamic_cast<PBREffect*>(effect);
        if (pbr)
        {
            pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL[m_ibl].Get());
        }
    });

    m_cubeInst->UpdateEffects([&](IEffect* effect)
    {
        auto pbr = dynamic_cast<PBREffect*>(effect);
        if (pbr)
        {
            pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL[m_ibl].Get());
        }
    });

    m_teapot->UpdateEffects([&](IEffect* effect)
    {
        auto pbr = dynamic_cast<PBREffect*>(effect);
        if (pbr)
        {
            pbr->SetIBLTextures(m_radianceIBL[m_ibl].Get(), static_cast<int>(desc.TextureCube.MipLevels), m_irradianceIBL[m_ibl].Get());
        }
    });

    //--- Draw SDKMESH models ---
    XMMATRIX local = XMMatrixTranslation(1.5f, row0, 0.f);
    local = XMMatrixMultiply(world, local);
    m_cube->Draw(context, *m_states, local, m_view, m_projection);

    {
        XMMATRIX scale = XMMatrixScaling(0.75f, 0.75f, 0.75f);
        XMMATRIX trans = XMMatrixTranslation(-2.5f, row0, 0.f);
        local = XMMatrixMultiply(scale, trans);
        local = XMMatrixMultiply(world, local);
    }
    m_sphere->Draw(context, *m_states, local, m_view, m_projection);

    {
        XMMATRIX scale = XMMatrixScaling(0.75f, 0.75f, 0.75f);
        XMMATRIX trans = XMMatrixTranslation(-2.5f, row2, 0.f);
        local = XMMatrixMultiply(scale, trans);
        local = XMMatrixMultiply(world, local);
    }
    m_sphere2->Draw(context, *m_states, local, m_view, m_projection);

    {
        XMMATRIX scale = XMMatrixScaling(0.1f, 0.1f, 0.1f);
        XMMATRIX trans = XMMatrixTranslation(1.5f, row2, 0.f);
        local = XMMatrixMultiply(scale, trans);
        local = XMMatrixMultiply(world, local);
    }
    m_robot->Draw(context, *m_states, local, m_view, m_projection);

    //--- Draw with instancing ---
    local = XMMatrixTranslation(0.f, rowtop, 0.f) * XMMatrixScaling(0.25f, 0.25f, 0.25f);
    {
        {
            size_t j = 0;
            for (float x = -16.f; x <= 16.f; x += 4.f)
            {
                XMMATRIX m = world * XMMatrixTranslation(x, rowtop, cos(time + float(j) * XM_PIDIV4));
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

        for (const auto& mit : m_cubeInst->meshes)
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

    //--- Draw with skinning ---
    local = XMMatrixMultiply(XMMatrixScaling(0.02f, 0.02f, 0.02f), XMMatrixTranslation(4.f, row1, 0.f));

    auto nbones = static_cast<uint32_t>(m_teapot->bones.size());
    auto bones = ModelBone::MakeArray(nbones);
    m_teapotAnim.Apply(*m_teapot, m_teapot->bones.size(), bones.get());

    m_teapot->DrawSkinned(context, *m_states,
        nbones, bones.get(),
        local, m_view, m_projection);

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

    XMVECTORF32 color;
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
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

    m_hdrScene->SetDevice(device);

    m_toneMap = std::make_unique<ToneMapPostProcess>(device);
    m_toneMap->SetOperator(static_cast<ToneMapPostProcess::Operator>(m_toneMapMode));
    m_toneMap->SetTransferFunction(ToneMapPostProcess::SRGB);

#ifdef XBOX
    m_toneMap->SetMRTOutput(true);
#endif

    m_states = std::make_unique<CommonStates>(device);

    m_fxFactory = std::make_unique<PBREffectFactory>(device);

#ifdef LH_COORDS
    bool ccw = false;
#else
    bool ccw = true;
#endif

#ifdef PC
#define IBL_PATH L"..\\PBRTest\\"
#else
#define IBL_PATH
#endif

    static const wchar_t* s_radianceIBL[s_nIBL] =
    {
        IBL_PATH L"Atrium_diffuseIBL.dds",
        IBL_PATH L"Garage_diffuseIBL.dds",
        IBL_PATH L"SunSubMixer_diffuseIBL.dds",
    };
    static const wchar_t* s_irradianceIBL[s_nIBL] =
    {
        IBL_PATH L"Atrium_specularIBL.dds",
        IBL_PATH L"Garage_specularIBL.dds",
        IBL_PATH L"SunSubMixer_specularIBL.dds",
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

    // DirectX SDK Mesh
    ModelLoaderFlags flags = ccw ? ModelLoader_Clockwise : ModelLoader_CounterClockwise;
    m_cube = Model::CreateFromSDKMESH(device, L"BrokenCube.sdkmesh", *m_fxFactory, flags);
    m_sphere = Model::CreateFromSDKMESH(device, L"Sphere.sdkmesh", *m_fxFactory, flags);
    m_sphere2 = Model::CreateFromSDKMESH(device, L"Sphere2.sdkmesh", *m_fxFactory, flags);
    m_robot = Model::CreateFromSDKMESH(device, L"ToyRobot.sdkmesh", *m_fxFactory, flags);

    // Create instanced mesh.
    m_fxFactory->SetSharing(false); // We do not want to reuse the effects created above!
    m_cubeInst = Model::CreateFromSDKMESH(device, L"BrokenCube.sdkmesh", *m_fxFactory, flags);

    static const D3D11_INPUT_ELEMENT_DESC s_instElements[] =
    {
        // XMFLOAT3X4
        { "InstMatrix",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "InstMatrix",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    };

    m_cubeInst->UpdateEffects([&](IEffect* effect)
    {
        auto pbr = dynamic_cast<PBREffect*>(effect);
        if (pbr)
        {
            pbr->SetInstancingEnabled(true);
        }
    });

    for (const auto& mit : m_cubeInst->meshes)
    {
        auto mesh = mit.get();
        assert(mesh != 0);

        for (const auto& it : mesh->meshParts)
        {
            auto part = it.get();
            assert(part != 0);

            auto il = *part->vbDecl;
            il.push_back(s_instElements[0]);
            il.push_back(s_instElements[1]);
            il.push_back(s_instElements[2]);

            DX::ThrowIfFailed(
                CreateInputLayoutFromEffect(device, part->effect.get(),
                    il.data(), il.size(),
                    part->inputLayout.ReleaseAndGetAddressOf()));
        }
    }

    // Create instance transforms.
    {
        size_t j = 0;
        for (float x = -16.f; x <= 16.f; x += 4.f)
        {
            ++j;
        }
        m_instanceCount = static_cast<UINT>(j);

        m_instanceTransforms = std::make_unique<XMFLOAT3X4[]>(j);

        constexpr XMFLOAT3X4 s_identity = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f };

        j = 0;
        for (float x = -16.f; x <= 16.f; x += 4.f)
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

    // Create skinning teapot.
    flags = (ccw ? ModelLoader_CounterClockwise : ModelLoader_Clockwise)
        | ModelLoader_IncludeBones;

    size_t animsOffset;
    m_teapot = Model::CreateFromCMO(device, L"teapot.cmo", *m_fxFactory, flags, &animsOffset);

    if (!animsOffset)
    {
        OutputDebugStringA("ERROR: 'teapot.cmo' - No animation clips found in file!\n");
    }
    else
    {
        DX::ThrowIfFailed(m_teapotAnim.Load(L"teapot.cmo", animsOffset, L"Take 001"));

        OutputDebugStringA("'teapot.cmo' contains animation clips.\n");
    }

    m_teapotAnim.Bind(*m_teapot);
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

    for (size_t j = 0; j < s_nIBL; ++j)
    {
        m_radianceIBL[j].Reset();
        m_irradianceIBL[j].Reset();
    }

    m_cube.reset();
    m_sphere.reset();
    m_sphere2.reset();
    m_robot.reset();
    m_cubeInst.reset();
    m_teapot.reset();

    m_fxFactory.reset();

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
