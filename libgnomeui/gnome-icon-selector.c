/*
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
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

/* GnomeIconSelector widget - an icon selector widget.
 *
 * Author: Martin Baulig <baulig@suse.de>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkvscrollbar.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-icon-selector.h"
#include "gnome-icon-list.h"
#include "gnome-entry.h"

#define ICON_SIZE 48

struct _GnomeIconSelectorPrivate {
};
	

static void gnome_icon_selector_class_init (GnomeIconSelectorClass *class);
static void gnome_icon_selector_init       (GnomeIconSelector      *fselector);
static void gnome_icon_selector_destroy    (GtkObject       *object);
static void gnome_icon_selector_finalize   (GObject         *object);

static GnomeSelectorClass *parent_class;

guint
gnome_icon_selector_get_type (void)
{
	static guint fselector_type = 0;

	if (!fselector_type) {
		GtkTypeInfo fselector_info = {
			"GnomeIconSelector",
			sizeof (GnomeIconSelector),
			sizeof (GnomeIconSelectorClass),
			(GtkClassInitFunc) gnome_icon_selector_class_init,
			(GtkObjectInitFunc) gnome_icon_selector_init,
			NULL,
			NULL,
			NULL
		};

		fselector_type = gtk_type_unique 
			(gnome_selector_get_type (), &fselector_info);
	}

	return fselector_type;
}

static void
gnome_icon_selector_class_init (GnomeIconSelectorClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gnome_selector_get_type ());

	object_class->destroy = gnome_icon_selector_destroy;
	gobject_class->finalize = gnome_icon_selector_finalize;
}

static void
gnome_icon_selector_init (GnomeIconSelector *fselector)
{
	fselector->_priv = g_new0 (GnomeIconSelectorPrivate, 1);
}

/**
 * gnome_icon_selector_construct:
 * @fselector: Pointer to GnomeIconSelector object.
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Constructs a #GnomeIconSelector object, for language bindings or subclassing
 * use #gnome_icon_selector_new from C
 *
 * Returns: 
 */
void
gnome_icon_selector_construct (GnomeIconSelector *fselector,
			       const gchar *history_id,
			       const gchar *dialog_title,
			       GtkWidget *selector_widget,
			       gboolean is_popup)
{
	g_return_if_fail (fselector != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (fselector));

	gnome_selector_construct (GNOME_SELECTOR (fselector),
				  history_id, dialog_title,
				  selector_widget, is_popup);
}

/**
 * gnome_icon_selector_new
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Description: Creates a new GnomeIconSelector widget.  If  @history_id
 * is not %NULL, then the history list will be saved and restored between
 * uses under the given id.
 *
 * Returns: Newly-created GnomeIconSelector widget.
 */
GtkWidget *
gnome_icon_selector_new (const gchar *history_id,
			 const gchar *dialog_title)
{
	GnomeIconSelector *fselector;
	GtkWidget *box, *sb, *frame, *list;

	fselector = gtk_type_new (gnome_icon_selector_get_type ());

	box = gtk_hbox_new (FALSE, 5);

	sb = gtk_vscrollbar_new (NULL);
	gtk_box_pack_end (GTK_BOX(box), sb, FALSE, FALSE, 0);
	gtk_widget_show (sb);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	list = gnome_icon_list_new (ICON_SIZE+30,
				    gtk_range_get_adjustment(GTK_RANGE(sb)),
				    FALSE);
	gtk_widget_set_usize (list, 350, 300);

	gnome_icon_list_set_selection_mode (GNOME_ICON_LIST (list),
					    GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (frame), list);
	
	gtk_widget_show_all (box);

	gnome_icon_selector_construct (fselector, history_id,
				       dialog_title, box, FALSE);

	return GTK_WIDGET (fselector);
}

GtkWidget *
gnome_icon_selector_new_custom (const gchar *history_id,
				const gchar *dialog_title,
				GtkWidget *selector_widget,
				gboolean is_popup)
{
	GnomeIconSelector *fselector;

	fselector = gtk_type_new (gnome_icon_selector_get_type ());

	gnome_icon_selector_construct (fselector, history_id,
				       dialog_title, selector_widget,
				       is_popup);

	return GTK_WIDGET (fselector);
}


static void
gnome_icon_selector_destroy (GtkObject *object)
{
	GnomeIconSelector *fselector;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (object));

	fselector = GNOME_ICON_SELECTOR (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_icon_selector_finalize (GObject *object)
{
	GnomeIconSelector *fselector;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (object));

	fselector = GNOME_ICON_SELECTOR (object);

	g_free (fselector->_priv);
	fselector->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

