#include "pch.h"
#include "Scene.h"

using namespace Windows::Graphics::Display;

Scene::Scene(vector<Platform::String ^> textureFileNames)
{
   m_textureFileNames = textureFileNames;
}


void Scene::Initialize(
   _In_ ID3D11Device1        *d3dDevice,
   _In_ ID3D11DeviceContext1 *d3dContext,
   _In_ ID2D1Device          *d2dDevice,
   _In_ ID2D1DeviceContext   *d2dContext,
   _In_ IDWriteFactory       *dwriteFactory
   )
{
   m_d3dDevice     = d3dDevice;
   m_d3dContext    = d3dContext;
   m_d2dDevice     = d2dDevice;
   m_d2dContext    = d2dContext;
   m_dwriteFactory = dwriteFactory;

   // create the constant buffer for updating model and camera data
   CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
   DX::ThrowIfFailed(
      m_d3dDevice->CreateBuffer(
         &constantBufferDesc,
         nullptr,
         &m_constantBuffer
         )
      );

   // Create transparency blends.
   ZeroMemory(&m_transparentStateDesc, sizeof(D3D11_BLEND_DESC));
   m_transparentStateDesc.RenderTarget[0].BlendEnable           = TRUE;
   m_transparentStateDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
   m_transparentStateDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
   m_transparentStateDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
   m_transparentStateDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
   m_transparentStateDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
   m_transparentStateDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
   m_transparentStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
   m_d3dDevice->CreateBlendState(&m_transparentStateDesc, &m_transparentState);
   ZeroMemory(&m_opaqueStateDesc, sizeof(D3D11_BLEND_DESC));
   m_opaqueStateDesc.RenderTarget[0].BlendEnable           = FALSE;
   m_opaqueStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
   m_d3dDevice->CreateBlendState(&m_opaqueStateDesc, &m_opaqueState);

   // Create shapes.
   CreateShapes();

   // Create textures.
   Loader ^ loader = ref new Loader(m_d3dDevice.Get());
   CreateTextures(loader);

   // Create the camera.
   CreateCamera();
}


void Scene::CreateShapes()
{
   m_shapes = ref new SceneShapes(m_d3dDevice.Get());

   m_shapes->CreateBlock(
      &m_blockVertexBuffer,
      &m_blockIndexBuffer,
      nullptr,
      &m_blockIndexCount
      );

   m_shapes->CreateTile(
      &m_tileVertexBuffer,
      &m_tileIndexBuffer,
      nullptr,
      &m_tileIndexCount
      );

   m_shapes->CreateBillboard(
      &m_billboardVertexBuffer,
      &m_billboardIndexBuffer,
      nullptr,
      &m_billboardIndexCount
      );
}


