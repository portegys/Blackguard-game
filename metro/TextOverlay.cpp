#include "pch.h"
#include "TextOverlay.h"

using namespace Windows::Graphics::Display;
using namespace D2D1;

TextOverlay::TextOverlay()
{
   view                 = View::FPVIEW;
   promptPresent        = false;
   playerX              = playerZ = -1;
   m_drawOverlay        = false;
   m_windowBounds.Width = m_windowBounds.Height = -1.0f;
   m_orientation        = DisplayOrientations::None;
   m_welcomeTimer       = ref new Timer();
   m_welcome            = true;
}


void TextOverlay::Initialize(
   _In_ ID2D1Device        *d2dDevice,
   _In_ ID2D1DeviceContext *d2dContext,
   _In_ IWICImagingFactory *wicFactory,
   _In_ IDWriteFactory     *dwriteFactory
   )
{
   m_wicFactory    = wicFactory;
   m_dwriteFactory = dwriteFactory;
   m_d2dDevice     = d2dDevice;
   m_d2dContext    = d2dContext;

   ComPtr<ID2D1Factory> factory;
   d2dDevice->GetFactory(&factory);

   DX::ThrowIfFailed(
      factory.As(&m_d2dFactory)
      );

   ResetDirectXResources();
}


void TextOverlay::ResetDirectXResources()
{
   m_drawOverlay = false;

   DX::ThrowIfFailed(
      m_d2dContext->CreateSolidColorBrush(ColorF(ColorF::White), &m_whiteBrush)
      );

   DX::ThrowIfFailed(
      m_d2dContext->CreateSolidColorBrush(ColorF(0x7FFF00, 1.0f), &m_neonGreenBrush)
      );

   DX::ThrowIfFailed(
      m_d2dFactory->CreateDrawingStateBlock(&m_stateBlock)
      );

   m_windowBounds.Width = m_windowBounds.Height = -1.0f;
   m_orientation        = DisplayOrientations::None;
}


// Welcome text.
static const wchar_t *welcomeText[] =
{
   L"Swipe to move",
   L"Tap for keyboard",
   L"Type ? for commands",
   L"Type / or tap object to identify",
   L"dialectek.com",
   NULL
};

void TextOverlay::UpdateForWindowSizeChange(Windows::Foundation::Rect windowBounds,
                                            DisplayOrientations       orientation)
{
   if ((windowBounds.Width != m_windowBounds.Width) ||
       (windowBounds.Height != m_windowBounds.Height) ||
       (orientation != m_orientation))
   {
      m_drawOverlay  = false;
      m_windowBounds = windowBounds;
      m_orientation  = orientation;
      float width  = windowBounds.Width;
      float height = windowBounds.Height;

      m_fontSize = width / (float)TEXT_WIDTH;
      if (m_fontSize > (height / (float)TEXT_HEIGHT))
      {
         m_fontSize = height / (float)TEXT_HEIGHT;
      }
      ComPtr<IDWriteTextFormat> characterFormat;
      DX::ThrowIfFailed(
         m_dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_LIGHT,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            m_fontSize,
            L"en-US",
            &characterFormat
            )
         );

      DX::ThrowIfFailed(
         characterFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)
         );

      DX::ThrowIfFailed(
         characterFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
         );

      m_characterLayouts.clear();
      Microsoft::WRL::ComPtr<IDWriteTextLayout> characterLayout;
      wchar_t c[2];
      c[1] = 0;
      for (int i = 32; i <= 126; i++)
      {
         c[0] = (char)i;
         DX::ThrowIfFailed(
            m_dwriteFactory->CreateTextLayout(
               c,
               1,
               characterFormat.Get(),
               m_fontSize,
               m_fontSize,
               &characterLayout
               )
            );
         m_characterLayouts[(char)i] = characterLayout;
      }

      m_welcomeFontSize = width / ((float)TEXT_WIDTH / 2.0f);
      if (m_welcomeFontSize > (height / ((float)TEXT_HEIGHT / 2.0f)))
      {
         m_welcomeFontSize = height / ((float)TEXT_HEIGHT / 2.0f);
      }
      ComPtr<IDWriteTextFormat> welcomeFormat;
      DX::ThrowIfFailed(
         m_dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_LIGHT,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            m_welcomeFontSize,
            L"en-US",
            &welcomeFormat
            )
         );

      DX::ThrowIfFailed(
         welcomeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)
         );

      DX::ThrowIfFailed(
         welcomeFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR)
         );

      m_welcomeLayouts.clear();
      Microsoft::WRL::ComPtr<IDWriteTextLayout> stringLayout;
      for (int i = 0; welcomeText[i] != NULL; i++)
      {
         int len = wcslen(welcomeText[i]);
         DX::ThrowIfFailed(
            m_dwriteFactory->CreateTextLayout(
               welcomeText[i],
               len,
               welcomeFormat.Get(),
               m_welcomeFontSize * (float)len,
               m_welcomeFontSize,
               &stringLayout
               )
            );
         m_welcomeLayouts.push_back(stringLayout);
      }
   }
   m_drawOverlay = true;
}


