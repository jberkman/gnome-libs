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

DIE=0

if [ -n "$GNOME2_DIR" ]; then
  ACLOCAL_FLAGS="-I $GNOME2_DIR/share/aclocal $ACLOCAL_FLAGS"
  PATH="$GNOME2_DIR/bin:$PATH"
  LD_LIBRARY_PATH="$GNOME2_DIR/lib:$LD_LIBRARY_PATH"
  export PATH
  export LD_LIBRARY_PATH
fi

# This is a bit complicated here since we can't use gnome-config yet.
# It'll be easier after switching to pkg-config since we can then
# use pkg-config to find the gnome-autogen.sh script.

gnome_autogen=

ifs_save="$IFS"; IFS=":"
for dir in $PATH ; do
  test -z "$dir" && dir=.
  if test -f $dir/gnome-autogen.sh ; then
    gnome_autogen="$dir/gnome-autogen.sh"
    break
  fi
done
IFS="$ifs_save"

if test -z "$gnome_autogen" ; then
  echo "You need to install the gnome-common module and make"
  echo "sure the gnome-autogen.sh script is in your \$PATH."
  exit 1
fi

USE_GNOME2_MACROS=1 . $gnome_autogen
