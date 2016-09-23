//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK DDSTextureLoader & WICTextureLoader
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
#include "Game.h"

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#include <Windows.ApplicationModel.h>
#include <Windows.Storage.h>
#endif

#pragma warning(disable : 4238)

#define GAMMA_CORRECT_RENDERING
#define USE_FAST_SEMANTICS

// Build for LH vs. RH coords
//#define LH_COORDS

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    float dist = 10.f;

    bool ValidateDesc(
        ID3D11ShaderResourceView* srv,
        D3D_SRV_DIMENSION chkdim,
        DXGI_FORMAT chkfmt,
        UINT chkmip,
        UINT chkarray = 1)
    {
        if (!srv)
            return false;

        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        srv->GetDesc(&desc);

        if (desc.ViewDimension != chkdim
            || desc.Format != chkfmt)
            return false;

        switch (desc.ViewDimension)
        {
        case D3D_SRV_DIMENSION_TEXTURE1D:
            return (desc.Texture1D.MipLevels == chkmip);

        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
            return (desc.Texture1DArray.ArraySize == chkarray && desc.Texture1DArray.MipLevels == chkmip);

        case D3D_SRV_DIMENSION_TEXTURE2D:
            return (desc.Texture2D.MipLevels == chkmip);

        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
            return (desc.Texture2DArray.ArraySize == chkarray && desc.Texture2DArray.MipLevels == chkmip);

        case D3D_SRV_DIMENSION_TEXTURE3D:
            return (desc.Texture3D.MipLevels == chkmip);

        case D3D_SRV_DIMENSION_TEXTURECUBE:
            return (desc.TextureCube.MipLevels == chkmip);

        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
            return (desc.TextureCubeArray.NumCubes == (chkarray / 6) && desc.TextureCubeArray.MipLevels == chkmip);
        }

        return false;
    }

    bool ValidateDesc(
        ID3D11Resource* res,
        D3D11_RESOURCE_DIMENSION chkdim,
        DXGI_FORMAT chkfmt,
        UINT chkmip,
        UINT chkwidth,
        UINT chkheight = 1,
        UINT chkdepthOrArray = 1)
    {
        if (!res)
            return false;

        D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        res->GetType(&dim);

        if (dim != chkdim)
            return false;

        switch (dim)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            {
                ComPtr<ID3D11Texture1D> tex;
                DX::ThrowIfFailed(res->QueryInterface(IID_GRAPHICS_PPV_ARGS(tex.GetAddressOf())));

                D3D11_TEXTURE1D_DESC desc;
                tex->GetDesc(&desc);

                if (desc.ArraySize == chkdepthOrArray
                    && desc.Format == chkfmt
                    && desc.MipLevels == chkmip
                    && desc.Width == chkwidth)
                {
                    return true;
                }
            }
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            {
                ComPtr<ID3D11Texture2D> tex;
                DX::ThrowIfFailed(res->QueryInterface(IID_GRAPHICS_PPV_ARGS(tex.GetAddressOf())));

                D3D11_TEXTURE2D_DESC desc;
                tex->GetDesc(&desc);

                if (desc.ArraySize == chkdepthOrArray
                    && desc.Format == chkfmt
                    && desc.MipLevels == chkmip
                    && desc.Width == chkwidth
                    && desc.Height == chkheight)
                {
                    return true;
                }
            }
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            {
                ComPtr<ID3D11Texture3D> tex;
                DX::ThrowIfFailed(res->QueryInterface(IID_GRAPHICS_PPV_ARGS(tex.GetAddressOf())));

                D3D11_TEXTURE3D_DESC desc;
                tex->GetDesc(&desc);

                if (desc.Format == chkfmt
                    && desc.MipLevels == chkmip
                    && desc.Width == chkwidth
                    && desc.Height == chkheight
                    && desc.Depth == chkdepthOrArray)
                {
                    return true;
                }
            }
            break;
        }

        return false;
    }
};

Game::Game() :
    m_frame(0)
{
#if defined(_XBOX_ONE) && defined(_TITLE) && defined(USE_FAST_SEMANTICS)
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, 2, true);
#elif defined(GAMMA_CORRECT_RENDERING)
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>();
#endif

