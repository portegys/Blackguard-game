/*
 * Blackguard: a 3D rogue-like game.
 *
 * @(#) RogueInterface.cpp	1.0	(tep)	 8/15/2012
 *
 * Blackguard
 * Copyright (C) 2012 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include "pch.h"

#include "RogueInterface.hpp"
#include "MediaReader.h"
#include <string>

// Current window.
WINDOW *CurrentWindow;

// Player direction.
DIRECTION PlayerDir;

// Rogue thread and synchronization.
CRITICAL_SECTION   RenderMutex;
CRITICAL_SECTION   InputMutex;
CONDITION_VARIABLE InputCond1;
CONDITION_VARIABLE InputCond2;
int                InputChar;
char               InputBuf[LINLEN];
int                InputIndex;
int                InputReq;

extern "C" {
// Arguments.
int  Argc;
char **Argv;

// Local directory.
char LocalDir[LINLEN];

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

// Monster animation list.
struct Animator *Animators;
bool            DisposeAnimations;

// Sound effects.
vector<SoundEffect ^> soundEffects;
Audio ^ audioController;

// Initialize game.
void initGame(char *id)
{
   CurrentWindow = NULL;
   showtext      = 0;
   PlayerDir     = NORTH;
   if (InitializeCriticalSectionEx(&RenderMutex, 0, 0) == 0)
   {
      fprintf(stderr, "InitializeCriticalSectionEx failed, error=%d\n", GetLastError());
      exit(1);
   }
   if (InitializeCriticalSectionEx(&InputMutex, 0, 0) == 0)
   {
      fprintf(stderr, "InitializeCriticalSectionEx failed, error=%d\n", GetLastError());
      exit(1);
   }
   InitializeConditionVariable(&InputCond1);
   InitializeConditionVariable(&InputCond2);
   InputReq          = 0;
   spaceprompt       = 0;
   isblind           = 0;
   RefreshCall       = 0;
   mute              = 0;
   displaymsg        = NULL;
   Animators         = NULL;
   DisposeAnimations = false;

   // Intialize sounds.
   try
   {
      audioController = ref new Audio;
      audioController->CreateDeviceIndependentResources();
      MediaReader ^ mediaReader = ref new MediaReader;
      auto engine    = audioController->SoundEffectEngine();
      auto format    = mediaReader->GetOutputWaveFormatEx();
      auto soundData = mediaReader->LoadMedia("Assets/sounds/opendungeon.wav");
      SoundEffect ^ soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/pickupgold.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/pickupobject.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/hitmonster.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/hitplayer.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/killmonster.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/experienceup.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/throwobject.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/thunkmissile.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/zapwand.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/dipobject.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/eatfood.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/quaffpotion.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/readscroll.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/gostairs.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/die.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
      soundData   = mediaReader->LoadMedia("Assets/sounds/winner.wav");
      soundEffect = ref new SoundEffect();
      soundEffect->Initialize(engine, format, soundData);
      soundEffects.push_back(soundEffect);
   }
   catch (Platform::Exception ^)
   {
   }

   // Start rogue thread.
   char **argv = new char * [2];
   argv[0] = (char *)"Blackguard.exe";
   argv[1] = NULL;
   Argc    = 1;
   Argv    = argv;
   ID      = id;
   create_task([]
               {
                  rogue_main(NULL);
               }
               );
}


// Save game.
void saveGame()
{
   dosave();
}


// Delete game.
void deleteGame()
{
   WIN32_FILE_ATTRIBUTE_DATA attr;

   if (file_name[0] != '\0')
   {
      _unlink(file_name);
   }
}


// Get local directory.
bool getLocalDir()
{
   std::wstring w;

   try
   {
      Windows::Storage::StorageFolder ^ f = (Windows::Storage::ApplicationData::Current)->LocalFolder;
      w = f->Path->Data();
   }
   catch (Platform::COMException ^ e)
   {
      return(false);
   }
   std::string s;
   s.assign(w.begin(), w.end());
   if ((strlen(s.c_str()) + 2) > LINLEN)
   {
      return(false);
   }
   else
   {
      sprintf(LocalDir, "%s", s.c_str());
      return(true);
   }
}


// Get resource path to file.
bool getResourcePath(char *filename, char *path, int len)
{
   Windows::Storage::StorageFolder ^ f = (Windows::Storage::ApplicationData::Current)->LocalFolder;
   std::wstring w = f->Path->Data();
   std::string  s;
   s.assign(w.begin(), w.end());
   if (((int)strlen(s.c_str()) + (int)strlen(filename) + 2) > len)
   {
      return(false);
   }
   else
   {
      sprintf(path, "%s\\%s", s.c_str(), filename);
      return(true);
   }
}


// Lock display.
void lockDisplay()
{
   EnterCriticalSection(&RenderMutex);
}


// Unlock display.
void unlockDisplay()
{
   LeaveCriticalSection(&RenderMutex);
}


// Current window?
bool currentWindow()
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


// Get window character at given location.
char getWindowChar(int line, int col)
{
   return(mvwinch(CurrentWindow, line, col));
}


// Get screen character at given location.
char getScreenChar(int line, int col)
{
   return(mvwinch(sw, line, col));
}


// Get showtext value.
int getShowtext()
{
   return(showtext);
}


// Blind?
bool isBlind()
{
   return(isblind != 0);
}


void playSound(int soundID)
{
   if ((mute == 0) && (soundID >= 0) && (soundID < (int)soundEffects.size()))
   {
      try
      {
         soundEffects[soundID]->PlaySound(SOUND_VOLUME);
      }
      catch (Platform::Exception ^)
      {
      }
   }
}


// Get display message.
char *getDisplaymsg()
{
   return(displaymsg);
}


// Lock input.
void lockInput()
{
   EnterCriticalSection(&InputMutex);
}


// Unlock input.
void unlockInput()
{
   LeaveCriticalSection(&InputMutex);
}


// Wait for input.
void waitInput()
{
   SleepConditionVariableCS(&InputCond1, &InputMutex, INFINITE);
}


// Signal input.
void signalInput()
{
   WakeConditionVariable(&InputCond2);
}


// Get input request.
int getInputReq()
{
   return(InputReq);
}


// Set input request.
void setInputReq(int val)
{
   InputReq = val;
}


// Set input character.
void setInputChar(char c)
{
   InputChar = c;
}


// Get input index.
int getInputIndex()
{
   return(InputIndex);
}


// Set input index.
void setInputIndex(int idx)
{
   InputIndex = idx;
}


// Buffer input character.
void bufferInputChar(int idx, char c)
{
   InputBuf[idx] = c;
}


// Get input buffer size.
int getBufferSize()
{
   return(LINLEN);
}


// Get and clear space prompt condition.
bool spacePrompt()
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


// Set dispose monster animations.
void setDisposeAnimations()
{
   DisposeAnimations = true;
}


// Get and clear dispose monster animations.
bool getDisposeAnimations()
{
   bool ret = DisposeAnimations;

   DisposeAnimations = false;
   return(ret);
}


// Get next animator.
bool getNextAnimator(int vals[])
{
   struct Animator *a;

   a = Animators;
   if ((a == NULL) || (vals == NULL))
   {
      return(false);
   }
   Animators = a->next;
   vals[0]   = (int)(a->show);
   vals[1]   = a->x;
   vals[2]   = a->z;
   if (a->killed)
   {
      vals[3] = 1;
   }
   else
   {
      vals[3] = 0;
   }
   delete a;
   return(true);
}


// Get mute value.
bool getMute()
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


// Set mute value.
void setMute(bool value)
{
   if (value)
   {
      mute = 1;
   }
   else
   {
      mute = 0;
   }
}


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
int clearok2(WINDOW *win, int bf)
{
   return(clearok(win, bf));
}


int clear2()
{
   return(wclear(sw));
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
   overwrite(sw, CurrentWindow);
   RefreshCall = 1;
   if (showtext > 0)
   {
      showtext--;
   }
   setDisposeAnimations();

   return(wrefresh(sw));
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
   setDisposeAnimations();

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
   return(wmove(sw, y, x));
}


int wmove2(WINDOW *win, int y, int x)
{
   return(wmove(win, y, x));
}


// Input
int getchar2()
{
   LeaveCriticalSection(&RenderMutex);
   EnterCriticalSection(&InputMutex);
   InputReq = 1;
   WakeConditionVariable(&InputCond1);
   SleepConditionVariableCS(&InputCond2, &InputMutex, INFINITE);
   LeaveCriticalSection(&InputMutex);
   EnterCriticalSection(&RenderMutex);
   return(InputChar);
}


int wgetch2(WINDOW *win)
{
   LeaveCriticalSection(&RenderMutex);
   EnterCriticalSection(&InputMutex);
   InputReq = 1;
   WakeConditionVariable(&InputCond1);
   SleepConditionVariableCS(&InputCond2, &InputMutex, INFINITE);
   LeaveCriticalSection(&InputMutex);
   EnterCriticalSection(&RenderMutex);
   return(InputChar);
}


int wgetnstr2(WINDOW *win, char *buf, int len)
{
   LeaveCriticalSection(&RenderMutex);
   EnterCriticalSection(&InputMutex);
   InputReq    = 2;
   InputIndex  = 0;
   InputBuf[0] = '\0';
   WakeConditionVariable(&InputCond1);
   SleepConditionVariableCS(&InputCond2, &InputMutex, INFINITE);
   strncpy(buf, InputBuf, len - 1);
   LeaveCriticalSection(&InputMutex);
   EnterCriticalSection(&RenderMutex);
   return(OK);
}


chtype winch2(WINDOW *win)
{
   return(winch(win));
}


chtype mvinch2(int y, int x)
{
   return(mvwinch(sw, y, x));
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
   return(waddch(sw, ch));
}


int waddch2(WINDOW *win, chtype ch)
{
   return(waddch(win, ch));
}


int mvaddch2(int y, int x, chtype ch)
{
   return(mvwaddch(sw, y, x, ch));
}


int mvwaddch2(WINDOW *win, int y, int x, chtype ch)
{
   return(mvwaddch(win, y, x, ch));
}


int addstr2(char *str)
{
   return(waddstr(sw, str));
}


int waddstr2(WINDOW *win, char *str)
{
   return(waddstr(win, str));
}


int mvaddstr2(int y, int x, char *str)
{
   return(mvwaddstr(sw, y, x, str));
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
   return(wprintw(sw, buf));
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
   return(mvwprintw(sw, y, x, buf));
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
   // Unlock renderer for open dungeon message.
   LeaveCriticalSection(&RenderMutex);
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


void Sleep(int ms)
{
   HANDLE event = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);

   WaitForSingleObjectEx(event, ms, false);
}
};
