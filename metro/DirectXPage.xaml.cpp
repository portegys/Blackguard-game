//
// DirectXPage.xaml.cpp
// Implementation of the DirectXPage.xaml class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"
#include "ManualPage.xaml.h"
#include "MutePage.xaml.h"
#include "HelpPage.xaml.h"
#include "RogueInterface.hpp"

using namespace Blackguard;

using namespace Windows::Devices::Input;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Windows::System;
using namespace Windows::UI::ApplicationSettings;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Popups;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

DirectXPage::DirectXPage()
{
   InitializeComponent();

   // Initialize game.
   m_localSettings = ApplicationData::Current->LocalSettings;
   m_localValues   = m_localSettings->Values;
   getLocalDir();
   SetID();
   initGame(m_id);
   String ^ value = safe_cast<String ^>(m_localSettings->Values->Lookup("Mute"));
   if (value)
   {
      if (value == "Off")
      {
         setMute(false);
      }
      else
      {
         setMute(true);
      }
   }
   else
   {
      setMute(false);
      m_localValues->Insert("Mute", dynamic_cast<PropertyValue ^>(PropertyValue::CreateString("Off")));
   }

   // Create renderer.
   m_renderer = ref new Renderer();
   m_renderer->Initialize(
      Window::Current->CoreWindow,
      SwapChainPanel,
      DisplayProperties::LogicalDpi
      );

   // Register event handlers.
   Window::Current->CoreWindow->CharacterReceived +=
      ref new TypedEventHandler<CoreWindow ^, CharacterReceivedEventArgs ^>(this, &DirectXPage::OnCharacterReceived);

   Window::Current->CoreWindow->KeyDown +=
      ref new TypedEventHandler<CoreWindow ^, KeyEventArgs ^>(this, &DirectXPage::OnKeyDown);

   Window::Current->CoreWindow->SizeChanged +=
      ref new TypedEventHandler<CoreWindow ^, WindowSizeChangedEventArgs ^>(this, &DirectXPage::OnWindowSizeChanged);

   DisplayProperties::LogicalDpiChanged +=
      ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnLogicalDpiChanged);

   DisplayProperties::OrientationChanged +=
      ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnOrientationChanged);

   DisplayProperties::DisplayContentsInvalidated +=
      ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnDisplayContentsInvalidated);

   m_renderingEventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object ^>(this, &DirectXPage::OnRendering));

   // Initialize touch.
   m_touch         = ref new TouchCapabilities();
   m_pointerActive = false;

   // Virtual keyboard.
   m_keyboardUp          = false;
   m_keyboardIconVisible = false;
   m_keyboardIconFocus   = false;
   m_inputPaneHelper     = ref new InputPaneHelper();
   m_inputPaneHelper->SubscribeToKeyboard(true);
   m_inputPaneHelper->AddShowingHandler(KeyboardTextBox, ref new InputPaneShowingHandler(this, &DirectXPage::KeyboardUp));
   m_inputPaneHelper->SetHidingHandler(ref new InputPaneHidingHandler(this, &DirectXPage::KeyboardDown));

   // Register for settings commands requested.
   m_settingsEventRegistrationToken =
      SettingsPane::GetForCurrentView()->CommandsRequested +=
         ref new TypedEventHandler<SettingsPane ^, SettingsPaneCommandsRequestedEventArgs ^> (this, &DirectXPage::OnSettingsCommandsRequested);
   m_isSettingsEventRegistered = true;

   // Start timer.
   m_timer = ref new Timer();
}


void DirectXPage::SetID()
{
   // ID exists?
   m_id           = NULL;
   String ^ value = safe_cast<String ^>(m_localSettings->Values->Lookup("ID"));
   if (value)
   {
      wstring w = value->Data();
      string  s;
      s.assign(w.begin(), w.end());
      int len = strlen(s.c_str());
      m_id = new char[len + 1];
      strcpy(m_id, s.c_str());
   }
   else
   {
      // Create ID.
      GUID guid;
      CoCreateGuid(&guid);
      OLECHAR *bstrGuid;
      StringFromCLSID(guid, &bstrGuid);
      wstring w = bstrGuid;
      string  s;
      s.assign(w.begin(), w.end());
      int len = strlen(s.c_str());
      m_id = new char[len + 1];
      strcpy(m_id, s.c_str());
      String ^ id = ref new String(w.c_str());
      m_localValues->Insert("ID", dynamic_cast<PropertyValue ^>(PropertyValue::CreateString(id)));
      ::CoTaskMemFree(bstrGuid);
   }
}


