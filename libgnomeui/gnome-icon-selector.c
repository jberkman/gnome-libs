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
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkvscrollbar.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-selectorP.h"
#include "gnome-icon-selector.h"
#include "gnome-icon-list.h"
#include "gnome-entry.h"

#define ICON_SIZE 48

struct _GnomeIconSelectorPrivate {
	GnomeIconList *icon_list;

	GSList *file_list;

	/* a flag set to stop the loading of images in midprocess */
	gboolean stop_loading;
};
	

static void gnome_icon_selector_class_init  (GnomeIconSelectorClass *class);
static void gnome_icon_selector_init        (GnomeIconSelector      *iselector);
static void gnome_icon_selector_destroy     (GtkObject       *object);
static void gnome_icon_selector_finalize    (GObject         *object);

static void      clear_handler              (GnomeSelector   *selector);
static gboolean  check_filename_handler     (GnomeSelector   *selector,
                                             const gchar     *filename);
static void      freeze_handler             (GnomeSelector   *selector);
static void      update_file_list_handler   (GnomeSelector   *selector);
static void      thaw_handler               (GnomeSelector   *selector);
static void      set_selection_mode_handler (GnomeSelector   *selector,
                                             guint            mode);
static GSList *  get_selection_handler      (GnomeSelector   *selector);

static void      free_entry_func            (gpointer         data,
					     gpointer         user_data);

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
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	selector_class = (GnomeSelectorClass *) class;
	widget_class = (GtkWidgetClass *) class;
	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	parent_class = gtk_type_class (gnome_selector_get_type ());

	object_class->destroy = gnome_icon_selector_destroy;
	gobject_class->finalize = gnome_icon_selector_finalize;

	selector_class->clear = clear_handler;
	selector_class->check_filename = check_filename_handler;
	selector_class->freeze = freeze_handler;
	selector_class->update_file_list = update_file_list_handler;
	selector_class->thaw = thaw_handler;
	selector_class->set_selection_mode = set_selection_mode_handler;
	selector_class->get_selection = get_selection_handler;
}

static void
gnome_icon_selector_init (GnomeIconSelector *iselector)
{
	iselector->_priv = g_new0 (GnomeIconSelectorPrivate, 1);
}

static void
browse_dialog_cancel (GtkWidget *widget, gpointer data)
{
        GnomeSelector *selector;
	GtkFileSelection *fs;

        selector = GNOME_SELECTOR (data);
	fs = GTK_FILE_SELECTION (selector->_priv->browse_dialog);

	if (GTK_WIDGET (fs)->window)
		gdk_window_lower (GTK_WIDGET (fs)->window);
	gtk_widget_hide (GTK_WIDGET (fs));
}

