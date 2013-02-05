// Blackguard scene.

package com.dialectek.blackguard;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.opengl.GLU;
import android.opengl.GLUtils;
import android.os.SystemClock;
import android.util.FloatMath;
import android.util.Log;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.Vector;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Scene
{
   private static final String LOG_TAG = Scene.class .getSimpleName();

   // Width and height.
   int WIDTH;
   int HEIGHT;

   // Ready?
   public boolean ready;

   // Unit size.
   public static final float UNIT_SIZE = 5.0f;

   // Context.
   Context m_context;

   // Scene maps.
   char    m_actual[][];
   char    m_visible[][];
   boolean m_wall[][];

   // Directions.
   public static enum DIRECTION
   {
      NORTH,
      NORTHEAST,
      EAST,
      SOUTHEAST,
      SOUTH,
      SOUTHWEST,
      WEST,
      NORTHWEST
   };

   // Player properties.
   int       m_playerX;
   int       m_playerZ;
   DIRECTION m_playerDir;
   float[]  m_position;
   float m_rotation;
   float[]  m_forward;
   enum LOCALE
   {
      IN_ROOM,
      IN_PASSAGE,
      IN_WALL
   };
   LOCALE m_locale;
   int    m_xmin, m_xmax;
   int    m_zmin, m_zmax;

   // Text drawing.
   LabelMaker m_labelMaker;
   int[] m_charLabels;
   int[][] m_charColors;
   enum CHAR_LABEL_PARMS
   {
      CHAR_OFFSET(32),
      NUM_CHARS(95),
      TEXT_SIZE(128);
      private int value;
      CHAR_LABEL_PARMS(int value) { this.value = value; }
      int getValue() { return(value); }
   };

   // Topology textures.
   enum TEXTURE_TYPE
   {
      WALL_TEXTURE(0),
      DOOR_TEXTURE(1),
      FLOOR_TEXTURE(2);
      private int value;
      TEXTURE_TYPE(int value) { this.value = value; }
      int getValue() { return(value); }
   };
   int[] m_textures;

   // Textures are valid?
   public boolean texturesValid;

   // Wall, floor, and object meshes.
   Wall      m_wallBlock;
   Floor     m_floor;
   Billboard m_billboard;

   // Monster animation.
   class Animator
   {
      char    show;
      int     x, z;
      boolean killed;
      boolean dispose;
      float   prog1, prog2;
      long    startTime;
      Animator(char show, int x, int z, boolean killed)
      {
         this.show   = show;
         this.x      = x;
         this.z      = z;
         this.killed = killed;
         dispose     = false;
         prog1       = prog2 = 0.0f;
         startTime   = SystemClock.uptimeMillis();
      }
   };
   Vector<Animator>   m_animators;
   static final long  DEATH_ANIMATION_DURATION = 250;
   static final long  HIT_ANIMATION_DURATION   = 100;
   static final float HIT_RECOIL_ANGLE         = 45.0f;
   private native boolean disposeAnimations();

   private native int[] getAnimator();

   // Picking objects.
   public Vector<float[]> pickingObjects;

   // Constructor.
   public Scene(Context context)
   {
      ready          = false;
      texturesValid  = false;
      m_context      = context;
      m_labelMaker   = null;
      m_animators    = new Vector<Animator>();
      pickingObjects = new Vector<float[]>();
   }


   // Initialize.
   public void init(GL10 gl, int width, int height, float aspect)
   {
      int         i, j, r, g, b;
      char        c;
      InputStream is;

      // Set dungeon dimensions.
      WIDTH  = width;
      HEIGHT = height;

      // Allocate maps.
      m_actual  = new char[WIDTH][HEIGHT];
      m_visible = new char[WIDTH][HEIGHT];
      m_wall    = new boolean[WIDTH][HEIGHT];
      for (i = 0; i < WIDTH; i++)
      {
         for (j = 0; j < HEIGHT; j++)
         {
            m_actual[i][j]  = ' ';
            m_visible[i][j] = ' ';
            m_wall[i][j]    = true;
         }
      }

      // Player.
      m_position = new float[3];
      for (i = 0; i < 3; i++) { m_position[i] = 0.0f; }
      m_rotation   = 180.0f;
      m_forward    = new float[3];
      m_forward[0] = 0.0f;
      m_forward[1] = 0.0f;
      m_forward[2] = -1.0f;

      // Initialize camera.
      initCamera(gl, aspect);

      // Load character colors.
      m_charColors = new int[CHAR_LABEL_PARMS.NUM_CHARS.getValue()][3];
      for (i = 0; i < CHAR_LABEL_PARMS.NUM_CHARS.getValue(); i++)
      {
         j = i + CHAR_LABEL_PARMS.CHAR_OFFSET.getValue();
         if (((j >= (int)'a') && (j <= (int)'z')) ||
             ((j >= (int)'A') && (j <= (int)'Z')))
         {
            m_charColors[i][0] = 255;
            m_charColors[i][1] = 0;
            m_charColors[i][2] = 0;
         }
         else
         {
            m_charColors[i][0] = 0;
            m_charColors[i][1] = 255;
            m_charColors[i][2] = 0;
         }
      }

      try
      {
         if ((is = m_context.getAssets().open("object_colors.txt")) != null)
         {
            BufferedReader in = new BufferedReader(new InputStreamReader(is));
            String         s;
            while ((s = in.readLine()) != null)
            {
               String[] t = s.split("\\s+");
               if (t.length >= 4)
               {
                  c = new StringBuffer(t[0]).charAt(0);
                  r = Integer.parseInt(t[1]);
                  g = Integer.parseInt(t[2]);
                  b = Integer.parseInt(t[3]);
                  i = (int)c - CHAR_LABEL_PARMS.CHAR_OFFSET.getValue();
                  if ((i >= 0) && (i < CHAR_LABEL_PARMS.NUM_CHARS.getValue()))
                  {
                     m_charColors[i][0] = r;
                     m_charColors[i][1] = g;
                     m_charColors[i][2] = b;
                  }
               }
            }
            in.close();
         }
      }
      catch (IOException e)
      {
         Log.w("Blackguard", "Cannot open object_colors.txt file");
      }

      // Initialize vertex and texture coordinates.
      m_wallBlock = new Wall();
      m_floor     = new Floor();
      m_billboard = new Billboard();

      // Ready.
      ready = true;
   }


   // Make textures.
   public void makeTextures(GL10 gl)
   {
      int         i;
      InputStream is;

      // Create character texture labels.
      if (m_labelMaker != null)
      {
         m_labelMaker.shutdown(gl);
      }
      int textSize = CHAR_LABEL_PARMS.TEXT_SIZE.getValue();
      int x        = CHAR_LABEL_PARMS.NUM_CHARS.getValue() * textSize * 2;
      int y        = textSize * 2;
      int w2, h2;
      for (w2 = 2; w2 < x; w2 *= 2) {}
      for (h2 = 2; h2 < y; h2 *= 2) {}
      m_labelMaker = new LabelMaker(true, w2, h2);
      m_labelMaker.initialize(gl);
      m_labelMaker.beginAdding(gl);
      Paint labelPaint = new Paint();
      labelPaint.setTypeface(Typeface.MONOSPACE);
      labelPaint.setTextSize(textSize);
      labelPaint.setAntiAlias(true);
      m_charLabels = new int[CHAR_LABEL_PARMS.NUM_CHARS.getValue()];
      for (i = 0; i < m_charLabels.length; i++)
      {
         labelPaint.setARGB(0xff, m_charColors[i][0], m_charColors[i][1], m_charColors[i][2]);
         m_charLabels[i] =
            m_labelMaker.add(gl,
                             Character.toString((char)(i + CHAR_LABEL_PARMS.CHAR_OFFSET.getValue())),
                             labelPaint);
      }
      m_labelMaker.endAdding(gl);

      // Create topology textures.
      m_textures = new int[3];
      gl.glGenTextures(3, m_textures, 0);
      for (i = 0; i < 3; i++)
      {
         gl.glBindTexture(GL10.GL_TEXTURE_2D, m_textures[i]);
         gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER,
                            GL10.GL_NEAREST);
         gl.glTexParameterf(GL10.GL_TEXTURE_2D,
                            GL10.GL_TEXTURE_MAG_FILTER,
                            GL10.GL_LINEAR);
         gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S,
                            GL10.GL_CLAMP_TO_EDGE);
         gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T,
                            GL10.GL_CLAMP_TO_EDGE);
         gl.glTexEnvf(GL10.GL_TEXTURE_ENV, GL10.GL_TEXTURE_ENV_MODE,
                      GL10.GL_REPLACE);

         is = null;
         try
         {
            switch (i)
            {
            case 0:
               is = m_context.getAssets().open("textures/wall.png");
               break;

            case 1:
               is = m_context.getAssets().open("textures/door.png");
               break;

            case 2:
               is = m_context.getAssets().open("textures/floor.png");
               break;
            }
         }
         catch (IOException e)
         {
            Log.w("Blackguard", "Cannot open texture file");
         }
         if (is != null)
         {
            Bitmap bitmap = null;
            try {
               bitmap = BitmapFactory.decodeStream(is);
            }
            finally {
               try {
                  is.close();
               }
               catch (IOException e) {}
            }
            if (bitmap != null)
            {
               GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, bitmap, 0);
               bitmap.recycle();
            }
            else
            {
               Log.w("Blackguard", "Cannot create texture bitmap");
            }
         }
      }
   }


   // Place object in scene.
   public void placeObject(int x, int z, char actual, char visible)
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
         m_playerX = x;
         m_playerZ = z;
      }
   }


   // Place player in scene.
   public void placePlayer(int x, int z, int dir)
   {
      m_playerX     = x;
      m_playerZ     = z;
      m_position[0] = ((float)m_playerX * UNIT_SIZE) + (UNIT_SIZE * 0.5f);
      m_position[1] = UNIT_SIZE * 0.5f;
      m_position[2] = ((float)m_playerZ * UNIT_SIZE) + (UNIT_SIZE * 0.5f);
      switch (dir)
      {
      case 4:
         m_playerDir  = DIRECTION.SOUTH;
         m_rotation   = 0.0f;
         m_forward[0] = 0.0f;
         m_forward[2] = 1.0f;
         break;

      case 3:
         m_playerDir  = DIRECTION.SOUTHEAST;
         m_rotation   = 45.0f;
         m_forward[0] = -1.0f;
         m_forward[2] = 1.0f;
         break;

      case 2:
         m_playerDir  = DIRECTION.EAST;
         m_rotation   = 90.0f;
         m_forward[0] = -1.0f;
         m_forward[2] = 0.0f;
         break;

      case  1:
         m_playerDir  = DIRECTION.NORTHEAST;
         m_rotation   = 135.0f;
         m_forward[0] = -1.0f;
         m_forward[2] = -1.0f;
         break;

      case  0:
         m_playerDir  = DIRECTION.NORTH;
         m_rotation   = 180.0f;
         m_forward[0] = 0.0f;
         m_forward[2] = -1.0f;
         break;

      case 7:
         m_playerDir  = DIRECTION.NORTHWEST;
         m_rotation   = 225.0f;
         m_forward[0] = 1.0f;
         m_forward[2] = -1.0f;
         break;

      case 6:
         m_playerDir  = DIRECTION.WEST;
         m_rotation   = 270.0f;
         m_forward[0] = 1.0f;
         m_forward[2] = 0.0f;
         break;

      case 5:
         m_playerDir  = DIRECTION.SOUTHWEST;
         m_rotation   = 315.0f;
         m_forward[0] = 1.0f;
         m_forward[2] = 1.0f;
         break;
      }

      // Set player locale.
      setPlayerLocale(m_playerX, m_playerZ, m_playerDir);
   }


   // Set player locale and visible bounds.
   void setPlayerLocale(int x, int z, DIRECTION dir)
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
         m_locale = LOCALE.IN_WALL;
         return;
      }
      m_locale = LOCALE.IN_PASSAGE;
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
                     m_locale = LOCALE.IN_ROOM;
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
                        m_locale = LOCALE.IN_ROOM;
                     }
                     else
                     {
                        m_locale = LOCALE.IN_PASSAGE;
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
   void setPassageVisibility(int x, int z, DIRECTION dir)
   {
      int       xmin2, xmax2, zmin2, zmax2;
      DIRECTION dir2, dir3;

      dir2 = dir3 = DIRECTION.NORTH;
      switch (dir)
      {
      case NORTH:
         dir2 = dir3 = DIRECTION.NORTH;
         break;

      case NORTHEAST:
         dir2 = DIRECTION.NORTH;
         dir3 = DIRECTION.EAST;
         break;

      case EAST:
         dir2 = dir3 = DIRECTION.EAST;
         break;

      case SOUTHEAST:
         dir2 = DIRECTION.SOUTH;
         dir3 = DIRECTION.EAST;
         break;

      case SOUTH:
         dir2 = dir3 = DIRECTION.SOUTH;
         break;

      case SOUTHWEST:
         dir2 = DIRECTION.SOUTH;
         dir3 = DIRECTION.WEST;
         break;

      case WEST:
         dir2 = dir3 = DIRECTION.WEST;
         break;

      case NORTHWEST:
         dir2 = DIRECTION.NORTH;
         dir3 = DIRECTION.WEST;
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
   static final float FRUSTUM_NEAR             = 1.0f;
   static final float FRUSTUM_FAR              = UNIT_SIZE * 50.0f;
   static final long  VIEW_TRANSITION_DURATION = 200;
   float[] m_currentPosition, m_lastPosition;
   float[] m_currentForward, m_lastForward;
   float m_viewTransitionState;
   long  m_viewTransitionTime;

   // Initialize camera.
   void initCamera(GL10 gl, float aspect)
   {
      m_currentPosition = new float[3];
      m_lastPosition    = new float[3];
      m_currentForward  = new float[3];
      m_lastForward     = new float[3];
      for (int i = 0; i < 3; i++)
      {
         m_currentPosition[i] = m_lastPosition[i] = 0.0f;
         m_currentForward[i]  = m_lastForward[i] = 0.0f;
      }
      m_currentPosition[1]  = m_lastPosition[1] = UNIT_SIZE * 0.5f;
      m_currentForward[2]   = m_lastForward[2] = -1.0f;
      m_viewTransitionState = 0.0f;
      m_viewTransitionTime  = SystemClock.uptimeMillis();
      configureCamera(gl, aspect);
   }


   // Configure camera.
   public void configureCamera(GL10 gl, float aspect)
   {
      gl.glMatrixMode(GL10.GL_PROJECTION);
      gl.glLoadIdentity();
      gl.glFrustumf(-aspect, aspect, -1, 1, FRUSTUM_NEAR, FRUSTUM_FAR);
      gl.glMatrixMode(GL10.GL_MODELVIEW);
   }


   // Smooth transition camera to player viewpoint.
   void placeCamera(GL10 gl)
   {
      // View is changing?
      if ((m_position[0] != m_currentPosition[0]) ||
          (m_position[1] != m_currentPosition[1]) ||
          (m_position[2] != m_currentPosition[2]) ||
          (m_forward[0] != m_currentForward[0]) ||
          (m_forward[1] != m_currentForward[1]) ||
          (m_forward[2] != m_currentForward[2]))
      {
         for (int i = 0; i < 3; i++)
         {
            m_lastPosition[i]    = m_currentPosition[i];
            m_currentPosition[i] = m_position[i];
            m_lastForward[i]     = m_currentForward[i];
            m_currentForward[i]  = m_forward[i];
         }
         m_viewTransitionState = 1.0f;
         m_viewTransitionTime  = SystemClock.uptimeMillis();
         if (Math.abs(m_currentPosition[0] - m_lastPosition[0]) > (UNIT_SIZE * 1.5f))
         {
            if (m_currentPosition[0] < m_lastPosition[0])
            {
               m_lastPosition[0] = m_currentPosition[0] + UNIT_SIZE;
            }
            else
            {
               m_lastPosition[0] = m_currentPosition[0] - UNIT_SIZE;
            }
         }
         if (Math.abs(m_currentPosition[2] - m_lastPosition[2]) > (UNIT_SIZE * 1.5f))
         {
            if (m_currentPosition[2] < m_lastPosition[2])
            {
               m_lastPosition[2] = m_currentPosition[2] + UNIT_SIZE;
            }
            else
            {
               m_lastPosition[2] = m_currentPosition[2] - UNIT_SIZE;
            }
         }
      }

      // Set camera to intermediate viewpoint.
      m_viewTransitionState =
         (float)(SystemClock.uptimeMillis() - m_viewTransitionTime) / (float)VIEW_TRANSITION_DURATION;
      if (m_viewTransitionState > 1.0f)
      {
         m_viewTransitionState = 1.0f;
      }
      m_viewTransitionState = 1.0f - m_viewTransitionState;
      float[] position      = new float[3];
      float[] forward       = new float[3];
      for (int i = 0; i < 3; i++)
      {
         position[i] = (m_currentPosition[i] * (1.0f - m_viewTransitionState)) +
                       (m_lastPosition[i] * m_viewTransitionState);
         forward[i] = (m_currentForward[i] * (1.0f - m_viewTransitionState)) +
                      (m_lastForward[i] * m_viewTransitionState);
      }
      gl.glMatrixMode(GL10.GL_MODELVIEW);
      gl.glLoadIdentity();
      GLU.gluLookAt(gl, position[0], position[1], position[2],
                    position[0] + forward[0], position[1] + forward[1],
                    position[2] + forward[2],
                    0.0f, 1.0f, 0.0f);
   }


   // Camera has pending updates?
   boolean cameraPending()
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


   // Object transform.
   class ObjectTransform
   {
      float[] player;
      float[] object;
      float[] rotation;
      float dx;
      float dz;

      ObjectTransform(float[] player, float[] object)
      {
         this.player = new float[3];
         this.object = new float[3];
         rotation    = new float[4];
         for (int i = 0; i < 3; i++)
         {
            this.player[i] = player[i];
            this.object[i] = object[i];
         }
      }
   };

   // Draw scene.
   public void draw(GL10 gl, int windowWidth, int windowHeight)
   {
      int   i, j;
      float x, z;

      float[]        player = new float[3];
      float[] object        = new float[3];

      // Make textures?
      if (!texturesValid)
      {
         makeTextures(gl);
         texturesValid = true;
      }

      // Inside of wall?
      if (m_locale == LOCALE.IN_WALL)
      {
         return;
      }

      // Enable vertex and texture array drawing.
      gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
      gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);

      gl.glPushMatrix();
      gl.glLoadIdentity();

      // Place camera at player view point.
      placeCamera(gl);

      // Initialize for picking.
      int[] viewport     = new int[4];
      float[] modelview  = new float[16];
      float[] projection = new float[16];
      gl.glGetIntegerv(GL11.GL_VIEWPORT, viewport, 0);
      ((GL11)gl).glGetFloatv(GL11.GL_MODELVIEW_MATRIX, modelview, 0);
      ((GL11)gl).glGetFloatv(GL11.GL_PROJECTION_MATRIX, projection, 0);
      float[] pickingObject;

      // Get player position.
      player[0] = m_position[0];
      player[1] = 0.0f;
      player[2] = m_position[2];

      // Draw textured topology.
      gl.glEnable(GL10.GL_TEXTURE_2D);
      gl.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
      for (i = m_xmin; i <= m_xmax; i++)
      {
         for (j = m_zmin; j <= m_zmax; j++)
         {
            x = (float)i * UNIT_SIZE;
            z = (float)j * UNIT_SIZE;
            if (m_wall[i][j])
            {
               if ((m_locale == LOCALE.IN_ROOM) && (m_visible[i][j] != ' '))
               {
                  // Visible room wall.
                  gl.glPushMatrix();
                  gl.glTranslatef(x, 0.0f, z);
                  gl.glBindTexture(GL10.GL_TEXTURE_2D,
                                   m_textures[TEXTURE_TYPE.WALL_TEXTURE.getValue()]);
                  m_wallBlock.draw(gl);
                  gl.glPopMatrix();
               }
            }
            else if (m_actual[i][j] == '+')
            {
               if (m_visible[i][j] != ' ')
               {
                  if ((m_playerX != i) || (m_playerZ != j))
                  {
                     // Visible door.
                     gl.glPushMatrix();
                     gl.glTranslatef(x, 0.0f, z);
                     gl.glBindTexture(GL10.GL_TEXTURE_2D,
                                      m_textures[TEXTURE_TYPE.DOOR_TEXTURE.getValue()]);
                     m_wallBlock.draw(gl);
                     gl.glPopMatrix();
                  }
               }
            }
            else if (m_actual[i][j] == '&')
            {
               if (m_visible[i][j] != ' ')
               {
                  // Secret door looks like wall.
                  gl.glPushMatrix();
                  gl.glTranslatef(x, 0.0f, z);
                  gl.glBindTexture(GL10.GL_TEXTURE_2D,
                                   m_textures[TEXTURE_TYPE.WALL_TEXTURE.getValue()]);
                  m_wallBlock.draw(gl);
                  gl.glPopMatrix();
               }
            }
            else if (m_visible[i][j] != ' ')
            {
               // Draw floor.
               gl.glPushMatrix();
               gl.glTranslatef(x, 0.0f, z);
               gl.glBindTexture(GL10.GL_TEXTURE_2D,
                                m_textures[TEXTURE_TYPE.FLOOR_TEXTURE.getValue()]);
               m_floor.draw(gl);
               gl.glPopMatrix();
            }
         }
      }

      // Do monster animations.
      Animator a;
      boolean  dispose = disposeAnimations();
      int[] na;
      boolean killed;
      while ((na = getAnimator()) != null)
      {
         if (na[3] == 1)
         {
            killed = true;
         }
         else
         {
            killed = false;
         }
         a = new Animator((char)na[0], na[1], na[2], killed);
         m_animators.add(a);
      }
      Vector<Animator> h  = new Vector<Animator>();
      long             tm = SystemClock.uptimeMillis();
      float            rot;
      while (m_animators.size() > 0)
      {
         a = m_animators.get(0);
         m_animators.remove(0);
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
               a.prog1 = (float)(tm - a.startTime) / (float)DEATH_ANIMATION_DURATION;
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
               a.prog1 = (float)(tm - a.startTime) / (float)HIT_ANIMATION_DURATION;
               if (a.prog1 > 1.0f)
               {
                  a.prog1 = 1.0f;
               }
               rot = -(a.prog1 * HIT_RECOIL_ANGLE);
            }
            else if (a.prog2 < 1.0f)
            {
               a.prog2 = (float)(tm - a.startTime) / (float)HIT_ANIMATION_DURATION;
               if (a.prog2 > 1.0f)
               {
                  a.prog2 = 1.0f;
               }
               rot = -(HIT_RECOIL_ANGLE - (a.prog2 * HIT_RECOIL_ANGLE));
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
         h.add(a);
         m_visible[a.x][a.z] = ' ';
         gl.glPushMatrix();
         x         = (float)(a.x) * UNIT_SIZE;
         z         = (float)(a.z) * UNIT_SIZE;
         object[0] = x + (UNIT_SIZE * 0.5f);
         object[1] = 0.0f;
         object[2] = z + (UNIT_SIZE * 0.5f);
         ObjectTransform t = new ObjectTransform(player, object);
         getObjectTransform(t);
         gl.glTranslatef(x + (UNIT_SIZE * 0.5f) + t.dx, UNIT_SIZE * 0.1f, z + (UNIT_SIZE * 0.5f) + t.dz);
         gl.glRotatef(t.rotation[3], t.rotation[0], t.rotation[1], t.rotation[2]);
         gl.glRotatef(rot, 1.0f, 0.0f, 0.0f);
         m_billboard.drawChar(gl, a.show);
         gl.glPopMatrix();
         pickingObject    = new float[4];
         pickingObject[0] = (float)a.show;
         GLU.gluProject(object[0], UNIT_SIZE * 0.6f, object[2], modelview, 0, projection, 0, viewport, 0, pickingObject, 1);
         pickingObjects.add(pickingObject);
      }
      m_animators = h;

      // Draw dark topology and objects.
      for (i = m_xmin; i <= m_xmax; i++)
      {
         for (j = m_zmin; j <= m_zmax; j++)
         {
            x = (float)i * UNIT_SIZE;
            z = (float)j * UNIT_SIZE;
            if (m_wall[i][j])
            {
               if ((m_locale == LOCALE.IN_PASSAGE) || (m_visible[i][j] == ' '))
               {
                  // Dark wall.
                  gl.glPushMatrix();
                  gl.glTranslatef(x, 0.0f, z);
                  gl.glDisable(GL10.GL_TEXTURE_2D);
                  gl.glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
                  m_wallBlock.draw(gl);
                  gl.glPopMatrix();
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
                     gl.glPushMatrix();
                     object[0] = x + (UNIT_SIZE * 0.5f);
                     object[1] = 0.0f;
                     object[2] = z + (UNIT_SIZE * 0.5f);
                     ObjectTransform t = new ObjectTransform(player, object);
                     getObjectTransform(t);
                     gl.glTranslatef(x + (UNIT_SIZE * 0.5f) + t.dx, UNIT_SIZE * 0.1f, z + (UNIT_SIZE * 0.5f) + t.dz);
                     gl.glRotatef(t.rotation[3], t.rotation[0], t.rotation[1], t.rotation[2]);
                     gl.glEnable(GL10.GL_TEXTURE_2D);
                     m_billboard.drawChar(gl, m_visible[i][j]);
                     gl.glPopMatrix();
                     pickingObject    = new float[4];
                     pickingObject[0] = (float)m_visible[i][j];
                     GLU.gluProject(object[0], UNIT_SIZE * 0.6f, object[2], modelview, 0, projection, 0, viewport, 0, pickingObject, 1);
                     pickingObjects.add(pickingObject);
                  }
               }
            }
         }
      }
      gl.glPopMatrix();
      gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
      gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
   }


   // Get transform for object to face player.
   void getObjectTransform(ObjectTransform transform)
   {
      float[]  target, source, axis;
      float angle;

      transform.rotation[0] = 0.0f;
      transform.rotation[1] = -1.0f;
      transform.rotation[2] = 0.0f;
      transform.rotation[3] = 0.0f;
      transform.dx          = transform.dz = 0.0f;

      if (Math.abs(transform.player[0] - transform.object[0]) < (UNIT_SIZE * 0.5f))
      {
         if (transform.player[2] < transform.object[2])
         {
            transform.rotation[3] = 180.0f;
            transform.dx          = UNIT_SIZE * 0.5f;
            transform.dz          = 0.0f;
         }
         else
         {
            transform.rotation[3] = 0.0f;
            transform.dx          = -UNIT_SIZE * 0.5f;
            transform.dz          = 0.0f;
         }
         return;
      }
      else if (Math.abs(transform.player[2] - transform.object[2]) < (UNIT_SIZE * 0.5f))
      {
         if (transform.player[0] < transform.object[0])
         {
            transform.rotation[3] = 90.0f;
            transform.dx          = 0.0f;
            transform.dz          = -UNIT_SIZE * 0.5f;
         }
         else
         {
            transform.rotation[3] = 270.0f;
            transform.dx          = 0.0f;
            transform.dz          = UNIT_SIZE * 0.5f;
         }
         return;
      }
      target    = new float[3];
      target[0] = transform.player[0] - transform.object[0];
      target[1] = 0.0f;
      target[2] = transform.player[2] - transform.object[2];
      normalize(target);
      source    = new float[3];
      source[0] = source[1] = 0.0f;
      source[2] = -1.0f;
      axis      = new float[3];
      vcross(target, source, axis);
      transform.dx = vdot(target, source);
      angle        = (float)Math.acos(transform.dx);
      angle        = angle * 180.0f / (float)Math.PI;
      if (axis[1] > 0.0f)
      {
         angle += 180.0f;
      }
      else
      {
         angle = 180.0f - angle;
      }
      transform.rotation[3] = angle;
      transform.dx          = Math.abs(transform.dx);
      transform.dz          = 1.0f - transform.dx;
      if ((angle > 0.0f) && (angle < 180.0f))
      {
         transform.dz = -transform.dz;
      }
      if ((angle < 90.0f) || (angle > 270.0f))
      {
         transform.dx = -transform.dx;
      }
      transform.dx *= (UNIT_SIZE * 0.5f);
      transform.dz *= (UNIT_SIZE * 0.5f);
   }


   // Normalize vector.
   void normalize(float[] v)
   {
      float       m   = FloatMath.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
      final float tol = 0.0000000001f;

      if (m <= tol)
      {
         m = 1;
      }
      v[0] /= m;
      v[1] /= m;
      v[2] /= m;

      if (Math.abs(v[0]) < tol)
      {
         v[0] = 0.0f;
      }
      if (Math.abs(v[1]) < tol)
      {
         v[1] = 0.0f;
      }
      if (Math.abs(v[2]) < tol)
      {
         v[2] = 0.0f;
      }
   }


   // Vector cross product (u cross v)
   void vcross(float[] u, float[] v, float[] out)
   {
      out[0] = (u[1] * v[2]) - (u[2] * v[1]);
      out[1] = (-u[0] * v[2]) + (u[2] * v[0]);
      out[2] = (u[0] * v[1]) - (u[1] * v[0]);
   }


   // Vector dot product
   float vdot(float[] u, float[] v)
   {
      return(u[0] * v[0] + u[1] * v[1] + u[2] * v[2]);
   }


   // Wall.
   class Wall
   {
      public Wall()
      {
         ByteBuffer vbb = ByteBuffer.allocateDirect(VERTS * 3 * 4);

         vbb.order(ByteOrder.nativeOrder());
         mFVertexBuffer = vbb.asFloatBuffer();

         ByteBuffer tbb = ByteBuffer.allocateDirect(VERTS * 2 * 4);
         tbb.order(ByteOrder.nativeOrder());
         mTexBuffer = tbb.asFloatBuffer();

         float[] vertCoords =
         {
            0.0f,           0.0f,      0.0f,      0.0f, UNIT_SIZE,      0.0f, UNIT_SIZE, UNIT_SIZE,      0.0f,
            UNIT_SIZE, UNIT_SIZE,      0.0f, UNIT_SIZE,      0.0f,      0.0f,      0.0f,      0.0f,      0.0f,
            UNIT_SIZE,      0.0f,      0.0f, UNIT_SIZE, UNIT_SIZE,      0.0f, UNIT_SIZE, UNIT_SIZE, UNIT_SIZE,
            UNIT_SIZE, UNIT_SIZE, UNIT_SIZE, UNIT_SIZE,      0.0f, UNIT_SIZE, UNIT_SIZE,      0.0f,      0.0f,
            UNIT_SIZE,      0.0f, UNIT_SIZE, UNIT_SIZE, UNIT_SIZE, UNIT_SIZE,      0.0f, UNIT_SIZE, UNIT_SIZE,
            0.0f,      UNIT_SIZE, UNIT_SIZE,      0.0f,      0.0f, UNIT_SIZE, UNIT_SIZE,      0.0f, UNIT_SIZE,
            0.0f,           0.0f, UNIT_SIZE,      0.0f, UNIT_SIZE, UNIT_SIZE,      0.0f, UNIT_SIZE,      0.0f,
            0.0f,      UNIT_SIZE,      0.0f,      0.0f,      0.0f,      0.0f,      0.0f,      0.0f, UNIT_SIZE
         };

         float[] texCoords =
         {
            1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f
         };

         for (int i = 0; i < VERTS; i++)
         {
            for (int j = 0; j < 3; j++)
            {
               mFVertexBuffer.put(vertCoords[i * 3 + j]);
            }
         }

         for (int i = 0; i < VERTS; i++)
         {
            for (int j = 0; j < 2; j++)
            {
               mTexBuffer.put(texCoords[i * 2 + j]);
            }
         }

         mFVertexBuffer.position(0);
         mTexBuffer.position(0);
      }


      public void draw(GL10 gl)
      {
         gl.glFrontFace(GL10.GL_CW);
         gl.glCullFace(GL10.GL_FRONT);
         gl.glVertexPointer(3, GL10.GL_FLOAT, 0, mFVertexBuffer);
         gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mTexBuffer);
         gl.glDrawArrays(GL10.GL_TRIANGLES, 0, VERTS);
      }


      private final static int VERTS = 24;
      private FloatBuffer      mFVertexBuffer;
      private FloatBuffer      mTexBuffer;
   }

   // Floor.
   class Floor
   {
      public Floor()
      {
         ByteBuffer vbb = ByteBuffer.allocateDirect(VERTS * 3 * 4);

         vbb.order(ByteOrder.nativeOrder());
         mFVertexBuffer = vbb.asFloatBuffer();

         ByteBuffer tbb = ByteBuffer.allocateDirect(VERTS * 2 * 4);
         tbb.order(ByteOrder.nativeOrder());
         mTexBuffer = tbb.asFloatBuffer();

         float[] vertCoords =
         {
            0.0f,      0.0f,      0.0f,      0.0f, 0.0f, UNIT_SIZE, UNIT_SIZE, 0.0f, UNIT_SIZE,
            UNIT_SIZE, 0.0f, UNIT_SIZE, UNIT_SIZE, 0.0f,      0.0f,      0.0f, 0.0f, 0.0f
         };

         float[] texCoords =
         {
            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f
         };

         for (int i = 0; i < VERTS; i++)
         {
            for (int j = 0; j < 3; j++)
            {
               mFVertexBuffer.put(vertCoords[i * 3 + j]);
            }
         }

         for (int i = 0; i < VERTS; i++)
         {
            for (int j = 0; j < 2; j++)
            {
               mTexBuffer.put(texCoords[i * 2 + j]);
            }
         }

         mFVertexBuffer.position(0);
         mTexBuffer.position(0);
      }


      public void draw(GL10 gl)
      {
         gl.glFrontFace(GL10.GL_CW);
         gl.glCullFace(GL10.GL_FRONT);
         gl.glVertexPointer(3, GL10.GL_FLOAT, 0, mFVertexBuffer);
         gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mTexBuffer);
         gl.glDrawArrays(GL10.GL_TRIANGLES, 0, VERTS);
      }


      private final static int VERTS = 6;
      private FloatBuffer      mFVertexBuffer;
      private FloatBuffer      mTexBuffer;
   }

   // Billboard for object textures.
   class Billboard
   {
      public Billboard()
      {
         ByteBuffer vbb = ByteBuffer.allocateDirect(VERTS * 3 * 4);

         vbb.order(ByteOrder.nativeOrder());
         mFVertexBuffer = vbb.asFloatBuffer();

         ByteBuffer tbb = ByteBuffer.allocateDirect(VERTS * 2 * 4);
         tbb.order(ByteOrder.nativeOrder());
         mTexBuffer = tbb.asFloatBuffer();

         tbb = ByteBuffer.allocateDirect(VERTS * 2 * 4);
         tbb.order(ByteOrder.nativeOrder());
         mCharTexBuffer = tbb.asFloatBuffer();

         float[] vertCoords =
         {
            0.0f,           0.0f, 0.0f,      0.0f, UNIT_SIZE, 0.0f, UNIT_SIZE, UNIT_SIZE, 0.0f,
            UNIT_SIZE, UNIT_SIZE, 0.0f, UNIT_SIZE,      0.0f, 0.0f,      0.0f,      0.0f, 0.0f
         };

         float[] texCoords =
         {
            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f
         };

         for (int i = 0; i < VERTS; i++)
         {
            for (int j = 0; j < 3; j++)
            {
               mFVertexBuffer.put(vertCoords[i * 3 + j]);
            }
         }

         for (int i = 0; i < VERTS; i++)
         {
            for (int j = 0; j < 2; j++)
            {
               mTexBuffer.put(texCoords[i * 2 + j]);
            }
         }

         for (int i = 0; i < VERTS; i++)
         {
            for (int j = 0; j < 2; j++)
            {
               mCharTexBuffer.put(texCoords[i * 2 + j]);
            }
         }

         mFVertexBuffer.position(0);
         mTexBuffer.position(0);
         mCharTexBuffer.position(0);
      }


      public void draw(GL10 gl)
      {
         gl.glFrontFace(GL10.GL_CW);
         gl.glCullFace(GL10.GL_BACK);
         gl.glVertexPointer(3, GL10.GL_FLOAT, 0, mFVertexBuffer);
         gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mTexBuffer);
         gl.glDrawArrays(GL10.GL_TRIANGLES, 0, VERTS);
      }


      public void drawChar(GL10 gl, char c)
      {
         m_labelMaker.bind(gl,
                           m_charLabels[(int)c - CHAR_LABEL_PARMS.CHAR_OFFSET.getValue()],
                           mCharTexBuffer);
         gl.glFrontFace(GL10.GL_CW);
         gl.glCullFace(GL10.GL_BACK);
         gl.glVertexPointer(3, GL10.GL_FLOAT, 0, mFVertexBuffer);
         gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mCharTexBuffer);
         gl.glDrawArrays(GL10.GL_TRIANGLES, 0, VERTS);
         m_labelMaker.endBind(gl);
      }


      private final static int VERTS = 6;
      private FloatBuffer      mFVertexBuffer;
      private FloatBuffer      mTexBuffer;
      private FloatBuffer      mCharTexBuffer;
   }
}
