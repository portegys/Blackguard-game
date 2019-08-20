# Makefile for Blackguard
# %W% (Berkeley) %G%
#
# Blackguard
# Copyright (C) 2010 Tom Portegys
# All rights reserved.
#
# Based on Super-Rogue
# Copyright (C) 1984 Robert D. Kindelberger
# All rights reserved.
#
# Based on "Rogue: Exploring the Dungeons of Doom"
# Copyright (C) 1980, 1981 Michael Toy, Ken Arnold and Glenn Wichman
# All rights reserved.
#
# See the file LICENSE.TXT for full copyright and licensing information.

DISTNAME = blackguard_1_0
PROGRAM = blackguard
TARGET = ../bin/blackguard

FILES =	blackguard.cpp ../../rogue/*.h ../../rogue/*.c scene/*.h scene/*.hpp scene/*.cpp Makefile \
       ../../rogue/Makefile scene/Makefile blackguard.sln blackguard.vcxproj blackguard.vcxproj.filters LICENSE.TXT doc/blackguard.nr

CC    = cc
CFLAGS = -I/usr/include/ncurses -I../../include/linux -c
LINKLIBS = -lncurses -lglut -lGLU -lGL -lm -lpthread -lstdc++
TAR   = tar

all: debug.linux

debug.linux:
	make DEBUG_OBJECTS
	$(CC) -DFMOD_UPDATE $(CFLAGS) -DWIZARD -D_DEBUG -g $(PROGRAM).cpp
	$(CC) ../../rogue/*.o scene/*.o $(PROGRAM).o -L../../lib/linux -lfmod $(LINKLIBS) -o $(TARGET)

dist.linux:
	make DIST_OBJECTS
	$(CC) -DFMOD_UPDATE $(CFLAGS) -O3 $(PROGRAM).cpp
	$(CC) ../../rogue/*.o scene/*.o $(PROGRAM).o -L../../lib/linux -lfmod $(LINKLIBS) -o $(TARGET)
	groff -P-c -t -mm -Tascii doc/$(PROGRAM).nr | sed -e 's/.\x08//g' >../doc/$(PROGRAM).doc
	tar cf $(DISTNAME)-linux.tar $(TARGET) ../../lib/linux/*.so LICENSE.TXT ../resource/object_colors.txt ../resource/*/* ../doc/$(PROGRAM).doc
	gzip -f $(DISTNAME)-linux.tar

dist.src:
	tar cf $(DISTNAME)-src.tar $(FILES) ../../lib/*/* ../resource/object_colors.txt ../resource/*/*
	gzip -f $(DISTNAME)-src.tar

DEBUG_OBJECTS:
	@(cd ../../rogue; make debug)
	@(cd scene; make debug)

DIST_OBJECTS:
	@(cd ../../rogue; make dist)
	@(cd scene; make dist)

clean:
	@(cd ../../rogue; make clean)
	@(cd scene; make clean)
	/bin/rm -f $(PROGRAM).o core
	/bin/rm -f $(PROGRAM).tar $(PROGRAM).tar.gz ../doc/$(PROGRAM).doc