void Scene::CreateTextures(Loader ^ loader)
{
   Microsoft::WRL::ComPtr<ID3D11Texture2D>          fileTexture;
   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> fileTextureSRV;
   Microsoft::WRL::ComPtr<ID3D11Texture2D>          blackTexture;
   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blackTextureSRV;
   Microsoft::WRL::ComPtr<ID3D11Texture2D>          textTexture;
   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textTextureSRV;
   Microsoft::WRL::ComPtr<ID3D11Texture2D>          stagingTextTexture;
   int textureSize = 512;

   // Load file textures.
   for (int i = 0; i < (int)m_textureFileNames.size(); i++)
   {
      loader->LoadTexture(
         m_textureFileNames[i],
         &fileTexture,
         &fileTextureSRV
         );
      m_textures[i]    = fileTexture;
      m_textureSRVs[i] = fileTextureSRV;
   }

   // Create staging texture.
   CD3D11_TEXTURE2D_DESC stagingTextTextureDesc(
      DXGI_FORMAT_B8G8R8A8_UNORM,
      textureSize, // Width
      textureSize, // Height
      1,           // MipLevels
      1,           // ArraySize
      0
      );
   stagingTextTextureDesc.Usage          = D3D11_USAGE_STAGING;
   stagingTextTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

   DX::ThrowIfFailed(
      m_d3dDevice->CreateTexture2D(
         &stagingTextTextureDesc,
         nullptr,
         &stagingTextTexture
         )
      );

   // Create black texture.
   CD3D11_TEXTURE2D_DESC blackTextureDesc(
      DXGI_FORMAT_B8G8R8A8_UNORM,
      textureSize, // Width
      textureSize, // Height
      1,           // MipLevels
      1,           // ArraySize
      D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET
      );

   DX::ThrowIfFailed(
      m_d3dDevice->CreateTexture2D(
         &blackTextureDesc,
         nullptr,
         &blackTexture
         )
      );

   CD3D11_SHADER_RESOURCE_VIEW_DESC blackResourceViewDesc(
      blackTexture.Get(),
      D3D11_SRV_DIMENSION_TEXTURE2D
      );

   DX::ThrowIfFailed(
      m_d3dDevice->CreateShaderResourceView(
         blackTexture.Get(),
         &blackResourceViewDesc,
         &blackTextureSRV
         )
      );

   D3D11_MAPPED_SUBRESOURCE mappedTex;
   DX::ThrowIfFailed(
      m_d3dContext->Map(stagingTextTexture.Get(),
                        D3D11CalcSubresource(0, 0, 1),
                        D3D11_MAP_WRITE, 0,
                        &mappedTex
                        )
      );

   UCHAR *pTexels = (UCHAR *)mappedTex.pData;
   for (UINT row = 0; row < stagingTextTextureDesc.Height; row++)
   {
      UINT rowStart = row * mappedTex.RowPitch;
      for (UINT col = 0; col < stagingTextTextureDesc.Width; col++)
      {
         UINT colStart = col * 4;

         // Set to black
         pTexels[rowStart + colStart + 0] = 0;                // Red
         pTexels[rowStart + colStart + 1] = 0;                // Green
         pTexels[rowStart + colStart + 2] = 0;                // Blue
         pTexels[rowStart + colStart + 3] = 255;              // Alpha=opaque
      }
   }
   m_d3dContext->Unmap(stagingTextTexture.Get(), D3D11CalcSubresource(0, 0, 1));

   m_d3dContext->CopyResource(
      blackTexture.Get(),
      stagingTextTexture.Get()
      );

   m_textures[Black]    = blackTexture;
   m_textureSRVs[Black] = blackTextureSRV;

   // Load character colors.
   unsigned long charColors[NUM_CHARS], i, j;
   for (i = 0; i < NUM_CHARS; i++)
   {
      j = i + CHAR_OFFSET;
      if (((j >= (int)'a') && (j <= (int)'z')) ||
          ((j >= (int)'A') && (j <= (int)'Z')))
      {
         charColors[i] = ((unsigned long)255 << 16);
      }
      else
      {
         charColors[i] = ((unsigned long)255 << 8);
      }
   }
   ReaderWriter ^ readerWriter       = ref new ReaderWriter();
   Platform::Array<byte> ^ colorData = readerWriter->ReadData(L"assets/character_colors.txt");
   char         buf[50], ch, *pch;
   unsigned int r, g, b;
   int          p, q;
   memset(buf, 0, 50);
   for (i = j = 0; i < colorData->Length; i++)
   {
      ch = colorData->get(i);
      switch (ch)
      {
      case '\n':
         p   = 0;
         pch = strtok(buf, " ");
         while (pch != NULL)
         {
            switch (p)
            {
            case 0:
               q = pch[0];
               break;

            case 1:
               r = atoi(pch);
               break;

            case 2:
               g = atoi(pch);
               break;

            case 3:
               b = atoi(pch);
               break;
            }
            p++;
            pch = strtok(NULL, " ");
         }
         q -= CHAR_OFFSET;
         if ((q >= 0) && (q < NUM_CHARS))
         {
            charColors[q] = (r << 16) | (g << 8) | b;
         }
         memset(buf, 0, 50);
         j = 0;
         break;

      case '\r':
         break;

      default:
         if (j < 49)
         {
            buf[j] = ch;
            j++;
         }
         break;
      }
   }

   // Create text character textures.
   CD3D11_TEXTURE2D_DESC textTextureDesc(
      DXGI_FORMAT_B8G8R8A8_UNORM,
      textureSize, // Width
      textureSize, // Height
      1,           // MipLevels
      1,           // ArraySize
      D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET
      );

   const float dxgiDpi = 256.0f;
   m_d2dContext->SetDpi(dxgiDpi, dxgiDpi);
   D2D1_BITMAP_PROPERTIES1 bitmapProperties =
      D2D1::BitmapProperties1(
         D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
         D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
         dxgiDpi,
         dxgiDpi
         );

   ComPtr<IDWriteTextFormat> textFormat;
   DX::ThrowIfFailed(
      m_dwriteFactory->CreateTextFormat(
         L"Verdana",
         nullptr,
         DWRITE_FONT_WEIGHT_LIGHT,
         DWRITE_FONT_STYLE_NORMAL,
         DWRITE_FONT_STRETCH_NORMAL,
         128,
         L"en-US",    // locale
         &textFormat
         )
      );

   // Center the text horizontally.
   DX::ThrowIfFailed(
      textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER)
      );

   // Center the text vertically.
   DX::ThrowIfFailed(
      textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
      );

   // Create space through tilde characters.
   wchar_t c[2];
   c[1] = 0;
   for (int i = 32; i <= 126; i++)
   {
      c[0] = (char)i;

      DX::ThrowIfFailed(
         m_d3dDevice->CreateTexture2D(
            &textTextureDesc,
            nullptr,
            &textTexture
            )
         );

      CD3D11_SHADER_RESOURCE_VIEW_DESC textResourceViewDesc(
         textTexture.Get(),
         D3D11_SRV_DIMENSION_TEXTURE2D
         );

      DX::ThrowIfFailed(
         m_d3dDevice->CreateShaderResourceView(
            textTexture.Get(),
            &textResourceViewDesc,
            &textTextureSRV
            )
         );

      ComPtr<ID2D1Bitmap1> textureTarget;
      ComPtr<IDXGISurface> textureSurface;
      DX::ThrowIfFailed(
         textTexture.As(&textureSurface)
         );

      DX::ThrowIfFailed(
         m_d2dContext->CreateBitmapFromDxgiSurface(
            textureSurface.Get(),
            &bitmapProperties,
            &textureTarget
            )
         );

      m_d2dContext->SetTarget(textureTarget.Get());

      D2D1_SIZE_F renderTargetSize = m_d2dContext->GetSize();

      m_d2dContext->BeginDraw();

      m_d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());

      m_d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

      ComPtr<ID2D1SolidColorBrush> colorBrush;
      DX::ThrowIfFailed(
         m_d2dContext->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF(charColors[(int)c[0] - CHAR_OFFSET], 1.0f)),
            &colorBrush
            )
         );

      m_d2dContext->DrawText(
         c,
         1,
         textFormat.Get(),
         D2D1::RectF(0.0f, 0.0f, renderTargetSize.width, renderTargetSize.height),
         colorBrush.Get()
         );

      HRESULT hr = m_d2dContext->EndDraw();
      if (hr != D2DERR_RECREATE_TARGET)
      {
         DX::ThrowIfFailed(hr);
      }

      // Set transparency and picking pixel.
      m_d3dContext->CopyResource(
         stagingTextTexture.Get(),
         textTexture.Get()
         );

      D3D11_MAPPED_SUBRESOURCE mappedTex;
      DX::ThrowIfFailed(
         m_d3dContext->Map(stagingTextTexture.Get(),
                           D3D11CalcSubresource(0, 0, 1),
                           D3D11_MAP_WRITE, 0,
                           &mappedTex
                           )
         );

      UCHAR *pTexels = (UCHAR *)mappedTex.pData;
      for (UINT row = 0; row < stagingTextTextureDesc.Height; row++)
      {
         UINT rowStart = row * mappedTex.RowPitch;
         for (UINT col = 0; col < stagingTextTextureDesc.Width; col++)
         {
            UINT colStart = col * 4;

            // Black?
            if ((pTexels[rowStart + colStart + 0] == 0) &&           // Red
                (pTexels[rowStart + colStart + 1] == 0) &&           // Green
                (pTexels[rowStart + colStart + 2] == 0))             // Blue
            {
               pTexels[rowStart + colStart + 3] = 0;                 // Alpha=transparent
            }
         }
      }
      m_d3dContext->Unmap(stagingTextTexture.Get(), D3D11CalcSubresource(0, 0, 1));

      m_d3dContext->CopyResource(
         textTexture.Get(),
         stagingTextTexture.Get()
         );

      // Save texture.
      m_textures[c[0]]    = textTexture;
      m_textureSRVs[c[0]] = textTextureSRV;
   }
}


