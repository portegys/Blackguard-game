/*
 * File for the fun, ends in death or a total win
 *
 * @(#) rip.c	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#else
#include <pwd.h>
#endif
#include <fcntl.h>
#include "rogue.h"

static char scoreline[100];

static char *rip[] = {
"                          ____________________",
"                         /                    \\",
"                        /     Blackguards      \\",
"                       /       Graveyard        \\",
"                      /                          \\",
"                     /       REST IN PEACE        \\",
"                    /                              \\",
"                    |                              |",
"                    |                              |",
"                    |        Destroyed by a        |",
"                    |                              |",
"                    |                              |",
"                    |                              |",
"                    |                              |",
"                    |                              |",
"                    |                              |",
"                    |                              |",
"                   *|     *     *     *     *      |*",
"            _______)\\\\//\\\\//\\)//\\\\//)/\\\\//\\\\)/\\\\//\\\\)/\\\\//\\\\//(________",
};

#define RIP_LINES (sizeof rip / (sizeof (char *)))

char	*killname();

/*
 * death:
 *	Do something really fun when he dies
 */

#include <time.h>
void
death(monst)
char monst;
{
	reg char dp, *killer;
	struct tm *lt;
	time_t date;
	char buf[LINLEN];
	struct tm *localtime();

#ifdef BLACKGUARD
	notifyDie();
#endif
	time(&date);
	lt = localtime(&date);
	clear();
	move(3, 0);
	for (dp = 0; dp < RIP_LINES; dp++)
		printw("%s\n", rip[dp]);
	mvaddstr(10, 36 - ((strlen(whoami) + 1) / 2), whoami);
	killer = killname(monst);
	mvaddstr(12, 43, vowelstr(killer));
	mvaddstr(14, 36 - ((strlen(killer) + 1) / 2), killer);
	purse -= purse/10;
	sprintf(buf, "%d Gold Pieces", purse);
	mvaddstr(16, 36 - ((strlen(buf) + 1) / 2), buf);
	sprintf(prbuf, "%d/%d/%d", lt->tm_mon + 1, lt->tm_mday, 1900+lt->tm_year);
	mvaddstr(18, 32, prbuf);
	move(LINES-1, 0);
	refresh();
	score(purse, KILLED, monst);
	byebye(0);
}

/*
 * top ten entry structure
 */
static struct sc_ent {
	int sc_score;			/* gold */
	char sc_name[LINLEN];		/* players name */
	int sc_flags;			/* reason for being here */
	int sc_level;			/* dungeon level */
#ifndef WIN32
	int sc_uid;			/* user ID */
#endif
	unsigned char sc_monster;       /* killer */
	int sc_explvl;			/* experience level */
	int sc_exppts;		/* experience points */
	time_t sc_date;			/* time this score was posted */
} top_ten[10];

char *reason[] = {
	"Killed",
	"Chickened out",
	"A Total Winner"
};
int oldpurse;

/*
 * score:
 *	Figure score and post it.
 */
