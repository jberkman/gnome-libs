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
#include <gtk/gtkmain.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkvscrollbar.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-selectorP.h"
#include "gnome-icon-selector.h"
#include "gnome-icon-list.h"
#include "gnome-entry.h"

#define ICON_SIZE 48

struct _GnomeIconSelectorPrivate {
	GnomeIconList *icon_list;

	/* a flag set to stop the loading of images in midprocess */
	gboolean stop_loading;
};
	

static void gnome_icon_selector_class_init (GnomeIconSelectorClass *class);
static void gnome_icon_selector_init       (GnomeIconSelector      *iselector);
static void gnome_icon_selector_destroy    (GtkObject       *object);
static void gnome_icon_selector_finalize   (GObject         *object);

static gboolean  check_filename            (GnomeSelector   *selector,
                                            const gchar     *filename);
static void      update_filelist           (GnomeSelector   *selector);


static GnomeSelectorClass *parent_class;

guint
gnome_icon_selector_get_type (void)
{
	static guint iselector_type = 0;

	if (!iselector_type) {
		GtkTypeInfo iselector_info = {
			"GnomeIconSelector",
			sizeof (GnomeIconSelector),
			sizeof (GnomeIconSelectorClass),
			(GtkClassInitFunc) gnome_icon_selector_class_init,
			(GtkObjectInitFunc) gnome_icon_selector_init,
			NULL,
			NULL,
			NULL
		};

		iselector_type = gtk_type_unique 
			(gnome_selector_get_type (), &iselector_info);
	}

	return iselector_type;
}

static void
gnome_icon_selector_class_init (GnomeIconSelectorClass *class)
{
	GnomeSelectorClass *selector_class;
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	selector_class = (GnomeSelectorClass *) class;
	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gnome_selector_get_type ());

	object_class->destroy = gnome_icon_selector_destroy;
	gobject_class->finalize = gnome_icon_selector_finalize;

	selector_class->check_filename = check_filename;
	selector_class->update_filelist = update_filelist;
}

static void
gnome_icon_selector_init (GnomeIconSelector *iselector)
{
	iselector->_priv = g_new0 (GnomeIconSelectorPrivate, 1);
}

/**
 * gnome_icon_selector_construct:
 * @iselector: Pointer to GnomeIconSelector object.
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Constructs a #GnomeIconSelector object, for language bindings or subclassing
 * use #gnome_icon_selector_new from C
 *
 * Returns: 
 */
