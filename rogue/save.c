/*
 * save and restore routines
 *
 * @(#) save.c	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#ifndef WIN32
#include <unistd.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include "rogue.h"

#ifdef WIN32
#define srand48(seed)	srand(seed)
#endif

EXTCHAR version[];
EXTCHAR *ctime();

typedef struct stat STAT;
STAT sbuf;

/*
 * ignore:
 *	Ignore ALL signals possible
 */
void
ignore()
{
#ifdef WIN32
	signal(SIGINT, SIG_IGN);
#else
	int i;

	for (i = 0; i < NSIG; i++)
		signal(i, SIG_IGN);
#endif
}

/*
 * save_game:
 *	Save the current game
 */
int
save_game()
{
	reg FILE *savef;
	reg int c;
	char buf[LINLEN];

	mpos = 0;
	if (file_name[0] != '\0') {
		msg("Save file (%s)? ", file_name);
		do {
			c = wgetch(cw);
			if(c == ESCAPE) {
				msg("");
				return FALSE;
			}
		} while (c != 'n' && c != 'y');
		mpos = 0;
		if (c == 'y')
			goto gotfile;
	}
	msg("File name: ");
	mpos = 0;
	buf[0] = '\0';
	if (get_str(buf, cw) == QUIT) {
		msg("");
		return FALSE;
	}
	msg("");
	strcpy(file_name, buf);
gotfile:
	c = dosave();		/* try to save this game */
	if (c == FALSE)
		msg("Could not save game to file %s", file_name);
	return c;
}

/*
 * auto_save:
 *	Automatically save a game
 */
void
auto_save(int a)
{
	dosave();		/* save this game */
	byebye(1);		/* so long for now */
}

/*
 * game_err:
 *	When an error occurs. Set error flag and save game.
 */
void
game_err(int a)
{
	int ok;

	ok = dosave();			/* try to save this game */
	clear();
	refresh();
	endwin();

	printf("\nInternal error !!!\n\nYour game was ");
	if (ok)
		printf("saved.");
	else
		printf("NOT saveable.");

	fflush(stdout);

#ifdef SIGIOT
	signal(SIGIOT, SIG_DFL);	/* allow core dump signal */
#endif

	abort();			/* cause core dump */
	byebye(3);
}

/*
 * dosave:
 *	Set UID back to user and save the game
 */
