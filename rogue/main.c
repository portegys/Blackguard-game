/*
 * Main
 * Exploring the dungeons of doom
 *
 * @(#) main.c	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <sys/stat.h>
#ifndef WIN32
#include <pwd.h>
#endif
#include "rogue.h"

#ifdef WIN32
#define srand48(seed)	srand(seed)
#define lrand48()	rand()
#define PATH_MAX 1024
#endif

#ifndef WIN32
#include <termios.h>
struct termios terminal;
#endif

#if ( ANDROID || METRO || NEW_GAME )
#include <setjmp.h>
jmp_buf newgame;
#endif

#ifdef BLACKGUARD
#ifndef METRO
#include <pthread.h>
extern pthread_mutex_t InputMutex;
#endif

void *rogue_main(void *arg)
{
        int argc;
        char **argv;
#ifndef METRO
        char **envp;
#endif
#else
int main(argc, argv, envp)
int argc;
char **argv;
char **envp;
{
#endif
	register char *env;
	register struct linked_list *item;
	register struct object *obj;
	struct passwd *pw;
	struct passwd *getpwuid();
	char alldone, wpt;
	char *xcrypt();
#ifndef ANDROID
    char *strrchr();
#endif
	char *getpass(char *);
	int lowtime;
	time_t now;
    char *roguehome();
	char *homedir = roguehome();
#ifdef METRO
	struct stat stbuf;
#ifdef NEVER
    WIN32_FILE_ATTRIBUTE_DATA attr;
#endif
#endif
#ifdef BLACKGUARD
	char buf[LINLEN];
        argc = Argc;
        argv = Argv;
#ifndef METRO
        envp = Envp;
#endif
#endif

#ifdef __DJGPP__
	_fmode = O_BINARY;
#endif

	if (homedir == NULL)
        	homedir = "";

#ifndef WIN32
	playuid = getuid();
#ifndef ANDROID
	if (setuid(playuid) < 0) {
		printf("Cannot change to effective uid: %d\n", playuid);
		exit(1);
	}
	playgid = getgid();
#endif
#endif

	/* check for print-score option */
	memset(scorefile, 0, LINLEN);
#ifdef BLACKGUARD
	strncpy(scorefile, homedir, LINLEN - (strlen("blackguard.scr") + 2));
#else
	strncpy(scorefile, homedir, LINLEN - (strlen("srogue.scr") + 2));
#endif

        if (*scorefile)
#ifdef WIN32
            strcat(scorefile,"\\");
#else
            strcat(scorefile,"/");
#endif
#ifdef BLACKGUARD
        strcat(scorefile, "blackguard.scr");
#else
        strcat(scorefile, "srogue.scr");
#endif

	if (argc >= 2 && strcmp(argv[1], "-s") == 0)
	{
		showtop(0);
		exit(0);
	}

	if (argc >= 2 && author() && strcmp(argv[1],"-a") == 0)
	{
		wizard = TRUE;
		argv++;
		argc--;
	}

	/* Check to see if he is a wizard */

	if (argc >= 2 && strcmp(argv[1], "-w") == 0)
	{
		if (strcmp(PASSWD, xcrypt(getpass(wizstr),"mT")) == 0)
		{
			wizard = TRUE;
			argv++;
			argc--;
		}
	}

	/* get home and options from environment */
	memset(home, 0, LINLEN);
	memset(file_name, 0, LINLEN);
#if ANDROID || METRO
    strncpy(home, LocalDir, LINLEN - (strlen("blackguard.sav") + 2));
#else
	if ((env = getenv("HOME")) != NULL)
#ifdef BLACKGUARD
		strncpy(home, env, LINLEN - (strlen("blackguard.sav") + 2));
#else
		strncpy(home, env, LINLEN - (strlen("srogue.sav") + 2));
#endif
#ifndef WIN32
	else if ((pw = getpwuid(playuid)) != NULL)
#ifdef BLACKGUARD
		strncpy(home, pw->pw_dir, LINLEN - (strlen("blackguard.sav") + 2));
