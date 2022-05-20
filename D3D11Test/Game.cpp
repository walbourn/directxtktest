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
// As of DirectXMath 3.13, these types are is_nothrow_copy/move_assignable

// VertexPosition
static_assert(std::is_nothrow_default_constructible<VertexPosition>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPosition>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPosition>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPosition>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPosition>::value, "Move Assign.");

// VertexPositionColor
static_assert(std::is_nothrow_default_constructible<VertexPositionColor>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionColor>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionColor>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionColor>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionColor>::value, "Move Assign.");

// VertexPositionTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionTexture>::value, "Move Assign.");

// VertexPositionDualTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionDualTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionDualTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionDualTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionDualTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionDualTexture>::value, "Move Assign.");

// VertexPositionNormal
static_assert(std::is_nothrow_default_constructible<VertexPositionNormal>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormal>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormal>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormal>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormal>::value, "Move Assign.");

// VertexPositionColorTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionColorTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionColorTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionColorTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionColorTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionColorTexture>::value, "Move Assign.");

// VertexPositionNormalColor
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalColor>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalColor>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalColor>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalColor>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalColor>::value, "Move Assign.");

// VertexPositionNormalTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalTexture>::value, "Move Assign.");

// VertexPositionNormalColorTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalColorTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalColorTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalColorTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalColorTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalColorTexture>::value, "Move Assign.");

// VertexPositionNormalTangentColorTexture
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalTangentColorTexture>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalTangentColorTexture>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalTangentColorTexture>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalTangentColorTexture>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalTangentColorTexture>::value, "Move Assign.");

// VertexPositionNormalTangentColorTextureSkinning
static_assert(std::is_nothrow_default_constructible<VertexPositionNormalTangentColorTextureSkinning>::value, "Default Ctor.");
static_assert(std::is_nothrow_copy_constructible<VertexPositionNormalTangentColorTextureSkinning>::value, "Copy Ctor.");
static_assert(std::is_copy_assignable<VertexPositionNormalTangentColorTextureSkinning>::value, "Copy Assign.");
static_assert(std::is_nothrow_move_constructible<VertexPositionNormalTangentColorTextureSkinning>::value, "Move Ctor.");
static_assert(std::is_move_assignable<VertexPositionNormalTangentColorTextureSkinning>::value, "Move Assign.");

//--------------------------------------------------------------------------------------

static_assert(std::is_nothrow_move_constructible<CommonStates>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<CommonStates>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<GraphicsMemory>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<GraphicsMemory>::value, "Move Assign.");

static_assert(std::is_nothrow_move_constructible<PrimitiveBatch<VertexPositionColor>>::value, "Move Ctor.");
static_assert(std::is_nothrow_move_assignable<PrimitiveBatch<VertexPositionColor>>::value, "Move Assign.");

#pragma warning(disable : 4061)

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
#if defined(NTDDI_WIN10_FE)
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
        VertexPositionColor points[]
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
        VertexPositionColor lines[] =
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
        VertexPositionColor v1(Vector3(0.f, 0.5f, 0.5f), red);
        VertexPositionColor v2(Vector3(0.5f, -0.5f, 0.5f), green);
        VertexPositionColor v3(Vector3(-0.5f, -0.5f, 0.5f), blue);

        m_batch->DrawTriangle(v1, v2, v3);
    }

    // Quads
    {
        VertexPositionColor quad[] =
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

    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetVertexColorEnabled(true);

    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<VertexPositionColor>(device, m_effect.get(), m_inputLayout.ReleaseAndGetAddressOf())
    );

    m_states = std::make_unique<CommonStates>(device);

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);
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

namespace
{
    inline bool CheckIsPowerOf2(size_t x) noexcept
    {
        if (!x)
            return false;

        return (ceil(log2(x)) == float(log2(x)));
    }

    inline uint64_t CheckAlignUp(uint64_t size, size_t alignment) noexcept
    {
        return ((size + alignment - 1) / alignment) * alignment;
    }

    inline uint64_t CheckAlignDown(uint64_t size, size_t alignment) noexcept
    {
        return (size / alignment) * alignment;
    }
}

template<class T>
inline bool TestVertexType(_In_ ID3D11Device *device, _In_ IEffect* effect)
{
    static_assert(T::InputElementCount > 0, "element count must be non-zero");
    static_assert(T::InputElementCount <= 32 /* D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT */, "element count is too large");

    if (_stricmp(T::InputElements[0].SemanticName, "SV_Position") != 0)
    {
        return false;
    }

    for (size_t j = 0; j < T::InputElementCount; ++j)
    {
        if (T::InputElements[j].SemanticName == nullptr)
            return false;
    }

    ComPtr<ID3D11InputLayout> inputLayout;
    HRESULT hr = CreateInputLayoutFromEffect<T>(device, effect, inputLayout.GetAddressOf());
    if (FAILED(hr))
        return false;

    return true;
}

