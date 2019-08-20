#include "pch.h"
#include "Renderer.h"
#include "RogueInterface.hpp"
#include "Scene.h"

using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;

Renderer::Renderer()
{
   m_textureFileNames.push_back(L"Assets/textures/wall.dds");
   m_textureFileNames.push_back(L"Assets/textures/floor.dds");
   m_textureFileNames.push_back(L"Assets/textures/door.dds");
   m_timer  = ref new Timer();
   m_active = true;
}


void Renderer::CreateDeviceIndependentResources()
{
   DirectXBase::CreateDeviceIndependentResources();
}


void Renderer::CreateDeviceResources()
{
   DirectXBase::CreateDeviceResources();

   Loader ^ loader = ref new Loader(m_d3dDevice.Get());

   loader->LoadShader(
      L"SimpleVertexShader.cso",
      nullptr,
      0,
      &m_vertexShader,
      &m_inputLayout
      );

   loader->LoadShader(
      L"SimplePixelShader.cso",
      &m_pixelShader
      );

   ReaderWriter ^ reader = ref new ReaderWriter();
   auto vertexShaderBytecode = reader->ReadData("ColoredVertexShader.cso");
   DX::ThrowIfFailed(
      m_d3dDevice->CreateVertexShader(
         vertexShaderBytecode->Data,
         vertexShaderBytecode->Length,
         nullptr,
         &m_coloredVertexShader
         )
      );

   const D3D11_INPUT_ELEMENT_DESC coloredVertexLayoutDesc[] =
   {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
   };

   DX::ThrowIfFailed(
      m_d3dDevice->CreateInputLayout(
         coloredVertexLayoutDesc,
         ARRAYSIZE(coloredVertexLayoutDesc),
         vertexShaderBytecode->Data,
         vertexShaderBytecode->Length,
         &m_coloredInputLayout
         )
      );

   auto pixelShaderBytecode = reader->ReadData("ColoredPixelShader.cso");
   ComPtr<ID3D11PixelShader> pixelShader;
   DX::ThrowIfFailed(
      m_d3dDevice->CreatePixelShader(
         pixelShaderBytecode->Data,
         pixelShaderBytecode->Length,
         nullptr,
         &m_coloredPixelShader
         )
      );

   // create the sampler
   D3D11_SAMPLER_DESC samplerDesc;
   ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
   samplerDesc.Filter     = D3D11_FILTER_ANISOTROPIC;
   samplerDesc.AddressU   = D3D11_TEXTURE_ADDRESS_WRAP;
   samplerDesc.AddressV   = D3D11_TEXTURE_ADDRESS_WRAP;
   samplerDesc.AddressW   = D3D11_TEXTURE_ADDRESS_WRAP;
   samplerDesc.MipLODBias = 0.0f;
   // use 4x on feature level 9.2 and above, otherwise use only 2x
   samplerDesc.MaxAnisotropy  = m_d3dFeatureLevel > D3D_FEATURE_LEVEL_9_1 ? 4 : 2;
   samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
   samplerDesc.BorderColor[0] = 0.0f;
   samplerDesc.BorderColor[1] = 0.0f;
   samplerDesc.BorderColor[2] = 0.0f;
   samplerDesc.BorderColor[3] = 0.0f;
   // allow use of all mip levels
   samplerDesc.MinLOD = 0;
   samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

   DX::ThrowIfFailed(
      m_d3dDevice->CreateSamplerState(
         &samplerDesc,
         &m_sampler)
      );

   m_textOverlay = ref new TextOverlay();
   m_textOverlay->Initialize(
      m_d2dDevice.Get(),
      m_d2dContext.Get(),
      m_wicFactory.Get(),
      m_dwriteFactory.Get()
      );

   m_scene = ref new Scene(m_textureFileNames);
   m_scene->Initialize(
      m_d3dDevice.Get(),
      m_d3dContext.Get(),
      m_d2dDevice.Get(),
      m_d2dContext.Get(),
      m_dwriteFactory.Get()
      );
}


void Renderer::CreateWindowSizeDependentResources()
{
   DirectXBase::CreateWindowSizeDependentResources();
   m_textOverlay->UpdateForWindowSizeChange(m_windowBounds, m_orientation);
   m_scene->UpdateForWindowSizeChange(m_windowBounds,
                                      m_orientation, m_orientationTransform3D);
}


void Renderer::UpdateForWindowSizeChange()
{
   DirectXBase::UpdateForWindowSizeChange();
   m_textOverlay->UpdateForWindowSizeChange(m_windowBounds, m_orientation);
   m_scene->UpdateForWindowSizeChange(m_windowBounds,
                                      m_orientation, m_orientationTransform3D);
}