#else
		strncpy(home, pw->pw_dir, LINLEN - (strlen("srogue.sav") + 2));
#endif
#endif
	else
		home[0] = '\0';

        if (strcmp(home,"/") == 0)
		home[0] = '\0';
#endif

#ifdef WIN32
        if ((strlen(home) > 0) && (home[strlen(home)-1] != '\\'))
		strcat(home, "\\");
#else
        if ((strlen(home) > 0) && (home[strlen(home)-1] != '/'))
		strcat(home, "/");
#endif

	strcpy(file_name, home);
#ifdef BLACKGUARD
	strcat(file_name, "blackguard.sav");
#else
	strcat(file_name, "srogue.sav");
#endif

#ifdef METRO
	if (whoami[0] == '\0')
	{
#else
#ifdef BLACKGUARD
	if ((env = getenv("BLACKGUARDOPTS")) != NULL)
#else
	if ((env = getenv("ROGUEOPTS")) != NULL)
#endif
		parse_opts(env);
	if (env == NULL || whoami[0] == '\0')
	{
#endif
#if ANDROID || METRO
        strcpy(whoami,"Rodney");
#else
#ifdef WIN32
		DWORD n = sizeof(whoami);
		if (!GetUserName(whoami, &n)) {
			printf("Say, who are you?\n");
			exit(1);
		}
#else
		if((pw = getpwuid(playuid)) == NULL)
		{
			printf("Say, who are you?\n");
			exit(1);
		}
		else
			strucpy(whoami, pw->pw_name, strlen(pw->pw_name));
#endif
#endif
	}

#ifdef METRO
	if (fruit[0] == '\0')
#else
	if (env == NULL || fruit[0] == '\0')
#endif
		strcpy(fruit, "juicy-fruit");

#if ANDROID || METRO
#ifdef ANDROID
    if (access(file_name, 4) != -1)
		if(!restore(file_name, envp)) /* NOTE: NEVER RETURNS */
			reset();
#else
	if (stat(file_name, &stbuf) == 0)
#ifdef NEVER
    if (GetFileAttributesExA(file_name, GetFileExInfoStandard, &attr) != 0)
#endif
		if(!restore(file_name)) /* NOTE: NEVER RETURNS */
			reset();
#endif
#else
	if (argc == 2)
		if(!restore(argv[1], envp)) /* NOTE: NEVER RETURNS */
			exit(1);
#endif

#if ( ANDROID || METRO || NEW_GAME )
        /* Jump here to start new game. */
        if(setjmp(newgame) != 0)
        {
            /* Release resources. */
            reset();
        }
#endif

	time(&now);
	lowtime = (int) now;

#ifdef METRO
	dnum = lowtime;
#else
	dnum = (wizard == TRUE && getenv("SEED") != NULL ?
		atoi(getenv("SEED")) : lowtime + getpid());
#endif

	seed = dnum;
	srand48(seed);			/* init rnd number gen */

	signal(SIGINT, byebye);		/* just in case */
#ifdef SIGQUIT
	signal(SIGQUIT ,byebye);
#endif

	init_everything();

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

	/* Set up windows */
	initscr();
	cw = newwin(0, 0, 0, 0);
	mw = newwin(0, 0, 0, 0);
	hw = newwin(0, 0, 0, 0);
#if METRO
	sw = newwin(0, 0, 0, 0);
#endif

#ifdef BLACKGUARD
	notifyOpenDungeon();
	if(wizard == TRUE)
		sprintf(buf, "Hello %s, welcome to dungeon #%d", whoami, dnum);
	else
		sprintf(buf, "Hello %s, One moment while I open the door to the dungeon...", whoami);
	displaymsg = buf;
	wclear(stdscr);
	wprintw(stdscr, "%s", buf);
	wrefresh(stdscr);
#ifdef WIN32
	Sleep(3000);
#else
	sleep(3);
#endif
	displaymsg = NULL;
#else
	if(wizard == TRUE)
		printf("Hello %s, welcome to dungeon #%d\n", whoami, dnum);
	else
		printf("Hello %s, One moment while I open the door to the dungeon...\n", whoami);
	fflush(stdout);
#endif

	if (strcmp(termname(),"dumb") == 0)
	{
		endwin();
		printf("ERROR in terminal parameters.\n");
		printf("Check TERM in environment.\n");
		byebye(1);
	}

	if (LINES < 24 || COLS < 80) {
		endwin();
		printf("ERROR: screen size too small\n");
		byebye(1);
	}

	if ((whoami == NULL) || (*whoami == '\0') || (strcmp(whoami,"dosuser")==0))
	{
		echo();
#ifdef BLACKGUARD
		mvaddstr(23,2,"Blackguard's Name? ");
#else
		mvaddstr(23,2,"Rogue's Name? ");
#endif
#if METRO
		wgetnstr(sw,whoami,MAXSTR);
#else
		wgetnstr(stdscr,whoami,MAXSTR);
#endif
		noecho();
	}

	if ((whoami == NULL) || (*whoami == '\0'))
		strcpy(whoami,"Rodney");
	
	setup();

	waswizard = wizard;

	/* Draw current level */

	new_level(NORMLEV);

	/* Start up daemons and fuses */

	do_daemon(status, TRUE, BEFORE);
	do_daemon(doctor, TRUE, BEFORE);
	do_daemon(stomach, TRUE, BEFORE);
	do_daemon(runners, TRUE, AFTER);
	fuse(swander, TRUE, WANDERTIME);

	/* Give the rogue his weaponry */

	do {
		wpt = pick_one(w_magic);
		switch (wpt)
		{
			case MACE:	case SWORD:	case TWOSWORD:
			case SPEAR:	case TRIDENT:	case SPETUM:
			case BARDICHE:	case PIKE:	case BASWORD:
			case HALBERD:
				alldone = TRUE;
			otherwise:
				alldone = FALSE;
		}
	} while(!alldone);

	item = new_thing(FALSE, WEAPON, wpt);
	obj = OBJPTR(item);
	obj->o_hplus = rnd(3);
	obj->o_dplus = rnd(3);
	obj->o_flags = ISKNOW;
	add_pack(item, TRUE);
	cur_weapon = obj;

	/* Now a bow */

	item = new_thing(FALSE, WEAPON, BOW);
	obj = OBJPTR(item);
	obj->o_hplus = rnd(3);
	obj->o_dplus = rnd(3);
	obj->o_flags = ISKNOW;
	add_pack(item, TRUE);

	/* Now some arrows */

	item = new_thing(FALSE, WEAPON, ARROW);
	obj = OBJPTR(item);
	obj->o_count = 25 + rnd(15);
	obj->o_hplus = rnd(2);
	obj->o_dplus = rnd(2);
	obj->o_flags = ISKNOW;
	add_pack(item, TRUE);

	/* And his suit of armor */

	wpt = pick_one(a_magic);
	item = new_thing(FALSE, ARMOR, wpt);
	obj = OBJPTR(item);
	obj->o_flags = ISKNOW;
	obj->o_ac = armors[wpt].a_class - rnd(4);
	cur_armor = obj;
	add_pack(item, TRUE);
	
	/* Give him some food */

	item = new_thing(FALSE, FOOD, 0);
	add_pack(item, TRUE);
	
#if ANDROID || METRO
	/* Some identify scrolls */

	item = new_thing(FALSE, SCROLL, S_IDENT);
	obj = OBJPTR(item);
	obj->o_count = 3;
	obj->o_flags = ISKNOW;
	add_pack(item, TRUE);
#endif

	playit();
	
	return 0;
}


