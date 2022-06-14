From: http://credentiality2.blogspot.com/2010/08/compile-ncurses-for-android.html

Make sure both agcc and arm-eabi-gcc are in your path.

export CC=agcc
./configure --host=arm-eabi --without-cxx-binding --with-fallbacks=linux
make

When I tried to build, I got errors saying "'struct lconv' has no member named 'decimal_point'"",
because android has a broken locale implementation.

So I commented out the HAVE_LOCALE_H line from include/ncurses_cfg.h. (Is there a better way to
force configure to set a value like that during the ./configure process?)
