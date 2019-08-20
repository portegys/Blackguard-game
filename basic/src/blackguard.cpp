/*
 * Blackguard: a 3D rogue-like game.
 *
 * @(#) blackguard.cpp	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#ifdef WIN32
#ifndef _DEBUG
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <GL/glut.h>
#include <vector>
#include <curses.h>
#include <pthread.h>
#include <fmod.h>
#include <fmod_errors.h>
#include <errno.h>
#include "scene/scene.hpp"

using namespace std;

// Graphics window dimensions.
#define WINDOW_WIDTH     725
#define WINDOW_HEIGHT    380
int   WindowWidth  = WINDOW_WIDTH;
int   WindowHeight = WINDOW_HEIGHT;
float WindowAspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
int   MainWindow;

// Viewing controls.
enum
{
   FPVIEW=0, OVERVIEW=1
}
     ViewMode;
bool TextScene;
bool PromptPresent;

// Current window.
WINDOW *CurrentWindow;

// Player character.
#define PLAYER    '@'

// Player direction.
Scene::DIRECTION CurrDir;

// Player position.
int PlayerCol, PlayerLine;

// Rogue thread and synchronization.
#define LINLEN    80
pthread_mutex_t RefreshMutex;
pthread_mutex_t InputMutex;
pthread_cond_t  InputCond1;
pthread_cond_t  InputCond2;
int             InputChar;
char            InputBuf[LINLEN];
int             InputIndex;
int             InputReq;
pthread_t       RogueThread;

extern "C" {
// Rogue main.
int  Argc;
char **Argv;
char **Envp;
void *rogue_main(void *);

// Player is blind?
int isblind;

// Refresh called?
int RefreshCall;

// Show text display.
int showtext;

// Display message.
char *displaymsg;

// Mute.
int mute;
};

/*
 *  Available fonts:
 *  GLUT_BITMAP_8_BY_13
 *  GLUT_BITMAP_9_BY_15
 *  GLUT_BITMAP_TIMES_ROMAN_10
 *  GLUT_BITMAP_TIMES_ROMAN_24
 *  GLUT_BITMAP_HELVETICA_10
 *  GLUT_BITMAP_HELVETICA_12
 *  GLUT_BITMAP_HELVETICA_18
 */
#define FONT                      GLUT_BITMAP_9_BY_15
#define LINE_VERTICAL_MARGIN      30
#define LINE_HORIZONTAL_MARGIN    5
#define LINE_SPACE                15

// 2D functions.
void enter2Dmode(), exit2Dmode();
void draw2Dstring(GLfloat x, GLfloat y, void *font, char *string);
void enter2DMode(int width, int height), exit2DMode();

// Player scene.
Scene *scene;

// Sound.
FMOD_SYSTEM *soundSystem       = NULL;
FMOD_SOUND  *openDungeonSound  = NULL;
FMOD_SOUND  *pickupGoldSound   = NULL;
FMOD_SOUND  *pickupObjectSound = NULL;
FMOD_SOUND  *hitMonsterSound   = NULL;
FMOD_SOUND  *hitPlayerSound    = NULL;
FMOD_SOUND  *killMonsterSound  = NULL;
FMOD_SOUND  *levelUpSound      = NULL;
FMOD_SOUND  *throwObjectSound  = NULL;
FMOD_SOUND  *thunkMissileSound = NULL;
FMOD_SOUND  *zapWandSound      = NULL;
FMOD_SOUND  *dipObjectSound    = NULL;
FMOD_SOUND  *eatFoodSound      = NULL;
FMOD_SOUND  *quaffPotionSound  = NULL;
FMOD_SOUND  *readScrollSound   = NULL;
FMOD_SOUND  *stairsSound       = NULL;
FMOD_SOUND  *dieSound          = NULL;
FMOD_SOUND  *winnerSound       = NULL;