void
gnome_icon_selector_construct (GnomeIconSelector *iselector,
			       const gchar *history_id,
			       const gchar *dialog_title,
			       GtkWidget *selector_widget,
			       gboolean is_popup)
{
	g_return_if_fail (iselector != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (iselector));

	gnome_selector_construct (GNOME_SELECTOR (iselector),
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
	GnomeIconSelector *iselector;
	GtkWidget *box, *sb, *frame, *list;

	iselector = gtk_type_new (gnome_icon_selector_get_type ());

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

	iselector->_priv->icon_list = GNOME_ICON_LIST (list);
	
	gtk_widget_show_all (box);

	gnome_icon_selector_construct (iselector, history_id,
				       dialog_title, box, FALSE);

	return GTK_WIDGET (iselector);
}

GtkWidget *
gnome_icon_selector_new_custom (const gchar *history_id,
				const gchar *dialog_title,
				GtkWidget *selector_widget,
				gboolean is_popup)
{
	GnomeIconSelector *iselector;

	iselector = gtk_type_new (gnome_icon_selector_get_type ());

	gnome_icon_selector_construct (iselector, history_id,
				       dialog_title, selector_widget,
				       is_popup);

	return GTK_WIDGET (iselector);
}


static void
gnome_icon_selector_destroy (GtkObject *object)
{
	GnomeIconSelector *iselector;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (object));

	iselector = GNOME_ICON_SELECTOR (object);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_icon_selector_finalize (GObject *object)
{
	GnomeIconSelector *iselector;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_SELECTOR (object));

	iselector = GNOME_ICON_SELECTOR (object);

	g_free (iselector->_priv);
	iselector->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static gboolean
check_filename (GnomeSelector *selector, const gchar *filename)
{
	const char *mimetype;

	mimetype = gnome_mime_type (filename);
	if (!mimetype || strncmp (mimetype, "image", sizeof("image")-1))
		return FALSE;
	else
		return TRUE;

	return TRUE;
}

static void 
append_an_icon (GnomeIconSelector *gis, const gchar *path)
{
	GdkPixbuf *iml;
	GdkPixbuf *im;
	gchar *basename;
	int pos;
	int w,h;

	iml = gdk_pixbuf_new_from_file (path);
	/*if I can't load it, ignore it*/
	if(!iml)
		return;
	
	w = gdk_pixbuf_get_width (iml);
	h = gdk_pixbuf_get_height (iml);
	if(w>h) {
		if(w>ICON_SIZE) {
			h = h*((double)ICON_SIZE/w);
			w = ICON_SIZE;
		}
	} else {
		if(h>ICON_SIZE) {
			w = w*((double)ICON_SIZE/h);
			h = ICON_SIZE;
		}
	}
	w = w>0?w:1;
	h = h>0?h:1;
	
	im = gdk_pixbuf_scale_simple (iml, w, h, GDK_INTERP_BILINEAR);
	gdk_pixbuf_unref (iml);
	if(!im)
		return;

	basename = g_path_get_basename (path);
	
	pos = gnome_icon_list_append_pixbuf (gis->_priv->icon_list, im,
					     basename);
	gnome_icon_list_set_icon_data_full (gis->_priv->icon_list, pos,
					    g_strdup (path),
					    (GtkDestroyNotify) g_free);
	gdk_pixbuf_unref (im);

	g_free (basename);
}


static void
set_flag (GtkWidget *w, int *flag)
{
	*flag = TRUE;
}

static void
update_filelist (GnomeSelector *gs)
{
	GtkWidget *label;
	GtkWidget *progressbar;
	int file_count, i;
	int local_dest;
	int was_destroyed = FALSE;
	GnomeIconSelector *gis;

	g_return_if_fail (gs != NULL);
	if (!gs->_priv->file_list) return;

	gis = GNOME_ICON_SELECTOR (gs);

	file_count = g_slist_length (gs->_priv->file_list);
	i = 0;

	/* Locate previous progressbar/label,
	 * if previously called. */
	progressbar = label = NULL;
	progressbar = gtk_object_get_user_data (GTK_OBJECT (gis));
	if (progressbar)
		label = gtk_object_get_user_data (GTK_OBJECT (progressbar));

	if (!label && !progressbar) {
		label = gtk_label_new (_("Loading Icons..."));
		gtk_box_pack_start (GTK_BOX (gs->_priv->box), label,
				    FALSE, FALSE, 0);
		gtk_widget_show (label);

		progressbar = gtk_progress_bar_new ();
		gtk_box_pack_start (GTK_BOX (gs->_priv->box),
				    progressbar, FALSE, FALSE, 0);
		gtk_widget_show (progressbar);

		/* attach label to progressbar, progressbar to gs 
		 * for recovery if show_icons() called again */
		gtk_object_set_user_data (GTK_OBJECT (progressbar), label);
		gtk_object_set_user_data (GTK_OBJECT (gis), progressbar);
	} else {
		if (!label && progressbar) g_assert_not_reached();
		if (label && !progressbar) g_assert_not_reached();
	}
         
	gnome_icon_list_freeze (gis->_priv->icon_list);

	/* this can be set with the stop_loading method to stop the
	   display in the middle */
	gis->_priv->stop_loading = FALSE;
  
	/*bind destroy so that we can bail out of this function if the
	  whole thing was destroyed while doing the main_iteration*/
	local_dest = gtk_signal_connect (GTK_OBJECT (gis), "destroy",
					 GTK_SIGNAL_FUNC (set_flag),
					 &was_destroyed);

	while (gs->_priv->file_list) {
		GSList * list = gs->_priv->file_list;
		append_an_icon (gis, list->data);
		g_free (list->data);
		gs->_priv->file_list = g_slist_remove_link
			(gs->_priv->file_list, list);
		g_slist_free_1 (list);

		gtk_progress_bar_update (GTK_PROGRESS_BAR (progressbar),
					 (float)i / file_count);
		while (gtk_events_pending()) {
			gtk_main_iteration ();

			/*if the gs was destroyed from underneath us ...
			 * bail out*/
			if (was_destroyed) 
				return;
                  
			if (gis->_priv->stop_loading)
				goto out;
		}

		i++;
	}

 out:
  
	gtk_signal_disconnect (GTK_OBJECT (gis), local_dest);

	gnome_icon_list_thaw (gis->_priv->icon_list);

	progressbar = label = NULL;
	progressbar = gtk_object_get_user_data (GTK_OBJECT (gis));
	if (progressbar)
		label = gtk_object_get_user_data (GTK_OBJECT (progressbar));
	if (progressbar) gtk_widget_destroy (progressbar);
	if (label) gtk_widget_destroy (label);

	/* cleanse gs of evil progressbar/label ptrs */
	/* also let previous calls to show_icons() know that rendering
	 * is done. */
	gtk_object_set_user_data (GTK_OBJECT(gis), NULL);
}
