// Blackguard scene.

package blackguard;


import java.awt.Font;
import java.awt.Color;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.InputStream;
import java.io.IOException;
import java.util.Vector;
import javax.media.opengl.GL2;
import javax.media.opengl.glu.GLU;
import javax.media.opengl.awt.GLCanvas;
import javax.media.opengl.GLCapabilities;
import com.sun.opengl.util.texture.Texture;
import com.sun.opengl.util.texture.TextureData;
import com.sun.opengl.util.texture.TextureIO;
import com.sun.opengl.util.awt.TextRenderer;

public class Scene extends GLCanvas
{
   // Width and height.
   int WIDTH;
   int HEIGHT;

   // Ready?
   public boolean ready;

   // Unit size.
   public static final float UNIT_SIZE = 0.05f;

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

   // Object colors and texture files.
   static final String OBJECT_COLORS_FILE = "object_colors.txt";
   static final String WALL_TEXTURE_FILE  = "wall.png";
   static final String DOOR_TEXTURE_FILE  = "door.png";
   static final String FLOOR_TEXTURE_FILE = "floor.png";
   enum COLOR_PARMS
   {
      COLOR_OFFSET(33), NUM_COLORS(94);
      private int value;
      COLOR_PARMS(int value) { this.value = value; }
      int getValue() { return(value); }
   };
   char[][] m_objectColors;
   enum TEXTURE_TYPE
   {
      WALL_TEXTURE(0),
      DOOR_TEXTURE(1),
      FLOOR_TEXTURE(2);
      private int value;
      TEXTURE_TYPE(int value) { this.value = value; }
      int getValue() { return(value); }
   };
   Texture[] m_textures;

   // OpenGL drawing displays.
   boolean m_displaysCreated;
   int     m_wallDisplay;
   int     m_floorTileDisplay;
   int     m_ceilingTileDisplay;

   // OpenGL utilities.
   GLU m_glu;