// Display text.
// Return true if scene requires drawing.
bool displayText()
{
   int  i, j, k, nch;
   char c, c2, c3;
   bool draw;
   bool post;
   bool maze;

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();
   enter2Dmode();

   char *msg = displaymsg;
   if (msg)
   {
      draw2Dstring(0, 15, FONT, msg);
   }

   draw = true;
   post = false;
   maze = false;
   if (CurrentWindow)
   {
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
         for (j = 0; j < COLS; j++)
         {
            nch = mvwinch(CurrentWindow, k, j);
            c   = nch;
            glRasterPos2i(j * 9, (k * 15) + 15);
            glutBitmapCharacter(FONT, c);
         }
      }

      // Check for presence of possible prompt.
      nch = mvwinch(CurrentWindow, 0, 0);
      c   = nch;
      if (c != ' ')
      {
         PromptPresent = true;
      }
      else
      {
         PromptPresent = false;
      }

      // Check for player character.
      if (showtext == 0)
      {
         for (i = 1, k = LINES - 2; i < k; i++)
         {
            for (j = 0; j < COLS; j++)
            {
               nch = mvwinch(CurrentWindow, i, j);
               c   = nch;
               if (c == PLAYER)
               {
                  PlayerCol  = j;
                  PlayerLine = i;
                  draw       = false;
               }

               // Could be in trading post?
               if ((c >= '0') && (c <= '4'))
               {
                  post = true;
               }

               // Check for maze level.
               nch = mvinch(i, j);
               c   = nch;
               if (c == '-')
               {
                  nch = mvinch(i - 1, j);
                  c   = nch;
                  if (c == '|')
                  {
                     nch = mvinch(i + 1, j);
                     c   = nch;
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
            draw     = true;
            ViewMode = FPVIEW;
         }
      }

      // Set text scene state.
      if (draw)
      {
         TextScene = true;
      }
      else
      {
         TextScene = false;
      }

      // Force text scene for level overview?
      if (ViewMode == OVERVIEW)
      {
         if (!draw)
         {
            draw2Dstring(5, 15, FONT, "Level overview:");
            draw = true;
         }
      }

      if (draw)
      {
         for (i = 1, k = LINES - 2; i < k; i++)
         {
            for (j = 0; j < COLS; j++)
            {
               nch = mvwinch(CurrentWindow, i, j);
               c   = nch;
               glRasterPos2i(j * 9, (i * 15) + 15);
               glutBitmapCharacter(FONT, c);
            }
         }
      }
   }

   exit2Dmode();
   glPopMatrix();

   return(!draw);
}


// Display scene.
void displayScene()
{
   int  i, j, k, nch;
   char actual, visible;

   if (CurrentWindow)
   {
      // Create scene?
      if (scene == NULL)
      {
         scene = new Scene(COLS, LINES, WindowAspect);
         assert(scene != NULL);
      }

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      // Set up viewport for scene.
      i = (int)(WindowHeight / (float)LINES);
      glViewport(0, (2 * i) + 2, WindowWidth, WindowHeight - (3 * i) - 4);

      // Load window content into scene.
      for (i = 1, k = LINES - 2; i < k; i++)
      {
         for (j = 0; j < COLS; j++)
         {
            nch     = mvinch(i, j);
            actual  = nch;
            nch     = mvwinch(CurrentWindow, i, j);
            visible = nch;
            scene->placeObject(j, i, actual, visible);
         }
      }

      // Place player.
      scene->placePlayer(PlayerCol, PlayerLine, CurrDir);

      // Draw scene.
      scene->draw();

      glViewport(0, 0, WindowWidth, WindowHeight);
   }
}


// Display function.
void display()
{
   pthread_mutex_lock(&RefreshMutex);

   // Clear display.
   glutSetWindow(MainWindow);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   // Display text and scene.
   if (displayText() && !isblind)
   {
      displayScene();
   }

   // Swap and flush frame buffers.
   glutSwapBuffers();
   glFlush();

   pthread_mutex_unlock(&RefreshMutex);
}


// Reshape.
void reshape(int width, int height)
{
   WindowWidth  = width;
   WindowHeight = height;
   WindowAspect = (float)width / (float)height;

   // Configure scene camera.
   if (scene != NULL)
   {
      scene->configureCamera(WindowAspect);
   }

   glutPostRedisplay();
}


// Keyboard input.
#define DELETE_KEY       127
#define RETURN_KEY       13
#define BACKSPACE_KEY    8
void
keyboard(unsigned char key, int x, int y)
{
   if (!TextScene && (ViewMode == OVERVIEW))
   {
      return;
   }

   // Wait for input request?
   pthread_mutex_lock(&InputMutex);
   if (InputReq == 0)
   {
      pthread_cond_wait(&InputCond1, &InputMutex);
   }

   switch (InputReq)
   {
   // Request for character.
   case 1:
      switch (key)
      {
      case RETURN_KEY:
         InputChar = '\n';
         break;

      case BACKSPACE_KEY:
         InputChar = '\b';
         break;

      default:
         InputChar = key;
         break;
      }
      InputReq = 0;
      pthread_cond_signal(&InputCond2);
      break;

   // Request for string.
   case 2:
      switch (key)
      {
      case RETURN_KEY:
         InputReq = 0;
         pthread_cond_signal(&InputCond2);
         break;

      case BACKSPACE_KEY:
         if (InputIndex > 0)
         {
            InputIndex--;
            InputBuf[InputIndex] = '\0';
         }
         break;

      default:
         if (InputIndex < LINLEN - 1)
         {
            InputBuf[InputIndex] = key;
            InputIndex++;
            InputBuf[InputIndex] = '\0';
         }
         break;
      }
      break;

   default:
      break;
   }
   pthread_mutex_unlock(&InputMutex);
}


// Special keyboard input.
void
specialKeyboard(int key, int x, int y)
{
   int d, mod;

   if (!TextScene && (ViewMode == OVERVIEW))
   {
      return;
   }

   mod = glutGetModifiers();
   switch (key)
   {
   case GLUT_KEY_UP:
      if (mod == GLUT_ACTIVE_SHIFT)
      {
         keyboard((int)'K', x, y);
      }
      else
      {
         keyboard((int)'k', x, y);
      }
      break;

   case GLUT_KEY_DOWN:
      if (mod == GLUT_ACTIVE_SHIFT)
      {
         keyboard((int)'J', x, y);
      }
      else
      {
         keyboard((int)'j', x, y);
      }
      break;

   case GLUT_KEY_RIGHT:
      d = (int)CurrDir;
      d--;
      if (d < 0)
      {
         d += 8;
      }
      if (mod == GLUT_ACTIVE_SHIFT)
      {
         d--;
         if (d < 0)
         {
            d += 8;
         }
      }
      CurrDir = (Scene::DIRECTION)d;
      glutPostRedisplay();
      break;

   case GLUT_KEY_LEFT:
      d = (int)CurrDir;
      d = (d + 1) % 8;
      if (mod == GLUT_ACTIVE_SHIFT)
      {
         d = (d + 1) % 8;
      }
      CurrDir = (Scene::DIRECTION)d;
      glutPostRedisplay();
      break;
   }
}


// Mouse click callback.
void mouseClicked(int button, int state, int x, int y)
{
   if ((button != GLUT_LEFT_BUTTON) || (state != GLUT_DOWN) || (PromptPresent && (ViewMode == FPVIEW)))
   {
      return;
   }

   if (ViewMode == OVERVIEW)
   {
      ViewMode = FPVIEW;
   }
   else
   {
      ViewMode = OVERVIEW;
   }
   glutPostRedisplay();
}


// Idle function.
void idle()
{
   // Update when refresh called.
   if (RefreshCall)
   {
      RefreshCall = 0;
      glutPostRedisplay();
   }
   else if ((scene != NULL) && (scene->cameraPending() || scene->hasAnimations()))
   {
      // Camera shot pending.
      glutPostRedisplay();
   }
}


// 2D mode.
void enter2Dmode()
{
   GLint viewport[4];

   glColor3f(1.0, 1.0, 1.0);
   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_LIGHTING);

   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   glGetIntegerv(GL_VIEWPORT, viewport);
   gluOrtho2D(0, viewport[2], 0, viewport[3]);

   // Invert the y axis, down is positive.
   glScalef(1, -1, 1);

   // Move the origin from the bottom left corner to the upper left corner.
   glTranslatef(0, -viewport[3], 0);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();
}


void exit2Dmode()
{
   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
}


// Print string on screen at specified location.
void draw2Dstring(GLfloat x, GLfloat y, void *font, char *string)
{
   char *c;

   glRasterPos2f(x, y);
   for (c = string; *c != '\0'; c++)
   {
      glutBitmapCharacter(font, *c);
   }
}


// Load sound.
void loadSound(char *soundFile, FMOD_SOUND **sound)
{
   char filename[128], path[1024];
   bool getResourcePath(char *filename, char *path, int len);

   sprintf(filename, "sounds/%s", soundFile);
   if (!getResourcePath(filename, path, 1024) &&
       !getResourcePath(soundFile, path, 1024))
   {
      fprintf(stderr, "Cannot access %s file\n", soundFile);
      return;
   }
#ifdef FMOD_UPDATE
   FMOD_RESULT result = FMOD_System_CreateSound(soundSystem, path, FMOD_DEFAULT, 0, sound);
#else
   FMOD_RESULT result = FMOD_System_CreateSound(soundSystem, path, FMOD_SOFTWARE | FMOD_3D, 0, sound);
#endif
   if (result != FMOD_OK)
   {
      fprintf(stderr, "FMOD_System_CreateSound error: (%d) %s\n", result, FMOD_ErrorString(result));
      *sound = NULL;
      return;
   }
   result = FMOD_Sound_SetMode(*sound, FMOD_LOOP_OFF);
   if (result != FMOD_OK)
   {
      fprintf(stderr, "FMOD_System_CreateSound error: (%d) %s\n", result, FMOD_ErrorString(result));
      *sound = NULL;
      return;
   }
}


// Initialize sounds.
void initSounds()
{
   FMOD_RESULT result = FMOD_System_Create(&soundSystem);

   if (result != FMOD_OK)
   {
      fprintf(stderr, "FMOD_System_Create error: (%d) %s\n", result, FMOD_ErrorString(result));
      exit(1);
   }
   result = FMOD_System_Init(soundSystem, 10, FMOD_INIT_NORMAL, 0);
   if (result != FMOD_OK)
   {
      fprintf(stderr, "FMOD_System_Init error: (%d) %s\n", result, FMOD_ErrorString(result));
      exit(1);
   }

   // Load sounds.
   loadSound("opendungeon.wav", &openDungeonSound);
   loadSound("pickupgold.wav", &pickupGoldSound);
   loadSound("pickupobject.wav", &pickupObjectSound);
   loadSound("hitmonster.wav", &hitMonsterSound);
   loadSound("hitplayer.wav", &hitPlayerSound);
   loadSound("killmonster.wav", &killMonsterSound);
   loadSound("experienceup.wav", &levelUpSound);
   loadSound("throwobject.wav", &throwObjectSound);
   loadSound("thunkmissile.wav", &thunkMissileSound);
   loadSound("zapwand.wav", &zapWandSound);
   loadSound("dipobject.wav", &dipObjectSound);
   loadSound("eatfood.wav", &eatFoodSound);
   loadSound("quaffpotion.wav", &quaffPotionSound);
   loadSound("readscroll.wav", &readScrollSound);
   loadSound("gostairs.wav", &stairsSound);
   loadSound("die.wav", &dieSound);
   loadSound("winner.wav", &winnerSound);
}


// Play sound.
void playSound(FMOD_SOUND *sound)
{
   if (!mute && (soundSystem != NULL) && (sound != NULL))
   {
#ifdef FMOD_UPDATE
      FMOD_System_PlaySound(soundSystem, sound, NULL, false, NULL);
#else
      FMOD_System_PlaySound(soundSystem, FMOD_CHANNEL_FREE, sound, 0, 0);
#endif
   }
}


// Get resource path to file.
bool getResourcePath(char *filename, char *path, int len)
{
   char *home;

   if (((home = getenv("BLACKGUARDHOME")) != NULL) && *home)
   {
      if ((strlen(home) + strlen(filename) + 2) > len)
      {
         return(false);
      }
      sprintf(path, "%s/%s", home, filename);
#ifndef WIN32
      if (access(path, R_OK) != -1)
#else
      if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
#endif
      {
         return(true);
      }
      if ((strlen(home) + strlen(filename) + strlen("/resource/") + 1) > len)
      {
         return(false);
      }
      sprintf(path, "%s/resource/%s", home, filename);
#ifndef WIN32
      if (access(path, R_OK) != -1)
#else
      if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
#endif
      {
         return(true);
      }
   }
   else
   {
      if ((strlen(filename) + 1) > len)
      {
         return(false);
      }
      sprintf(path, "%s", filename);
#ifndef WIN32
      if (access(path, R_OK) != -1)
#else
      if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
#endif
      {
         return(true);
      }
      if ((strlen(filename) + strlen("../resource/") + 1) > len)
      {
         return(false);
      }
      sprintf(path, "../resource/%s", filename);
#ifndef WIN32
      if (access(path, R_OK) != -1)
#else
      if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
#endif
      {
         return(true);
      }
   }
   return(false);
}


// Main.
int main(int argc, char **argv, char **envp)
{
#ifdef WIN32
#ifndef _DEBUG
   // Create hidden windows console for curses stdio.
   if (AllocConsole())
   {
      HWND hWnd = GetConsoleWindow();
      if (hWnd != NULL)
      {
         ShowWindow(hWnd, SW_HIDE);
      }
   }
#endif
#endif

   // Initialize graphics.
   glutInit(&argc, argv);
   glutInitWindowSize(WindowWidth, WindowHeight);
   glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
   MainWindow = glutCreateWindow("Blackguard");
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutSpecialFunc(specialKeyboard);
   glutMouseFunc(mouseClicked);
   glutIdleFunc(idle);
   glEnable(GL_DEPTH_TEST);
   glShadeModel(GL_SMOOTH);
   glEnable(GL_LINE_SMOOTH);
   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);
   glClearColor(0.0, 0.0, 0.0, 1.0);
   glColor3f(1.0f, 1.0f, 1.0f);
   glPointSize(3.0);
   glLineWidth(1.0);
   glutReshapeWindow(WindowWidth, WindowHeight);

   // Initialize interactivity.
   ViewMode      = FPVIEW;
   TextScene     = true;
   PromptPresent = false;
   CurrentWindow = NULL;
   showtext      = 0;
   CurrDir       = Scene::NORTH;
   if (pthread_mutex_init(&RefreshMutex, NULL) != 0)
   {
      fprintf(stderr, "pthread_mutex_init failed, errno=%d\n", errno);
      exit(1);
   }
   if (pthread_mutex_init(&InputMutex, NULL) != 0)
   {
      fprintf(stderr, "pthread_mutex_init failed, errno=%d\n", errno);
      exit(1);
   }
   if (pthread_cond_init(&InputCond1, NULL) != 0)
   {
      fprintf(stderr, "pthread_cond_init failed, errno=%d\n", errno);
      exit(1);
   }
   if (pthread_cond_init(&InputCond2, NULL) != 0)
   {
      fprintf(stderr, "pthread_cond_init failed, errno=%d\n", errno);
      exit(1);
   }
   InputReq    = 0;
   isblind     = 0;
   RefreshCall = 0;
   displaymsg  = NULL;
   mute        = 0;
   scene       = NULL;

#ifndef WIN32
#ifndef _DEBUG
   // Hide curses stdout and stderr.
   freopen("/dev/null", "w", stdout);
   freopen("/dev/null", "w", stderr);
#endif
#endif

   // Initialize sounds.
   initSounds();

   // Start rogue thread.
   Argc = argc;
   Argv = argv;
   Envp = envp;
   if (pthread_create(&RogueThread, NULL, rogue_main, NULL) != 0)
   {
      fprintf(stderr, "pthread_create failed, errno=%d\n", errno);
      exit(1);
   }

   // Game loop - does not return.
   glutMainLoop();
}


