// Blackguard scene.

#ifdef WIN32
#include <windows.h>
#include <io.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "scene.hpp"

// Unit size.
const GLfloat Scene::UNIT_SIZE = 0.05f;

// Graphics character size.
const GLfloat Scene::CHAR_SIZE = 104.76f;

// Object colors file.
// Format: <character> <red> <green> <blue> (0-255)
const char *Scene::OBJECT_COLORS_FILE = "object_colors.txt";

// Wall texture file.
const char *Scene::WALL_TEXTURE_FILE = "wall.bmp";

// Door texture file.
const char *Scene::DOOR_TEXTURE_FILE = "door.bmp";

//Floor texture file.
const char *Scene::FLOOR_TEXTURE_FILE = "floor.bmp";

// Constructor.
Scene::Scene(int width, int height, GLfloat aspect)
{
   int           i, j, r, g, b;
   unsigned char c;
   char          filename[128], path[1024], buf[50];
   FILE          *fp;
   bool          getResourcePath(char *filename, char *path, int len);

   assert(width > 0 && height > 0);
   this->WIDTH       = width;
   this->HEIGHT      = height;
   m_displaysCreated = false;

   // Allocate maps.
   m_actual = new char *[width];
   assert(m_actual != NULL);
   m_visible = new char *[width];
   assert(m_visible != NULL);
   m_wall = new bool *[width];
   assert(m_wall != NULL);
   for (i = 0; i < width; i++)
   {
      m_actual[i] = new char[height];
      assert(m_actual[i] != NULL);
      m_visible[i] = new char[height];
      assert(m_visible[i] != NULL);
      m_wall[i] = new bool[height];
      assert(m_wall[i] != NULL);
      for (j = 0; j < height; j++)
      {
         m_actual[i][j]  = ' ';
         m_visible[i][j] = ' ';
         m_wall[i][j]    = true;
      }
   }

   // Player.
   m_position.Zero();
   m_rotation  = 180.0f;
   m_forward.x = 0.0f;
   m_forward.y = 0.0f;
   m_forward.z = -1.0f;

   // Configure camera.
   m_viewTransitionState = 0.0f;
   m_viewTransitionTime  = gettime();
   configureCamera(aspect);

   // Character scale.
   m_charScale = (UNIT_SIZE / CHAR_SIZE) * 0.8f;

   // Initialize object colors.
   for (i = 0; i < NUM_COLORS; i++)
   {
      j = i + COLOR_OFFSET;
      if (((j >= (int)'a') && (j <= (int)'z')) ||
          ((j >= (int)'A') && (j <= (int)'Z')))
      {
         m_objectColors[i][0] = 255;
         m_objectColors[i][1] = 0;
         m_objectColors[i][2] = 0;
      }
      else
      {
         m_objectColors[i][0] = 0;
         m_objectColors[i][1] = 255;
         m_objectColors[i][2] = 0;
      }
   }
   if (getResourcePath((char *)OBJECT_COLORS_FILE, path, 1024))
   {
      if ((fp = fopen(path, "r")) != NULL)
      {
         while (fgets(buf, 49, fp) != NULL)
         {
            if (sscanf(buf, "%c %d %d %d", &c, &r, &g, &b) == 4)
            {
               i = (int)c - COLOR_OFFSET;
               if ((i >= 0) && (i < NUM_COLORS))
               {
                  m_objectColors[i][0] = (unsigned char)r;
                  m_objectColors[i][1] = (unsigned char)g;
                  m_objectColors[i][2] = (unsigned char)b;
               }
            }
         }
         fclose(fp);
      }
   }

   // Load textures.
   sprintf(filename, "textures/%s", (char *)WALL_TEXTURE_FILE);
   if (!getResourcePath(filename, path, 1024) &&
       !getResourcePath((char *)WALL_TEXTURE_FILE, path, 1024))
   {
      fprintf(stderr, "Cannot access %s\n", (char *)WALL_TEXTURE_FILE);
      exit(1);
   }
   if (!CreateTexture(path, m_textures, WALL_TEXTURE))
   {
      fprintf(stderr, "Cannot load texture from %s\n", path);
      exit(1);
   }
   sprintf(filename, "textures/%s", (char *)DOOR_TEXTURE_FILE);
   if (!getResourcePath(filename, path, 1024) &&
       !getResourcePath((char *)DOOR_TEXTURE_FILE, path, 1024))
   {
      fprintf(stderr, "Cannot access %s\n", (char *)DOOR_TEXTURE_FILE);
      exit(1);
   }
   if (!CreateTexture(path, m_textures, DOOR_TEXTURE))
   {
      fprintf(stderr, "Cannot load texture from %s\n", path);
      exit(1);
   }
   sprintf(filename, "textures/%s", (char *)FLOOR_TEXTURE_FILE);
   if (!getResourcePath(filename, path, 1024) &&
       !getResourcePath((char *)FLOOR_TEXTURE_FILE, path, 1024))
   {
      fprintf(stderr, "Cannot access %s\n", (char *)FLOOR_TEXTURE_FILE);
      exit(1);
   }
   if (!CreateTexture(path, m_textures, FLOOR_TEXTURE))
   {
      fprintf(stderr, "Cannot load texture from %s\n", path);
      exit(1);
   }

   m_animators = NULL;
}


