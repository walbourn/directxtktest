//--------------------------------------------------------------------------------------
// File: Game.cpp
//
// Developer unit test for DirectXTK SpriteFont
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"

#define GAMMA_CORRECT_RENDERING
#define USE_FAST_SEMANTICS

namespace
{
    const float SWAP_TIME = 3.f;

    const float EPSILON = 0.000001f;
}

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false) :
    m_frame(0),
    m_showUTF8(false),
    m_delay(0)
{
#ifdef GAMMA_CORRECT_RENDERING
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
#else
    const DXGI_FORMAT c_RenderFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
#endif

    // 2D only rendering
#ifdef XBOX
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_UNKNOWN, 2,
        DX::DeviceResources::c_Enable4K_UHD
#ifdef USE_FAST_SEMANTICS
        | DX::DeviceResources::c_FastSemantics
#endif
        );
#elif defined(UWP)
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        c_RenderFormat, DXGI_FORMAT_UNKNOWN, 2, D3D_FEATURE_LEVEL_9_3,
        DX::DeviceResources::c_Enable4K_Xbox
        );
#else
    m_deviceResources = std::make_unique<DX::DeviceResources>(c_RenderFormat, DXGI_FORMAT_UNKNOWN);
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

    UnitTests();

    m_delay = SWAP_TIME;
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
    auto kb = m_keyboard->GetState();
    if (kb.Escape || (pad.IsConnected() && pad.IsViewPressed()))
    {
        ExitGame();
    }

    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    m_keyboardButtons.Update(kb);

    if (kb.Left || (pad.IsConnected() && pad.dpad.left))
    {
        m_spriteBatch->SetRotation(DXGI_MODE_ROTATION_ROTATE270);
        assert(m_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_ROTATE270);
    }
    else if (kb.Right || (pad.IsConnected() && pad.dpad.right))
    {
        m_spriteBatch->SetRotation(DXGI_MODE_ROTATION_ROTATE90);
        assert(m_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_ROTATE90);
    }
    else if (kb.Up || (pad.IsConnected() && pad.dpad.up))
    {
        m_spriteBatch->SetRotation(DXGI_MODE_ROTATION_IDENTITY);
        assert(m_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_IDENTITY);
    }
    else if (kb.Down || (pad.IsConnected() && pad.dpad.down))
    {
        m_spriteBatch->SetRotation(DXGI_MODE_ROTATION_ROTATE180);
        assert(m_spriteBatch->GetRotation() == DXGI_MODE_ROTATION_ROTATE180);
    }

    if (m_keyboardButtons.IsKeyPressed(Keyboard::Space) || (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED))
    {
        m_showUTF8 = !m_showUTF8;
        m_delay = SWAP_TIME;
    }
    else if (!kb.Space && !(pad.IsConnected() && pad.IsYPressed()))
    {
        m_delay -= static_cast<float>(timer.GetElapsedSeconds());

        if (m_delay <= 0.f)
        {
            m_showUTF8 = !m_showUTF8;
            m_delay = SWAP_TIME;
        }
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

    XMVECTORF32 red, blue, dgreen, cyan, yellow, gray, dgray;
#ifdef GAMMA_CORRECT_RENDERING
    red.v = XMColorSRGBToRGB(Colors::Red);
    blue.v = XMColorSRGBToRGB(Colors::Blue);
    dgreen.v = XMColorSRGBToRGB(Colors::DarkGreen);
    cyan.v = XMColorSRGBToRGB(Colors::Cyan);
    yellow.v = XMColorSRGBToRGB(Colors::Yellow);
    gray.v = XMColorSRGBToRGB(Colors::Gray);
    dgray.v = XMColorSRGBToRGB(Colors::DarkGray);
#else
    red.v = Colors::Red;
    blue.v = Colors::Blue;
    dgreen.v = Colors::DarkGreen;
    cyan.v = Colors::Cyan;
    yellow.v = Colors::Yellow;
    gray.v = Colors::Gray;
    dgray.v = Colors::DarkGray;
#endif

    Clear();

    auto context = m_deviceResources->GetD3DDeviceContext();

    m_spriteBatch->Begin();

    float time = 60.f * static_cast<float>(m_timer.GetTotalSeconds());

    if (m_showUTF8)
    {
        m_comicFont->DrawString(m_spriteBatch.get(), "Hello, world!", XMFLOAT2(0, 0));
        m_italicFont->DrawString(m_spriteBatch.get(), "This text is in italics.\nIs it well spaced?", XMFLOAT2(220, 0));
        m_scriptFont->DrawString(m_spriteBatch.get(), "Script font, yo...", XMFLOAT2(0, 50));

        SpriteEffects flip = (SpriteEffects)((int)(time / 100) & 3);
        m_multicoloredFont->DrawString(m_spriteBatch.get(), "OMG it's full of stars!", XMFLOAT2(610, 130), Colors::White, XM_PIDIV2, XMFLOAT2(0, 0), 1, flip);

        m_comicFont->DrawString(m_spriteBatch.get(), u8"This is a larger block\nof text using a\nfont scaled to a\nsmaller size.\nSome c\x1234ha\x1543rac\x2453te\x1634r\x1563s are not in the font, but should show up as hyphens.", XMFLOAT2(10, 90), Colors::Black, 0, XMFLOAT2(0, 0), 0.5f);

        char tmp[256] = {};
        sprintf_s(tmp, "%llu frames", m_frame);

        m_nonproportionalFont->DrawString(m_spriteBatch.get(), tmp, XMFLOAT2(201, 130), Colors::Black);
        m_nonproportionalFont->DrawString(m_spriteBatch.get(), tmp, XMFLOAT2(200, 131), Colors::Black);
        m_nonproportionalFont->DrawString(m_spriteBatch.get(), tmp, XMFLOAT2(200, 130), red);

        float scale = sin(time / 100) + 1;
        auto spinText = "Spinning\nlike a cat";
        auto size = m_comicFont->MeasureString(spinText);
        m_comicFont->DrawString(m_spriteBatch.get(), spinText, XMVectorSet(150, 350, 0, 0), blue, time / 60, size / 2, scale);

        auto mirrorText = "It's a\nmirror...";
        auto mirrorSize = m_comicFont->MeasureString(mirrorText);
        m_comicFont->DrawString(m_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), Colors::Black, 0, mirrorSize * XMVectorSet(0, 1, 0, 0), 1, SpriteEffects_None);
        m_comicFont->DrawString(m_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), gray, 0, mirrorSize * XMVectorSet(1, 1, 0, 0), 1, SpriteEffects_FlipHorizontally);
        m_comicFont->DrawString(m_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), gray, 0, mirrorSize * XMVectorSet(0, 0, 0, 0), 1, SpriteEffects_FlipVertically);
        m_comicFont->DrawString(m_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), dgray, 0, mirrorSize * XMVectorSet(1, 0, 0, 0), 1, SpriteEffects_FlipBoth);

        m_japaneseFont->DrawString(m_spriteBatch.get(), u8"\x79C1\x306F\x65E5\x672C\x8A9E\x304C\x8A71\x305B\x306A\x3044\x306E\x3067\x3001\n\x79C1\x306F\x3053\x308C\x304C\x4F55\x3092\x610F\x5473\x3059\x308B\x306E\x304B\x308F\x304B\x308A\x307E\x305B\x3093", XMFLOAT2(10, 512));
    }
    else
    {
        m_comicFont->DrawString(m_spriteBatch.get(), L"Hello, world!", XMFLOAT2(0, 0));
        m_italicFont->DrawString(m_spriteBatch.get(), L"This text is in italics.\nIs it well spaced?", XMFLOAT2(220, 0));
        m_scriptFont->DrawString(m_spriteBatch.get(), L"Script font, yo...", XMFLOAT2(0, 50));

        SpriteEffects flip = (SpriteEffects)((int)(time / 100) & 3);
        m_multicoloredFont->DrawString(m_spriteBatch.get(), L"OMG it's full of stars!", XMFLOAT2(610, 130), Colors::White, XM_PIDIV2, XMFLOAT2(0, 0), 1, flip);

        m_comicFont->DrawString(m_spriteBatch.get(), L"This is a larger block\nof text using a\nfont scaled to a\nsmaller size.\nSome c\x1234ha\x1543rac\x2453te\x1634r\x1563s are not in the font, but should show up as hyphens.", XMFLOAT2(10, 90), Colors::Black, 0, XMFLOAT2(0, 0), 0.5f);

        wchar_t tmp[256] = {};
        swprintf_s(tmp, L"%llu frames", m_frame);

        m_nonproportionalFont->DrawString(m_spriteBatch.get(), tmp, XMFLOAT2(201, 130), Colors::Black);
        m_nonproportionalFont->DrawString(m_spriteBatch.get(), tmp, XMFLOAT2(200, 131), Colors::Black);
        m_nonproportionalFont->DrawString(m_spriteBatch.get(), tmp, XMFLOAT2(200, 130), red);

        float scale = sin(time / 100) + 1;
        auto spinText = L"Spinning\nlike a cat";
        auto size = m_comicFont->MeasureString(spinText);
        m_comicFont->DrawString(m_spriteBatch.get(), spinText, XMVectorSet(150, 350, 0, 0), blue, time / 60, size / 2, scale);

        auto mirrorText = L"It's a\nmirror...";
        auto mirrorSize = m_comicFont->MeasureString(mirrorText);
        m_comicFont->DrawString(m_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), Colors::Black, 0, mirrorSize * XMVectorSet(0, 1, 0, 0), 1, SpriteEffects_None);
        m_comicFont->DrawString(m_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), gray, 0, mirrorSize * XMVectorSet(1, 1, 0, 0), 1, SpriteEffects_FlipHorizontally);
        m_comicFont->DrawString(m_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), gray, 0, mirrorSize * XMVectorSet(0, 0, 0, 0), 1, SpriteEffects_FlipVertically);
        m_comicFont->DrawString(m_spriteBatch.get(), mirrorText, XMVectorSet(400, 400, 0, 0), dgray, 0, mirrorSize * XMVectorSet(1, 0, 0, 0), 1, SpriteEffects_FlipBoth);

        m_japaneseFont->DrawString(m_spriteBatch.get(), L"\x79C1\x306F\x65E5\x672C\x8A9E\x304C\x8A71\x305B\x306A\x3044\x306E\x3067\x3001\n\x79C1\x306F\x3053\x308C\x304C\x4F55\x3092\x610F\x5473\x3059\x308B\x306E\x304B\x308F\x304B\x308A\x307E\x305B\x3093", XMFLOAT2(10, 512));
    }

    {
        char ascii[256] = {};
        int i = 0;
        for (size_t j = 32; j < 256; ++j)
        {
            if (j == L'\n' || j == L'\r' || j == L'\t')
                continue;

            if (j > 0 && (j % 128) == 0)
            {
                ascii[i++] = L'\n';
                ascii[i++] = L'\n';
            }

            ascii[i++] = static_cast<char>(j + 1);
        }

        wchar_t unicode[256] = {};

        UINT cp = 437;
    #ifdef XBOX
        cp = CP_UTF8;

        m_consolasFont->SetDefaultCharacter('-');
    #endif

        if (!MultiByteToWideChar(cp, 0, ascii, i, unicode, 256))
            swprintf_s(unicode, L"<ERROR: %u>\n", GetLastError());

        m_consolasFont->DrawString(m_spriteBatch.get(), unicode, XMFLOAT2(10, 600), cyan);
    }

    m_spriteBatch->End();

    m_spriteBatch->Begin();

    m_ctrlFont->DrawString(m_spriteBatch.get(), L" !\"\n#$%\n&'()\n*+,-", XMFLOAT2(650, 130), Colors::White, 0.f, XMFLOAT2(0.f, 0.f), 0.5f);

    m_ctrlOneFont->DrawString(m_spriteBatch.get(), L" !\"\n#$%\n&'()\n*+,-", XMFLOAT2(950, 130), Colors::White, 0.f, XMFLOAT2(0.f, 0.f), 0.5f);

