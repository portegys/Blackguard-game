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

typedef long long   __int64;

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <curses.h>
#include <pthread.h>
#include <errno.h>
#include <android/log.h>
#include "com_dialectek_blackguard_BlackguardView.h"
#include "com_dialectek_blackguard_BlackguardRenderer.h"
#include "com_dialectek_blackguard_Scene.h"
#include "com_dialectek_blackguard_SoundManager.h"

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
#define LINLEN    128
pthread_mutex_t RenderMutex;
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

// Save and load.
int dosave();
int doload();

extern char file_name[];

// Local directory.
char *LocalDir;

// Identity UUID.
char *ID;

// Space prompt.
int spaceprompt;

// Player is blind?
int isblind;

// Refresh called?
int RefreshCall;

// Show text display.
int showtext;

// Mute.
int mute;

// Display message.
char *displaymsg;

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

// Sounds.
// See also SoundManager.java
enum SOUND_ID
{
   openDungeonSound  = 0,
   pickupGoldSound   = 1,
   pickupObjectSound = 2,
   hitMonsterSound   = 3,
   hitPlayerSound    = 4,
   killMonsterSound  = 5,
   levelUpSound      = 6,
   throwObjectSound  = 7,
   thunkMissileSound = 8,
   zapWandSound      = 9,
   dipObjectSound    = 10,
   eatFoodSound      = 11,
   quaffPotionSound  = 12,
   readScrollSound   = 13,
   stairsSound       = 14,
   dieSound          = 15,
   winnerSound       = 16
};

