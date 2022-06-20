// Blackguard renderer.

package com.dialectek.blackguard;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.os.SystemClock;
import android.view.inputmethod.InputMethodManager;
import android.speech.RecognizerIntent;
import android.util.Log;

public class BlackguardRenderer implements GLSurfaceView.Renderer
{
   private static final String LOG_TAG = BlackguardRenderer.class .getSimpleName();

   // Window dimensions.
   int   windowWidth  = 320;
   int   windowHeight = 480;
   float windowAspect = (float)windowWidth / (float)windowHeight;

   // Character dimensions.
   public static final int COLS  = 80;
   public static final int LINES = 24;

   // Set value to rogue.h LINEZ + 2
   public static final int LINEZ = 15;

   // Viewing controls.
   public enum View
   {
      FPVIEW, OVERVIEW
   }
   public View    view;
   public boolean promptPresent;

   // Player position.
   int playerX, playerZ;

   // Context.
   Context context;

   // View.
   BlackguardView glview;

   // Scene.
   Scene scene;

   // Text drawing.
   LabelMaker smallLabelMaker;
   LabelMaker largeLabelMaker;
   LabelMaker welcomeLabelMaker;
   int[] smallCharLabels;
   int[] largeCharLabels;
   int[] welcomeLabels;
   String[] welcomeText;
   enum CHAR_LABEL_PARMS
   {
      CHAR_OFFSET(32),
      NUM_CHARS(95);
      private int value;
      CHAR_LABEL_PARMS(int value) { this.value = value; }
      int getValue() { return(value); }
   };
   public static final int MAX_TEXT_SIZE = 24;

   // Soft keyboard toggle.
   public SoftKeyboardToggle softKeyboardToggle;

   // Voice prompter.
   public VoicePrompter voicePrompter;
   public boolean       voicePrompterVisible;

   // Textures are valid?
   public boolean texturesValid;

   // Render mode timing.
   int  RENDERMODE_CONTINUOUSLY_TIME = 10000;
   long modeTimer;

   // Welcome timing.
   int     WELCOME_TIME = 5000;
   long    welcomeTimer;
   boolean welcome;

   // View manual?
   public boolean viewManual;

   // Constructor.
   public BlackguardRenderer(Context context, BlackguardView glview)
   {
      this.context = context;
      this.glview  = glview;

      // Create scene.
      scene = new Scene(context);

      // Initialize state.
      view                 = View.FPVIEW;
      promptPresent        = false;
      smallLabelMaker      = largeLabelMaker = welcomeLabelMaker = null;
      softKeyboardToggle   = null;
      voicePrompter        = null;
      voicePrompterVisible = false;
      texturesValid        = false;
      modeTimer            = welcomeTimer = SystemClock.uptimeMillis();
      welcome              = true;
      viewManual           = false;
   }


   // Surface creation.
   @Override
   public void onSurfaceCreated(GL10 gl, EGLConfig config)
   {
      gl.glDisable(GL10.GL_DITHER);
      gl.glHint(GL10.GL_PERSPECTIVE_CORRECTION_HINT,
                GL10.GL_FASTEST);
      gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      gl.glShadeModel(GL10.GL_SMOOTH);
      gl.glEnable(GL10.GL_DEPTH_TEST);
      gl.glEnable(GL10.GL_TEXTURE_2D);
      gl.glEnable(GL10.GL_CULL_FACE);
      gl.glViewport(0, 0, windowWidth, windowHeight);
   }


   // Reshape.
   @Override
   public void onSurfaceChanged(GL10 gl, int w, int h)
   {
      if ((smallLabelMaker == null) || (windowWidth != w) || (windowHeight != h))
      {
         invalidateTextures();
      }
      windowWidth  = w;
      windowHeight = h;
      windowAspect = (float)windowWidth / (float)windowHeight;
      gl.glViewport(0, 0, w, h);
      scene.configureCamera(gl, windowAspect);
   }


