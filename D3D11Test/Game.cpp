//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for basic Direct3D 11 support
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

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

//--------------------------------------------------------------------------------------

#pragma warning(disable : 4061)

namespace
{
#ifdef GAMMA_CORRECT_RENDERING
    // Linear colors for DirectXMath were not added until v3.17 in the Windows SDK (22621)
    const XMVECTORF32 c_clearColor = { { { 0.127437726f, 0.300543845f, 0.846873462f, 1.f } } };
#else
    const XMVECTORF32 c_clearColor = Colors::CornflowerBlue;
#endif
}

//--------------------------------------------------------------------------------------

// Constructor.
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

#ifdef _DEBUG
    switch (m_deviceResources->GetDeviceFeatureLevel())
    {
    case D3D_FEATURE_LEVEL_9_1: OutputDebugStringA("INFO: Direct3D Hardware Feature level 9.1\n"); break;
    case D3D_FEATURE_LEVEL_9_2: OutputDebugStringA("INFO: Direct3D Hardware Feature level 9.2\n"); break;
    case D3D_FEATURE_LEVEL_9_3: OutputDebugStringA("INFO: Direct3D Hardware Feature level 9.3\n"); break;
    case D3D_FEATURE_LEVEL_10_0: OutputDebugStringA("INFO: Direct3D Hardware Feature level 10.0\n"); break;
    case D3D_FEATURE_LEVEL_10_1: OutputDebugStringA("INFO: Direct3D Hardware Feature level 10.1\n"); break;
    case D3D_FEATURE_LEVEL_11_0: OutputDebugStringA("INFO: Direct3D Hardware Feature level 11.0\n"); break;
    case D3D_FEATURE_LEVEL_11_1: OutputDebugStringA("INFO: Direct3D Hardware Feature level 11.1\n"); break;
    case D3D_FEATURE_LEVEL_12_0: OutputDebugStringA("INFO: Direct3D Hardware Feature level 12.0\n"); break;
    case D3D_FEATURE_LEVEL_12_1: OutputDebugStringA("INFO: Direct3D Hardware Feature level 12.1\n"); break;
#if defined(NTDDI_WIN10_FE) && !defined(__MINGW32__)
    case D3D_FEATURE_LEVEL_12_2: OutputDebugStringA("INFO: Direct3D Hardware Feature level 12.2\n"); break;
#endif
    default: OutputDebugStringA("INFO: Direct3D Hardware Feature level **UNKNOWN**\n");
    }
#endif

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    UnitTests();
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

    m_effect->Apply(context);

    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context->RSSetState(m_states->CullNone());

    context->IASetInputLayout(m_inputLayout.Get());

    m_batch->Begin();

    XMVECTORF32 red, green, blue, dred, dgreen, dblue, yellow, cyan, magenta, gray, dgray;
#ifdef GAMMA_CORRECT_RENDERING
    red.v = XMColorSRGBToRGB(Colors::Red);
    green.v = XMColorSRGBToRGB(Colors::Green);
    blue.v = XMColorSRGBToRGB(Colors::Blue);
    dred.v = XMColorSRGBToRGB(Colors::DarkRed);
    dgreen.v = XMColorSRGBToRGB(Colors::DarkGreen);
    dblue.v = XMColorSRGBToRGB(Colors::DarkBlue);
    yellow.v = XMColorSRGBToRGB(Colors::Yellow);
    cyan.v = XMColorSRGBToRGB(Colors::Cyan);
    magenta.v = XMColorSRGBToRGB(Colors::Magenta);
    gray.v = XMColorSRGBToRGB(Colors::Gray);
    dgray.v = XMColorSRGBToRGB(Colors::DarkGray);
#else
    red.v = Colors::Red;
    green.v = Colors::Green;
    blue.v = Colors::Blue;
    dred.v = Colors::DarkRed;
    dgreen.v = Colors::DarkGreen;
    dblue.v = Colors::DarkBlue;
    yellow.v = Colors::Yellow;
    cyan.v = Colors::Cyan;
    magenta.v = Colors::Magenta;
    gray.v = Colors::Gray;
    dgray.v = Colors::DarkGray;
