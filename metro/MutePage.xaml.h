#pragma once

#include "MutePage.g.h"

namespace Blackguard
{
public ref class MutePage sealed
{
public:
   MutePage();
private:
   void ToggleMute(Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
   void BackButton_Click(Platform::Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ e);
};
}
