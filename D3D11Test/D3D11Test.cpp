//--------------------------------------------------------------------------------------
// File: D3D11Test.cpp
//
// Developer unit test for basic Direct3D 11 support
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
#include "D3D11Test.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

// Constructor.
Game::Game()
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    IUnknown* window,
#else
    HWND window,
#endif
    int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_keyboard = std::make_unique<Keyboard>();

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    // SimpleMath interop tests for Windows Runtime types
    Rectangle test1(10, 20, 50, 100);

    Windows::Foundation::Rect test2 = test1;
    if (test1.x != long(test2.X)
        && test1.y != long(test2.Y)
        && test1.width != long(test2.Width)
        && test1.height != long(test2.Height))
    {
        OutputDebugStringA("SimpleMath::Rectangle operator test A failed!");
        throw ref new Platform::Exception(E_FAIL);
    }
#endif
}

// Executes basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world
void Game::Update(DX::StepTimer const&)
{
    auto kb = m_keyboard->GetState();

    if (kb.Escape)
    {
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
        PostQuitMessage(0);
#else
        Windows::ApplicationModel::Core::CoreApplication::Exit();
#endif
    }
}

// Draws the scene
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
        return;

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    m_effect->Apply(context);

    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context->RSSetState(m_states->CullNone());

    context->IASetInputLayout(m_inputLayout.Get());

    m_batch->Begin();
    
    // Point
    {
        VertexPositionColor points[]
        {
            { Vector3(-0.75f, -0.75f, 0.5f), Colors::Red },
            { Vector3(-0.75f, -0.5f,  0.5f), Colors::Green },
            { Vector3(-0.75f, -0.25f, 0.5f), Colors::Blue },
            { Vector3(-0.75f,  0.0f,  0.5f), Colors::Yellow },
            { Vector3(-0.75f,  0.25f, 0.5f), Colors::Magenta },
            { Vector3(-0.75f,  0.5f,  0.5f), Colors::Cyan },
            { Vector3(-0.75f,  0.75f, 0.5f), Colors::White },
        };

        m_batch->Draw(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, points, _countof(points));
    }

    // Lines
    {
        VertexPositionColor lines[] =
        {
            { Vector3(-0.75f, -0.85f, 0.5f), Colors::Red },   { Vector3(0.75f, -0.85f, 0.5f), Colors::DarkRed },
            { Vector3(-0.75f, -0.90f, 0.5f), Colors::Green }, { Vector3(0.75f, -0.90f, 0.5f), Colors::DarkGreen },
            { Vector3(-0.75f, -0.95f, 0.5f), Colors::Blue },  { Vector3(0.75f, -0.95f, 0.5f), Colors::DarkBlue },
        };

        m_batch->DrawLine(lines[0], lines[1]);
        m_batch->DrawLine(lines[2], lines[3]);
        m_batch->DrawLine(lines[4], lines[5]);
    }

    // Triangle
    {
        VertexPositionColor v1(Vector3(0.f, 0.5f, 0.5f), Colors::Red);
        VertexPositionColor v2(Vector3(0.5f, -0.5f, 0.5f), Colors::Green);
        VertexPositionColor v3(Vector3(-0.5f, -0.5f, 0.5f), Colors::Blue);

        m_batch->DrawTriangle(v1, v2, v3);
    }

    // Quads
    {
        VertexPositionColor quad[] =
        {
            { Vector3(0.75f, 0.75f, 0.5), Colors::Gray },
            { Vector3(0.95f, 0.75f, 0.5), Colors::Gray },
            { Vector3(0.95f, -0.75f, 0.5), Colors::DarkGray },
            { Vector3(0.75f, -0.75f, 0.5), Colors::DarkGray },
        };

        m_batch->DrawQuad(quad[0], quad[1], quad[2], quad[3]);
    }

    m_batch->End();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the backbuffers
void Game::Clear()
{
    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetBackBufferRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}

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
}

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

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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

// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetVertexColorEnabled(true);

    void const* shaderByteCode;
    size_t byteCodeLength;

    m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    DX::ThrowIfFailed(
            device->CreateInputLayout(VertexPositionColor::InputElements,
                VertexPositionColor::InputElementCount,
                shaderByteCode, byteCodeLength,
                m_inputLayout.ReleaseAndGetAddressOf()));

    m_states = std::make_unique<CommonStates>(device);

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    SetDebugObjectName(m_deviceResources->GetDepthStencilView(), "DepthStencil");
}

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