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

/*
 * Source code compatibility functions for GNOME
 *
 */

#ifndef GNOME_COMPAT_H
#define GNOME_COMPAT_H

#ifndef GNOME_DISABLE_COMPAT_H

#include <gnome-entry.h>

#  define GNOME_IS_MDI_MDI_CHILD(obj)     GNOME_IS_MDI_GENERIC_CHILD(obj)

/*
 * GnomeEntry
 */

const gchar *gnome_entry_get_history_id   (GnomeEntry  *gentry);

void         gnome_entry_set_history_id   (GnomeEntry  *gentry,
					   const gchar *history_id);
const gchar *gnome_entry_get_history_id   (GnomeEntry  *gentry);

void         gnome_entry_set_max_saved    (GnomeEntry  *gentry,
					   guint        max_saved);
guint        gnome_entry_get_max_saved    (GnomeEntry  *gentry);

void         gnome_entry_prepend_history  (GnomeEntry  *gentry,
					   gboolean    save,
					   const gchar *text);
void         gnome_entry_append_history   (GnomeEntry  *gentry,
					   gboolean     save,
					   const gchar *text);
void         gnome_entry_load_history     (GnomeEntry  *gentry);
void         gnome_entry_save_history     (GnomeEntry  *gentry);
void         gnome_entry_clear_history    (GnomeEntry  *gentry);

#endif /* GNOME_DISABLE_COMPAT_H */
#endif