static void
browse_dialog_ok (GtkWidget *widget, gpointer data)
{
        GnomeSelector *selector;
	GtkFileSelection *fs;
	gchar *filename;

        selector = GNOME_SELECTOR (data);

	fs = GTK_FILE_SELECTION (selector->_priv->browse_dialog);
	filename = gtk_file_selection_get_filename (fs);

	/* This is "directory safe". */
	if (!gnome_selector_set_filename (selector, filename)) {
		gdk_beep ();
		return;
	}

	if (GTK_WIDGET (fs)->window)
		gdk_window_lower (GTK_WIDGET (fs)->window);
	gtk_widget_hide (GTK_WIDGET (fs));
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
			       GtkWidget *browse_dialog,
			       guint32 flags)
{
	g_return_if_fail (iselector != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (iselector));

	/* Create the default selector widget if requested. */
	if (flags & GNOME_SELECTOR_DEFAULT_SELECTOR_WIDGET) {
		GtkWidget *box, *sb, *frame, *list;
		GtkAdjustment *vadj;

		if (selector_widget != NULL) {
			g_warning (G_STRLOC ": It makes no sense to use "
				   "GNOME_SELECTOR_DEFAULT_SELECTOR_WIDGET "
				   "and pass a `selector_widget' as well.");
			return;
		}

		box = gtk_hbox_new (FALSE, 5);

		list = gnome_icon_list_new (ICON_SIZE+30, FALSE);
		vadj = gtk_layout_get_vadjustment (GTK_LAYOUT (list));
		gtk_widget_set_usize (list, 100, 300);

		sb = gtk_vscrollbar_new (vadj);
		gtk_box_pack_end (GTK_BOX(box), sb, FALSE, FALSE, 0);
		gtk_widget_show (sb);

		frame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
		gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
		gtk_widget_show (frame);

		gtk_container_add (GTK_CONTAINER (frame), list);

		gnome_icon_list_set_selection_mode (GNOME_ICON_LIST (list),
						    GTK_SELECTION_SINGLE);

		iselector->_priv->icon_list = GNOME_ICON_LIST (list);
	
		gtk_widget_show_all (box);

		selector_widget = box;
	}

	/* Create the default browser dialog if requested. */
	if (flags & GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG) {
		GtkWidget *filesel_widget;
		GtkFileSelection *filesel;

		if (browse_dialog != NULL) {
			g_warning (G_STRLOC ": It makes no sense to use "
				   "GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG "
				   "and pass a `browse_dialog' as well.");
			return;
		}

		filesel_widget = gtk_file_selection_new (dialog_title);
		filesel = GTK_FILE_SELECTION (filesel_widget);

		gtk_signal_connect (GTK_OBJECT (filesel->cancel_button),
				    "clicked", browse_dialog_cancel,
				    iselector);
		gtk_signal_connect (GTK_OBJECT (filesel->ok_button),
				    "clicked", browse_dialog_ok,
				    iselector);

		browse_dialog = GTK_WIDGET (filesel);
	}

	gnome_selector_construct (GNOME_SELECTOR (iselector),
				  history_id, dialog_title,
				  selector_widget, browse_dialog,
				  flags);

	if (flags & GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG) {
		/* We need to unref this since it isn't put in any
		 * container so it won't get destroyed otherwise. */
		gtk_widget_unref (browse_dialog);
	}
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
	guint32 flags;

	iselector = gtk_type_new (gnome_icon_selector_get_type ());

	flags = GNOME_SELECTOR_DEFAULT_SELECTOR_WIDGET |
		GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG |
		GNOME_SELECTOR_WANT_BROWSE_BUTTON |
		GNOME_SELECTOR_WANT_CLEAR_BUTTON;

	gnome_icon_selector_construct (iselector, history_id,
				       dialog_title, NULL, NULL,
				       flags);

	return GTK_WIDGET (iselector);
}

GtkWidget *
gnome_icon_selector_new_custom (const gchar *history_id,
				const gchar *dialog_title,
				GtkWidget *selector_widget,
				GtkWidget *browse_dialog,
				guint32 flags)
{
	GnomeIconSelector *iselector;

	iselector = gtk_type_new (gnome_icon_selector_get_type ());

	gnome_icon_selector_construct (iselector, history_id,
				       dialog_title, selector_widget,
				       browse_dialog, flags);

	return GTK_WIDGET (iselector);
}


static void
gnome_icon_selector_destroy (GtkObject *object)
{
	GnomeIconSelector *iselector;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (object));

	iselector = GNOME_ICON_SELECTOR (object);

	if (iselector->_priv) {
#if 0
		if (iselector->_priv->icon_list)
			gtk_widget_unref (iselector->_priv->icon_list);
#endif
		iselector->_priv->icon_list = NULL;

		g_slist_foreach (iselector->_priv->file_list,
				 free_entry_func, iselector);
		iselector->_priv->file_list = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gnome_icon_selector_finalize (GObject *object)
{
	GnomeIconSelector *iselector;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (object));

	iselector = GNOME_ICON_SELECTOR (object);

	g_free (iselector->_priv);
	iselector->_priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
clear_handler (GnomeSelector *selector)
{
	GnomeIconSelector *iselector;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (selector));

	iselector = GNOME_ICON_SELECTOR (selector);

	gnome_icon_list_clear (iselector->_priv->icon_list);

	if (GNOME_SELECTOR_CLASS (parent_class)->clear)
		(* GNOME_SELECTOR_CLASS (parent_class)->clear) (selector);
}