// Destructor.
Scene::~Scene()
{
   for (int i = 0; i < WIDTH; i++)
   {
      delete [] m_actual[i];
      delete [] m_visible[i];
      delete [] m_wall[i];
   }
   delete [] m_actual;
   delete [] m_visible;
   delete [] m_wall;
   if (m_displaysCreated)
   {
      glDeleteLists(m_wallDisplay, 3);
   }
   glDeleteTextures(3, m_textures);

   struct Animator *a;
   while (m_animators != NULL)
   {
      a           = m_animators;
      m_animators = a->next;
      delete a;
   }
}


// Place object in scene.
void Scene::placeObject(int x, int z, char actual, char visible)
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
}


// Place player in scene.
void Scene::placePlayer(int x, int z, DIRECTION dir)
{
   m_position.x = ((GLfloat)x * UNIT_SIZE) + (UNIT_SIZE * 0.5f);
   m_position.y = UNIT_SIZE * 0.5f;
   m_position.z = ((GLfloat)z * UNIT_SIZE) + (UNIT_SIZE * 0.5f);
   switch (dir)
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
   setPlayerLocale(x, z, dir);
}


// Set player locale and visible bounds.
void Scene::setPlayerLocale(int x, int z, DIRECTION dir)
{
   int i, j, k, w, h, x2, z2;

   m_xmin = x - 1;
   m_xmax = x + 1;
   m_zmin = z - 1;
   m_zmax = z + 1;
   if (m_xmin < 0)
   {
      m_xmin = 0;
   }
   if (m_xmax >= WIDTH)
   {
      m_xmax = WIDTH - 1;
   }
   if (m_zmin < 0)
   {
      m_zmin = 0;
   }
   if (m_zmax >= HEIGHT)
   {
      m_zmax = HEIGHT - 1;
   }

   if (m_wall[x][z])
   {
      m_locale = IN_WALL;
      return;
   }
   m_locale = IN_PASSAGE;
   for (i = 0; i < WIDTH; i++)
   {
      for (j = 0; j < HEIGHT; j++)
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
                     setPassageVisibility(x, z, dir);
                  }
               }
               return;
            }
         }
      }
   }
   setPassageVisibility(x, z, dir);
}