void Scene::UpdateForWindowSizeChange(Windows::Foundation::Rect windowBounds,
                                      DisplayOrientations orientation, XMFLOAT4X4& orientationTransform3D)
{
   m_windowBounds = windowBounds;
   m_orientation  = orientation;
   ConfigureCamera(orientationTransform3D);
}


// Render scene.
void Scene::Render()
{
   int   i, j, k;
   char  actual, visible;
   float x, z;

   Vector3<float> object;
   Drawable       drawable;

   // Window unavailable?
   if (!currentWindow())
   {
      return;
   }

   // Load window content into scene.
   for (i = 1, k = TEXT_HEIGHT - 2; i < k; i++)
   {
      for (j = 0; j < TEXT_WIDTH; j++)
      {
         actual = getScreenChar(i, j);
         if ((actual < 32) || (actual > 126))
         {
            actual = ' ';
         }
         visible = getWindowChar(i, j);
         if ((visible < 32) || (visible > 126))
         {
            visible = ' ';
         }
         PlaceObject(j, i, actual, visible);
      }
   }

   // Place player.
   PlacePlayer();

   // Inside of wall?
   if (m_locale == IN_WALL)
   {
      return;
   }

   // Place camera at player view point.
   PlaceCamera();

   // Disable transparency.
   m_d3dContext->OMSetBlendState(m_opaqueState, NULL, 0xffffffff);

   // Draw topology.
   for (i = m_xmin; i <= m_xmax; i++)
   {
      for (j = m_zmin; j <= m_zmax; j++)
      {
         x = (float)i;
         z = (float)j;
         if (m_wall[i][j])
         {
            if ((m_locale == IN_ROOM) && (m_visible[i][j] != ' '))
            {
               // Visible room wall.
               drawable.type         = Block;
               drawable.translation  = translation(x, 0.0f, z);
               drawable.rotation     = ident();
               drawable.textureIndex = Wall;
               RenderShape(drawable);
            }
            else if ((m_locale == IN_PASSAGE) || (m_visible[i][j] == ' '))
            {
               // Dark wall.
               drawable.type         = Block;
               drawable.translation  = translation(x, 0.0f, z);
               drawable.rotation     = ident();
               drawable.textureIndex = Black;
               RenderShape(drawable);
            }
         }
         else if (m_actual[i][j] == '+')
         {
            if (m_visible[i][j] != ' ')
            {
               // Visible door.
               drawable.type         = Block;
               drawable.translation  = translation(x, 0.0f, z);
               drawable.rotation     = ident();
               drawable.textureIndex = Door;
               RenderShape(drawable);
            }
         }
         else if (m_actual[i][j] == '&')
         {
            if (m_visible[i][j] != ' ')
            {
               // Secret door looks like wall.
               drawable.type         = Block;
               drawable.translation  = translation(x, 0.0f, z);
               drawable.rotation     = ident();
               drawable.textureIndex = Wall;
               RenderShape(drawable);
            }
         }
         else if (m_visible[i][j] != ' ')
         {
            // Floor.
            drawable.type         = Tile;
            drawable.translation  = translation(x, 0.0f, z);
            drawable.rotation     = ident();
            drawable.textureIndex = Floor;
            RenderShape(drawable);
         }
      }
   }

   // Enable transparency.
   m_d3dContext->OMSetBlendState(m_transparentState, NULL, 0xffffffff);

   // Schedule animations.
   Animator a;
   bool     dispose = getDisposeAnimations();
   int      na[4];
   bool     killed;
   while (getNextAnimator(na))
   {
      if (na[3] == 1)
      {
         killed = true;
      }
      else
      {
         killed = false;
      }
      a = Animator((char)na[0], na[1], na[2], killed);
      m_animators.push_back(a);
   }
   vector<Animator> h;
   int64            tm = GetTickCount64();
   float            rot;
   while (m_animators.size() > 0)
   {
      a = m_animators.front();
      m_animators.erase(m_animators.begin());
      if (dispose)
      {
         a.dispose = dispose;
      }
      if (a.killed)
      {
         if (a.prog1 >= 1.0f)
         {
            if (a.dispose)
            {
               continue;
            }
         }
         else
         {
            a.prog1 = (float)(tm - a.startTime) / (float)Animator::DEATH_ANIMATION_DURATION;
            if (a.prog1 > 1.0f)
            {
               a.prog1 = 1.0f;
            }
         }
         rot = -(a.prog1 * 90.0f);
      }
      else
      {
         if (a.prog1 < 1.0f)
         {
            a.prog1 = (float)(tm - a.startTime) / (float)Animator::HIT_ANIMATION_DURATION;
            if (a.prog1 > 1.0f)
            {
               a.prog1 = 1.0f;
            }
            rot = -(a.prog1 * Animator::HIT_RECOIL_ANGLE);
         }
         else if (a.prog2 < 1.0f)
         {
            a.prog2 = (float)(tm - a.startTime) / (float)Animator::HIT_ANIMATION_DURATION;
            if (a.prog2 > 1.0f)
            {
               a.prog2 = 1.0f;
            }
            rot = -(Animator::HIT_RECOIL_ANGLE - (a.prog2 * Animator::HIT_RECOIL_ANGLE));
         }
         else if (a.dispose)
         {
            continue;
         }
         else
         {
            rot = 0.0f;
         }
      }
      a.rotation = -rot;
      h.push_back(a);
      m_visible[a.x][a.z] = ' ';
   }
   m_animators = h;

   // Draw objects from furthest to nearest.
   vector<pair<int, int> > objectList;
   for (i = m_xmin; i <= m_xmax; i++)
   {
      for (j = m_zmin; j <= m_zmax; j++)
      {
         if (!m_wall[i][j] && (m_visible[i][j] != ' '))
         {
            if ((m_visible[i][j] != '|') && (m_visible[i][j] != '-') &&
                (m_visible[i][j] != '+') && (m_visible[i][j] != '&') &&
                (m_visible[i][j] != '.') && (m_visible[i][j] != '#') &&
                (m_visible[i][j] != '@'))
            {
               objectList.push_back(pair<int, int>(i, j));
            }
         }
      }
   }
   float d1, d2;
   vector<pair<int, int> >::iterator itr1, itr2;
   while (!objectList.empty())
   {
      d1 = -1.0f;
      for (itr1 = objectList.begin(); itr1 != objectList.end(); itr1++)
      {
         d2 = fabs(m_position.x - (float)itr1->first) + fabs(m_position.z - (float)itr1->second);
         if ((d1 < 0.0f) || (d2 > d1))
         {
            itr2 = itr1;
            d1   = d2;
         }
      }
      i = itr2->first;
      j = itr2->second;
      objectList.erase(itr2);
      x        = (float)i;
      z        = (float)j;
      object.x = x;
      object.y = 0.0f;
      object.z = z;
      BillboardRotation brot(m_position, object);
      drawable.type         = Billboard;
      drawable.translation  = translation(x, -0.5f, z);
      drawable.rotation     = rotationArbitrary(brot.axis, brot.angle);
      drawable.textureIndex = m_visible[i][j];
      RenderShape(drawable);
   }

   // Draw animations.
   for (i = 0; i < (int)m_animators.size(); i++)
   {
      a        = m_animators[i];
      x        = (float)(a.x);
      z        = (float)(a.z);
      object.x = x;
      object.y = 0.0f;
      object.z = z;
      BillboardRotation brot(m_position, object);
      drawable.type        = Billboard;
      drawable.translation = translation(x, -0.5f, z);
      drawable.rotation    =
         mul(rotationArbitrary(brot.axis, brot.angle),
             rotationArbitrary(Vector3<float>(1.0f, 0.0f, 0.0f), a.rotation));
      drawable.textureIndex = a.show;
      RenderShape(drawable);
   }
}


