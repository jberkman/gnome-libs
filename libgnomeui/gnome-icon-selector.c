/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
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
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkvscrollbar.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnomeP.h"
#include "gnome-macros.h"
#include "gnome-selectorP.h"
#include "gnome-vfs-util.h"
#include "gnome-file-selector.h"
#include "gnome-icon-selector.h"
#include "gnome-icon-list.h"
#include "gnome-entry.h"

#include <libgnomevfs/gnome-vfs.h>

#define ICON_SIZE 48

typedef struct _GnomeIconSelectorAsyncData      GnomeIconSelectorAsyncData;

struct _GnomeIconSelectorAsyncData {
    GnomeSelectorAsyncHandle *async_handle;
    GnomeGdkPixbufAsyncHandle *handle;
    GnomeIconSelector *iselector;
    GnomeVFSURI *uri;
    gint position;
};

struct _GnomeIconSelectorPrivate {
    GnomeIconList *icon_list;

    GtkWidget *entry;

    GSList *file_list;
};

static void gnome_icon_selector_class_init  (GnomeIconSelectorClass *class);
static void gnome_icon_selector_init        (GnomeIconSelector      *iselector);
static void gnome_icon_selector_destroy     (GtkObject              *object);
static void gnome_icon_selector_finalize    (GObject                *object);

static void      clear_handler              (GnomeSelector     *selector);
static void      freeze_handler             (GnomeSelector     *selector);
static void      thaw_handler               (GnomeSelector     *selector);
static void      set_selection_mode_handler (GnomeSelector     *selector,
                                             guint              mode);
static GSList *  get_selection_handler      (GnomeSelector     *selector);

static void      free_entry_func            (gpointer           data,
					     gpointer           user_data);

static void      add_file_handler           (GnomeSelector            *selector,
                                             const gchar              *uri,
                                             gint                      position,
                                             GnomeSelectorAsyncHandle *async_handle);


static GnomeFileSelectorClass *parent_class;

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
	    (gnome_file_selector_get_type (), &iselector_info);
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

    parent_class = gtk_type_class (gnome_file_selector_get_type ());

    object_class->destroy = gnome_icon_selector_destroy;
    gobject_class->finalize = gnome_icon_selector_finalize;

    selector_class->clear = clear_handler;
    selector_class->freeze = freeze_handler;
    selector_class->thaw = thaw_handler;
    selector_class->set_selection_mode = set_selection_mode_handler;
    selector_class->get_selection = get_selection_handler;

    selector_class->add_file = add_file_handler;
}

static void
gnome_icon_selector_init (GnomeIconSelector *iselector)
{
    iselector->_priv = g_new0 (GnomeIconSelectorPrivate, 1);
}

static void
icon_selected_cb (GnomeIconList *gil, gint num, GdkEvent *event,
		  GnomeIconSelector *iselector)
{
	g_return_if_fail (iselector != NULL);
	g_return_if_fail (GNOME_IS_ICON_SELECTOR (iselector));

	gtk_signal_emit_by_name (GTK_OBJECT (iselector),
				 "selection_changed");
}