// Set passage visibility bounds.
void Scene::setPassageVisibility(int x, int z, DIRECTION dir)
{
   int       xmin2, xmax2, zmin2, zmax2;
   DIRECTION dir2, dir3;

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
      for (m_zmax = z; !m_wall[x][m_zmax] && m_zmax < HEIGHT - 1; m_zmax++)
      {
      }
      xmin2 = x - 3;
      xmax2 = x + 3;
      break;

   case WEST:
      for (m_xmax = x; !m_wall[m_xmax][z] && m_xmax < WIDTH - 1; m_xmax++)
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
         for (m_zmax = z; !m_wall[x][m_zmax] && m_zmax < HEIGHT - 1; m_zmax++)
         {
         }
         xmin2 = x - 3;
         xmax2 = x + 3;
         break;

      case WEST:
         for (m_xmax = x; !m_wall[m_xmax][z] && m_xmax < WIDTH - 1; m_xmax++)
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
   if (m_xmax >= WIDTH)
   {
      m_xmax = WIDTH - 1;
   }
   if (m_zmin < 0)
   {
      m_zmin = 0;
   }
   if (m_zmax >= HEIGHT)
   {
      m_zmax = HEIGHT - 1;
   }
}


// Camera properties.
const GLfloat Scene::FRUSTUM_ANGLE            = 60.0f;
const GLfloat Scene::FRUSTUM_NEAR             = 0.01f;
const GLfloat Scene::FRUSTUM_FAR              = 100.0f;
const TIME Scene::   VIEW_TRANSITION_DURATION = 200;

// Configure camera.
void Scene::configureCamera(GLfloat aspect)
{
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(FRUSTUM_ANGLE, aspect, FRUSTUM_NEAR, FRUSTUM_FAR);
   glMatrixMode(GL_MODELVIEW);
}


// Smooth transition camera to player viewpoint.
void Scene::placeCamera()
{
   Vector position, forward;

   // View is changing?
   if ((m_position != m_currentPosition) || (m_forward != m_currentForward))
   {
      m_lastPosition        = m_currentPosition;
      m_lastForward         = m_currentForward;
      m_currentPosition     = m_position;
      m_currentForward      = m_forward;
      m_viewTransitionState = 1.0f;
      m_viewTransitionTime  = gettime();
      if (fabs(m_currentPosition.x - m_lastPosition.x) > (UNIT_SIZE * 1.5f))
      {
         if (m_currentPosition.x < m_lastPosition.x)
         {
            m_lastPosition.x = m_currentPosition.x + UNIT_SIZE;
         }
         else
         {
            m_lastPosition.x = m_currentPosition.x - UNIT_SIZE;
         }
      }
      if (fabs(m_currentPosition.z - m_lastPosition.z) > (UNIT_SIZE * 1.5f))
      {
         if (m_currentPosition.z < m_lastPosition.z)
         {
            m_lastPosition.z = m_currentPosition.z + UNIT_SIZE;
         }
         else
         {
            m_lastPosition.z = m_currentPosition.z - UNIT_SIZE;
         }
      }
   }

   // Set camera to intermediate viewpoint.
   m_viewTransitionState =
      (float)(gettime() - m_viewTransitionTime) / (float)VIEW_TRANSITION_DURATION;
   if (m_viewTransitionState > 1.0f)
   {
      m_viewTransitionState = 1.0f;
   }
   m_viewTransitionState = 1.0f - m_viewTransitionState;
   position = (m_currentPosition * (1.0f - m_viewTransitionState)) +
              (m_lastPosition * m_viewTransitionState);
   forward = (m_currentForward * (1.0f - m_viewTransitionState)) +
             (m_lastForward * m_viewTransitionState);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   gluLookAt(position.x, position.y, position.z,
             position.x + forward.x, position.y + forward.y,
             position.z + forward.z,
             0.0f, 1.0f, 0.0f);
}


// Camera has pending updates?
bool Scene::cameraPending()
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


