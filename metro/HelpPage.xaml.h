#pragma once

#include "HelpPage.g.h"

namespace Blackguard
{
public ref class HelpPage sealed
{
public:
   HelpPage();
private:
   void BackButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
};
}
