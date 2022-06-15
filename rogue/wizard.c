/*
 * Mostly wizard commands. Sometimes used by players.
 *
 * @(#) wizard.c	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <ctype.h>
#include "rogue.h"
//#include <pwd.h>

#ifdef WIN32
#define VERASE 0x8
#define VKILL 0x15
#else
#include <termios.h>
extern struct termios terminal;
#endif

/*
 * whatis:
 *	What a certain object is
 */
void
whatis(what)
struct linked_list *what;
{
	reg struct object *obj;
	reg struct linked_list *item;
	reg int wh;

	if (what == NULL) {				/* we need to ask */
		if ((item = get_item("identify", 0)) == NULL)
			return;
	}
	else							/* no need to ask */
		item = what;
	obj = OBJPTR(item);
	setoflg(obj, ISKNOW);
	wh = obj->o_which;
	switch (obj->o_type) {
		case SCROLL:
			s_know[wh] = TRUE;
			if (s_guess[wh]) {
				free(s_guess[wh]);
				s_guess[wh] = NULL;
			}
	    when POTION:
			p_know[wh] = TRUE;
			if (p_guess[wh]) {
				free(p_guess[wh]);
				p_guess[wh] = NULL;
			}
		when STICK:
			ws_know[wh] = TRUE;
			if (ws_guess[wh]) {
				free(ws_guess[wh]);
			ws_guess[wh] = NULL;
			}
	    when RING:
			r_know[wh] = TRUE;
			if (r_guess[wh]) {
				free(r_guess[wh]);
				r_guess[wh] = NULL;
			}
	}
	if (what == NULL)
		msg(inv_name(obj, FALSE));
}


/*
 * create_obj:
 *	Create any object for wizard or scroll (almost)
 */
