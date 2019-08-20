# Makefile for Blackguard, rogue component.

HDRS = rogue.h
CFILES = vers.c armor.c chase.c command.c daemon.c daemons.c disply.c encumb.c \
      fight.c global.c init.c io.c list.c main.c misc.c monsters.c move.c \
      new_leve.c options.c pack.c passages.c potions.c pstats.c rings.c rip.c \
      rooms.c save.c scrolls.c state.c sticks.c things.c trader.c weapons.c \
      wizard.c xcrypt.c

CC    = cc
DEBUG_CFLAGS = -I/usr/include/ncurses -DBLACKGUARD -DWIZARD -D_DEBUG -w -g -c
DIST_CFLAGS = -I/usr/include/ncurses -DBLACKGUARD -w -O3 -c

debug: $(HDRS) $(CFILES)
	$(CC) $(DEBUG_CFLAGS) $(CFILES)

dist: $(HDRS) $(CFILES)
	$(CC) $(DIST_CFLAGS) $(CFILES)

clean:
	/bin/rm -f *.o