#endif

    // Point
    {
        Vertex points[]
        {
            { Vector3(-0.75f, -0.75f, 0.5f), red },
            { Vector3(-0.75f, -0.5f,  0.5f), green },
            { Vector3(-0.75f, -0.25f, 0.5f), blue },
            { Vector3(-0.75f,  0.0f,  0.5f), yellow },
            { Vector3(-0.75f,  0.25f, 0.5f), magenta },
            { Vector3(-0.75f,  0.5f,  0.5f), cyan },
            { Vector3(-0.75f,  0.75f, 0.5f), Colors::White },
        };

        m_batch->Draw(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, points, static_cast<UINT>(std::size(points)));
    }

    // Lines
    {
        Vertex lines[] =
        {
            { Vector3(-0.75f, -0.85f, 0.5f), red },{ Vector3(0.75f, -0.85f, 0.5f), dred },
            { Vector3(-0.75f, -0.90f, 0.5f), green },{ Vector3(0.75f, -0.90f, 0.5f), dgreen },
            { Vector3(-0.75f, -0.95f, 0.5f), blue },{ Vector3(0.75f, -0.95f, 0.5f), dblue },
        };

        m_batch->DrawLine(lines[0], lines[1]);
        m_batch->DrawLine(lines[2], lines[3]);
        m_batch->DrawLine(lines[4], lines[5]);
    }

    // Triangle
    {
        Vertex v1(Vector3(0.f, 0.5f, 0.5f), red);
        Vertex v2(Vector3(0.5f, -0.5f, 0.5f), green);
        Vertex v3(Vector3(-0.5f, -0.5f, 0.5f), blue);

        m_batch->DrawTriangle(v1, v2, v3);
    }

    // Quads
    {
        Vertex quad[] =
        {
            { Vector3(0.75f, 0.75f, 0.5), gray },
            { Vector3(0.95f, 0.75f, 0.5), gray },
            { Vector3(0.95f, -0.75f, 0.5), dgray },
            { Vector3(0.75f, -0.75f, 0.5), dgray },
        };

        m_batch->DrawQuad(quad[0], quad[1], quad[2], quad[3]);
    }

    m_batch->End();

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
    auto context = m_deviceResources->GetD3DDeviceContext();

#ifdef XBOX
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device, m_deviceResources->GetBackBufferCount());
#endif

    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetVertexColorEnabled(true);

    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<Vertex>(device, m_effect.get(), m_inputLayout.ReleaseAndGetAddressOf())
    );

    m_states = std::make_unique<CommonStates>(device);

    m_batch = std::make_unique<PrimitiveBatch<Vertex>>(context);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    SetDebugObjectName(m_deviceResources->GetRenderTargetView(), L"RenderTarget");
    SetDebugObjectName(m_deviceResources->GetDepthStencilView(), "DepthStencil");
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_effect.reset();
    m_states.reset();
    m_batch.reset();
    m_inputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion

void Game::UnitTests()
{
    bool success = true;
    OutputDebugStringA("*********** UNIT TESTS BEGIN ***************\n");

#if defined(__cplusplus_winrt)
    // SimpleMath interop tests for Windows Runtime types
    Rectangle test1(10, 20, 50, 100);

    Windows::Foundation::Rect test2 = test1;
    if (test1.x != long(test2.X)
        && test1.y != long(test2.Y)
        && test1.width != long(test2.Width)
        && test1.height != long(test2.Height))
    {
        OutputDebugStringA("SimpleMath::Rectangle operator test A failed!");
        success = false;
    }
#endif

    OutputDebugStringA(success ? "Passed\n" : "Failed\n");
    OutputDebugStringA("***********  UNIT TESTS END  ***************\n");

    if (!success)
    {
        throw std::runtime_error("Unit Tests Failed");
    }
}
