//
// Main.cpp
//

#include "pch.h"
#include "Game.h"

#pragma warning(push)
#pragma warning(disable : 4265)
#include <wrl/wrappers/corewrappers.h>
#include <shellapi.h>
#pragma warning(pop)

#ifdef _GAMING_DESKTOP
#include <XGameRuntimeInit.h>
#include <XGameErr.h>
#endif

using namespace DirectX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

namespace
{
    std::unique_ptr<Game> g_game;
    bool g_testTimer = false;

#ifdef WM_DEVICECHANGE
    HDEVNOTIFY g_hNewAudio;
#endif
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ExitGame() noexcept;
void ParseCommandLine(_In_ LPWSTR lpCmdLine);

// Indicates to hybrid graphics systems to prefer the discrete part by default
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

// Entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    if (!XMVerifyCPUSupport())
        return 1;

#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef USING_WINDOWS_GAMING_INPUT
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize))
        return 1;
#else
    if (FAILED(CoInitializeEx(nullptr, COINITBASE_MULTITHREADED)))
        return 1;
#endif

#ifdef _GAMING_DESKTOP
    HRESULT hr = XGameRuntimeInitialize();
    if (FAILED(hr))
    {
        if (hr == E_GAMERUNTIME_DLL_NOT_FOUND || hr == E_GAMERUNTIME_VERSION_MISMATCH)
        {
            std::ignore = MessageBoxW(nullptr, L"Game Runtime is not installed on this system or needs updating (Microsoft GDK).", L"D3D11Test", MB_ICONERROR | MB_OK);
        }
        return 1;
    }
#endif

    ParseCommandLine(lpCmdLine);

    g_game = std::make_unique<Game>();

    // Register class and create window
    {
        // Register class
        WNDCLASSEXW wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEXW);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIconW(hInstance, L"IDI_ICON");
        wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wcex.lpszClassName = L"D3D11TestWindowClass";
        wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");
        if (!RegisterClassExW(&wcex))
            return 1;

        // Create window
        int w, h;
        g_game->GetDefaultSize(w, h);

        RECT rc = { 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };

        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwnd = CreateWindowExW(0, L"D3D11TestWindowClass", g_game->GetAppName(), WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, hInstance,
            g_game.get());

        if (!hwnd)
            return 1;

        ShowWindow(hwnd, nCmdShow);

        GetClientRect(hwnd, &rc);

        g_game->Initialize(hwnd, rc.right - rc.left, rc.bottom - rc.top, DXGI_MODE_ROTATION_IDENTITY);

        if (g_testTimer)
        {
            SetTimer(hwnd, 1, c_testTimeout, nullptr);
        }

#ifdef _DBT_H
        DEV_BROADCAST_DEVICEINTERFACE filter = {};
        filter.dbcc_size = sizeof(filter);
        filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        filter.dbcc_classguid = KSCATEGORY_AUDIO;

        g_hNewAudio = RegisterDeviceNotification(hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
#endif
    }

    // Main message loop
    MSG msg = {};
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_game->Tick();
        }
    }

    g_game.reset();

#ifdef WM_DEVICECHANGE
    UnregisterDeviceNotification(g_hNewAudio);
#endif

#ifdef _GAMING_DESKTOP
    XGameRuntimeUninitialize();
#endif

#ifndef USING_WINDOWS_GAMING_INPUT
    CoUninitialize();
#endif

    return static_cast<int>(msg.wParam);
}