void
score(amount, aflag, monst)
char monst;
int amount, aflag;
{
	reg struct sc_ent *scp, *sc2;
	reg int i, fd, prflags = 0;
	reg FILE *outf;
	char *packend;

	signal(SIGINT, byebye);
#ifdef SIGQUIT
	signal(SIGQUIT, byebye);
#endif
	if (aflag != WINNER) {
		if (aflag == CHICKEN)
			packend = "when you chickened out";
		else
			packend = "at your untimely demise";
#if ANDROID || METRO
		mvaddstr(LINES - 1, 0, spacemsg);
		refresh();
#ifdef ANDROID
		wait_for(stdscr, ' ');
#else
		wait_for(sw, ' ');
#endif
#else
		mvaddstr(LINES - 1, 0, retstr);
		refresh();
		wgetnstr(stdscr,prbuf,80);
#endif
		oldpurse = purse;
		showpack(FALSE, packend);
	}
	/*
	 * Open file and read list
	 */
#ifdef WIN32
	if ((fd = open(scorefile, O_RDWR | O_CREAT | O_BINARY, 0666)) < 0)
#else
	if ((fd = open(scorefile, O_RDWR | O_CREAT, 0666)) < 0)
#endif
		return;
#ifdef WIN32
	outf = (FILE *) _fdopen(fd, "w");
#else
	outf = (FILE *) fdopen(fd, "w");
#endif
	for (scp = top_ten; scp <= &top_ten[9]; scp++) {
		scp->sc_score = 0;
		for (i = 0; i < 80; i++)
			scp->sc_name[i] = rnd(255);
		scp->sc_flags = rnd(255);
		scp->sc_level = rnd(255);
		scp->sc_monster = rnd(255);
#ifndef WIN32
		scp->sc_uid = rnd(255);
#endif
		scp->sc_date = rnd(255);
	}
#if ANDROID || METRO
	mvaddstr(LINES - 1, 0, spacemsg);
        refresh();
#ifdef ANDROID
        wait_for(stdscr, ' ');
#else
        wait_for(sw, ' ');
#endif
#else
	mvaddstr(LINES - 1, 0, retstr);
	refresh();
	wgetnstr(stdscr,prbuf,80);
#endif
	if (author() || wizard)
		if (strcmp(prbuf, "names") == 0)
			prflags = 1;
        for(i = 0; i < 10; i++)
        {
            unsigned int mon;

            encread((char *) &top_ten[i].sc_name, LINLEN, fd);
            encread((char *) scoreline, 100, fd);
#ifdef WIN32
            sscanf(scoreline, " %d %d %d %u %d %d %lx \n",
                &top_ten[i].sc_score,   &top_ten[i].sc_flags,
                &top_ten[i].sc_level,
                &mon,                   &top_ten[i].sc_explvl,
                &top_ten[i].sc_exppts,  &top_ten[i].sc_date);
#else
            sscanf(scoreline, " %d %d %d %d %u %d %d %lx \n",
                &top_ten[i].sc_score,   &top_ten[i].sc_flags,
                &top_ten[i].sc_level,   &top_ten[i].sc_uid,
                &mon,                   &top_ten[i].sc_explvl,
                &top_ten[i].sc_exppts,  &top_ten[i].sc_date);
#endif
            top_ten[i].sc_monster = mon;
        }
	/*
	 * Insert it in list if need be
	 */
	if (!waswizard) {
		for (scp = top_ten; scp <= &top_ten[9]; scp++)
			if (amount > scp->sc_score)
				break;
			if (scp <= &top_ten[9]) {
				for (sc2 = &top_ten[9]; sc2 > scp; sc2--)
					*sc2 = *(sc2-1);
				scp->sc_score = amount;
				strcpy(scp->sc_name, whoami);
				scp->sc_flags = aflag;
				if (aflag == WINNER)
					scp->sc_level = max_level;
				else
					scp->sc_level = level;
				scp->sc_monster = monst;
#ifndef WIN32
				scp->sc_uid = playuid;
#endif
				scp->sc_explvl = him->s_lvl;
				scp->sc_exppts = him->s_exp;
				time(&scp->sc_date);
		}
	}
	ignore();
	fseek(outf, 0L, 0);
        for(i = 0; i < 10; i++)
        {
            memset(scoreline,0,100);
            encwrite((char *) top_ten[i].sc_name, LINLEN, outf);
#ifdef WIN32
            sprintf(scoreline, " %d %d %d %u %d %d %lx \n",
                top_ten[i].sc_score, top_ten[i].sc_flags,
                top_ten[i].sc_level,
                top_ten[i].sc_monster, top_ten[i].sc_explvl,
                top_ten[i].sc_exppts, top_ten[i].sc_date);
#else
            sprintf(scoreline, " %d %d %d %d %u %d %d %lx \n",
                top_ten[i].sc_score, top_ten[i].sc_flags,
                top_ten[i].sc_level, top_ten[i].sc_uid,
                top_ten[i].sc_monster, top_ten[i].sc_explvl,
                top_ten[i].sc_exppts, top_ten[i].sc_date);
#endif
            encwrite((char *) scoreline, 100, outf);
        }
	fclose(outf);
	signal(SIGINT, byebye);
#ifdef SIGQUIT
	signal(SIGQUIT, byebye);
#endif
	clear();
	refresh();
#ifdef BLACKGUARD
	showtop(prflags);		/* print top ten list */
#if ANDROID || METRO
	mvaddstr(LINES - 1, 0, spacemsg);
        refresh();
#ifdef ANDROID
        wait_for(stdscr, ' ');
#else
        wait_for(sw, ' ');
#endif
#else
	mvaddstr(LINES - 1, 0, retstr);
	refresh();
	wgetnstr(stdscr,prbuf,80);
#endif
	clear();
	refresh();
	endwin();
#else
	endwin();
	showtop(prflags);		/* print top ten list */
#endif
}

