/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gtkvt102: VT102 implementation for GtkVtEmu
 implementation * gtkvtemui: virtual terminal emulation interface for GtkTty
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
#ifndef __GTK_VT102_H__
#define __GTK_VT102_H__


#include "gtkvtemui.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


extern	GtkVtEmuTermEntry       gtk_vt102_entry;

guint   gtk_vt102_input         (GtkVtEmu       *base_emu,
				 const guchar   *buffer,
				 guint          count);
void	gtk_vt102_reset		(GtkVtEmu       *base_emu,
				 gboolean       blank_screen);






#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */


#endif /* __GTK_VT102_H__ */