extern "C"
{
// Add direction to movement.
void add_dir(int *dx, int *dy)
{
   Scene::DIRECTION dir;

   if (*dx > 0)
   {
      if (*dy > 0)
      {
         dir = Scene::SOUTHEAST;
      }
      else if (*dy == 0)
      {
         dir = Scene::SOUTH;
      }
      else
      {
         dir = Scene::SOUTHWEST;
      }
   }
   else if (*dx == 0)
   {
      if (*dy > 0)
      {
         dir = Scene::EAST;
      }
      else if (*dy == 0)
      {
         return;
      }
      else
      {
         dir = Scene::WEST;
      }
   }
   else
   {
      if (*dy > 0)
      {
         dir = Scene::NORTHEAST;
      }
      else if (*dy == 0)
      {
         dir = Scene::NORTH;
      }
      else
      {
         dir = Scene::NORTHWEST;
      }
   }

   dir = (Scene::DIRECTION)(((int)dir + (int)CurrDir) % 8);

   switch (dir)
   {
   case Scene::NORTH:
      *dx = -1;
      *dy = 0;
      break;

   case Scene::NORTHEAST:
      *dx = -1;
      *dy = 1;
      break;

   case Scene::EAST:
      *dx = 0;
      *dy = 1;
      break;

   case Scene::SOUTHEAST:
      *dx = 1;
      *dy = 1;
      break;

   case Scene::SOUTH:
      *dx = 1;
      *dy = 0;
      break;

   case Scene::SOUTHWEST:
      *dx = 1;
      *dy = -1;
      break;

   case Scene::WEST:
      *dx = 0;
      *dy = -1;
      break;

   case Scene::NORTHWEST:
      *dx = -1;
      *dy = -1;
      break;
   }
}


// Get/set direction.
int getDir()
{
   return((int)CurrDir);
}


void setDir(int dir)
{
   CurrDir = (Scene::DIRECTION)dir;
}


/* Terminal function wrappers: */

/* Initialization. */
WINDOW *initscr2()
{
   return(initscr());
}


WINDOW *newwin2(int nlines, int ncols, int begin_y, int begin_x)
{
   return(newwin(nlines, ncols, begin_y, begin_x));
}


/* Window control. */
int clearok2(WINDOW *win, bool bf)
{
   return(clearok(win, bf));
}


int clear2()
{
   return(clear());
}


int wclear2(WINDOW *win)
{
   return(wclear(win));
}


int wclrtoeol2(WINDOW *win)
{
   return(wclrtoeol(win));
}


int refresh2()
{
   pthread_mutex_lock(&RefreshMutex);
   if (CurrentWindow == NULL)
   {
      CurrentWindow = newwin(0, 0, 0, 0);
   }
   overwrite(stdscr, CurrentWindow);
   RefreshCall = 1;
   if (showtext > 0)
   {
      showtext--;
   }
   if (scene != NULL)
   {
      scene->disposeAnimations();
   }
   pthread_mutex_unlock(&RefreshMutex);

   return(refresh());
}


int wrefresh2(WINDOW *win)
{
   pthread_mutex_lock(&RefreshMutex);
   if (CurrentWindow == NULL)
   {
      CurrentWindow = newwin(0, 0, 0, 0);
   }
   overwrite(win, CurrentWindow);
   RefreshCall = 1;
   if (showtext > 0)
   {
      showtext--;
   }
   if (scene != NULL)
   {
      scene->disposeAnimations();
   }
   pthread_mutex_unlock(&RefreshMutex);

   return(wrefresh(win));
}


int touchwin2(WINDOW *win)
{
   return(touchwin(win));
}


int overlay2(WINDOW *srcwin, WINDOW *dstwin)
{
   return(overlay(srcwin, dstwin));
}


int overwrite2(WINDOW *srcwin, WINDOW *dstwin)
{
   return(overwrite(srcwin, dstwin));
}


// Move cursor
int move2(int y, int x)
{
   return(move(y, x));
}


int wmove2(WINDOW *win, int y, int x)
{
   return(wmove(win, y, x));
}


// Input
int getchar2()
{
   pthread_mutex_lock(&InputMutex);
   InputReq = 1;
   pthread_cond_signal(&InputCond1);
   pthread_cond_wait(&InputCond2, &InputMutex);
   pthread_mutex_unlock(&InputMutex);
   return(InputChar);
}


#ifdef WIN32
extern int _getch();

int _getch2()
{
   pthread_mutex_lock(&InputMutex);
   InputReq = 1;
   pthread_cond_signal(&InputCond1);
   pthread_cond_wait(&InputCond2, &InputMutex);
   pthread_mutex_unlock(&InputMutex);
   return(InputChar);
}
#endif





int wgetch2(WINDOW *win)
{
   pthread_mutex_lock(&InputMutex);
   InputReq = 1;
   pthread_cond_signal(&InputCond1);
   pthread_cond_wait(&InputCond2, &InputMutex);
   pthread_mutex_unlock(&InputMutex);
   return(InputChar);
}


int wgetnstr2(WINDOW *win, char *buf, int len)
{
   pthread_mutex_lock(&InputMutex);
   InputReq    = 2;
   InputIndex  = 0;
   InputBuf[0] = '\0';
   pthread_cond_signal(&InputCond1);
   pthread_cond_wait(&InputCond2, &InputMutex);
   strncpy(buf, InputBuf, len - 1);
   pthread_mutex_unlock(&InputMutex);
   return(OK);
}


chtype winch2(WINDOW *win)
{
   return(winch(win));
}


chtype mvinch2(int y, int x)
{
   return(mvinch(y, x));
}


chtype mvwinch2(WINDOW *win, int y, int x)
{
   return(mvwinch(win, y, x));
}


// Output
int putchar2(int character)
{
   return(putchar(character));
}


int addch2(chtype ch)
{
   return(addch(ch));
}


int waddch2(WINDOW *win, chtype ch)
{
   return(waddch(win, ch));
}


int mvaddch2(int y, int x, chtype ch)
{
   return(mvaddch(y, x, ch));
}


int mvwaddch2(WINDOW *win, int y, int x, chtype ch)
{
   return(mvwaddch(win, y, x, ch));
}


int addstr2(char *str)
{
   return(addstr(str));
}


int waddstr2(WINDOW *win, char *str)
{
   return(waddstr(win, str));
}


int mvaddstr2(int y, int x, char *str)
{
   return(mvaddstr(y, x, str));
}


int mvwaddstr2(WINDOW *win, int y, int x, char *str)
{
   return(mvwaddstr(win, y, x, str));
}


int printw2(char *fmt, ...)
{
   char    buf[LINLEN];
   va_list args;

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);
   return(printw(buf));
}


