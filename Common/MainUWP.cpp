//
// Main.cpp
//

#include "pch.h"
#include "Game.h"

#include <ppltasks.h>

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::ViewManagement;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace DirectX;

#pragma warning(disable : 4061)

void ExitGame() noexcept;
void GetWindowBounds(_In_ IUnknown* window, _Out_ RECT* rect);

namespace
{
    inline int ConvertDipsToPixels(float dips, float dpi) noexcept
    {
        return int(dips * dpi / 96.f + 0.5f);
    }

    inline float ConvertPixelsToDips(int pixels, float dpi) noexcept
    {
        return (float(pixels) * 96.f / dpi);
    }
}

ref class ViewProvider sealed : public IFrameworkView
{
public:
    ViewProvider() :
        m_exit(false),
        m_visible(true),
        m_in_sizemove(false),
        m_DPI(96.f),
        m_logicalWidth(800.f),
        m_logicalHeight(600.f),
        m_nativeOrientation(DisplayOrientations::None),
        m_currentOrientation(DisplayOrientations::None)
    {
    }

    // IFrameworkView methods
    virtual void Initialize(CoreApplicationView^ applicationView)
    {
        applicationView->Activated +=
            ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &ViewProvider::OnActivated);

        CoreApplication::Suspending +=
            ref new EventHandler<SuspendingEventArgs^>(this, &ViewProvider::OnSuspending);

        CoreApplication::Resuming +=
            ref new EventHandler<Platform::Object^>(this, &ViewProvider::OnResuming);

        m_game = std::make_unique<Game>();

        using namespace Windows::Graphics::Display::Core;

        auto displayInfo = HdmiDisplayInformation::GetForCurrentView();
        if (displayInfo)
        {
#ifdef _DEBUG
            auto current = displayInfo->GetCurrentDisplayMode();

            if (current)
            {
                char buff[256] = {};
                sprintf_s(buff, "INFO: Current display mode %u x %u (%f Hz)\n", current->ResolutionWidthInRawPixels, current->ResolutionHeightInRawPixels, current->RefreshRate);
                OutputDebugStringA(buff);
            }
#endif

            if (m_game->RequestHDRMode())
            {
                auto modes = displayInfo->GetSupportedDisplayModes();

                for (unsigned i = 0; i < modes->Size; i++)
                {
                    auto mode = modes->GetAt(i);

                    if (mode->ColorSpace == HdmiDisplayColorSpace::BT2020 &&
                        mode->ResolutionHeightInRawPixels == 2160 &&
                        mode->ResolutionWidthInRawPixels == 3840 &&
                        mode->StereoEnabled == false &&
                        mode->RefreshRate >= 59.0 && mode->RefreshRate <= 60.0)
                    {
                        displayInfo->RequestSetCurrentDisplayModeAsync(mode, HdmiDisplayHdrOption::Eotf2084);
#ifdef _DEBUG
                        OutputDebugStringA("INFO: TV in HDR mode\n");
#endif
                    }
                }
            }
        }

#ifdef AUDIO_WATCHER
        using namespace Windows::Devices::Enumeration;

        m_audioWatcher = DeviceInformation::CreateWatcher(DeviceClass::AudioRender);

        m_audioWatcher->Added += ref new TypedEventHandler<DeviceWatcher^, DeviceInformation^>(this, &ViewProvider::OnAudioDeviceAdded);
        m_audioWatcher->Updated += ref new TypedEventHandler<DeviceWatcher^, DeviceInformationUpdate^>(this, &ViewProvider::OnAudioDeviceUpdated);

        m_audioWatcher->Start();
#endif
    }

    virtual void Uninitialize()
    {
        m_game.reset();
    }

    virtual void SetWindow(CoreWindow^ window)
    {
        window->SizeChanged +=
            ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &ViewProvider::OnWindowSizeChanged);

        try
        {
            window->ResizeStarted +=
                ref new TypedEventHandler<CoreWindow^, Object^>(this, &ViewProvider::OnResizeStarted);

            window->ResizeCompleted +=
                ref new TypedEventHandler<CoreWindow^, Object^>(this, &ViewProvider::OnResizeCompleted);
        }
        catch (...)
        {
            // Requires Windows 10 Creators Update (10.0.15063) or later
        }

        window->VisibilityChanged +=
            ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &ViewProvider::OnVisibilityChanged);

        window->Closed +=
            ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &ViewProvider::OnWindowClosed);

        auto dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;

        dispatcher->AcceleratorKeyActivated +=
            ref new TypedEventHandler<CoreDispatcher^, AcceleratorKeyEventArgs^>(this, &ViewProvider::OnAcceleratorKeyActivated);

        auto navigation = Windows::UI::Core::SystemNavigationManager::GetForCurrentView();

        navigation->BackRequested +=
            ref new EventHandler<BackRequestedEventArgs^>(this, &ViewProvider::OnBackRequested);

        auto currentDisplayInformation = DisplayInformation::GetForCurrentView();

        currentDisplayInformation->DpiChanged +=
            ref new TypedEventHandler<DisplayInformation^, Object^>(this, &ViewProvider::OnDpiChanged);

        currentDisplayInformation->OrientationChanged +=
            ref new TypedEventHandler<DisplayInformation^, Object^>(this, &ViewProvider::OnOrientationChanged);

        DisplayInformation::DisplayContentsInvalidated +=
            ref new TypedEventHandler<DisplayInformation^, Object^>(this, &ViewProvider::OnDisplayContentsInvalidated);

        m_DPI = currentDisplayInformation->LogicalDpi;

        m_logicalWidth = window->Bounds.Width;
        m_logicalHeight = window->Bounds.Height;

        m_nativeOrientation = currentDisplayInformation->NativeOrientation;
        m_currentOrientation = currentDisplayInformation->CurrentOrientation;

        int outputWidth = ConvertDipsToPixels(m_logicalWidth, m_DPI);
        int outputHeight = ConvertDipsToPixels(m_logicalHeight, m_DPI);

        DXGI_MODE_ROTATION rotation = ComputeDisplayRotation();

        if (rotation == DXGI_MODE_ROTATION_ROTATE90 || rotation == DXGI_MODE_ROTATION_ROTATE270)
        {
            std::swap(outputWidth, outputHeight);
        }

        m_game->Initialize(reinterpret_cast<IUnknown*>(window),
                           outputWidth, outputHeight, rotation );

        Mouse::SetDpi(m_DPI);
    }

    virtual void Load(Platform::String^ entryPoint)
    {
    }

    virtual void Run()
    {
        while (!m_exit)
        {
            if (m_visible)
            {
                m_game->Tick();

                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
            }
            else
            {
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
            }
        }
    }