static gboolean
iselector_directory_filter_func (const GnomeVFSFileInfo *info, gpointer data)
{
    GnomeIconSelector *iselector;
    const gchar *mimetype;

    g_return_val_if_fail (data != NULL, FALSE);
    g_return_val_if_fail (GNOME_IS_ICON_SELECTOR (data), FALSE);

    iselector = GNOME_ICON_SELECTOR (data);

    mimetype = gnome_vfs_file_info_get_mime_type ((GnomeVFSFileInfo *) info);

    if (!mimetype || strncmp (mimetype, "image", sizeof("image")-1))
	return FALSE;
    else
	return TRUE;
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
			       GtkWidget *entry_widget,
			       GtkWidget *selector_widget,
			       GtkWidget *browse_dialog,
			       guint32 flags)
{
    GnomeVFSDirectoryFilter *filter;

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

        gtk_signal_connect_after (GTK_OBJECT (list), "select_icon",
                                  GTK_SIGNAL_FUNC (icon_selected_cb),
                                  iselector);
        gtk_signal_connect_after (GTK_OBJECT (list), "unselect_icon",
                                  GTK_SIGNAL_FUNC (icon_selected_cb),
                                  iselector);

	gtk_object_ref (GTK_OBJECT (list));
	iselector->_priv->icon_list = GNOME_ICON_LIST (list);
	
	gtk_widget_show_all (box);

	selector_widget = box;
    }

    gnome_file_selector_construct (GNOME_FILE_SELECTOR (iselector),
				   history_id, dialog_title,
				   entry_widget, selector_widget,
				   browse_dialog, flags);

    filter = gnome_vfs_directory_filter_new_custom
	(iselector_directory_filter_func,
	 GNOME_VFS_DIRECTORY_FILTER_NEEDS_NAME |
	 GNOME_VFS_DIRECTORY_FILTER_NEEDS_TYPE |
	 GNOME_VFS_DIRECTORY_FILTER_NEEDS_MIMETYPE,
	 iselector);

    gnome_file_selector_set_filter (GNOME_FILE_SELECTOR (iselector), filter);
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
			 const gchar *dialog_title,
			 guint32 flags)
{
    GnomeIconSelector *iselector;

    g_return_val_if_fail ((flags & ~GNOME_SELECTOR_USER_FLAGS) == 0, NULL);

    iselector = gtk_type_new (gnome_icon_selector_get_type ());

    flags |= GNOME_SELECTOR_DEFAULT_ENTRY_WIDGET |
	GNOME_SELECTOR_DEFAULT_SELECTOR_WIDGET |
	GNOME_SELECTOR_DEFAULT_BROWSE_DIALOG |
	GNOME_SELECTOR_WANT_BROWSE_BUTTON |
	GNOME_SELECTOR_WANT_DEFAULT_BUTTON |
	GNOME_SELECTOR_WANT_CLEAR_BUTTON;

    gnome_icon_selector_construct (iselector, history_id, dialog_title,
				   NULL, NULL, NULL, flags);

    return GTK_WIDGET (iselector);
}

