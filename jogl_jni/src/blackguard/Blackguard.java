/*
 * Blackguard: a 3D rogue-like game.
 *
 * @(#) Blackguard.java	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

package blackguard;

import java.awt.event.KeyListener;
import java.awt.event.KeyEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.BorderLayout;
import java.awt.Font;
import java.awt.Color;
import java.io.File;
import javax.media.opengl.GL2;
import javax.media.opengl.glu.GLU;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLProfile;
import javax.media.opengl.GLCapabilities;
import javax.media.opengl.GLEventListener;
import javax.swing.JFrame;


import com.sun.opengl.util.FPSAnimator;
import com.sun.opengl.util.awt.TextRenderer;

public class Blackguard extends JFrame
implements GLEventListener, KeyListener, MouseListener
{
   // Serial version UID.
   static final long serialVersionUID = 1L;

   // Window dimensions.
   static final int WINDOW_WIDTH  = 800;
   static final int WINDOW_HEIGHT = 475;
   int              windowWidth   = WINDOW_WIDTH;
   int              windowHeight  = WINDOW_HEIGHT;
   float            windowAspect  = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

   // Scene.
   Scene scene;

   // Frame rate.
   static int FPS = 20;

   // Animator.
   FPSAnimator animator;

   // Viewing controls.
   enum View
   {
      FPVIEW, OVERVIEW
   }
   View    view;
   boolean textScene;
   boolean promptPresent;

   // Player position.
   int playerX, playerZ;

   // OpenGL utility.
   GLU glu;

   // Text.
   TextRenderer textRenderer;

   // Load native Blackguard.
   static
   {
      try
      {
         System.loadLibrary("Blackguard");
      }
      catch (UnsatisfiedLinkError e)
      {
         System.out.println("Blackguard native library not found in 'java.library.path': " +
                            System.getProperty("java.library.path"));
         throw e;
      }
   }

   // Command line args.
   String[] args;

   // Constructor.
   public Blackguard(String[] args)
   {
      super("Blackguard");
      setSize(WINDOW_WIDTH, WINDOW_HEIGHT);
      setLayout(new BorderLayout());

      this.args = args;

      // Create scene.
      GLProfile      profile      = GLProfile.get(GLProfile.GL2);
      GLCapabilities capabilities = new GLCapabilities(profile);
      capabilities.setDoubleBuffered(true);
      capabilities.setHardwareAccelerated(true);
      scene = new Scene(capabilities);
      scene.addGLEventListener(this);
      scene.addKeyListener(this);
      scene.addMouseListener(this);
      getContentPane().add(scene, BorderLayout.CENTER);
      setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
      addWindowListener(new WindowAdapter()
                        {
                           public void windowActivated(WindowEvent e)
                           {
                              scene.requestFocusInWindow();
                           }
                        }
                        );
      setVisible(true);
      scene.requestFocusInWindow();
      pack();
   }


   // Initialize.
   public void init(GLAutoDrawable drawable)
   {
      final GL2 gl = (GL2)drawable.getGL();

      // Enable z-buffer.
      gl.glEnable(GL2.GL_DEPTH_TEST);
      gl.glDepthFunc(GL2.GL_LEQUAL);

      // Enable smooth shading.
      gl.glShadeModel(GL2.GL_SMOOTH);

      // Eliminate back face drawing.
      gl.glEnable(GL2.GL_CULL_FACE);
      gl.glCullFace(GL2.GL_BACK);

      // Define clear color.
      gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

      // Perspective hint.
      gl.glHint(GL2.GL_PERSPECTIVE_CORRECTION_HINT, GL2.GL_NICEST);

      // Get utilities.
      glu = new GLU();

      // Create text renderer.
      textRenderer = new TextRenderer(new Font("Lucida Console", Font.PLAIN, 16));

      // Set controls.
      view          = View.FPVIEW;
      textScene     = false;
      promptPresent = false;

      // Ensure reshape is called.
      reshape(scene, 0, 0, WIDTH, HEIGHT);

      // Initialize native Blackguard.
      initBlackGuard(args);

      // Start animator.
      animator = new FPSAnimator(scene, FPS);
      animator.setRunAsFastAsPossible(false);
      animator.start();
   }


   // Display.
   public void display(GLAutoDrawable drawable)
   {
      if (!animator.isAnimating())
      {
         return;
      }

      // Lock display.
      lockDisplay();

      GL2 gl = (GL2)drawable.getGL();

      // Clear display.
      gl.glClear(GL2.GL_COLOR_BUFFER_BIT | GL2.GL_DEPTH_BUFFER_BIT);

      // Display text and scene.
      if (displayText(gl) && !isBlind())
      {
         displayScene(gl);
      }

      gl.glFlush();

      unlockDisplay();
   }


   // Display text.
   // Return true if scene requires drawing.
   boolean displayText(GL2 gl)
   {
      int  i, j, k, lines, cols, clen, llen;
      char c;

      char[] linebuf;
      boolean draw;
      boolean post;
      boolean maze;

      gl.glMatrixMode(GL2.GL_MODELVIEW);
      gl.glPushMatrix();
      gl.glLoadIdentity();

      textRenderer.beginRendering(windowWidth, windowHeight);
      textRenderer.setColor(Color.white);

      String msg = displaymsg();
      if (msg != null)
      {
         textRenderer.draw(msg, 0, windowHeight - 15);
      }

      draw = true;
      post = false;
      maze = false;
      if (currentWindow())
      {
         lines   = getLines();
         cols    = getCols();
         clen    = windowWidth / cols;
         llen    = windowHeight / lines;
         linebuf = new char[cols];
         k       = 0;
         for (i = 0; i < 3; i++)
         {
            switch (i)
            {
            case 0:
               k = 0;
               break;

            case 1:
               k = lines - 2;
               break;

            case 2:
               k = lines - 1;
               break;
            }
            for (j = 0; j < cols; j++)
            {
               c          = getWindowChar(k, j);
               linebuf[j] = c;
            }
            textRenderer.draw(new String(linebuf), 0,
                              windowHeight - ((k * llen) + (int)((float)k * 0.4f) + llen));
         }

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

         // Check for player character.
         if (showtext() == 0)
         {
            for (i = 1, k = lines - 2; i < k; i++)
            {
               for (j = 0; j < cols; j++)
               {
                  c = getWindowChar(i, j);
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

                  // Check for maze level.
                  c = getScreenChar(i, j);
                  if (c == '-')
                  {
                     c = getScreenChar(i - 1, j);
                     if (c == '|')
                     {
                        c = getScreenChar(i + 1, j);
                        if (c == '|')
                        {
                           maze = true;
                        }
                     }
                  }
               }
            }
            if (!draw && (post || maze))
            {
               draw = true;
               view = View.FPVIEW;
            }
         }

         // Set text scene state.
         if (draw)
         {
            textScene = true;
         }
         else
         {
            textScene = false;
         }

         // Force text scene for level overview?
         if (view == View.OVERVIEW)
         {
            if (!draw)
            {
               textRenderer.draw("Level overview:", clen, windowHeight - llen);
               draw = true;
            }
         }

         if (draw)
         {
            for (i = 1, k = lines - 2; i < k; i++)
            {
               for (j = 0; j < cols; j++)
               {
                  c          = getWindowChar(i, j);
                  linebuf[j] = c;
               }
               textRenderer.draw(new String(linebuf), 0,
                                 windowHeight - ((i * llen) + (int)((float)i * 0.4f) + llen));
            }
         }
      }

      textRenderer.endRendering();

      gl.glPopMatrix();

      return(!draw);
   }


   // Display scene.
   void displayScene(GL2 gl)
   {
      int  i, j, k, lines, cols;
      char actual, visible;

      if (currentWindow())
      {
         lines = getLines();
         cols  = getCols();

         // Initialize scene?
         if (!scene.ready)
         {
            scene.init(cols, lines, windowAspect);
         }

         gl.glMatrixMode(GL2.GL_MODELVIEW);
         gl.glLoadIdentity();

         // Set up viewport for scene.
         i = (int)(windowHeight / (float)lines);
         gl.glViewport(0, (2 * i) + 2, windowWidth, windowHeight - (3 * i) - 4);

         // Load window content into scene.
         for (i = 1, k = lines - 2; i < k; i++)
         {
            for (j = 0; j < cols; j++)
            {
               actual  = getScreenChar(i, j);
               visible = getWindowChar(i, j);
               scene.placeObject(j, i, actual, visible);
            }
         }

         // Place player.
         scene.placePlayer(playerX, playerZ, playerDir());

         // Draw scene.
         scene.draw();

         gl.glViewport(0, 0, windowWidth, windowHeight);
      }
   }


   // Resize the screen.
   public void reshape(GLAutoDrawable drawable, int x, int y, int width, int height)
   {
      GL2 gl = (GL2)drawable.getGL();

      windowWidth  = width;
      windowHeight = height;
      windowAspect = (float)width / (float)height;
      gl.glViewport(0, 0, width, height);

      // Configure scene camera.
      scene.configureCamera(windowAspect);
   }


   // Changing devices is not supported.
   public void displayChanged(GLAutoDrawable drawable, boolean modeChanged, boolean deviceChanged)
   {
      throw new UnsupportedOperationException("Changing display is not supported.");
   }


   // Key pressed event.
   public void keyPressed(KeyEvent e)
   {
      if (!textScene && (view == View.OVERVIEW))
      {
         return;
      }

      int  keyCode = e.getKeyCode();
      char keyChar = e.getKeyChar();

      switch (keyCode)
      {
      case KeyEvent.VK_SHIFT:
         return;

      case KeyEvent.VK_UP:
         keyChar = (int)'K';
         break;

      case KeyEvent.VK_DOWN:
         keyChar = (int)'J';
         break;

      case KeyEvent.VK_RIGHT:
         {
            int d = playerDir();
            d--;
            if (d < 0)
            {
               d += 8;
            }
            if (e.isShiftDown())
            {
               d--;
               if (d < 0)
               {
                  d += 8;
               }
            }
            playerDir(d);
            return;
         }

      case KeyEvent.VK_LEFT:
         {
            int d = playerDir();
            d = (d + 1) % 8;
            if (e.isShiftDown())
            {
               d = (d + 1) % 8;
            }
            playerDir(d);
            return;
         }
      }

      // Wait for input request?
      lockInput();
      if (inputReq() == 0)
      {
         waitInput();
      }

      switch (inputReq())
      {
      // Request for character.
      case 1:
         switch (keyCode)
         {
         case KeyEvent.VK_ENTER:
            inputChar('\n');
            break;

         case KeyEvent.VK_BACK_SPACE:
            inputChar('\b');
            break;

         default:
            if (!e.isShiftDown())
            {
               keyChar = Character.toLowerCase(keyChar);
            }
            inputChar(keyChar);
            break;
         }
         inputReq(0);
         signalInput();
         break;

      // Request for string.
      case 2:
         switch (keyCode)
         {
         case KeyEvent.VK_ENTER:
            inputReq(0);
            signalInput();
            break;

         case KeyEvent.VK_BACK_SPACE:
            if (inputIndex() > 0)
            {
               inputIndex(inputIndex() - 1);
               inputBuf(inputIndex(), '\0');
            }
            break;

         default:
            if (inputIndex() < inputSize() - 1)
            {
               if (!e.isShiftDown())
               {
                  keyChar = Character.toLowerCase(keyChar);
               }
               inputBuf(inputIndex(), keyChar);
               inputIndex(inputIndex() + 1);
               inputBuf(inputIndex(), '\0');
            }
            break;
         }
         break;

      default:
         break;
      }
      unlockInput();
   }


   // Key released event.
   public void keyReleased(KeyEvent e)
   {
   }


   // Key typed event.
   public void keyTyped(KeyEvent e)
   {
   }


   // Mouse callbacks.
   public void mouseClicked(MouseEvent e)
   {
      if ((e.getButton() != MouseEvent.BUTTON1) ||
          (promptPresent && (view == View.FPVIEW)))
      {
         return;
      }

      if (view == View.OVERVIEW)
      {
         view = View.FPVIEW;
      }
      else
      {
         view = View.OVERVIEW;
      }
   }


   public void mousePressed(MouseEvent e) {}

   public void mouseReleased(MouseEvent e) {}

   public void mouseEntered(MouseEvent e) {}

   public void mouseExited(MouseEvent e) {}

   // Dispose.
   public void dispose(GLAutoDrawable drawable) {}

   // Get resource path to file.
   static String getResourcePath(String filename)
   {
      String home, path;

      if (((home = System.getenv("BLACKGUARDHOME")) != null) && (home.length() > 0))
      {
         path = home + "/" + filename;
         if (new File(path).canRead())
         {
            return(path);
         }
         path = home + "/resource/" + filename;
         if (new File(path).canRead())
         {
            return(path);
         }
      }
      else
      {
         path = new String(filename);
         if (new File(path).canRead())
         {
            return(path);
         }
         path = "../resource/" + filename;
         if (new File(path).canRead())
         {
            return(path);
         }
      }
      return(null);
   }


   // Native methods:

   // Initialize Blackguard.
   private native void initBlackGuard(String[] args);

   // Lock/unlock display.
   private native void lockDisplay();
   private native void unlockDisplay();

   // Current window is available?
   private native boolean currentWindow();

   // Get lines and column quantities.
   private native int getLines();
   private native int getCols();

   // Get characters.
   private native char getWindowChar(int line, int col);
   private native char getScreenChar(int line, int col);

   // Show text?
   private native int showtext();

   // Get display message.
   private native String displaymsg();

   // Player is blind?
   private native boolean isBlind();

   // Mute.
   private native boolean mute();

   // Get/set player direction.
   private native int playerDir();
   private native int playerDir(int dir);

   // Input synchronization.
   private native void lockInput();
   private native void unlockInput();
   private native void waitInput();
   private native void signalInput();
   private native int inputReq();
   private native int inputReq(int state);

   // Input.
   private native void inputChar(char c);
   private native int inputIndex();
   private native int inputIndex(int i);
   private native void inputBuf(int i, char c);
   private native int inputSize();

   // Main.
   public final static void main(String[] args)
   {
      Blackguard blackguard = new Blackguard(args);
   }
}