#ifndef XBOX
    {
        UINT w, h;

        auto outputSize = m_deviceResources->GetOutputSize();

        switch (m_spriteBatch->GetRotation())
        {
        case DXGI_MODE_ROTATION_ROTATE90:
        case DXGI_MODE_ROTATION_ROTATE270:
            w = outputSize.bottom;
            h = outputSize.right;
            break;

        default:
            w = outputSize.right;
            h = outputSize.bottom;
            break;
        }

        for (UINT x = 0; x < w; x += 100)
        {
            wchar_t tmp[16] = {};
            swprintf_s(tmp, L"%u\n", x);
            m_nonproportionalFont->DrawString(m_spriteBatch.get(), tmp, XMFLOAT2(float(x), float(h - 50)), yellow);
        }

        for (UINT y = 0; y < h; y += 100)
        {
            wchar_t tmp[16] = {};
            swprintf_s(tmp, L"%u\n", y);
            m_nonproportionalFont->DrawString(m_spriteBatch.get(), tmp, XMFLOAT2(float(w - 100), float(y)), red);
        }
    }
#endif

    m_spriteBatch->End();

    m_spriteBatch->Begin(SpriteSortMode_Deferred, nullptr, nullptr, nullptr, m_scissorState.Get(), [&]()
    {
        CD3D11_RECT r(640, 20, 740, 38);
        context->RSSetScissorRects(1, &r);
    });

    m_comicFont->DrawString(m_spriteBatch.get(), L"Clipping!", XMFLOAT2(640, 0), dgreen);

    m_spriteBatch->End();

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

    XMVECTORF32 color;
