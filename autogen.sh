#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

if test -z "$CERTIFIED_GNOMIE"; then
# If you can't figure out how to set this envar, please don't come on IRC
# whining incessantly about how your apps are broken.
  cat $srcdir/message-of-doom
  exit 1
fi

PKG_NAME="Gnome Libraries"

(test -f $srcdir/configure.in \
  && test -f $srcdir/HACKING \
  && test -d $srcdir/libgnomeui) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level gnome directory"
    exit 1
}

. $srcdir/macros/autogen.sh
