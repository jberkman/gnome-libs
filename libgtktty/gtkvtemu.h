/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gtkvtemu: virtual terminal emulation for GtkTty
 * Copyright (C) 1997 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GTK_VT_EMU_H__
#define __GTK_VT_EMU_H__


#include "gtkterm.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */



typedef	struct	_GtkVtEmu		GtkVtEmu;
typedef void    (GtkVtEmuReporter)      (GtkVtEmu       *vtemu,
					 const guchar   *buffer,
					 guint          count,
					 gpointer       user_data);


struct	_GtkVtEmu
{
  GtkTerm		*term;
  
  guint			internal_id;
  guint			internal_index;
  
  gchar			*terminal_type;
  gchar			**terminal_aliases;
  guint			n_terminal_aliases;
  
  guint32		led_states;
  gboolean		led_override;
  guint			cur_x;
  guint			cur_y;
  GtkCursorMode		dfl_cursor_mode;
  gboolean		dfl_cursor_blinking;
  gboolean		insert_mode;
  gboolean		lf_plus_cr;
  gboolean		need_wrap;
  gboolean		term_inverted;
  
  GtkVtEmuReporter	*reporter;
  gpointer		reporter_data;
};


GList*		gtk_vtemu_create_type_list	(void);

GtkVtEmu*	gtk_vtemu_new		(GtkTerm		*term,
					 const gchar		*terminal_type);
guint		gtk_vtemu_input		(GtkVtEmu		*vtemu,
					 const guchar		*buffer,
					 guint			count);
void		gtk_vtemu_report	(GtkVtEmu		*vtemu,
					 const guchar		*buffer,
					 guint			count);
void		gtk_vtemu_set_reporter	(GtkVtEmu		*vtemu,
					 GtkVtEmuReporter	*callback,
					 gpointer		user_data);
void		gtk_vtemu_reset		(GtkVtEmu		*vtemu,
					 gboolean		blank_screen);
void		gtk_vtemu_invert	(GtkVtEmu		*vtemu);
void		gtk_vtemu_destroy	(GtkVtEmu		*vtemu);






#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */


#endif /* __GTK_VT_EMU_H__ */