// Render text.
bool TextOverlay::Render(Matrix3x2F& orientationTransform2D)
{
   int  i, j, k, clen, llen;
   bool draw, post;
   char c;

   Microsoft::WRL::ComPtr<IDWriteTextLayout> characterLayout;

   if (!m_drawOverlay)
   {
      return(false);
   }

   m_d2dContext->SaveDrawingState(m_stateBlock.Get());
   m_d2dContext->BeginDraw();
   m_d2dContext->SetTransform(orientationTransform2D);

   char *msg = getDisplaymsg();

   draw = true;
   clen = (int)(m_windowBounds.Width / TEXT_WIDTH);
   llen = (int)(m_windowBounds.Height / TEXT_HEIGHT);
   if (currentWindow() && !m_welcome)
   {
      // Check for presence of possible prompt.
      c = getWindowChar(0, 0);
      if (c != ' ')
      {
         promptPresent = true;
      }
      else
      {
         promptPresent = false;
      }

      // Check for special displays.
      post = false;
      for (i = 1, k = TEXT_HEIGHT - 2; i < k; i++)
      {
         for (j = 0; j < TEXT_WIDTH; j++)
         {
            c = getWindowChar(i, j);

            if (showtext == 0)
            {
               // Player visible?
               if (c == '@')
               {
                  playerX = j;
                  playerZ = i;
                  draw    = false;
               }

               // Could be in trading post?
               if ((c >= '0') && (c <= '4'))
               {
                  post = true;
               }
            }
         }
         if (!draw && !isBlind() && post)
         {
            draw = true;
            view = View::FPVIEW;
         }
      }

      // Force text scene for level overview?
      if (!draw && (view == View::OVERVIEW))
      {
         DrawText("Level overview:", 0, 0, clen, llen);
         if (!isBlind())
         {
            draw = true;
         }
      }

      if (msg != NULL)
      {
         DrawText(msg, 0, 0, clen, llen);
      }

      k = 0;
      for (i = 0; i < 3; i++)
      {
         switch (i)
         {
         case 0:
            k = 0;
            break;

         case 1:
            k = TEXT_HEIGHT - 2;
            break;

         case 2:
            k = TEXT_HEIGHT - 1;
            break;
         }
         for (j = 0; j < TEXT_WIDTH; j++)
         {
            c = getWindowChar(k, j);
            if ((c < 32) || (c > 126))
            {
               c = ' ';
            }
            characterLayout = m_characterLayouts[c];
            m_d2dContext->DrawTextLayout(
               Point2F((float)(j * clen), (float)(k * llen)),
               characterLayout.Get(),
               m_whiteBrush.Get()
               );
         }
      }

      // Draw text "scene"?
      if (draw)
      {
         for (i = 1, k = TEXT_HEIGHT - 2; i < k; i++)
         {
            for (j = 0; j < TEXT_WIDTH; j++)
            {
               c = getWindowChar(i, j);
               if (c == ';')
               {
                  c = '@';
               }
               if ((c < 32) || (c > 126))
               {
                  c = ' ';
               }
               characterLayout = m_characterLayouts[c];
               m_d2dContext->DrawTextLayout(
                  Point2F((float)(j * clen), (float)(i * llen)),
                  characterLayout.Get(),
                  m_whiteBrush.Get()
                  );
            }
         }
      }
   }
   else
   {
      if (msg != NULL)
      {
         DrawText(msg, 0, 0, clen, llen);
      }
   }

   // Welcome?
   if (m_welcome)
   {
      for (i = 0, j = (int)m_welcomeLayouts.size(); i < j; i++)
      {
         m_d2dContext->DrawTextLayout(
            Point2F(0.0f, (float)(i + 2) * m_welcomeFontSize),
            m_welcomeLayouts[i].Get(),
            m_neonGreenBrush.Get()
            );
      }
      m_welcomeTimer->Update();
      if (m_welcomeTimer->Total >= (float)WELCOME_SECONDS)
      {
         m_welcome = false;
      }
   }

   m_d2dContext->EndDraw();
   m_d2dContext->RestoreDrawingState(m_stateBlock.Get());

   return(draw);
}


void TextOverlay::DrawText(char *text, int x, int y, int clen, int llen)
{
   int  i, j;
   char c;

   Microsoft::WRL::ComPtr<IDWriteTextLayout> characterLayout;

   for (i = 0, j = strlen(text); i < j; i++)
   {
      c = text[i];
      if ((c < 32) || (c > 126))
      {
         c = ' ';
      }
      characterLayout = m_characterLayouts[c];
      m_d2dContext->DrawTextLayout(
         Point2F((float)((x + i) * clen), (float)(y * llen)),
         characterLayout.Get(),
         m_whiteBrush.Get()
         );
   }
}
