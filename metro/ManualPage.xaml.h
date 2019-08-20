#pragma once

#include "pch.h"
#include "ManualPage.g.h"

namespace Blackguard
{
[Windows::Foundation::Metadata::WebHostHidden]
public ref class ManualPage sealed
{
public:

   ManualPage(DirectXPage ^ directXPage);

private:

   void ReturnButtonClick(Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ args);

   DirectXPage ^ m_directXPage;
};
}