void Game::UnitTests()
{
    bool success = true;
    OutputDebugStringA("*********** UINT TESTS BEGIN ***************\n");

    std::random_device rd;
    std::default_random_engine generator(rd());

    {
        for (size_t j = 0; j < 0x20000; ++j)
        {
            if (IsPowerOf2(j) != CheckIsPowerOf2(j))
            {
                OutputDebugStringA("ERROR: Failed IsPowerOf2 tests\n");
                success = false;
            }
        }
    }

    // uint32_t
    {
        std::uniform_int_distribution<uint32_t> dist(1, UINT16_MAX);
        for (size_t j = 1; j < 0x20000; j <<= 1)
        {
            if (!IsPowerOf2(j))
            {
                OutputDebugStringA("ERROR: Failed IsPowerOf2 Align(32)\n");
                success = false;
            }

            for (size_t k = 0; k < 20000; k++)
            {
                uint32_t value = dist(generator);
                uint32_t up = AlignUp(value, j);
                uint32_t down = AlignDown(value, j);
                auto upCheck = static_cast<uint32_t>(CheckAlignUp(value, j));
                auto downCheck = static_cast<uint32_t>(CheckAlignDown(value, j));

                if (!up)
                {
                    OutputDebugStringA("ERROR: Failed AlignUp(32) tests\n");
                    success = false;
                }
                else if (!down && value > j)
                {
                    OutputDebugStringA("ERROR: Failed AlignDown(32) tests\n");
                    success = false;
                }
                else if (up < down)
                {
                    OutputDebugStringA("ERROR: Failed Align(32) tests\n");
                    success = false;
                }
                else if (value > up)
                {
                    OutputDebugStringA("ERROR: Failed AlignUp(32) tests\n");
                    success = false;
                }
                else if (value < down)
                {
                    OutputDebugStringA("ERROR: Failed AlignDown(32) tests\n");
                    success = false;
                }
                else if (up != upCheck)
                {
                    OutputDebugStringA("ERROR: Failed AlignUp(32) tests\n");
                    success = false;
                }
                else if (down != downCheck)
                {
                    OutputDebugStringA("ERROR: Failed AlignDown(32) tests\n");
                    success = false;
                }
            }
        }
    }

    // uint64_t
    {
        std::uniform_int_distribution<uint64_t> dist(1, UINT32_MAX);
        for (size_t j = 1; j < 0x20000; j <<= 1)
        {
            if (!IsPowerOf2(j))
            {
                OutputDebugStringA("ERROR: Failed IsPowerOf2 Align(64)\n");
                success = false;
            }

            for (size_t k = 0; k < 20000; k++)
            {
                uint64_t value = dist(generator);
                uint64_t up = AlignUp(value, j);
                uint64_t down = AlignDown(value, j);
                uint64_t upCheck = CheckAlignUp(value, j);
                uint64_t downCheck = CheckAlignDown(value, j);

                if (!up)
                {
                    OutputDebugStringA("ERROR: Failed AlignUp(64) tests\n");
                    success = false;
                }
                else if (!down && value > j)
                {
                    OutputDebugStringA("ERROR: Failed AlignDown(64) tests\n");
                    success = false;
                }
                else if (up < down)
                {
                    OutputDebugStringA("ERROR: Failed Align(64) tests\n");
                    success = false;
                }
                else if (value > up)
                {
                    OutputDebugStringA("ERROR: Failed AlignUp(64) tests\n");
                    success = false;
                }
                else if (value < down)
                {
                    OutputDebugStringA("ERROR: Failed AlignDown(64) tests\n");
                    success = false;
                }
                else if (up != upCheck)
                {
                    OutputDebugStringA("ERROR: Failed AlignUp(64) tests\n");
                    success = false;
                }
                else if (down != downCheck)
                {
                    OutputDebugStringA("ERROR: Failed AlignDown(64) tests\n");
                    success = false;
                }
            }
        }
    }

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

    auto device = m_deviceResources->GetD3DDevice();

    // CreateStaticBuffer (BufferHelpers.h)
    {
        static const VertexPositionColor s_vertexData[3] =
        {
            { XMFLOAT3{ 0.0f,   0.5f,  0.5f }, XMFLOAT4{ 1.0f, 0.0f, 0.0f, 1.0f } },  // Top / Red
            { XMFLOAT3{ 0.5f,  -0.5f,  0.5f }, XMFLOAT4{ 0.0f, 1.0f, 0.0f, 1.0f } },  // Right / Green
            { XMFLOAT3{ -0.5f, -0.5f,  0.5f }, XMFLOAT4{ 0.0f, 0.0f, 1.0f, 1.0f } }   // Left / Blue
        };

        ComPtr<ID3D11Buffer> vb;
        if (FAILED(CreateStaticBuffer(device, s_vertexData, std::size(s_vertexData), sizeof(VertexPositionColor),
            D3D11_BIND_VERTEX_BUFFER, vb.GetAddressOf())))
        {
            OutputDebugStringA("ERROR: Failed CreateStaticBuffer(1) test\n");
            success = false;
        }

        ComPtr<ID3D11Buffer> vb2;
        if (FAILED(CreateStaticBuffer(device, s_vertexData, std::size(s_vertexData),
            D3D11_BIND_VERTEX_BUFFER, vb2.GetAddressOf())))
        {
            OutputDebugStringA("ERROR: Failed CreateStaticBuffer(2) test\n");
            success = false;
        }

        ComPtr<ID3D11Buffer> vb3;
        std::vector<VertexPositionColor> verts(s_vertexData, s_vertexData + std::size(s_vertexData));

        if (FAILED(CreateStaticBuffer(device, verts, D3D11_BIND_VERTEX_BUFFER, vb3.GetAddressOf())))
        {
            OutputDebugStringA("ERROR: Failed CreateStaticBuffer(3) test\n");
            success = false;
        }

        if (m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
        {
            ComPtr<ID3D11Buffer> vb4;
            if (FAILED(CreateStaticBuffer(device, verts, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS,
                vb4.GetAddressOf())))
            {
                OutputDebugStringA("ERROR: Failed CreateStaticBuffer(4) test\n");
                success = false;
            }
        }
    }

    // CreateInputLayoutFromEffect (BufferHelpers.h)
    {
        static const D3D11_INPUT_ELEMENT_DESC s_inputElementDesc[2] =
        {
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA,  0 },
            { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA , 0 },
        };

        ComPtr<ID3D11InputLayout> il;
        if (FAILED(CreateInputLayoutFromEffect(device, m_effect.get(), s_inputElementDesc, std::size(s_inputElementDesc), il.GetAddressOf())))
        {
            OutputDebugStringA("ERROR: Failed CreateInputLayoutFromEffect(1) test\n");
            success = false;
        }

        ComPtr<ID3D11InputLayout> il2;
        if (FAILED(CreateInputLayoutFromEffect<VertexPositionColor>(device, m_effect.get(), il2.GetAddressOf())))
        {
            OutputDebugStringA("ERROR: Failed CreateInputLayoutFromEffect(2) test\n");
            success = false;
        }
    }

    // CommonStates.h
    if (m_states->Opaque() == nullptr
        || m_states->AlphaBlend() == nullptr
        || m_states->Additive() == nullptr
        || m_states->NonPremultiplied() == nullptr)
    {
        OutputDebugStringA("ERROR: Failed CommonStates blend state tests\n");
        success = false;
    }

    if (m_states->DepthNone() == nullptr
        || m_states->DepthDefault() == nullptr
        || m_states->DepthRead() == nullptr
        || m_states->DepthReverseZ() == nullptr
        || m_states->DepthReadReverseZ() == nullptr)
    {
        OutputDebugStringA("ERROR: Failed CommonStates depth/stencil state tests\n");
        success = false;
    }

    if (m_states->CullNone() == nullptr
        || m_states->CullClockwise() == nullptr
        || m_states->CullCounterClockwise() == nullptr
        || m_states->Wireframe() == nullptr)
    {
        OutputDebugStringA("ERROR: Failed CommonStates rasterizer state tests\n");
        success = false;
    }

    if (m_states->PointWrap() == nullptr
        || m_states->PointClamp() == nullptr
        || m_states->LinearWrap() == nullptr
        || m_states->LinearClamp() == nullptr
        || m_states->AnisotropicWrap() == nullptr
        || m_states->AnisotropicClamp() == nullptr)
    {
        OutputDebugStringA("ERROR: Failed CommonStates sampler state tests\n");
        success = false;
    }

    // VertexTypes.h
    {
        auto effect = std::make_unique<BasicEffect>(device);

        if (!TestVertexType<VertexPosition>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPosition tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionColor>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionColor tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionTexture>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionTexture tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionDualTexture>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionDualTexture tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionNormal>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionNormal tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionColorTexture>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionColorTexture tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionNormalColor>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionNormalColor tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionNormalTexture>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionNormalTexture tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionNormalColorTexture>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionNormalColorTexture tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionNormalTangentColorTexture>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionNormalTangentColorTexture tests\n");
            success = false;
        }

        if (!TestVertexType<VertexPositionNormalTangentColorTextureSkinning>(device, effect.get()))
        {
            OutputDebugStringA("ERROR: Failed VertexPositionNormalTangentColorTextureSkinning tests\n");
            success = false;
        }
    }

    OutputDebugStringA(success ? "Passed\n" : "Failed\n");
    OutputDebugStringA("***********  UNIT TESTS END  ***************\n");

    if (!success)
    {
        throw std::runtime_error("Unit Tests Failed");
    }
}