int
dosave()
{
	FILE *savef;

	ignore();
#ifndef WIN32
#ifndef ANDROID
	setuid(playuid);
	setgid(playgid);
#endif
#endif
	umask(022);

	if (file_name[0] != '\0') {
		if ((savef = fopen(file_name,"w")) != NULL)
		{
#ifdef WIN32
  			_setmode( _fileno( savef ), _O_BINARY );
#endif
			save_file(savef);
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * save_file:
 *	Do the actual save of this game to a file
 */
void
save_file(savef)
FILE *savef;
{
	reg int fnum;
	int slines = LINES;
	int scols = COLS;

	/*
	 * force allocation of the buffer now so that inodes, etc
	 * can be checked when restoring saved games.
	 */
#ifdef WIN32
	fnum = _fileno(savef);
#else
	fnum = fileno(savef);
#endif
	fstat(fnum, &sbuf);
#ifdef BLACKGUARD
	write(fnum, "TEP", 4);
#else
	write(fnum, "RDK", 4);
#endif
	lseek(fnum, 0L, 0);
	
	encwrite(version,strlen(version)+1,savef);
	encwrite(&sbuf.st_ino,sizeof(sbuf.st_ino),savef);
	encwrite(&sbuf.st_dev,sizeof(sbuf.st_dev),savef);
	encwrite(&sbuf.st_ctime,sizeof(sbuf.st_ctime),savef);
	encwrite(&sbuf.st_mtime,sizeof(sbuf.st_mtime),savef);
	encwrite(&slines,sizeof(slines),savef);
	encwrite(&scols,sizeof(scols),savef);
#if !ANDROID && !METRO
	msg("");
#endif
	rs_save_file(savef);
	close(fnum);
	signal(SIGINT, byebye);
#ifdef SIGQUIT
	signal(SIGQUIT, byebye);
#endif
#if !ANDROID && !METRO
	wclear(cw);
	draw(cw);
#endif
}

/*
 * docheckpoint:
 *	Checkpoint the game
 */
FILE *checkpointf;

int
docheckpoint()
{
	if (file_name[0] != '\0') {
		if (checkpointf == NULL) {
			if ((checkpointf = fopen(file_name, "w")) != NULL) {
#ifdef WIN32
				_setmode( _fileno( checkpointf ), _O_BINARY );
#endif
			}
		}
		fseek(checkpointf, 0, 0);
		checkpoint_file(checkpointf);
		fflush(checkpointf);
		return TRUE;
	}
	return FALSE;
}

/*
 * checkpoint_file:
 *	Do the actual checkpoint of this game to a file
 */
void
checkpoint_file(savef)
		FILE *savef;
{
	reg int fnum;
	int slines = LINES;
	int scols = COLS;

	/*
	 * force allocation of the buffer now so that inodes, etc
	 * can be checked when restoring saved games.
	 */
#ifdef WIN32
	fnum = _fileno(savef);
#else
	fnum = fileno(savef);
#endif
	fstat(fnum, &sbuf);
#ifdef BLACKGUARD
	write(fnum, "TEP", 4);
#else
	write(fnum, "RDK", 4);
#endif
	lseek(fnum, 0L, 0);

	encwrite(version,strlen(version)+1,savef);
	encwrite(&sbuf.st_ino,sizeof(sbuf.st_ino),savef);
	encwrite(&sbuf.st_dev,sizeof(sbuf.st_dev),savef);
	encwrite(&sbuf.st_ctime,sizeof(sbuf.st_ctime),savef);
	encwrite(&sbuf.st_mtime,sizeof(sbuf.st_mtime),savef);
	encwrite(&slines,sizeof(slines),savef);
	encwrite(&scols,sizeof(scols),savef);
	rs_save_file(savef);
}

/*
 * restore:
 *	Restore a saved game from a file
 */
int
#ifdef METRO
restore(file)
char *file;
#else
restore(file, envp)
char *file, **envp;
#endif
{
	register int inf, pid;
	int ret_status;
#ifndef _AIX
	extern char **environ;
#endif
	char buf[LINLEN];
	STAT sbuf2;
	int slines, scols;

#ifdef WIN32
	if ((inf = open(file, O_RDONLY | O_BINARY)) < 0) {
#else
	if ((inf = open(file, O_RDONLY)) < 0) {
#endif
		printf("Cannot read save game %s\n",file);
		return FALSE;
	}

	encread(buf, strlen(version) + 1, inf);

	if (strcmp(buf, version) != 0) {
		printf("Sorry, saved game version is out of date.\n");
		return FALSE;
	}

	fstat(inf, &sbuf2);

	encread(&sbuf.st_ino,sizeof(sbuf.st_ino), inf);
	encread(&sbuf.st_dev,sizeof(sbuf.st_dev), inf);
	encread(&sbuf.st_ctime,sizeof(sbuf.st_ctime), inf);
	encread(&sbuf.st_mtime,sizeof(sbuf.st_mtime), inf);
	encread(&slines,sizeof(slines),inf);
	encread(&scols,sizeof(scols),inf);

	/*
	 * we do not close the file so that we will have a hold of the
	 * inode for as long as possible
	 */

	if (!wizard)
	{
		if(sbuf2.st_ino!=sbuf.st_ino || sbuf2.st_dev!=sbuf.st_dev) {
			printf("Sorry, saved game is not in the same file.\n");
			return FALSE;
		}
	}

#ifdef __INTERIX
	setenv("TERM","interix");
#endif
#ifdef BLACKGUARD
#if ANDROID
		/* Set LINES and COLS. */
		putenv("LINES=24");
		putenv("COLS=80");

		/* Set TERM. */
		putenv("TERM=linux");
#elif !METRO
		/* Set LINES and COLS. */
		putenv("LINES=25");
		putenv("COLS=80");
#endif
#endif

	initscr();

	if (slines > LINES)
	{
		endwin();
		printf("Sorry, original game was played on a screen with %d lines.\n",slines);
		printf("Current screen only has %d lines. Unable to restore game\n",LINES);
		return(FALSE);
	}

	if (scols > COLS)
	{
		endwin();
		printf("Sorry, original game was played on a screen with %d columns.\n", scols);
		printf("Current screen only has %d columns. Unable to restore game\n",COLS);
		return(FALSE);
	}

	cw = newwin(LINES, COLS, 0, 0);
	mw = newwin(LINES, COLS, 0, 0);
	hw = newwin(LINES, COLS, 0, 0);
#if METRO
	sw = newwin(LINES, COLS, 0, 0);
#endif

	mpos = 0;
	mvwprintw(cw, 0, 0, "%s: %s", file, ctime(&sbuf2.st_mtime));

	/* defeat multiple restarting from the same place */

	if (!wizard)
	{
		if (sbuf2.st_nlink != 1)
		{
                        endwin();
			printf("Cannot restore from a linked file\n");
			return FALSE;
		}
	}

	if (rs_restore_file(inf) == FALSE)
	{
		endwin();
		printf("Cannot restore file\n");
		return(FALSE);
	}

#if ANDROID || METRO
	close(inf);
#else
#if defined(__CYGWIN__) || defined(__DJGPP__)
	close(inf);
#endif
	if (!wizard)
	{
#ifdef WIN32
		close(inf);
		if (_unlink(file) < 0)
		{
			printf("Cannot unlink file\n");
			return FALSE;
		}
#else
#ifndef __DJGPP__
			endwin();
			while((pid = fork()) < 0)
				sleep(1);

			/* set id to unlink file */
			if(pid == 0)
			{
#ifndef ANDROID
				setuid(playuid);
				setgid(playgid);
#endif
				unlink(file);
				exit(0);
			}
			/* wait for unlink to finish */
			else
			{
				while(wait(&ret_status) != pid)
					continue;
				if (ret_status < 0)
				{
					printf("Cannot unlink file\n");
					return FALSE;
				}
			}
#else
		if (unlink(file) < 0)
		{
			printf("Cannot unlink file\n");
			return FALSE;
		}
#endif
#endif
	}
#endif

#ifndef METRO
	environ = envp;
#endif

	strcpy(file_name, file);
	setup();
	restscr(cw);
	srand48(getpid());
	playit();

	return TRUE;
}
