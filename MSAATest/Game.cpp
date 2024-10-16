//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK MSAATest
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"

//#define GAMMA_CORRECT_RENDERING
//#define USE_FAST_SEMANTICS

// Build for LH vs. RH coords
//#define LH_COORDS

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#include <Windows.ApplicationModel.h>
#include <Windows.Storage.h>
#endif

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
#ifdef GAMMA_CORRECT_RENDERING
    // Linear colors for DirectXMath were not added until v3.17 in the Windows SDK (22621)
    const XMVECTORF32 c_clearColor = { { { 0.015996292f, 0.258182913f, 0.015996292f, 1.f } } };
    const XMVECTORF32 c_clearColor2 = { { { 0.417885154f, 0.686685443f, 0.791298151f, 1.f } } };
    const XMVECTORF32 c_clearColor4 = { { { 0.127437726f, 0.300543845f, 0.846873462f, 1.f } } };
    const XMVECTORF32 c_clearColor8 = { { { 0.009721218f, 0.009721218f, 0.162029430f, 1.f } } };
#else
    const XMVECTORF32 c_clearColor = Colors::ForestGreen;
    const XMVECTORF32 c_clearColor2 = Colors::LightBlue;
    const XMVECTORF32 c_clearColor4 = Colors::CornflowerBlue;
    const XMVECTORF32 c_clearColor8 = Colors::MidnightBlue;
#endif

    constexpr float ADVANCE_TIME = 1.f;
    constexpr float INTERACTIVE_TIME = 10.f;
}

