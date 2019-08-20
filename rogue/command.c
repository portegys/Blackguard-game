/*
 * Read and execute the user commands
 *
 * @(#) command.c	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <ctype.h>
#include <signal.h>
#include <limits.h>
#include "rogue.h"
#ifdef __DJGPP__
#include <process.h>
#endif

/*
 * command:
 *	Process the user commands
 */
void
command()
{
	reg char ch;
	reg int ntimes = 1;		/* Number of player moves */
	static char countch, direction, newcount = FALSE;

	if (pl_on(ISHASTE))
		ntimes++;
	/*
	 * Let the daemons start up
	 */
	while (ntimes-- > 0) {
		do_daemons(BEFORE);
		look(TRUE);
		if (!running)
			door_stop = FALSE;
		lastscore = purse;
		wmove(cw, hero.y, hero.x);
		if (!(running || count))
			draw(cw);			/* Draw screen */
		take = 0;
		after = TRUE;
		/*
		 * Read command or continue run
		 */
		if (wizard)
			waswizard = TRUE;
		if (player.t_nocmd <= 0) {
			player.t_nocmd = 0;
			if (running)
				ch = runch;
			else if (count)
				ch = countch;
			else {
				ch = readchar();
				if (mpos != 0 && !running) /* Erase message if its there */
					msg("");
			}
		}
		else
			ch = '.';
		if (player.t_nocmd > 0) {
			if (--player.t_nocmd <= 0)
				msg("You can move again.");	
		}
		else {
			/*
			 * check for prefixes
			 */
			if (isdigit(ch)) {
				count = 0;
				newcount = TRUE;
				while (isdigit(ch)) {
					count = count * 10 + (ch - '0');
					ch = readchar();
				}
				countch = ch;
				/*
				 * turn off count for commands which don't make sense
				 * to repeat
				 */
				switch (ch) {
					case 'h': case 'j': case 'k': case 'l':
					case 'y': case 'u': case 'b': case 'n':
					case 'H': case 'J': case 'K': case 'L':
					case 'Y': case 'U': case 'B': case 'N':
					case 'q': case 'r': case 's': case 'f':
					case 't': case 'C': case 'I': case '.':
#ifdef BLACKGUARD
					case 'z':
#else
					case 'z': case 'p':
#endif
						break;
					default:
						count = 0;
				}
			}
			switch (ch) {
			case 'f':
			case 'g':
				if (pl_off(ISBLIND)) {
					door_stop = TRUE;
					firstmove = TRUE;
				}
				if (count && !newcount)
					ch = direction;
				else
#ifdef BLACKGUARD
                    ch = 'k';
#else
					ch = readchar();
#endif
				switch (ch) {
					case 'h': case 'j': case 'k': case 'l':
					case 'y': case 'u': case 'b': case 'n':
						ch = toupper(ch);
				}
				direction = ch;
			}
			newcount = FALSE;
			/*
			 * execute a command
			 */
			if (count && !running)
				count--;
			switch (ch) {
				when 'h' : do_move(0, -1);
				when 'j' : do_move(1, 0);
				when 'k' : do_move(-1, 0);
				when 'l' : do_move(0, 1);
				when 'y' : do_move(-1, -1);
				when 'u' : do_move(-1, 1);
				when 'b' : do_move(1, -1);
				when 'n' : do_move(1, 1);
				when 'H' : do_run('h');
				when 'J' : do_run('j');
				when 'K' : do_run('k');
				when 'L' : do_run('l');
				when 'Y' : do_run('y');
				when 'U' : do_run('u');
				when 'B' : do_run('b');
				when 'N' : do_run('n');
				when 't' :
#ifdef BLACKGUARD
                        delta.y = -1; delta.x = 0;
                        add_dir(&delta.x, &delta.y);
                        missile(delta.y, delta.x);
#else
					if (!get_dir())
						after = FALSE;
					else
						missile(delta.y, delta.x);
#endif
				when 'Q' : after = FALSE; quit(-1);
				when 'i' : after = FALSE; inventory(pack, 0);
				when 'I' : after = FALSE; picky_inven();
				when 'd' : drop(NULL);
				when 'q' : quaff();
				when 'r' : read_scroll();
				when 'e' : eat();
				when 'w' : wield();
				when 'W' : wear();
				when 'T' : take_off();
				when 'P' : ring_on();
				when 'R' : ring_off();
				when 'O' : option();
				when 'c' : call();
				when '>' : after = FALSE; d_level();
				when '<' : after = FALSE; u_level();
				when '?' : after = FALSE; help();
				when '/' : after = FALSE; identify(0);
				when 's' : search();
#ifdef BLACKGUARD
				when 'z' :
                        delta.y = -1; delta.x = 0;
                        add_dir(&delta.x, &delta.y);
                        do_zap(TRUE);
#else
				when 'z' : do_zap(FALSE);
				when 'p':
					if (get_dir())
						do_zap(TRUE);
					else
						after = FALSE;
#endif
#ifdef BLACKGUARD
				when 'v': after = FALSE; msg("Blackguard version %s.",release);
#else
				when 'v': after = FALSE; msg("Super Rogue version %s.",release);
#endif
				when 'D': dip_it();
#if !ANDROID && !METRO
				when CTRL('L') : after = FALSE; restscr(cw);
				when CTRL('R') : after = FALSE; msg(huh);
#endif
				when 'a': after = FALSE; dispmax();
#if ANDROID || METRO
				when '@': after = FALSE; msg("Identity UUID: %s",ID);
#else
				when '@' : after = FALSE; if (author() || wizard)
					msg("Hero @ %d,%d : Stairs @ %d,%d",hero.y,hero.x,stairs.y,stairs.x);
#endif
#if ANDROID || METRO
				when 'S' : after = FALSE; showscores();
#else
				when 'S' : 
					after = FALSE;
					if (save_game()) {
						wclear(cw);
						draw(cw);
						endwin();
						byebye(0);
					}
#endif
				when '.' : ;				/* Rest command */
				when ' ' : after = FALSE;	/* do nothing */
				when '=' :
					if (author() || wizard) {
						activity();
						after = FALSE;
					}
#ifdef BLACKGUARD
				when 'M':
				     after = FALSE;
                                     if (mute)
                                     {
                                         mute = 0;
                                         msg("Mute off");
                                     } else {
                                         mute = 1;
                                         msg("Mute on");
                                     }
#endif
#if ANDROID
				when 'm':
				     after = FALSE;
				     displaymsg = "View manual";
#endif
#if ANDROID || METRO
				when 'X' :
#else
				when CTRL('P') :
#endif
					after = FALSE;
					if (wizard) {
						wizard = FALSE;
						msg("Not wizard any more");
					}
					else {
#if ANDROID || METRO
						wizard = passwd(cw);
#else
						wizard = passwd();
#endif
						if (wizard) {
							msg("Hail the mighty wizard!!!!!");
							waswizard = TRUE;
						}
						else
							msg("Sorry");
					}
				when ESCAPE :	/* Escape */
					door_stop = FALSE;
					count = 0;
					after = FALSE;
				when '#':
					if (levtype == POSTLEV)		/* buy something */
						buy_it();
					after = FALSE;
				when '$':
					if (levtype == POSTLEV)		/* price something */
						price_it();
					after = FALSE;
				when '%':
					if (levtype == POSTLEV)		/* sell something */
						sell_it();
					after = FALSE;
				otherwise :
					after = FALSE;
					if (wizard) switch (ch) {
#if ANDROID || METRO
					when '~'     :	create_obj(FALSE);
					when '!' :	inventory(lvl_obj, 1);
					when '^' :	whatis(NULL);
					when '&' :	level++; new_level(NORMLEV);
					when '*' :	if (level > 1) level--; new_level(NORMLEV);
					when '(' :	displevl();
					when ')' :	dispmons();
					when '{' :	teleport(rndspot,&player);
					when '}' :	msg("food left: %d", food_left);
					when '[' :	add_pass();
					when ']' : {
#else
					case CTRL('A') : ;
					when 'C'     :	create_obj(FALSE);
					when CTRL('I') :	inventory(lvl_obj, 1);
					when CTRL('W') :	whatis(NULL);
					when CTRL('D') :	level++; new_level(NORMLEV);
					when CTRL('U') :	if (level > 1) level--; new_level(NORMLEV);
					when CTRL('F') :	displevl();
					when CTRL('X') :	dispmons();
					when CTRL('T') :	teleport(rndspot,&player);
					when CTRL('E') :	msg("food left: %d", food_left);
					when CTRL('O') :	add_pass();
					when 'M' : {
#endif
						int tlev, whichlev;
						prbuf[0] = '\0';
						msg("Which level? ");
						if (get_str(prbuf,cw) == NORM) {
							whichlev = NORMLEV;
							tlev = atoi(prbuf);
							if (tlev < 1)
								level = 1;
							if (tlev >= 200) {
								tlev -= 199;
								whichlev = MAZELEV;
							}
							else if (tlev >= 100) {
								tlev -= 99;
								whichlev = POSTLEV;
							}
							level = tlev;
							new_level(whichlev);
						}
					}
#if ANDROID || METRO
					when '"' : {
#else
					when CTRL('N') : {
#endif
						struct linked_list *item;

						item = get_item("charge", STICK);
						if (item != NULL) {
							(OBJPTR(item))->o_charges = 10000;
							msg("");
						}
					}
#if ANDROID || METRO
					when '+' : {
#else
					when CTRL('H') : {
#endif
						int i;
						struct linked_list *item;
						struct object *obj;

						him->s_exp = e_levels[him->s_lvl + 7] + 1;
						check_level();
						/*
						 * Give the rogue a very good sword
						 */
						item = new_thing(FALSE, WEAPON, TWOSWORD);
						obj = OBJPTR(item);
						obj->o_hplus = 3;
						obj->o_dplus = 3;
						obj->o_flags = ISKNOW;
						i = add_pack(item, TRUE);
						if (i)
							cur_weapon = obj;
						else
							discard(item);
						/*
						 * And his suit of armor
						 */
						item = new_thing(FALSE, ARMOR, PLATEARMOR);
						obj = OBJPTR(item);
						obj->o_ac = -8;
						obj->o_flags = ISKNOW;
						i = add_pack(item, TRUE);
						if (i)
							cur_armor = obj;
						else
							discard(item);
						nochange = FALSE;
					}
					otherwise:
#ifdef METRO
						if (ch != '\n' && ch != '\r')
#endif
						msg(illegal, unctrl(ch));
						count = 0;
				}
				else {
#ifdef METRO
					if (ch != '\n' && ch != '\r')
#endif
					msg(illegal, unctrl(ch));
					count = 0;
				}
			}
			/*
			 * turn off flags if no longer needed
			 */
			if (!running)
				door_stop = FALSE;
		}
		/*
		 * If he ran into something to take, let the
		 * hero pick it up if not in a trading post.
		 */
		if (take != 0 && levtype != POSTLEV)
			pick_up(take);
		if (!running)
			door_stop = FALSE;
	}
	/*
	 * Kick off the rest if the daemons and fuses
	 */
	if (after) {
		int  j;

		look(FALSE);
		do_daemons(AFTER);
		do_fuses();
		if (pl_on(ISSLOW))
			waste_time();
		for (j = LEFT; j <= RIGHT; j++) {
			if (cur_ring[j] != NULL) {
				if (cur_ring[j]->o_which == R_SEARCH)
					search();
				else if (cur_ring[j]->o_which == R_TELEPORT)
					if (rnd(100) < 5)
						teleport(rndspot, &player);
	 		}
		}
	}
}


/*
 * quit:
 *	Have player make certain, then exit.
 */
void
quit(int a)
{
	reg char ch, good;
	/*
	 * Reset the signal in case we got here via an interrupt
	 */
	if (signal(SIGINT, quit) != quit)
		mpos = 0;
#if ANDROID || METRO
	msg("Really quit? [y/n]");
#else
	msg("Really quit? [y/n/s]");
#endif
/*	ch = tolower(readchar());*/
	ch = readchar();
	if (ch == 'y') {
		clear();
		move(LINES-1, 0);
		refresh();
		score(purse, CHICKEN, 0);
		byebye(0);
#if !ANDROID && !METRO
	}
	else if (ch == 's') {
		good = save_game();
		if (good) {
			wclear(cw);
			draw(cw);
			endwin();
			byebye(0);
		}
#endif
	}
	else {
		signal(SIGINT, quit);
		wmove(cw, 0, 0);
		wclrtoeol(cw);
		draw(cw);
		mpos = 0;
		count = 0;
		nochange = FALSE;
	}
}

/*
 * search:
 *	Player gropes about him to find hidden things.
 */
void
search()
{
	reg int x, y;
	reg char ch;

	/*
	 * Look all around the hero, if there is something hidden there,
	 * give him a chance to find it.  If its found, display it.
	 */
	if (pl_on(ISBLIND))
		return;
	for (x = hero.x - 1; x <= hero.x + 1; x++) {
		for (y = hero.y - 1; y <= hero.y + 1; y++) {
			ch = winat(y, x);
			if (isatrap(ch)) {		/* see if its a trap */
				reg struct trap *tp;

				if ((tp = trap_at(y, x)) == NULL)
					break;
				if (tp->tr_flags & ISFOUND)
					break;		/* no message if its seen */
				if (mvwinch(cw, y, x) == ch)
					break;
				if (rnd(100) > (him->s_lvl * 9 + herowis() * 5))
					 break;
				tp->tr_flags |= ISFOUND;
				mvwaddch(cw, y, x, tp->tr_type);
				count = 0;
				running = FALSE;
				msg(tr_name(tp->tr_type));
			}
			else if(ch == SECRETDOOR) {
				if (rnd(100) < (him->s_lvl * 4 + herowis() * 5)) {
					mvaddch(y, x, DOOR);
					count = 0;
				}
			}
		}
	}
}

/*
 * help:
 *	Give single character help, or the whole mess if he wants it
 */
void
help()
{
	extern struct h_list helpstr[];
	reg struct h_list *strp;
	reg char helpch;
	reg int cnt;

	strp = &helpstr[0];
	msg("Character you want help for (* for all): ");
	helpch = readchar();
	mpos = 0;
	/*
	 * If its not a *, print the right help string
	 * or an error if he typed a funny character.
	 */
	if (helpch != '*') {
		wmove(cw, 0, 0);
		while (strp->h_ch) {
			if (strp->h_ch == helpch) {
				msg("%s%s", unctrl(strp->h_ch), strp->h_desc);
				break;
			}
			strp++;
		}
		if (strp->h_ch != helpch)
			msg("Unknown character '%s'", unctrl(helpch));
		return;
	}
	/*
	 * Here we print help for everything.
	 * Then wait before we return to command mode
	 */
	wclear(hw);
	cnt = 0;
	while (strp->h_ch) {
#if ANDROID
		mvwaddstr(hw, cnt, 0, unctrl(strp->h_ch));
#elif METRO
		if (strp->h_ch == '@')
		{
		    mvwaddch(hw, cnt, 0, ';');
		} else {
		    mvwaddstr(hw, cnt, 0, unctrl(strp->h_ch));
		}
#else
		mvwaddstr(hw, cnt % 23, cnt > 22 ? 40 : 0, unctrl(strp->h_ch));
#endif
		waddstr(hw, strp->h_desc);
		cnt++;
		strp++;
#if ANDROID || METRO
                if (cnt >= LINEZ && strp->h_ch) {
                    dbotline(hw, morestr);
	                cnt = 0;
                    wclear(hw);
                }
#endif
	}
	wmove(hw, LINES-1, 0);
	wprintw(hw,spacemsg);
	draw(hw);
	wait_for(hw,' ');
	draw(hw);
	wmove(cw, 0, 0);
	wclrtoeol(cw);
	touchwin(cw);
	nochange = FALSE;
}


/*
 * identify:
 *	Tell the player what a certain thing is.
 */
char *
identify(what)
int what;
{
	reg char ch, *str;

	if (what == 0) {
		msg("What do you want identified? ");
		ch = readchar();
		mpos = 0;
		if (ch == ESCAPE) {
			msg("");
			return NULL;
		}
	}
	else
		ch = what;
	if (isalpha(ch))
		str = monsters[midx(ch)].m_name;
	else {
		switch(ch) {
			case '|':
			case '-':	str = "the wall of a room";
			when GOLD:	str = "gold";
			when STAIRS:	str = "passage leading up/down";
			when DOOR:	str = "door";
			when FLOOR:	str = "room floor";
			when PLAYER:	str = "you";
			when PASSAGE:	str = "passage";
			when POST:	str = "trading post";
			when MAZETRAP:	str = "maze trap";
#if ANDROID || METRO
			when TRAPDOOR:	str = "trapdoor; ':' : food";
#else
			when TRAPDOOR:	str = "trapdoor";
#endif
			when ARROWTRAP:	str = "arrow trap";
			when SLEEPTRAP:	str = "sleeping gas trap";
			when BEARTRAP:	str = "bear trap";
			when TELTRAP:	str = "teleport trap";
			when DARTTRAP:	str = "dart trap";
			when POOL:	str = "magic pool";
			when POTION:	str = "potion";
			when SCROLL:	str = "scroll";
			when FOOD:	str = "food";
			when WEAPON:	str = "weapon";
			when ' ' :	str = "solid rock";
			when ARMOR:	str = "armor";
			when AMULET:	str = "The Amulet of Yendor";
			when RING:	str = "ring";
			when STICK:	str = "wand or staff";
			otherwise:
				if (what == 0)
					str = "unknown character";
				else
					str = "a magical ghost";
		}
	}
	if (what == 0)
		msg("'%s' : %s", unctrl(ch), str);
	return str;
}

/*
 * d_level:
 *	He wants to go down a level
 */
void
d_level()
{
	if (winat(hero.y, hero.x) != STAIRS)
		msg("I see no way down.");
	else {
		if (pl_on(ISHELD)) {
			msg("You are being held.");
			return;
		}
		level++;
#ifdef BLACKGUARD
		notifyStairs();
#endif
		new_level(NORMLEV);
	}
}

/*
 * u_level:
 *	He wants to go up a level
 */
void
u_level()
{
	if (winat(hero.y, hero.x) == STAIRS)  {
		if (pl_on(ISHELD)) {
			msg("You are being held.");
			return;
		}
		else {				/* player not held here */
			if (amulet) {
				level--;
				if (level == 0)
					total_winner();
#ifdef BLACKGUARD
				notifyStairs();
#endif
				new_level(NORMLEV);
				msg("You feel a wrenching sensation in your gut.");
				return;
			}
		}
	}
	msg("I see no way up.");
}


/*
 * Let him escape for a while
 */
#ifndef WIN32
void
shell()
{
	reg int pid;
	reg char *sh;
	int ret_status;

	/*
	 * Set the terminal back to original mode
	 */
	sh = getenv("SHELL");
	wclear(hw);
	wmove(hw, LINES-1, 0);
	draw(hw);
	endwin();
	in_shell = TRUE;
	fflush(stdout);
	/*
	 * Fork and do a shell
	 */
#ifndef __DJGPP__
	while((pid = fork()) < 0)
		sleep(1);
	if (pid == 0) {
#ifndef ANDROID
		setuid(playuid);	/* Set back to original user */
		setgid(playgid);
#endif
		execl(sh == NULL ? "/bin/sh" : sh, "shell", "-i", 0);
		perror("No shelly");
		byebye(-1);
	}
	else {
		signal(SIGINT, SIG_IGN);
#ifdef SIGQUIT
		signal(SIGQUIT, SIG_IGN);
#endif
		while (wait(&ret_status) != pid)
			continue;
		signal(SIGINT, quit);
#ifdef SIGQUIT
		signal(SIGQUIT, endit);
#endif

#else
	{
		char shell[PATH_MAX];

		if (sh && *sh)
			strncpy(shell,sh,PATH_MAX);
		else
			sprintf(shell, "%s\\bin\\sh.exe", getenv("DJDIR"));

		if (spawnl(P_WAIT,shell, "shell", "-i", 0) == -1)
			msg("No shelly: %s", shell);

#endif
		printf("\n%s", retstr);
		fflush(stdout);
		noecho();
		crmode();
		in_shell = FALSE;
		wait_for(cw, '\n');
		restscr(cw);
	}
}
#endif

/*
 * call:
 *	Allow a user to call a potion, scroll, or ring something
 */
void
call()
{
	reg struct object *obj;
	reg struct linked_list *item;
	reg char **guess, *elsewise;
	int wh;

	if ((item = get_item("call", 0)) == NULL)
		return;
	obj = OBJPTR(item);
	wh = obj->o_which;
	switch (obj->o_type) {
	case RING:
		guess = r_guess;
		elsewise = (r_guess[wh] != NULL ? r_guess[wh] : r_stones[wh]);
	when POTION:
		guess = p_guess;
		elsewise = (p_guess[wh] != NULL ? p_guess[wh] : p_colors[wh]);
	when SCROLL:
		guess = s_guess;
		elsewise = (s_guess[wh] != NULL ? s_guess[wh] : s_names[wh]);
	when STICK:
		guess = ws_guess;
		elsewise =(ws_guess[wh] != NULL ?
			ws_guess[wh] : ws_stuff[wh].ws_made);
	otherwise:
		msg("You can't call %ss anything",obj->o_typname);
		return;
	}
	msg("Was called \"%s\"", elsewise);
	msg(callit);
	strcpy(prbuf, elsewise);
	if (get_str(prbuf, cw) == NORM) {
		if (guess[wh] != NULL)
			free(guess[wh]);
		guess[wh] = alloc(strlen(prbuf) + 1);
		strcpy(guess[wh], prbuf);
	}
	msg("");
}