/*
 * showtop:
 *	Display the top ten on the screen
 */
int
showtop(showname)
int showname;
{
	reg int fd, i;
	char *killer;
	struct sc_ent *scp;
#ifdef BLACKGUARD
	char buf[LINLEN];
#endif

#ifdef WIN32
	if ((fd = open(scorefile, O_RDONLY | O_BINARY)) < 0)
#else
	if ((fd = open(scorefile, O_RDONLY)) < 0)
#endif
		return FALSE;
       
        for(i = 0; i < 10; i++)
        {
            unsigned int mon;
            encread((char *) &top_ten[i].sc_name, LINLEN, fd);
            encread((char *) scoreline, 100, fd);
#ifdef WIN32
            sscanf(scoreline, " %d %d %d %u %d %d %lx \n",
                &top_ten[i].sc_score,   &top_ten[i].sc_flags,
                &top_ten[i].sc_level,
                &mon,                   &top_ten[i].sc_explvl,
                &top_ten[i].sc_exppts,  &top_ten[i].sc_date);
#else
            sscanf(scoreline, " %d %d %d %d %u %d %d %lx \n",
                &top_ten[i].sc_score,   &top_ten[i].sc_flags,
                &top_ten[i].sc_level,   &top_ten[i].sc_uid,
                &mon,                   &top_ten[i].sc_explvl,
                &top_ten[i].sc_exppts,  &top_ten[i].sc_date);
#endif
            top_ten[i].sc_monster = mon;
        }
	close(fd);
#ifdef BLACKGUARD
	clear();
	sprintf(buf,"Top Ten Adventurers:\nRank\tScore\tName\n");
	addstr(buf);
#else
	printf("Top Ten Adventurers:\nRank\tScore\tName\n");
#endif
	for (scp = top_ten; scp <= &top_ten[9]; scp++) {
		if (scp->sc_score > 0) {
#ifdef BLACKGUARD
			sprintf(buf,"%d\t%d\t%s: %s\t\t--> %s on level %d",
			  scp - top_ten + 1, scp->sc_score, scp->sc_name,
			  ctime(&scp->sc_date), reason[scp->sc_flags],
			  scp->sc_level);
			addstr(buf);
#else
			printf("%d\t%d\t%s: %s\t\t--> %s on level %d",
			  scp - top_ten + 1, scp->sc_score, scp->sc_name,
			  ctime(&scp->sc_date), reason[scp->sc_flags],
			  scp->sc_level);
#endif
			if (scp->sc_flags == KILLED) {
				killer = killname(scp->sc_monster);
#ifdef BLACKGUARD
				sprintf(buf, " by a%s %s",vowelstr(killer), killer);
				addstr(buf);
#else
				printf(" by a%s %s",vowelstr(killer), killer);
#endif
			}
#ifdef BLACKGUARD
			sprintf(buf," [Exp: %d/%d]",scp->sc_explvl,scp->sc_exppts);
			addstr(buf);
#else
			printf(" [Exp: %d/%d]",scp->sc_explvl,scp->sc_exppts);
#endif
			if (showname) {
#ifdef WIN32
#ifdef BLACKGUARD
				addstr("\n");
#else
				printf("\n");
#endif
#else
				struct passwd *pp, *getpwuid();

#ifdef BLACKGUARD
				if ((pp = getpwuid(scp->sc_uid)) == NULL)
					sprintf(buf," (%d)\n", scp->sc_uid);
				else
					sprintf(buf," (%s)\n", pp->pw_name);
				addstr(buf);
#else
				if ((pp = getpwuid(scp->sc_uid)) == NULL)
					printf(" (%d)\n", scp->sc_uid);
				else
					printf(" (%s)\n", pp->pw_name);
#endif
#endif
			}
			else
#ifdef BLACKGUARD
				addstr("\n");
#else
				printf("\n");
#endif
		}
	}
	return TRUE;
}

