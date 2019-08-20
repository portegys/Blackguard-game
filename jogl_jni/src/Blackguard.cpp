/*
 * Blackguard: a 3D rogue-like game.
 *
 * @(#) Blackguard.cpp	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#ifndef WIN32
typedef long long   __int64;
#endif

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <string.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <curses.h>
#include <pthread.h>
#include <fmod.h>
#include <fmod_errors.h>
#include <errno.h>
#include "blackguard_Blackguard.h"
#include "blackguard_Scene.h"

using namespace std;

// Current window.
WINDOW *CurrentWindow;

// Player direction.
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
DIRECTION PlayerDir;

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

// Environment.
char **environ;
};

// Monster animation list.
struct Animator
{
   char            show;
   int             x, z;
   bool            killed;
   struct Animator *next;
};
struct Animator *Animators;
bool            DisposeAnimations;

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
   FMOD_RESULT result = FMOD_System_CreateSound(soundSystem, path, FMOD_SOFTWARE | FMOD_3D, 0, sound);
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
      FMOD_System_PlaySound(soundSystem, FMOD_CHANNEL_FREE, sound, 0, 0);
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


/*
 * Class:     blackguard_Blackguard
 * Method:    initBlackGuard
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_blackguard_Blackguard_initBlackGuard(JNIEnv *env, jobject obj, jobjectArray array)
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

   CurrentWindow = NULL;
   showtext      = 0;
   PlayerDir     = NORTH;
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
   InputReq          = 0;
   isblind           = 0;
   RefreshCall       = 0;
   displaymsg        = NULL;
   mute              = 0;
   Animators         = NULL;
   DisposeAnimations = false;

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
   jint length = env->GetArrayLength(array);
   char **argv = new char * [length + 1];
   argv[0] = "Blackguard.exe";
   for (int i = 0; i < length; i++)
   {
      jstring string = (jstring)(env->GetObjectArrayElement(array, i));
      argv[i + 1] = (char *)(env->GetStringUTFChars(string, NULL));
   }
   Argc = length + 1;
   Argv = argv;
   Envp = environ;
   if (pthread_create(&RogueThread, NULL, rogue_main, NULL) != 0)
   {
      fprintf(stderr, "pthread_create failed, errno=%d\n", errno);
      exit(1);
   }

   /*
    * for(int i = 0; i < length; i++)
    * {
    * jstring string = (jstring)(env->GetObjectArrayElement(array, i));
    * env->ReleaseStringUTFChars(string, (const char *)argv[i + 1]);
    * }
    * delete [] argv;
    */
}


/*
 * Class:     blackguard_Blackguard
 * Method:    lockDisplay
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_blackguard_Blackguard_lockDisplay(JNIEnv *env, jobject obj)
{
   pthread_mutex_lock(&RefreshMutex);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    unlockDisplay
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_blackguard_Blackguard_unlockDisplay(JNIEnv *env, jobject obj)
{
   pthread_mutex_unlock(&RefreshMutex);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    currentWindow
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_blackguard_Blackguard_currentWindow(JNIEnv *env, jobject obj)
{
   if (CurrentWindow != NULL)
   {
      return(true);
   }
   else
   {
      return(false);
   }
}


/*
 * Class:     blackguard_Blackguard
 * Method:    getLines
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_getLines(JNIEnv *env, jobject obj)
{
   return(LINES);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    getCols
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_getCols(JNIEnv *env, jobject obj)
{
   return(COLS);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    getWindowChar
 * Signature: (II)C
 */
JNIEXPORT jchar JNICALL Java_blackguard_Blackguard_getWindowChar(JNIEnv *env, jobject obj, jint line, jint col)
{
   return(mvwinch(CurrentWindow, line, col));
}


/*
 * Class:     blackguard_Blackguard
 * Method:    getScreenChar
 * Signature: (II)C
 */
JNIEXPORT jchar JNICALL Java_blackguard_Blackguard_getScreenChar(JNIEnv *env, jobject obj, jint line, jint col)
{
   return(mvinch(line, col));
}


/*
 * Class:     blackguard_Blackguard
 * Method:    showtext
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_showtext(JNIEnv *env, jobject obj)
{
   return(showtext);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    displaymsg
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_blackguard_Blackguard_displaymsg(JNIEnv *env, jobject obj)
{
   return(env->NewStringUTF(displaymsg));
}


/*
 * Class:     blackguard_Blackguard
 * Method:    isBlind
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_blackguard_Blackguard_isBlind(JNIEnv *env, jobject obj)
{
   return(isblind);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    mute
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_blackguard_Blackguard_mute__(JNIEnv *env, jobject obj)
{
   return(mute);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    playerDir
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_playerDir__(JNIEnv *env, jobject obj)
{
   return((int)PlayerDir);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    playerDir
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_playerDir__I(JNIEnv *env, jobject obj, jint dir)
{
   PlayerDir = (DIRECTION)dir;
   return((int)PlayerDir);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    lockInput
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_blackguard_Blackguard_lockInput(JNIEnv *env, jobject obj)
{
   pthread_mutex_lock(&InputMutex);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    unlockInput
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_blackguard_Blackguard_unlockInput(JNIEnv *env, jobject obj)
{
   pthread_mutex_unlock(&InputMutex);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    waitInput
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_blackguard_Blackguard_waitInput(JNIEnv *env, jobject obj)
{
   pthread_cond_wait(&InputCond1, &InputMutex);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    signalInput
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_blackguard_Blackguard_signalInput(JNIEnv *env, jobject obj)
{
   pthread_cond_signal(&InputCond2);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    inputReq
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_inputReq__(JNIEnv *env, jobject obj)
{
   return(InputReq);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    inputReq
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_inputReq__I(JNIEnv *env, jobject obj, jint val)
{
   InputReq = val;
   return(InputReq);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    inputChar
 * Signature: (C)V
 */