#if !defined(_XBOX_ONE) || !defined(_TITLE)
    m_deviceResources->RegisterDeviceNotify(this);
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
    HWND window,
#else
    IUnknown* window,
#endif
    int width, int height, DXGI_MODE_ROTATION rotation)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();

#if defined(_XBOX_ONE) && defined(_TITLE)
    UNREFERENCED_PARAMETER(rotation);
    UNREFERENCED_PARAMETER(width);
    UNREFERENCED_PARAMETER(height);
    m_deviceResources->SetWindow(window);
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
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
void Game::Update(DX::StepTimer const&)
{
    auto pad = m_gamePad->GetState(0);
    auto kb = m_keyboard->GetState();
    if (kb.Escape || (pad.IsConnected() && pad.IsViewPressed()))
    {
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
        PostQuitMessage(0);
#else
        Windows::ApplicationModel::Core::CoreApplication::Exit();
#endif
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

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources->Prepare();
#endif

    Clear();

    auto t = static_cast<float>(m_timer.GetTotalSeconds());

    // Cube 1
    XMMATRIX world = XMMatrixRotationY(t) * XMMatrixTranslation(1.5f, -2.1f, (dist / 2.f) + dist * sin(t));
    m_effect->SetWorld(world);
    m_effect->SetTexture(m_earth.Get());
    m_cube->Draw(m_effect.get(), m_layout.Get());

    // Cube 2
    world = XMMatrixRotationY(-t) * XMMatrixTranslation(1.5f, 0, (dist / 2.f) + dist * sin(t));
    m_effect->SetWorld(world);
    m_effect->SetTexture(m_win95_2.Get());
    m_cube->Draw(m_effect.get(), m_layout.Get());

    // Cube 3
    world = XMMatrixRotationY(t) * XMMatrixTranslation(1.5f, 2.1f, (dist / 2.f) + dist * sin(t));
    m_effect->SetWorld(world);
    m_effect->SetTexture(m_win95.Get());
    m_cube->Draw(m_effect.get(), m_layout.Get());

    // Cube 4
    world = XMMatrixRotationY(-t) * XMMatrixTranslation(-1.5f, -2.1f, (dist / 2.f) + dist * sin(t));
    m_effect->SetWorld(world);
    m_effect->SetTexture(m_earth2.Get());
    m_cube->Draw(m_effect.get(), m_layout.Get());

    // Cube 5
    world = XMMatrixRotationY(t) * XMMatrixTranslation(-1.5f, 0, (dist / 2.f) + dist * sin(t));
    m_effect->SetWorld(world);
    m_effect->SetTexture(m_dxlogo2.Get());
    m_cube->Draw(m_effect.get(), m_layout.Get());

    // Cube 6
    world = XMMatrixRotationY(-t) * XMMatrixTranslation(-1.5f, 2.1f, (dist / 2.f) + dist * sin(t));
    m_effect->SetWorld(world);
    m_effect->SetTexture(m_dxlogo.Get());
    m_cube->Draw(m_effect.get(), m_layout.Get());

    if (m_frame == 10)
    {
        OutputDebugStringA("******** SCREENSHOT TEST BEGIN *************\n");

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

        swprintf_s(sspng, L"%ls\\SCREENSHOT.PNG", sspath);
        swprintf_s(ssjpg, L"%ls\\SCREENSHOT.JPG", sspath);
        swprintf_s(ssbmp, L"%ls\\SCREENSHOT.BMP", sspath);
        swprintf_s(sstif, L"%ls\\SCREENSHOT.TIF", sspath);
        swprintf_s(ssdds, L"%ls\\SCREENSHOT.DDS", sspath);

        DeleteFileW(sspng);
        DeleteFileW(ssjpg);
        DeleteFileW(ssbmp);
        DeleteFileW(sstif);
        DeleteFileW(ssdds);

        auto context = m_deviceResources->GetD3DDeviceContext();
        auto backBufferTex = m_deviceResources->GetRenderTarget();

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
            PROPBAG2 options[2] = { 0, 0 };
            options[0].pstrName = L"CompressionQuality";
            options[1].pstrName = L"TiffCompressionMethod";

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

        // TODO - pass in staging texture with read access

        // ScreenGrab of an MSAA resource is tested elsewhere

        OutputDebugStringA(success ? "Passed\n" : "Failed\n");
        OutputDebugStringA("********* SCREENSHOT TEST END **************\n");
    }

    // Show the new frame.
    m_deviceResources->Present();

#if defined(_XBOX_ONE) && defined(_TITLE)
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
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
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
#endif

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
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_graphicsMemory = std::make_unique<GraphicsMemory>(device, m_deviceResources->GetBackBufferCount());
#endif

    auto context = m_deviceResources->GetD3DDeviceContext();

#ifdef LH_COORDS
    m_cube = GeometricPrimitive::CreateCube(context, 2.f, false);
#else
    m_cube = GeometricPrimitive::CreateCube(context, 2.f, true);
#endif

    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetTextureEnabled(true);

    m_cube->CreateInputLayout(m_effect.get(), m_layout.ReleaseAndGetAddressOf());

    // View textures
    bool success = true;
    OutputDebugStringA("*********** UINT TESTS BEGIN ***************\n");

    // Earth
    {
        DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;

        ComPtr<ID3D11Resource> res;
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"earth_A2B10G10R10.dds",
            res.GetAddressOf(), m_earth.ReleaseAndGetAddressOf(), 0, &alphaMode));

        if (alphaMode != DDS_ALPHA_MODE_UNKNOWN)
        {
            OutputDebugStringA("FAILED: earth_A2B10G10R10.dds alpha mode unexpected\n");
            success = false;
        }

        if (!ValidateDesc(m_earth.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R10G10B10A2_UNORM, 10))
        {
            OutputDebugStringA("FAILED: earth_A2B10G10R10.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R10G10B10A2_UNORM, 10, 512, 256))
        {
            OutputDebugStringA("FAILED: earth_A2B10G10R10.dds res desc unexpected\n");
            success = false;
        }
    }

    {
        DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;

        ComPtr<ID3D11Resource> res;
        DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"earth_A2B10G10R10.dds",
            0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
            res.GetAddressOf(), m_earth2.ReleaseAndGetAddressOf(), &alphaMode));

        if (alphaMode != DDS_ALPHA_MODE_UNKNOWN)
        {
            OutputDebugStringA("FAILED: earth_A2B10G10R10.dds (2) alpha mode unexpected\n");
            success = false;
        }

        // forceSRGB has no effect for 10:10:10:2

        if (!ValidateDesc(m_earth2.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R10G10B10A2_UNORM, 10))
        {
            OutputDebugStringA("FAILED: earth_A2B10G10R10.dds (2) srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R10G10B10A2_UNORM, 10, 512, 256))
        {
            OutputDebugStringA("FAILED: earth_A2B10G10R10.dds (2) res desc unexpected\n");
            success = false;
        }
    }

    // DirectX Logo
    {
        DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;

        ComPtr<ID3D11Resource> res;
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, context, L"dx5_logo_autogen.dds", 
            res.GetAddressOf(), m_dxlogo.ReleaseAndGetAddressOf(), 0, &alphaMode));

        if (alphaMode != DDS_ALPHA_MODE_UNKNOWN)
        {
            OutputDebugStringA("FAILED: dx5_logo_autogen.dds (autogen) alpha mode unexpected\n");
            success = false;
        }

        if (!ValidateDesc(m_dxlogo.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM, 9))
        {
            OutputDebugStringA("FAILED: dx5_logo_autogen.dds (autogen) srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM, 9, 256, 256))
        {
            OutputDebugStringA("FAILED: dx5_logo_autogen.dds (autogen) res desc unexpected\n");
            success = false;
        }
    }

    {
        DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;

        ComPtr<ID3D11Resource> res;
        DX::ThrowIfFailed(CreateDDSTextureFromFileEx(device, L"dx5_logo.dds",
            0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
            res.GetAddressOf(), m_dxlogo2.ReleaseAndGetAddressOf(), &alphaMode));

        if (alphaMode != DDS_ALPHA_MODE_UNKNOWN)
        {
            OutputDebugStringA("FAILED: dx5_logo.dds (BC1 sRGB) alpha mode unexpected\n");
            success = false;
        }

        if (!ValidateDesc(m_dxlogo2.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_BC1_UNORM_SRGB, 9))
        {
            OutputDebugStringA("FAILED: dx5_logo.dds (BC1 sRGB) srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_BC1_UNORM_SRGB, 9, 256, 256))
        {
            OutputDebugStringA("FAILED: dx5_logo.dds (BC1 sRGB) res desc unexpected\n");
            success = false;
        }
    }

    // Windows 95 logo
    {
        ComPtr<ID3D11Resource> res;
        DX::ThrowIfFailed(CreateWICTextureFromFile(device, context, L"win95.bmp",
            res.GetAddressOf(), m_win95.ReleaseAndGetAddressOf()));

        if (!ValidateDesc(m_win95.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 9))
        {
            OutputDebugStringA("FAILED: win95.bmp (autogen) srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 9, 256, 256))
        {
            OutputDebugStringA("FAILED: win95.bmp (autogen) res desc unexpected\n");
            success = false;
        }
    }

    {
        ComPtr<ID3D11Resource> res;
        DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, context, L"win95.bmp",
            0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true, res.GetAddressOf(), m_win95_2.ReleaseAndGetAddressOf()));

        if (!ValidateDesc(m_win95_2.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 9))
        {
            OutputDebugStringA("FAILED: win95.bmp (sRGB) srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 9, 256, 256))
        {
            OutputDebugStringA("FAILED: win95.bmp (sRGB) res desc unexpected\n");
            success = false;
        }
    }

    UnitTests(success);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    static const XMVECTORF32 eyePosition = { 0.0f, 3.0f, -6.0f, 0.0f };
    static const XMVECTORF32 At = { 0.0f, 1.0f, 0.0f, 0.0f };
    static const XMVECTORF32 Up = { 0.0f, 1.0f, 0.0f, 0.0f };

    auto size = m_deviceResources->GetOutputSize();
    float aspect = (float)size.right / (float)size.bottom;

#ifdef LH_COORDS
    XMMATRIX view = XMMatrixLookAtLH(eyePosition, At, Up);
    XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspect, 0.01f, 100.0f);
#else
    XMMATRIX view = XMMatrixLookAtRH(eyePosition, At, Up);
    XMMATRIX projection = XMMatrixPerspectiveFovRH(XM_PIDIV4, aspect, 0.01f, 100.0f);
#endif

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    XMMATRIX orient = XMLoadFloat4x4(&m_deviceResources->GetOrientationTransform3D());
    projection *= orient;
#endif

    m_effect->SetView(view);
    m_effect->SetProjection(projection);
}

#if !defined(_XBOX_ONE) || !defined(_TITLE)
void Game::OnDeviceLost()
{
    m_earth.Reset();
    m_earth2.Reset();
    m_dxlogo.Reset();
    m_dxlogo2.Reset();
    m_win95.Reset();
    m_win95_2.Reset();

    m_cube.reset();
    m_effect.reset();
    m_layout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#endif
#pragma endregion

void Game::UnitTests(bool success)
{
    auto device = m_deviceResources->GetD3DDevice();

    // DirectX Logo (verify DDS for autogen has no mipmaps)
    {
        DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;

        ComPtr<ID3D11Resource> res;
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"dx5_logo_autogen.dds",
            res.GetAddressOf(), nullptr, 0, &alphaMode));

        if (alphaMode != DDS_ALPHA_MODE_UNKNOWN)
        {
            OutputDebugStringA("FAILED: dx5_logo_autogen.dds (no autogen) alpha mode unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM, 1, 256, 256))
        {
            OutputDebugStringA("FAILED: dx5_logo_autogen.dds (no autogen) res desc unexpected\n");
            success = false;
        }
    }

    // DirectX Logo (verify DDS is BC1 without sRGB)
    {
        DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;

        ComPtr<ID3D11Resource> res;
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"dx5_logo.dds",
            res.GetAddressOf(), nullptr, 0, &alphaMode));

        if (alphaMode != DDS_ALPHA_MODE_UNKNOWN)
        {
            OutputDebugStringA("FAILED: dx5_logo.dds (BC1) alpha mode unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_BC1_UNORM, 9, 256, 256))
        {
            OutputDebugStringA("FAILED: dx5_logo.dds (BC1) res desc unexpected\n");
            success = false;
        }
    }

    // Windows 95 logo
    {
        ComPtr<ID3D11Resource> res;
        DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"win95.bmp",
            res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 256, 256))
        {
            OutputDebugStringA("FAILED: win95.bmp res desc unexpected\n");
            success = false;
        }
    }

    // Alpha mode test
    {
        DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;

        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"tree02S_pmalpha.dds",
            res.GetAddressOf(), tex.GetAddressOf(), 0, &alphaMode));

        if (alphaMode != DDS_ALPHA_MODE_PREMULTIPLIED)
        {
            OutputDebugStringA("FAILED: tree02S_pmalpha.dds alpha mode unexpected\n");
            success = false;
        }

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM, 9))
        {
            OutputDebugStringA("FAILED: tree02S_pmalpha.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM, 9, 304, 268))
        {
            OutputDebugStringA("FAILED: tree02S_pmalpha.dds res desc unexpected\n");
            success = false;
        }
    }

    // 1D texture
    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds",
            res.GetAddressOf(), tex.GetAddressOf()));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE1D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE1D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 32))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds res desc unexpected\n");
            success = false;
        }
    }

    // 1D texture array
    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds",
            res.GetAddressOf(), tex.GetAddressOf()));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE1DARRAY, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 6))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE1D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 32, 1, 6))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds res desc unexpected\n");
            success = false;
        }
    }

    // 2D texture array
    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds",
            res.GetAddressOf(), tex.GetAddressOf()));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE2DARRAY, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 6))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 32, 128, 6))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds res desc unexpected\n");
            success = false;
        }
    }

    // 3D texture
    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds",
            res.GetAddressOf(), tex.GetAddressOf()));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE3D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE3D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 32, 128, 32))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds res desc unexpected\n");
            success = false;
        }
    }

    // Autogen 1D texture
    auto context = m_deviceResources->GetD3DDeviceContext();

    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, context, L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds",
            res.GetAddressOf(), tex.GetAddressOf()));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE1D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 6))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE1D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 6, 32))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1D_MipOff.dds res desc unexpected\n");
            success = false;
        }
    }

    // Autogen 1D texture array
    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, context, L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds",
            res.GetAddressOf(), tex.GetAddressOf()));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE1DARRAY, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 6, 6))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE1D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 6, 32, 1, 6))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE1DArray_MipOff.dds res desc unexpected\n");
            success = false;
        }
    }

    // Autogen 2D texture array
    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, context, L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds",
            res.GetAddressOf(), tex.GetAddressOf()));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE2DARRAY, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 8, 6))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 8, 32, 128, 6))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE2DArray_MipOff.dds res desc unexpected\n");
            success = false;
        }
    }

    // Autogen 3D texture
    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, context, L"io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds",
            res.GetAddressOf(), tex.GetAddressOf()));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE3D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 8))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE3D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 8, 32, 128, 32))
        {
            OutputDebugStringA("FAILED: io_R8G8B8A8_UNORM_SRGB_SRV_DIMENSION_TEXTURE3D_MipOff.dds res desc unexpected\n");
            success = false;
        }
    }

    // sRGB test
    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"cup_small.jpg",
            res.GetAddressOf(), tex.GetAddressOf(), 0));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1))
        {
            OutputDebugStringA("FAILED: cup_small.jpg srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 512, 683))
        {
            OutputDebugStringA("FAILED: cup_small.jpg res desc unexpected\n");
            success = false;
        }
    }

    {
        ComPtr<ID3D11Resource> res;
        ComPtr<ID3D11ShaderResourceView> tex;

        DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, L"cup_small.jpg",
            0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
            WIC_LOADER_IGNORE_SRGB, res.GetAddressOf(), tex.GetAddressOf()));

        if (!ValidateDesc(tex.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 1))
        {
            OutputDebugStringA("FAILED: cup_small.jpg (ignore srgb) srv desc unexpected\n");
            success = false;
        }

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 512, 683))
        {
            OutputDebugStringA("FAILED: cup_small.jpg (ignore srgb) res desc unexpected\n");
            success = false;
        }
    }

    OutputDebugStringA(success ? "Passed\n" : "Failed\n");
    OutputDebugStringA("***********  UNIT TESTS END  ***************\n");
}
