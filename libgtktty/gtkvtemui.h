/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gtkvtemui: virtual terminal emulation interface for GtkTty
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
#ifndef __GTK_VT_EMU_INTERFACE_H__
#define __GTK_VT_EMU_INTERFACE_H__


#include "gtkvtemu.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */



typedef enum
{
  GTK_VTEMU_ID_FIRST,
  GTK_VTEMU_ID_LINUX,
  /* GTK_VTEMU_ID_XTERM, */
  GTK_VTEMU_ID_LAST
} GtkVtEmuInterfaceId;


typedef guint	(GtkVtEmuInputFunc)	(GtkVtEmu	*vtemu,
					 const guchar	*buffer,
					 guint		count);
typedef void	(GtkVtEmuResetFunc)	(GtkVtEmu	*vtemu,
					 gboolean	blank_screen);


typedef struct  _GtkVtEmuTermEntry      GtkVtEmuTermEntry;

struct  _GtkVtEmuTermEntry
{
  GtkVtEmuInterfaceId	internal_id;
  guint                 vtemu_struct_size;
  GtkVtEmuInputFunc     *input_func;
  GtkVtEmuResetFunc     *reset_func;
  gchar                 **terminal_types;
  guint			n_terminal_types;
};








#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */


#endif /* __GTK_VT_EMU_INTERFACE_H__ */