// Draw scene.
void Scene::draw()
{
   int           i, j, k;
   unsigned char r, g, b;
   GLfloat       x, z, dx, dz, w, rotation[4];
   Vector        player, object;

   glMatrixMode(GL_MODELVIEW);

   // Build block and tile displays?
   if (!m_displaysCreated)
   {
      createDisplays();
      m_displaysCreated = true;
   }

   // Inside of wall?
   if (m_locale == IN_WALL)
   {
      return;
   }

   glPushMatrix();
   glLoadIdentity();

   // Place camera at player view point.
   placeCamera();

   // Get player position.
   player.x = m_position.x;
   player.y = 0.0f;
   player.z = m_position.z;

   // Draw textured topology.
   glEnable(GL_TEXTURE_2D);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glColor3f(1.0f, 1.0f, 1.0f);
   for (i = m_xmin; i <= m_xmax; i++)
   {
      for (j = m_zmin; j <= m_zmax; j++)
      {
         x = (GLfloat)i * UNIT_SIZE;
         z = (GLfloat)j * UNIT_SIZE;
         if (m_wall[i][j])
         {
            if ((m_locale == IN_ROOM) && (m_visible[i][j] != ' '))
            {
               // Visible room wall.
               glPushMatrix();
               glTranslatef(x, 0.0f, z);
               glBindTexture(GL_TEXTURE_2D, m_textures[WALL_TEXTURE]);
               glCallList(m_wallDisplay);
               glPopMatrix();
            }
         }
         else if (m_actual[i][j] == '+')
         {
            if (m_visible[i][j] != ' ')
            {
               // Visible door.
               glPushMatrix();
               glTranslatef(x, 0.0f, z);
               glBindTexture(GL_TEXTURE_2D, m_textures[DOOR_TEXTURE]);
               glCallList(m_wallDisplay);
               glPopMatrix();
            }
         }
         else if (m_actual[i][j] == '&')
         {
            if (m_visible[i][j] != ' ')
            {
               // Secret door looks like wall.
               glPushMatrix();
               glTranslatef(x, 0.0f, z);
               glBindTexture(GL_TEXTURE_2D, m_textures[WALL_TEXTURE]);
               glCallList(m_wallDisplay);
               glPopMatrix();
            }
         }
         else if (m_visible[i][j] != ' ')
         {
            // Draw floor.
            glPushMatrix();
            glTranslatef(x, 0.0f, z);
            glBindTexture(GL_TEXTURE_2D, m_textures[FLOOR_TEXTURE]);
            glCallList(m_floorTileDisplay);
            glPopMatrix();
         }
      }
   }
   glDisable(GL_TEXTURE_2D);

   // Do monster animations.
   struct Animator *a, *h;
   h = NULL;
   TIME  t = gettime();
   float rot;
   while (m_animators != NULL)
   {
      a           = m_animators;
      m_animators = a->next;
      if (a->killed)
      {
         if (a->prog1 >= 1.0f)
         {
            if (a->dispose)
            {
               delete a;
               continue;
            }
         }
         else
         {
            a->prog1 = (float)(t - a->startTime) / (float)DEATH_ANIMATION_DURATION;
            if (a->prog1 > 1.0f)
            {
               a->prog1 = 1.0f;
            }
         }
         rot = -(a->prog1 * 90.0f);
      }
      else
      {
         if (a->prog1 < 1.0f)
         {
            a->prog1 = (float)(t - a->startTime) / (float)HIT_ANIMATION_DURATION;
            if (a->prog1 > 1.0f)
            {
               a->prog1 = 1.0f;
            }
            rot = -(a->prog1 * HIT_RECOIL_ANGLE);
         }
         else if (a->prog2 < 1.0f)
         {
            a->prog2 = (float)(t - a->startTime) / (float)HIT_ANIMATION_DURATION;
            if (a->prog2 > 1.0f)
            {
               a->prog2 = 1.0f;
            }
            rot = -(HIT_RECOIL_ANGLE - (a->prog2 * HIT_RECOIL_ANGLE));
         }
         else if (a->dispose)
         {
            delete a;
            continue;
         }
         else
         {
            rot = 0.0f;
         }
      }
      a->next = h;
      h       = a;
      m_visible[a->x][a->z] = ' ';
      k = (int)(a->show) - COLOR_OFFSET;
      r = m_objectColors[k][0];
      g = m_objectColors[k][1];
      b = m_objectColors[k][2];
      glColor3ub(r, g, b);
      glPushMatrix();
      x        = (GLfloat)(a->x) * UNIT_SIZE;
      z        = (GLfloat)(a->z) * UNIT_SIZE;
      object.x = x + (UNIT_SIZE * 0.5f);
      object.y = 0.0f;
      object.z = z + (UNIT_SIZE * 0.5f);
      getObjectTransform(player, object, rotation, dx, dz, w);
      glLineWidth(w);
      glTranslatef(x + (UNIT_SIZE * 0.5f) + dx, UNIT_SIZE * 0.1f, z + (UNIT_SIZE * 0.5f) + dz);
      glRotatef(rotation[3], rotation[0], rotation[1], rotation[2]);
      glRotatef(rot, 1.0f, 0.0f, 0.0f);
      glScalef(m_charScale, m_charScale, m_charScale);
      glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, a->show);
      glPopMatrix();
      glLineWidth(1.0f);
   }
   m_animators = h;

   // Draw dark topology and objects.
   for (i = m_xmin; i <= m_xmax; i++)
   {
      for (j = m_zmin; j <= m_zmax; j++)
      {
         x = (GLfloat)i * UNIT_SIZE;
         z = (GLfloat)j * UNIT_SIZE;
         if (m_wall[i][j])
         {
            if ((m_locale == IN_PASSAGE) || (m_visible[i][j] == ' '))
            {
               // Dark wall.
               glPushMatrix();
               glTranslatef(x, 0.0f, z);
               glColor3f(0.0f, 0.0f, 0.0f);
               glCallList(m_wallDisplay);
               glPopMatrix();
            }
         }
         else
         {
            if (m_visible[i][j] != ' ')
            {
               if ((m_visible[i][j] != '|') && (m_visible[i][j] != '-') &&
                   (m_visible[i][j] != '+') && (m_visible[i][j] != '&') &&
                   (m_visible[i][j] != '.') && (m_visible[i][j] != '#') &&
                   (m_visible[i][j] != '@'))
               {
                  // Draw object.
                  k = (int)m_visible[i][j] - COLOR_OFFSET;
                  r = m_objectColors[k][0];
                  g = m_objectColors[k][1];
                  b = m_objectColors[k][2];
                  glColor3ub(r, g, b);
                  glPushMatrix();
                  object.x = x + (UNIT_SIZE * 0.5f);
                  object.y = 0.0f;
                  object.z = z + (UNIT_SIZE * 0.5f);
                  getObjectTransform(player, object, rotation, dx, dz, w);
                  glLineWidth(w);
                  glTranslatef(x + (UNIT_SIZE * 0.5f) + dx, UNIT_SIZE * 0.1f, z + (UNIT_SIZE * 0.5f) + dz);
                  glRotatef(rotation[3], rotation[0], rotation[1], rotation[2]);
                  glScalef(m_charScale, m_charScale, m_charScale);
                  glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, m_visible[i][j]);
                  glPopMatrix();
                  glLineWidth(1.0f);
               }
            }
         }
      }
   }

   glPopMatrix();
}