#if ANDROID || METRO
/*
 * showscores:
 *	Show score list.
 */
int
showscores()
{
	reg int fd, i, cnt;
	char *killer;
	struct sc_ent *scp,*scp2;

	if ((fd = open(scorefile, O_RDONLY)) < 0)
		return FALSE;

        for(i = 0; i < 10; i++)
        {
            unsigned int mon;
            encread((char *) &top_ten[i].sc_name, LINLEN, fd);
            encread((char *) scoreline, 100, fd);
#ifdef WIN32
            sscanf(scoreline, " %d %d %d %u %d %d %lx \n",
                &top_ten[i].sc_score,   &top_ten[i].sc_flags,
                &top_ten[i].sc_level,
                &mon,                   &top_ten[i].sc_explvl,
                &top_ten[i].sc_exppts,  &top_ten[i].sc_date);
#else
            sscanf(scoreline, " %d %d %d %d %u %d %d %lx \n",
                &top_ten[i].sc_score,   &top_ten[i].sc_flags,
                &top_ten[i].sc_level,   &top_ten[i].sc_uid,
                &mon,                   &top_ten[i].sc_explvl,
                &top_ten[i].sc_exppts,  &top_ten[i].sc_date);
#endif
            top_ten[i].sc_monster = mon;
        }
	close(fd);

	wclear(hw);
	wprintw(hw,"Top Ten Adventurers:\nRank\tScore\tName\n");
	cnt = 0;
	for (scp = top_ten; scp <= &top_ten[9]; scp++) {
		if (scp->sc_score > 0) {
			wprintw(hw,"%d\t%d\t%s: %s\t\t--> %s on level %d",
			  scp - top_ten + 1, scp->sc_score, scp->sc_name,
			  ctime(&scp->sc_date), reason[scp->sc_flags],
			  scp->sc_level);
			if (scp->sc_flags == KILLED) {
				killer = killname(scp->sc_monster);
				wprintw(hw, " by a%s %s",vowelstr(killer), killer);
			}
			wprintw(hw," [Exp: %d/%d]\n",scp->sc_explvl,scp->sc_exppts);
        		if (++cnt >= (LINEZ / 2) && scp < &top_ten[9])
                        {
                            scp2 = scp; scp2++;
                            if (scp2->sc_score > 0) {
        			dbotline(hw, morestr);
        			cnt = 0;
        			wclear(hw);
                            }
        		}
		}
	}
	dbotline(hw,spacemsg);
	restscr(cw);
	return TRUE;
}
#endif

/*
 * total_winner:
 *	The hero made it back out alive
 */