#ifdef GAMMA_CORRECT_RENDERING
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);
#else
    color.v = Colors::CornflowerBlue;
#endif
    context->ClearRenderTargetView(renderTarget, color);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
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

    m_spriteBatch = std::make_unique<SpriteBatch>(context);

#ifdef GAMMA_CORRECT_RENDERING
    bool forceSRGB = true;
#else
    bool forceSRGB = false;
#endif

    m_comicFont = std::make_unique<SpriteFont>(device, L"comic.spritefont");
    m_italicFont = std::make_unique<SpriteFont>(device, L"italic.spritefont");
    m_scriptFont = std::make_unique<SpriteFont>(device, L"script.spritefont");
    m_nonproportionalFont = std::make_unique<SpriteFont>(device, L"nonproportional.spritefont");
    m_multicoloredFont = std::make_unique<SpriteFont>(device, L"multicolored.spritefont", forceSRGB);
    m_japaneseFont = std::make_unique<SpriteFont>(device, L"japanese.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, L"xboxController.spritefont", forceSRGB);
    m_ctrlOneFont = std::make_unique<SpriteFont>(device, L"xboxOneController.spritefont", forceSRGB);
    m_consolasFont = std::make_unique<SpriteFont>(device, L"consolas.spritefont");

    CD3D11_RASTERIZER_DESC rsDesc(D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE, 0, 0.f, 0.f, TRUE, TRUE, TRUE, FALSE);
    DX::ThrowIfFailed(device->CreateRasterizerState(&rsDesc, m_scissorState.ReleaseAndGetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);

#ifdef XBOX
    if (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_Enable4K_UHD)
    {
        // Scale sprite batch rendering when running 4k
        static const D3D11_VIEWPORT s_vp1080 = { 0.f, 0.f, 1920.f, 1080.f, D3D11_MIN_DEPTH, D3D11_MAX_DEPTH };
        m_spriteBatch->SetViewport(s_vp1080);
    }
#elif defined(UWP)
    if (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_Enable4K_Xbox)
    {
        // Scale sprite batch rendering when running 4k
        static const D3D11_VIEWPORT s_vp1080 = { 0.f, 0.f, 1920.f, 1080.f, D3D11_MIN_DEPTH, D3D11_MAX_DEPTH };
        m_spriteBatch->SetViewport(s_vp1080);
    }

    auto rotation = m_deviceResources->GetRotation();
    m_spriteBatch->SetRotation(rotation);
#endif
}

#ifdef LOSTDEVICE
void Game::OnDeviceLost()
{
    m_comicFont.reset();
    m_italicFont.reset();
    m_scriptFont.reset();
    m_nonproportionalFont.reset();
    m_multicoloredFont.reset();
    m_japaneseFont.reset();
    m_ctrlFont.reset();
    m_ctrlOneFont.reset();
    m_consolasFont.reset();

    m_spriteBatch.reset();

    m_scissorState.Reset();
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
    OutputDebugStringA("*********** UNIT TESTS BEGIN ***************\n");

    bool success = true;

    // GetDefaultCharacterTest
    if (m_comicFont->GetDefaultCharacter() != 0)
    {
        OutputDebugStringA("FAILED: GetDefaultCharacter\n");
        success = false;
    }

    // ContainsCharacter tests
    if (m_comicFont->ContainsCharacter(27)
        || !m_comicFont->ContainsCharacter('-'))
    {
        OutputDebugStringA("FAILED: ContainsCharacter\n");
        success = false;
    }

    // FindGlyph/GetSpriteSheet tests
    {
        auto g = m_comicFont->FindGlyph('-');
        if (g->Character != '-' || g->XOffset != 6 || g->YOffset != 24)
        {
            OutputDebugStringA("FAILED: FindGlyph\n");
            success = false;
        }

        ComPtr<ID3D11ShaderResourceView> sheet;
        m_comicFont->GetSpriteSheet(sheet.GetAddressOf());
        if (!sheet)
        {
            OutputDebugStringA("FAILED: GetSpriteSheet\n");
            success = false;
        }
    }

    // DefaultCharacter tests
    m_comicFont->SetDefaultCharacter('-');
    if (m_comicFont->GetDefaultCharacter() != '-')
    {
        OutputDebugStringA("FAILED: Get/SetDefaultCharacter\n");
        success = false;
    }

    // Linespacing tests
    float s = m_ctrlFont->GetLineSpacing();
    if (s != 186.f)
    {
        OutputDebugStringA("FAILED: GetLineSpacing\n");
        success = false;
    }
    m_ctrlFont->SetLineSpacing(256.f);
    s = m_ctrlFont->GetLineSpacing();
    if (s != 256.f)
    {
        OutputDebugStringA("FAILED: Get/SetLineSpacing failed\n");
        success = false;
    }
    m_ctrlFont->SetLineSpacing(186.f);

    // Measure tests
    {
        auto spinText = L"Spinning\nlike a cat";

        auto drawSize = m_comicFont->MeasureString(spinText);

        if (fabs(XMVectorGetX(drawSize) - 136.f) > EPSILON
            || fabs(XMVectorGetY(drawSize) - 85.4713516f) > EPSILON)
        {
            OutputDebugStringA("FAILED: MeasureString\n");
            success = false;
        }

        auto rect = m_comicFont->MeasureDrawBounds(spinText, XMFLOAT2(150, 350));

        if (rect.top != 361
            || rect.bottom != 428
            || rect.left != 157
            || rect.right != 286)
        {
            OutputDebugStringA("FAILED: MeasureDrawBounds\n");
            success = false;
        }

        auto text2 = L"  Spinning like a cat ";

        drawSize = m_comicFont->MeasureString(text2);

        if (fabs(XMVectorGetX(drawSize) - 284.f) > EPSILON
            || fabs(XMVectorGetY(drawSize) - 44.f) > EPSILON)
        {
            OutputDebugStringA("FAILED: MeasureString (2)\n");
            success = false;
        }

        rect = m_comicFont->MeasureDrawBounds(text2, XMFLOAT2(150, 350));

        if (rect.top != 359
            || rect.bottom != 394
            || rect.left != 175
            || rect.right != 434)
        {
            OutputDebugStringA("FAILED: MeasureDrawBounds (2)\n");
            success = false;
        }

        // test measurements that factor in whitespace characters
        // against measurements that do not factor in whitespace characters

        auto text3 = L"aeiou";
        auto text4 = L"a e i o u";
        auto text5 = L"aeiou ";
        auto text6 = L"  ";
        auto testLineHeight = m_comicFont->GetLineSpacing();

        const auto testPos = XMFLOAT2{ 10.0f, 15.0f };

        // this test makes sure that including whitespace in the measurements
        // for a string that does not include whitespace results in no change

        auto drawSize3_1 = m_comicFont->MeasureString(text3, true);
        auto drawSize3_2 = m_comicFont->MeasureString(text3, false);

        if (XMVectorGetX(drawSize3_1) != XMVectorGetX(drawSize3_2))
        {
            OutputDebugStringA("FAILED: MeasureString ignoreSpace vs. !ignoreSpace test #1\n");
            success = false;
        }

        if (XMVectorGetY(drawSize3_2) != testLineHeight || XMVectorGetY(drawSize3_1) != XMVectorGetY(drawSize3_2))
        {
            OutputDebugStringA("FAILED: MeasureString ignoreSpace vs. !ignoreSpace test #2\n");
            success = false;
        }

        // this test makes sure that including whitespace in the measurements
        // for a string that does include whitespace still results in no change
        // if the whitespace is in the middle

        auto drawSize4_1 = m_comicFont->MeasureString(text4, true);
        auto drawSize4_2 = m_comicFont->MeasureString(text4, false);

        if (XMVectorGetX(drawSize4_1) != XMVectorGetX(drawSize4_2))
        {
            OutputDebugStringA("FAILED: MeasureString ignoreSpace vs. !ignoreSpace test #3\n");
            success = false;
        }

        if (XMVectorGetY(drawSize4_2) != testLineHeight || XMVectorGetY(drawSize4_1) != XMVectorGetY(drawSize4_2))
        {
            OutputDebugStringA("FAILED: MeasureString ignoreSpace vs. !ignoreSpace test #4\n");
            success = false;
        }

        // this test makes sure that including whitespace in the measurements
        // for a string that does include whitespace results in change
        // if the whitespace is at the end for x, no change for y

        auto drawSize5_1 = m_comicFont->MeasureString(text5, true);
        auto drawSize5_2 = m_comicFont->MeasureString(text5, false);

        if (XMVectorGetX(drawSize5_1) >= XMVectorGetX(drawSize5_2))
        {
            OutputDebugStringA("FAILED: MeasureString ignoreSpace vs. !ignoreSpace test #5\n");
            success = false;
        }

        if (XMVectorGetY(drawSize5_2) != testLineHeight || XMVectorGetY(drawSize5_1) != XMVectorGetY(drawSize5_2))
        {
            OutputDebugStringA("FAILED: MeasureString ignoreSpace vs. !ignoreSpace test #6\n");
            success = false;
        }

        // this test makes sure that including whitespace in the measurements
        // for a string that only includes whitespace results in change

        auto drawSize6_1 = m_comicFont->MeasureString(text6, true);
        auto drawSize6_2 = m_comicFont->MeasureString(text6, false);

        if (XMVectorGetX(drawSize6_1) >= XMVectorGetX(drawSize6_2))
        {
            OutputDebugStringA("FAILED: MeasureString ignoreSpace vs. !ignoreSpace test #7\n");
            success = false;
        }

        if (XMVectorGetY(drawSize6_2) != testLineHeight || XMVectorGetY(drawSize6_1) >= XMVectorGetY(drawSize6_2))
        {
            OutputDebugStringA("FAILED: MeasureString ignoreSpace vs. !ignoreSpace test #8\n");
            success = false;
        }

        // this test makes sure that including whitespace in the measurements
        // for a draw bounds that does not include whitespace results in no change

        auto rect3_1 = m_comicFont->MeasureDrawBounds(text3, testPos, true);
        auto rect3_2 = m_comicFont->MeasureDrawBounds(text3, testPos, false);

        if (rect3_1.left != rect3_2.left || rect3_1.top != rect3_2.top ||
            rect3_1.right != rect3_2.right || rect3_1.bottom != rect3_2.bottom)
        {
            OutputDebugStringA("FAILED: MeasureDrawBounds ignoreSpace vs. !ignoreSpace test #1\n");
            success = false;
        }

        // this test makes sure that including whitespace in the measurements
        // for a string that does include whitespace results still in no change
        // if the whitespace is in the middle for x, but results in change for y

        auto rect4_1 = m_comicFont->MeasureDrawBounds(text4, testPos, true);
        auto rect4_2 = m_comicFont->MeasureDrawBounds(text4, testPos, false);

        if (rect4_1.left != rect4_2.left || rect4_1.right != rect4_2.right)
        {
            OutputDebugStringA("FAILED: MeasureDrawBounds ignoreSpace vs. !ignoreSpace test #2\n");
            success = false;
        }

        if (rect4_1.top <= rect4_2.top || rect4_1.bottom >= rect4_2.bottom)
        {
            OutputDebugStringA("FAILED: MeasureDrawBounds ignoreSpace vs. !ignoreSpace test #3\n");
            success = false;
        }

        // this test makes sure that including whitespace in the measurements
        // for a string that does include whitespace results in change
        // if the whitespace is at the end for x, and also change for y

        auto rect5_1 = m_comicFont->MeasureDrawBounds(text5, testPos, true);
        auto rect5_2 = m_comicFont->MeasureDrawBounds(text5, testPos, false);

        if (rect5_1.left != rect5_2.left || rect5_1.right >= rect5_2.right)
        {
            OutputDebugStringA("FAILED: MeasureDrawBounds ignoreSpace vs. !ignoreSpace test #4\n");
            success = false;
        }

        if (rect5_1.top <= rect5_2.top || rect5_1.bottom >= rect5_2.bottom)
        {
            OutputDebugStringA("FAILED: MeasureDrawBounds ignoreSpace vs. !ignoreSpace test #5\n");
            success = false;
        }

        // this test makes sure that including whitespace in the measurements
        // for a draw bounds that only includes whitespace results in change

        auto rect6_1 = m_comicFont->MeasureDrawBounds(text6, testPos, true);
        auto rect6_2 = m_comicFont->MeasureDrawBounds(text6, testPos, false);

        if (rect6_1.left >= rect6_2.left || rect6_1.right >= rect6_2.right)
        {
            OutputDebugStringA("FAILED: MeasureDrawBounds ignoreSpace vs. !ignoreSpace test #6\n");
            success = false;
        }

        if (rect6_1.top >= rect6_2.top || rect6_1.bottom >= rect6_2.bottom)
        {
            OutputDebugStringA("FAILED: MeasureDrawBounds ignoreSpace vs. !ignoreSpace test #7\n");
            success = false;
        }
    }

    OutputDebugStringA(success ? "Passed\n" : "Failed\n");
    OutputDebugStringA("***********  UNIT TESTS END  ***************\n");
}