   // Make textures.
   public void makeTextures(GL10 gl)
   {
      int i;

      // Create small text labels.
      if (smallLabelMaker != null)
      {
         smallLabelMaker.shutdown(gl);
      }
      int textSize = Math.min((windowWidth / COLS), (windowHeight / LINES));

      if (context.getResources().getConfiguration().orientation ==
          Configuration.ORIENTATION_PORTRAIT)
      {
         textSize = (int)((float)textSize * 1.9f);
      }
      else
      {
         textSize = (int)((float)textSize * 1.5f);
      }
      if (textSize > MAX_TEXT_SIZE) { textSize = MAX_TEXT_SIZE; }
      int x = CHAR_LABEL_PARMS.NUM_CHARS.getValue() * textSize * 2;
      int y = textSize * 2;
      int w2, h2;
      for (w2 = 2; w2 < x; w2 *= 2) {}
      for (h2 = 2; h2 < y; h2 *= 2) {}
      smallLabelMaker = new LabelMaker(true, w2, h2);
      smallLabelMaker.initialize(gl);
      smallLabelMaker.beginAdding(gl);
      Paint labelPaint = new Paint();
      labelPaint.setTypeface(Typeface.MONOSPACE);
      labelPaint.setTextSize(textSize);
      labelPaint.setAntiAlias(true);
      labelPaint.setARGB(255, 255, 255, 255);
      smallCharLabels = new int[CHAR_LABEL_PARMS.NUM_CHARS.getValue()];
      for (i = 0; i < smallCharLabels.length; i++)
      {
         smallCharLabels[i] =
            smallLabelMaker.add(gl,
                                Character.toString((char)(i + CHAR_LABEL_PARMS.CHAR_OFFSET.getValue())),
                                labelPaint);
      }
      smallLabelMaker.endAdding(gl);

      // Create large text labels.
      if (largeLabelMaker != null)
      {
         largeLabelMaker.shutdown(gl);
      }
      textSize = Math.min((windowWidth / COLS), (windowHeight / LINEZ));
      if (context.getResources().getConfiguration().orientation ==
          Configuration.ORIENTATION_PORTRAIT)
      {
         textSize = (int)((float)textSize * 1.9f);
      }
      else
      {
         textSize = (int)((float)textSize * 2.1f);
      }
      if (textSize > MAX_TEXT_SIZE) { textSize = MAX_TEXT_SIZE; }
      x = CHAR_LABEL_PARMS.NUM_CHARS.getValue() * textSize * 2;
      y = textSize * 2;
      for (w2 = 2; w2 < x; w2 *= 2) {}
      for (h2 = 2; h2 < y; h2 *= 2) {}
      largeLabelMaker = new LabelMaker(true, w2, h2);
      largeLabelMaker.initialize(gl);
      largeLabelMaker.beginAdding(gl);
      labelPaint = new Paint();
      labelPaint.setTypeface(Typeface.MONOSPACE);
      labelPaint.setTextSize(textSize);
      labelPaint.setAntiAlias(true);
      labelPaint.setARGB(255, 255, 255, 255);
      largeCharLabels = new int[CHAR_LABEL_PARMS.NUM_CHARS.getValue()];
      for (i = 0; i < largeCharLabels.length; i++)
      {
         largeCharLabels[i] =
            largeLabelMaker.add(gl,
                                Character.toString((char)(i + CHAR_LABEL_PARMS.CHAR_OFFSET.getValue())),
                                labelPaint);
      }
      largeLabelMaker.endAdding(gl);

      // Create welcome text.
      if (welcomeLabelMaker != null)
      {
         welcomeLabelMaker.shutdown(gl);
      }
      welcomeText    = new String[5];
      welcomeText[0] = "Swipe to move";
      welcomeText[1] = "Tap for keyboard";
      welcomeText[2] = "Type ? for commands";
      welcomeText[3] = "Type / or tap object to identify";
      welcomeText[4] = "dialectek.com";
      textSize       = windowWidth / welcomeText[0].length();
      if (textSize > MAX_TEXT_SIZE) { textSize = MAX_TEXT_SIZE; }
      x = welcomeText[0].length() * textSize * 2;
      y = textSize * 2;
      for (w2 = 2; w2 < x; w2 *= 2) {}
      for (h2 = 2; h2 < y; h2 *= 2) {}
      welcomeLabelMaker = new LabelMaker(true, w2, h2);
      welcomeLabelMaker.initialize(gl);
      welcomeLabelMaker.beginAdding(gl);
      labelPaint = new Paint();
      labelPaint.setTypeface(Typeface.MONOSPACE);
      labelPaint.setTextSize(textSize);
      labelPaint.setAntiAlias(true);
      labelPaint.setARGB(255, 255, 255, 255);
      welcomeLabels = new int[welcomeText.length];
      for (i = 0; i < welcomeText.length; i++)
      {
         welcomeLabels[i] = welcomeLabelMaker.add(gl, welcomeText[i], labelPaint);
      }
      welcomeLabelMaker.endAdding(gl);
   }