void
total_winner()
{
#ifdef BLACKGUARD
	notifyWinner();
#endif
	clear();
#ifdef METRO
addstr("                                                               \n");
addstr("  ;   ;               ;   ;           ;          ;;;  ;     ;  \n");
addstr("  ;   ;               ;; ;;           ;           ;   ;     ;  \n");
addstr("  ;   ;  ;;;  ;   ;   ; ; ;  ;;;   ;;;;  ;;;      ;  ;;;    ;  \n");
addstr("   ;;;; ;   ; ;   ;   ;   ;     ; ;   ; ;   ;     ;   ;     ;  \n");
addstr("      ; ;   ; ;   ;   ;   ;  ;;;; ;   ; ;;;;;     ;   ;     ;  \n");
addstr("  ;   ; ;   ; ;  ;;   ;   ; ;   ; ;   ; ;         ;   ;  ;     \n");
addstr("   ;;;   ;;;   ;; ;   ;   ;  ;;;;  ;;;;  ;;;     ;;;   ;;   ;  \n");
addstr("                                                               \n");
#else
addstr("                                                               \n");
addstr("  @   @               @   @           @          @@@  @     @  \n");
addstr("  @   @               @@ @@           @           @   @     @  \n");
addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
addstr("                                                               \n");
#endif
addstr("     Congratulations, you have made it to the light of day!    \n");
addstr("\nYou have joined the elite ranks of those who have escaped the\n");
addstr("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
addstr("a great profit and are admitted to the fighters guild.\n");

	mvaddstr(LINES - 1, 0,spacemsg);
	refresh();
#if METRO
	wait_for(sw, ' ');
#else
	wait_for(stdscr, ' ');
#endif
	clear();
	oldpurse = purse;
	showpack(TRUE, NULL);
	score(purse, WINNER, 0);
	byebye(0);
}

/*
 * showpack:
 *	Display the contents of the hero's pack
 */
void
showpack(winner, howso)
BOOL winner;
char *howso;
{
	reg char *iname;
	reg int cnt, worth, ch;
	reg struct linked_list *item;
	reg struct object *obj;

	idenpack();
	cnt = 1;
	clear();
	if (winner)
		mvaddstr(0, 0, "   Worth  Item");
	else
		mvprintw(0, 0, "Contents of your pack %s:\n",howso);
	ch = 'a';
	for (item = pack; item != NULL; item = next(item)) {
		obj = OBJPTR(item);
		iname = inv_name(obj, FALSE);
		if (winner) {
			worth = get_worth(obj);
			worth *= obj->o_count;
			mvprintw(cnt, 0, "  %6d  %s",worth,iname);
			purse += worth;
		}
		else {
			mvprintw(cnt, 0, "%c) %s\n",ch,iname);
			ch = npch(ch);
		}
#if ANDROID || METRO
		if (++cnt >= LINEZ && next(item) != NULL) {
#else
		if (++cnt >= LINES - 2 && next(item) != NULL) {
#endif
			cnt = 1;
			mvaddstr(LINES - 1, 0, morestr);
			refresh();
#if METRO
			wait_for(sw, ' ');
#else
			wait_for(stdscr, ' ');
#endif
			clear();
		}
	}
	mvprintw(cnt + 1,0,"--- %d  Gold Pieces ---",oldpurse);
	refresh();
}

/*
 * killname:
 *	Returns what the hero was killed by.
 */
char *
killname(monst)
unsigned char monst;
{
	if (monst < MAXMONS + 1)
		return monsters[monst].m_name;
	else		/* things other than monsters */
		switch (monst) {
			case K_ARROW:	return "crooked arrow";
			case K_DART:	return "sharp dart";
			case K_BOLT:	return "jagged bolt";
			case K_POOL:	return "magic pool";
			case K_ROD:	return "exploding rod";
			case K_SCROLL:	return "burning scroll";
			case K_STONE: 	return "transmogrification to stone";
			case K_STARVE:	return "starvation";
	}
	return "misfortune";
}