void DirectXPage::OnPointerPressed(Object ^ sender, PointerRoutedEventArgs ^ args)
{
   PointerPoint ^ pointerPoint = PointerPoint::GetCurrentPoint(
      args->GetCurrentPoint(nullptr)->PointerId, ref new ObjectTransform());
   m_beginPosition = m_lastPosition = pointerPoint->Position;
   m_pointerActive = true;
}


void DirectXPage::OnPointerMoved(Object ^ sender, PointerRoutedEventArgs ^ args)
{
   if (m_pointerActive)
   {
      PointerPoint ^ pointerPoint = PointerPoint::GetCurrentPoint(
         args->GetCurrentPoint(nullptr)->PointerId, ref new ObjectTransform());

      // Swiping?
      if (!pointerPoint->Position.Equals(m_lastPosition))
      {
         OnSwipe(pointerPoint->Position);
      }
   }
}


// Tap distance.
const float DirectXPage::TAP_DISTANCE = 5.0f;

void DirectXPage::OnPointerReleased(Object ^ sender, PointerRoutedEventArgs ^ args)
{
   m_pointerActive             = false;
   PointerPoint ^ pointerPoint = PointerPoint::GetCurrentPoint(
      args->GetCurrentPoint(nullptr)->PointerId, ref new ObjectTransform());

   if ((fabs(pointerPoint->Position.X - m_lastPosition.X) <= TAP_DISTANCE) &&
       (fabs(pointerPoint->Position.Y - m_lastPosition.Y) <= TAP_DISTANCE))
   {
      OnTapped(pointerPoint);
   }
   else
   {
      OnSwipe(pointerPoint->Position);
   }
}


void DirectXPage::OnTapped(PointerPoint ^ pointerPoint)
{
   int c;

   // Show virtual keyboard "icon"?
   if (m_touch->TouchPresent && !m_keyboardIconVisible)
   {
      m_keyboardIconVisible       = true;
      KeyboardTextBox->Visibility = Windows::UI::Xaml::Visibility::Visible;
      return;
   }

   // Hiding keyboard?
   if (m_keyboardUp)
   {
      return;
   }

   if (m_renderer->InOverview())
   {
      // Leave overview.
      m_renderer->FPview();
      m_renderer->Activate();
   }
   else
   {
      if (spacePrompt())
      {
         // Shortcut for space key.
         HandleCharacter(' ');
      }
      else if (m_renderer->PromptPresent())
      {
         // Escape from prompt.
         HandleCharacter(27);
      }
      else if ((c = m_renderer->PickCharacter(pointerPoint->RawPosition)) != 0)
      {
         // Identify character.
         HandleCharacter('/');
         HandleCharacter(c);
      }
      else
      {
         // Go to overview.
         m_renderer->Overview();
         m_renderer->Activate();
      }
   }
}


// Swipe move/turn parameters.
const float DirectXPage::SWIPE_MOVE_SCALE = 0.1f;
const float DirectXPage::SWIPE_TURN_SCALE = 0.2f;

void DirectXPage::OnSwipe(Point toPosition)
{
   if (m_renderer->InOverview())
   {
      return;
   }

   float moveX        = toPosition.X - m_lastPosition.X;
   float moveY        = toPosition.Y - m_lastPosition.Y;
   float windowWidth  = Window::Current->CoreWindow->Bounds.Width;
   float windowHeight = Window::Current->CoreWindow->Bounds.Height;
   float moveDistance = 0.0f;
   float turnDistance = 0.0f;
   if (windowWidth < windowHeight)
   {
      moveDistance = windowWidth * SWIPE_MOVE_SCALE;
      turnDistance = windowWidth * SWIPE_TURN_SCALE;
   }
   else
   {
      moveDistance = windowHeight * SWIPE_MOVE_SCALE;
      turnDistance = windowHeight * SWIPE_TURN_SCALE;
   }
   while (fabs(moveX) >= turnDistance || fabs(moveY) > moveDistance)
   {
      if (moveY >= moveDistance)
      {
         moveY            -= moveDistance;
         m_lastPosition.Y += moveDistance;
         HandleCharacter('j');
      }
      else if (moveY <= -moveDistance)
      {
         moveY            += moveDistance;
         m_lastPosition.Y -= moveDistance;
         HandleCharacter('k');
      }
      if (moveX >= turnDistance)
      {
         moveX            -= turnDistance;
         m_lastPosition.X += turnDistance;
         HandleKey(VirtualKey::Right, false);
      }
      else if (moveX <= -turnDistance)
      {
         moveX            += turnDistance;
         m_lastPosition.X -= turnDistance;
         HandleKey(VirtualKey::Left, false);
      }
   }
}