int wprintw2(WINDOW *win, char *fmt, ...)
{
   char    buf[LINLEN];
   va_list args;

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);
   return(wprintw(win, buf));
}


int mvprintw2(int y, int x, char *fmt, ...)
{
   char    buf[LINLEN];
   va_list args;

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);
   return(mvprintw(y, x, buf));
}


int mvwprintw2(WINDOW *win, int y, int x, char *fmt, ...)
{
   char    buf[LINLEN];
   va_list args;

   va_start(args, fmt);
   vsprintf(buf, fmt, args);
   va_end(args);
   return(mvwprintw(win, y, x, buf));
}


int printf2(char *format, ...)
{
   int     ret;
   va_list args;

   va_start(args, format);
   ret = vprintf(format, args);
   va_end(args);
   return(ret);
}


int fputs2(char *s, FILE *stream)
{
   return(fputs(s, stream));
}


int fflush2(FILE *stream)
{
   return(fflush(stream));
}


// Termination
int isendwin2()
{
   return(isendwin());
}


int endwin2()
{
   return(endwin());
}


// Event notifications.
void notifyOpenDungeon()
{
   playSound(openDungeonSound);
}


void notifyPickupGold()
{
   playSound(pickupGoldSound);
}


void notifyPickupObject()
{
   playSound(pickupObjectSound);
}


