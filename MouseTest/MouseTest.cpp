//--------------------------------------------------------------------------------------
// File: MouseTest.cpp
//
// Developer unit test for DirectXTK Mouse
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
#include "MouseTest.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

static const XMVECTORF32 START_POSITION = { 0.f, -1.5f, 0.f, 0.f };
static const XMVECTORF32 ROOM_BOUNDS = { 8.f, 6.f, 12.f, 0.f };

// Constructor.
Game::Game() :
    m_pitch(0),
    m_yaw(0),
    m_lastStr(nullptr),
    m_lastMode(Mouse::MODE_ABSOLUTE)
{
    m_cameraPos = START_POSITION.v;

    *m_lastStrBuff = 0;

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
    m_mouse = std::make_unique<Mouse>();

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    m_deviceResources->SetWindow(window, width, height, rotation);
    m_mouse->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
#else
    UNREFERENCED_PARAMETER(rotation);
    m_deviceResources->SetWindow(window, width, height);
    m_mouse->SetWindow(window);
#endif

    // Singleton test
    {
        bool thrown = false;

        try
        {
            std::unique_ptr<Mouse> mouse2(new Mouse);
        }
        catch (...)
        {
            thrown = true;
        }

        if (!thrown)
        {
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
            throw std::exception("Mouse not acting like a singleton");
#else
            MessageBox(window, L"Mouse not acting like a singleton", L"MouseTest", MB_ICONERROR);
#endif
        }

        auto state = Mouse::Get().GetState();
        state;
    }

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
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
    auto mouse = m_mouse->GetState();
    m_lastMode = mouse.positionMode;

    if (mouse.positionMode == Mouse::MODE_RELATIVE)
    {
        static const XMVECTORF32 ROTATION_GAIN = { 0.004f, 0.004f, 0.f, 0.f };
        XMVECTOR delta = XMVectorSet(float(mouse.x), float(mouse.y), 0.f, 0.f) * ROTATION_GAIN;

        m_pitch -= XMVectorGetY(delta);
        m_yaw -= XMVectorGetX(delta);

        // limit pitch to straight up or straight down
        float limit = XM_PI / 2.0f - 0.01f;
        m_pitch = std::max(-limit, m_pitch);
        m_pitch = std::min(+limit, m_pitch);

        // keep longitude in sane range by wrapping
        if (m_yaw > XM_PI)
        {
            m_yaw -= XM_PI * 2.0f;
        }
        else if (m_yaw < -XM_PI)
        {
            m_yaw += XM_PI * 2.0f;
        }
    }

    m_tracker.Update(mouse);

    if (m_tracker.leftButton == Mouse::ButtonStateTracker::ButtonState::PRESSED)
        m_lastStr = L"LeftButton was pressed";
    else if (m_tracker.leftButton == Mouse::ButtonStateTracker::ButtonState::RELEASED)
        m_lastStr = L"LeftButton was released";
    else if (m_tracker.rightButton == Mouse::ButtonStateTracker::ButtonState::PRESSED)
        m_lastStr = L"RightButton was pressed";
    else if (m_tracker.rightButton == Mouse::ButtonStateTracker::ButtonState::RELEASED)
        m_lastStr = L"RightButton was released";
    else if (m_tracker.middleButton == Mouse::ButtonStateTracker::ButtonState::PRESSED)
        m_lastStr = L"MiddleButton was pressed";
    else if (m_tracker.middleButton == Mouse::ButtonStateTracker::ButtonState::RELEASED)
        m_lastStr = L"MiddleButton was released";
    else if (m_tracker.xButton1 == Mouse::ButtonStateTracker::ButtonState::PRESSED)
        m_lastStr = L"XButton1 was pressed";
    else if (m_tracker.xButton1 == Mouse::ButtonStateTracker::ButtonState::RELEASED)
        m_lastStr = L"XButton1 was released";
    else if (m_tracker.xButton2 == Mouse::ButtonStateTracker::ButtonState::PRESSED)
        m_lastStr = L"XButton2 was pressed";
    else if (m_tracker.xButton2 == Mouse::ButtonStateTracker::ButtonState::RELEASED)
        m_lastStr = L"XButton2 was released";

    if (m_tracker.leftButton == Mouse::ButtonStateTracker::ButtonState::PRESSED)
    {
        m_mouse->SetMode(Mouse::MODE_RELATIVE);
    }
    else if (m_tracker.leftButton == Mouse::ButtonStateTracker::ButtonState::RELEASED)
    {
        m_mouse->SetMode(Mouse::MODE_ABSOLUTE);
    }

    m_ms = mouse;
}

