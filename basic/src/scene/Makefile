# Makefile for Blackguard, scene component.

HDRS= gettime.h glbmp.h scene.hpp texture.h vector.hpp
CFILES= gettime.cpp glbmp.cpp scene.cpp texture.cpp

CC    = gcc
DEBUG_CFLAGS= -DWIZARD -D_DEBUG -g -c
DIST_CFLAGS= -O3 -c

debug: $(HDRS) $(CFILES)
	$(CC) $(DEBUG_CFLAGS) $(CFILES)

dist: $(HDRS) $(CFILES)
	$(CC) $(DIST_CFLAGS) $(CFILES)

clean:
	/bin/rm -f *.o core