// Render scene for picking.
void Scene::RenderForPicking()
{
   int   i, j, k;
   char  actual, visible;
   float x, z;

   Vector3<float> object;
   Drawable       drawable;

   // Window unavailable?
   if (!currentWindow())
   {
      return;
   }

   // Load window content into scene.
   for (i = 1, k = TEXT_HEIGHT - 2; i < k; i++)
   {
      for (j = 0; j < TEXT_WIDTH; j++)
      {
         actual  = getScreenChar(i, j);
         visible = getWindowChar(i, j);
         PlaceObject(j, i, actual, visible);
      }
   }

   // Place player.
   PlacePlayer();

   // Inside of wall?
   if (m_locale == IN_WALL)
   {
      return;
   }

   // Place camera at player view point.
   PlaceCamera();

   // Disable transparency.
   m_d3dContext->OMSetBlendState(m_opaqueState, NULL, 0xffffffff);

   // Draw objects from furthest to nearest.
   vector<pair<int, int> > objectList;
   for (i = m_xmin; i <= m_xmax; i++)
   {
      for (j = m_zmin; j <= m_zmax; j++)
      {
         if (!m_wall[i][j] && (m_visible[i][j] != ' '))
         {
            if ((m_visible[i][j] != '|') && (m_visible[i][j] != '-') &&
                (m_visible[i][j] != '+') && (m_visible[i][j] != '&') &&
                (m_visible[i][j] != '.') && (m_visible[i][j] != '#') &&
                (m_visible[i][j] != '@'))
            {
               objectList.push_back(pair<int, int>(i, j));
            }
         }
      }
   }
   float d1, d2;
   vector<pair<int, int> >::iterator itr1, itr2;
   while (!objectList.empty())
   {
      d1 = -1.0f;
      for (itr1 = objectList.begin(); itr1 != objectList.end(); itr1++)
      {
         d2 = fabs(m_position.x - (float)itr1->first) + fabs(m_position.z - (float)itr1->second);
         if ((d1 < 0.0f) || (d2 > d1))
         {
            itr2 = itr1;
            d1   = d2;
         }
      }
      i = itr2->first;
      j = itr2->second;
      objectList.erase(itr2);
      x        = (float)i;
      z        = (float)j;
      object.x = x;
      object.y = 0.0f;
      object.z = z;
      BillboardRotation brot(m_position, object);
      drawable.type         = ColoredQuad;
      drawable.translation  = translation(x, -0.5f, z);
      drawable.rotation     = rotationArbitrary(brot.axis, brot.angle);
      drawable.textureIndex = -1;
      drawable.color.x      = (float)BLUE_PICK / 255.0f;
      drawable.color.y      = (float)GREEN_PICK / 255.0f;
      drawable.color.z      = (float)m_visible[i][j] / 255.0f;
      RenderShape(drawable);
   }
}


