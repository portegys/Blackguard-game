/*
 * The game of Blackguard.
 *
 * Copyright (c) 2016 Tom Portegys (portegys@gmail.com). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name(s) of the author(s) nor the names of other contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "s3e.h"
#include "IwGx.h"
#include "s3eOSReadString.h"
#include "IwGxFont.h"
#include "IwResManager.h"
#include "AppMain.h"
#include <curses.h>
#include <pthread.h>
#include <errno.h>

// Current window.
WINDOW *CurrentWindow;
bool   PromptPresent;

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
DIRECTION CurrDir;

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

// Quit mode.
bool quitMode;
};

// Font.
CIwGxFont *font      = NULL;
int       fontHeight = 15;

// Display game.
void displayGame();

// Process keyboard input.
#define SPACE_KEY        32
#define DELETE_KEY       127
#define LINE_FEED        12
#define RETURN_KEY       13
#define BACKSPACE_KEY    8
#define ESCAPE           27
void keyboard(unsigned char);

// Set font to match dimensions.
void SetFont()
{
   if (font != NULL)
   {
      IwGxFontDestroyTTFont(font);
   }
   int width  = (int)((float)IwGxGetScreenWidth() / 80.0f);
   int height = (int)(((float)IwGxGetScreenHeight() * 0.9f) / 25.0f);
   int pixels = width;
   if (pixels > height)
   {
      pixels = height;
   }
   int points = (int)((float)pixels * 0.45f);
   font       = IwGxFontCreateTTFont("Anonymous.ttf", points);
   fontHeight = font->GetHeight();
   IwGxFontSetFont(font);
}


void SurfaceChangedCallback()
{
   SetFont();
}


void AppInit()
{
   // Initialize.
   IwGxRegister(IW_GX_SCREENSIZE, SurfaceChangedCallback);
   IwResManagerInit();

   // Initialize font.
   IwGxFontInit();
   SetFont();

   // Initialize rogue.
   CurrentWindow = NULL;
   CurrDir       = NORTH;
   showtext      = 0;
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
   Argc        = 0;
   Argv        = NULL;
   Envp        = NULL;
   if (pthread_create(&RogueThread, NULL, rogue_main, NULL) != 0)
   {
      fprintf(stderr, "pthread_create failed, errno=%d\n", errno);
      exit(1);
   }
}


void AppCheckQuit()
{
   if (!PromptPresent)
   {
      s3eDeviceRequestQuit();
   }
}


void AppShutDown()
{
   // Stop rogue thread.
   bool save_game = false;

   if (s3eOSReadStringAvailable())
   {
      const char *input = s3eOSReadStringUTF8("Save game (y|n)?");
      if (input && ((input[0] == 'Y') || (input[0] == 'y')))
      {
         save_game = true;
      }
   }
   quitMode = true;
   if (save_game)
   {
      keyboard('S');
   }
   else
   {
      keyboard('Q');
   }
   pthread_join(RogueThread, NULL);

   // Terminate.
   if (font != NULL)
   {
      IwGxFontDestroyTTFont(font);
   }
   IwGxFontTerminate();
   IwResManagerTerminate();
   IwGxUnRegister(IW_GX_SCREENSIZE, SurfaceChangedCallback);
}


void AppInput()
{
   if (s3eOSReadStringAvailable())
   {
      const char *input = s3eOSReadStringUTF8("Input:");
      if (input)
      {
         for (const char *c = input; *c != '\0'; c++)
         {
            char ch = *c;
            if ((ch == LINE_FEED) || (ch == RETURN_KEY) ||
                (ch == ESCAPE) || (ch == BACKSPACE_KEY) ||
                ((ch >= SPACE_KEY) && (ch <= DELETE_KEY)))
            {
               keyboard(' ');
               keyboard(ch);
            }
         }
      }
   }
}


bool AppUpdate()
{
   // Get cursor state.
   UpdateCursorState();
   switch (CheckCursorState())
   {
   case EXCURSOR_CLICK:
      keyboard(' ');
      return(true);

   case EXCURSOR_LEFT:
      keyboard('h');
      return(true);

   case EXCURSOR_RIGHT:
      keyboard('l');
      return(true);

   case EXCURSOR_UP:
      keyboard('k');
      return(true);

   case EXCURSOR_DOWN:
      keyboard('j');
      return(true);
   }

   // Get keyboard input.
   bool shift = false;
   if ((s3eKeyboardGetState(s3eKeyLeftShift) & S3E_KEY_STATE_DOWN) ||
       (s3eKeyboardGetState(s3eKeyRightShift) & S3E_KEY_STATE_DOWN))
   {
      shift = true;
   }
   switch (s3eKeyboardAnyKey())
   {
   case s3eKeyEsc:
      keyboard(ESCAPE);
      break;

   case s3eKeyTab:
      keyboard('\t');
      break;

   case s3eKeyBackspace:
      keyboard(8);
      break;

   case s3eKeyEnter:
      keyboard(13);
      break;

   case s3eKeySpace:
      keyboard(' ');
      break;

   case s3eKey0:
      if (shift)
      {
         keyboard(')');
      }
      else
      {
         keyboard('0');
      }
      break;

   case s3eKey1:
      if (shift)
      {
         keyboard('!');
      }
      else
      {
         keyboard('1');
      }
      break;

   case s3eKey2:
      if (shift)
      {
         keyboard('@');
      }
      else
      {
         keyboard('2');
      }
      break;

   case s3eKey3:
      if (shift)
      {
         keyboard('#');
      }
      else
      {
         keyboard('3');
      }
      break;

   case s3eKey4:
      if (shift)
      {
         keyboard('$');
      }
      else
      {
         keyboard('4');
      }
      break;

   case s3eKey5:
      if (shift)
      {
         keyboard('%');
      }
      else
      {
         keyboard('5');
      }
      break;

   case s3eKey6:
      if (shift)
      {
         keyboard('^');
      }
      else
      {
         keyboard('6');
      }
      break;

   case s3eKey7:
      if (shift)
      {
         keyboard('&');
      }
      else
      {
         keyboard('7');
      }
      break;

   case s3eKey8:
      if (shift)
      {
         keyboard('*');
      }
      else
      {
         keyboard('8');
      }
      break;

   case s3eKey9:
      if (shift)
      {
         keyboard('(');
      }
      else
      {
         keyboard('9');
      }
      break;

   case s3eKeyA:
      if (shift)
      {
         keyboard('A');
      }
      else
      {
         keyboard('a');
      }
      break;

   case s3eKeyB:
      if (shift)
      {
         keyboard('B');
      }
      else
      {
         keyboard('b');
      }
      break;

   case s3eKeyC:
      if (shift)
      {
         keyboard('C');
      }
      else
      {
         keyboard('c');
      }
      break;

   case s3eKeyD:
      if (shift)
      {
         keyboard('D');
      }
      else
      {
         keyboard('d');
      }
      break;

   case s3eKeyE:
      if (shift)
      {
         keyboard('E');
      }
      else
      {
         keyboard('e');
      }
      break;

   case s3eKeyF:
      if (shift)
      {
         keyboard('F');
      }
      else
      {
         keyboard('f');
      }
      break;

   case s3eKeyG:
      if (shift)
      {
         keyboard('G');
      }
      else
      {
         keyboard('g');
      }
      break;

   case s3eKeyH:
      if (shift)
      {
         keyboard('H');
      }
      else
      {
         keyboard('h');
      }
      break;

   case s3eKeyI:
      if (shift)
      {
         keyboard('I');
      }
      else
      {
         keyboard('i');
      }
      break;

   case s3eKeyJ:
      if (shift)
      {
         keyboard('J');
      }
      else
      {
         keyboard('j');
      }
      break;

   case s3eKeyK:
      if (shift)
      {
         keyboard('K');
      }
      else
      {
         keyboard('k');
      }
      break;

   case s3eKeyL:
      if (shift)
      {
         keyboard('L');
      }
      else
      {
         keyboard('l');
      }
      break;

   case s3eKeyM:
      if (shift)
      {
         keyboard('M');
      }
      else
      {
         keyboard('m');
      }
      break;

   case s3eKeyN:
      if (shift)
      {
         keyboard('N');
      }
      else
      {
         keyboard('n');
      }
      break;

   case s3eKeyO:
      if (shift)
      {
         keyboard('O');
      }
      else
      {
         keyboard('o');
      }
      break;

   case s3eKeyP:
      if (shift)
      {
         keyboard('P');
      }
      else
      {
         keyboard('p');
      }
      break;

   case s3eKeyQ:
      if (shift)
      {
         keyboard('Q');
      }
      else
      {
         keyboard('q');
      }
      break;

   case s3eKeyR:
      if (shift)
      {
         keyboard('R');
      }
      else
      {
         keyboard('r');
      }
      break;

   case s3eKeyS:
      if (shift)
      {
         keyboard('S');
      }
      else
      {
         keyboard('s');
      }
      break;

   case s3eKeyT:
      if (shift)
      {
         keyboard('T');
      }
      else
      {
         keyboard('t');
      }
      break;

   case s3eKeyU:
      if (shift)
      {
         keyboard('U');
      }
      else
      {
         keyboard('u');
      }
      break;

   case s3eKeyV:
      if (shift)
      {
         keyboard('V');
      }
      else
      {
         keyboard('v');
      }
      break;

   case s3eKeyW:
      if (shift)
      {
         keyboard('W');
      }
      else
      {
         keyboard('w');
      }
      break;

   case s3eKeyX:
      if (shift)
      {
         keyboard('X');
      }
      else
      {
         keyboard('x');
      }
      break;

   case s3eKeyY:
      if (shift)
      {
         keyboard('Y');
      }
      else
      {
         keyboard('y');
      }
      break;

   case s3eKeyZ:
      if (shift)
      {
         keyboard('Z');
      }
      else
      {
         keyboard('z');
      }
      break;

   case s3eKeyBacktick:
      if (shift)
      {
         keyboard('~');
      }
      else
      {
         keyboard('`');
      }
      break;

   case s3eKeyComma:
      if (shift)
      {
         keyboard('<');
      }
      else
      {
         keyboard(',');
      }
      break;

   case s3eKeyPeriod:
      if (shift)
      {
         keyboard('>');
      }
      else
      {
         keyboard('.');
      }
      break;

   case s3eKeySlash:
      if (shift)
      {
         keyboard('?');
      }
      else
      {
         keyboard('/');
      }
      break;

   case s3eKeyBackSlash:
      if (shift)
      {
         keyboard('|');
      }
      else
      {
         keyboard('\\');
      }
      break;

   case s3eKeySemicolon:
      if (shift)
      {
         keyboard(':');
      }
      else
      {
         keyboard(';');
      }
      break;

   case s3eKeyApostrophe:
      if (shift)
      {
         keyboard('"');
      }
      else
      {
         keyboard('\'');
      }
      break;

   case s3eKeyLeftBracket:
      if (shift)
      {
         keyboard('{');
      }
      else
      {
         keyboard('[');
      }
      break;

   case s3eKeyRightBracket:
      if (shift)
      {
         keyboard('}');
      }
      else
      {
         keyboard(']');
      }
      break;

   case s3eKeyEquals:
      if (shift)
      {
         keyboard('+');
      }
      else
      {
         keyboard('=');
      }
      break;

   case s3eKeyMinus:
      if (shift)
      {
         keyboard('_');
      }
      else
      {
         keyboard('-');
      }
      break;
   }

   return(true);
}


void AppRender()
{
   // Text drawing preparation.
   IwGxLightingOn();
   IwGxFontSetCol(0xff000000);

   // Show cursor keys.
   RenderCursorskeys();

   // Display game.
   displayGame();

   // Flush and swap.
   IwGxFlush();
   IwGxSwapBuffers();
}


// Display game.
void displayGame()
{
   int     i, j, k, nch;
   char    c, buf[LINLEN + 1];
   CIwRect rect(0, fontHeight, IwGxGetScreenWidth(), fontHeight);

   if (displaymsg)
   {
      IwGxFontSetRect(CIwRect(0, fontHeight, IwGxGetScreenWidth(), fontHeight));
      IwGxFontDrawText(displaymsg);
   }

   memset(buf, 0, 10);
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
         memset(buf, 0, LINLEN + 1);
         for (j = 0; j < COLS; j++)
         {
            nch    = mvwinch(CurrentWindow, k, j);
            buf[j] = (char)nch;
         }
         rect.y = (k * fontHeight) + fontHeight;
         IwGxFontSetRect(rect);
         IwGxFontDrawText(buf);
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

      for (i = 1, k = LINES - 2; i < k; i++)
      {
         memset(buf, 0, LINLEN + 1);
         for (j = 0; j < COLS; j++)
         {
            nch    = mvwinch(CurrentWindow, i, j);
            buf[j] = (char)nch;
         }
         rect.y = (i * fontHeight) + fontHeight;
         IwGxFontSetRect(rect);
         IwGxFontDrawText(buf);
      }
   }
}


// Keyboard input.
void keyboard(unsigned char key)
{
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

   dir = (DIRECTION)(((int)dir + (int)CurrDir) % 8);

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
   return((int)CurrDir);
}


void setDir(int dir)
{
   CurrDir = (DIRECTION)dir;
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
#ifdef SCENE
   if (scene != NULL)
   {
      scene->disposeAnimations();
   }
#endif
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
#ifdef SCENE
   if (scene != NULL)
   {
      scene->disposeAnimations();
   }
#endif
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
#ifdef SOUND
   playSound(openDungeonSound);
#endif
}


void notifyPickupGold()
{
#ifdef SOUND
   playSound(pickupGoldSound);
#endif
}


void notifyPickupObject()
{
#ifdef SOUND
   playSound(pickupObjectSound);
#endif
}


void notifyHitMonster(char show, int x, int z)
{
#ifdef SOUND
   playSound(hitMonsterSound);
#endif
   pthread_mutex_lock(&RefreshMutex);
#ifdef SCENE
   if (scene != NULL)
   {
      scene->addHitAnimation(show, x, z);
   }
#endif
   pthread_mutex_unlock(&RefreshMutex);
}


void notifyHitPlayer()
{
#ifdef SOUND
   playSound(hitPlayerSound);
#endif
}


void notifyKilled(char show, int x, int z)
{
#ifdef SOUND
   playSound(killMonsterSound);
#endif
   pthread_mutex_lock(&RefreshMutex);
#ifdef SCENE
   if (scene != NULL)
   {
      scene->addKilledAnimation(show, x, z);
   }
#endif
   pthread_mutex_unlock(&RefreshMutex);
}


void notifyLevelUp()
{
#ifdef SOUND
   playSound(levelUpSound);
#endif
}


void notifyThrowObject()
{
#ifdef SOUND
   playSound(throwObjectSound);
#endif
}


void notifyThunkMissile()
{
#ifdef SOUND
   playSound(thunkMissileSound);
#endif
}


void notifyZapWand()
{
#ifdef SOUND
   playSound(zapWandSound);
#endif
}


void notifyDipObject()
{
#ifdef SOUND
   playSound(dipObjectSound);
#endif
}


void notifyEatFood()
{
#ifdef SOUND
   playSound(eatFoodSound);
#endif
}


void notifyQuaffPotion()
{
#ifdef SOUND
   playSound(quaffPotionSound);
#endif
}


void notifyReadScroll()
{
#ifdef SOUND
   playSound(readScrollSound);
#endif
}


void notifyStairs()
{
#ifdef SOUND
   playSound(stairsSound);
#endif
}


void notifyDie()
{
#ifdef SOUND
   playSound(dieSound);
#endif
}


void notifyWinner()
{
#ifdef SOUND
   playSound(winnerSound);
#endif
}
}