void Renderer::Render()
{
   // Lock display.
   lockDisplay();

   // clear both the render target and depth stencil to default values
   m_d3dContext->OMSetRenderTargets(
      1,
      m_d3dRenderTargetView.GetAddressOf(),
      m_d3dDepthStencilView.Get()
      );

   const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
   m_d3dContext->ClearRenderTargetView(
      m_d3dRenderTargetView.Get(),
      ClearColor
      );

   m_d3dContext->ClearDepthStencilView(
      m_d3dDepthStencilView.Get(),
      D3D11_CLEAR_DEPTH,
      1.0f,
      0
      );

   m_d3dContext->IASetInputLayout(m_inputLayout.Get());

   // specify the way the vertex and index buffers define geometry
   m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   // set the vertex shader stage state
   m_d3dContext->VSSetShader(
      m_vertexShader.Get(),
      nullptr,                  // don't use shader linkage
      0                         // don't use shader linkage
      );

   // set the pixel shader stage state
   m_d3dContext->PSSetShader(
      m_pixelShader.Get(),
      nullptr,                  // don't use shader linkage
      0                         // don't use shader linkage
      );

   m_d3dContext->PSSetSamplers(
      0,                            // starting at the first sampler slot
      1,                            // set one sampler binding
      m_sampler.GetAddressOf()
      );

   // Render text.
   if (!m_textOverlay->Render(m_orientationTransform2D))
   {
      if (!isBlind())
      {
         // Set up viewport for scene.
         float           width  = m_d3dRenderTargetSize.Width;
         float           height = m_d3dRenderTargetSize.Height;
         float           u      = height / (float)(LINES - 3);
         CD3D11_VIEWPORT viewport(
            0.0f,
            u,
            width,
            height - (3.0f * u)
            );
         switch (DisplayProperties::CurrentOrientation)
         {
         case DisplayOrientations::Landscape:
            break;

         case DisplayOrientations::Portrait:
            u = width / (float)(LINES - 4);
            viewport.TopLeftX = u;
            viewport.TopLeftY = 0.0f;
            viewport.Width    = width - (3.0f * u);
            viewport.Height   = height;
            break;

         case DisplayOrientations::LandscapeFlipped:
            viewport.TopLeftY = u * 2.0f;
            break;

         case DisplayOrientations::PortraitFlipped:
            u = width / (float)(LINES - 4);
            viewport.TopLeftX = u * 2.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width    = width - (3.0f * u);
            viewport.Height   = height;
            break;
         }
         m_d3dContext->RSSetViewports(1, &viewport);

         // Render scene.
         m_scene->Render();

         // Restore the viewport to the entire window.
         viewport.TopLeftX = viewport.TopLeftY = 0.0f;
         viewport.Width    = width;
         viewport.Height   = height;
         m_d3dContext->RSSetViewports(1, &viewport);
      }
   }

   unlockDisplay();

   // Deactivate rendering?
   m_timer->Update();
   if (m_timer->Total >= (float)ACTIVE_RENDER_SECONDS)
   {
      m_active = false;
   }
}