// Render shape.
void Scene::RenderShape(Drawable drawable)
{
   // set the vertex and index buffers
   UINT stride = sizeof(BasicVertex);
   UINT offset = 0;

   switch (drawable.type)
   {
   case Block:
      m_d3dContext->IASetVertexBuffers(
         0,                                         // starting at the first vertex buffer slot
         1,                                         // set one vertex buffer binding
         m_blockVertexBuffer.GetAddressOf(),
         &stride,                                   // specify the size in bytes of a single vertex
         &offset                                    // specify the base vertex in the buffer
         );

      // set the index buffer
      m_d3dContext->IASetIndexBuffer(
         m_blockIndexBuffer.Get(),
         DXGI_FORMAT_R16_UINT,              // unsigned short index format
         0                                  // specify the base index in the buffer
         );

      // Set the model transform.
      m_constantBufferData.model = mul(
         drawable.translation, drawable.rotation
         );

      m_d3dContext->UpdateSubresource(
         m_constantBuffer.Get(),
         0,
         nullptr,
         &m_constantBufferData,
         0,
         0
         );

      m_d3dContext->VSSetConstantBuffers(
         0,                                                                 // starting at the first constant buffer slot
         1,                                                                 // set one constant buffer binding
         m_constantBuffer.GetAddressOf()
         );

      // Texture?
      if (drawable.textureIndex != -1)
      {
         m_d3dContext->PSSetShaderResources(
            0,                                                                               // starting at the first shader resource slot
            1,                                                                               // set one shader resource binding
            m_textureSRVs[drawable.textureIndex].GetAddressOf()
            );
      }

      // Draw
      m_d3dContext->DrawIndexed(
         m_blockIndexCount,                                 // draw all created vertices
         0,                                                 // starting with the first vertex
         0                                                  // and the first index
         );
      break;

   case Tile:
      m_d3dContext->IASetVertexBuffers(
         0,                             // starting at the first vertex buffer slot
         1,                             // set one vertex buffer binding
         m_tileVertexBuffer.GetAddressOf(),
         &stride,                       // specify the size in bytes of a single vertex
         &offset                        // specify the base vertex in the buffer
         );

      // set the index buffer
      m_d3dContext->IASetIndexBuffer(
         m_tileIndexBuffer.Get(),
         DXGI_FORMAT_R16_UINT,  // unsigned short index format
         0                      // specify the base index in the buffer
         );

      // Set the model transform.
      m_constantBufferData.model = mul(
         drawable.translation, drawable.rotation
         );

      m_d3dContext->UpdateSubresource(
         m_constantBuffer.Get(),
         0,
         nullptr,
         &m_constantBufferData,
         0,
         0
         );

      m_d3dContext->VSSetConstantBuffers(
         0,                                                     // starting at the first constant buffer slot
         1,                                                     // set one constant buffer binding
         m_constantBuffer.GetAddressOf()
         );

      // Texture?
      if (drawable.textureIndex != -1)
      {
         m_d3dContext->PSSetShaderResources(
            0,                                                                          // starting at the first shader resource slot
            1,                                                                          // set one shader resource binding
            m_textureSRVs[drawable.textureIndex].GetAddressOf()
            );
      }

      m_d3dContext->DrawIndexed(
         m_tileIndexCount,                    // draw all created vertices
         0,                                   // starting with the first vertex
         0                                    // and the first index
         );
      break;

   case Billboard:
      m_d3dContext->IASetVertexBuffers(
         0,                             // starting at the first vertex buffer slot
         1,                             // set one vertex buffer binding
         m_billboardVertexBuffer.GetAddressOf(),
         &stride,                       // specify the size in bytes of a single vertex
         &offset                        // specify the base vertex in the buffer
         );

      // set the index buffer
      m_d3dContext->IASetIndexBuffer(
         m_billboardIndexBuffer.Get(),
         DXGI_FORMAT_R16_UINT,  // unsigned short index format
         0                      // specify the base index in the buffer
         );

      // Set the model transform.
      m_constantBufferData.model = mul(
         drawable.translation, drawable.rotation
         );

      m_d3dContext->UpdateSubresource(
         m_constantBuffer.Get(),
         0,
         nullptr,
         &m_constantBufferData,
         0,
         0
         );

      m_d3dContext->VSSetConstantBuffers(
         0,                                                     // starting at the first constant buffer slot
         1,                                                     // set one constant buffer binding
         m_constantBuffer.GetAddressOf()
         );

      // Texture?
      if (drawable.textureIndex != -1)
      {
         m_d3dContext->PSSetShaderResources(
            0,                                                                          // starting at the first shader resource slot
            1,                                                                          // set one shader resource binding
            m_textureSRVs[drawable.textureIndex].GetAddressOf()
            );
      }

      m_d3dContext->DrawIndexed(
         m_billboardIndexCount,                 // draw all created vertices
         0,                                     // starting with the first vertex
         0                                      // and the first index
         );
      break;

   case ColoredQuad:

      ComPtr<ID3D11Buffer> coloredQuadVertexBuffer;
      ComPtr<ID3D11Buffer> coloredQuadIndexBuffer;
      unsigned int         coloredQuadIndexCount;
      UINT                 coloredStride = sizeof(ColoredVertex);

      m_shapes->CreateColoredQuad(
         drawable.color,
         &coloredQuadVertexBuffer,
         &coloredQuadIndexBuffer,
         nullptr,
         &coloredQuadIndexCount
         );

      m_d3dContext->IASetVertexBuffers(
         0,                             // starting at the first vertex buffer slot
         1,                             // set one vertex buffer binding
         coloredQuadVertexBuffer.GetAddressOf(),
         &coloredStride,                // specify the size in bytes of a single vertex
         &offset                        // specify the base vertex in the buffer
         );

      // set the index buffer
      m_d3dContext->IASetIndexBuffer(
         coloredQuadIndexBuffer.Get(),
         DXGI_FORMAT_R16_UINT,  // unsigned short index format
         0                      // specify the base index in the buffer
         );

      // Set the model transform.
      m_constantBufferData.model = mul(
         drawable.translation, drawable.rotation
         );

      m_d3dContext->UpdateSubresource(
         m_constantBuffer.Get(),
         0,
         nullptr,
         &m_constantBufferData,
         0,
         0
         );

      m_d3dContext->VSSetConstantBuffers(
         0,                                                     // starting at the first constant buffer slot
         1,                                                     // set one constant buffer binding
         m_constantBuffer.GetAddressOf()
         );

      if (drawable.textureIndex != -1)
      {
         m_d3dContext->PSSetShaderResources(
            0,                                                                          // starting at the first shader resource slot
            1,                                                                          // set one shader resource binding
            m_textureSRVs[drawable.textureIndex].GetAddressOf()
            );
      }

      m_d3dContext->DrawIndexed(
         coloredQuadIndexCount,                 // draw all created vertices
         0,                                     // starting with the first vertex
         0                                      // and the first index
         );
      break;
   }
}