// Windows procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool s_in_sizemove = false;
    static bool s_in_suspend = false;
    static bool s_minimized = false;
    static bool s_fullscreen = false;

    auto game = reinterpret_cast<Game*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        if (lParam)
        {
            auto params = reinterpret_cast<LPCREATESTRUCTW>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params->lpCreateParams));
        }
        break;

    case WM_PAINT:
        if (s_in_sizemove && game)
        {
            game->Tick();
        }
        else
        {
            PAINTSTRUCT ps;
            std::ignore = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_DISPLAYCHANGE:
        if (game)
        {
            game->OnDisplayChange();
        }
        break;

    case WM_MOVE:
        if (game)
        {
            game->OnWindowMoved();
        }
        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            if (!s_minimized)
            {
                s_minimized = true;
                if (!s_in_suspend && game)
                    game->OnSuspending();
                s_in_suspend = true;
            }
        }
        else if (s_minimized)
        {
            s_minimized = false;
            if (s_in_suspend && game)
                game->OnResuming();
            s_in_suspend = false;
        }
        else if (!s_in_sizemove && game)
        {
            game->OnWindowSizeChanged(LOWORD(lParam), HIWORD(lParam), DXGI_MODE_ROTATION_IDENTITY);
        }
        break;

    case WM_ENTERSIZEMOVE:
        s_in_sizemove = true;
        break;

    case WM_EXITSIZEMOVE:
        s_in_sizemove = false;
        if (game)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);

            game->OnWindowSizeChanged(rc.right - rc.left, rc.bottom - rc.top, DXGI_MODE_ROTATION_IDENTITY);
        }
        break;

    case WM_GETMINMAXINFO:
        if (lParam)
        {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = 320;
            info->ptMinTrackSize.y = 200;
        }
        break;

    case WM_ACTIVATEAPP:
        Keyboard::ProcessMessage(message, wParam, lParam);
        Mouse::ProcessMessage(message, wParam, lParam);

        if (game)
        {
            if (wParam)
            {
                game->OnActivated();
            }
            else
            {
                game->OnDeactivated();
            }
        }
        break;

    case WM_ACTIVATE:
        Keyboard::ProcessMessage(message, wParam, lParam);
        Mouse::ProcessMessage(message, wParam, lParam);
        break;

    case WM_POWERBROADCAST:
        switch (wParam)
        {
        case PBT_APMQUERYSUSPEND:
            if (!s_in_suspend && game)
                game->OnSuspending();
            s_in_suspend = true;
            return TRUE;

        case PBT_APMRESUMESUSPEND:
            if (!s_minimized)
            {
                if (s_in_suspend && game)
                    game->OnResuming();
                s_in_suspend = false;
            }
            return TRUE;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        Keyboard::ProcessMessage(message, wParam, lParam);
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
        {
            // Implements the classic ALT+ENTER fullscreen toggle
            if (s_fullscreen)
            {
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);

                int width = 800;
                int height = 600;
                if (game)
                    game->GetDefaultSize(width, height);

                ShowWindow(hWnd, SW_SHOWNORMAL);

                SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
            }
            else
            {
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP);
                SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_TOPMOST);

                SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

                ShowWindow(hWnd, SW_SHOWMAXIMIZED);
            }

            s_fullscreen = !s_fullscreen;
        }
        Keyboard::ProcessMessage(message, wParam, lParam);
        break;

    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        Mouse::ProcessMessage(message, wParam, lParam);
        break;

    case WM_MOUSEACTIVATE:
        // When you click activate the window, we want Mouse to ignore it.
        return MA_ACTIVATEANDEAT;

#ifdef _DBT_H
    case WM_DEVICECHANGE:
        if (wParam == DBT_DEVICEARRIVAL)
        {
            auto pDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
            if (pDev)
            {
                if (pDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
                {
                    auto pInter = reinterpret_cast<const PDEV_BROADCAST_DEVICEINTERFACE>(pDev);
                    if (pInter->dbcc_classguid == KSCATEGORY_AUDIO)
                    {
#ifdef _DEBUG
                        OutputDebugStringA("INFO: New audio device detected: ");
                        OutputDebugString(pInter->dbcc_name);
                        OutputDebugStringA("\n");
#endif
                        PostMessage(hWnd, WM_USER, 0, 0);
                    }
                }
            }
        }
        return 0;

    case WM_USER:
        if (g_game)
        {
            g_game->OnAudioDeviceChange();
        }
        break;
#endif

    case WM_MENUCHAR:
        // A menu is active and the user presses a key that does not correspond
        // to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
        return MAKELRESULT(0, MNC_CLOSE);

    case WM_TIMER:
        if (g_testTimer)
        {
            ExitGame();
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

void ParseCommandLine(_In_ LPWSTR lpCmdLine)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(lpCmdLine, &argc);

    for (int iArg = 0; iArg < argc; iArg++)
    {
        wchar_t* pArg = argv[iArg];

        if (('-' == pArg[0]) || ('/' == pArg[0]))
        {
            pArg++;
            wchar_t* pValue;

            for (pValue = pArg; *pValue && (':' != *pValue); pValue++);

            if (*pValue)
                *pValue++ = 0;

            if (_wcsicmp(pArg, L"ctest") == 0)
            {
                g_testTimer = true;
            }
            else if (_wcsicmp(pArg, L"forcewarp") == 0)
            {
                DX::DeviceResources::DebugForceWarp(true);
            }
            else if (_wcsicmp(pArg, L"minpower") == 0)
            {
                DX::DeviceResources::DebugPreferMinimumPower(true);
            }
            else if (_wcsicmp(pArg, L"adapter") == 0)
            {
                if (pValue && *pValue != 0)
                {
                    DX::DeviceResources::DebugSetAdapter(_wtoi(pValue));
                }
            }
        }
    }

    LocalFree(argv);
}

// Exit helper
void ExitGame() noexcept
{
    PostQuitMessage(0);
}