void DirectXPage::OnCharacterReceived(CoreWindow ^ sender, CharacterReceivedEventArgs ^ args)
{
   if (!m_keyboardIconFocus)
   {
      HandleCharacter(args->KeyCode);
   }
}


void DirectXPage::KeyboardTextChanged(Object ^ sender, TextChangedEventArgs ^ args)
{
   Platform::String ^ text = KeyboardTextBox->Text;
   if ((text != nullptr) && (text->Length() > 0))
   {
      KeyboardTextBox->Text = "";
      wstring w = text->Data();
      string  s;
      s.assign(w.begin(), w.end());
      const char *c = s.c_str();
      for (int i = 0; c[i] != '\0'; i++)
      {
         HandleCharacter(c[i]);
      }
   }
}


void DirectXPage::KeyboardKeyDown(Object ^ sender, KeyEventArgs ^ args)
{
   VirtualKey keyCode = args->VirtualKey;

   if (keyCode == VirtualKey::Enter)
   {
      HandleCharacter('\n');
   }
   if (keyCode == VirtualKey::Back)
   {
      HandleCharacter('\b');
   }
}


void DirectXPage::KeyboardIconGotFocus(CoreWindow ^ sender, RoutedEventArgs ^ args)
{
   m_keyboardIconFocus = true;
}


void DirectXPage::KeyboardIconLostFocus(CoreWindow ^ sender, RoutedEventArgs ^ args)
{
   m_keyboardIconFocus = false;
}


void DirectXPage::HandleCharacter(int keyChar)
{
   int i;

   if (m_renderer->InOverview())
   {
      return;
   }

   // Clear prompt for space signal.
   spacePrompt();

   // Wait for input request?
   lockInput();
   if (getInputReq() == 0)
   {
      waitInput();
   }

   switch (getInputReq())
   {
   // Request for character.
   case 1:
      setInputChar(keyChar);
      setInputReq(0);
      signalInput();
      break;

   // Request for string.
   case 2:
      switch (keyChar)
      {
      // Return
      case 13:
         setInputReq(0);
         signalInput();
         break;

      // Backspace
      case 8:
         i = getInputIndex();
         if (i > 0)
         {
            i--;
            setInputIndex(i);
            bufferInputChar(i, '\0');
         }
         break;

      default:
         i = getInputIndex();
         if (i < getBufferSize() - 1)
         {
            bufferInputChar(i, keyChar);
            i++;
            setInputIndex(i);
            bufferInputChar(i, '\0');
         }
         break;
      }
      break;

   default:
      break;
   }
   unlockInput();
   m_renderer->Activate();
}


void DirectXPage::OnKeyDown(CoreWindow ^ sender, KeyEventArgs ^ args)
{
   bool shift;

   VirtualKey keyCode = args->VirtualKey;

   if (sender->GetKeyState(VirtualKey::Shift) != CoreVirtualKeyStates::None)
   {
      shift = true;
   }
   else
   {
      shift = false;
   }
   HandleKey(keyCode, shift);
}