// Place object in scene.
void Scene::PlaceObject(int x, int z, char actual, char visible)
{
   m_actual[x][z]  = actual;
   m_visible[x][z] = visible;
   if ((actual == ' ') || (actual == '-') || (actual == '|'))
   {
      m_wall[x][z] = true;
   }
   else
   {
      m_wall[x][z] = false;
   }
   if (visible == '@')
   {
      m_playerX   = x;
      m_playerZ   = z;
      m_playerDir = (DIRECTION)getDir();
   }
}


// Place player in scene.
void Scene::PlacePlayer()
{
   m_position.x = (float)m_playerX;
   m_position.y = 0.0f;
   m_position.z = (float)m_playerZ;

   switch (m_playerDir)
   {
   case SOUTH:
      m_rotation  = 0.0f;
      m_forward.x = 0.0f;
      m_forward.z = 1.0f;
      break;

   case SOUTHEAST:
      m_rotation  = 45.0f;
      m_forward.x = -1.0f;
      m_forward.z = 1.0f;
      break;

   case EAST:
      m_rotation  = 90.0f;
      m_forward.x = -1.0f;
      m_forward.z = 0.0f;
      break;

   case NORTHEAST:
      m_rotation  = 135.0f;
      m_forward.x = -1.0f;
      m_forward.z = -1.0f;
      break;

   case NORTH:
      m_rotation  = 180.0f;
      m_forward.x = 0.0f;
      m_forward.z = -1.0f;
      break;

   case NORTHWEST:
      m_rotation  = 225.0f;
      m_forward.x = 1.0f;
      m_forward.z = -1.0f;
      break;

   case WEST:
      m_rotation  = 270.0f;
      m_forward.x = 1.0f;
      m_forward.z = 0.0f;
      break;

   case SOUTHWEST:
      m_rotation  = 315.0f;
      m_forward.x = 1.0f;
      m_forward.z = 1.0f;
      break;
   }

   // Set player locale.
   SetPlayerLocale();
}


// Set player locale and visible bounds.
void Scene::SetPlayerLocale()
{
   int i, j, k, w, h, x2, z2;

   int       x   = m_playerX;
   int       z   = m_playerZ;
   DIRECTION dir = m_playerDir;

   m_xmin = x - 1;
   m_xmax = x + 1;
   m_zmin = z - 1;
   m_zmax = z + 1;
   if (m_xmin < 0)
   {
      m_xmin = 0;
   }
   if (m_xmax >= TEXT_WIDTH)
   {
      m_xmax = TEXT_WIDTH - 1;
   }
   if (m_zmin < 0)
   {
      m_zmin = 0;
   }
   if (m_zmax >= TEXT_HEIGHT)
   {
      m_zmax = TEXT_HEIGHT - 1;
   }

   if (m_wall[x][z])
   {
      m_locale = IN_WALL;
      return;
   }
   m_locale = IN_PASSAGE;
   for (i = 0; i < TEXT_WIDTH; i++)
   {
      for (j = 0; j < TEXT_HEIGHT; j++)
      {
         if ((m_actual[i][j] == '-') && ((m_actual[i][j + 1] == '|') ||
                                         (m_actual[i][j + 1] == '+') || (m_actual[i][j + 1] == '&')))
         {
            for (k = i, w = 0; m_actual[k][j] == '-' ||
                 m_actual[k][j] == '+' || m_actual[k][j] == '&'; k++, w++)
            {
            }
            for (k = j + 1, h = 2; m_actual[i][k] == '|' ||
                 m_actual[i][k] == '+' || m_actual[i][k] == '&'; k++, h++)
            {
            }
            if ((x >= i) && (x < (i + w)) &&
                (z >= j) && (z < (j + h)))
            {
               m_xmin = i;
               m_xmax = i + w - 1;
               m_zmin = j;
               m_zmax = j + h - 1;

               if ((m_actual[x][z] != '+') && (m_actual[x][z] != '&'))
               {
                  m_locale = IN_ROOM;
               }
               else
               {
                  // In doorway: check if facing room or passage.
                  x2 = x;
                  z2 = z;
                  switch (dir)
                  {
                  case NORTH:
                     x2 = x;
                     z2 = z - 1;
                     break;

                  case NORTHEAST:
                     x2 = x - 1;
                     z2 = z - 1;
                     break;

                  case EAST:
                     x2 = x - 1;
                     z2 = z;
                     break;

                  case SOUTHEAST:
                     x2 = x - 1;
                     z2 = z + 1;
                     break;

                  case SOUTH:
                     x2 = x;
                     z2 = z + 1;
                     break;

                  case SOUTHWEST:
                     x2 = x + 1;
                     z2 = z + 1;
                     break;

                  case WEST:
                     x2 = x + 1;
                     z2 = z;
                     break;

                  case NORTHWEST:
                     x2 = x + 1;
                     z2 = z - 1;
                     break;
                  }

                  if ((x2 >= i) && (x2 < (i + w)) &&
                      (z2 >= j) && (z2 < (j + h)))
                  {
                     m_locale = IN_ROOM;
                  }
                  else
                  {
                     m_locale = IN_PASSAGE;
                     SetPassageVisibility(x, z, dir);
                  }
               }
               return;
            }
         }
      }
   }
   SetPassageVisibility(x, z, dir);
}


