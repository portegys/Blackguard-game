/*
 * Blackguard: a 3D rogue-like game.
 *
 * @(#) RogueInterface.hpp	1.0	(tep)	 8/15/2012
 *
 * Blackguard
 * Copyright (C) 2012 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <curses.h>
#include <errno.h>
#include <Windows.h>
#include <ppltasks.h>
#include <vector>
#include "Audio.h"
#include "SoundEffect.h"
using namespace concurrency;
using namespace std;

// Surface text dimensions.
enum
{
   TEXT_HEIGHT=24, TEXT_WIDTH=80
};

// Current window.
extern WINDOW *CurrentWindow;

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
extern DIRECTION PlayerDir;

// Rogue thread and synchronization.
#define LINLEN    256
extern CRITICAL_SECTION   RenderMutex;
extern CRITICAL_SECTION   InputMutex;
extern CONDITION_VARIABLE InputCond1;
extern CONDITION_VARIABLE InputCond2;
extern int                InputChar;
extern char               InputBuf[LINLEN];
extern int                InputIndex;
extern int                InputReq;

extern "C" {
// "stdscr"
extern WINDOW *sw;

// Rogue main.
extern int  Argc;
extern char **Argv;
void *rogue_main(void *);

// Save and load.
int dosave();
int doload();

extern char file_name[];

// Local directory.
extern char LocalDir[];
bool getLocalDir();

// Identity UUID.
extern char *ID;

// Space prompt.
extern int spaceprompt;

// Player is blind?
extern int isblind;

// Refresh called?
extern int RefreshCall;

// Show text display.
extern int showtext;

// Mute.
extern int mute;

// Display message.
extern char *displaymsg;

// Monster animation list.
struct Animator
{
   char            show;
   int             x, z;
   bool            killed;
   struct Animator *next;
};
extern struct Animator *Animators;
extern bool            DisposeAnimations;

// Sounds.
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
#define SOUND_VOLUME    1.0f
extern vector<SoundEffect ^> soundEffects;
extern Audio ^ audioController;

// Initialize game.
void initGame(char *id);

// Save game.
void saveGame();

// Delete game file.
void deleteGame();

// Get resource path to file.
bool getResourcePath(char *filename, char *path, int len);

// Lock display.
void lockDisplay();

// Unlock display.
void unlockDisplay();

// Current window?
bool currentWindow();

// Get window character at given location.
char getWindowChar(int line, int col);

// Get screen character at given location.
char getScreenChar(int line, int col);

// Get showtext value.
int getShowtext();

// Blind?
bool isBlind();

// Play sound.
void playSound(int soundID);

// Get display message.
char *getDisplaymsg();

// Lock input.
void lockInput();

// Unlock input.
void unlockInput();

// Wait for input.
void waitInput();

// Signal input.
void signalInput();

// Get input request.
int getInputReq();

// Set input request.
void setInputReq(int val);

// Set input character.
void setInputChar(char c);

// Get input index.
int getInputIndex();

// Set input index.
void setInputIndex(int idx);

// Buffer input character.
void bufferInputChar(int idx, char c);

// Get input buffer size.
int getBufferSize();

// Get and clear space prompt condition.
bool spacePrompt();

// Add monster animation.
void addAnimation(char show, int x, int z, bool killed);

// Set dispose monster animations.
void setDisposeAnimations();

// Get and clear dispose monster animations.
bool getDisposeAnimations();

// Get next animator.
bool getNextAnimator(int vals[]);

// Get mute value.
bool getMute();

// Set mute value.
void setMute(bool);

// Add direction to movement.
void add_dir(int *dx, int *dy);

// Get direction.
int getDir();

// Set direction.
void setDir(int dir);

/* Terminal function wrappers: */

/* Initialization. */
WINDOW *initscr2();

WINDOW *newwin2(int nlines, int ncols, int begin_y, int begin_x);

/* Window control. */
int clearok2(WINDOW *win, int bf);

int clear2();

int wclear2(WINDOW *win);

int wclrtoeol2(WINDOW *win);

int refresh2();

int wrefresh2(WINDOW *win);

int touchwin2(WINDOW *win);

int overlay2(WINDOW *srcwin, WINDOW *dstwin);

int overwrite2(WINDOW *srcwin, WINDOW *dstwin);

// Move cursor
int move2(int y, int x);

int wmove2(WINDOW *win, int y, int x);

// Input
int getchar2();

int wgetch2(WINDOW *win);

int wgetnstr2(WINDOW *win, char *buf, int len);

chtype winch2(WINDOW *win);

chtype mvinch2(int y, int x);

chtype mvwinch2(WINDOW *win, int y, int x);

// Output
int putchar2(int character);

int addch2(chtype ch);

int waddch2(WINDOW *win, chtype ch);

int mvaddch2(int y, int x, chtype ch);

int mvwaddch2(WINDOW *win, int y, int x, chtype ch);

int addstr2(char *str);

int waddstr2(WINDOW *win, char *str);

int mvaddstr2(int y, int x, char *str);

int mvwaddstr2(WINDOW *win, int y, int x, char *str);

int printw2(char *fmt, ...);

int wprintw2(WINDOW *win, char *fmt, ...);

int mvprintw2(int y, int x, char *fmt, ...);

int mvwprintw2(WINDOW *win, int y, int x, char *fmt, ...);

int printf2(char *format, ...);

int fputs2(char *s, FILE *stream);

int fflush2(FILE *stream);

// Termination
int isendwin2();

int endwin2();

// Event notifications.
void notifyOpenDungeon();

void notifyPickupGold();

void notifyPickupObject();

void notifyHitMonster(char show, int x, int z);

void notifyHitPlayer();

void notifyKilled(char show, int x, int z);

void notifyLevelUp();

void notifyThrowObject();

void notifyThunkMissile();

void notifyZapWand();

void notifyDipObject();

void notifyEatFood();

void notifyQuaffPotion();

void notifyReadScroll();

void notifyStairs();

void notifyDie();

void notifyWinner();

void Sleep(int ms);
};
