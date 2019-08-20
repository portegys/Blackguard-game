#pragma once

#include "Timer.h"
#include "RogueInterface.hpp"
#include <hash_map>

using namespace std;


ref class TextOverlay
{
internal:

   TextOverlay();

   void Initialize(
      _In_ ID2D1Device        *d2dDevice,
      _In_ ID2D1DeviceContext *d2dContext,
      _In_ IWICImagingFactory *wicFactory,
      _In_ IDWriteFactory     *dwriteFactory
      );

   void ResetDirectXResources();

   void UpdateForWindowSizeChange(Windows::Foundation::Rect                       windowBounds,
                                  Windows::Graphics::Display::DisplayOrientations orientation);

   bool Render(D2D1::Matrix3x2F& orientationTransform2D);

   // Viewing controls.
   typedef enum View
   {
      FPVIEW, OVERVIEW
   } VIEW;
   VIEW view;
   bool promptPresent;

   // Player position.
   int playerX, playerZ;

private:

   Microsoft::WRL::ComPtr<ID2D1Factory1>          m_d2dFactory;
   Microsoft::WRL::ComPtr<ID2D1Device>            m_d2dDevice;
   Microsoft::WRL::ComPtr<ID2D1DeviceContext>     m_d2dContext;
   Microsoft::WRL::ComPtr<IDWriteFactory>         m_dwriteFactory;
   Microsoft::WRL::ComPtr<IWICImagingFactory>     m_wicFactory;
   Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock> m_stateBlock;

   Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>               m_whiteBrush;
   hash_map<char, Microsoft::WRL::ComPtr<IDWriteTextLayout> > m_characterLayouts;
   bool m_drawOverlay;
   Windows::Foundation::Rect m_windowBounds;
   Windows::Graphics::Display::DisplayOrientations m_orientation;
   float m_fontSize;
   void DrawText(char *text, int x, int y, int clen, int llen);

   // Welcome.
   enum { WELCOME_SECONDS = 5 };
   Timer ^ m_welcomeTimer;
   bool  m_welcome;
   float m_welcomeFontSize;
   Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>       m_neonGreenBrush;
   vector<Microsoft::WRL::ComPtr<IDWriteTextLayout> > m_welcomeLayouts;
};
