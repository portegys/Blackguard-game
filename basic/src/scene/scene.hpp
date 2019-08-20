// Blackguard scene.

#ifndef SCENE_HPP
#define SCENE_HPP

#include <stdlib.h>
#include <GL/glut.h>
#include "texture.h"
#include "vector.hpp"
#include "gettime.h"

class Scene
{
public:

   // Directions.
   typedef enum
   {
      NORTH     = 0,
      NORTHEAST = 1,
      EAST      = 2,
      SOUTHEAST = 3,
      SOUTH     = 4,
      SOUTHWEST = 5,
      WEST      = 6,
      NORTHWEST = 7
   } DIRECTION;

   // Width and height.
   int WIDTH;
   int HEIGHT;

   // Unit size.
   static const GLfloat UNIT_SIZE;

   // Colors and texture files.
   static const char *OBJECT_COLORS_FILE;
   static const char *WALL_TEXTURE_FILE;
   static const char *DOOR_TEXTURE_FILE;
   static const char *FLOOR_TEXTURE_FILE;

   // Constructor/destructor.
   Scene(int width, int height, GLfloat aspect);
   ~Scene();

   // Place object in scene.
   void placeObject(int x, int z, char actual, char visible);

   // Place player in scene.
   void placePlayer(int x, int z, DIRECTION dir);

   // Camera.
   static const GLfloat FRUSTUM_ANGLE;
   static const GLfloat FRUSTUM_NEAR;
   static const GLfloat FRUSTUM_FAR;
   static const TIME    VIEW_TRANSITION_DURATION;
   void configureCamera(GLfloat aspect);
   void placeCamera();
   bool cameraPending();

   Vector m_currentPosition, m_lastPosition;
   Vector m_currentForward, m_lastForward;
   float  m_viewTransitionState;
   TIME   m_viewTransitionTime;

   // Draw scene.
   void draw();

   // Scene maps.
   char **m_actual;
   char **m_visible;
   bool **m_wall;

   // Player properties.
   Vector  m_position;
   GLfloat m_rotation;
   Vector  m_forward;
   enum
   {
      IN_ROOM   =0,
      IN_PASSAGE=1,
      IN_WALL   =2
   }
   m_locale;
   void setPlayerLocale(int x, int z, DIRECTION dir);

   // Visibility boundaries.
   int m_xmin, m_xmax, m_zmin, m_zmax;
   void setPassageVisibility(int x, int z, DIRECTION dir);

   // OpenGL displays for drawing.
   bool   m_displaysCreated;
   GLuint m_wallDisplay;
   GLuint m_floorTileDisplay;
   GLuint m_ceilingTileDisplay;
   void createDisplays();

   // Object colors.
   enum { COLOR_OFFSET=33, NUM_COLORS=94 };
   unsigned char m_objectColors[NUM_COLORS][3];

   // Get transform for object to face player.
   void getObjectTransform(Vector & player, Vector & object,
                           GLfloat rotation[4], GLfloat & dx, GLfloat & dz, GLfloat & w);

   // Character size and scale.
   static const GLfloat CHAR_SIZE;
   GLfloat              m_charScale;

   // Textures.
   enum
   {
      WALL_TEXTURE  =0,
      DOOR_TEXTURE  =1,
      FLOOR_TEXTURE =2
   };
   GLuint m_textures[3];

   // Monster animation.
   struct Animator
   {
      char            show;
      int             x, z;
      bool            killed;
      bool            dispose;
      float           prog1, prog2;
      TIME            startTime;
      struct Animator *next;
   };
   struct Animator    *m_animators;
   static const TIME  DEATH_ANIMATION_DURATION;
   static const TIME  HIT_ANIMATION_DURATION;
   static const float HIT_RECOIL_ANGLE;
   void addHitAnimation(char show, int x, int z);
   void addKilledAnimation(char show, int x, int z);
   void addAnimation(char show, int x, int z, bool killed);
   void disposeAnimations();

   bool hasAnimations() { return(m_animators != NULL); }
};
#endif