// Pick character.
int Renderer::PickCharacter(Windows::Foundation::Point pickPoint)
{
   // Lock display.
   lockDisplay();

   // Render picking scene.
   RenderForPicking();

   // Capture screen pixels.
   ID3D11Texture2D *screenBuffer;
   m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&screenBuffer);
   D3D11_TEXTURE2D_DESC screenDesc;
   screenBuffer->GetDesc(&screenDesc);

   // Check for pixel pick color code.
   CD3D11_TEXTURE2D_DESC stagingTextureDesc(
      DXGI_FORMAT_B8G8R8A8_UNORM,
      screenDesc.Width,  // Width
      screenDesc.Height, // Height
      1,                 // MipLevels
      1,                 // ArraySize
      0
      );
   stagingTextureDesc.Usage          = D3D11_USAGE_STAGING;
   stagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

   Microsoft::WRL::ComPtr<ID3D11Texture2D> stagingTexture;
   DX::ThrowIfFailed(
      m_d3dDevice->CreateTexture2D(
         &stagingTextureDesc,
         nullptr,
         &stagingTexture
         )
      );

   m_d3dContext->CopyResource(
      stagingTexture.Get(),
      screenBuffer
      );

   unlockDisplay();

   UINT col;
   UINT row;
   switch (DisplayProperties::CurrentOrientation)
   {
   case DisplayOrientations::Landscape:
      col = (int)pickPoint.X;
      row = (int)pickPoint.Y;
      break;

   case DisplayOrientations::Portrait:
      col = (int)(m_d3dRenderTargetSize.Width - pickPoint.Y);
      row = (int)(m_d3dRenderTargetSize.Height - pickPoint.X);
      break;

   case DisplayOrientations::LandscapeFlipped:
      col = (int)(m_d3dRenderTargetSize.Width - pickPoint.X);
      row = (int)(m_d3dRenderTargetSize.Height - pickPoint.Y);
      break;

   case DisplayOrientations::PortraitFlipped:
      col = (int)pickPoint.Y;
      row = (int)pickPoint.X;
      break;
   }

   D3D11_MAPPED_SUBRESOURCE mappedTex;
   DX::ThrowIfFailed(
      m_d3dContext->Map(stagingTexture.Get(),
                        D3D11CalcSubresource(0, 0, 1),
                        D3D11_MAP_READ, 0,
                        &mappedTex
                        )
      );
   UCHAR *pTexels = (UCHAR *)mappedTex.pData;
   UINT  rowStart = row * mappedTex.RowPitch;
   UINT  colStart = col * 4;
   int   result   = 0;
   int   r        = pTexels[rowStart + colStart + 0];
   int   g        = pTexels[rowStart + colStart + 1];
   int   b        = pTexels[rowStart + colStart + 2];
   if ((g == Scene::GREEN_PICK) && (b == Scene::BLUE_PICK))
   {
      result = r;
   }
   m_d3dContext->Unmap(stagingTexture.Get(), D3D11CalcSubresource(0, 0, 1));

   return(result);
}


// Render for picking.
void Renderer::RenderForPicking()
{
   // clear both the render target and depth stencil to default values
   m_d3dContext->OMSetRenderTargets(
      1,
      m_d3dRenderTargetView.GetAddressOf(),
      m_d3dDepthStencilView.Get()
      );

   const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
   m_d3dContext->ClearRenderTargetView(
      m_d3dRenderTargetView.Get(),
      ClearColor
      );

   m_d3dContext->ClearDepthStencilView(
      m_d3dDepthStencilView.Get(),
      D3D11_CLEAR_DEPTH,
      1.0f,
      0
      );

   m_d3dContext->IASetInputLayout(m_coloredInputLayout.Get());

   m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   // Set the vertex and pixel shader stage state.
   m_d3dContext->VSSetShader(
      m_coloredVertexShader.Get(),
      nullptr,
      0
      );

   m_d3dContext->PSSetShader(
      m_coloredPixelShader.Get(),
      nullptr,
      0
      );

   // Render scene.
   if (!m_textOverlay->Render(m_orientationTransform2D))
   {
      if (!isBlind())
      {
         // Set up viewport for scene.
         float           u = m_d3dRenderTargetSize.Height / (float)(LINES - 3);
         CD3D11_VIEWPORT viewport(
            0.0f,
            u,
            m_d3dRenderTargetSize.Width,
            m_d3dRenderTargetSize.Height - (3.0f * u)
            );
         switch (DisplayProperties::CurrentOrientation)
         {
         case DisplayOrientations::Landscape:
            break;

         case DisplayOrientations::Portrait:
            u = m_d3dRenderTargetSize.Width / (float)(LINES - 4);
            viewport.TopLeftX = u;
            viewport.TopLeftY = 0.0f;
            viewport.Width    = m_d3dRenderTargetSize.Width - (3.0f * u);
            viewport.Height   = m_d3dRenderTargetSize.Height;
            break;

         case DisplayOrientations::LandscapeFlipped:
            viewport.TopLeftY = u * 2.0f;
            break;

         case DisplayOrientations::PortraitFlipped:
            u = m_d3dRenderTargetSize.Width / (float)(LINES - 4);
            viewport.TopLeftX = u * 2.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width    = m_d3dRenderTargetSize.Width - (3.0f * u);
            viewport.Height   = m_d3dRenderTargetSize.Height;
            break;
         }
         m_d3dContext->RSSetViewports(1, &viewport);

         m_scene->RenderForPicking();

         // Restore the viewport to the entire window.
         viewport.TopLeftX = viewport.TopLeftY = 0.0f;
         viewport.Width    = m_d3dRenderTargetSize.Width;
         viewport.Height   = m_d3dRenderTargetSize.Height;
         m_d3dContext->RSSetViewports(1, &viewport);
      }
   }
}


void Renderer::SaveInternalState(IPropertySet ^ state)
{
}


void Renderer::LoadInternalState(IPropertySet ^ state)
{
}
