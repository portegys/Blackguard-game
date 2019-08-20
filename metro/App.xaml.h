//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"
#include "DirectXPage.xaml.h"

namespace Blackguard
{
/// <summary>
/// Provides application-specific behavior to supplement the default Application class.
/// </summary>
ref class App sealed
{
public:
   App();
   virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^ args) override;

private:
   void OnSuspending(Platform::Object ^ sender, Windows::ApplicationModel::SuspendingEventArgs ^ args);
   void OnResuming(Object ^ sender, Object ^ e);
   DirectXPage ^ m_directXPage;

protected:
   virtual void OnSearchActivated(Windows::ApplicationModel::Activation::SearchActivatedEventArgs ^ pArgs) override;
};
}