   // Re-make textures on next draw.
   public void invalidateTextures()
   {
      texturesValid       = false;
      scene.texturesValid = false;
   }


   // Draw.
   @Override
   public void onDrawFrame(GL10 gl)
   {
      // Play sound?
      int soundID;

      while ((soundID = getSound()) != -1)
      {
         SoundManager.playSound(soundID, 1.0f);
      }

      gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);
      gl.glMatrixMode(GL10.GL_MODELVIEW);
      gl.glLoadIdentity();

      // Make textures?
      if (!texturesValid)
      {
         makeTextures(gl);
         softKeyboardToggle =
            new SoftKeyboardToggle(windowWidth, windowHeight, 0, 0, gl, context);
         voicePrompter =
            new VoicePrompter(windowWidth, windowHeight,
                              -(int)((float)softKeyboardToggle.width * 0.1f),
                              -(int)((float)softKeyboardToggle.height * 1.1f),
                              gl, context);
         texturesValid = true;
      }

      // Lock display.
      lockDisplay();

      // Display keyboard and voice prompt?
      if (view == View.FPVIEW)
      {
         softKeyboardToggle.draw(gl);

         // Display voice prompter?
         if (glview.voiceEnabled)
         {
            voicePrompterVisible = true;
            voicePrompter.draw(gl);
         }
         else
         {
            voicePrompterVisible = false;
         }
      } else {
         voicePrompterVisible = false;
      }

      // Clear picking objects.
      scene.pickingObjects.clear();

      // Display text and scene.
      if (!displayText(gl))
      {
         if (!isBlind())
         {
            displayScene(gl);
         }
      }

      gl.glFlush();

      unlockDisplay();

      // Enter reactive mode?
      if ((SystemClock.uptimeMillis() - modeTimer) > RENDERMODE_CONTINUOUSLY_TIME)
      {
         glview.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
      }