void notifyHitMonster(char show, int x, int z)
{
   playSound(hitMonsterSound);
   pthread_mutex_lock(&RefreshMutex);
   if (scene != NULL)
   {
      scene->addHitAnimation(show, x, z);
   }
   pthread_mutex_unlock(&RefreshMutex);
}


void notifyHitPlayer()
{
   playSound(hitPlayerSound);
}


void notifyKilled(char show, int x, int z)
{
   playSound(killMonsterSound);
   pthread_mutex_lock(&RefreshMutex);
   if (scene != NULL)
   {
      scene->addKilledAnimation(show, x, z);
   }
   pthread_mutex_unlock(&RefreshMutex);
}


void notifyLevelUp()
{
   playSound(levelUpSound);
}


void notifyThrowObject()
{
   playSound(throwObjectSound);
}


void notifyThunkMissile()
{
   playSound(thunkMissileSound);
}


void notifyZapWand()
{
   playSound(zapWandSound);
}


void notifyDipObject()
{
   playSound(dipObjectSound);
}


void notifyEatFood()
{
   playSound(eatFoodSound);
}


void notifyQuaffPotion()
{
   playSound(quaffPotionSound);
}


void notifyReadScroll()
{
   playSound(readScrollSound);
}


void notifyStairs()
{
   playSound(stairsSound);
}


void notifyDie()
{
   playSound(dieSound);
}


void notifyWinner()
{
   playSound(winnerSound);
}
}