// Constructor.
Game::Game() noexcept(false) :
    m_state(State::MSAA4X),
    m_frame(0),
    m_delay(0)
{
#ifdef GAMMA_CORRECT_RENDERING
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    constexpr DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif

    // If you weren't doing 'switchable' MSAA, then you'd use DXGI_FORMAT_UNKNOWN because you never need the non-MSAA depth buffer
    constexpr DXGI_FORMAT c_DepthFormat = DXGI_FORMAT_D32_FLOAT;

#ifdef XBOX
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, c_DepthFormat, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(UWP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, c_DepthFormat, 2, D3D_FEATURE_LEVEL_10_0,
        DX::DeviceResources::c_Enable4K_Xbox | DX::DeviceResources::c_EnableQHD_Xbox
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(c_RenderFormat, c_DepthFormat);
#endif

#ifdef LOSTDEVICE
    m_deviceResources->RegisterDeviceNotify(this);
#endif

    // Set up for MSAA rendering.

    m_msaaHelper2 = std::make_unique<DX::MSAAHelper>(m_deviceResources->GetBackBufferFormat(),
        c_DepthFormat,
        2);

    m_msaaHelper4 = std::make_unique<DX::MSAAHelper>(m_deviceResources->GetBackBufferFormat(),
        c_DepthFormat,
        4);

    m_msaaHelper8 = std::make_unique<DX::MSAAHelper>(m_deviceResources->GetBackBufferFormat(),
        c_DepthFormat,
        8);

    m_delay = ADVANCE_TIME;
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

    ++m_frame;
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitGame();
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED
            || m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            m_delay = INTERACTIVE_TIME;

            ++m_state;
            if (m_state >= State::COUNT)
                m_state = State::NOMSAA;
        }
        else if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED
            || m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            m_delay = INTERACTIVE_TIME;

            --m_state;
            if (m_state < 0)
                m_state = State::COUNT - 1;
        }
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitGame();
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Space))
    {
        m_delay = INTERACTIVE_TIME;

        ++m_state;
        if (m_state >= State::COUNT)
            m_state = State::NOMSAA;
    }
    else if (m_keyboardButtons.IsKeyPressed(Keyboard::Back))
    {
        m_delay = INTERACTIVE_TIME;

        --m_state;
        if (m_state < 0)
            m_state = State::COUNT - 1;
    }

    m_delay -= static_cast<float>(timer.GetElapsedSeconds());

    if (m_delay <= 0.f)
    {
        m_delay = ADVANCE_TIME;

        ++m_state;
        if (m_state >= State::COUNT)
            m_state = State::NOMSAA;
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

    // Draw test scene
    auto context = m_deviceResources->GetD3DDeviceContext();
    m_effect->Apply(context);

    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context->RSSetState((m_state == State::NOMSAA) ? m_states->CullCounterClockwise() : m_rstate.Get());

    context->IASetInputLayout(m_inputLayout.Get());

    auto linear = m_states->LinearClamp();
    context->PSSetSamplers(0, 1, &linear);

    m_batch->Begin();

    {
        Vertex v1(Vector3(0.f, 0.5f, 0.5f), Vector2(0, 0));
        Vertex v2(Vector3(0.5f, -0.5f, 0.5f), Vector2(1, 1));
        Vertex v3(Vector3(-0.5f, -0.5f, 0.5f), Vector2(0, 1));

        m_batch->DrawTriangle(v1, v2, v3);
    }

#if 0
    {
        Vertex quad[] =
        {
            { Vector3(0.75f, 0.75f, 0.5), Vector2(0, 0) },
            { Vector3(0.95f, 0.75f, 0.5), Vector2(0, 1) },
            { Vector3(0.95f, -0.75f, 0.5), Vector2(1, 0) },
            { Vector3(0.75f, -0.75f, 0.5), Vector2(1, 1) },
        };

        m_batch->DrawQuad(quad[0], quad[1], quad[2], quad[3]);
    }
#endif

    m_batch->End();

    // Resolve the frame.
    switch (m_state)
    {
    case State::MSAA2X:
        m_msaaHelper2->Resolve(context, m_deviceResources->GetRenderTarget());
        break;

    case State::MSAA4X:
        m_msaaHelper4->Resolve(context, m_deviceResources->GetRenderTarget());
        break;

    case State::MSAA8X:
        m_msaaHelper8->Resolve(context, m_deviceResources->GetRenderTarget());
        break;

    default:
        break;
    }

    if (m_frame == 10)
    {
        OutputDebugStringA("****** MSAA SCREENSHOT TEST BEGIN **********\n");

        bool success = true;

    #if defined(_XBOX_ONE) && defined(_TITLE)
        const wchar_t sspath[MAX_PATH] = L"T:\\";
    #elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
        wchar_t sspath[MAX_PATH] = {};

        using namespace Microsoft::WRL;
        using namespace Microsoft::WRL::Wrappers;
        using namespace ABI::Windows::Foundation;

        ComPtr<ABI::Windows::Storage::IApplicationDataStatics> appStatics;
        DX::ThrowIfFailed(GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), appStatics.GetAddressOf()));

        ComPtr<ABI::Windows::Storage::IApplicationData> appData;
        DX::ThrowIfFailed(appStatics->get_Current(appData.GetAddressOf()));

        // Temporary folder
        {
            ComPtr<ABI::Windows::Storage::IStorageFolder> folder;
            DX::ThrowIfFailed(appData->get_TemporaryFolder(folder.GetAddressOf()));

            ComPtr<ABI::Windows::Storage::IStorageItem> item;
            DX::ThrowIfFailed(folder.As(&item));

            HString folderName;
            DX::ThrowIfFailed(item->get_Path(folderName.GetAddressOf()));

            unsigned int len;
            PCWSTR szPath = folderName.GetRawBuffer(&len);
            if (wcscpy_s(sspath, MAX_PATH, szPath) > 0
                || wcscat_s(sspath, L"\\") > 0)
            {
                throw std::exception("TemporaryFolder");
            }
        }
    #else
        const wchar_t sspath[MAX_PATH] = L".";
    #endif

        OutputDebugStringA("Output path: ");
        OutputDebugStringW(sspath);
        OutputDebugStringA("\n");

        wchar_t sspng[MAX_PATH] = {};
        wchar_t ssjpg[MAX_PATH] = {};
        wchar_t ssbmp[MAX_PATH] = {};
        wchar_t sstif[MAX_PATH] = {};
        wchar_t ssdds[MAX_PATH] = {};
        wchar_t ssdds2[MAX_PATH] = {};

        swprintf_s(sspng, L"%ls\\SCREENSHOT.PNG", sspath);
        swprintf_s(ssjpg, L"%ls\\SCREENSHOT.JPG", sspath);
        swprintf_s(ssbmp, L"%ls\\SCREENSHOT.BMP", sspath);
        swprintf_s(sstif, L"%ls\\SCREENSHOT.TIF", sspath);
        swprintf_s(ssdds, L"%ls\\SCREENSHOT.DDS", sspath);
        swprintf_s(ssdds2, L"%ls\\SCREENSHOT2.DDS", sspath);

        DeleteFileW(sspng);
        DeleteFileW(ssjpg);
        DeleteFileW(ssbmp);
        DeleteFileW(sstif);
        DeleteFileW(ssdds);
        DeleteFileW(ssdds2);

        auto backBufferTex = m_msaaHelper4->GetMSAARenderTarget();

        // DirectX Tool Kit for DirectX 11 ScreenGrab
        DX::ThrowIfFailed(SaveWICTextureToFile(context, backBufferTex, GUID_ContainerFormatPng, sspng));

        if (GetFileAttributesW(sspng) != INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringA("Wrote SCREENSHOT.PNG\n");
        }
        else
        {
            OutputDebugStringA("ERROR: Missing SCREENSHOT.PNG!\n");
            success = false;
        }

        DX::ThrowIfFailed(SaveWICTextureToFile(context, backBufferTex, GUID_ContainerFormatJpeg, ssjpg));

        if (GetFileAttributesW(ssjpg) != INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringA("Wrote SCREENSHOT.JPG\n");
        }
        else
        {
            OutputDebugStringA("ERROR: Missing SCREENSHOT.JPG!\n");
            success = false;
        }

        DX::ThrowIfFailed(SaveWICTextureToFile(context, backBufferTex, GUID_ContainerFormatBmp, ssbmp,
            &GUID_WICPixelFormat16bppBGR565));

        if (GetFileAttributesW(ssbmp) != INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringA("Wrote SCREENSHOT.BMP\n");
        }
        else
        {
            OutputDebugStringA("ERROR: Missing SCREENSHOT.BMP!\n");
            success = false;
        }

        DX::ThrowIfFailed(SaveWICTextureToFile(context, backBufferTex, GUID_ContainerFormatTiff, sstif, nullptr,
            [&](IPropertyBag2* props)
            {
                PROPBAG2 options[2] = {};
                options[0].pstrName = const_cast<LPWSTR>(L"CompressionQuality");
                options[1].pstrName = const_cast<LPWSTR>(L"TiffCompressionMethod");

                VARIANT varValues[2];
                varValues[0].vt = VT_R4;
                varValues[0].fltVal = 0.75f;

                varValues[1].vt = VT_UI1;
                varValues[1].bVal = WICTiffCompressionNone;

                (void)props->Write(2, options, varValues);
            }));

        if (GetFileAttributesW(sstif) != INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringA("Wrote SCREENSHOT.TIF\n");
        }
        else
        {
            OutputDebugStringA("ERROR: Missing SCREENSHOT.TIF!\n");
            success = false;
        }

        DX::ThrowIfFailed(SaveDDSTextureToFile(context, backBufferTex, ssdds));

        if (GetFileAttributesW(ssdds) != INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringA("Wrote SCREENSHOT.DDS\n");
        }
        else
        {
            OutputDebugStringA("ERROR: Missing SCREENSHOT.DDS!\n");
            success = false;
        }

        OutputDebugStringA(success ? "Passed\n" : "Failed\n");
        OutputDebugStringA("******* MSAA SCREENSHOT TEST END ***********\n");
    }

    // Show the new frame.