protected:
    // Event handlers
    void OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
    {
        if (args->Kind == ActivationKind::Launch)
        {
            auto launchArgs = static_cast<LaunchActivatedEventArgs^>(args);

            if (launchArgs->PrelaunchActivated)
            {
                // Opt-out of Prelaunch
                CoreApplication::Exit();
                return;
            }
        }

        int w, h;
        m_game->GetDefaultSize(w, h);

        m_DPI = DisplayInformation::GetForCurrentView()->LogicalDpi;

        ApplicationView::PreferredLaunchWindowingMode = ApplicationViewWindowingMode::PreferredLaunchViewSize;

        auto desiredSize = Size(ConvertPixelsToDips(w, m_DPI), ConvertPixelsToDips(h, m_DPI));

        ApplicationView::PreferredLaunchViewSize = desiredSize;

        auto view = ApplicationView::GetForCurrentView();

        auto minSize = Size(ConvertPixelsToDips(320, m_DPI), ConvertPixelsToDips(200, m_DPI));

        view->SetPreferredMinSize(minSize);

        CoreWindow::GetForCurrentThread()->Activate();

        view->FullScreenSystemOverlayMode = FullScreenSystemOverlayMode::Minimal;

        view->TryResizeView(desiredSize);
    }

    void OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
    {
        auto deferral = args->SuspendingOperation->GetDeferral();

        create_task([this, deferral]()
        {
            m_game->OnSuspending();

            deferral->Complete();
        });
    }

    void OnResuming(Platform::Object^ sender, Platform::Object^ args)
    {
        m_game->OnResuming();
    }

    void OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
    {
        m_logicalWidth = sender->Bounds.Width;
        m_logicalHeight = sender->Bounds.Height;

        if (m_in_sizemove)
            return;

        HandleWindowSizeChanged();
    }

    void OnResizeStarted(CoreWindow^ sender, Platform::Object^ args)
    {
        m_in_sizemove = true;
    }

    void OnResizeCompleted(CoreWindow^ sender, Platform::Object^ args)
    {
        m_in_sizemove = false;

        HandleWindowSizeChanged();
    }

    void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
    {
        m_visible = args->Visible;
        if (m_visible)
            m_game->OnActivated();
        else
            m_game->OnDeactivated();
    }

    void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
    {
        m_exit = true;
    }

    void OnAcceleratorKeyActivated(CoreDispatcher^, AcceleratorKeyEventArgs^ args)
    {
        if (args->EventType == CoreAcceleratorKeyEventType::SystemKeyDown
            && args->VirtualKey == VirtualKey::Enter
            && args->KeyStatus.IsMenuKeyDown
            && !args->KeyStatus.WasKeyDown)
        {
            // Implements the classic ALT+ENTER fullscreen toggle
            auto view = ApplicationView::GetForCurrentView();

            if (view->IsFullScreenMode)
                view->ExitFullScreenMode();
            else
                view->TryEnterFullScreenMode();

            args->Handled = true;
        }
    }

    void OnBackRequested(Platform::Object^, Windows::UI::Core::BackRequestedEventArgs^ args)
    {
        // UWP on Xbox One triggers a back request whenever the B button is pressed
        // which can result in the app being suspended if unhandled
        args->Handled = true;
    }

    void OnDpiChanged(DisplayInformation^ sender, Object^ args)
    {
        m_DPI = sender->LogicalDpi;

        HandleWindowSizeChanged();

        Mouse::SetDpi(m_DPI);
    }

    void OnOrientationChanged(DisplayInformation^ sender, Object^ args)
    {
        auto resizeManager = CoreWindowResizeManager::GetForCurrentView();
        resizeManager->ShouldWaitForLayoutCompletion = true;

        m_currentOrientation = sender->CurrentOrientation;

        HandleWindowSizeChanged();

        resizeManager->NotifyLayoutCompleted();
    }

    void OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
    {
        m_game->ValidateDevice();
        m_game->OnDisplayChange();
    }

