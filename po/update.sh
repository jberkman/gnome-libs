#!/bin/sh

xgettext --default-domain=gnome-libs --directory=.. \
  --add-comments --keyword=_ --keyword=N_ \
  --files-from=./POTFILES.in \
&& test ! -f gnome-libs.po \
   || ( rm -f ./gnome-libs.pot \
    && mv gnome-libs.po ./gnome-libs.pot )