int SoundIDs[3];

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
      if (access(path, R_OK) != -1)
      {
         return(true);
      }
      if ((strlen(home) + strlen(filename) + strlen("/resource/") + 1) > len)
      {
         return(false);
      }
      sprintf(path, "%s/resource/%s", home, filename);
      if (access(path, R_OK) != -1)
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
      if (access(path, R_OK) != -1)
      {
         return(true);
      }
      if ((strlen(filename) + strlen("../resource/") + 1) > len)
      {
         return(false);
      }
      sprintf(path, "../resource/%s", filename);
      if (access(path, R_OK) != -1)
      {
         return(true);
      }
   }
   return(false);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    lockDisplay
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_lockDisplay(JNIEnv *env, jobject obj)
{
   pthread_mutex_lock(&RenderMutex);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    unlockDisplay
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_unlockDisplay(JNIEnv *env, jobject obj)
{
   pthread_mutex_unlock(&RenderMutex);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    currentWindow
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_currentWindow(JNIEnv *env, jobject obj)
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
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    getLines
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_getLines(JNIEnv *env, jobject obj)
{
   return(LINES);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    getCols
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_getCols(JNIEnv *env, jobject obj)
{
   return(COLS);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    getWindowChar
 * Signature: (II)C
 */
JNIEXPORT jchar JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_getWindowChar(JNIEnv *env, jobject obj, jint line, jint col)
{
   return(mvwinch(CurrentWindow, line, col));
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    getScreenChar
 * Signature: (II)C
 */
JNIEXPORT jchar JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_getScreenChar(JNIEnv *env, jobject obj, jint line, jint col)
{
   return(mvinch(line, col));
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    showtext
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_showtext(JNIEnv *env, jobject obj)
{
   return(showtext);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    playerDir
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_playerDir(JNIEnv *env, jobject obj)
{
   return((int)PlayerDir);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    isBlind
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_isBlind(JNIEnv *env, jobject obj)
{
   return(isblind);
}


void playSound(int soundID)
{
   int i;

   for (i = 0; i < 3; i++)
   {
      if (SoundIDs[i] == -1)
      {
         SoundIDs[i] = soundID;
         break;
      }
   }
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    getSound
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_getSound(JNIEnv *env, jobject obj)
{
   int i, j;

   for (i = 0; i < 3; i++)
   {
      if (SoundIDs[i] != -1)
      {
         j           = SoundIDs[i];
         SoundIDs[i] = -1;
         return(j);
      }
   }
   return(-1);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardRenderer
 * Method:    displaymsg
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_dialectek_blackguard_BlackguardRenderer_displaymsg(JNIEnv *env, jobject obj)
{
   return(env->NewStringUTF(displaymsg));
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    initBlackguard
 * Signature: ([Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardView_initBlackguard(JNIEnv *env, jobject obj,
                                                                                   jobjectArray array, jstring localDir, jstring id)
{
   CurrentWindow = NULL;
   showtext      = 0;
   PlayerDir     = NORTH;
   if (pthread_mutex_init(&RenderMutex, NULL) != 0)
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
   spaceprompt       = 0;
   isblind           = 0;
   RefreshCall       = 0;
   mute              = 0;
   displaymsg        = NULL;
   Animators         = NULL;
   DisposeAnimations = false;
   SoundIDs[0]       = SoundIDs[1] = SoundIDs[2] = -1;

   // Start rogue thread.
   jint length = env->GetArrayLength(array);
   char **argv = new char * [length + 1];
   argv[0] = (char *)"Blackguard.exe";
   for (int i = 0; i < length; i++)
   {
      jstring string = (jstring)(env->GetObjectArrayElement(array, i));
      argv[i + 1] = (char *)(env->GetStringUTFChars(string, NULL));
   }
   Argc     = length + 1;
   Argv     = argv;
   Envp     = environ;
   LocalDir = (char *)(env->GetStringUTFChars(localDir, NULL));
   ID       = (char *)(env->GetStringUTFChars(id, NULL));
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
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    saveBlackguard
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardView_saveBlackguard(JNIEnv *env, jobject obj)
{
   dosave();
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    deleteBlackguard
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardView_deleteBlackguard(JNIEnv *env, jobject obj)
{
   if ((file_name[0] != '\0') && (access(file_name, R_OK) != -1))
   {
      unlink(file_name);
   }
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    playerDir
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardView_playerDir__(JNIEnv *env, jobject obj)
{
   return((int)PlayerDir);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    playerDir
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardView_playerDir__I(JNIEnv *env, jobject obj, jint dir)
{
   PlayerDir = (DIRECTION)dir;
   return((int)PlayerDir);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    lockInput
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardView_lockInput(JNIEnv *env, jobject obj)
{
   pthread_mutex_lock(&InputMutex);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    unlockInput
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardView_unlockInput(JNIEnv *env, jobject obj)
{
   pthread_mutex_unlock(&InputMutex);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    waitInput
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardView_waitInput(JNIEnv *env, jobject obj)
{
   pthread_cond_wait(&InputCond1, &InputMutex);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    signalInput
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardView_signalInput(JNIEnv *env, jobject obj)
{
   pthread_cond_signal(&InputCond2);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    inputReq
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardView_inputReq__(JNIEnv *env, jobject obj)
{
   return(InputReq);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    inputReq
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardView_inputReq__I(JNIEnv *env, jobject obj, jint val)
{
   InputReq = val;
   return(InputReq);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    inputChar
 * Signature: (C)V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardView_inputChar(JNIEnv *env, jobject obj, jchar c)
{
   InputChar = c;
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    inputIndex
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardView_inputIndex__(JNIEnv *env, jobject obj)
{
   return(InputIndex);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    inputIndex
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardView_inputIndex__I(JNIEnv *env, jobject obj, jint idx)
{
   InputIndex = idx;
   return(InputIndex);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    inputBuf
 * Signature: (IC)V
 */
JNIEXPORT void JNICALL Java_com_dialectek_blackguard_BlackguardView_inputBuf(JNIEnv *env, jobject obj, jint idx, jchar c)
{
   InputBuf[idx] = c;
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    inputSize
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_dialectek_blackguard_BlackguardView_inputSize(JNIEnv *env, jobject obj)
{
   return(LINLEN);
}


/*
 * Class:     com_dialectek_blackguard_BlackguardView
 * Method:    spacePrompt
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_dialectek_blackguard_BlackguardView_spacePrompt(JNIEnv *env, jobject obj)
{
   int ret = spaceprompt;

   spaceprompt = 0;
   if (ret == 0)
   {
      return(false);
   }
   else
   {
      return(true);
   }
}


// Add monster animation.
void addAnimation(char show, int x, int z, bool killed)
{
   struct Animator *a;

   a         = new struct Animator;
   a->show   = show;
   a->x      = x;
   a->z      = z;
   a->killed = killed;
   a->next   = Animators;
   Animators = a;
}


// Dispose of monster animations.
void disposeAnimations()
{
   DisposeAnimations = true;
}


/*
 * Class:     com_dialectek_blackguard_Scene
 * Method:    disposeAnimations
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_dialectek_blackguard_Scene_disposeAnimations(JNIEnv *env, jobject obj)
{
   jboolean ret = DisposeAnimations;

   DisposeAnimations = false;
   return(ret);
}


/*
 * Class:     com_dialectek_blackguard_Scene
 * Method:    getAnimator
 * Signature: ()[I
 */
JNIEXPORT jintArray JNICALL Java_com_dialectek_blackguard_Scene_getAnimator(JNIEnv *env, jobject obj)
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


/*
 * Class:     com_dialectek_blackguard_SoundManager
 * Method:    mute
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_dialectek_blackguard_SoundManager_mute(JNIEnv *env, jclass cls)
{
   if (mute == 0)
   {
      return(false);
   }
   else
   {
      return(true);
   }
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

   return(refresh());
}


int wrefresh2(WINDOW *win)
{
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
   pthread_mutex_unlock(&RenderMutex);  
   pthread_mutex_lock(&InputMutex);
   InputReq = 1;
   pthread_cond_signal(&InputCond1);
   pthread_cond_wait(&InputCond2, &InputMutex);
   pthread_mutex_unlock(&InputMutex);
   pthread_mutex_lock(&RenderMutex);   
   return(InputChar);
}


int wgetch2(WINDOW *win)
{
   pthread_mutex_unlock(&RenderMutex);
   pthread_mutex_lock(&InputMutex);
   InputReq = 1;
   pthread_cond_signal(&InputCond1);
   pthread_cond_wait(&InputCond2, &InputMutex);
   pthread_mutex_unlock(&InputMutex);
   pthread_mutex_lock(&RenderMutex);
   return(InputChar);
}


int wgetnstr2(WINDOW *win, char *buf, int len)
{
   pthread_mutex_unlock(&RenderMutex);
   pthread_mutex_lock(&InputMutex);
   InputReq    = 2;
   InputIndex  = 0;
   InputBuf[0] = '\0';
   pthread_cond_signal(&InputCond1);
   pthread_cond_wait(&InputCond2, &InputMutex);
   strncpy(buf, InputBuf, len - 1);
   pthread_mutex_unlock(&InputMutex);
   pthread_mutex_lock(&RenderMutex);
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
   pthread_mutex_unlock(&RenderMutex);  
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
