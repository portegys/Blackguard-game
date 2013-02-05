# Build libBlackguard.so:
# export NDK_PROJECT_PATH=<android directory>
# cd $NDK_PROJECT_PATH
# <sdk>/android-ndk-<ver>/ndk-build

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := Blackguard

LOCAL_SRC_FILES := Blackguard.cpp \
  ../../rogue/armor.c \
  ../../rogue/chase.c \
  ../../rogue/command.c \
  ../../rogue/daemon.c \
  ../../rogue/daemons.c \
  ../../rogue/disply.c \
  ../../rogue/encumb.c \
  ../../rogue/fight.c \
  ../../rogue/global.c \
  ../../rogue/init.c \
  ../../rogue/io.c \
  ../../rogue/list.c \
  ../../rogue/main.c \
  ../../rogue/misc.c \
  ../../rogue/monsters.c \
  ../../rogue/move.c \
  ../../rogue/new_leve.c \
  ../../rogue/options.c \
  ../../rogue/pack.c \
  ../../rogue/passages.c \
  ../../rogue/potions.c \
  ../../rogue/pstats.c \
  ../../rogue/rings.c \
  ../../rogue/rip.c \
  ../../rogue/rooms.c \
  ../../rogue/save.c \
  ../../rogue/scrolls.c \
  ../../rogue/state.c \
  ../../rogue/sticks.c \
  ../../rogue/things.c \
  ../../rogue/trader.c \
  ../../rogue/vers.c \
  ../../rogue/weapons.c \
  ../../rogue/wizard.c \
  ../../rogue/xcrypt.c

LOCAL_CFLAGS := -DBLACKGUARD

LOCAL_C_INCLUDES := ncurses-5.7/include

LOCAL_LDLIBS := ncurses-5.7/lib/libncurses.a -llog

include $(BUILD_SHARED_LIBRARY)