      // View manual?
      if (viewManual)
      {
         viewManual = false;
         softKeyboardToggle.toggle();
         glview.viewManual();
      }
   }


   // Display text.
   // Return true if text scene drawn.
   boolean displayText(GL10 gl)
   {
      int  i, j, k, clen, llen, llenz, cols2;
      char c;

      char[] linebuf, linebuf2;
      boolean draw;
      boolean post;
      boolean largeText;
      int     showText;

      gl.glMatrixMode(GL10.GL_MODELVIEW);
      gl.glPushMatrix();
      gl.glLoadIdentity();

      smallLabelMaker.beginDrawing(gl, windowWidth, windowHeight);
      largeLabelMaker.beginDrawing(gl, windowWidth, windowHeight);
      welcomeLabelMaker.beginDrawing(gl, windowWidth, windowHeight);

      String msg = displaymsg();
      if ((msg != null) && msg.startsWith("View manual"))
      {
         viewManual = true;
      }

      draw = true;
      if (currentWindow() && !welcome)
      {
         clen    = windowWidth / COLS;
         llen    = windowHeight / LINES;
         llenz   = windowHeight / LINEZ;
         linebuf = new char[COLS];
         if (context.getResources().getConfiguration().orientation ==
             Configuration.ORIENTATION_PORTRAIT)
         {
            cols2 = (int)((float)COLS * 0.9f);
         }
         else
         {
            cols2 = (int)((float)COLS * 0.75f);
         }
         linebuf2 = new char[cols2];

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
         post      = false;
         largeText = true;
         showText  = showtext();
         for (i = 1, k = LINES - 2; i < k; i++)
         {
            for (j = 0; j < COLS; j++)
            {
               c = getWindowChar(i, j);

               // Cannot use large text?
               if (((i >= LINEZ) || (j >= 65)) && (c != ' '))
               {
                  largeText = false;
               }

               if (showText == 0)
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
               view = View.FPVIEW;
            }
         }

         // Force text scene for level overview?
         if (!draw && (view == View.OVERVIEW))
         {
            drawSmallText(gl, "Level overview:".toCharArray(), clen, windowHeight - llen);
            if (!isBlind())
            {
               draw = true;
            }
         }

         if (!draw)
         {
            largeText = true;
         }

         if (msg != null)
         {
            if (largeText)
            {
               drawLargeText(gl, msg.toCharArray(), 0, (int)(windowHeight - largeLabelMaker.getHeight(0)));
            }
            else
            {
               drawSmallText(gl, msg.toCharArray(), 0, (int)(windowHeight - smallLabelMaker.getHeight(0)));
            }
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
               k = LINES - 2;
               break;

            case 2:
               k = LINES - 1;
               break;
            }
            if (i == 0)
            {
               for (j = 0; j < cols2; j++)
               {
                  c           = getWindowChar(k, j);
                  linebuf2[j] = c;
               }
            }
            else
            {
               for (j = 0; j < COLS; j++)
               {
                  c          = getWindowChar(k, j);
                  linebuf[j] = c;
               }
            }
            if (largeText)
            {
               if (i == 0)
               {
                  drawLargeText(gl, linebuf2, 0, windowHeight - ((k + 1) * llenz));
               }
               else
               {
                  drawLargeText(gl, linebuf, 0, windowHeight - ((k - (LINES - LINEZ) + 1) * llenz));
               }
            }
            else
            {
               if (i == 0)
               {
                  drawSmallText(gl, linebuf2, 0, windowHeight - ((k + 1) * llen));
               }
               else
               {
                  drawSmallText(gl, linebuf, 0, windowHeight - ((k + 1) * llen));
               }
            }
         }

         // Draw text "scene"?
         if (draw)
         {
            for (i = 1, k = LINES - 2; i < k; i++)
            {
               for (j = 0; j < COLS; j++)
               {
                  c          = getWindowChar(i, j);
                  linebuf[j] = c;
               }
               if (largeText)
               {
                  drawLargeText(gl, linebuf, 0, windowHeight - ((i + 1) * llenz));
               }
               else
               {
                  drawSmallText(gl, linebuf, 0, windowHeight - ((i + 1) * llen));
               }
            }
         }
      }
      else
      {
         if (msg != null)
         {
            drawLargeText(gl, msg.toCharArray(), 0, (int)(windowHeight - largeLabelMaker.getHeight(0)));
         }
      }

      // Welcome?
      if (welcome)
      {
         drawWelcomeText(gl);
         if (SystemClock.uptimeMillis() - welcomeTimer >= WELCOME_TIME)
         {
            welcome = false;
         }
      }

      smallLabelMaker.endDrawing(gl);
      largeLabelMaker.endDrawing(gl);
      welcomeLabelMaker.endDrawing(gl);

      gl.glPopMatrix();

      return(draw);
   }


   // Draw text.
   void drawLargeText(GL10 gl, char[] text, int x, int y)
   {
      int i, j;

      int[] labels = new int[text.length];
      for (i = 0; i < text.length; i++)
      {
         j = (int)text[i] - CHAR_LABEL_PARMS.CHAR_OFFSET.getValue();
         if ((j < 0) || (j >= CHAR_LABEL_PARMS.NUM_CHARS.getValue()))
         {
            j = (int)' ' - CHAR_LABEL_PARMS.CHAR_OFFSET.getValue();
         }
         labels[i] = largeCharLabels[j];
      }
      largeLabelMaker.drawText(gl, labels, x, y);
   }


   void drawSmallText(GL10 gl, char[] text, int x, int y)
   {
      int i, j;

      int[] labels = new int[text.length];
      for (i = 0; i < text.length; i++)
      {
         j = (int)text[i] - CHAR_LABEL_PARMS.CHAR_OFFSET.getValue();
         if ((j < 0) || (j >= CHAR_LABEL_PARMS.NUM_CHARS.getValue()))
         {
            j = (int)' ' - CHAR_LABEL_PARMS.CHAR_OFFSET.getValue();
         }
         labels[i] = smallCharLabels[j];
      }
      smallLabelMaker.drawText(gl, labels, x, y);
   }


   void drawWelcomeText(GL10 gl)
   {
      float offset = largeLabelMaker.getHeight(0) * 2.0f;

      for (int i = 0; i < welcomeText.length; i++)
      {
         offset += welcomeLabelMaker.getHeight(i);
         welcomeLabelMaker.draw(gl, 0,
                                (int)((float)windowHeight - offset),
                                welcomeLabels[i]);
      }
   }


   // Display scene.
   void displayScene(GL10 gl)
   {
      int  i, j, k, c;
      char actual, visible;

      if (currentWindow())
      {
         // Initialize scene?
         if (!scene.ready)
         {
            scene.init(gl, COLS, LINES, windowAspect);
         }

         gl.glMatrixMode(GL10.GL_MODELVIEW);
         gl.glLoadIdentity();

         // Set up viewport for scene.
         i = (int)(windowHeight / (float)LINEZ);
         gl.glViewport(0, (2 * i), windowWidth, windowHeight - (3 * i));

         // Load window content into scene.
         for (i = 1, k = LINES - 2; i < k; i++)
         {
            for (j = 0; j < COLS; j++)
            {
               actual = getScreenChar(i, j);
               c      = (int)actual - CHAR_LABEL_PARMS.CHAR_OFFSET.getValue();
               if ((c < 0) || (c >= CHAR_LABEL_PARMS.NUM_CHARS.getValue()))
               {
                  actual = ' ';
               }
               visible = getWindowChar(i, j);
               c       = (int)visible - CHAR_LABEL_PARMS.CHAR_OFFSET.getValue();
               if ((c < 0) || (c >= CHAR_LABEL_PARMS.NUM_CHARS.getValue()))
               {
                  visible = ' ';
               }
               scene.placeObject(j, i, actual, visible);
            }
         }

         // Place player.
         scene.placePlayer(playerX, playerZ, playerDir());

         // Draw scene.
         scene.draw(gl, windowWidth, windowHeight);

         gl.glViewport(0, 0, windowWidth, windowHeight);
      }
   }


   // Pick character.
   int pickCharacter(int x_pointer, int y_pointer)
   {
      int character = 0;

      lockDisplay();

      int d1, d2;
      d2           = -1;
      float[] obj1 = null;
      float[] obj2 = null;
      for (int i = 0; i < scene.pickingObjects.size(); i++)
      {
         obj1 = scene.pickingObjects.get(i);
         if (obj1[3] < 1.0f)
         {
            d1 = (int)(Math.abs((float)x_pointer - obj1[1]) +
                       Math.abs((float)(windowHeight - y_pointer) - obj1[2]));
            if ((d2 == -1) || (d1 < d2))
            {
               d2   = d1;
               obj2 = obj1;
            }
         }
      }
      if (obj2 != null)
      {
         if (touched(x_pointer, y_pointer, (int)obj2[1], (int)obj2[2]))
         {
            character = (int)obj2[0];
         }
      }

      unlockDisplay();

      return(character);
   }


   // Touched point?
   boolean touched(int x1, int y1, int x2, int y2)
   {
      float TOUCH_DISTANCE_SCALE = 0.15f;
      float touchDistance        = 0.0f;

      if (windowWidth < windowHeight)
      {
         touchDistance = windowWidth * TOUCH_DISTANCE_SCALE;
      }
      else
      {
         touchDistance = windowHeight * TOUCH_DISTANCE_SCALE;
      }
      if ((Math.abs((float)x1 - (float)x2) <= touchDistance) &&
          (Math.abs((float)y1 - (float)windowHeight + (float)y2) <= touchDistance))
      {
         return(true);
      }
      else
      {
         return(false);
      }
   }


   // Soft keyboard toggle.
   public class SoftKeyboardToggle
   {
      int textureID;
      int windowWidth, windowHeight;
      int x, y, width, height;

      protected FloatBuffer vertexBuffer;
      protected float       vertices[] =
      {
         0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         1.0f, 0.0f, 0.0f,
         1.0f, 1.0f, 0.0f
      };

      protected FloatBuffer textureBuffer;
      protected float       texture[] =
      {
         0.0f, 1.0f,
         0.0f, 0.0f,
         1.0f, 1.0f,
         1.0f, 0.0f
      };

      public SoftKeyboardToggle(int windowWidth, int windowHeight,
                                int x_offset, int y_offset, GL10 gl, Context context)
      {
         this.windowWidth  = windowWidth;
         this.windowHeight = windowHeight;
         int d = 0;
         if (context.getResources().getConfiguration().orientation ==
             Configuration.ORIENTATION_PORTRAIT)
         {
            width = (int)((float)windowWidth / 8.0f);
            d     = (int)((float)windowHeight / 8.0f);
         }
         else
         {
            width = (int)((float)windowWidth / 6.0f);
            d     = (int)((float)windowHeight / 6.0f);
         }
         if (d < width)
         {
            width = d;
         }
         height    = width;
         x         = (int)((float)windowWidth - (float)width) + x_offset;
         y         = (int)((float)windowHeight - (float)height) + y_offset;
         textureID = createTexture("textures/keyboard.png", gl, context);
         ByteBuffer byteBuffer = ByteBuffer.allocateDirect(vertices.length * 4);
         byteBuffer.order(ByteOrder.nativeOrder());
         vertexBuffer = byteBuffer.asFloatBuffer();
         for (int i = 0; i < VERTS; i++)
         {
            vertices[i * 3]     *= width;
            vertices[i * 3 + 1] *= height;
         }
         vertexBuffer.put(vertices);
         vertexBuffer.position(0);
         byteBuffer = ByteBuffer.allocateDirect(texture.length * 4);
         byteBuffer.order(ByteOrder.nativeOrder());
         textureBuffer = byteBuffer.asFloatBuffer();
         textureBuffer.put(texture);
         textureBuffer.position(0);
      }


      public SoftKeyboardToggle() {}

      // Create texture from image file.
      private int createTexture(String imageFile, GL10 gl, Context context)
      {
         int[] textures = new int[1];
         gl.glGenTextures(1, textures, 0);
         int textureID = textures[0];

         InputStream is = null;
         try
         {
            is = context.getAssets().open(imageFile);
         }
         catch (IOException e) {}
         if (is == null)
         {
            Log.w("Blackguard", "Cannot open input mode texture file " + imageFile);
            return(textureID);
         }
         Bitmap b = null;
         try {
            b = BitmapFactory.decodeStream(is);
         }
         finally {
            try {
               is.close();
            }
            catch (IOException e) {}
         }
         if (b == null)
         {
            Log.w("Blackguard", "Cannot create input mode texture bitmap from image file " + imageFile);
            return(textureID);
         }
         gl.glBindTexture(GL10.GL_TEXTURE_2D, textureID);
         gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_NEAREST);
         gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
         GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, b, 0);
         b.recycle();
         return(textureID);
      }


      // Touched?
      boolean touched(int x_pointer, int y_pointer)
      {
         y_pointer = windowHeight - y_pointer;
         if ((x_pointer >= x) && (x_pointer <= (x + width)) &&
             (y_pointer >= y) && (y_pointer <= (y + height)))
         {
            return(true);
         }
         else
         {
            return(false);
         }
      }


      // Toggle soft keyboard.
      void toggle()
      {
         InputMethodManager imm =
            (InputMethodManager)context.getSystemService(Context.INPUT_METHOD_SERVICE);

         imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
      }


      // Draw.
      public void draw(GL10 gl)
      {
         gl.glMatrixMode(GL10.GL_PROJECTION);
         gl.glPushMatrix();
         gl.glLoadIdentity();
         gl.glOrthof(0.0f, windowWidth, 0.0f, windowHeight, 0.0f, 1.0f);
         gl.glMatrixMode(GL10.GL_MODELVIEW);
         gl.glPushMatrix();
         gl.glLoadIdentity();
         gl.glTranslatef(x, y, 0.0f);
         gl.glBindTexture(GL10.GL_TEXTURE_2D, textureID);
         gl.glEnable(GL10.GL_TEXTURE_2D);
         gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
         gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
         gl.glFrontFace(GL10.GL_CW);
         gl.glCullFace(GL10.GL_BACK);
         gl.glVertexPointer(3, GL10.GL_FLOAT, 0, vertexBuffer);
         gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, textureBuffer);
         gl.glDrawArrays(GL10.GL_TRIANGLE_STRIP, 0, vertices.length / 3);
         gl.glFrontFace(GL10.GL_CCW);
         gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
         gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
         gl.glMatrixMode(GL10.GL_PROJECTION);
         gl.glPopMatrix();
         gl.glMatrixMode(GL10.GL_MODELVIEW);
         gl.glPopMatrix();
      }


      private final static int VERTS = 4;
   }

   // Voice prompter.
   public class VoicePrompter
   {
      static final int VOICE_REQUEST_CODE = 1234;

      int textureID;
      int windowWidth, windowHeight;
      int x, y, width, height;

      protected FloatBuffer vertexBuffer;
      protected float       vertices[] =
      {
         0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         1.0f, 0.0f, 0.0f,
         1.0f, 1.0f, 0.0f
      };

      protected FloatBuffer textureBuffer;
      protected float       texture[] =
      {
         0.0f, 1.0f,
         0.0f, 0.0f,
         1.0f, 1.0f,
         1.0f, 0.0f
      };

      public VoicePrompter(int windowWidth, int windowHeight,
                           int x_offset, int y_offset, GL10 gl, Context context)
      {
         this.windowWidth  = windowWidth;
         this.windowHeight = windowHeight;
         int d = 0;
         if (context.getResources().getConfiguration().orientation ==
             Configuration.ORIENTATION_PORTRAIT)
         {
            width = (int)((float)windowWidth / 10.0f);
            d     = (int)((float)windowHeight / 10.0f);
         }
         else
         {
            width = (int)((float)windowWidth / 8.0f);
            d     = (int)((float)windowHeight / 8.0f);
         }
         if (d < width)
         {
            width = d;
         }
         height    = width;
         x         = (int)((float)windowWidth - (float)width) + x_offset;
         y         = (int)((float)windowHeight - (float)height) + y_offset;
         textureID = createTexture("textures/microphone.png", gl, context);
         ByteBuffer byteBuffer = ByteBuffer.allocateDirect(vertices.length * 4);
         byteBuffer.order(ByteOrder.nativeOrder());
         vertexBuffer = byteBuffer.asFloatBuffer();
         for (int i = 0; i < VERTS; i++)
         {
            vertices[i * 3]     *= width;
            vertices[i * 3 + 1] *= height;
         }
         vertexBuffer.put(vertices);
         vertexBuffer.position(0);
         byteBuffer = ByteBuffer.allocateDirect(texture.length * 4);
         byteBuffer.order(ByteOrder.nativeOrder());
         textureBuffer = byteBuffer.asFloatBuffer();
         textureBuffer.put(texture);
         textureBuffer.position(0);
      }


      public VoicePrompter() {}

      // Create texture from image file.
      private int createTexture(String imageFile, GL10 gl, Context context)
      {
         int[] textures = new int[1];
         gl.glGenTextures(1, textures, 0);
         int textureID = textures[0];

         InputStream is = null;
         try
         {
            is = context.getAssets().open(imageFile);
         }
         catch (IOException e) {}
         if (is == null)
         {
            Log.w("Blackguard", "Cannot open input mode texture file " + imageFile);
            return(textureID);
         }
         Bitmap b = null;
         try {
            b = BitmapFactory.decodeStream(is);
         }
         finally {
            try {
               is.close();
            }
            catch (IOException e) {}
         }
         if (b == null)
         {
            Log.w("Blackguard", "Cannot create input mode texture bitmap from image file " + imageFile);
            return(textureID);
         }
         gl.glBindTexture(GL10.GL_TEXTURE_2D, textureID);
         gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_NEAREST);
         gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
         GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, b, 0);
         b.recycle();
         return(textureID);
      }


      // Touched?
      boolean touched(int x_pointer, int y_pointer)
      {
         y_pointer = windowHeight - y_pointer;
         if ((x_pointer >= x) && (x_pointer <= (x + width)) &&
             (y_pointer >= y) && (y_pointer <= (y + height)))
         {
            return(true);
         }
         else
         {
            return(false);
         }
      }

      // Prompt for voice command.
      void prompt()
      {
         Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);

         intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL,
                         RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
         intent.putExtra(RecognizerIntent.EXTRA_PROMPT, "Say command...");
         ((Blackguard)context).intentActive = true;
         ((Blackguard)context).startActivityForResult(intent, VOICE_REQUEST_CODE);
      }


      // Draw.
      public void draw(GL10 gl)
      {
         gl.glMatrixMode(GL10.GL_PROJECTION);
         gl.glPushMatrix();
         gl.glLoadIdentity();
         gl.glOrthof(0.0f, windowWidth, 0.0f, windowHeight, 0.0f, 1.0f);
         gl.glMatrixMode(GL10.GL_MODELVIEW);
         gl.glPushMatrix();
         gl.glLoadIdentity();
         gl.glTranslatef(x, y, 0.0f);
         gl.glBindTexture(GL10.GL_TEXTURE_2D, textureID);
         gl.glEnable(GL10.GL_TEXTURE_2D);
         gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
         gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
         gl.glFrontFace(GL10.GL_CW);
         gl.glCullFace(GL10.GL_BACK);
         gl.glVertexPointer(3, GL10.GL_FLOAT, 0, vertexBuffer);
         gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, textureBuffer);
         gl.glDrawArrays(GL10.GL_TRIANGLE_STRIP, 0, vertices.length / 3);
         gl.glFrontFace(GL10.GL_CCW);
         gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
         gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
         gl.glMatrixMode(GL10.GL_PROJECTION);
         gl.glPopMatrix();
         gl.glMatrixMode(GL10.GL_MODELVIEW);
         gl.glPopMatrix();
      }


      private final static int VERTS = 4;
   }

   // Native methods:

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

   // Get/set player direction.
   private native int playerDir();

   // Player is blind?
   private native boolean isBlind();

   // Get sound to play.
   private native int getSound();

   // Get display message.
   private native String displaymsg();
}
