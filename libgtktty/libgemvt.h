/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * libGemVt: include file for GtkLed GtkTerm GtkTty and GtkVtEmu which
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
#ifndef __LIBGEMVT_H__
#define __LIBGEMVT_H__


#include "gtk/gtk.h"

#ifndef __LIBGEMVT_OMIT_INCLUDES__
#define	__LIBGEMVT_OMIT_INCLUDES__

#include "gemvt/gtkled.h"
#include "gemvt/gtkterm.h"
#include "gemvt/gtktty.h"
#include "gemvt/gtkvtemu.h"

#endif	/* __LIBGEMVT_OMIT_INCLUDES__ */


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */




extern	const guint	gemvt_libversion_major;
extern	const guint	gemvt_libversion_revision;
extern	const guint	gemvt_libversion_age;
extern	const gchar	*gemvt_libversion;






#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */


#endif	/* __LIBGEMVT_H__ */