// Get transform for object to face player.
void Scene::getObjectTransform(Vector& player, Vector& object,
                               GLfloat rotation[4], GLfloat& dx, GLfloat& dz, GLfloat& w)
{
   Vector  target, source, axis;
   GLfloat angle, d;

   rotation[0] = 0.0f;
   rotation[1] = -1.0f;
   rotation[2] = 0.0f;
   rotation[3] = 0.0f;
   dx          = dz = 0.0f;

   // Determine line width.
   d = fabs(player.x - object.x) + fabs(player.z - object.z);
   if (d <= 2.0f * UNIT_SIZE)
   {
      w = 5.0f;
   }
   else if (d <= 4.0f * UNIT_SIZE)
   {
      w = 4.0f;
   }
   else if (d <= 6.0f * UNIT_SIZE)
   {
      w = 3.0f;
   }
   else if (d <= 8.0f * UNIT_SIZE)
   {
      w = 2.0f;
   }
   else
   {
      w = 1.0f;
   }

   if (fabs(player.x - object.x) < (UNIT_SIZE * 0.5f))
   {
      if (player.z < object.z)
      {
         rotation[3] = 180.0f;
         dx          = UNIT_SIZE * 0.4f;
         dz          = 0.0f;
      }
      else
      {
         rotation[3] = 0.0f;
         dx          = -UNIT_SIZE * 0.4f;
         dz          = 0.0f;
      }
      return;
   }
   else if (fabs(player.z - object.z) < (UNIT_SIZE * 0.5f))
   {
      if (player.x < object.x)
      {
         rotation[3] = 90.0f;
         dx          = 0.0f;
         dz          = -UNIT_SIZE * 0.4f;
      }
      else
      {
         rotation[3] = 270.0f;
         dx          = 0.0f;
         dz          = UNIT_SIZE * 0.4f;
      }
      return;
   }
   target.x = player.x - object.x;
   target.y = 0.0f;
   target.z = player.z - object.z;
   target.Normalize();
   source.x = source.y = 0.0f;
   source.z = -1.0f;
   axis     = target ^ source;
   dx       = target * source;
   angle    = acos(dx);
   angle    = RadiansToDegrees(angle);
   if (axis.y > 0.0f)
   {
      angle += 180.0f;
   }
   else
   {
      angle = 180.0f - angle;
   }
   rotation[3] = angle;
   dx          = fabs(dx);
   dz          = 1.0f - dx;
   if ((angle > 0.0f) && (angle < 180.0f))
   {
      dz = -dz;
   }
   if ((angle < 90.0f) || (angle > 270.0f))
   {
      dx = -dx;
   }
   dx *= (UNIT_SIZE * 0.4f);
   dz *= (UNIT_SIZE * 0.4f);
}


