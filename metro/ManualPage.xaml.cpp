#include "pch.h"
#include "ManualPage.xaml.h"

using namespace Blackguard;

using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::ApplicationSettings;

ManualPage::ManualPage(DirectXPage ^ directXPage)
{
   m_directXPage = directXPage;

   InitializeComponent();

   // Load manual text.
   ReaderWriter ^ readerWriter        = ref new ReaderWriter();
   Platform::Array<byte> ^ manualData = readerWriter->ReadData(L"assets/doc/blackguard.txt");
   string  s = (char *)manualData->Data;
   wstring w;
   w.assign(s.begin(), s.end());
   ManualText->Text = ref new Platform::String(w.c_str());
}


void ManualPage::ReturnButtonClick(Object ^ sender, RoutedEventArgs ^ args)
{
   Window::Current->Content = m_directXPage;
   Window::Current->Activate();
}