   // Text.
   TextRenderer m_textRenderer;
   float        m_objectCharScale;

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
         startTime   = System.currentTimeMillis();
      }
   };
   Vector<Animator>   m_animators;
   static final long  DEATH_ANIMATION_DURATION = 250;
   static final long  HIT_ANIMATION_DURATION   = 100;
   static final float HIT_RECOIL_ANGLE         = 45.0f;
   private native boolean disposeAnimations();

   private native int[] getAnimator();

   // Constructor.
   public Scene(GLCapabilities capabilities)
   {
      super(capabilities);
      m_glu       = new GLU();
      ready       = false;
      m_animators = new Vector<Animator>();
   }


   // Initialize.
   public void init(int width, int height, float aspect)
   {
      int    i, j, r, g, b;
      char   c;
      String path;

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

      // Defer display creation.
      m_displaysCreated = false;

      // Create text renderer.
      m_textRenderer    = new TextRenderer(new Font("Lucida Console", Font.PLAIN, 128), true, true);
      m_objectCharScale = 0.0005f;

      // Initialize camera.
      initCamera(aspect);

      // Initialize object colors.
      m_objectColors = new char[COLOR_PARMS.NUM_COLORS.getValue()][3];
      for (i = 0; i < COLOR_PARMS.NUM_COLORS.getValue(); i++)
      {
         j = i + COLOR_PARMS.COLOR_OFFSET.getValue();
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
      if ((path = Blackguard.getResourcePath(OBJECT_COLORS_FILE)) != null)
      {
         try
         {
            BufferedReader in = new BufferedReader(new FileReader(path));
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
                  i = (int)c - COLOR_PARMS.COLOR_OFFSET.getValue();
                  if ((i >= 0) && (i < COLOR_PARMS.NUM_COLORS.getValue()))
                  {
                     m_objectColors[i][0] = (char)r;
                     m_objectColors[i][1] = (char)g;
                     m_objectColors[i][2] = (char)b;
                  }
               }
            }
            in.close();
         }
         catch (IOException e)
         {
            System.err.println("Error reading " + OBJECT_COLORS_FILE);
            System.exit(1);
         }
      }

      // Load textures.
      m_textures = new Texture[3];
      if (((path = Blackguard.getResourcePath("textures/" + WALL_TEXTURE_FILE)) == null) &&
          ((path = Blackguard.getResourcePath(WALL_TEXTURE_FILE)) == null))
      {
         System.err.println("Cannot access " + WALL_TEXTURE_FILE);
         System.exit(1);
      }
      try {
         InputStream stream = new FileInputStream(path);
         TextureData data   = TextureIO.newTextureData(stream, false, "bmp");
         m_textures[TEXTURE_TYPE.WALL_TEXTURE.getValue()] = TextureIO.newTexture(data);
      }
      catch (IOException e) {
         System.err.println("Cannot create texture from " + path);
         System.exit(1);
      }
      if (((path = Blackguard.getResourcePath("textures/" + DOOR_TEXTURE_FILE)) == null) &&
          ((path = Blackguard.getResourcePath(DOOR_TEXTURE_FILE)) == null))
      {
         System.err.println("Cannot access " + DOOR_TEXTURE_FILE);
         System.exit(1);
      }
      try {
         InputStream stream = new FileInputStream(path);
         TextureData data   = TextureIO.newTextureData(stream, false, "bmp");
         m_textures[TEXTURE_TYPE.DOOR_TEXTURE.getValue()] = TextureIO.newTexture(data);
      }
      catch (IOException e) {
         System.err.println("Cannot create texture from " + path);
         System.exit(1);
      }
      if (((path = Blackguard.getResourcePath("textures/" + FLOOR_TEXTURE_FILE)) == null) &&
          ((path = Blackguard.getResourcePath(FLOOR_TEXTURE_FILE)) == null))
      {
         System.err.println("Cannot access " + FLOOR_TEXTURE_FILE);
         System.exit(1);
      }
      try {
         InputStream stream = new FileInputStream(path);
         TextureData data   = TextureIO.newTextureData(stream, false, "bmp");
         m_textures[TEXTURE_TYPE.FLOOR_TEXTURE.getValue()] = TextureIO.newTexture(data);
      }
      catch (IOException e) {
         System.err.println("Cannot create texture from " + path);
         System.exit(1);
      }

      // Ready.
      ready = true;
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
   static final float FRUSTUM_ANGLE            = 60.0f;
   static final float FRUSTUM_NEAR             = 0.01f;
   static final float FRUSTUM_FAR              = 100.0f;
   static final long  VIEW_TRANSITION_DURATION = 200;
   float[] m_currentPosition, m_lastPosition;
   float[] m_currentForward, m_lastForward;
   float m_viewTransitionState;
   long  m_viewTransitionTime;

   // Initialize camera.
   void initCamera(float aspect)
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
      m_viewTransitionState = 0.0f;
      m_viewTransitionTime  = System.currentTimeMillis();
      configureCamera(aspect);
   }


   // Configure camera.
   public void configureCamera(float aspect)
   {
      GL2 gl = (GL2)getGL();

      gl.glMatrixMode(GL2.GL_PROJECTION);
      gl.glLoadIdentity();
      m_glu.gluPerspective(FRUSTUM_ANGLE, aspect, FRUSTUM_NEAR, FRUSTUM_FAR);
      gl.glMatrixMode(GL2.GL_MODELVIEW);
   }


   // Smooth transition camera to player viewpoint.
   void placeCamera()
   {
      GL2 gl = (GL2)getGL();

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
         m_viewTransitionTime  = System.currentTimeMillis();
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
         (float)(System.currentTimeMillis() - m_viewTransitionTime) / (float)VIEW_TRANSITION_DURATION;
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
      gl.glMatrixMode(GL2.GL_MODELVIEW);
      gl.glLoadIdentity();
      m_glu.gluLookAt(position[0], position[1], position[2],
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
   public void draw()
   {
      int  i, j, k;
      char r, g, b;

      char[] buf = new char[1];
      float x, z;
      float[]        player = new float[3];
      float[] object        = new float[3];

      GL2 gl = (GL2)getGL();
      gl.glMatrixMode(GL2.GL_MODELVIEW);

      // Build block and tile displays?
      if (!m_displaysCreated)
      {
         createDisplays(gl);
         m_displaysCreated = true;
      }

      // Inside of wall?
      if (m_locale == LOCALE.IN_WALL)
      {
         return;
      }

      gl.glPushMatrix();
      gl.glLoadIdentity();

      // Place camera at player view point.
      placeCamera();

      // Get player position.
      player[0] = m_position[0];
      player[1] = 0.0f;
      player[2] = m_position[2];

      // Draw textured topology.
      gl.glEnable(GL2.GL_TEXTURE_2D);
      gl.glColor3f(1.0f, 1.0f, 1.0f);
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
                  m_textures[TEXTURE_TYPE.WALL_TEXTURE.getValue()].enable();
                  m_textures[TEXTURE_TYPE.WALL_TEXTURE.getValue()].bind();
                  gl.glCallList(m_wallDisplay);
                  m_textures[TEXTURE_TYPE.WALL_TEXTURE.getValue()].disable();
                  gl.glPopMatrix();
               }
            }
            else if (m_actual[i][j] == '+')
            {
               if (m_visible[i][j] != ' ')
               {
                  // Visible door.
                  gl.glPushMatrix();
                  gl.glTranslatef(x, 0.0f, z);
                  m_textures[TEXTURE_TYPE.DOOR_TEXTURE.getValue()].enable();
                  m_textures[TEXTURE_TYPE.DOOR_TEXTURE.getValue()].bind();
                  gl.glCallList(m_wallDisplay);
                  m_textures[TEXTURE_TYPE.DOOR_TEXTURE.getValue()].disable();
                  gl.glPopMatrix();
               }
            }
            else if (m_actual[i][j] == '&')
            {
               if (m_visible[i][j] != ' ')
               {
                  // Secret door looks like wall.
                  gl.glPushMatrix();
                  gl.glTranslatef(x, 0.0f, z);
                  m_textures[TEXTURE_TYPE.WALL_TEXTURE.getValue()].enable();
                  m_textures[TEXTURE_TYPE.WALL_TEXTURE.getValue()].bind();
                  gl.glCallList(m_wallDisplay);
                  m_textures[TEXTURE_TYPE.WALL_TEXTURE.getValue()].disable();
                  gl.glPopMatrix();
               }
            }
            else if (m_visible[i][j] != ' ')
            {
               // Draw floor.
               gl.glPushMatrix();
               gl.glTranslatef(x, 0.0f, z);
               m_textures[TEXTURE_TYPE.FLOOR_TEXTURE.getValue()].enable();
               m_textures[TEXTURE_TYPE.FLOOR_TEXTURE.getValue()].bind();
               gl.glCallList(m_floorTileDisplay);
               m_textures[TEXTURE_TYPE.FLOOR_TEXTURE.getValue()].disable();
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
      long             tm = System.currentTimeMillis();
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
         m_textRenderer.begin3DRendering();
         k = (int)(a.show) - COLOR_PARMS.COLOR_OFFSET.getValue();
         r = m_objectColors[k][0];
         g = m_objectColors[k][1];
         b = m_objectColors[k][2];
         m_textRenderer.setColor(new Color(r, g, b));
         buf[0] = a.show;
         m_textRenderer.draw3D(new String(buf), 0.0f, 0.0f, 0.0f, m_objectCharScale);
         m_textRenderer.end3DRendering();
         gl.glPopMatrix();
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
                  gl.glColor3f(0.0f, 0.0f, 0.0f);
                  gl.glCallList(m_wallDisplay);
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
                     m_textRenderer.begin3DRendering();
                     k = (int)m_visible[i][j] - COLOR_PARMS.COLOR_OFFSET.getValue();
                     r = m_objectColors[k][0];
                     g = m_objectColors[k][1];
                     b = m_objectColors[k][2];
                     m_textRenderer.setColor(new Color(r, g, b));
                     buf[0] = m_visible[i][j];
                     m_textRenderer.draw3D(new String(buf), 0.0f, 0.0f, 0.0f, m_objectCharScale);
                     m_textRenderer.end3DRendering();
                     gl.glPopMatrix();
                  }
               }
            }
         }
      }
      gl.glPopMatrix();
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
            transform.dx          = UNIT_SIZE * 0.4f;
            transform.dz          = 0.0f;
         }
         else
         {
            transform.rotation[3] = 0.0f;
            transform.dx          = -UNIT_SIZE * 0.4f;
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
            transform.dz          = -UNIT_SIZE * 0.4f;
         }
         else
         {
            transform.rotation[3] = 270.0f;
            transform.dx          = 0.0f;
            transform.dz          = UNIT_SIZE * 0.4f;
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
      transform.dx *= (UNIT_SIZE * 0.4f);
      transform.dz *= (UNIT_SIZE * 0.4f);
   }


   // Normalize vector.
   void normalize(float[] v)
   {
      float       m   = (float)Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
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


   // Create displays.
   void createDisplays(GL2 gl)
   {
      float x, y, z;

      m_wallDisplay = gl.glGenLists(3);
      gl.glNewList(m_wallDisplay, GL2.GL_COMPILE);
      gl.glBegin(GL2.GL_QUADS);
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      gl.glEnd();
      gl.glBegin(GL2.GL_QUADS);
      x = UNIT_SIZE;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      gl.glEnd();
      gl.glBegin(GL2.GL_QUADS);
      x = UNIT_SIZE;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      gl.glEnd();
      gl.glBegin(GL2.GL_QUADS);
      x = 0.0f;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      gl.glEnd();
      gl.glEndList();
      m_floorTileDisplay = m_wallDisplay + 1;
      gl.glNewList(m_floorTileDisplay, GL2.GL_COMPILE);
      gl.glBegin(GL2.GL_QUADS);
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(0.0f, 0.0f, 0.0f);
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(0.0f, 0.0f, UNIT_SIZE);
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(UNIT_SIZE, 0.0f, UNIT_SIZE);
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(UNIT_SIZE, 0.0f, 0.0f);
      gl.glEnd();
      gl.glEndList();
      m_ceilingTileDisplay = m_floorTileDisplay + 1;
      gl.glNewList(m_ceilingTileDisplay, GL2.GL_COMPILE);
      gl.glBegin(GL2.GL_QUADS);
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(0.0f, 0.0f, 0.0f);
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(UNIT_SIZE, 0.0f, 0.0f);
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(UNIT_SIZE, 0.0f, UNIT_SIZE);
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(0.0f, 0.0f, UNIT_SIZE);
      gl.glEnd();
      gl.glEndList();
   }


   // Draw wall.
   void drawWall(GL2 gl)
   {
      float x, y, z;

      gl.glBegin(GL2.GL_TRIANGLES);
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);

      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      gl.glEnd();


      gl.glBegin(GL2.GL_TRIANGLES);
      x = UNIT_SIZE;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);

      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      gl.glEnd();


      gl.glBegin(GL2.GL_TRIANGLES);
      x = UNIT_SIZE;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);

      x = 0.0f;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = UNIT_SIZE;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      gl.glEnd();


      gl.glBegin(GL2.GL_TRIANGLES);
      x = 0.0f;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = UNIT_SIZE;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);

      x = 0.0f;
      y = UNIT_SIZE;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      x = 0.0f;
      y = 0.0f;
      z = UNIT_SIZE;
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(x, y, z);
      gl.glEnd();
   }


   // Draw floor.
   void drawFloor(GL2 gl)
   {
      gl.glBegin(GL2.GL_TRIANGLES);
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(0.0f, 0.0f, 0.0f);
      gl.glTexCoord2f(0.0f, 1.0f);
      gl.glVertex3f(0.0f, 0.0f, UNIT_SIZE);
      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(UNIT_SIZE, 0.0f, UNIT_SIZE);

      gl.glTexCoord2f(1.0f, 1.0f);
      gl.glVertex3f(UNIT_SIZE, 0.0f, UNIT_SIZE);
      gl.glTexCoord2f(1.0f, 0.0f);
      gl.glVertex3f(UNIT_SIZE, 0.0f, 0.0f);
      gl.glTexCoord2f(0.0f, 0.0f);
      gl.glVertex3f(0.0f, 0.0f, 0.0f);
      gl.glEnd();
   }
};
