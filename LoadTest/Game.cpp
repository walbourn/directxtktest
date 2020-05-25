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

#include "ReadData.h"

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#include <Windows.ApplicationModel.h>
#include <Windows.Storage.h>
#endif

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

Game::Game() noexcept(false) :
    m_frame(0)
{
#ifdef GAMMA_CORRECT_RENDERING
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D32_FLOAT, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_D24_UNORM_S8_UINT, 2, D3D_FEATURE_LEVEL_9_3,
        DX::DeviceResources::c_Enable4K_Xbox
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(c_RenderFormat);
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
    m_keyboard->SetWindow(reinterpret_cast<ABI::Windows::UI::Core::ICoreWindow*>(window));
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

#if defined(_XBOX_ONE) && defined(_TITLE)
    UINT decompressFlags = D3D11X_DECOMPRESS_PROPAGATE_COLOR_CLEAR;
#endif

    if (m_frame == 10)
    {
        OutputDebugStringA("******** SCREENSHOT TEST BEGIN *************\n");

        bool success = true;

        auto context = m_deviceResources->GetD3DDeviceContext();
        auto backBufferTex = m_deviceResources->GetRenderTarget();

#if defined(_XBOX_ONE) && defined(_TITLE)
        const wchar_t sspath[MAX_PATH] = L"T:\\";

#ifdef USE_FAST_SEMANTICS
        context->DecompressResource(
            backBufferTex, 0, nullptr,
            backBufferTex, 0, nullptr,
            m_deviceResources->GetBackBufferFormat(), decompressFlags);
#endif

        decompressFlags = 0;

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

#ifdef GAMMA_CORRECT_RENDERING
        HRESULT hr = SaveWICTextureToFile(context, backBufferTex, GUID_ContainerFormatPng, sspng, &GUID_WICPixelFormat32bppBGRA);
#else
        HRESULT hr = SaveWICTextureToFile(context, backBufferTex, GUID_ContainerFormatPng, sspng, &GUID_WICPixelFormat32bppBGRA, nullptr, true);
#endif

        if (FAILED(hr))
        {
            char buff[128] = {};
            sprintf_s(buff, "ERROR: SaveWICTextureToFile (PNG) failed %08X\n", static_cast<unsigned int>(hr));
            OutputDebugStringA(buff);
            success = false;
        }
        else if (GetFileAttributesW(sspng) != INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringA("Wrote SCREENSHOT.PNG\n");
        }
        else
        {
            OutputDebugStringA("ERROR: Missing SCREENSHOT.PNG!\n");
            success = false;
        }

        hr = SaveWICTextureToFile(context, backBufferTex, GUID_ContainerFormatJpeg, ssjpg);

        if (FAILED(hr))
        {
            char buff[128] = {};
            sprintf_s(buff, "ERROR: SaveWICTextureToFile (JPG) failed %08X\n", static_cast<unsigned int>(hr));
            OutputDebugStringA(buff);
            success = false;
        }
        else if (GetFileAttributesW(ssjpg) != INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringA("Wrote SCREENSHOT.JPG\n");
        }
        else
        {
            OutputDebugStringA("ERROR: Missing SCREENSHOT.JPG!\n");
            success = false;
        }

        hr = SaveWICTextureToFile(context, backBufferTex, GUID_ContainerFormatBmp, ssbmp,
            &GUID_WICPixelFormat16bppBGR565);

        if (FAILED(hr))
        {
            char buff[128] = {};
            sprintf_s(buff, "ERROR: SaveWICTextureToFile (BMP) failed %08X\n", static_cast<unsigned int>(hr));
            OutputDebugStringA(buff);
            success = false;
        }
        else if (GetFileAttributesW(ssbmp) != INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringA("Wrote SCREENSHOT.BMP\n");
        }
        else
        {
            OutputDebugStringA("ERROR: Missing SCREENSHOT.BMP!\n");
            success = false;
        }

        hr = SaveWICTextureToFile(context, backBufferTex, GUID_ContainerFormatTiff, sstif, nullptr,
            [&](IPropertyBag2* props)
        {
            PROPBAG2 options[2] = { 0, 0 };
            options[0].pstrName = const_cast<wchar_t*>(L"CompressionQuality");
            options[1].pstrName = const_cast<wchar_t*>(L"TiffCompressionMethod");

            VARIANT varValues[2];
            varValues[0].vt = VT_R4;
            varValues[0].fltVal = 0.75f;

            varValues[1].vt = VT_UI1;
            varValues[1].bVal = WICTiffCompressionNone;

            (void)props->Write(2, options, varValues);
        });

        if (FAILED(hr))
        {
            char buff[128] = {};
            sprintf_s(buff, "ERROR: SaveWICTextureToFile (TIFF) failed %08X\n", static_cast<unsigned int>(hr));
            OutputDebugStringA(buff);
            success = false;
        }
        else if (GetFileAttributesW(sstif) != INVALID_FILE_ATTRIBUTES)
        {
            OutputDebugStringA("Wrote SCREENSHOT.TIF\n");
        }
        else
        {
            OutputDebugStringA("ERROR: Missing SCREENSHOT.TIF!\n");
            success = false;
        }

        hr = SaveDDSTextureToFile(context, backBufferTex, ssdds);

        if (FAILED(hr))
        {
            char buff[128] = {};
            sprintf_s(buff, "ERROR: SaveWICTextureToFile (DDS) failed %08X\n", static_cast<unsigned int>(hr));
            OutputDebugStringA(buff);
            success = false;
        }
        else if (GetFileAttributesW(ssdds) != INVALID_FILE_ATTRIBUTES)
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
#if defined(_XBOX_ONE) && defined(_TITLE)
    m_deviceResources->Present(decompressFlags);
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
#if defined(_XBOX_ONE) && defined(_TITLE)
    auto context = m_deviceResources->GetD3DDeviceContext();
    context->Suspend(0);
#endif
}

void Game::OnResuming()
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    auto context = m_deviceResources->GetD3DDeviceContext();
    context->Resume();
#endif

    m_timer.ResetElapsedTime();
}

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}
#endif

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

    {
        auto blob = DX::ReadData(L"earth_A2B10G10R10.dds");

        {
            DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;

            ComPtr<ID3D11Resource> res;
            ComPtr<ID3D11ShaderResourceView> srv;
            DX::ThrowIfFailed(CreateDDSTextureFromMemory(device, blob.data(), blob.size(),
                res.GetAddressOf(), srv.GetAddressOf(), 0, &alphaMode));

            if (alphaMode != DDS_ALPHA_MODE_UNKNOWN)
            {
                OutputDebugStringA("FAILED: earth_A2B10G10R10.dds (mem) alpha mode unexpected\n");
                success = false;
            }

            if (!ValidateDesc(srv.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R10G10B10A2_UNORM, 10))
            {
                OutputDebugStringA("FAILED: earth_A2B10G10R10.dds (mem) srv desc unexpected\n");
                success = false;
            }

            if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R10G10B10A2_UNORM, 10, 512, 256))
            {
                OutputDebugStringA("FAILED: earth_A2B10G10R10.dds (mem) res desc unexpected\n");
                success = false;
            }
        }

        {
            DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;

            ComPtr<ID3D11Resource> res;
            ComPtr<ID3D11ShaderResourceView> srv;
            DX::ThrowIfFailed(CreateDDSTextureFromMemoryEx(device, blob.data(), blob.size(),
                0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, true,
                res.GetAddressOf(), srv.ReleaseAndGetAddressOf(), &alphaMode));

            if (alphaMode != DDS_ALPHA_MODE_UNKNOWN)
            {
                OutputDebugStringA("FAILED: earth_A2B10G10R10.dds (mem 2) alpha mode unexpected\n");
                success = false;
            }

            // forceSRGB has no effect for 10:10:10:2

            if (!ValidateDesc(srv.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R10G10B10A2_UNORM, 10))
            {
                OutputDebugStringA("FAILED: earth_A2B10G10R10.dds (mem 2) srv desc unexpected\n");
                success = false;
            }

            if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R10G10B10A2_UNORM, 10, 512, 256))
            {
                OutputDebugStringA("FAILED: earth_A2B10G10R10.dds (mem 2) res desc unexpected\n");
                success = false;
            }
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

    {
        auto blob = DX::ReadData(L"win95.bmp");

        {
            ComPtr<ID3D11Resource> res;
            ComPtr<ID3D11ShaderResourceView> srv;
            DX::ThrowIfFailed(CreateWICTextureFromMemory(device, context, blob.data(), blob.size(),
                res.GetAddressOf(), srv.ReleaseAndGetAddressOf()));

            if (!ValidateDesc(srv.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 9))
            {
                OutputDebugStringA("FAILED: win95.bmp (mem autogen) srv desc unexpected\n");
                success = false;
            }

            if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 9, 256, 256))
            {
                OutputDebugStringA("FAILED: win95.bmp (mem autogen) res desc unexpected\n");
                success = false;
            }
        }

        {
            ComPtr<ID3D11Resource> res;
            ComPtr<ID3D11ShaderResourceView> srv;
            DX::ThrowIfFailed(CreateWICTextureFromMemoryEx(device, context, blob.data(), blob.size(),
                0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true, res.GetAddressOf(), srv.ReleaseAndGetAddressOf()));

            if (!ValidateDesc(srv.Get(), D3D_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 9))
            {
                OutputDebugStringA("FAILED: win95.bmp (mem sRGB) srv desc unexpected\n");
                success = false;
            }

            if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 9, 256, 256))
            {
                OutputDebugStringA("FAILED: win95.bmp (mem sRGB) res desc unexpected\n");
                success = false;
            }
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
    {
        auto orient3d = m_deviceResources->GetOrientationTransform3D();
        XMMATRIX orient = XMLoadFloat4x4(&orient3d);
        projection *= orient;
    }
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
    auto context = m_deviceResources->GetD3DDeviceContext();

    //----------------------------------------------------------------------------------
    // CreateStaticTexture 1D
    {
        static const uint32_t s_pixels[4] = { 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff };

        ComPtr<ID3D11Texture1D> res;

        D3D11_SUBRESOURCE_DATA initData = { s_pixels, 0, 0 };

        DX::ThrowIfFailed(CreateStaticTexture(device, 4u, DXGI_FORMAT_B8G8R8A8_UNORM, initData,
            res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE1D, DXGI_FORMAT_B8G8R8A8_UNORM, 1, 4))
        {
            OutputDebugStringA("FAILED: CreateStaticTexture 1D res desc unexpected\n");
            success = false;
        }
    }

    // CreateStaticTexture 2D
    {
        static const uint32_t s_pixels[16] = {
            0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff,
            0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff,
        };

        ComPtr<ID3D11Texture2D> res;

        D3D11_SUBRESOURCE_DATA initData = { s_pixels, sizeof(uint32_t) * 8, 0 };

        DX::ThrowIfFailed(CreateStaticTexture(device, 8u, 2u, DXGI_FORMAT_B8G8R8A8_UNORM, initData,
            res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM, 1, 8, 2))
        {
            OutputDebugStringA("FAILED: CreateStaticTexture 2D res desc unexpected\n");
            success = false;
        }

        initData = { s_pixels, sizeof(uint32_t) * 4, 0 };

        DX::ThrowIfFailed(CreateStaticTexture(device, 4u, 4u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            res.ReleaseAndGetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 4, 4))
        {
            OutputDebugStringA("FAILED: CreateStaticTexture 2Db res desc unexpected\n");
            success = false;
        }
    }

    // CreateStaticTexture 2D (autogen)
    {
        auto pixels = std::make_unique<uint32_t[]>(256 * 256 * sizeof(uint32_t));
        memset(pixels.get(), 0xff, 256 * 256 * sizeof(uint32_t));

        ComPtr<ID3D11Texture2D> res;

        D3D11_SUBRESOURCE_DATA initData = { pixels.get(), sizeof(uint32_t) * 256, 0 };

        ComPtr<ID3D11ShaderResourceView> srv;
        DX::ThrowIfFailed(CreateStaticTexture(device, context, 256u, 256u, DXGI_FORMAT_B8G8R8A8_UNORM, initData,
            res.GetAddressOf(), srv.GetAddressOf()));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM, 9, 256, 256))
        {
            OutputDebugStringA("FAILED: CreateStaticTexture 2D autogen res desc unexpected\n");
            success = false;
        }
    }

    // CreateStaticTexture 3D
    {
        static const uint32_t s_pixels[16] = {
            0xff0000ff, 0xff0000ff,
            0xff0000ff, 0xff0000ff,
            0xff00ff00, 0xff00ff00,
            0xff00ff00, 0xff00ff00,
            0xffff0000, 0xffff0000,
            0xffff0000, 0xffff0000,
            0xffffffff, 0xffffffff,
            0xffffffff, 0xffffffff,
        };

        ComPtr<ID3D11Texture3D> res;

        D3D11_SUBRESOURCE_DATA initData = { s_pixels, sizeof(uint32_t) * 2, sizeof(uint32_t) * 4 };

        DX::ThrowIfFailed(CreateStaticTexture(device, 2u, 2u, 4, DXGI_FORMAT_B8G8R8A8_UNORM, initData,
            res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE3D, DXGI_FORMAT_B8G8R8A8_UNORM, 1, 2, 2, 4))
        {
            OutputDebugStringA("FAILED: CreateStaticTexture 3D res desc unexpected\n");
            success = false;
        }

        initData = { s_pixels, sizeof(uint32_t) * 4, sizeof(uint32_t) * 8 };

        DX::ThrowIfFailed(CreateStaticTexture(device, 4u, 2u, 2u, DXGI_FORMAT_R8G8B8A8_UNORM, initData,
            res.ReleaseAndGetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE3D, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 4, 2, 2))
        {
            OutputDebugStringA("FAILED: CreateStaticTexture 2Db res desc unexpected\n");
            success = false;
        }
    }

    //----------------------------------------------------------------------------------
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

    // Video textures
    {
        ComPtr<ID3D11Resource> res;

        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, L"lenaNV12.dds", res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_NV12, 1, 200, 200))
        {
            OutputDebugStringA("FAILED: lenaNV12.dds res desc unexpected\n");
            success = false;
        }
    }

    // WIC load without format conversion or resize
    {
        ComPtr<ID3D11Resource> res;

        DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"testpattern.png", res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, 1, 1280, 1024))
        {
            OutputDebugStringA("FAILED: testpattern.png res desc unexpected\n");
            success = false;
        }
    }

    // WIC load with resize
    {
        ComPtr<ID3D11Resource> res;

        DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"testpattern.png", res.GetAddressOf(), nullptr, 1024));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, 1, 1024, 819))
        {
            OutputDebugStringA("FAILED: testpattern.png resize res desc unexpected\n");
            success = false;
        }
    }

    // WIC load with resize and format conversion
    {
        ComPtr<ID3D11Resource> res;

        DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"cup_small.jpg", res.GetAddressOf(), nullptr, 256));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 191, 256))
        {
            OutputDebugStringA("FAILED: cup_small.jpg resize res desc unexpected\n");
            success = false;
        }
    }

    // WIC force RGBA32
    {
        ComPtr<ID3D11Resource> res;

        DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"pentagon.tiff", res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8_UNORM, 1, 1024, 1024))
        {
            OutputDebugStringA("FAILED: pentagon.tiff res desc unexpected\n");
            success = false;
        }

        ComPtr<ID3D11Resource> res2;

        DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, L"pentagon.tiff",
            0,
            D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,0, 0,
            WIC_LOADER_FORCE_RGBA32,
            res2.GetAddressOf(), nullptr));

        if (!ValidateDesc(res2.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1024, 1024))
        {
            OutputDebugStringA("FAILED: pentagon.tiff res desc rgba32 unexpected\n");
            success = false;
        }
    }

    // WIC SQUARE flags
    {
        ComPtr<ID3D11Resource> res;

        DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, L"cup_small.jpg",
            0,
            D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
            WIC_LOADER_MAKE_SQUARE,
            res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 683, 683))
        {
            OutputDebugStringA("FAILED: cup_small.jpg square res desc unexpected\n");
            success = false;
        }
    }

    // WIC POW2 flags
    {
        ComPtr<ID3D11Resource> res;

        DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, L"cup_small.jpg",
            0,
            D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
            WIC_LOADER_FIT_POW2,
            res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 256, 512))
        {
            OutputDebugStringA("FAILED: cup_small.jpg pow2 res desc unexpected\n");
            success = false;
        }
    }

    // WIC POW2 + SQUARE flags
    {
        ComPtr<ID3D11Resource> res;

        DX::ThrowIfFailed(CreateWICTextureFromFileEx(device, L"cup_small.jpg",
            0,
            D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
            WIC_LOADER_FIT_POW2 | WIC_LOADER_MAKE_SQUARE,
            res.GetAddressOf(), nullptr));

        if (!ValidateDesc(res.Get(), D3D11_RESOURCE_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 512, 512))
        {
            OutputDebugStringA("FAILED: cup_small.jpg pow2+square res desc unexpected\n");
            success = false;
        }
    }

    OutputDebugStringA(success ? "Passed\n" : "Failed\n");
    OutputDebugStringA("***********  UNIT TESTS END  ***************\n");
}
