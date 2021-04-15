#!/bin/sh

aclocal -I acinclude
# we have customizations in libtool files
#libtoolize -c -i
autoheader -f
automake --foreign --include-deps --add-missing --copy
autoconf

echo "Now you are ready to run ./configure"