void
create_obj(fscr, fromscrolls)
BOOL fscr;
BOOL fromscrolls;
{
	reg struct linked_list *item;
	reg struct object *obj;
	reg int wh, ch, otype;
	char newitem, newtype, msz, *oname;
	struct magic_info *mf;
	BOOL nogood = TRUE, inhw = FALSE;

	if (fscr)
		msg(" ");
	if (nogood) {
		inhw = TRUE;
		wclear(hw);
		wprintw(hw,"Item\tKey\n\n");
		for (otype = 0; otype < NUMTHINGS; otype++) {
            if (fromscrolls) {
                if (otype != TYP_AMULET) {
                    mf = &thnginfo[otype];
                    wprintw(hw, "%s\t %c\n", things[otype].mi_name, mf->mf_show);
                }
            } else if (wizard == TRUE) {
				mf = &thnginfo[otype];
				wprintw(hw, "%s\t %c\n", things[otype].mi_name, mf->mf_show);
            } else if (wizard == CONJURER) {
                if (otype == TYP_FOOD || otype == TYP_POTION || otype == TYP_SCROLL) {
                    mf = &thnginfo[otype];
                    wprintw(hw, "%s\t %c\n", things[otype].mi_name, mf->mf_show);
                }
            }
		}
		if (wizard)
			waddstr(hw,"monster\t (A-z)");
		wprintw(hw,"\n\nWhat do you want to create? ");
		draw(hw);
		do {
			ch = readchar();
			if (ch == ESCAPE) {
				after = FALSE;
				restscr(cw);
				return;
			}
			switch (ch) {
				case POTION:
				case SCROLL:
				case FOOD:
                case 62:   /* food? */
					nogood = FALSE;
					break;
				case RING:
				case STICK:
				case ARMOR:
				case WEAPON:
					if (fromscrolls || wizard == TRUE) {
						nogood = FALSE;
					}
					break;
				case AMULET:
					if (wizard == TRUE) {
						nogood = FALSE;
					}
					break;
				default:
					if (isalpha(ch))
						nogood = FALSE;
			}
		} while (nogood);
	}
    if (ch == FOOD || ch == 62) {
        if (inhw)
            restscr(cw);
        msg("Bon appetit!");
        item = new_thing(FALSE, FOOD, 0);
        wh = add_pack(item, FALSE);
        if (wh == FALSE)			/* won't fit in pack */
            discard(item);
        return;
    }
	if (isalpha(ch)) {
		if (inhw)
			restscr(cw);
        msg("Your monster, as requested");
		makemons(ch);		/* make monster & be done with it */
		return;
	}
	otype = getindex(ch);
	if (otype == -1) {
		if (inhw)
			restscr(cw);
		mpos = 0;
		msg("You can't create that !!");
		return;
	}
	newitem = ch;
	mf = &thnginfo[otype];
	oname = things[otype].mi_name;
	msz = mf->mf_max;
	nogood = TRUE;
	if (msz == 1) {		/* if only one type of item */
		ch = 'a';
		nogood = FALSE;
	}
	else if (!fscr && wizard) {
		if (!inhw) {
			msg("Which %s?%s: ", oname, starlist);
			ch = readchar();
			if (ch == ESCAPE)
				return;
			if (ch != '*')
				nogood = FALSE;
		}
	}
	if (nogood) {
		struct magic_item *wmi;
		int ii;

		mpos = 0;
		inhw = TRUE;
		switch(newitem) {
			case POTION:	wmi = &p_magic[0];
			when SCROLL:	wmi = &s_magic[0];
			when RING:		wmi = &r_magic[0];
			when STICK:		wmi = &ws_magic[0];
			when WEAPON:	wmi = &w_magic[0];
			otherwise:		wmi = &a_magic[0];
		}
		wclear(hw);
		for (ii = 0 ; ii < msz ; ii++) {
			mvwaddch(hw,ii % 13,ii > 12 ? COLS/2 : 0, ii + 'a');
			waddstr(hw,") ");
			waddstr(hw,wmi->mi_name);
			wmi++;
		}
		sprintf(prbuf,"Which %s? ", oname);
		mvwaddstr(hw,LINES - 1, 0, prbuf);
		draw(hw);
		do {
			ch = readchar();
			if (ch == ESCAPE) {
				restscr(cw);
				msg("");
				return;
			}
		} while (!isalpha(ch));
	}
	if (inhw)			/* restore screen if need be */
		restscr(cw);

	newtype = tolower(ch) - 'a';
	if (newtype < 0 || newtype >= msz) {	/* if an illegal value */
		mpos = 0;
		after = FALSE;
		if (inhw)
			restscr(cw);
		msg("There is no such %s", oname);
		return;
	}
	if (fromscrolls == FALSE && wizard == CONJURER) {  /* conjurer can't create scroll of acquirement */
		if (newitem == SCROLL && newtype == S_MAKEIT) {
			mpos = 0;
			after = FALSE;
			if (inhw)
				restscr(cw);
			msg("Sorry, you can't conjure a scroll of acquirement");
			return;
		}
	}
	mpos = 0;
	item = new_thing(FALSE, newitem, newtype);
	obj = OBJPTR(item);
	wh = obj->o_type;
	if (wh == WEAPON || wh == ARMOR || wh == RING) {
		if (fscr)					/* users get +3 to -3 */
			ch = rnd(7) - 3;
		else {						/* wizard gets to choose */
			if (wh == RING)
				init_ring(obj, TRUE);
			else
				ch = getbless();
		}
		if (wh == WEAPON)
			obj->o_hplus = obj->o_dplus = ch;
		else if (wh == ARMOR)
			obj->o_ac = armors[obj->o_which].a_class - ch;
		if (ch < 0)
			setoflg(obj, ISCURSED);
		else
			resoflg(obj, ISCURSED);
	}
	mpos = 0;
	if (fscr)
		whatis(item);			/* identify for aquirement scroll */
	wh = add_pack(item, FALSE);
	if (wh == FALSE)			/* won't fit in pack */
		discard(item);
}


/*
 * getbless:
 *	Get a blessing for a wizards object
 */
int
getbless()
{
	int bless;

	msg("Blessing: ");
	prbuf[0] = '\0';
	bless = get_str(prbuf, cw);
	if (bless == NORM)
		bless = atoi(prbuf);
	else
		bless = 0;
	return bless;
}

/*
 * makemons:
 *	Make a monster
 */
int
makemons(what)
int what;
{
	reg int x, y, oktomake = FALSE, appear = 1;
	struct coord mp;

	oktomake = FALSE;
	for (x = hero.x - 1 ; x <= hero.x + 1 ; x++) {
		for (y = hero.y - 1 ; y <= hero.y + 1 ; y++) {
			if (x != hero.x || y != hero.y) {
				if (step_ok(winat(y, x)) && rnd(++appear) == 0) {
					mp.x = x;
					mp.y = y;
					oktomake = TRUE;
					break;
				}
			}
		}
	}
	if (oktomake) {
		new_monster(midx(what), &mp, FALSE);
		look(FALSE);
	}
	return oktomake;
}

/*
 * telport:
 *	Bamf the thing someplace else
 */
int
teleport(spot, th)
struct coord spot;
struct thing *th;
{
	reg int rm, y, x;
	struct coord oldspot;
	struct room *rp;
	BOOL ishero;

	ishero = (th == &player);
	oldspot = th->t_pos;
	y = th->t_pos.y;
	x = th->t_pos.x;
	mvwaddch(cw, y, x, th->t_oldch); 
	if (!ishero)
		mvwaddch(mw, y, x, ' ');
	rp = roomin(&spot);
	if (spot.y < 0 || !step_ok(winat(spot.y, spot.x))) {
		rp = &rooms[rnd_room()];
		th->t_pos = *rnd_pos(rp);
	}
	else
		th->t_pos = spot;
	rm = rp - &rooms[0];
	th->t_room = rp;
	th->t_oldch = mvwinch(cw, th->t_pos.y, th->t_pos.x);
	light(&oldspot);
	th->t_nomove = 0;
	if (ishero) {
		light(&hero);
		mvwaddch(cw, hero.y, hero.x, PLAYER);
		/*
		 * turn off ISHELD in case teleportation was done
		 * while fighting a Fungi or Bone Devil.
		 */
		if (pl_on(ISHELD))
			unhold('F');
		count = 0;
		running = FALSE;
		flushinp();			/* flush typeahead */
		nochange = FALSE;
	}
	else
		mvwaddch(mw, th->t_pos.y, th->t_pos.x, th->t_type);
	return rm;
}

/*
 * passwd:
 *	See if user knows password
 */
#if ANDROID || METRO
int
passwd(awin)
WINDOW *awin;
#else
int
passwd()
#endif
{
	reg char *sp, c;
	int i;
	BOOL passok;
	char buf[LINLEN], *xcrypt();

	msg(wizstr);
#if ANDROID || METRO
        passok = FALSE;
        buf[0] = '\0';
        if (get_str(buf, awin) == NORM)
        {
            passok = (strcmp(PASSWD, xcrypt(buf, "mT")) == 0);
	}
#else
	sp = buf;
	i = 0;
	while ((c = getchar()) != '\n' && c != '\r' && c != ESCAPE)
	{
#ifdef WIN32
		if (c == VKILL)
#else
		if (c == terminal.c_cc[VKILL])
#endif
		{
			i = 0;
			sp = buf;
		}
#ifdef WIN32
		else if (c == VERASE && sp > buf)
#else
		else if (c == terminal.c_cc[VERASE] && sp > buf)
#endif
		{
			if (i > 0)
			{
				i--;
				sp--;
			}
		}
		else
		{
			if (i < (LINLEN - 1))
			{
				i++;
				*sp++ = c;
			}
		}
	}
	if (sp == buf)
		passok = FALSE;
	else {
		*sp = '\0';
		passok = (strcmp(PASSWD, xcrypt(buf, "mT")) == 0);
	}
#endif
	return passok;
}
