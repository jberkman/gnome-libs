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
#include <config.h>
#include "gtkvtemui.h"



/* --- terminal emulations --- */
#include "gtkvt102.h"


/* --- VT Emulation list --- */
static	GtkVtEmuTermEntry	*gtk_vtemu_entries[] =
{
  &gtk_vt102_entry	/* vt102 / vt100 / linux */,
};
static	guint	gtk_vtemu_n_entries = sizeof (gtk_vtemu_entries) / sizeof (GtkVtEmuTermEntry*);


/* --- prototypes --- */

/* --- functions --- */
GList*
gtk_vtemu_create_type_list (void)
{
  guint i;
  GList *list;
  
  list = NULL;
  i = gtk_vtemu_n_entries;
  do
  {
    GtkVtEmuTermEntry *entry;
    guint n;

    i--;
    entry = gtk_vtemu_entries[i];
    n = entry->n_terminal_types;
    do
    {
      n--;
      list = g_list_prepend (list, entry->terminal_types[n]);
    }
    while (n > 0);
  }
  while (i > 0);
  
  /*  list = g_list_reverse (list); */
  
  return list;
}

GtkVtEmu*
gtk_vtemu_new (GtkTerm		*term,
	       const gchar	*terminal_type)
{
  GtkVtEmuTermEntry *entry;
  GtkVtEmu *vtemu;
  guint alias_index = 0;
  guint i;
  
  g_return_val_if_fail (term != NULL, NULL);
  g_return_val_if_fail (GTK_IS_TERM (term), NULL);
  g_return_val_if_fail (terminal_type != NULL, NULL);

  entry = NULL;
  for (i = 0; i < gtk_vtemu_n_entries; i++)
  {
    for (alias_index = 0; alias_index < gtk_vtemu_entries[i]->n_terminal_types; alias_index++)
      if (g_str_equal (gtk_vtemu_entries[i]->terminal_types[alias_index], (gpointer) terminal_type))
      {
	entry = gtk_vtemu_entries[i];
	break;
      }
    if (entry)
      break;
  }
  
  if (!entry)
  {
    g_warning ("Failed to lookup terminal type `%s'", terminal_type);
    return NULL;
  }
  
  g_assert (entry->internal_id > GTK_VTEMU_ID_FIRST && entry->internal_id < GTK_VTEMU_ID_LAST);
  g_assert (entry->vtemu_struct_size >= sizeof (GtkVtEmu));
  
  vtemu = g_malloc0 (entry->vtemu_struct_size);
  
  vtemu->term = term;
  vtemu->internal_id = entry->internal_id;
  vtemu->internal_index = i;
  vtemu->terminal_aliases = entry->terminal_types;
  vtemu->n_terminal_aliases = entry->n_terminal_types;
  vtemu->terminal_type = vtemu->terminal_aliases[alias_index];
  vtemu->led_states = 0;
  vtemu->led_override = FALSE;
  vtemu->cur_x = 0;
  vtemu->cur_y = 0;
  vtemu->dfl_cursor_mode = GTK_CURSOR_UNDERLINE;
  vtemu->dfl_cursor_blinking = TRUE;
  vtemu->insert_mode = FALSE;
  vtemu->lf_plus_cr = FALSE;
  vtemu->need_wrap = FALSE;
  vtemu->term_inverted = FALSE;
  vtemu->reporter = NULL;
  vtemu->reporter_data = NULL;
  
  gtk_vtemu_reset (vtemu, FALSE);
  
  return vtemu;
}

guint
gtk_vtemu_input (GtkVtEmu	*vtemu,
		 const guchar	*buffer,
		 guint		count)
{
  guint return_val;
  
  g_return_val_if_fail (vtemu != NULL, 0);
  g_return_val_if_fail (vtemu->internal_id > GTK_VTEMU_ID_FIRST, 0);
  g_return_val_if_fail (vtemu->internal_id < GTK_VTEMU_ID_LAST, 0);
  g_return_val_if_fail (vtemu->internal_index < gtk_vtemu_n_entries, 0);
  
  return_val = (* gtk_vtemu_entries[vtemu->internal_index]->input_func) (vtemu, buffer, count);
  
  return return_val;
}

void
gtk_vtemu_report (GtkVtEmu		 *vtemu,
		  const guchar		 *buffer,
		  guint			 count)
{
  g_return_if_fail (vtemu != NULL);
  g_return_if_fail (vtemu->internal_id > GTK_VTEMU_ID_FIRST);
  g_return_if_fail (vtemu->internal_id < GTK_VTEMU_ID_LAST);
  
  if (vtemu->reporter)
    (*vtemu->reporter) (vtemu, buffer, count, vtemu->reporter_data);
}

void
gtk_vtemu_set_reporter (GtkVtEmu	       *vtemu,
			GtkVtEmuReporter       *callback,
			gpointer	       user_data)
{
  g_return_if_fail (vtemu != NULL);
  g_return_if_fail (vtemu->internal_id > GTK_VTEMU_ID_FIRST);
  g_return_if_fail (vtemu->internal_id < GTK_VTEMU_ID_LAST);
  
  vtemu->reporter = callback;
  vtemu->reporter_data = user_data;
}

void
gtk_vtemu_reset (GtkVtEmu		*vtemu,
		 gboolean		blank_screen)
{
  g_return_if_fail (vtemu != NULL);
  g_return_if_fail (vtemu->internal_id > GTK_VTEMU_ID_FIRST);
  g_return_if_fail (vtemu->internal_id < GTK_VTEMU_ID_LAST);
  g_return_if_fail (vtemu->internal_index < gtk_vtemu_n_entries);
  
  (* gtk_vtemu_entries[vtemu->internal_index]->reset_func) (vtemu, blank_screen != FALSE);
}

void
gtk_vtemu_invert (GtkVtEmu               *vtemu)
{
  g_return_if_fail (vtemu != NULL);
  g_return_if_fail (vtemu->internal_id > GTK_VTEMU_ID_FIRST);
  g_return_if_fail (vtemu->internal_id < GTK_VTEMU_ID_LAST);

  vtemu->term_inverted = ! vtemu->term_inverted;
  gtk_term_invert (vtemu->term);
}

void
gtk_vtemu_destroy (GtkVtEmu		  *vtemu)
{
  g_return_if_fail (vtemu != NULL);
  g_return_if_fail (vtemu->internal_id > GTK_VTEMU_ID_FIRST);
  g_return_if_fail (vtemu->internal_id < GTK_VTEMU_ID_LAST);
  
  g_free (vtemu);
}