// Monster animation parameters.
const TIME Scene:: DEATH_ANIMATION_DURATION = 250;
const TIME Scene:: HIT_ANIMATION_DURATION   = 100;
const float Scene::HIT_RECOIL_ANGLE         = 45.0f;

// Add hit monster animation.
void Scene::addHitAnimation(char show, int x, int z)
{
   addAnimation(show, x, z, false);
}


// Add killed monster animation.
void Scene::addKilledAnimation(char show, int x, int z)
{
   addAnimation(show, x, z, true);
}


// Add monster animation.
void Scene::addAnimation(char show, int x, int z, bool killed)
{
   struct Animator *a, *a2;

   // Remove previous duplicate.
   for (a = m_animators, a2 = NULL; a != NULL; a2 = a, a = a->next)
   {
      if ((a->x == x) && (a->z == z) && (a->show == show))
      {
         if (a2 == NULL)
         {
            m_animators = a->next;
         }
         else
         {
            a2->next = a->next;
         }
         delete a;
         break;
      }
   }

   // Add animation.
   a            = new struct Animator;
   a->show      = show;
   a->x         = x;
   a->z         = z;
   a->prog1     = a->prog2 = 0.0f;
   a->killed    = killed;
   a->dispose   = false;
   a->startTime = gettime();
   a->next      = m_animators;
   m_animators  = a;
}


// Dispose of monster animations.
void Scene::disposeAnimations()
{
   struct Animator *a;

   for (a = m_animators; a != NULL; a = a->next)
   {
      a->dispose = true;
   }
}