static gboolean
check_filename_handler (GnomeSelector *selector, const gchar *filename)
{
	const char *mimetype;

	g_return_val_if_fail (selector != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_ICON_SELECTOR (selector), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	if (g_file_test (filename, G_FILE_TEST_ISDIR))
		return TRUE;

	mimetype = gnome_mime_type_from_magic (filename);
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
freeze_handler (GnomeSelector *selector)
{
	GnomeIconSelector *iselector;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (selector));

	iselector = GNOME_ICON_SELECTOR (selector);

	g_message (G_STRLOC ": %p", iselector->_priv->icon_list);

	gnome_icon_list_freeze (iselector->_priv->icon_list);

	if (GNOME_SELECTOR_CLASS (parent_class)->freeze)
		(* GNOME_SELECTOR_CLASS (parent_class)->freeze) (selector);
}

static void
thaw_handler (GnomeSelector *selector)
{
	GnomeIconSelector *iselector;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (selector));

	iselector = GNOME_ICON_SELECTOR (selector);

	g_message (G_STRLOC ": %p", iselector->_priv->icon_list);

	gnome_icon_list_thaw (iselector->_priv->icon_list);

	if (GNOME_SELECTOR_CLASS (parent_class)->thaw)
		(* GNOME_SELECTOR_CLASS (parent_class)->thaw) (selector);
}

static void
set_selection_mode_handler (GnomeSelector *selector, guint mode)
{
	GnomeIconSelector *iselector;

	g_return_if_fail (selector != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (selector));

	iselector = GNOME_ICON_SELECTOR (selector);

	g_message (G_STRLOC ": %d", mode);

	gnome_icon_list_set_selection_mode (iselector->_priv->icon_list,
					    (GtkSelectionMode) mode);

	if (GNOME_SELECTOR_CLASS (parent_class)->set_selection_mode)
		(* GNOME_SELECTOR_CLASS (parent_class)->set_selection_mode)
			(selector, mode);
}

static GSList *
get_selection_handler (GnomeSelector *selector)
{
	GnomeIconSelector *iselector;
	GnomeIconList *gil;
	GSList *selection = NULL;
	GList *c;

	g_return_val_if_fail (selector != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_SELECTOR (selector), NULL);

	iselector = GNOME_ICON_SELECTOR (selector);
	gil = GNOME_ICON_LIST (iselector->_priv->icon_list);

	g_message (G_STRLOC);

	for (c = gil->selection; c; c = c->next) {
		GSList *total_list, *item;

		total_list = gnome_selector_get_file_list (selector, TRUE);
		item = g_slist_nth (total_list, GPOINTER_TO_INT (c->data));

		selection = g_slist_prepend (selection, item->data);
	}

	return g_slist_reverse (selection);
}

static void
free_entry_func (gpointer data, gpointer user_data)
{
	g_free (data);
}

static void
update_file_list_handler (GnomeSelector *gs)
{
	GtkWidget *label;
	GtkWidget *progressbar;
	int file_count, i;
	int local_dest;
	int was_destroyed = FALSE;
	GnomeIconSelector *gis;

	g_return_if_fail (gs != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (gs));

	if (GNOME_SELECTOR_CLASS (parent_class)->update_file_list)
		(* GNOME_SELECTOR_CLASS (parent_class)->update_file_list) (gs);

	gis = GNOME_ICON_SELECTOR (gs);

	g_slist_foreach (gis->_priv->file_list, free_entry_func, gis);
	gis->_priv->file_list = g_slist_copy (gs->_priv->total_list);

	file_count = g_slist_length (gis->_priv->file_list);
	i = 0;

	g_message (G_STRLOC ": %d", file_count);

	/* Locate previous progressbar/label,
	 * if previously called. */
	progressbar = label = NULL;
	progressbar = gtk_object_get_user_data (GTK_OBJECT (gis));
	if (progressbar)
		label = gtk_object_get_user_data (GTK_OBJECT (progressbar));

	if (!label && !progressbar) {
		label = gtk_label_new (_("Loading Icons..."));
		gtk_box_pack_end (GTK_BOX (gs->_priv->box), label,
				  FALSE, FALSE, 0);
		gtk_widget_show (label);

		progressbar = gtk_progress_bar_new ();
		gtk_box_pack_end (GTK_BOX (gs->_priv->box),
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
	gnome_icon_list_clear (gis->_priv->icon_list);

	/* this can be set with the stop_loading method to stop the
	   display in the middle */
	gis->_priv->stop_loading = FALSE;
  
	/*bind destroy so that we can bail out of this function if the
	  whole thing was destroyed while doing the main_iteration*/
	local_dest = gtk_signal_connect (GTK_OBJECT (gis), "destroy",
					 GTK_SIGNAL_FUNC (set_flag),
					 &was_destroyed);

	while (gis->_priv->file_list) {
		GSList * list = gis->_priv->file_list;
		append_an_icon (gis, list->data);
		gis->_priv->file_list = g_slist_remove_link
			(gis->_priv->file_list, list);
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
