#include "pch.h"
#include "HelpPage.xaml.h"

using namespace Blackguard;

using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::ApplicationSettings;

HelpPage::HelpPage()
{
   InitializeComponent();
}


void HelpPage::BackButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e)
{
   Popup ^ popup = safe_cast<Popup ^> (this->Parent);
   popup->IsOpen = false;
   SettingsPane::Show();
}