/*
 * endit:
 *	Exit the program abnormally.
 */
void
endit(int a)
{
	fatal("Ok, if you want to exit that badly, I'll have to allow it");
}

/*
 * fatal:
 *	Exit the program, printing a message.
 */
void
fatal(s)
char *s;
{
	clear();
	refresh();
	endwin();
	fprintf(stderr,"%s\n\r",s);
	fflush(stderr);
	byebye(2);
}

/*
 * byebye:
 *	Exit here and reset the users terminal parameters
 *	to the way they were when he started
 */

void
byebye(how)
int how;
{
	if (!isendwin())
		endwin();

#ifdef ANDROID
       unlink(file_name);
#endif
#ifdef METRO
       _unlink(file_name);
#endif
#if ( METRO || NEW_GAME )
       /* Start new game. */
       longjmp(newgame, 4);
#endif

	exit(how);		/* exit like flag says */
}


/*
 * rnd:
 *	Pick a very random number.
 */
int
rnd(range)
int range;
{
	reg int wh;

	if (range == 0)
		wh = 0;
	else {
		wh = lrand48() % range;
		wh &= 0x7FFFFFFF;
	}
	return wh;
}

/*
 * roll:
 *	roll a number of dice
 */
int
roll(number, sides)
int number, sides;
{
	reg int dtotal = 0;

	while(number-- > 0)
		dtotal += rnd(sides)+1;
	return dtotal;
}


