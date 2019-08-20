//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"
#include "ManualPage.xaml.h"

using namespace Blackguard;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Storage;

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
   m_directXPage = nullptr;
   InitializeComponent();
   Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
   Resuming   += ref new EventHandler<Platform::Object ^>(this, &App::OnResuming);
}


/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used when the application is launched to open a specific file, to display
/// search results, and so forth.
/// </summary>
/// <param name="args">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs ^ args)
{
   if (m_directXPage == nullptr)
   {
      m_directXPage = ref new DirectXPage();
   }
   if (args->PreviousExecutionState == ApplicationExecutionState::Terminated)
   {
      m_directXPage->OnResuming(ApplicationData::Current->LocalSettings->Values);
   }

   // Place the page in the current window and ensure that it is active.
   Window::Current->Content = m_directXPage;
   Window::Current->Activate();
}


/// <summary>
/// Invoked when the application is being suspended.
/// </summary>
/// <param name="sender">Details about the origin of the event.</param>
/// <param name="args">Details about the suspending event.</param>
void App::OnSuspending(Object ^ sender, SuspendingEventArgs ^ args)
{
   (void)sender;     // Unused parameter.
   (void)args;       // Unused parameter.

   m_directXPage->OnSuspending(ApplicationData::Current->LocalSettings->Values);
}


void App::OnResuming(Object ^ sender, Object ^ e)
{
   m_directXPage->OnResuming(ApplicationData::Current->LocalSettings->Values);
}


/// <summary>
/// Invoked when the application is activated to display search results.
/// </summary>
/// <param name="args">Details about the activation request.</param>
void App::OnSearchActivated(Windows::ApplicationModel::Activation::SearchActivatedEventArgs ^ args)
{
   if (m_directXPage == nullptr)
   {
      m_directXPage = ref new DirectXPage();
   }
   Window::Current->Content = ref new ManualPage(m_directXPage);
   Window::Current->Activate();
}
