/*
 * Handles i18n for the Gnome libraries. Libraries need to use
 * dgettext in order to use a non-default translation domain.
 * Author: Tom Tromey <tromey@creche.cygnus.com>
 */

#ifndef __GNOME_I18NP_H__
#define __GNOME_I18NP_H__ 1

#ifdef ENABLE_NLS
#    include <libintl.h>
#    undef _
#    define _(String) dgettext (PACKAGE, String)
#    ifdef gettext_noop
#        define N_(String) gettext_noop (String)
#    else
#        define N_(String) (String)
#    endif
#else
/* Stubs that do something close enough.  */
#    define textdomain(String) (String)
#    define gettext(String) (String)
#    define dgettext(Domain,Message) (Message)
#    define dcgettext(Domain,Message,Type) (Message)
#    define bindtextdomain(Domain,Directory) (Domain)
#    define _(String) (String)
#    define N_(String) (String)
#endif

#include "libgnome/gnome-i18n.h"

#endif /* __GNOME_I18NP_H__ */
