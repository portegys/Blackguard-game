//
// DirectXPage.xaml.h
// Declaration of the BlankPage.xaml class.
//

#pragma once

#include "DirectXPage.g.h"
#include "InputPaneHelper.h"
#include "Renderer.h"

namespace Blackguard
{
[Windows::Foundation::Metadata::WebHostHidden]
public ref class DirectXPage sealed
{
public:

   DirectXPage();

   void OnSuspending(Windows::Foundation::Collections::IPropertySet ^ state);
   void OnResuming(Windows::Foundation::Collections::IPropertySet ^ state);

protected:

   virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e) override;

private:

   // Events.
   void OnWindowSizeChanged(Windows::UI::Core::CoreWindow ^ sender, Windows::UI::Core::WindowSizeChangedEventArgs ^ args);
   void OnLogicalDpiChanged(Platform::Object ^ sender);
   void OnOrientationChanged(Platform::Object ^ sender);
   void OnDisplayContentsInvalidated(Platform::Object ^ sender);

   // Touch.
   Windows::Devices::Input::TouchCapabilities ^ m_touch;
   void OnPointerPressed(Platform::Object ^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ args);
   void OnPointerMoved(Platform::Object ^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ args);
   void OnPointerReleased(Platform::Object ^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ args);
   void OnTapped(Windows::UI::Input::PointerPoint ^ pointerPoint);
   void OnSwipe(Windows::Foundation::Point toPosition);

   bool m_pointerActive;
   Windows::Foundation::Point m_beginPosition;
   Windows::Foundation::Point m_lastPosition;
   static const float         TAP_DISTANCE;
   static const float         SWIPE_MOVE_SCALE;
   static const float         SWIPE_TURN_SCALE;

   // Keyboard.
   void OnCharacterReceived(Windows::UI::Core::CoreWindow ^ sender, Windows::UI::Core::CharacterReceivedEventArgs ^ args);
   void KeyboardTextChanged(Object ^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs ^ args);
   void KeyboardKeyDown(Object ^ sender, Windows::UI::Core::KeyEventArgs ^ args);
   void HandleCharacter(int keyChar);

   void OnKeyDown(Windows::UI::Core::CoreWindow ^ sender, Windows::UI::Core::KeyEventArgs ^ args);
   void HandleKey(Windows::System::VirtualKey keyCode, bool shift);

   void KeyboardUp(Object ^ sender, InputPaneVisibilityEventArgs ^ args);
   void KeyboardDown(InputPane ^ sender, InputPaneVisibilityEventArgs ^ args);
   void KeyboardIconGotFocus(Windows::UI::Core::CoreWindow ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ args);
   void KeyboardIconLostFocus(Windows::UI::Core::CoreWindow ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ args);
   bool m_keyboardUp;
   bool m_keyboardIconVisible;
   bool m_keyboardIconFocus;
   InputPaneHelper ^ m_inputPaneHelper;

   // Rendering.
   Renderer ^ m_renderer;
   void OnRendering(Object ^ sender, Object ^ args);
   Windows::Foundation::EventRegistrationToken m_renderingEventToken;

   // ID.
   char *m_id;
   void SetID();

   // Settings.
   void OnSettingsCommandsRequested(Windows::UI::ApplicationSettings::SettingsPane ^ settingsPane,
                                    Windows::UI::ApplicationSettings::SettingsPaneCommandsRequestedEventArgs ^ eventArgs);
   void OnSettingsCommand(Windows::UI::Popups::IUICommand ^ command);
   bool m_isSettingsEventRegistered;
   Windows::Foundation::EventRegistrationToken m_settingsEventRegistrationToken;
   Windows::UI::Xaml::Controls::Primitives::Popup ^ m_settingsPopup;
   enum { SETTINGS_POPUP_WIDTH = 346 };
   Windows::Storage::ApplicationDataContainer ^ m_localSettings;
   Windows::Foundation::Collections::IPropertySet ^ m_localValues;

   // Button callbacks.
   void ManualButtonClick(Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ args);
   void CommandsButtonClick(Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ args);
   void PackButtonClick(Object ^ sender, Windows::UI::Xaml::RoutedEventArgs ^ args);

   // Update timer.
   Timer ^ m_timer;
};
}

ref class ObjectTransform sealed : public Windows::UI::Input::IPointerPointTransform
{
public:
   ObjectTransform();

   virtual bool TryTransform(_In_ Windows::Foundation::Point inPoint, _Out_ Windows::Foundation::Point *outPoint);
   virtual Windows::Foundation::Rect TransformBounds(_In_ Windows::Foundation::Rect rect);

   virtual property Windows::UI::Input::IPointerPointTransform ^ Inverse { Windows::UI::Input::IPointerPointTransform ^ get(); }

private:
   void SetMatrix(D2D1::Matrix3x2F matrix) { _matrix = matrix; }

   D2D1::Matrix3x2F _matrix;  // client (global) to object parent coordinate system transform
};

// Logging.
extern "C" {
void Log(char *entry);
};
