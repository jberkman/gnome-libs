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

#ifndef __GNOME_GEOMETRY_H_
#define __GNOME_GEOMETRY_H_ 

#include <libgnome/gnome-defs.h>
#include <gdk/gdk.h>

BEGIN_GNOME_DECLS

/* Return TRUE on success */
gboolean gnome_parse_geometry (const gchar *geometry, gint *xpos, gint *ypos, 
			       gint *width, gint *height);

/* Return a g_malloc'd string representing the window's geometry, suitable
   for parsing by gnome_parse_geometry. */
gchar * gnome_geometry_string (GdkWindow * window);

END_GNOME_DECLS

#endif