void DirectXPage::HandleKey(VirtualKey keyCode, bool shift)
{
   int  i, d;
   char keyChar;

   if (m_renderer->InOverview())
   {
      return;
   }

   keyChar = 0;
   switch (keyCode)
   {
   case VirtualKey::Delete:
      keyChar = (char)127;
      break;

   case VirtualKey::Up:
      keyChar = 'k';
      break;

   case VirtualKey::Down:
      keyChar = 'j';
      break;

   case VirtualKey::Right:
      d = getDir();
      d--;
      if (d < 0)
      {
         d += 8;
      }
      if (shift)
      {
         d--;
         if (d < 0)
         {
            d += 8;
         }
      }
      setDir(d);
      m_renderer->Activate();
      return;

   case VirtualKey::Left:
      d = getDir();
      d = (d + 1) % 8;
      if (shift)
      {
         d = (d + 1) % 8;
      }
      setDir(d);
      m_renderer->Activate();
      return;

   default:
      return;
   }

   // Clear prompt for space signal.
   spacePrompt();

   // Wait for input request?
   lockInput();
   if (getInputReq() == 0)
   {
      waitInput();
   }

   switch (getInputReq())
   {
   // Request for character.
   case 1:
      setInputChar(keyChar);
      setInputReq(0);
      signalInput();
      break;

   // Request for string.
   case 2:
      switch (keyChar)
      {
      // Delete
      case 127:
         i = getInputIndex();
         if (i > 0)
         {
            i--;
            setInputIndex(i);
            bufferInputChar(i, '\0');
         }
         break;

      default:
         i = getInputIndex();
         if (i < getBufferSize() - 1)
         {
            bufferInputChar(i, keyChar);
            i++;
            setInputIndex(i);
            bufferInputChar(i, '\0');
         }
         break;
      }
      break;

   default:
      break;
   }
   unlockInput();
   m_renderer->Activate();
}


void DirectXPage::KeyboardUp(Object ^ sender, InputPaneVisibilityEventArgs ^ e)
{
   m_keyboardUp = true;
}


void DirectXPage::KeyboardDown(InputPane ^ sender, InputPaneVisibilityEventArgs ^ e)
{
   m_keyboardUp = false;
}


void DirectXPage::OnWindowSizeChanged(CoreWindow ^ sender, WindowSizeChangedEventArgs ^ args)
{
   if (ApplicationView::Value == ApplicationViewState::Snapped)
   {
      SnappedOverlay->Visibility = Windows::UI::Xaml::Visibility::Visible;
   }
   else
   {
      SnappedOverlay->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
      m_renderer->UpdateForWindowSizeChange();
      m_renderer->Activate();
   }
}


void DirectXPage::OnLogicalDpiChanged(Object ^ sender)
{
   m_renderer->SetDpi(DisplayProperties::LogicalDpi);
   m_renderer->Activate();
}


void DirectXPage::OnOrientationChanged(Object ^ sender)
{
   m_renderer->UpdateForWindowSizeChange();
   m_renderer->Activate();
}


void DirectXPage::OnDisplayContentsInvalidated(Object ^ sender)
{
   m_renderer->ValidateDevice();
   m_renderer->Activate();
}


void DirectXPage::OnRendering(Object ^ sender, Object ^ args)
{
   if (m_renderer->IsActive())
   {
      m_timer->Update();
      m_renderer->Render();
      m_renderer->Present();
   }
}


void DirectXPage::OnSuspending(IPropertySet ^ state)
{
   m_renderer->SaveInternalState(state);

   // Wait for game thread to block on input.
   lockInput();
   if (getInputReq() == 0)
   {
      waitInput();
   }
   unlockInput();

   // Save game.
   saveGame();
}


void DirectXPage::OnResuming(IPropertySet ^ state)
{
   m_renderer->LoadInternalState(state);
}


// View manual button click.
void DirectXPage::ManualButtonClick(Object ^ sender, RoutedEventArgs ^ args)
{
   Window::Current->Content = ref new ManualPage(this);
   Window::Current->Activate();
}


// Commands button click.
void DirectXPage::CommandsButtonClick(Object ^ sender, RoutedEventArgs ^ args)
{
   if (m_keyboardUp || m_renderer->InOverview() ||
       spacePrompt() || m_renderer->PromptPresent())
   {
      return;
   }
   HandleCharacter('?');
   HandleCharacter('*');
}


// Pack button click.
void DirectXPage::PackButtonClick(Object ^ sender, RoutedEventArgs ^ args)
{
   if (m_keyboardUp || m_renderer->InOverview() ||
       spacePrompt() || m_renderer->PromptPresent())
   {
      return;
   }
   HandleCharacter('i');
}