#ifdef XBOX
    m_deviceResources->Present((m_state == State::NOMSAA) ? D3D11X_DECOMPRESS_PROPAGATE_COLOR_CLEAR : 0);

    m_graphicsMemory->Commit();
#else
    m_deviceResources->Present();
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

    switch (m_state)
    {
    case State::MSAA2X:
        renderTarget = m_msaaHelper2->GetMSAARenderTargetView();
        depthStencil = m_msaaHelper2->GetMSAADepthStencilView();
        color.v = c_clearColor2.v;
        break;

    case State::MSAA4X:
        renderTarget = m_msaaHelper4->GetMSAARenderTargetView();
        depthStencil = m_msaaHelper4->GetMSAADepthStencilView();
        color.v = c_clearColor4.v;
        break;

    case State::MSAA8X:
        renderTarget = m_msaaHelper8->GetMSAARenderTargetView();
        depthStencil = m_msaaHelper8->GetMSAADepthStencilView();
        color.v = c_clearColor8.v;
        break;

    default:
        color.v = c_clearColor.v;
        break;
    }

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

    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();

    m_delay = ADVANCE_TIME;
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
    auto context = m_deviceResources->GetD3DDeviceContext();

#ifdef XBOX
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device, m_deviceResources->GetBackBufferCount());
#endif

    // Set the MSAA device. Note this updates updates GetSampleCount.
    m_msaaHelper2->SetDevice(device);
    m_msaaHelper4->SetDevice(device);
    m_msaaHelper8->SetDevice(device);

    // Test scene objects
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, L"reftexture.dds", nullptr, m_texture.ReleaseAndGetAddressOf())
    );

    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetTextureEnabled(true);
    m_effect->SetTexture(m_texture.Get());

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.MultisampleEnable = TRUE;

    DX::ThrowIfFailed(
        device->CreateRasterizerState(&rsDesc, m_rstate.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<Vertex>(device, m_effect.get(), m_inputLayout.ReleaseAndGetAddressOf())
    );

    m_states = std::make_unique<CommonStates>(device);

    m_batch = std::make_unique<PrimitiveBatch<Vertex>>(context);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();

    // Set windows size for MSAA.
    m_msaaHelper2->SetWindow(size);
    m_msaaHelper4->SetWindow(size);
    m_msaaHelper8->SetWindow(size);
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_effect.reset();
    m_states.reset();
    m_batch.reset();

    m_inputLayout.Reset();
    m_texture.Reset();
    m_rstate.Reset();

    m_msaaHelper2->ReleaseDevice();
    m_msaaHelper4->ReleaseDevice();
    m_msaaHelper8->ReleaseDevice();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion
