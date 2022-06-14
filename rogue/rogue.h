/*
 * Rogue definitions and variable declarations
 *
 * @(#) rogue.h	1.0	(tep)	 9/1/2010
 *
 * Blackguard
 * Copyright (C) 2010 Tom Portegys
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <curses.h>
#ifdef ANDROID
#include <android/log.h>
#endif
#define reg     register        /* register abbr.       */

#ifndef BOOL
#define BOOL char
#endif

/*
 * Maximum number of different things
 */

#define NCOLORS		32
#define NSYLS       159
#define NSTONES     35
#define NWOOD       24
#define NMETAL      15

#define MAXDAEMONS 20

#define TYPETRAPS	9	/* max types of traps */
#define MAXROOMS	9	/* max rooms per level */
#define MAXTHINGS	9	/* max things on each level */
#define MAXOBJ		9	/* max goodies on each level */	
#define MAXPACK		23	/* max things this hero can carry */
#define MAXTRAPS	10	/* max traps per level */
#define MAXMONS		52	/* max available monsters */
#define MONRANGE	20	/* max # of monsters avail each level */
#define AMLEVEL		35	/* earliest level that amulet can appear */
#define MAXPURCH	4	/* max purchases in trading post */
#define MINABIL		3	/* minimum for any ability */
#define MAXSTR		24	/* maximum strength */
#define MAXOTHER	18	/* maximum wis, dex, con */
#define NORMAC		10	/* normal hero armor class (no armor) */
#define MONWIS		10	/* monsters standard wisdom */

#define NORMLEV		0	/* normal level */
#define POSTLEV		1	/* trading post level */
#define MAZELEV		2	/* maze level */

#define NORMFOOD	0	/* normal food's group no. */
#define FRUITFOOD	1	/* fruit's group no. */
#define NEWGROUP	2	/* start of group no. other than food */

#define	NUMTHINGS	8	/* types of goodies for hero */
#define TYP_POTION	0
#define TYP_SCROLL	1
#define TYP_FOOD	2
#define TYP_WEAPON	3
#define TYP_ARMOR	4
#define TYP_RING	5
#define TYP_STICK	6
#define TYP_AMULET	7

#define V_PACK		3600	/* max volume in pack */
#define V_POTION	50		/* volume of potion */
#define V_SCROLL	80		/* volume of scroll */
#define V_FOOD		35		/* volume of food */
#define V_WEAPON	0		/* volume of weapon (depends on wep) */
#define V_ARMOR		0		/* volume of armor (depends on armor) */
#define V_RING		20		/* volume of ring */
#define V_STICK		0		/* volume of stick (depends on staff/wand) */
#define V_AMULET	30		/* volume of amulet */

#define V_WS_STAFF	200		/* volume of a staff */
#define V_WS_WAND	110		/* volume of a wand */
#define W_WS_STAFF	100		/* weight of a staff */
#define W_WS_WAND	 60		/* weight of a wand */

#define FROMRING	2
#define DONTCARE	-1
#define ANYTHING	-1,-1	/* DONTCARE, DONTCARE */

#define K_ARROW		240		/* killed by an arrow */
#define K_DART		241		/* killed by a dart */
#define K_BOLT		242		/* killed by a bolt */
#define K_POOL		243		/* killed by drowning */
#define K_ROD		244		/* killed by an exploding rod */
#define K_SCROLL	245		/* killed by a burning scroll */
#define K_STONE		246		/* killed by materializing in rock */
#define K_STARVE	247		/* killed by starvation */
/*
 * return values for get functions
 */

#define	NORM	0		/* normal exit */
#define	QUIT	1		/* quit option setting */
#define	MINUS	2		/* back up one option */

/*
 * Return values for games end
 */
#define KILLED	0		/* hero was killed */
#define CHICKEN	1		/* hero chickened out (quit) */
#define WINNER	2		/* hero was a total winner */

/*
 * return values for chase routines
 */
#define CHASE	0		/* continue chasing hero */
#define FIGHT	1		/* fight the hero */
#define GONER	2		/* chaser fell into a trap */

/* Conjurer */
#define CONJURER 2

#ifdef BLACKGUARD

/* Terminal function wrappers: */

/* Initialization. */
WINDOW *initscr2();
#define initscr initscr2
WINDOW *newwin2(int nlines, int ncols, int begin_y, int begin_x);
#define newwin newwin2

/* Window control. */
int clearok2(WINDOW *win, int bf);
#define clearok clearok2
int clear2();
#define clear clear2
int wclear2(WINDOW *win);
#define wclear wclear2
int wclrtoeol2(WINDOW *win);
#define wclrtoeol wclrtoeol2
int refresh2();
#define refresh refresh2
int wrefresh2(WINDOW *win);
#define wrefresh wrefresh2
int touchwin2(WINDOW *win);
#define touchwin touchwin2
int overlay2(WINDOW *srcwin, WINDOW *dstwin);
#define overlay overlay2
int overwrite2(WINDOW *srcwin, WINDOW *dstwin);
#define overwrite overwrite2

// Move cursor
int move2(int y, int x);
#define move move2
int wmove2(WINDOW *win, int y, int x);
#define wmove wmove2

// Input
int getchar2();
#define getchar getchar2
#if WIN32 && !METRO
int _getch2();
#define _getch _getch2
#endif
int wgetch2 (WINDOW *win);
#define wgetch wgetch2
int wgetnstr2 (WINDOW *win, char *, int);
#define wgetnstr wgetnstr2
int winch2(WINDOW *win);
#define winch winch2
int mvinch2(int y, int x);
#define mvinch mvinch2
int mvwinch2(WINDOW *win, int y, int x);
#define mvwinch mvwinch2

// Output
int putchar2( int character );
#define putchar putchar2
int addch2(char ch);
#define addch addch2
int waddch2(WINDOW *win, char ch);
#define waddch waddch2
int mvaddch2(int y, int x, char ch);
#define mvaddch mvaddch2
int mvwaddch2(WINDOW *win, int y, int x, char ch);
#define mvwaddch mvwaddch2
int addstr2(char *str);
#define addstr addstr2
int waddstr2(WINDOW *win, char *str);
#define waddstr waddstr2
int mvaddstr2(int y, int x, char *str);
#define mvaddstr mvaddstr2
int mvwaddstr2(WINDOW *win, int y, int x, char *str);
#define mvwaddstr mvwaddstr2
int printw2(char *fmt, ...);
#define printw printw2
int wprintw2(WINDOW *win, char *fmt, ...);
#define wprintw wprintw2
int mvprintw2(int y, int x, char *fmt, ...);
#define mvprintw mvprintw2
int mvwprintw2(WINDOW *win, int y, int x, char *fmt, ...);
#define mvwprintw mvwprintw2
int printf2(char *format, ... );
#define printf printf2
int fputs2(char *s, FILE *stream);
#define fputs fputs2
int fflush2(FILE *stream);
#define fflush fflush2

// Termination
int isendwin2();
#define isendwin isendwin2
int endwin2();
#define endwin endwin2

// Add direction to movement.
void add_dir(int *dx, int *dy);

// Get/set direction.
int getDir();
void setDir(int);

// Arguments.
extern int Argc;
extern char **Argv;
extern char **Envp;

#if ANDROID || METRO
// Local directory.
#if ANDROID
extern char *LocalDir;
#endif
#if METRO
extern char LocalDir[];
#endif

// Identity UUID.
extern char *ID;

// Prompt for space.
extern int spaceprompt;
#endif

// Player is blind?
extern int isblind;

// Show text display. 
extern int showtext;

// Mute.
extern int mute;

// Display message.
extern char *displaymsg;
#endif

/*
 * All the fun defines
 */
#define next(ptr)	(*ptr).l_next
#define prev(ptr)	(*ptr).l_prev
#define ldata(ptr)	(*ptr).l_data
#define OBJPTR(what)	(struct object *)((*what).l_data)
#define THINGPTR(what)	(struct thing *)((*what).l_data)

#define inroom(rp, cp) (\
	(cp)->x <= (rp)->r_pos.x + ((rp)->r_max.x - 1) && \
	(rp)->r_pos.x <= (cp)->x && (cp)->y <= (rp)->r_pos.y + \
	((rp)->r_max.y - 1) && (rp)->r_pos.y <= (cp)->y)

#define unc(cp) (cp).y, (cp).x
#define cmov(xy) move((xy).y, (xy).x)
#define DISTANCE(y1,x1,y2,x2) ((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1))
#define when break;case
#define otherwise break;default
#define until(expr) while(!(expr))

#define ce(a, b) ((a).x == (b).x && (a).y == (b).y)
#define draw(window) wrefresh(window)

#define hero player.t_pos
#define pstats player.t_stats
#define pack player.t_pack

#define herowis() (getpwis(him))
#define herodex() (getpdex(him,FALSE))
#define herostr() (pstats.s_ef.a_str)
#define herocon() (pstats.s_ef.a_con)

#define attach(a,b) _attach(&a,b)
#define detach(a,b) _detach(&a,b)
#define free_list(a) _free_list(&a)
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#define goingup() (level < max_level)

#define on(thing, flag) (((thing).t_flags & flag) != 0)
#define off(thing, flag) (((thing).t_flags & flag) == 0)
#undef CTRL
#define CTRL(ch) (ch & 0x1F)

#define ALLOC(x) malloc((unsigned int) x)
#define FREE(x) free((char *) x)
#define	EQSTR(a, b, c)	(strncmp(a, b, c) == 0)
#define GOLDCALC (rnd(50 + 10 * level) + 2)
#define ISMULT(type) (type == POTION || type == SCROLL || type == FOOD)

#define newgrp() ++group
#define o_charges o_ac


/*
 * Things that appear on the screens
 */
#define PASSAGE		'#'
#define DOOR		'+'
#define FLOOR		'.'
#define PLAYER		'@'
#define POST		'^'
#define MAZETRAP	'\\'
#define TRAPDOOR	'>'
#define ARROWTRAP	'{'
#define SLEEPTRAP	'$'
#define BEARTRAP	'}'
#define TELTRAP		'~'
#define DARTTRAP	'`'
#define POOL		'"'
#define SECRETDOOR	'&'
#define STAIRS		'%'
#define GOLD		'*'
#define POTION		'!'
#define SCROLL		'?'
#define MAGIC		'$'
#define FOOD		':'
#define WEAPON		')'
#define ARMOR		']'
#define AMULET		','
#define RING		'='
#define STICK		'/'
#define CALLABLE	-1


/*
 *	stuff to do with encumberence 
 */
#define NORMENCB 1500	/* normal encumberence */
#define SOMTHERE 5		/* something is in the way for dropping */
#define CANTDROP 6		/* cant drop it cause its cursed */
#define F_OKAY	 0		/* have plenty of food in stomach */
#define F_HUNGRY 1		/* player is hungry */
#define F_WEAK	 2		/* weak from lack of food */
#define F_FAINT	 3		/* fainting from lack of food */


/*
 * Various constants
 */
#define	PASSWD		"mTqDomBotCbQw"
#define BEARTIME	3
#define SLEEPTIME	5
#define HEALTIME	30
#define HOLDTIME	2
#define STPOS		0
#define WANDERTIME	70
#define BEFORE		1
#define AFTER		2
#define HUHDURATION	20
#define SEEDURATION	850
#define HUNGERTIME	1300
#define WEAKTIME	150
#define HUNGTIME	300		/* 2 * WEAKTIME */
#define STOMACHSIZE	2000
#define ESCAPE		27
#define LEFT		0
#define RIGHT		1
#define BOLT_LENGTH	6

#define STR		1
#define DEX		2
#define CON		3
#define WIS		4

/*
 * Save against things
 */
#define VS_POISON			00
#define VS_PARALYZATION		00
#define VS_DEATH			00
#define VS_PETRIFICATION	01
#define VS_BREATH			02
#define VS_MAGIC			03


/*
 * Various flag bits
 */
#define ISSTUCK 0000001		/* monster can't run (violet fungi) */
#define ISDARK	0000001		/* room is dark */
#define ISCURSED 000001		/* object is cursed */
#define ISBLIND 0000001		/* hero is blind */
#define ISPARA  0000002		/* monster is paralyzed */
#define ISGONE	0000002		/* room is gone */
#define ISKNOW  0000002		/* object is known */
#define ISRUN	0000004		/* Hero & monsters are running */
#define ISTREAS 0000004		/* room is a treasure room */
#define ISPOST  0000004		/* object is in a trading post */
#define ISFOUND 0000010		/* trap is found */
#define ISINVINC 000010		/* player is invincible */
#define ISINVIS 0000020		/* monster is invisible */
#define ISPROT	0000020		/* object is protected somehow */
#define ISMEAN  0000040		/* monster is mean */
#define ISBLESS 0000040		/* object is blessed */
#define ISGREED 0000100		/* monster is greedy */
#define ISWOUND 0000200		/* monster is wounded */
#define ISHELD  0000400		/* hero is held fast */
#define ISHUH   0001000		/* hero | monster is confused */
#define ISREGEN 0002000		/* monster is regenerative */
#define CANHUH  0004000		/* hero can confuse monsters */
#define CANSEE  0010000		/* hero can see invisible monsters */
#define WASHIT	0010000		/* hero has hit monster at least once */
#define ISMISL  0020000		/* object is normally thrown in attacks */
#define ISCANC	0020000		/* monsters special attacks are canceled */
#define ISMANY  0040000		/* objects are found in a group (> 1) */
#define ISSLOW	0040000		/* hero | monster is slow */
#define ISHASTE 0100000		/* hero | monster is fast */
#define ISETHER	0200000		/* hero is thin as air */
#define NONE	100			/* equal to 'd' (used for weaps) */


/*
 * Potion types
 */
#define P_CONFUSE	0		/* confusion */
#define P_PARALYZE	1		/* paralysis */
#define P_POISON	2		/* poison */
#define P_STRENGTH	3		/* gain strength */
#define P_SEEINVIS	4		/* see invisible */
#define P_HEALING	5		/* healing */
#define P_MFIND		6		/* monster detection */
#define P_TFIND		7		/* magic detection */
#define P_RAISE		8		/* raise level */
#define P_XHEAL		9		/* extra healing */
#define P_HASTE		10		/* haste self */
#define P_RESTORE	11		/* restore strength */
#define P_BLIND		12		/* blindness */
#define P_NOP		13		/* thirst quenching */
#define P_DEX		14		/* increase dexterity */
#define P_ETH		15		/* etherealness */
#define P_SMART		16		/* wisdom */
#define P_REGEN		17		/* regeneration */
#define P_SUPHERO	18		/* super ability */
#define P_DECREP	19		/* decrepedness */
#define P_INVINC	20		/* invicibility */
#define MAXPOTIONS	21		/* types of potions */


/*
 * Scroll types
 */
#define S_CONFUSE	0		/* monster confusion */
#define S_MAP		1		/* magic mapping */
#define S_LIGHT		2		/* light */
#define S_HOLD		3		/* hold monster */
#define S_SLEEP		4		/* sleep */
#define S_ARMOR		5		/* enchant armor */
#define S_IDENT		6		/* identify */
#define S_SCARE		7		/* scare monster */
#define S_GFIND		8		/* gold detection */
#define S_TELEP		9		/* teleportation */
#define S_ENCH		10		/* enchant weapon */
#define S_CREATE	11		/* create monster */
#define S_REMOVE	12		/* remove curse */
#define S_AGGR		13		/* aggravate monster */
#define S_NOP		14		/* blank paper */
#define S_GENOCIDE	15		/* genocide */
#define S_KNOWALL	16		/* item knowledge */
#define S_PROTECT	17		/* item protection */
#define S_DCURSE	18		/* demons curse */
#define S_DLEVEL	19		/* transport */
#define S_ALLENCH	20		/* enchantment */
#define S_BLESS		21		/* gods blessing */
#define S_MAKEIT	22		/* aquirement */
#define S_BAN		23		/* banishment */
#define S_CWAND		24		/* charge wands */
#define S_LOCTRAP	25		/* locate traps */
#define MAXSCROLLS	26		/* types of scrolls */


/*
 * Weapon types
 */
#define MACE		0		/* mace */
#define SWORD		1		/* long sword */
#define BOW			2		/* short bow */
#define ARROW		3		/* arrow */
#define DAGGER		4		/* dagger */
#define ROCK		5		/* rocks */
#define TWOSWORD	6		/* two-handed sword */
#define SLING		7		/* sling */
#define DART		8		/* darts */
#define CROSSBOW	9		/* crossbow */
#define BOLT		10		/* crossbow bolt */
#define SPEAR		11		/* spear */
#define TRIDENT		12		/* trident */
#define SPETUM		13		/* spetum */
#define BARDICHE	14 		/* bardiche */
#define PIKE		15		/* pike */
#define BASWORD		16		/* bastard sword */
#define HALBERD		17		/* halberd */
#define MAXWEAPONS	18		/* types of weapons */


/*
 * Armor types
 */
#define LEATHER		0		/* leather */
#define RINGMAIL	1		/* ring */
#define STUDDED		2		/* studded leather */
#define SCALE		3		/* scale */
#define PADDED		4		/* padded */
#define CHAIN		5		/* chain */
#define SPLINT		6		/* splint */
#define BANDED		7		/* banded */
#define PLATEMAIL	8		/* plate mail */
#define PLATEARMOR	9		/* plate armor */
#define MAXARMORS	10		/* types of armor */


/*
 * Ring types
 */
#define R_PROTECT	0		/* protection */
#define R_ADDSTR	1		/* add strength */
#define R_SUSTSTR	2		/* sustain strength */
#define R_SEARCH	3		/* searching */
#define R_SEEINVIS	4		/* see invisible */
#define R_CONST		5		/* constitution */
#define R_AGGR		6		/* aggravate monster */
#define R_ADDHIT	7		/* agility */
#define R_ADDDAM	8		/* increase damage */
#define R_REGEN		9		/* regeneration */
#define R_DIGEST	10		/* slow digestion */
#define R_TELEPORT	11		/* teleportation */
#define R_STEALTH	12		/* stealth */
#define R_SPEED		13		/* speed */
#define R_FTRAPS	14		/* find traps */
#define R_DELUS		15		/* delusion */
#define R_SUSAB		16		/* sustain ability */
#define R_BLIND		17		/* blindness */
#define R_SLOW		18		/* lethargy */
#define R_GIANT		19		/* ogre strength */
#define R_SAPEM		20		/* enfeeblement */
#define R_HEAVY		21		/* burden */
#define R_LIGHT		22		/* illumination */
#define R_BREATH	23		/* fire protection */
#define R_KNOW		24		/* wisdom */
#define R_DEX		25		/* dexterity */
#define MAXRINGS	26		/* types of rings */


/*
 * Rod/Wand/Staff types
 */

#define WS_LIGHT	0		/* light */
#define WS_HIT		1		/* striking */
#define WS_ELECT	2		/* lightning */
#define WS_FIRE		3		/* fire */
#define WS_COLD		4		/* cold */
#define WS_POLYM	5		/* polymorph */
#define WS_MISSILE	6		/* magic missile */
#define WS_HASTE_M	7		/* haste monster */
#define WS_SLOW_M	8		/* slow monster */
#define WS_DRAIN	9		/* drain life */
#define WS_NOP		10		/* nothing */
#define WS_TELAWAY	11		/* teleport away */
#define WS_TELTO	12		/* teleport to */
#define WS_CANCEL	13		/* cancellation */
#define WS_SAPLIFE	14		/* sap life */
#define WS_CURE		15		/* curing */
#define WS_PYRO		16		/* pyromania */
#define WS_ANNIH	17		/* annihilate monster */
#define WS_PARZ		18		/* paralyze monster */
#define WS_HUNGER	19		/* food absorption */
#define WS_MREG		20		/* regenerate monster */
#define WS_MINVIS	21		/* hide monster */
#define WS_ANTIM	22		/* anti-matter */
#define WS_MOREMON	23		/* clone monster */
#define WS_CONFMON	24		/* confuse monster */
#define WS_MDEG		25		/* degenerate monster */
#define MAXSTICKS	26		/* max types of sticks */

#define MAXAMULETS	1		/* types of amulets */
#define MAXFOODS	1		/* types of food */


/*
 * Now we define the structures and types
 */

struct delayed_action {
	int d_type;
	int (*d_func)();
	int d_arg;
	int d_time;
};

/*
 * Help list
 */
struct h_list {
	char h_ch;
	char *h_desc;
};


/*
 * Coordinate data type
 */
struct coord {
	int x;			/* column position */
	int y;			/* row position */
};

struct monlev {
	int l_lev;		/* lowest level for a monster */
	int h_lev;		/* highest level for a monster */
	BOOL d_wand;	/* TRUE if monster wanders */
};

/*
 * Linked list data type
 */
struct linked_list {
	struct linked_list *l_next;
	struct linked_list *l_prev;
	char *l_data;			/* Various structure pointers */
};


/*
 * Stuff about magic items
 */
#define mi_wght mi_worth
struct magic_item {
	char *mi_name;			/* name of item */
	int mi_prob;			/* probability of getting item */
	int mi_prob_base;		/* base probability */
	int mi_worth;			/* worth of item */
};

struct magic_info {
	int mf_max;						/* max # of this type */
	int	mf_vol;						/* volume of this item */
	char mf_show;					/* appearance on screen */
	struct magic_item *mf_magic;	/* pointer to magic tables */
};

/*
 * staff/wand stuff
 */
struct rod {
	char *ws_type;		/* either "staff" or "wand" */
	char *ws_made;		/* "mahogany", etc */
	int	 ws_vol;		/* volume of this type stick */
	int  ws_wght;		/* weight of this type stick */
};

/*
 * armor structure 
 */
struct init_armor {
	int a_class;		/* normal armor class */
	int a_wght;			/* weight of armor */
	int a_vol;			/* volume of armor */
};

/*
 * weapon structure
 */
struct init_weps {
    char *w_dam;		/* hit damage */
    char *w_hrl;		/* hurl damage */
    unsigned int  w_flags;		/* flags */
    int  w_wght;		/* weight of weapon */
	int  w_vol;			/* volume of weapon */
    char w_launch;		/* need to launch it */
};


/*
 * Room structure
 */
struct room {
	struct coord r_pos;		/* Upper left corner */
	struct coord r_max;		/* Size of room */
	struct coord r_gold;	/* Where the gold is */
	struct coord r_exit[4];	/* Where the exits are */
	struct room *r_ptr[4];	/* this exits' link to next rm */
	int r_goldval;			/* How much the gold is worth */
	unsigned int r_flags;			/* Info about the room */
	int r_nexits;			/* Number of exits */
};

/*
 * Array of all traps on this level
 */
struct trap {
	struct coord tr_pos;	/* Where trap is */
	struct coord tr_goto;	/* where trap tranports to (if any) */
	unsigned int tr_flags;			/* Info about trap */
	char tr_type;			/* What kind of trap */
};

/*
 * structure for describing true abilities
 */
struct real {
	int a_str;			/* strength (3-24) */
	int a_dex;			/* dexterity (3-18) */
	int a_wis;			/* wisdom (3-18) */
	int a_con;			/* constitution (3-18) */
};

/*
 * Structure describing a fighting being
 */
struct stats {
	struct real s_re;	/* True ability */
	struct real s_ef;	/* Effective ability */
	int s_exp;			/* Experience */
	int s_lvl;			/* Level of mastery */
	int s_arm;			/* Armor class */
	int s_hpt;			/* Hit points */
	int s_maxhp;		/* max value of hit points */
	int s_pack;			/* current weight of his pack */
	int s_carry;		/* max weight he can carry */
	char s_dmg[16];		/* String describing damage done */
};

/*
 * Structure for monsters and player
 */
struct thing {
	struct stats t_stats;		/* Physical description */
	struct coord t_pos;			/* Position */
	struct coord t_oldpos;		/* last spot of it */
	struct coord *t_dest;		/* Where it is running to */
	struct linked_list *t_pack;	/* What the thing is carrying */
	struct room *t_room;		/* Room this thing is in */
	unsigned int t_flags;				/* State word */
	int t_indx;					/* Index into monster structure */
	int t_nomove;				/* # turns you cant move */
	int t_nocmd;				/* # turns you cant do anything */
	BOOL t_turn;				/* If slow, is it a turn to move */
	char t_type;				/* What it is */
	char t_disguise;			/* What mimic looks like */
	char t_oldch;				/* Char that was where it was */
	char t_reserved;
};

/*
 * Array containing information on all the various types of mosnters
 */
struct monster {
	char *m_name;			/* What to call the monster */
	char m_show;			/* char that monster shows */
	short m_carry;			/* Probability of having an item */
	struct monlev m_lev;	/* level stuff */
	unsigned int m_flags;			/* Things about the monster */
	struct stats m_stats;	/* Initial stats */
};

/*
 * Structure for a thing that the rogue can carry
 */
struct object {
	struct coord o_pos;		/* Where it lives on the screen */
	char o_damage[8];		/* Damage if used like sword */
	char o_hurldmg[8];		/* Damage if thrown */
	char *o_typname;		/* name this thing is called */
	int o_type;				/* What kind of object it is */
	int o_count;			/* Count for plural objects */
	int o_which;			/* Which object of a type it is */
	int o_hplus;			/* Plusses to hit */
	int o_dplus;			/* Plusses to damage */
	int o_ac;				/* Armor class or charges */
	unsigned int o_flags;			/* Information about objects */
	int o_group;			/* Group number for this object */
	int o_weight;			/* weight of this object */
	int o_vol;				/* volume of this object */
	char o_launch;			/* What you need to launch it */
};

#if ANDROID || METRO
#define LINEZ   13          /* large text lines */
#ifdef ANDROID
#define LINLEN	128			/* length of buffers */
#endif
#ifdef METRO
#define LINLEN	256			/* length of buffers */
#endif
#else
#define LINLEN	80			/* length of buffers */
#endif

#define EXTLKL	extern struct linked_list
#define EXTTHG	extern struct thing
#define EXTOBJ	extern struct object
#define EXTSTAT extern struct stats
#define EXTCORD	extern struct coord
#define EXTMON	extern struct monster
#define EXTARM	extern struct init_armor
#define EXTWEP	extern struct init_weps
#define EXTMAG	extern struct magic_item
#define EXTROOM	extern struct room
#define EXTTRAP	extern struct trap
#define EXTINT	extern int
#define EXTBOOL	extern BOOL
#define EXTCHAR	extern char

/*
 * Externs (see global.c)
 */
extern struct room rooms[MAXROOMS];
extern struct room *oldrp;
extern struct linked_list *mlist;
extern struct thing player;
extern struct stats max_stats;
extern struct linked_list *lvl_obj;
extern struct object *cur_weapon;
extern struct object *cur_armor;
extern struct object *cur_ring[2];
extern struct stats *him;
extern struct trap traps[MAXTRAPS];

#ifndef WIN32
extern int playuid;
extern int playgid;
#endif
extern int level;
extern int levcount;
extern int levtype;
extern int trader;
extern int curprice;
extern int purse;
extern int mpos;
extern int ntraps;
extern int packvol;
extern int total;
extern int demoncnt;
extern int lastscore;
extern int no_food;
extern int seed;
extern int dnum;
extern int count;
extern int fung_hit;
extern int quiet;
extern int max_level;
extern int food_left;
extern int group;
extern int hungry_state;
extern int foodlev;
extern int ringfood;
extern char take;
extern char runch;
extern char curpurch[15];

extern char prbuf[LINLEN];
extern char whoami[LINLEN];
extern char fruit[LINLEN];
extern char huh[LINLEN];
extern char file_name[LINLEN];
extern char scorefile[LINLEN];
extern char home[LINLEN];
extern char outbuf[BUFSIZ];

extern char *s_guess[MAXSCROLLS];
extern char *p_guess[MAXPOTIONS];
extern char *r_guess[MAXRINGS];
extern char *ws_guess[MAXSTICKS];

extern BOOL isfight;
extern BOOL nlmove;
extern BOOL inpool;
extern BOOL inwhgt;
extern BOOL running;
extern BOOL playing;
extern BOOL wizard;
extern BOOL after;
extern BOOL door_stop;
extern BOOL firstmove;
extern BOOL waswizard;
extern BOOL amulet;
extern BOOL in_shell;
extern BOOL nochange;

extern BOOL s_know[MAXSCROLLS];
extern BOOL p_know[MAXPOTIONS];
extern BOOL r_know[MAXRINGS];
extern BOOL ws_know[MAXSTICKS];

extern char spacemsg[];
extern char morestr[];
extern char retstr[];
extern char wizstr[];
extern char illegal[];
extern char callit[];
extern char starlist[];

extern struct coord oldpos;
extern struct coord delta;
extern struct coord stairs;
extern struct coord rndspot;

extern struct monster *mtlev[MONRANGE];

extern struct monster monsters[MAXMONS + 1];

extern struct h_list helpstr[];

extern char *s_names[MAXSCROLLS];
extern char *p_colors[MAXPOTIONS];
extern char *r_stones[MAXRINGS];
extern struct rod ws_stuff[MAXSTICKS];

extern struct magic_item things[NUMTHINGS + 1];

extern struct magic_item a_magic[MAXARMORS + 1];

extern struct init_armor armors[MAXARMORS];

extern struct magic_item w_magic[MAXWEAPONS + 1];

extern struct init_weps weaps[MAXWEAPONS];

extern struct magic_item s_magic[MAXSCROLLS + 1];

extern struct magic_item p_magic[MAXPOTIONS + 1];

extern struct magic_item r_magic[MAXRINGS + 1];

extern struct magic_item ws_magic[MAXSTICKS + 1];

extern struct magic_info thnginfo[NUMTHINGS];

extern int e_levels[];

extern WINDOW *cw;
extern WINDOW *hw;
extern WINDOW *mw;
#if METRO
extern WINDOW *sw;
#endif

extern char *rainbow[NCOLORS];
extern char *sylls[NSYLS];
extern char *stones[];
extern char *wood[NWOOD];
extern char *metal[NMETAL];

extern struct delayed_action d_list[MAXDAEMONS];
extern int between;

extern char version[];
extern char *release;

/*
 * Function prototypes.
 */

/* armor.c */
void wear(void);
void take_off(void);
void initarmor(struct object *obj, int what);
int hurt_armor(struct object *obj);
/* chase.c */
void runners(void);
int do_chase(struct linked_list *mon);
int chase(struct thing *tp, struct coord *ee, BOOL runaway, BOOL dofight);
void runto(struct coord *runner, struct coord *spot);
struct room *roomin(struct coord *cp);
struct linked_list *find_mons(int y, int x);
int diag_ok(struct coord *sp, struct coord *ep);
int cansee(int y, int x);
/* command.c */
void command(void);
void quit(int a);
void search(void);
void help(void);
char *identify(int what);
void d_level(void);
void u_level(void);
void shell(void);
void call(void);
/* daemon.c */
struct delayed_action *d_insert(int (*func)(void), int arg, int type, int time);
void d_delete(struct delayed_action *wire);
struct delayed_action *find_slot(int (*func)(void));
void do_daemon(int (*func)(void), int arg, int type);
void do_daemons(int flag);
void fuse(int (*func)(void), int arg, int time);
void lengthen(int (*func)(void), int xtime);
void extinguish(int (*func)(void));
void do_fuses(void);
void activity(void);
/* daemons.c */
void doctor();
void swander();
void rollwand();
void unconfuse();
void unsee();
void sight();
void nohaste();
void stomach();
void noteth();
void sapem();
void notslow();
void notregen();
void notinvinc();
/* disply.c */
void displevl(void);
void dispmons(void);
char winat(int y, int x);
int cordok(int y, int x);
int pl_on(unsigned int what);
int pl_off(unsigned int what);
int o_on(struct object *what, unsigned int bit);
int o_off(struct object *what, unsigned int bit);
void setoflg(struct object *what, unsigned int bit);
void resoflg(struct object *what, unsigned int bit);
/* encumb.c */
void updpack(void);
int packweight(void);
int itemweight(struct object *wh);
int pack_vol(void);
int itemvol(struct object *wh);
int playenc(void);
int totalenc(void);
void wghtchk(int fromfuse);
int hitweight(void);
/* fight.c */
int fight(struct coord *mp, struct object *weap, BOOL thrown);
int attack(struct thing *mp);
int swing(int at_lvl, int op_arm, int wplus);
void check_level(void);
int roll_em(struct stats *att, struct stats *def, struct object *weap, BOOL hurl);
char *mindex(char *cp, int c);
char *prname(char *who, BOOL upper);
void hit(char *er);
void miss(char *er);
int save_throw(int which, struct thing *tp);
int save(int which);
void raise_level(void);
void thunk(struct object *weap, char *mname);
void bounce(struct object *weap, char *mname);
void remove_monster(struct coord *mp, struct linked_list *item);
int is_magic(struct object *obj);
void killed(struct linked_list *item, BOOL pr);
/* init.c */
void init_everything(void);
void init_globals(void);
void init_things(void);
void init_colors(void);
void init_names(void);
void init_stones(void);
void init_materials(void);
void badcheck(char *name, struct magic_item *magic);
void init_player(void);
int pinit(void);
/* io.c */
void msg(char *fmt, ...);
void addmsg(char *fmt, ...);
void endmsg(void);
void doadd(char *fmt, va_list ap);
int step_ok(int ch);
int dead_end(int ch);
int readchar(void);
void status(int fromfuse);
void dispmax(void);
int illeg_ch(int ch);
void wait_for(WINDOW *win, char ch);
void dbotline(WINDOW *scr, char *message);
void restscr(WINDOW *scr);
int npch(int ch);
/* list.c */
void _detach(struct linked_list **list, struct linked_list *item);
void _attach(struct linked_list **list, struct linked_list *item);
void _free_list(struct linked_list **ptr);
void discard(struct linked_list *item);
struct linked_list *new_item(int size);
char *alloc(int size);
/* main.c */
int main(int argc, char **argv, char **envp);
void endit(int a);
void fatal(char *s);
void byebye(int how);
int rnd(int range);
int roll(int number, int sides);
void setup(void);
void playit(void);
int author(void);
#ifndef METRO
int directory_exists(char *dirname);
#endif
char *roguehome(void);
#if ( ANDROID || METRO || NEW_GAME )
void reset();
#endif
/* misc.c */
void waste_time(void);
int getindex(int what);
char *tr_name(int ch);
void look(BOOL wakeup);
struct linked_list *find_obj(int y, int x);
void eat(void);
void aggravate(void);
char *vowelstr(char *str);
int is_current(struct object *obj);
int get_dir(void);
void initfood(struct object *what);
/* monsters.c */
int rnd_mon(BOOL wander, BOOL baddie);
void lev_mon(void);
struct linked_list *new_monster(char type, struct coord *cp, BOOL treas);
void wanderer(void);
struct linked_list *wake_monster(int y, int x);
void genocide(void);
void unhold(int whichmon);
int midx(int whichmon);
int monhurt(struct thing *th);
/* move.c */
void do_run(int ch);
void do_move(int dy, int dx);
void light(struct coord *cp);
int show(int y, int x);
int be_trapped(struct coord *tc, struct thing *th);
void dip_it(void);
struct trap *trap_at(int y, int x);
struct coord *rndmove(struct thing *who);
int isatrap(int ch);
/* new_leve.c */
void new_level(int ltype);
int rnd_room(void);
void put_things(void);
/* options.c */
void option(void);
int get_str(char *opt, WINDOW *awin);
void parse_opts(char *str);
void strucpy(char *s1, char *s2, int len);
/* pack.c */
int add_pack(struct linked_list *item, BOOL silent);
int inventory(struct linked_list *list, int type);
void pick_up(int ch);
void picky_inven(void);
struct linked_list *get_item(char *purpose, int type);
char pack_char(struct object *obj);
void idenpack(void);
void del_pack(struct linked_list *what);
void cur_null(struct object *op);
/* passages.c */
void do_passages(void);
void conn(int r1, int r2);
void door(struct room *rm, struct coord *cp);
void add_pass(void);
/* potions.c */
void quaff(void);
/* pstats.c */
void chg_hpt(int howmany, BOOL alsomax, char what);
void rchg_str(int amt);
void chg_abil(int what, int amt, int how);
void updabil(int what, int amt, struct real *pst, int how);
void add_haste(BOOL potion);
int getpdex(struct stats *who, BOOL heave);
int getpwis(struct stats *who);
int getpcon(struct stats *who);
int str_plus(struct stats *who);
int add_dam(struct stats *who);
int hungdam(void);
void heal_self(int factor, BOOL updmaxhp);
/* rings.c */
void ring_on(void);
void ring_off(void);
void toss_ring(struct object *what);
int gethand(BOOL isrmv);
int ring_eat(void);
char *ring_num(struct object *what);
int magring(struct object *what);
void ringabil(void);
void init_ring(struct object *what, BOOL fromwiz);	
int ringex(int rtype);
int iswearing(int ring);
int isring(int hand, int ring);
/* rip.c */
void death(int monst);
void score(int amount, int aflag, int monst);
int showtop(int showname);
#if ANDROID || METRO
int showscores();
#endif
void total_winner(void);
void showpack(BOOL winner, char *howso);
char *killname(int monst);
/* rooms.c */
void do_rooms(void);
void add_mon(struct room *rm, BOOL treas);
void draw_room(struct room *rp);
void horiz(int cnt);
void vert(int cnt);
struct coord *rnd_pos(struct room *rp);
int rf_on(struct room *rm, unsigned int bit);
/* save.c */
void ignore(void);
int save_game(void);
void auto_save(int a);
void game_err(int a);
int dosave(void);
void save_file(FILE *savef);
#ifdef METRO
int restore(char *file);
#else
int restore(char *file, char **envp);
#endif
#if ANDROID || METRO
int doload();
#endif
/* scrolls.c */
void read_scroll(void);
/* state.c */
void encwrite(register void *starta, unsigned int size, register FILE *outf);
int encread(register void *starta, unsigned int size, register int inf);
void *get_list_item(struct linked_list *l, int i);
int find_list_ptr(struct linked_list *l, void *ptr);
int list_size(struct linked_list *l);
int rs_write(FILE *savef, void *ptr, int size);
int rs_write_char(FILE *savef, char c);
int rs_write_shint(FILE *savef, unsigned char c);
int rs_write_short(FILE *savef, short c);
int rs_write_shorts(FILE *savef, short *c, int count);
int rs_write_ushort(FILE *savef, unsigned short c);
int rs_write_int(FILE *savef, int c);
int rs_write_ints(FILE *savef, int *c, int count);
int rs_write_uint(FILE *savef, unsigned int c);
int rs_write_long(FILE *savef, long c);
int rs_write_longs(FILE *savef, long *c, int count);
int rs_write_ulong(FILE *savef, unsigned long c);
int rs_write_ulongs(FILE *savef, unsigned long *c, int count);
int rs_write_string(FILE *savef, char *s);
int rs_write_string_index(FILE *savef, char *master[], int max, char *str);
int rs_write_strings(FILE *savef, char *s[], int count);
int rs_read(int inf, void *ptr, int size);
int rs_read_char(int inf, char *c);
int rs_read_shint(int inf, unsigned char *i);
int rs_read_short(int inf, short *i);
int rs_read_shorts(int inf, short *i, int count);
int rs_read_ushort(int inf, unsigned short *i);
int rs_read_int(int inf, int *i);
int rs_read_ints(int inf, int *i, int count);
int rs_read_uint(int inf, unsigned int *i);
int rs_read_long(int inf, long *i);
int rs_read_longs(int inf, long *i, int count);
int rs_read_ulong(int inf, unsigned long *i);
int rs_read_ulongs(int inf, unsigned long *i, int count);
int rs_read_string(int inf, char *s, int max);
int rs_read_new_string(int inf, char **s);
int rs_read_string_index(int inf, char *master[], int maxindex, char **str);
int rs_read_strings(int inf, char **s, int count, int max);
int rs_read_new_strings(int inf, char **s, int count);
int rs_write_coord(FILE *savef, struct coord c);
int rs_read_coord(int inf, struct coord *c);
int rs_write_daemons(FILE *savef, struct delayed_action *d_list, int count);
int rs_read_daemons(int inf, struct delayed_action *d_list, int count);
int rs_write_room_reference(FILE *savef, struct room *rp);
int rs_read_room_reference(int inf, struct room **rp);
int rs_write_rooms(FILE *savef, struct room r[], int count);
int rs_read_rooms(int inf, struct room *r, int count);
int rs_write_monlev(FILE *savef, struct monlev m);
int rs_read_monlev(int inf, struct monlev *m);
int rs_write_magic_items(FILE *savef, struct magic_item *i, int count);
int rs_read_magic_items(int inf, struct magic_item *mi, int count);
int rs_write_real(FILE *savef, struct real r);
int rs_read_real(int inf, struct real *r);
int rs_write_stats(FILE *savef, struct stats *s);
int rs_read_stats(int inf, struct stats *s);
int rs_write_monster_reference(FILE *savef, struct monster *m);
int rs_read_monster_reference(int inf, struct monster **mp);
void rs_write_monster_references(FILE *savef, struct monster *marray[], int count);
void rs_read_monster_references(int inf, struct monster *marray[], int count);
int rs_write_object(FILE *savef, struct object *o);
int rs_read_object(int inf, struct object *o);
int rs_read_object_list(int inf, struct linked_list **list);
int rs_write_object_list(FILE *savef, struct linked_list *l);
void rs_write_traps(FILE *savef, struct trap *trap, int count);
int rs_read_traps(int inf, struct trap *trap, int count);
int rs_write_monsters(FILE *savef, struct monster *m, int count);
int rs_read_monsters(int inf, struct monster *m, int count);
int find_thing_coord(struct linked_list *monlist, struct coord *c);
int find_object_coord(struct linked_list *objlist, struct coord *c);
void rs_fix_thing(struct thing *t);
int find_room_coord(struct room *rmlist, struct coord *c, int n);
int rs_write_thing(FILE *savef, struct thing *t);
int rs_read_thing(int inf, struct thing *t);
void rs_fix_monster_list(struct linked_list *list);
int rs_write_monster_list(FILE *savef, struct linked_list *l);
int rs_read_monster_list(int inf, struct linked_list **list);
int rs_write_object_reference(FILE *savef, struct linked_list *list, struct object *item);
int rs_read_object_reference(int inf, struct linked_list *list, struct object **item);
int rs_read_scrolls(int inf);
int rs_write_scrolls(FILE *savef);
int rs_read_potions(int inf);
int rs_write_potions(FILE *savef);
int rs_read_rings(int inf);
int rs_write_rings(FILE *savef);
int rs_write_sticks(FILE *savef);
int rs_read_sticks(int inf);
int rs_save_file(FILE *savef);
int rs_restore_file(int inf);
/* sticks.c */
void fix_stick(struct object *cur);
void do_zap(BOOL gotdir);
void drain(int ymin, int ymax, int xmin, int xmax);
char *charge_str(struct object *obj);
/* things.c */
char *inv_name(struct object *obj, BOOL drop);
void money(void);
int drop(struct linked_list *item);
int dropcheck(struct object *op);
struct linked_list *new_thing(BOOL treas, int type, int which);
void basic_init(struct object *cur);
int extras(void);
int pick_one(struct magic_item *mag);
/* trader.c */
void do_post(void);
int price_it(void);
void buy_it(void);
void sell_it(void);
int open_market(void);
int get_worth(struct object *obj);
void trans_line(void);
void do_maze(void);
void draw_maze(void);
char *moffset(int y, int x);
char *foffset(int y, int x);
int findcells(int y, int x);
void rmwall(int newy, int newx, int oldy, int oldx);
void crankout(void);
/* weapons.c */
void missile(int ydelta, int xdelta);
void do_motion(struct object *obj, int ydelta, int xdelta);
void fall(struct linked_list *item, BOOL pr);
void init_weapon(struct object *weap, int type);
int hit_monster(struct coord *mp, struct object *obj);
char *num(int n1, int n2);
void wield(void);
int fallpos(struct coord *pos, struct coord *newpos, BOOL passages);
/* wizard.c */
void whatis(struct linked_list *what);
void create_obj(BOOL fscr, BOOL fromscrolls);
int getbless(void);
int makemons(int what);
int teleport(struct coord spot, struct thing *th);
#if ANDROID || METRO
int passwd(WINDOW *win);
#else
int passwd(void);
#endif
/* xcrypt.c */
char *xcrypt(const char *key, const char *setting);
#ifdef METRO
void Sleep(int ms);
void Log(char *);
#endif