/*
** setup: 	Setup signal catching functions
*/
void
setup()
{
#ifdef SIGHUP
	signal(SIGHUP, auto_save);
#endif
	signal(SIGINT, auto_save);
#ifdef SIGQUIT
	signal(SIGQUIT,  byebye);
#endif
	signal(SIGILL, game_err);
#ifdef SIGTRAP
	signal(SIGTRAP, game_err);
#endif
#ifdef SIGIOT
	signal(SIGIOT, game_err);
#endif
#ifdef SIGEMT
	signal(SIGEMT, game_err);
#endif
	signal(SIGFPE, game_err);
#ifdef SIGBUS
	signal(SIGBUS, game_err);
#endif
	signal(SIGSEGV, game_err);
#ifdef SIGSYS
	signal(SIGSYS, game_err);
#endif
#ifdef SIGPIPE
	signal(SIGPIPE, game_err);
#endif
	signal(SIGTERM, game_err);


	cbreak();
	noecho();
}

/*
** playit:	The main loop of the program.  Loop until the game is over,
**		refreshing things and looking at the proper times.
*/
void
playit() {
	reg char *opts;

#ifndef WIN32
	tcgetattr(0, &terminal);
#endif

	/* parse environment declaration of options */
#ifdef BLACKGUARD
	if ((opts = getenv("BLACKGUARDOPTS")) != NULL)
#else
		if ((opts = getenv("ROGUEOPTS")) != NULL)
#endif
		parse_opts(opts);

	player.t_oldpos = hero;
	oldrp = roomin(&hero);
	nochange = FALSE;
	int checkpointcount = 0;
	while (playing) {
		command();        /* Command execution */
		checkpointcount++;
		if (checkpointcount >= CHECKPOINT_FREQ) {
			checkpointcount = 0;
			docheckpoint();
		}
	}
	endit(0);
}


/*
** author:	See if a user is an author of the program
*/
int
author()
{
#ifdef WIN32
	return FALSE;
#else
#if ANDROID || METRO
	return FALSE;
#else
	switch (playuid) {
		case 100:
		case 0:
			return TRUE;
		default:
			return FALSE;
	}
#endif
#endif
}

#ifdef WIN32
#ifndef METRO
int
directory_exists(char *dirname)
{
    const DWORD dwAttr = GetFileAttributes(dirname);
    if(dwAttr != 0xFFFFFFFF)
    {
        if((FILE_ATTRIBUTE_DIRECTORY & dwAttr) &&
           0 != strcmp(".", dirname) &&
           0 != strcmp("..", dirname))
        {
            return 1;
        }
    }

    return 0;
}
#endif
#else
int
directory_exists(char *dirname)
{
    struct stat sb;

    if (stat(dirname, &sb) == 0) /* path exists */
        return (S_ISDIR (sb.st_mode));

    return(0);
}
#endif