// Draws the scene
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
        return;

    Clear();

    float y = sinf(m_pitch);        // vertical
    float r = cosf(m_pitch);        // in the plane
    float z = r*cosf(m_yaw);        // fwd-back
    float x = r*sinf(m_yaw);        // left-right

    XMVECTOR lookAt = XMVectorAdd( m_cameraPos, XMVectorSet(x, y, z, 0.f));

    XMMATRIX view = XMMatrixLookAtRH( m_cameraPos, lookAt, Vector3::Up);

    m_room->Draw(Matrix::Identity, view, m_proj, Colors::White, m_roomTex.Get());

    XMVECTOR xsize = m_comicFont->MeasureString(L"X");

    float height = XMVectorGetY(xsize);

    m_spriteBatch->Begin();

    XMFLOAT2 pos(50, 50);

    // Buttons
    m_comicFont->DrawString(m_spriteBatch.get(), L"LeftButton", pos, m_ms.leftButton ? Colors::Red : Colors::LightGray);
    pos.y += height * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"RightButton", pos, m_ms.rightButton ? Colors::Red : Colors::LightGray);
    pos.y += height * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"MiddleButton", pos, m_ms.middleButton ? Colors::Red : Colors::LightGray);
    pos.y += height * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"XButton1", pos, m_ms.xButton1 ? Colors::Red : Colors::LightGray);
    pos.y += height * 2;

    m_comicFont->DrawString(m_spriteBatch.get(), L"XButton2", pos, m_ms.xButton2 ? Colors::Red : Colors::LightGray);

    // Scroll Wheel
    pos.y += height * 2;
    {
        wchar_t buff[16] = {};
        swprintf_s(buff, L"%d", m_ms.scrollWheelValue);
        m_comicFont->DrawString(m_spriteBatch.get(), buff, pos, Colors::Black);
    }

    m_comicFont->DrawString(m_spriteBatch.get(), (m_ms.positionMode == Mouse::MODE_RELATIVE) ? L"Relative" : L"Absolute",
        XMFLOAT2(50, 550), Colors::Blue);

    if (m_lastStr)
    {
        m_comicFont->DrawString(m_spriteBatch.get(), m_lastStr, XMFLOAT2(50, 600), Colors::Yellow);
    }

    if (m_ms.positionMode == Mouse::MODE_ABSOLUTE)
    {
        m_spriteBatch->Draw(m_cursor.Get(), XMFLOAT2((float)m_ms.x, (float)m_ms.y));
    }

    m_spriteBatch->End();

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
    m_tracker.Reset();
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

void Game::OnHome()
{
    if (m_mouse)
        m_mouse->ResetScrollWheelValue();
}

void Game::OnEnd()
{
    if (!m_mouse)
        return;

    if (m_lastMode == Mouse::MODE_ABSOLUTE)
    {
        m_mouse->SetMode(Mouse::MODE_RELATIVE);
    }
    else
    {
        m_mouse->SetMode(Mouse::MODE_ABSOLUTE);
    }
}

void Game::SetDPI(float dpi)
{
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    if (m_mouse)
        m_mouse->SetDpi(dpi);
#else
    UNREFERENCED_PARAMETER(dpi);
#endif
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 1024;
    height = 768;
}

// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    m_comicFont = std::make_unique<SpriteFont>(device, L"comic.spritefont");

    m_room = GeometricPrimitive::CreateBox(context, XMFLOAT3(ROOM_BOUNDS[0], ROOM_BOUNDS[1], ROOM_BOUNDS[2]), false, true);

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"texture.dds", nullptr, m_roomTex.GetAddressOf()));
    DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"arrow.png", nullptr, m_cursor.GetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    m_proj = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(70.f), float(size.right) / float(size.bottom), 0.01f, 100.f);

    auto viewPort = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewPort);

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP) 
    m_spriteBatch->SetRotation(m_deviceResources->GetRotation());
#endif
}

void Game::OnDeviceLost()
{
    m_room.reset();
    m_spriteBatch.reset();
    m_comicFont.reset();

    m_roomTex.Reset();
    m_cursor.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}