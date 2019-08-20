#pragma once

#include "Camera.h"
#include "Loader.h"
#include "RogueInterface.hpp"
#include "SceneShapes.h"
#include <hash_map>

using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace DirectX;
using namespace std;


ref class Scene
{
internal:

   Scene(vector<Platform::String ^> textureFileNames);

   void Initialize(
      _In_ ID3D11Device1        *d3dDevice,
      _In_ ID3D11DeviceContext1 *d3dContext,
      _In_ ID2D1Device          *d2dDevice,
      _In_ ID2D1DeviceContext   *d2dContext,
      _In_ IDWriteFactory       *dwriteFactory
      );

   void ResetDirectXResources();

   void UpdateForWindowSizeChange(Windows::Foundation::Rect                       windowBounds,
                                  Windows::Graphics::Display::DisplayOrientations orientation,
                                  XMFLOAT4X4&                                     orientationTransform3D);

   void Render();

   // Picking.
   typedef enum { GREEN_PICK = 45, BLUE_PICK = 17 }   PICKING_COLORS;
   void RenderForPicking();

private:

   Microsoft::WRL::ComPtr<ID3D11Device1>        m_d3dDevice;
   Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_d3dContext;
   Microsoft::WRL::ComPtr<ID2D1Device>          m_d2dDevice;
   Microsoft::WRL::ComPtr<ID2D1DeviceContext>   m_d2dContext;
   Microsoft::WRL::ComPtr<IDWriteFactory>       m_dwriteFactory;

   // Drawables.
   SceneShapes ^ m_shapes;
   Microsoft::WRL::ComPtr<ID3D11Buffer> m_blockVertexBuffer;
   Microsoft::WRL::ComPtr<ID3D11Buffer> m_blockIndexBuffer;
   unsigned int m_blockIndexCount;
   Microsoft::WRL::ComPtr<ID3D11Buffer> m_tileVertexBuffer;
   Microsoft::WRL::ComPtr<ID3D11Buffer> m_tileIndexBuffer;
   unsigned int m_tileIndexCount;
   Microsoft::WRL::ComPtr<ID3D11Buffer> m_billboardVertexBuffer;
   Microsoft::WRL::ComPtr<ID3D11Buffer> m_billboardIndexBuffer;
   unsigned int m_billboardIndexCount;

   typedef enum
   {
      Block,
      Tile,
      Billboard,
      ColoredQuad
   } DrawableType;

   struct Drawable
   {
      DrawableType type;
      float4x4     translation;
      float4x4     rotation;
      int          textureIndex;
      float3       color;
   };

   struct ConstantBuffer
   {
      float4x4 model;
      float4x4 view;
      float4x4 projection;
   };
   Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
   ConstantBuffer m_constantBufferData;

   void CreateShapes();

   void RenderShape(Drawable);

   // Transparency.
   D3D11_BLEND_DESC m_transparentStateDesc;
   ID3D11BlendState *m_transparentState;
   D3D11_BLEND_DESC m_opaqueStateDesc;
   ID3D11BlendState *m_opaqueState;

   // Textures.
   vector<Platform::String ^> m_textureFileNames;
   hash_map<int, Microsoft::WRL::ComPtr<ID3D11Texture2D> >          m_textures;
   hash_map<int, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> > m_textureSRVs;
   void CreateTextures(Loader ^ loader);

   enum ShapeTextureIDs
   {
      Wall =0,
      Floor=1,
      Door =2,
      Black=3
   };

   enum CHAR_COLOR_PARMS
   {
      CHAR_OFFSET=32,
      NUM_CHARS  =95
   };

   // Scene maps.
   char m_actual[TEXT_WIDTH][TEXT_HEIGHT];
   char m_visible[TEXT_WIDTH][TEXT_HEIGHT];
   bool m_wall[TEXT_WIDTH][TEXT_HEIGHT];

   enum LOCALE
   {
      IN_ROOM,
      IN_PASSAGE,
      IN_WALL
   };

   // Player properties.
   int            m_playerX;
   int            m_playerZ;
   DIRECTION      m_playerDir;
   Vector3<float> m_position;
   float          m_rotation;
   Vector3<float> m_forward;
   LOCALE         m_locale;
   int            m_xmin, m_xmax;
   int            m_zmin, m_zmax;

   // Placement.
   void PlaceObject(int x, int z, char actual, char visible);
   void PlacePlayer();
   void SetPlayerLocale();
   void SetPassageVisibility(int x, int z, DIRECTION dir);

   // Camera.
   Camera ^ m_camera;
   void CreateCamera();
   void ConfigureCamera(XMFLOAT4X4& orientation);
   void PlaceCamera();
   bool CameraPending();

   static const float        FIELD_OF_VIEW;
   static const float        FRUSTUM_NEAR;
   static const float        FRUSTUM_FAR;
   static const long         VIEW_TRANSITION_DURATION;
   Vector3<float>            m_currentPosition, m_lastPosition;
   Vector3<float>            m_currentForward, m_lastForward;
   float                     m_viewTransitionState;
   int64                     m_viewTransitionTime;
   Windows::Foundation::Rect m_windowBounds;
   Windows::Graphics::Display::DisplayOrientations m_orientation;

   // Billboard rotation.
   class BillboardRotation
   {
public:

      Vector3<float> player;
      Vector3<float> object;
      Vector3<float> axis;
      float          angle;

      BillboardRotation(Vector3<float>& player, Vector3<float>& object);
   };

   // Monster animation.
   class Animator
   {
public:

      static const long  DEATH_ANIMATION_DURATION;
      static const long  HIT_ANIMATION_DURATION;
      static const float HIT_RECOIL_ANGLE;

      char  show;
      int   x, z;
      bool  killed;
      bool  dispose;
      float prog1, prog2;
      int64 startTime;
      float rotation;

      Animator(char show, int x, int z, bool killed)
      {
         this->show   = show;
         this->x      = x;
         this->z      = z;
         this->killed = killed;
         dispose      = false;
         prog1        = prog2 = 0.0f;
         startTime    = GetTickCount64();
         rotation     = 0.0f;
      }


      Animator()
      {
         show      = false;
         x         = z = 0;
         killed    = false;
         dispose   = false;
         prog1     = prog2 = 0.0f;
         startTime = GetTickCount64();
         rotation  = 0.0f;
      }
   };
   vector<Animator> m_animators;
};