void DirectXPage::OnSettingsCommandsRequested(SettingsPane ^ settingsPane, SettingsPaneCommandsRequestedEventArgs ^ eventArgs)
{
   UICommandInvokedHandler ^ handler = ref new UICommandInvokedHandler(this, &DirectXPage::OnSettingsCommand);
   eventArgs->Request->ApplicationCommands->Clear();
   SettingsCommand ^ aboutCommand = ref new SettingsCommand("MutePage", "Mute", handler);
   eventArgs->Request->ApplicationCommands->Append(aboutCommand);
   SettingsCommand ^ helpCommand = ref new SettingsCommand("ManualPage", "Help", handler);
   eventArgs->Request->ApplicationCommands->Append(helpCommand);
}


void DirectXPage::OnSettingsCommand(IUICommand ^ command)
{
   SettingsCommand ^ settingsCommand = safe_cast<SettingsCommand ^>(command);
   UserControl ^ settingPane         = nullptr;
   if (settingsCommand->Label == "Mute")
   {
      settingPane = ref new MutePage();
   }
   else if (settingsCommand->Label == "Help")
   {
      settingPane = ref new HelpPage();
   }

   if (settingPane)
   {
      Windows::Foundation::Rect windowBounds = Windows::UI::Xaml::Window::Current->Bounds;
      double width = (double)SETTINGS_POPUP_WIDTH;
      m_settingsPopup = ref new Popup();
      m_settingsPopup->IsLightDismissEnabled = true;
      m_settingsPopup->Width  = width;
      m_settingsPopup->Height = windowBounds.Height;
      m_settingsPopup->Child  = settingPane;
      settingPane->Width      = width;
      settingPane->Height     = windowBounds.Height;
      m_settingsPopup->SetValue(Canvas::LeftProperty, windowBounds.Width - width);
      m_settingsPopup->SetValue(Canvas::TopProperty, 0);
      m_settingsPopup->IsOpen = true;
   }
}


void DirectXPage::OnNavigatedFrom(NavigationEventArgs ^ e)
{
   // Added to make sure the event handler for CommandsRequested in cleaned up before other scenarios
   if (m_isSettingsEventRegistered)
   {
      SettingsPane::GetForCurrentView()->CommandsRequested -= m_settingsEventRegistrationToken;
      m_isSettingsEventRegistered = false;
   }
}


// Logging.
extern "C"
{
const char *LogfileName = "blackguard.log";
bool       InitLog      = true;
void Log(char *entry)
{
   std::wstring w;

   try
   {
      Windows::Storage::StorageFolder ^ f = (Windows::Storage::ApplicationData::Current)->LocalFolder;
      w = f->Path->Data();
   }
   catch (Platform::COMException ^ e)
   {
      return;
   }
   std::string s;
   s.assign(w.begin(), w.end());
   char buf[LINLEN];
   sprintf(buf, "%s/%s", s.c_str(), LogfileName);
   FILE *fp;
   if (InitLog)
   {
      InitLog = false;
      fp      = fopen(buf, "w");
   }
   else
   {
      fp = fopen(buf, "a");
   }
   fprintf(fp, "%s\n", entry);
   fclose(fp);
}
};

ObjectTransform::ObjectTransform()
{
   _matrix = D2D1::Matrix3x2F::Identity();
}


bool ObjectTransform::TryTransform(_In_ Windows::Foundation::Point inPoint, _Out_ Windows::Foundation::Point *outPoint)
{
   D2D1_POINT_2F pt = _matrix.TransformPoint(D2D1::Point2F(inPoint.X, inPoint.Y));

   outPoint->X = pt.x;
   outPoint->Y = pt.y;
   return(TRUE);
}


Windows::Foundation::Rect ObjectTransform::TransformBounds(_In_ Windows::Foundation::Rect rect)
{
   Windows::Foundation::Point center(rect.X + rect.Width / 2, rect.Y + rect.Height / 2);
   float scale = sqrt(fabs(_matrix.Determinant()));

   if (TryTransform(center, &center) && (scale > 0))
   {
      rect.Width  *= scale;
      rect.Height *= scale;
      rect.X       = center.X - rect.Width / 2;
      rect.Y       = center.Y - rect.Height / 2;
   }
   return(rect);
}


Windows::UI::Input::IPointerPointTransform ^ ObjectTransform::Inverse::get()
{
   D2D1::Matrix3x2F matrixInv = _matrix;

   if (matrixInv.Invert())
   {
      ObjectTransform ^ transform = ref new ObjectTransform();
      transform->SetMatrix(matrixInv);
      return(transform);
   }
   else
   {
      return(nullptr);
   }
}