// Set passage visibility bounds.
void Scene::SetPassageVisibility(int x, int z, DIRECTION dir)
{
   int       xmin2, xmax2, zmin2, zmax2;
   DIRECTION dir2, dir3;

   dir2 = dir3 = NORTH;
   switch (dir)
   {
   case NORTH:
      dir2 = dir3 = NORTH;
      break;

   case NORTHEAST:
      dir2 = NORTH;
      dir3 = EAST;
      break;

   case EAST:
      dir2 = dir3 = EAST;
      break;

   case SOUTHEAST:
      dir2 = SOUTH;
      dir3 = EAST;
      break;

   case SOUTH:
      dir2 = dir3 = SOUTH;
      break;

   case SOUTHWEST:
      dir2 = SOUTH;
      dir3 = WEST;
      break;

   case WEST:
      dir2 = dir3 = WEST;
      break;

   case NORTHWEST:
      dir2 = NORTH;
      dir3 = WEST;
      break;
   }

   xmin2 = xmax2 = zmin2 = zmax2 = 0;
   switch (dir2)
   {
   case NORTH:
      for (m_zmin = z; !m_wall[x][m_zmin] && m_zmin > 0; m_zmin--)
      {
      }
      xmin2 = x - 3;
      xmax2 = x + 3;
      break;

   case EAST:
      for (m_xmin = x; !m_wall[m_xmin][z] && m_xmin > 0; m_xmin--)
      {
      }
      zmin2 = z - 3;
      zmax2 = z + 3;
      break;

   case SOUTH:
      for (m_zmax = z; !m_wall[x][m_zmax] && m_zmax < TEXT_HEIGHT - 1; m_zmax++)
      {
      }
      xmin2 = x - 3;
      xmax2 = x + 3;
      break;

   case WEST:
      for (m_xmax = x; !m_wall[m_xmax][z] && m_xmax < TEXT_WIDTH - 1; m_xmax++)
      {
      }
      zmin2 = z - 3;
      zmax2 = z + 3;
      break;
   }

   if (dir3 != dir2)
   {
      switch (dir3)
      {
      case NORTH:
         for (m_zmin = z; !m_wall[x][m_zmin] && m_zmin > 0; m_zmin--)
         {
         }
         xmin2 = x - 3;
         xmax2 = x + 3;
         break;

      case EAST:
         for (m_xmin = x; !m_wall[m_xmin][z] && m_xmin > 0; m_xmin--)
         {
         }
         zmin2 = z - 3;
         zmax2 = z + 3;
         break;

      case SOUTH:
         for (m_zmax = z; !m_wall[x][m_zmax] && m_zmax < TEXT_HEIGHT - 1; m_zmax++)
         {
         }
         xmin2 = x - 3;
         xmax2 = x + 3;
         break;

      case WEST:
         for (m_xmax = x; !m_wall[m_xmax][z] && m_xmax < TEXT_WIDTH - 1; m_xmax++)
         {
         }
         zmin2 = z - 3;
         zmax2 = z + 3;
         break;
      }
   }
   if (xmin2 < m_xmin)
   {
      m_xmin = xmin2;
   }
   if (xmax2 > m_xmax)
   {
      m_xmax = xmax2;
   }
   if (zmin2 < m_zmin)
   {
      m_zmin = zmin2;
   }
   if (zmax2 > m_zmax)
   {
      m_zmax = zmax2;
   }

   if (m_xmin < 0)
   {
      m_xmin = 0;
   }
   if (m_xmax >= TEXT_WIDTH)
   {
      m_xmax = TEXT_WIDTH - 1;
   }
   if (m_zmin < 0)
   {
      m_zmin = 0;
   }
   if (m_zmax >= TEXT_HEIGHT)
   {
      m_zmax = TEXT_HEIGHT - 1;
   }
}


// Camera parameters.
const float Scene::FIELD_OF_VIEW            = 70.0f;
const float Scene::FRUSTUM_NEAR             = 0.01f;
const float Scene::FRUSTUM_FAR              = 50.0f;
const long Scene:: VIEW_TRANSITION_DURATION = 200;

// Create camera.
void Scene::CreateCamera()
{
   m_camera              = ref new Camera();
   m_currentPosition.y   = m_lastPosition.y = 0.0f;
   m_currentForward.z    = m_lastForward.z = -1.0f;
   m_viewTransitionState = 0.0f;
   m_viewTransitionTime  = GetTickCount64();
}


