/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * LibGtkTty: include file for GtkLed GtkTerm GtkTty and GtkVtEmu which
 * are distributed apart from Gtk+ for the moment.
 * Copyright (C) 1997 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __LIBGTKTTY_H__
#define __LIBGTKTTY_H__


#include "gtk/gtk.h"

#ifndef __LIBGTKTTY_OMIT_INCLUDES__
#define	__LIBGTKTTY_OMIT_INCLUDES__

#include "gtktty/gtkled.h"
#include "gtktty/gtkterm.h"
#include "gtktty/gtktty.h"
#include "gtktty/gtkvtemu.h"

#endif	/* __LIBGTKTTY_OMIT_INCLUDES__ */


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */




extern	const guint	gtktty_libversion_major;
extern	const guint	gtktty_libversion_revision;
extern	const guint	gtktty_libversion_age;
extern	const gchar	*gtktty_libversion;






#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */


#endif	/* __LIBGTKTTY_H__ */
