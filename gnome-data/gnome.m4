AC_DEFUN([GNOME_INIT],
[	
	AC_SUBST(GNOME_LIBS)
	AC_SUBST(GNOMEUI_LIBS)
	AC_SUBST(GTKXMHTML_LIBS)
	AC_SUBST(GNOME_LIBDIR)
	AC_SUBST(GNOME_INCLUDEDIR)

	if test x$exec_prefix = xNONE; then
	    if test x$prefix = xNONE; then
	        gnome_prefix=$ac_default_prefix/lib
	    else
		gnome_prefix=$prefix/lib
	    fi
	else
	    gnome_prefix=`eval echo \`echo $libdir\``
	fi

	AC_ARG_WITH(gnome-includes,
	[--with-gnome-includes	Specify location of GNOME headers],[
	CFLAGS="$CFLAGS -I$withval"
	])

	AC_ARG_WITH(gnome-libs,
	[--with-gnome-libs	Specify location of GNOME libs],[
	LDFLAGS="$LDFLAGS -L$withval"
	gnome_prefix=$withval
	])

	AC_ARG_WITH(gnome,
	[--with-gnome		Specify prefix for GNOME files],[
	LDFLAGS="$LDFLAGS -L$withval/lib"
	CFLAGS="$CFLAGS -I$withval/include"
	gnome_prefix=$withval/lib
	])
	

        AC_MSG_CHECKING(for gnomeConf.sh file in $gnome_prefix)
	if test -f $gnome_prefix/gnomeConf.sh; then
	    AC_MSG_RESULT(found)
	    echo "loading gnome configuration from $gnome_prefix/gnomeConf.sh"
	    . $gnome_prefix/gnomeConf.sh
	else
	    AC_MSG_RESULT(not found)
	    AC_MSG_ERROR(Could not find the gnomeConf.sh file that is generated by gnome-libs install)
	fi
])
