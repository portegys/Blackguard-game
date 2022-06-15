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
PROGRAM = Blackguard

FILES =	blackguard/Blackguard.java blackguard/Scene.java Blackguard.cpp ../../rogue/*.h ../../rogue/*.c Makefile \
       ../../rogue/Makefile blackguard.sln blackguard.vcxproj blackguard.vcxproj.filters \
       ../.classpath ../.cproject ../.project ../.settings LICENSE.TXT doc/blackguard.nr

CC    = cc
TAR   = tar

# Windows code is always position independent
SHELL = /bin/sh
OSNAME := $(shell uname -o)
ifeq ($(OSNAME),Cygwin)
PICFLAG =
JAVA_OS = win32
JNILIB = ../bin/$(PROGRAM).dll
JAVA_INCLUDE = -I "$(JAVA_HOME)/include" -I "$(JAVA_HOME)/include/$(JAVA_OS)"
else
PICFLAG = -fPIC
JAVA_OS = linux
JNILIB = ../lib/lib$(PROGRAM).so
JAVA_INCLUDE = -I $(JAVA_HOME)/include -I $(JAVA_HOME)/include/$(JAVA_OS)
endif

LINUX_CFLAGS = $(PICFLAG) $(JAVA_INCLUDE) -I/usr/include/ncurses -I../../include/linux -c
LINUX_LDFLAGS = -shared
LINUX_LINKLIBS = -L../../lib/linux -lfmodex -lncurses -lm -lstdc++

all: debug.cygwin

debug.linux:
	make JAVA_LINUX
	@(cd ../../rogue; make -f Makefile.sharedlib debug.linux)
	$(CC) $(LINUX_CFLAGS) -DWIZARD -D_DEBUG -g $(PROGRAM).cpp
	$(CC) $(LINUX_LDFLAGS) ../../rogue/*.o $(PROGRAM).o $(LINUX_LINKLIBS) -o $(JNILIB)

dist.linux:
	make JAVA_LINUX
	@(cd ../../rogue; make -f Makefile.sharedlib dist.linux)
	$(CC) $(LINUX_CFLAGS) -O3 $(PROGRAM).cpp
	$(CC) $(LINUX_LDFLAGS) ../../rogue/*.o $(PROGRAM).o $(LINUX_LINKLIBS) -o $(JNILIB)
	groff -P-c -t -mm -Tascii doc/$(PROGRAM).nr | sed -e 's/.\x08//g' >../doc/$(PROGRAM).doc
	tar cf $(DISTNAME)-linux.tar ../bin/blackguard/*.class ../lib/*.jar $(JNILIB) ../../lib/linux/*.so ../bin/blackguard.sh LICENSE.TXT ../resource/object_colors.txt ../resource/*/* ../doc/$(PROGRAM).doc
	gzip -f $(DISTNAME)-linux.tar

JAVA_LINUX:
	javac -d ../bin -cp "../lib/jogl.all.jar:../lib/nativewindow.all.jar:../lib/gluegen-rt.jar:../lib/newt.all.jar" blackguard/*.java
	javah -jni -classpath "../bin:../lib/jogl.all.jar:../lib/nativewindow.all.jar:../lib/gluegen-rt.jar:../lib/newt.all.jar" blackguard.$(PROGRAM)
	javah -jni -classpath "../bin:../lib/jogl.all.jar:../lib/nativewindow.all.jar:../lib/gluegen-rt.jar:../lib/newt.all.jar" blackguard.Scene

CYGWIN_CFLAGS = $(PICFLAG) -mno-cygwin -D_WIN32_WINNT=0x0500 $(JAVA_INCLUDE) -I../../include/win32 -c
CYGWIN_LDFLAGS = -mno-cygwin -Wl,--add-stdcall-alias -shared
CYGWIN_LINKLIBS = ../../lib/win32/pdcurses.lib ../../lib/win32/pthreadVC2.lib ../../lib/win32/fmodex_vc.lib -lws2_32 -lm -lstdc++

debug.cygwin:
	make JAVA_CYGWIN
	@(cd ../../rogue; make -f Makefile.sharedlib debug.cygwin)
	$(CC) $(CYGWIN_CFLAGS) -g $(PROGRAM).cpp
	$(CC) $(CYGWIN_LDFLAGS) ../../rogue/*.o $(PROGRAM).o $(CYGWIN_LINKLIBS) -o $(PROGRAM).dll 
	cp $(PROGRAM).dll $(JNILIB)
	make COPY_DLLS

dist.cygwin:
	make JAVA_CYGWIN
	@(cd ../../rogue; make -f Makefile.sharedlib dist.cygwin)
	$(CC) $(CYGWIN_CFLAGS) $(PROGRAM).cpp
	$(CC) $(CYGWIN_LDFLAGS) ../../rogue/*.o $(PROGRAM).o $(CYGWIN_LINKLIBS) -o $(PROGRAM).dll
	cp $(PROGRAM).dll $(JNILIB)
	make COPY_DLLS		
	groff -P-c -t -mm -Tascii doc/$(PROGRAM).nr | sed -e 's/.\x08//g' >../doc/$(PROGRAM).doc
	tar cf $(DISTNAME)-cygwin.tar ../bin/blackguard/*.class ../lib/*.jar ../bin/*.dll ../bin/blackguard.sh ../bin/blackguard.bat LICENSE.TXT ../resource/object_colors.txt ../resource/*/* ../doc/$(PROGRAM).doc
	gzip -f $(DISTNAME)-cygwin.tar

