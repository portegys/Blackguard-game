#include "pch.h"
#include "MutePage.xaml.h"
#include "RogueInterface.hpp"

using namespace Blackguard;

using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::ApplicationSettings;

MutePage::MutePage()
{
   InitializeComponent();

   // Set mute state.
   if (getMute())
   {
      MuteToggle->IsOn = true;
   }
   else
   {
      MuteToggle->IsOn = false;
   }
}


void MutePage::ToggleMute(Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
   ApplicationDataContainer ^ localSettings = ApplicationData::Current->LocalSettings;
   IPropertySet ^ localValues = localSettings->Values;
   if (MuteToggle->IsOn)
   {
      setMute(true);
      localValues->Insert("Mute", dynamic_cast<PropertyValue ^>(PropertyValue::CreateString("On")));
   }
   else
   {
      setMute(false);
      localValues->Insert("Mute", dynamic_cast<PropertyValue ^>(PropertyValue::CreateString("Off")));
   }
}


void MutePage::BackButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
   Popup ^ popup = safe_cast<Popup ^> (this->Parent);
   popup->IsOpen = false;
   SettingsPane::Show();
}