GtkWidget *
gnome_icon_selector_new_custom (const gchar *history_id,
				const gchar *dialog_title,
				GtkWidget *entry_widget,
				GtkWidget *selector_widget,
				GtkWidget *browse_dialog,
				guint32 flags)
{
    GnomeIconSelector *iselector;

    iselector = gtk_type_new (gnome_icon_selector_get_type ());

    gnome_icon_selector_construct (iselector, history_id, dialog_title,
				   entry_widget, selector_widget,
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
	if (iselector->_priv->icon_list)
	    gtk_object_unref (GTK_OBJECT (iselector->_priv->icon_list));
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

static void
add_file_async_done_cb (gpointer data)
{
    GnomeIconSelectorAsyncData *async_data = data;
    GnomeIconSelector *iselector;

    /* We're called from gnome_async_handle_unref() during finalizing. */

    g_return_if_fail (async_data != NULL);
    g_assert (GNOME_IS_ICON_SELECTOR (async_data->iselector));

    iselector = async_data->iselector;

    g_message (G_STRLOC ": %p", async_data->async_handle);

    /* When the operation was successful, this is already NULL. */
    gnome_gdk_pixbuf_new_from_uri_cancel (async_data->handle);

    /* free the async data. */
    gtk_object_unref (GTK_OBJECT (async_data->iselector));
    gnome_vfs_uri_unref (async_data->uri);
    g_free (async_data);
}

static void
add_file_async_cb (GnomeGdkPixbufAsyncHandle *handle,
		   GnomeVFSResult error, GdkPixbuf *pixbuf,
		   gpointer callback_data)
{
    GnomeIconSelectorAsyncData *async_data = callback_data;
    GnomeIconSelector *iselector;
    const gchar *basename;
    GdkPixbuf *scaled;
    gchar *path;
    guint w, h;

    g_return_if_fail (async_data != NULL);
    g_assert (GNOME_IS_ICON_SELECTOR (async_data->iselector));

    iselector = GNOME_ICON_SELECTOR (async_data->iselector);

    if ((error != GNOME_VFS_OK) || ((pixbuf == NULL)))
	return;
	
    w = gdk_pixbuf_get_width (pixbuf);
    h = gdk_pixbuf_get_height (pixbuf);

    if(w > h) {
	if(w > ICON_SIZE) {
	    h = h * ((double) ICON_SIZE / w);
	    w = ICON_SIZE;
	}
    } else {
	if(h > ICON_SIZE) {
	    w = w * ((double) ICON_SIZE / h);
	    h = ICON_SIZE;
	}
    }
    w = w > 0 ? w : 1;
    h = h > 0 ? h : 1;

    scaled = gdk_pixbuf_scale_simple (pixbuf, w, h, GDK_INTERP_BILINEAR);
    if (!scaled)
	return;

    basename = gnome_vfs_uri_get_basename (async_data->uri);
    path = gnome_vfs_uri_to_string (async_data->uri, GNOME_VFS_URI_HIDE_NONE);

    if (async_data->position == -1)
	gnome_icon_list_append_pixbuf (iselector->_priv->icon_list,
				       scaled, path, basename);
    else
	gnome_icon_list_insert_pixbuf (iselector->_priv->icon_list,
				       async_data->position, scaled,
				       path, basename);

    /* This will be freed by our caller; we must set it to NULL here to avoid
     * that add_file_async_done_cb() attempts to free this. */
    async_data->handle = NULL;

    GNOME_CALL_PARENT_HANDLER (GNOME_SELECTOR_CLASS, add_file,
			       (GNOME_SELECTOR (iselector),
				path, async_data->position,
				async_data->async_handle));

    gdk_pixbuf_unref (scaled);
    g_free (path);
}

static void
add_file_done_cb (GnomeGdkPixbufAsyncHandle *handle,
		  gpointer callback_data)
{
    GnomeIconSelectorAsyncData *async_data = callback_data;

    g_return_if_fail (async_data != NULL);

    g_message (G_STRLOC ": %p", async_data->async_handle);

    _gnome_selector_async_handle_remove (async_data->async_handle,
					 async_data);
}

static void
add_file_handler (GnomeSelector *selector, const gchar *uri, gint position,
		  GnomeSelectorAsyncHandle *async_handle)
{
    GnomeIconSelector *iselector;
    GnomeIconSelectorAsyncData *async_data;

    g_return_if_fail (selector != NULL);
    g_return_if_fail (GNOME_IS_ICON_SELECTOR (selector));
    g_return_if_fail (position >= -1);
    g_return_if_fail (uri != NULL);
    g_return_if_fail (async_handle != NULL);
    g_assert (GNOME_IS_SELECTOR (async_handle->selector));

    iselector = GNOME_ICON_SELECTOR (selector);

    async_data = g_new0 (GnomeIconSelectorAsyncData, 1);
    async_data->iselector = iselector;
    async_data->async_handle = async_handle;
    async_data->uri = gnome_vfs_uri_new (uri);
    async_data->position = position;

    gtk_object_ref (GTK_OBJECT (async_data->iselector));

    _gnome_selector_async_handle_add (async_handle, async_data,
				      add_file_async_done_cb);

    async_data->handle = gnome_gdk_pixbuf_new_from_uri_async
	(uri, add_file_async_cb, add_file_done_cb, async_data);
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
    GList *list, *c;

    g_return_val_if_fail (selector != NULL, NULL);
    g_return_val_if_fail (GNOME_IS_ICON_SELECTOR (selector), NULL);

    iselector = GNOME_ICON_SELECTOR (selector);
    gil = GNOME_ICON_LIST (iselector->_priv->icon_list);
    list = gnome_icon_list_get_selection (gil);

    for (c = list; c; c = c->next) {
	guint pos = GPOINTER_TO_INT (c->data);
	gchar *filename;

	filename = gnome_icon_list_get_icon_filename (gil, pos);

	selection = g_slist_prepend (selection, filename);
    }

    return g_slist_reverse (selection);
}

static void
free_entry_func (gpointer data, gpointer user_data)
{
    g_free (data);
}

static void
add_defaults_async_cb (GnomeSelector *selector,
		       GnomeSelectorAsyncHandle *async_handle,
		       GnomeSelectorAsyncType async_type,
		       const char *uri, GError *error,
		       gboolean success, gpointer user_data)
{
    g_message (G_STRLOC ": %d - `%s' - %p - %d", success, uri,
	       async_handle, async_handle->refcount);
}

/**
 * gnome_icon_selector_add_defaults:
 * @gis: GnomeIconSelector to work with
 *
 * Description: Adds the default pixmap directory into the selector
 * widget.
 *
 * Returns:
 **/
void
gnome_icon_selector_add_defaults (GnomeIconSelector *iselector)
{
    gchar *pixmap_dir;

    g_return_if_fail (iselector != NULL);
    g_return_if_fail (GNOME_IS_ICON_SELECTOR (iselector));

    pixmap_dir = gnome_unconditional_datadir_file ("pixmaps");
  
    gnome_selector_add_directory (GNOME_SELECTOR (iselector), NULL, pixmap_dir,
				  -1, FALSE, add_defaults_async_cb, NULL);

    g_free (pixmap_dir);
}
