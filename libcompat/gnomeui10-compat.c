/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#include <gnomeui10-compat.h>

/* It's not as if there's anything to license here... */



/* ...but it's always possible someone would want to steal my "three
   lines of silence" rendition */

/*
 * GnomeEntry
 *

/**
 * gnome_entry_set_history_id
 * @gentry: Pointer to GnomeEntry object.
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Description: Set or clear the history id of the GnomeEntry widget.  If
 * @history_id is %NULL, the widget's history id is cleared.  Otherwise,
 * the given id replaces the previous widget history id.
 *
 * Returns:
 */
void
gnome_entry_set_history_id (GnomeEntry *gentry, const gchar *history_id)
{
    g_return_if_fail (gentry != NULL);
    g_return_if_fail (GNOME_IS_ENTRY (gentry));

    g_warning (G_STRLOC ": This function is deprecated, use "
	       "gnome_selector_set_history_id().");

    gnome_selector_set_history_id (GNOME_SELECTOR (gentry), history_id);
}


/**
 * gnome_entry_get_history_id
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Returns the current history id of the GnomeEntry widget.
 *
 * Returns: The current history id.
 */
const gchar *
gnome_entry_get_history_id (GnomeEntry *gentry)
{
    g_return_val_if_fail (gentry != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);

    g_warning (G_STRLOC ": This function is deprecated, use "
	       "gnome_selector_get_history_id().");

    return gnome_selector_get_history_id (GNOME_SELECTOR (gentry));
}

/**
 * gnome_entry_set_max_saved
 * @gentry: Pointer to GnomeEntry object.
 * @max_saved: Maximum number of history items to save
 *
 * Description: Set internal limit on number of history items saved
 * to the config file, when #gnome_entry_save_history() is called.
 * Zero is an acceptable value for @max_saved, but the same thing is
 * accomplished by setting the history id of @gentry to %NULL.
 *
 * Returns:
 */
void
gnome_entry_set_max_saved (GnomeEntry *gentry, guint max_saved)
{
    g_return_if_fail (gentry != NULL);
    g_return_if_fail (GNOME_IS_ENTRY (gentry));

    g_warning (G_STRLOC ": This function is deprecated, use "
	       "gnome_selector_set_history_length().");

    gnome_selector_set_history_length (GNOME_SELECTOR (gentry), max_saved);
}

/**
 * gnome_entry_get_max_saved
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Get internal limit on number of history items saved
 * to the config file, when #gnome_entry_save_history() is called.
 * See #gnome_entry_set_max_saved().
 *
 * Returns: An unsigned integer
 */
guint
gnome_entry_get_max_saved (GnomeEntry *gentry)
{
    g_return_val_if_fail (gentry != NULL, 0);
    g_return_val_if_fail (GNOME_IS_ENTRY (gentry), 0);

    g_warning (G_STRLOC ": This function is deprecated, use "
	       "gnome_selector_get_history_length().");

    return gnome_selector_get_history_length (GNOME_SELECTOR (gentry));
}


/**
 * gnome_entry_prepend_history
 * @gentry: Pointer to GnomeEntry object.
 * @save: If %TRUE, history entry will be saved to config file
 * @text: Text to add
 *
 * Description: Adds a history item of the given @text to the head of
 * the history list inside @gentry.  If @save is %TRUE, the history
 * item will be saved in the config file (assuming that @gentry's
 * history id is not %NULL).
 *
 * Returns:
 */
void
gnome_entry_prepend_history (GnomeEntry *gentry, gboolean save,
			     const gchar *text)
{
    g_return_if_fail (gentry != NULL);
    g_return_if_fail (GNOME_IS_ENTRY (gentry));

    g_warning (G_STRLOC ": This function is deprecated, use "
	       "gnome_selector_prepend_history().");

    gnome_selector_prepend_history (GNOME_SELECTOR (gentry), save, text);
}


/**
 * gnome_entry_append_history
 * @gentry: Pointer to GnomeEntry object.
 * @save: If %TRUE, history entry will be saved to config file
 * @text: Text to add
 *
 * Description: Adds a history item of the given @text to the tail
 * of the history list inside @gentry.  If @save is %TRUE, the
 * history item will be saved in the config file (assuming that
 * @gentry's history id is not %NULL).
 *
 * Returns:
 */