COPY_DLLS:
	cp ../lib/win32/fmodex.dll ../bin/fmodex.dll
	cp ../lib/win32/jogl_es1.dll ../bin/jogl_es1.dll
	cp ../lib/win32/jogl_gl2es12.dll ../bin/jogl_gl2es12.dll
	cp ../lib/win32/newt.dll ../bin/newt.dll
	cp ../lib/win32/gluegen-rt.dll ../bin/gluegen-rt.dll
	cp ../lib/win32/jogl_es2.dll ../bin/jogl_es2.dll
	cp ../lib/win32/nativewindow_awt.dll ../bin/nativewindow_awt.dll
	cp ../lib/win32/pdcurses.dll ../bin/pdcurses.dll
	cp ../lib/win32/jogl_cg.dll ../bin/jogl_cg.dll
	cp ../lib/win32/jogl_gl2.dll ../bin/jogl_gl2.dll
	cp ../lib/win32/nativewindow_jvm.dll ../bin/nativewindow_jvm.dll
	cp ../lib/win32/pthreadVC2.dll ../bin/pthreadVC2.dll

JAVA_CYGWIN:
	javac -d ../bin -cp "../lib/jogl.all.jar;../lib/nativewindow.all.jar;../lib/gluegen-rt.jar;../lib/newt.all.jar" blackguard/*.java
	javah -jni -classpath "../bin;../lib/jogl.all.jar;../lib/nativewindow.all.jar;../lib/gluegen-rt.jar;../lib/newt.all.jar" blackguard.$(PROGRAM)
	javah -jni -classpath "../bin;../lib/jogl.all.jar;../lib/nativewindow.all.jar;../lib/gluegen-rt.jar;../lib/newt.all.jar" blackguard.Scene

dist.src:
	tar cf $(DISTNAME)-src.tar $(FILES) ../../lib/*/* ../resource/object_colors.txt ../resource/*/*
	gzip -f $(DISTNAME)-src.tar

dist.windows:
	groff -P-c -t -mm -Tascii doc/$(PROGRAM).nr | sed -e 's/.\x08//g' >../doc/$(PROGRAM).doc
	tar cf $(DISTNAME).tar ../bin/blackguard/*.class ../lib/*.jar ../bin/*.dll ../bin/blackguard.sh ../bin/blackguard.bat LICENSE.TXT ../resource/object_colors.txt ../resource/*/* ../doc/$(PROGRAM).doc
	mkdir -p work
	mv $(DISTNAME).tar work
	@(cd work; tar xf $(DISTNAME).tar; /bin/rm *.tar; find . -print | zip -o ../$(DISTNAME)-win.zip -@)
	/bin/rm -fr work

clean:
	@(cd ../../rogue; make clean)
	/bin/rm -f *.o ../bin/*.o
	/bin/rm -f *.tar *.gz *.zip ../doc/$(PROGRAM).doc
