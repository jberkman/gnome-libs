/*
 * Handles all of the internationalization configuration options.
 * Author: Tom Tromey <tromey@creche.cygnus.com>
 */

#ifndef __GNOME_I18N_H__
#define __GNOME_I18N_H__

#ifdef  __GNOME_I18NP_H__
#warning "You should use either gnome-i18n.h OR gnome-i18nP.h"
#endif

BEGIN_GNOME_DECLS

#ifdef ENABLE_NLS
#    include <libintl.h>
#    define _(String) gettext (String)
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

const char *gnome_i18n_get_language(void);

/* 'gnome_i18n_get_language_list' returns a list of language strings.
 *
 * It searches for one of following environment variables:
 * LANGUAGE
 * LC_ALL
 * 'category_name'
 * LANG
 *
 * If one of these environment variables was found, it is split into
 * pieces, whereever a ':' is found. When the environment variable included
 * no C locale, the C locale is appended to the list of languages.
 *
 * Assume, you have the following environment variables set:
 *
 * LC_MONETARY="de_DE:es"
 * LANG="de_DE:de:C:en"
 * 
 * In this case 'gnome_i18n_get_language_list ("LC_COLLATE")' returns the
 * list: ("de_DE" "de" "C" "en").
 *
 * 'gnome_i18n_get_language_list ("LC_MONETARY")' returns:
 * ("de_DE" "es" "C")
 *
 * The returned list must not be changed.
 */

GList      *gnome_i18n_get_language_list (const gchar *category_name);

END_GNOME_DECLS

#endif /* __GNOME_UTIL_H__ */