// Configure camera.
void Scene::ConfigureCamera(XMFLOAT4X4& orientationIn)
{
   m_camera->SetProjectionParameters(
      FIELD_OF_VIEW,
      m_windowBounds.Width / m_windowBounds.Height,
      FRUSTUM_NEAR,
      FRUSTUM_FAR
      );
   m_camera->GetProjectionMatrix(&m_constantBufferData.projection);

   // Rotate for device orientation.
   float4x4 orientation;
   orientation._11 = orientationIn._11;
   orientation._12 = orientationIn._12;
   orientation._13 = orientationIn._13;
   orientation._14 = orientationIn._14;
   orientation._21 = orientationIn._21;
   orientation._22 = orientationIn._22;
   orientation._23 = orientationIn._23;
   orientation._24 = orientationIn._24;
   orientation._31 = orientationIn._31;
   orientation._32 = orientationIn._32;
   orientation._33 = orientationIn._33;
   orientation._34 = orientationIn._34;
   orientation._41 = orientationIn._41;
   orientation._42 = orientationIn._42;
   orientation._43 = orientationIn._43;
   orientation._44 = orientationIn._44;
   m_constantBufferData.projection = mul(m_constantBufferData.projection, orientation);
}


// Smoothly transition camera to player viewpoint.
void Scene::PlaceCamera()
{
   // View is changing?
   if ((m_position.x != m_currentPosition.x) ||
       (m_position.y != m_currentPosition.y) ||
       (m_position.z != m_currentPosition.z) ||
       (m_forward.x != m_currentForward.x) ||
       (m_forward.y != m_currentForward.y) ||
       (m_forward.z != m_currentForward.z))
   {
      m_lastPosition        = m_currentPosition;
      m_currentPosition     = m_position;
      m_lastForward         = m_currentForward;
      m_currentForward      = m_forward;
      m_viewTransitionState = 1.0f;
      m_viewTransitionTime  = GetTickCount64();
      if (fabs(m_currentPosition.x - m_lastPosition.x) > 1.5f)
      {
         if (m_currentPosition.x < m_lastPosition.x)
         {
            m_lastPosition.x = m_currentPosition.x + 1.0f;
         }
         else
         {
            m_lastPosition.x = m_currentPosition.x - 1.0f;
         }
      }
      if (fabs(m_currentPosition.z - m_lastPosition.z) > 1.5f)
      {
         if (m_currentPosition.z < m_lastPosition.z)
         {
            m_lastPosition.z = m_currentPosition.z + 1.0f;
         }
         else
         {
            m_lastPosition.z = m_currentPosition.z - 1.0f;
         }
      }
   }

   // Set camera to intermediate viewpoint.
   m_viewTransitionState =
      (float)(GetTickCount64() - m_viewTransitionTime) / (float)VIEW_TRANSITION_DURATION;
   if (m_viewTransitionState > 1.0f)
   {
      m_viewTransitionState = 1.0f;
   }
   m_viewTransitionState = 1.0f - m_viewTransitionState;
   Vector3<float> position;
   Vector3<float> forward;
   Vector3<float> up(0.0f, 1.0f, 0.0f);
   position.x = (m_currentPosition.x * (1.0f - m_viewTransitionState)) +
                (m_lastPosition.x * m_viewTransitionState);
   forward.x = (m_currentForward.x * (1.0f - m_viewTransitionState)) +
               (m_lastForward.x * m_viewTransitionState);
   position.y = 0.0f;
   forward.y  = 0.0f;
   position.z = (m_currentPosition.z * (1.0f - m_viewTransitionState)) +
                (m_lastPosition.z * m_viewTransitionState);
   forward.z = (m_currentForward.z * (1.0f - m_viewTransitionState)) +
               (m_lastForward.z * m_viewTransitionState);
   m_camera->SetViewParameters(position, position + forward, up);
   m_camera->GetViewMatrix(&m_constantBufferData.view);
   m_d3dContext->UpdateSubresource(
      m_constantBuffer.Get(),
      0,
      nullptr,
      &m_constantBufferData,
      0,
      0
      );
}


// Camera has pending updates?
bool Scene::CameraPending()
{
   if (m_viewTransitionState > 0.0f)
   {
      return(true);
   }
   else
   {
      return(false);
   }
}


// Create rotation for object to face player.
Scene::BillboardRotation::BillboardRotation(Vector3<float>& player, Vector3<float>& object)
{
   Vector3<float> target, source;

   this->player = player;
   this->object = object;

   axis.x = 0.0f;
   axis.y = 1.0f;
   axis.z = 0.0f;
   angle  = 0.0f;

   if (fabs(player.x - object.x) < 1.0f)
   {
      if (player.z < object.z)
      {
         angle = 180.0f;
      }
      else
      {
         angle = 0.0f;
      }
      return;
   }
   else if (fabs(player.z - object.z) < 1.0f)
   {
      if (player.x < object.x)
      {
         angle = 90.0f;
      }
      else
      {
         angle = 270.0f;
      }
      return;
   }
   target.x = player.x - object.x;
   target.y = 0.0f;
   target.z = player.z - object.z;
   target   = normalize(target);
   source.x = source.y = 0.0f;
   source.z = -1.0f;
   axis     = cross(target, source);
   angle    = acos(dot(target, source));
   angle    = angle * 180.0f / PI_F;
   if (axis.y > 0.0f)
   {
      angle += 180.0f;
   }
   else
   {
      angle  = 180.0f - angle;
      axis.y = -axis.y;
   }
}


const long Scene::Animator:: DEATH_ANIMATION_DURATION = 500;
const long Scene::Animator:: HIT_ANIMATION_DURATION   = 200;
const float Scene::Animator::HIT_RECOIL_ANGLE         = 45.0f;
