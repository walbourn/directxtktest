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
using namespace Windows::Foundation;
using namespace DirectX;

ref class ViewProvider sealed : public IFrameworkView
{
public:
    ViewProvider() :
        m_exit(false)
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
    }

    virtual void Uninitialize()
    {
        m_game.reset();
    }

    virtual void SetWindow(CoreWindow^ window)
    {
        window->Closed +=
            ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &ViewProvider::OnWindowClosed);

        // Moved Game::Initialize to OnActivated to handle command-line parameters
        m_window = reinterpret_cast<IUnknown*>(window);
    }

    virtual void Load(Platform::String^ entryPoint)
    {
    }

    virtual void Run()
    {
        while (!m_exit)
        {
            m_game->Tick();

            CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        }
    }

protected:
    // Event handlers
    void OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
    {
        CoreWindow::GetForCurrentThread()->Activate();

        if (args->Kind == Windows::ApplicationModel::Activation::ActivationKind::Launch)
        {
            auto launchArgs = (ILaunchActivatedEventArgs^)args;

            const wchar_t* commandLine = launchArgs->Arguments->Data();

            if (wcsstr(commandLine, L"-render4K") != 0)
            {
                DX::DeviceResources::DebugRender4K(true);
            }
        }

        m_game->Initialize(m_window.Get(), 1920, 1080, DXGI_MODE_ROTATION_IDENTITY);
    }

    void OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
    {
        SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

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

    void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
    {
        m_exit = true;
    }

private:
    bool                    m_exit;
    std::unique_ptr<Game>   m_game;
    Microsoft::WRL::ComPtr<IUnknown> m_window;
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
int main(Platform::Array<Platform::String^>^ argv)
{
    UNREFERENCED_PARAMETER(argv);

    auto viewProviderFactory = ref new ViewProviderFactory();
    CoreApplication::Run(viewProviderFactory);
    return 0;
}
