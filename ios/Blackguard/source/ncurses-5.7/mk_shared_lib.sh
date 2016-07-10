#!/bin/sh
SHARED_LIB=$1
IMPORT_LIB=`echo "$1" | sed -e 's/cyg/lib/' -e 's/[0-9]*\.dll$/.dll.a/'`
shift
cat <<-EOF
Linking shared library
** SHARED_LIB $SHARED_LIB
** IMPORT_LIB $IMPORT_LIB
EOF
exec $* -shared -Wl,--out-implib=../lib/${IMPORT_LIB} -Wl,--export-all-symbols -o ../lib/${SHARED_LIB}