JNIEXPORT void JNICALL Java_blackguard_Blackguard_inputChar(JNIEnv *env, jobject obj, jchar c)
{
   InputChar = c;
}


/*
 * Class:     blackguard_Blackguard
 * Method:    inputIndex
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_inputIndex__(JNIEnv *env, jobject obj)
{
   return(InputIndex);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    inputIndex
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_inputIndex__I(JNIEnv *env, jobject obj, jint idx)
{
   InputIndex = idx;
   return(InputIndex);
}


/*
 * Class:     blackguard_Blackguard
 * Method:    inputBuf
 * Signature: (IC)V
 */
JNIEXPORT void JNICALL Java_blackguard_Blackguard_inputBuf(JNIEnv *env, jobject obj, jint idx, jchar c)
{
   InputBuf[idx] = c;
}


/*
 * Class:     blackguard_Blackguard
 * Method:    inputSize
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_blackguard_Blackguard_inputSize(JNIEnv *env, jobject obj)
{
   return(LINLEN);
}


// Add monster animation.
void addAnimation(char show, int x, int z, bool killed)
{
   struct Animator *a;

   pthread_mutex_lock(&RefreshMutex);
   a         = new struct Animator;
   a->show   = show;
   a->x      = x;
   a->z      = z;
   a->killed = killed;
   a->next   = Animators;
   Animators = a;
   pthread_mutex_unlock(&RefreshMutex);
}


// Dispose of monster animations.
void disposeAnimations()
{
   DisposeAnimations = true;
}


/*
 * Class:     blackguard_Scene
 * Method:    disposeAnimations
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_blackguard_Scene_disposeAnimations(JNIEnv *env, jobject obj)
{
   jboolean ret = DisposeAnimations;

   DisposeAnimations = false;
   return(ret);
}


/*
 * Class:     blackguard_Scene
 * Method:    getAnimator
 * Signature: ()[I
 */
JNIEXPORT jintArray JNICALL Java_blackguard_Scene_getAnimator(JNIEnv *env, jobject obj)
{
   jintArray ret;

   ret = env->NewIntArray(4);
   if (ret == NULL)
   {
      return(NULL);
   }
   struct Animator *a;
   a = Animators;
   if (a == NULL)
   {
      return(NULL);
   }
   Animators = a->next;
   jint vals[4];
   vals[0] = (int)(a->show);
   vals[1] = a->x;
   vals[2] = a->z;
   if (a->killed)
   {
      vals[3] = 1;
   }
   else
   {
      vals[3] = 0;
   }
   delete a;
   env->SetIntArrayRegion(ret, 0, 4, vals);
   return(ret);
}


extern "C"
{
// Add direction to movement.
void add_dir(int *dx, int *dy)
{
   DIRECTION dir;

   if (*dx > 0)
   {
      if (*dy > 0)
      {
         dir = SOUTHEAST;
      }
      else if (*dy == 0)
      {
         dir = SOUTH;
      }
      else
      {
         dir = SOUTHWEST;
      }
   }
   else if (*dx == 0)
   {
      if (*dy > 0)
      {
         dir = EAST;
      }
      else if (*dy == 0)
      {
         return;
      }
      else
      {
         dir = WEST;
      }
   }
   else
   {
      if (*dy > 0)
      {
         dir = NORTHEAST;
      }
      else if (*dy == 0)
      {
         dir = NORTH;
      }
      else
      {
         dir = NORTHWEST;
      }
   }

   dir = (DIRECTION)(((int)dir + (int)PlayerDir) % 8);

   switch (dir)
   {
   case NORTH:
      *dx = -1;
      *dy = 0;
      break;

   case NORTHEAST:
      *dx = -1;
      *dy = 1;
      break;

   case EAST:
      *dx = 0;
      *dy = 1;
      break;

   case SOUTHEAST:
      *dx = 1;
      *dy = 1;
      break;

   case SOUTH:
      *dx = 1;
      *dy = 0;
      break;

   case SOUTHWEST:
      *dx = 1;
      *dy = -1;
      break;

   case WEST:
      *dx = 0;
      *dy = -1;
      break;

   case NORTHWEST:
      *dx = -1;
      *dy = -1;
      break;
   }
}


// Get/set direction.
int getDir()
{
   return((int)PlayerDir);
}


void setDir(int dir)
{
   PlayerDir = (DIRECTION)dir;
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
   disposeAnimations();
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
   disposeAnimations();
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
   addAnimation(show, x, z, false);
}


void notifyHitPlayer()
{
   playSound(hitPlayerSound);
}


void notifyKilled(char show, int x, int z)
{
   playSound(killMonsterSound);
   addAnimation(show, x, z, true);
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
