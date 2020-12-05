#!/bin/sh

aclocal -I acinclude
#libtoolize -c -i
autoheader -f
automake --foreign --include-deps --add-missing --copy
autoconf

echo "Now you are ready to run ./configure"