void
gnome_entry_append_history (GnomeEntry *gentry, gboolean save,
			    const gchar *text)
{
    g_return_if_fail (gentry != NULL);
    g_return_if_fail (GNOME_IS_ENTRY (gentry));

    g_warning (G_STRLOC ": This function is deprecated, use "
	       "gnome_selector_append_history().");

    gnome_selector_append_history (GNOME_SELECTOR (gentry), save, text);
}


/**
 * gnome_entry_load_history
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Loads a stored history list from the GNOME config file,
 * if one is available.  If the history id of @gentry is %NULL,
 * nothing occurs.
 *
 * Returns:
 */
void
gnome_entry_load_history (GnomeEntry *gentry)
{
    g_return_if_fail (gentry != NULL);
    g_return_if_fail (GNOME_IS_ENTRY (gentry));

    g_warning (G_STRLOC ": This function is deprecated, use "
	       "gnome_selector_load_history().");

    gnome_selector_load_history (GNOME_SELECTOR (gentry));
}


/**
 * gnome_entry_clear_history
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description:  Clears the history, you should call #gnome_entry_save_history
 * To make the change permanent.
 *
 * Returns:
 */
void
gnome_entry_clear_history (GnomeEntry *gentry)
{
    g_return_if_fail (gentry != NULL);
    g_return_if_fail (GNOME_IS_ENTRY (gentry));

    g_warning (G_STRLOC ": This function is deprecated, use "
	       "gnome_selector_clear_history().");

    gnome_selector_clear_history (GNOME_SELECTOR (gentry));
}


/**
 * gnome_entry_save_history
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Force the history items of the widget to be stored
 * in a configuration file.  If the history id of @gentry is %NULL,
 * nothing occurs.
 *
 * Returns:
 */
void
gnome_entry_save_history (GnomeEntry *gentry)
{
    g_return_if_fail (gentry != NULL);
    g_return_if_fail (GNOME_IS_ENTRY (gentry));

    g_warning (G_STRLOC ": This function is deprecated, use "
	       "gnome_selector_clear_history().");

    gnome_selector_save_history (GNOME_SELECTOR (gentry));
}

/*
 * GnomeIconEntry
 *

/**
 * gnome_icon_entry_set_pixmap_subdir:
 * @ientry: the GnomeIconEntry to work with
 * @subdir: subdirectory
 *
 * Description: Sets the subdirectory below gnome's default
 * pixmap directory to use as the default path for the file
 * entry.  The path can also be an absolute one.  If %NULL is passed
 * then the pixmap directory itself is used.
 *
 * Returns:
 **/
void
gnome_icon_entry_set_pixmap_subdir (GnomeIconEntry *ientry,
				    const gchar *subdir)
{
    g_return_if_fail (ientry != NULL);
    g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

    g_warning (G_STRLOC ": this function is deprecated and has "
	       "no effect at all.");
}

/**
 * gnome_icon_entry_set_filename:
 * @ientry: the GnomeIconEntry to work with
 * @filename: a filename
 * 
 * Description: Sets the icon of GnomeIconEntry to be the one pointed to by
 * @filename (in the current subdirectory).
 *
 * Returns: %TRUE if icon was loaded ok, %FALSE otherwise
 **/
gboolean
gnome_icon_entry_set_filename (GnomeIconEntry *ientry,
			       const gchar *filename)
{
	g_return_val_if_fail (ientry != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), FALSE);

	g_warning (G_STRLOC ": this function is deprecated; use "
		   "gnome_selector_set_uri().");

	gnome_selector_set_uri (GNOME_SELECTOR (ientry), NULL,
				filename, NULL, NULL);

	return FALSE;
}

/**
 * gnome_icon_entry_get_filename:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Gets the file name of the image if it was possible
 * to load it into the preview. That is, it will only return a filename
 * if the image exists and it was possible to load it as an image.
 *
 * Returns: a newly allocated string with the path or %NULL if it
 * couldn't load the file
 **/
gchar *
gnome_icon_entry_get_filename (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL,NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry),NULL);

	g_warning (G_STRLOC ": this function is deprecated; use "
		   "gnome_selector_get_uri().");

	return gnome_selector_get_uri (GNOME_SELECTOR (ientry));
}

/**
 * gnome_appbar_set_progress
 *
 * Description:
 * Deprecated, use #gnome_appbar_set_progress_percentage
 **/

void
gnome_appbar_set_progress (GnomeAppBar *appbar, gfloat percentage)
{
    g_warning(G_STRLOC ": this function is deprecated; use "
	      "gnome_appbar_set_progress_percentage().");

    gnome_appbar_set_progress_percentage (appbar, percentage);
}
