#pragma once

#include "DirectXBase.h"
#include "TextOverlay.h"
#include "Scene.h"

ref class Renderer sealed : public DirectXBase
{
public:

   Renderer();

   virtual void CreateDeviceIndependentResources() override;
   virtual void CreateDeviceResources() override;
   virtual void CreateWindowSizeDependentResources() override;
   virtual void UpdateForWindowSizeChange() override;
   virtual void Render() override;

   // Rendering activity.
   void Activate()
   {
      m_timer->Reset();
      m_active = true;
   }


   bool IsActive() { return(m_active); }

   // View state.
   bool InOverview() { return(m_textOverlay->view == TextOverlay::VIEW::OVERVIEW); }
   void Overview() { m_textOverlay->view = TextOverlay::VIEW::OVERVIEW; }
   bool InFPview() { return(m_textOverlay->view == TextOverlay::VIEW::FPVIEW); }
   void FPview() { m_textOverlay->view = TextOverlay::VIEW::FPVIEW; }

   // Prompt present?
   bool PromptPresent() { return(m_textOverlay->promptPresent); }

   // Character picking.
   int PickCharacter(Windows::Foundation::Point);

   // Methods to save and load state.
   void SaveInternalState(Windows::Foundation::Collections::IPropertySet ^ state);
   void LoadInternalState(Windows::Foundation::Collections::IPropertySet ^ state);

private:

   // Text overlay.
   TextOverlay ^ m_textOverlay;

   // Scene.
   Scene ^ m_scene;
   vector<Platform::String ^> m_textureFileNames;

   // Rendering.
   enum { ACTIVE_RENDER_SECONDS = 10 };
   bool m_active;
   Timer ^ m_timer;
   Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_inputLayout;                       // vertex input layout
   Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;                      // vertex shader
   Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;                       // pixel shader
   Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler;                           // texture sampler

   // Character picking.
   Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_coloredInputLayout;
   Microsoft::WRL::ComPtr<ID3D11VertexShader> m_coloredVertexShader;
   Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_coloredPixelShader;
   void RenderForPicking();
};