private:
    bool                    m_exit;
    bool                    m_visible;
    bool                    m_in_sizemove;
    float                   m_DPI;
    float                   m_logicalWidth;
    float                   m_logicalHeight;
    std::unique_ptr<Game>   m_game;

    Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
    Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;

#ifdef AUDIO_WATCHER
    Windows::Devices::Enumeration::DeviceWatcher^ m_audioWatcher;
#endif

    DXGI_MODE_ROTATION ComputeDisplayRotation() const
    {
        DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

        switch (m_nativeOrientation)
        {
        case DisplayOrientations::Landscape:
            switch (m_currentOrientation)
            {
            case DisplayOrientations::Landscape:
                rotation = DXGI_MODE_ROTATION_IDENTITY;
                break;

            case DisplayOrientations::Portrait:
                rotation = DXGI_MODE_ROTATION_ROTATE270;
                break;

            case DisplayOrientations::LandscapeFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE180;
                break;

            case DisplayOrientations::PortraitFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE90;
                break;

            default:
                break;
            }
            break;

        case DisplayOrientations::Portrait:
            switch (m_currentOrientation)
            {
            case DisplayOrientations::Landscape:
                rotation = DXGI_MODE_ROTATION_ROTATE90;
                break;

            case DisplayOrientations::Portrait:
                rotation = DXGI_MODE_ROTATION_IDENTITY;
                break;

            case DisplayOrientations::LandscapeFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE270;
                break;

            case DisplayOrientations::PortraitFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE180;
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }

        return rotation;
    }

    void HandleWindowSizeChanged()
    {
        int outputWidth = ConvertDipsToPixels(m_logicalWidth, m_DPI);
        int outputHeight = ConvertDipsToPixels(m_logicalHeight, m_DPI);

        DXGI_MODE_ROTATION rotation = ComputeDisplayRotation();

        if (rotation == DXGI_MODE_ROTATION_ROTATE90 || rotation == DXGI_MODE_ROTATION_ROTATE270)
        {
            std::swap(outputWidth, outputHeight);
        }

        m_game->OnWindowSizeChanged(outputWidth, outputHeight, rotation);
    }

#ifdef AUDIO_WATCHER
    void OnAudioDeviceAdded(Windows::Devices::Enumeration::DeviceWatcher^ sender, Windows::Devices::Enumeration::DeviceInformation^ args)
    {
        m_game->OnAudioDeviceChange();
    }

    void OnAudioDeviceUpdated(Windows::Devices::Enumeration::DeviceWatcher^ sender, Windows::Devices::Enumeration::DeviceInformationUpdate^ args)
    {
        m_game->OnAudioDeviceChange();
    }
#endif
};

ref class ViewProviderFactory : IFrameworkViewSource
{
public:
    virtual IFrameworkView^ CreateView()
    {
        return ref new ViewProvider();
    }
};


// Entry point
[Platform::MTAThread]
int __cdecl main(Platform::Array<Platform::String^>^ /*argv*/)
{
    if (!XMVerifyCPUSupport())
    {
        throw std::runtime_error("XMVerifyCPUSupport");
    }

    auto viewProviderFactory = ref new ViewProviderFactory();
    CoreApplication::Run(viewProviderFactory);
    return 0;
}


// Exit helper
void ExitGame() noexcept
{
    Windows::ApplicationModel::Core::CoreApplication::Exit();
}


// Window size helper
_Use_decl_annotations_
void GetWindowBounds(IUnknown* window, RECT* rect)
{
    if (!rect)
        return;

    *rect = {};

    if (!window)
        return;

    auto b = reinterpret_cast<CoreWindow^>(window)->Bounds;

    auto currentDisplayInformation = DisplayInformation::GetForCurrentView();
    float dpi = currentDisplayInformation->LogicalDpi;

    const int x = ConvertDipsToPixels(b.X, dpi);
    const int y = ConvertDipsToPixels(b.Y, dpi);
    const int w = ConvertDipsToPixels(b.Width, dpi);
    const int h = ConvertDipsToPixels(b.Height, dpi);

    rect->left = static_cast<long>(x);
    rect->top = static_cast<long>(y);
    rect->right = static_cast<long>(x + w);
    rect->bottom = static_cast<long>(y + h);
}