// Create displays.
void Scene::createDisplays()
{
   GLfloat x, y, z;

   m_wallDisplay = glGenLists(3);
   glNewList(m_wallDisplay, GL_COMPILE);
   glBegin(GL_QUADS);
   x = 0.0f;
   y = 0.0f;
   z = 0.0f;
   glTexCoord2f(1.0f, 0.0f);
   glVertex3f(x, y, z);
   x = 0.0f;
   y = UNIT_SIZE;
   z = 0.0f;
   glTexCoord2f(1.0f, 1.0f);
   glVertex3f(x, y, z);
   x = UNIT_SIZE;
   y = UNIT_SIZE;
   z = 0.0f;
   glTexCoord2f(0.0f, 1.0f);
   glVertex3f(x, y, z);
   x = UNIT_SIZE;
   y = 0.0f;
   z = 0.0f;
   glTexCoord2f(0.0f, 0.0f);
   glVertex3f(x, y, z);
   glEnd();
   glBegin(GL_QUADS);
   x = UNIT_SIZE;
   y = 0.0f;
   z = 0.0f;
   glTexCoord2f(1.0f, 0.0f);
   glVertex3f(x, y, z);
   x = UNIT_SIZE;
   y = UNIT_SIZE;
   z = 0.0f;
   glTexCoord2f(1.0f, 1.0f);
   glVertex3f(x, y, z);
   x = UNIT_SIZE;
   y = UNIT_SIZE;
   z = UNIT_SIZE;
   glTexCoord2f(0.0f, 1.0f);
   glVertex3f(x, y, z);
   x = UNIT_SIZE;
   y = 0.0f;
   z = UNIT_SIZE;
   glTexCoord2f(0.0f, 0.0f);
   glVertex3f(x, y, z);
   glEnd();
   glBegin(GL_QUADS);
   x = UNIT_SIZE;
   y = 0.0f;
   z = UNIT_SIZE;
   glTexCoord2f(1.0f, 0.0f);
   glVertex3f(x, y, z);
   x = UNIT_SIZE;
   y = UNIT_SIZE;
   z = UNIT_SIZE;
   glTexCoord2f(1.0f, 1.0f);
   glVertex3f(x, y, z);
   x = 0.0f;
   y = UNIT_SIZE;
   z = UNIT_SIZE;
   glTexCoord2f(0.0f, 1.0f);
   glVertex3f(x, y, z);
   x = 0.0f;
   y = 0.0f;
   z = UNIT_SIZE;
   glTexCoord2f(0.0f, 0.0f);
   glVertex3f(x, y, z);
   glEnd();
   glBegin(GL_QUADS);
   x = 0.0f;
   y = 0.0f;
   z = UNIT_SIZE;
   glTexCoord2f(1.0f, 0.0f);
   glVertex3f(x, y, z);
   x = 0.0f;
   y = UNIT_SIZE;
   z = UNIT_SIZE;
   glTexCoord2f(1.0f, 1.0f);
   glVertex3f(x, y, z);
   x = 0.0f;
   y = UNIT_SIZE;
   z = 0.0f;
   glTexCoord2f(0.0f, 1.0f);
   glVertex3f(x, y, z);
   x = 0.0f;
   y = 0.0f;
   z = 0.0f;
   glTexCoord2f(0.0f, 0.0f);
   glVertex3f(x, y, z);
   glEnd();
   glEndList();
   m_floorTileDisplay = m_wallDisplay + 1;
   glNewList(m_floorTileDisplay, GL_COMPILE);
   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 0.0f);
   glVertex3f(0.0f, 0.0f, 0.0f);
   glTexCoord2f(0.0f, 1.0f);
   glVertex3f(0.0f, 0.0f, UNIT_SIZE);
   glTexCoord2f(1.0f, 1.0f);
   glVertex3f(UNIT_SIZE, 0.0f, UNIT_SIZE);
   glTexCoord2f(1.0f, 0.0f);
   glVertex3f(UNIT_SIZE, 0.0f, 0.0f);
   glEnd();
   glEndList();
   m_ceilingTileDisplay = m_floorTileDisplay + 1;
   glNewList(m_ceilingTileDisplay, GL_COMPILE);
   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 0.0f);
   glVertex3f(0.0f, 0.0f, 0.0f);
   glTexCoord2f(1.0f, 0.0f);
   glVertex3f(UNIT_SIZE, 0.0f, 0.0f);
   glTexCoord2f(1.0f, 1.0f);
   glVertex3f(UNIT_SIZE, 0.0f, UNIT_SIZE);
   glTexCoord2f(0.0f, 1.0f);
   glVertex3f(0.0f, 0.0f, UNIT_SIZE);
   glEnd();
   glEndList();
}