char *
roguehome()
{
#if ANDROID || METRO
    return LocalDir;
#else
    static char path[PATH_MAX];
    char *end,*home;

	memset(path, 0, PATH_MAX);
#ifdef BLACKGUARD
    if ( (home = getenv("BLACKGUARDHOME")) != NULL)
#else
    if ( (home = getenv("ROGUEHOME")) != NULL)
#endif
    {
        if (*home)
        {
            strncpy(path, home, PATH_MAX - 20);

            end = &path[strlen(path)-1];


            while( (end >= path) && ((*end == '/') || (*end == '\\')))
                *end-- = '\0';

            if (directory_exists(path))
                return(path);
        }
    }

#ifndef WIN32
    if (directory_exists("/var/games/roguelike"))
        return("/var/games/roguelike");
    if (directory_exists("/var/lib/roguelike"))
        return("/var/lib/roguelike");
    if (directory_exists("/var/roguelike"))
        return("/var/roguelike");
    if (directory_exists("/usr/games/lib"))
        return("/usr/games/lib");
    if (directory_exists("/games/roguelik"))
        return("/games/roguelik");
#endif

    return(NULL);
#endif
}

char *getpass(char *prompt)
{
        static char buf[128];
        size_t i;
        
        fputs(prompt, stderr);
        fflush(stderr);
        for (i = 0; i < sizeof(buf) - 1; i++) {
#ifdef WIN32
                buf[i] = _getch();
#else
                buf[i] = getch();
#endif
                if (buf[i] == '\r')
                        break;
        }
        buf[i] = 0;
        fputs("\n", stderr);
        return buf;
}

#if ( ANDROID || METRO || NEW_GAME )
/* Release resources. */
void
reset()
{
        register struct linked_list *item;
        register struct thing *tp;
        register int i;

        for (item = mlist; item != NULL; item = next(item)) 
        {
            tp = THINGPTR(item);
            free_list(tp->t_pack);
        }
        free_list(mlist);
        free_list(pack);
        free_list(lvl_obj);
        for (i = 0; i < MAXSCROLLS; i++)
        {
            if (s_guess[i] != NULL)
            {
                free(s_guess[i]);
                s_guess[i] = NULL;
            }
            if (s_names[i] != NULL)
            {
                free(s_names[i]);
                s_names[i] = NULL;
            }
        }
        for (i = 0; i < MAXPOTIONS; i++)
        {
            if (p_guess[i] != NULL)
            {
                free(p_guess[i]);
                p_guess[i] = NULL;
            }
        }
        for (i = 0; i < MAXRINGS; i++)
        {
            if (r_guess[i] != NULL)
            {
                free(r_guess[i]);
                r_guess[i] = NULL;
            }
        }
        for (i = 0; i < MAXSTICKS; i++)
        {
            if (ws_guess[i] != NULL)
            {
                free(ws_guess[i]);
                ws_guess[i] = NULL;
            }
        }
        for (i = 0; i < MAXSCROLLS; i++)
        {
            if (ws_guess[i] != NULL)
            {
                free(ws_guess[i]);
                ws_guess[i] = NULL;
            }
        }

		/* Reset player. */
		player.t_flags = 0;
		isblind = FALSE;
		purse = 0;
		wizard = FALSE;
		waswizard = FALSE;
        
        /* Stop daemons. */
        demoncnt = 0;
        
        /* Restore probabilities in global.c */
        for (i = 0; i <= NUMTHINGS; i++)
        {
            things[i].mi_prob = things[i].mi_prob_base;
        }
        for (i = 0; i <= MAXARMORS; i++)
        {
            a_magic[i].mi_prob = a_magic[i].mi_prob_base;
        }
        for (i = 0; i <= MAXWEAPONS; i++)
        {
            w_magic[i].mi_prob = w_magic[i].mi_prob_base;
        }
        for (i = 0; i <= MAXSCROLLS; i++)
        {
            s_magic[i].mi_prob = s_magic[i].mi_prob_base;
        }
        for (i = 0; i <= MAXPOTIONS; i++)
        {
            p_magic[i].mi_prob = p_magic[i].mi_prob_base;
        }
        for (i = 0; i <= MAXRINGS; i++)
        {
            r_magic[i].mi_prob = r_magic[i].mi_prob_base;
        }
        for (i = 0; i <= MAXSTICKS; i++)
        {
            ws_magic[i].mi_prob = ws_magic[i].mi_prob_base;
        }
}
#endif


