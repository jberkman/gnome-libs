/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Stock icons, buttons, and menu items.
   Copyright (C) 1997, 1998 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Eckehard Berns  */

#include <config.h>

#include "libgnome/gnome-util.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-config.h"
#include "gnome-stock.h"
#include "gnome-pixmap.h"
#include "gnome-uidefs.h"
#include "gnome-preferences.h"

#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "pixmaps/gnome-stock-imlib.h"

#define STOCK_SEP '.'
#define STOCK_SEP_STR "."

#define GNOME_STOCK_BUTTON_PADDING 2

static GnomeStockPixmapEntry **lookup(const char *icon);

/***************************/
/*  new GnomeStock widget  */
/***************************/

static GnomePixmapClass* parent_class = NULL;

static void
gnome_stock_destroy(GtkObject *object)
{
	GnomeStock *stock;

	g_return_if_fail(object != NULL);
	g_return_if_fail(GNOME_IS_STOCK(object));

	stock = GNOME_STOCK(object);

        (* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}



static void
gnome_stock_class_init(GtkObjectClass * klass)
{
  	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) klass;
	widget_class = (GtkWidgetClass *) klass;

	parent_class = gtk_type_class (gnome_pixmap_get_type ());
  
	klass->destroy = gnome_stock_destroy;
}

static void
gnome_stock_init(GnomeStock *stock)
{
	stock->icon = NULL;
	gnome_pixmap_set_draw_mode (GNOME_PIXMAP (stock), GNOME_PIXMAP_COLOR);
}

/**
 * gnome_stock_get_type:
 *
 * Returns: The GtkType for the #GnomeStock widget.
 */
guint
gnome_stock_get_type(void)
{
	static guint new_type = 0;
	if (!new_type) {
		GtkTypeInfo type_info = {
			"GnomeStock",
			sizeof(GnomeStock),
			sizeof(GnomeStockClass),
			(GtkClassInitFunc)gnome_stock_class_init,
			(GtkObjectInitFunc)gnome_stock_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
			(GtkClassInitFunc) NULL
		};
		new_type = gtk_type_unique(gnome_pixmap_get_type(), &type_info);
	}
	return new_type;
}

/** 
 * gnome_stock_new:
 *
 * Returns: a new empty GnomeStock widget
 */
GtkWidget *
gnome_stock_new(void)
{
        GnomeStock *stock;
	stock = gtk_type_new(gnome_stock_get_type());

        return GTK_WIDGET(stock);
}

/**
 * gnome_stock_set_icon:
 * @stock: The GnomeStock object on which to operate
 * @icon: Icon code to set
 *
 * Sets the @stock object icon to be the one whose code is @icon.
 */
void
gnome_stock_set_icon(GnomeStock *stock, const char *icon)
{
	GnomeStockPixmapEntry **entries;
        GdkPixbuf *pixbufs[5] = { NULL, NULL, NULL, NULL, NULL };
        GdkBitmap *bitmasks[5] = { NULL, NULL, NULL, NULL, NULL };
        int i;
        
	g_return_if_fail(stock != NULL);
	g_return_if_fail(GNOME_IS_STOCK(stock));
	g_return_if_fail(icon != NULL);

	if (stock->icon) {
		if (0 == strcmp(icon, stock->icon))
			return;
		g_free(stock->icon);
                stock->icon = NULL;
	}
        
	entries = lookup(icon);
        
        if (entries == NULL) {
                g_warning("No such stock icon `%s'", icon);
                return;
        }
        
	stock->icon = g_strdup(icon);

        i = 0;
        while (i < 5) {
          if (entries[i] != NULL)
            pixbufs[i] = gnome_stock_pixmap_entry_get_gdk_pixbuf(entries[i]);
          else
            pixbufs[i] = NULL;

          bitmasks[i] = NULL;
          
          ++i;
        }

        i = 0;
        while (i < 5) {
          if (pixbufs[i] != NULL &&
              gdk_pixbuf_get_has_alpha(pixbufs[i])) {
                  bitmasks[i] = gdk_pixmap_new(NULL,
                                               gdk_pixbuf_get_width(pixbufs[i]),
                                               gdk_pixbuf_get_height(pixbufs[i]),
                                               1);

                  gdk_pixbuf_render_threshold_alpha(pixbufs[i], bitmasks[i],
                                                    0, 0, 0, 0,
                                                    gdk_pixbuf_get_width(pixbufs[i]),
                                                    gdk_pixbuf_get_height(pixbufs[i]),
                                                    128); /* clip at 50% alpha */
          }
          ++i;
        }

        /* Note that we may be building a new insensitive image for each widget,
           while we could share the insensitive versions in our hash table.
           It's an open question which is the right thing...
        */
        gnome_pixmap_set_state_pixbufs (GNOME_PIXMAP(stock),
                                        pixbufs,
                                        bitmasks);

        /* Set the main pixbuf from the normal version, in
           case pixbufs[] contains NULL for some images */
        if (pixbufs[GTK_STATE_NORMAL] != NULL)
                gnome_pixmap_set_pixbuf(GNOME_PIXMAP(stock),
                                        pixbufs[GTK_STATE_NORMAL]);
	return;
}

/**
 * gnome_stock_new_with_icon:
 * @icon: icon code.
 *
 * Returns: A GnomeStock widget created with the initial icon code
 * set to @icon. The icon must exist.
 */ 
GtkWidget *
gnome_stock_new_with_icon(const char *icon)
{
	GtkWidget *w;

	g_return_val_if_fail(icon != NULL, NULL);
	w = gtk_type_new(gnome_stock_get_type());

	gnome_stock_set_icon(GNOME_STOCK(w), icon);

	return w;
}

GtkWidget    *
gnome_stock_new_with_icon_at_size (const char *icon, int width, int height)
{
  /* FIXME */
  g_warning("%s not implemented", __FUNCTION__);
  return gnome_stock_new_with_icon(icon);
}

/*
 * Hash entry for the stock icon hash
 */


/* Structures for the internal stock image hash table entries */

typedef struct _GnomeStockPixmapEntryAny     GnomeStockPixmapEntryAny;
typedef struct _GnomeStockPixmapEntryData    GnomeStockPixmapEntryData;
typedef struct _GnomeStockPixmapEntryFile    GnomeStockPixmapEntryFile;
typedef struct _GnomeStockPixmapEntryPath    GnomeStockPixmapEntryPath;
typedef struct _GnomeStockPixmapEntryPixbuf     GnomeStockPixmapEntryPixbuf;
typedef struct _GnomeStockPixmapEntryPixbufScaled GnomeStockPixmapEntryPixbufScaled;

typedef enum {
        GNOME_STOCK_PIXMAP_TYPE_NONE,
        GNOME_STOCK_PIXMAP_TYPE_DATA,
        GNOME_STOCK_PIXMAP_TYPE_FILE,
        GNOME_STOCK_PIXMAP_TYPE_PATH,
        GNOME_STOCK_PIXMAP_TYPE_PIXBUF,
        GNOME_STOCK_PIXMAP_TYPE_PIXBUF_SCALED,
        GNOME_STOCK_PIXMAP_TYPE_LAST
} GnomeStockPixmapType;

/* for even easier debugging */ 
#define GNOME_IS_STOCK_PIXMAP_ENTRY(entry) \
        (entry->type >= 0 && entry->type < GNOME_STOCK_PIXMAP_TYPE_LAST)

struct _GnomeStockPixmapEntryAny {
        GnomeStockPixmapType type;
	int ref_count;
	char *label;
        GdkPixbuf *pixbuf;
};

/* a data entry holds a hardcoded pixmap */
struct _GnomeStockPixmapEntryData {
        GnomeStockPixmapType type;
	int ref_count;
	char *label;
        GdkPixbuf *pixbuf;
        const gchar **xpm_data;
};

/* a file entry holds a filename (no path) to the pixamp. this pixmap
   will be seached for using gnome_pixmap_file */
struct _GnomeStockPixmapEntryFile {
        GnomeStockPixmapType type;
	int ref_count;
	char *label;
        GdkPixbuf *pixbuf;
        gchar *filename;
};

/* a path entry holds the complete (absolut) path to the pixmap file */
struct _GnomeStockPixmapEntryPath {
        GnomeStockPixmapType type;
	int ref_count;
	char *label;
        GdkPixbuf *pixbuf;
        gchar *pathname;
};

/* a data entry holds a hardcoded pixmap */
struct _GnomeStockPixmapEntryPixbuf {
        GnomeStockPixmapType type;
	int ref_count;
        char *label;
        GdkPixbuf *pixbuf;
};

 
/* scales the Pixbuf data to the given size when used (allows scale-on-demand) */
struct _GnomeStockPixmapEntryPixbufScaled {
        GnomeStockPixmapType type;
        char *label;
        GdkPixbuf *pixbuf;
        int scaled_width, scaled_height;
        GdkPixbuf *unscaled_pixbuf;
};

union _GnomeStockPixmapEntry {
        GnomeStockPixmapType type;
        GnomeStockPixmapEntryAny any;
        GnomeStockPixmapEntryData data;
        GnomeStockPixmapEntryFile file;
        GnomeStockPixmapEntryPath path;
        GnomeStockPixmapEntryPixbuf pixbuf;
        GnomeStockPixmapEntryPixbufScaled scaled;
};

static GnomeStockPixmapEntry*
gnome_stock_pixmap_entry_new_blank (GnomeStockPixmapType type, const gchar* label)
{
        GnomeStockPixmapEntry* entry = NULL;

        switch (type) {
        case GNOME_STOCK_PIXMAP_TYPE_DATA:
                entry = (GnomeStockPixmapEntry*) g_new0(GnomeStockPixmapEntryData, 1);
                break;
        case GNOME_STOCK_PIXMAP_TYPE_FILE:
                entry = (GnomeStockPixmapEntry*) g_new0(GnomeStockPixmapEntryFile, 1);
                break;
                
        case GNOME_STOCK_PIXMAP_TYPE_PATH:
                entry = (GnomeStockPixmapEntry*) g_new0(GnomeStockPixmapEntryPath, 1);
                break;
                
        case GNOME_STOCK_PIXMAP_TYPE_PIXBUF:
                entry = (GnomeStockPixmapEntry*) g_new0(GnomeStockPixmapEntryPixbuf, 1);
                break;

        case GNOME_STOCK_PIXMAP_TYPE_PIXBUF_SCALED:
                entry = (GnomeStockPixmapEntry*) g_new0(GnomeStockPixmapEntryPixbufScaled, 1);
                break;

        default:
                g_assert_not_reached();
                break;
        }
        
        g_assert(entry != NULL);

        entry->type = type;
	entry->any.ref_count = 1;
        
        if (label != NULL)
                entry->any.label = g_strdup(label);

        return entry;
}

static void
gnome_stock_pixmap_entry_destroy (GnomeStockPixmapEntry* entry)
{
        switch (entry->type) {
        case GNOME_STOCK_PIXMAP_TYPE_DATA:
                /* nothing to free */
                break;

        case GNOME_STOCK_PIXMAP_TYPE_FILE:
                if (entry->file.filename != NULL) 
                        g_free(entry->file.filename);
                break;
                
        case GNOME_STOCK_PIXMAP_TYPE_PATH:
                if (entry->path.pathname != NULL)
                        g_free(entry->path.pathname);
                break;
                
        case GNOME_STOCK_PIXMAP_TYPE_PIXBUF:
                break;
                
        case GNOME_STOCK_PIXMAP_TYPE_PIXBUF_SCALED:
                if (entry->scaled.unscaled_pixbuf != NULL)
                        gdk_pixbuf_unref(entry->scaled.unscaled_pixbuf);
                break;

        default:
                g_assert_not_reached();
                break;
        }

        if (entry->any.pixbuf != NULL)
                gdk_pixbuf_unref(entry->any.pixbuf);
        
        if (entry->any.label != NULL)
                g_free(entry->any.label);

	/* just in case we double free, then GNOME_IS_STOCK_PIXMAP_ENTRY
	 * will fail */
	entry->type = -1;

        g_free(entry);
}

/**
 * gnome_stock_pixmap_entry_ref:
 * @entry: gnome stock pixmap entry
 *
 * Description:  Increments the reference count on the entry
 */
void
gnome_stock_pixmap_entry_ref (GnomeStockPixmapEntry* entry)
{
	g_return_if_fail (entry != NULL);
	g_return_if_fail (GNOME_IS_STOCK_PIXMAP_ENTRY(entry));

	entry->any.ref_count++;
}

/**
 * gnome_stock_pixmap_entry_unref:
 * @entry: gnome stock pixmap entry
 *
 * Description:  Decrements the reference count on the entry, and
 * destroys the structure if it went to 0
 */
void
gnome_stock_pixmap_entry_unref (GnomeStockPixmapEntry* entry)
{
	g_return_if_fail (entry != NULL);
	g_return_if_fail (GNOME_IS_STOCK_PIXMAP_ENTRY(entry));

	entry->any.ref_count--;
	if(entry->any.ref_count == 0)
		gnome_stock_pixmap_entry_destroy(entry);
}

GnomeStockPixmapEntry*
gnome_stock_pixmap_entry_new_from_gdk_pixbuf (GdkPixbuf *pixbuf,
                                              const gchar* label)
{
        GnomeStockPixmapEntry* entry;

        entry = gnome_stock_pixmap_entry_new_blank (GNOME_STOCK_PIXMAP_TYPE_PIXBUF,
                                                    label);

        gdk_pixbuf_ref(pixbuf);

        ((GnomeStockPixmapEntryPixbuf*)entry)->pixbuf = pixbuf;

        return entry;
}

GnomeStockPixmapEntry*
gnome_stock_pixmap_entry_new_from_gdk_pixbuf_at_size (GdkPixbuf *pixbuf,
                                                      const gchar* label,
                                                      gint width, gint height)
{
        GnomeStockPixmapEntry* entry;

        entry = gnome_stock_pixmap_entry_new_blank (GNOME_STOCK_PIXMAP_TYPE_PIXBUF_SCALED,
                                                    label);

        gdk_pixbuf_ref(pixbuf);

        ((GnomeStockPixmapEntryPixbufScaled*)entry)->unscaled_pixbuf = pixbuf;
        ((GnomeStockPixmapEntryPixbufScaled*)entry)->scaled_width = width;
        ((GnomeStockPixmapEntryPixbufScaled*)entry)->scaled_height = height;

        return entry;
}

GnomeStockPixmapEntry*
gnome_stock_pixmap_entry_new_from_filename (const gchar* filename,
                                            const gchar* label)
{
        GnomeStockPixmapEntry* entry;

        entry = gnome_stock_pixmap_entry_new_blank (GNOME_STOCK_PIXMAP_TYPE_FILE,
                                                    label);

        ((GnomeStockPixmapEntryFile*)entry)->filename = g_strdup(filename);

        return entry;
}

GnomeStockPixmapEntry*
gnome_stock_pixmap_entry_new_from_pathname (const gchar* pathname,
                                            const gchar* label)
{
        GnomeStockPixmapEntry* entry;

        entry = gnome_stock_pixmap_entry_new_blank (GNOME_STOCK_PIXMAP_TYPE_PATH,
                                                    label);

        entry->path.pathname = g_strdup(pathname);

        return entry;
}


GnomeStockPixmapEntry*
gnome_stock_pixmap_entry_new_from_xpm_data (const gchar** xpm_data, const gchar* label)
{
        GnomeStockPixmapEntry* entry;

        entry = gnome_stock_pixmap_entry_new_blank (GNOME_STOCK_PIXMAP_TYPE_DATA,
                                                    label);

        ((GnomeStockPixmapEntryData*)entry)->xpm_data = xpm_data;

        return entry;
}

/**
 * gnome_stock_pixmap_entry_get_gdk_pixbuf:
 * @entry: the #GnomeStockPixmapEntry
 *
 * Description: Returns the #GdkPixbuf that is used for drawing
 * this stock entry.  The returned pixbuf is the internal data with
 * it's reference count incremented, so do gdk_pixbuf_unref when you
 * are done with it and treat it as read only data.
 *
 * Returns: a #GdkPixbuf with an incremented reference count.
 **/
GdkPixbuf*
gnome_stock_pixmap_entry_get_gdk_pixbuf (GnomeStockPixmapEntry *entry)
{
	g_return_val_if_fail (entry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_STOCK_PIXMAP_ENTRY(entry), NULL);

        if (entry->any.pixbuf != NULL) {
                /* Return the cached pixbuf, with refcount incremented. */
                gdk_pixbuf_ref(entry->any.pixbuf);
                return entry->any.pixbuf;
        }

        switch (entry->type) {
        case GNOME_STOCK_PIXMAP_TYPE_DATA:
                entry->any.pixbuf = gdk_pixbuf_new_from_xpm_data(entry->data.xpm_data);
                break;
                
        case GNOME_STOCK_PIXMAP_TYPE_FILE: {
                gchar* pathname;

                g_assert(entry->file.filename != NULL);
                
                pathname = gnome_pixmap_file(entry->file.filename);
                entry->any.pixbuf = gdk_pixbuf_new_from_file(pathname);
                g_free(pathname);
                /* drop some memory */
                if (entry->any.pixbuf != NULL) {
                        g_free(entry->file.filename);
                        entry->file.filename = NULL;
                }
        }
                break;
                
        case GNOME_STOCK_PIXMAP_TYPE_PATH:
                g_assert(entry->path.pathname != NULL);
                
                entry->any.pixbuf = gdk_pixbuf_new_from_file(entry->path.pathname);
                /* drop some memory */
                if (entry->any.pixbuf != NULL) {
                        g_free(entry->path.pathname);
                        entry->path.pathname = NULL;
                }
                break;
                
        case GNOME_STOCK_PIXMAP_TYPE_PIXBUF:
                /* no work to do here */
                break;

        case GNOME_STOCK_PIXMAP_TYPE_PIXBUF_SCALED:
                if (entry->scaled.unscaled_pixbuf != NULL) {
                        g_assert(entry->any.pixbuf == NULL);

                        entry->any.pixbuf = gdk_pixbuf_scale_simple(entry->scaled.unscaled_pixbuf,
                                                                    entry->scaled.scaled_width,
                                                                    entry->scaled.scaled_height,
                                                                    GDK_INTERP_BILINEAR);
                }
                if (entry->any.pixbuf != NULL &&
                    entry->scaled.unscaled_pixbuf != NULL) {
                        gdk_pixbuf_unref(entry->scaled.unscaled_pixbuf);
                        entry->scaled.unscaled_pixbuf = NULL;
                }
                break;

        default:
                g_assert_not_reached();
                break;

        }

        if (entry->any.pixbuf != NULL) {
                /* Return the cached pixbuf, with refcount incremented. */
                gdk_pixbuf_ref(entry->any.pixbuf);
                return entry->any.pixbuf;
        } else {
                return NULL;
        }
}

/*
 * Builtin stock icons 
 */

struct _default_entries_data {
	const char *icon;
        GtkStateType state;
	const char *label;
	const gchar *rgb_data;
	int width, height;
	int scaled_width, scaled_height;
};

#define TB_W 20
#define TB_H 20
#define TIGERT_W 24
#define TIGERT_H 24
#define MENU_W 16
#define MENU_H 16

static const struct _default_entries_data entries_data[] = {
	{GNOME_STOCK_PIXMAP_NEW, GTK_STATE_NORMAL, NULL, imlib_new, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SAVE, GTK_STATE_NORMAL, NULL, imlib_save, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SAVE_AS, GTK_STATE_NORMAL, NULL, imlib_save_as, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_REVERT, GTK_STATE_NORMAL, NULL, imlib_revert, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_CUT, GTK_STATE_NORMAL, NULL, imlib_cut, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_HELP, GTK_STATE_NORMAL, NULL, imlib_help, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_PRINT, GTK_STATE_NORMAL, NULL, imlib_print, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SEARCH, GTK_STATE_NORMAL, NULL, imlib_search, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SRCHRPL, GTK_STATE_NORMAL, NULL, imlib_search_replace, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_BACK, GTK_STATE_NORMAL, NULL, imlib_left_arrow, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_FORWARD, GTK_STATE_NORMAL, NULL, imlib_right_arrow, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_FIRST, GTK_STATE_NORMAL, NULL, imlib_first, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_LAST, GTK_STATE_NORMAL, NULL, imlib_last, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_HOME, GTK_STATE_NORMAL, NULL, imlib_home, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_STOP, GTK_STATE_NORMAL, NULL, imlib_stop, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_REFRESH, GTK_STATE_NORMAL, NULL, imlib_refresh, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_UNDELETE, GTK_STATE_NORMAL, NULL, imlib_undelete, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_OPEN, GTK_STATE_NORMAL, NULL, imlib_open, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_CLOSE, GTK_STATE_NORMAL, NULL, imlib_close, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_COPY, GTK_STATE_NORMAL, NULL, imlib_copy, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_PASTE, GTK_STATE_NORMAL, NULL, imlib_paste, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_PROPERTIES, GTK_STATE_NORMAL, NULL, imlib_properties, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_PREFERENCES, GTK_STATE_NORMAL, NULL, imlib_preferences, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SCORES, GTK_STATE_NORMAL, NULL, imlib_scores, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_UNDO, GTK_STATE_NORMAL, NULL, imlib_undo, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_REDO, GTK_STATE_NORMAL, NULL, imlib_redo, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TIMER, GTK_STATE_NORMAL, NULL, imlib_timer, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TIMER_STOP, GTK_STATE_NORMAL, NULL, imlib_timer_stopped, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MAIL, GTK_STATE_NORMAL, NULL, imlib_mail, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MAIL_RCV, GTK_STATE_NORMAL, NULL, imlib_mail_receive, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MAIL_SND, GTK_STATE_NORMAL, NULL, imlib_mail_send, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MAIL_RPL, GTK_STATE_NORMAL, NULL, imlib_mail_reply, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MAIL_FWD, GTK_STATE_NORMAL, NULL, imlib_mail_forward, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MAIL_NEW, GTK_STATE_NORMAL, NULL, imlib_mail_compose, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TRASH, GTK_STATE_NORMAL, NULL, imlib_trash, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TRASH_FULL, GTK_STATE_NORMAL, NULL, imlib_trash_full, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_SPELLCHECK, GTK_STATE_NORMAL, NULL, imlib_spellcheck, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MIC, GTK_STATE_NORMAL, NULL, imlib_mic, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_VOLUME, GTK_STATE_NORMAL, NULL, imlib_volume, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MIDI, GTK_STATE_NORMAL, NULL, imlib_midi, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_LINE_IN, GTK_STATE_NORMAL, NULL, imlib_line_in, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_CDROM, GTK_STATE_NORMAL, NULL, imlib_cdrom, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_BOOK_RED, GTK_STATE_NORMAL, NULL, imlib_book_red, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_BOOK_GREEN, GTK_STATE_NORMAL, NULL, imlib_book_green, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_BOOK_BLUE, GTK_STATE_NORMAL, NULL, imlib_book_blue, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_BOOK_YELLOW, GTK_STATE_NORMAL, NULL, imlib_book_yellow, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_BOOK_OPEN, GTK_STATE_NORMAL, NULL, imlib_book_open, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_NOT, GTK_STATE_NORMAL, NULL, imlib_not, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_CONVERT, GTK_STATE_NORMAL, NULL, imlib_convert, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_JUMP_TO, GTK_STATE_NORMAL, NULL, imlib_jump_to, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_MULTIPLE, GTK_STATE_NORMAL, NULL, imlib_multiple_file, 32, 32, 32, 32},
	{GNOME_STOCK_PIXMAP_EXIT, GTK_STATE_NORMAL, NULL, imlib_exit, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_ABOUT, GTK_STATE_NORMAL, NULL, imlib_menu_about, MENU_W, MENU_H, MENU_W, MENU_H},
	{GNOME_STOCK_PIXMAP_UP, GTK_STATE_NORMAL, NULL, imlib_up_arrow, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_DOWN, GTK_STATE_NORMAL, NULL, imlib_down_arrow, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TOP, GTK_STATE_NORMAL, NULL, imlib_top, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_BOTTOM, GTK_STATE_NORMAL, NULL, imlib_bottom, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_ATTACH, GTK_STATE_NORMAL, NULL, imlib_attach, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_FONT, GTK_STATE_NORMAL, NULL, imlib_font, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_INDEX, GTK_STATE_NORMAL, NULL, imlib_index, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_EXEC, GTK_STATE_NORMAL, NULL, imlib_exec, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_ALIGN_LEFT, GTK_STATE_NORMAL, NULL, imlib_align_left, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_ALIGN_RIGHT, GTK_STATE_NORMAL, NULL, imlib_align_right, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_ALIGN_CENTER, GTK_STATE_NORMAL, NULL, imlib_align_center, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_ALIGN_JUSTIFY, GTK_STATE_NORMAL, NULL, imlib_align_justify, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TEXT_BOLD, GTK_STATE_NORMAL, NULL, imlib_text_bold, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TEXT_ITALIC, GTK_STATE_NORMAL, NULL, imlib_text_italic, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TEXT_UNDERLINE, GTK_STATE_NORMAL, NULL, imlib_text_underline, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},
	{GNOME_STOCK_PIXMAP_TEXT_STRIKEOUT, GTK_STATE_NORMAL, NULL, imlib_text_strikeout, TIGERT_W, TIGERT_H, TIGERT_W, TIGERT_H},

	{GNOME_STOCK_PIXMAP_ADD, GTK_STATE_NORMAL, N_("Add"), imlib_add, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_CLEAR, GTK_STATE_NORMAL, N_("Clear"), imlib_clear, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_COLORSELECTOR, GTK_STATE_NORMAL, N_("Select Color"), imlib_colorselector, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_REMOVE, GTK_STATE_NORMAL, N_("Remove"), imlib_remove, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_TABLE_BORDERS, GTK_STATE_NORMAL, N_("Table Borders"), imlib_table_borders, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_TABLE_FILL, GTK_STATE_NORMAL, N_("Table Fill"), imlib_table_fill, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_TEXT_BULLETED_LIST, GTK_STATE_NORMAL, N_("Bulleted List"), imlib_text_bulleted_list, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_TEXT_NUMBERED_LIST, GTK_STATE_NORMAL, N_("Numbered List"), imlib_text_numbered_list, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_TEXT_INDENT, GTK_STATE_NORMAL, N_("Indent"), imlib_text_indent, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_PIXMAP_TEXT_UNINDENT, GTK_STATE_NORMAL, N_("Un-Indent"), imlib_text_unindent, TIGERT_W, TIGERT_H, TB_W, TB_H},


	{GNOME_STOCK_BUTTON_OK, GTK_STATE_NORMAL, N_("OK"), imlib_button_ok, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_APPLY, GTK_STATE_NORMAL, N_("Apply"), imlib_button_apply, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_CANCEL, GTK_STATE_NORMAL, N_("Cancel"), imlib_button_cancel, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_CLOSE, GTK_STATE_NORMAL, N_("Close"), imlib_button_close, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_YES, GTK_STATE_NORMAL, N_("Yes"), imlib_button_yes, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_NO, GTK_STATE_NORMAL, N_("No"), imlib_button_no, TB_W, TB_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_HELP, GTK_STATE_NORMAL, N_("Help"), imlib_help, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_NEXT, GTK_STATE_NORMAL, N_("Next"), imlib_right_arrow, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_PREV, GTK_STATE_NORMAL, N_("Prev"), imlib_left_arrow, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_UP, GTK_STATE_NORMAL, N_("Up"), imlib_up_arrow, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_DOWN, GTK_STATE_NORMAL, N_("Down"), imlib_down_arrow, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_BUTTON_FONT, GTK_STATE_NORMAL, N_("Font"), imlib_font, TIGERT_W, TIGERT_H, TB_W, TB_H},
	{GNOME_STOCK_MENU_NEW, GTK_STATE_NORMAL, NULL, imlib_new, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_SAVE, GTK_STATE_NORMAL, NULL, imlib_save, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_SAVE_AS, GTK_STATE_NORMAL, NULL, imlib_save_as, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_REVERT, GTK_STATE_NORMAL, NULL, imlib_revert, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_OPEN, GTK_STATE_NORMAL, NULL, imlib_open, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_CLOSE, GTK_STATE_NORMAL, NULL, imlib_close, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_QUIT, GTK_STATE_NORMAL, NULL, imlib_exit, TB_W, TB_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_CUT, GTK_STATE_NORMAL, NULL, imlib_cut, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_COPY, GTK_STATE_NORMAL, NULL, imlib_copy, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_PASTE, GTK_STATE_NORMAL, NULL, imlib_paste, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_PROP, GTK_STATE_NORMAL, NULL, imlib_properties, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_PREF, GTK_STATE_NORMAL, NULL, imlib_preferences, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_UNDO, GTK_STATE_NORMAL, NULL, imlib_undo, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_REDO, GTK_STATE_NORMAL, NULL, imlib_redo, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_PRINT, GTK_STATE_NORMAL, NULL, imlib_print, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_SEARCH, GTK_STATE_NORMAL, NULL, imlib_search, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_SRCHRPL, GTK_STATE_NORMAL, NULL, imlib_search_replace, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_BACK, GTK_STATE_NORMAL, NULL, imlib_left_arrow, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_FORWARD, GTK_STATE_NORMAL, NULL, imlib_right_arrow, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_FIRST, GTK_STATE_NORMAL, NULL, imlib_first, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_LAST, GTK_STATE_NORMAL, NULL, imlib_last, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_HOME, GTK_STATE_NORMAL, NULL, imlib_home, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_STOP, GTK_STATE_NORMAL, NULL, imlib_stop, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_REFRESH, GTK_STATE_NORMAL, NULL, imlib_refresh, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_UNDELETE, GTK_STATE_NORMAL, NULL, imlib_undelete, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TIMER, GTK_STATE_NORMAL, NULL, imlib_timer, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TIMER_STOP, GTK_STATE_NORMAL, NULL, imlib_timer_stopped, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MAIL, GTK_STATE_NORMAL, NULL, imlib_mail, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MAIL_RCV, GTK_STATE_NORMAL, NULL, imlib_mail_receive, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MAIL_SND, GTK_STATE_NORMAL, NULL, imlib_mail_send, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MAIL_RPL, GTK_STATE_NORMAL, NULL, imlib_mail_reply, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MAIL_FWD, GTK_STATE_NORMAL, NULL, imlib_mail_forward, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MAIL_NEW, GTK_STATE_NORMAL, NULL, imlib_mail_compose, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TRASH, GTK_STATE_NORMAL, NULL, imlib_trash, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TRASH_FULL, GTK_STATE_NORMAL, NULL, imlib_trash_full, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_SPELLCHECK, GTK_STATE_NORMAL, NULL, imlib_spellcheck, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MIC, GTK_STATE_NORMAL, NULL, imlib_mic, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_LINE_IN, GTK_STATE_NORMAL, NULL, imlib_line_in, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_VOLUME, GTK_STATE_NORMAL, NULL, imlib_volume, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_MIDI, GTK_STATE_NORMAL, NULL, imlib_midi, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_CDROM, GTK_STATE_NORMAL, NULL, imlib_cdrom, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_BOOK_RED, GTK_STATE_NORMAL, NULL, imlib_book_red, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_BOOK_GREEN, GTK_STATE_NORMAL, NULL, imlib_book_green, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_BOOK_BLUE, GTK_STATE_NORMAL, NULL, imlib_book_blue, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_BOOK_YELLOW, GTK_STATE_NORMAL, NULL, imlib_book_yellow, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_BOOK_OPEN, GTK_STATE_NORMAL, NULL, imlib_book_open, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_CONVERT, GTK_STATE_NORMAL, NULL, imlib_convert, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_JUMP_TO, GTK_STATE_NORMAL, NULL, imlib_jump_to, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_ABOUT, GTK_STATE_NORMAL, NULL, imlib_menu_about, MENU_W, MENU_H, MENU_W, MENU_H},
	/* TODO: I shouldn't waste a pixmap for that */
	{GNOME_STOCK_MENU_BLANK, GTK_STATE_NORMAL, NULL, imlib_menu_blank, MENU_W, MENU_H, MENU_W, MENU_H}, 
	{GNOME_STOCK_MENU_SCORES, GTK_STATE_NORMAL, NULL, imlib_menu_scores, 20, 20, 20, 20},
	{GNOME_STOCK_MENU_UP, GTK_STATE_NORMAL, NULL, imlib_up_arrow, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_DOWN, GTK_STATE_NORMAL, NULL, imlib_down_arrow, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TOP, GTK_STATE_NORMAL, NULL, imlib_top, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_BOTTOM, GTK_STATE_NORMAL, NULL, imlib_bottom, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_ATTACH, GTK_STATE_NORMAL, NULL, imlib_attach, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_INDEX, GTK_STATE_NORMAL, NULL, imlib_index, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_FONT, GTK_STATE_NORMAL, NULL, imlib_font, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_EXEC, GTK_STATE_NORMAL, NULL, imlib_exec, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_ALIGN_LEFT, GTK_STATE_NORMAL, NULL, imlib_align_left, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_ALIGN_RIGHT, GTK_STATE_NORMAL, NULL, imlib_align_right, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_ALIGN_CENTER, GTK_STATE_NORMAL, NULL, imlib_align_center, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_ALIGN_JUSTIFY, GTK_STATE_NORMAL, NULL, imlib_align_justify, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TEXT_BOLD, GTK_STATE_NORMAL, NULL, imlib_text_bold, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TEXT_ITALIC, GTK_STATE_NORMAL, NULL, imlib_text_italic, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TEXT_UNDERLINE, GTK_STATE_NORMAL, NULL, imlib_text_underline, TIGERT_W, TIGERT_H, MENU_W, MENU_H},
	{GNOME_STOCK_MENU_TEXT_STRIKEOUT, GTK_STATE_NORMAL, NULL, imlib_text_strikeout, TIGERT_W, TIGERT_H, MENU_W, MENU_H},

};

static const int entries_data_num = sizeof(entries_data) / sizeof(entries_data[0]);


typedef struct _HashEntry HashEntry;

struct _HashEntry {
        /* indexed by state type */
        GnomeStockPixmapEntry *entries[5];
};

static HashEntry*
hash_entry_new (void)
{
        HashEntry* he;

        he = g_new0 (HashEntry, 1);

        return he;
}

static void
hash_entry_set (HashEntry *he, GtkStateType state, GnomeStockPixmapEntry *entry)
{
        g_return_if_fail (state < 5);
        
        if (he->entries[state] != NULL) {
                gnome_stock_pixmap_entry_unref (he->entries[state]);
                he->entries[state] = NULL;
        }

	gnome_stock_pixmap_entry_ref (entry);
        he->entries[state] = entry;
}

static void
hash_insert (GHashTable *hash, const gchar* icon, GtkStateType state,
             GnomeStockPixmapEntry *entry)
{
        gpointer key = NULL;
        gpointer value = NULL;
        
        if (g_hash_table_lookup_extended (hash, icon, &key, &value)) {
                HashEntry *he;
                
                he = value;

                g_assert (he != NULL);

                hash_entry_set (he, state, entry);

                return;
        } else {
                HashEntry *he;

                he = hash_entry_new ();

                hash_entry_set (he, state, entry);

                g_hash_table_insert (hash,
                                     g_strdup(icon),
                                     he);

                return;
        }
}

static GHashTable *
stock_pixmaps(void)
{
	static GHashTable *hash = NULL;
	int i;

	if (hash != NULL)
                return hash;

	hash = g_hash_table_new(g_str_hash, g_str_equal);

	for (i = 0; i < entries_data_num; i++) {
                GdkPixbuf *pixbuf;
                GnomeStockPixmapEntry *entry;

                pixbuf = gdk_pixbuf_new_from_data ( (guchar*) entries_data[i].rgb_data, /* cast const */
                                                    GDK_COLORSPACE_RGB,
                                                    TRUE,
                                                    8,
                                                    entries_data[i].width,
                                                    entries_data[i].height,
                                                    /* rowstride */
                                                    entries_data[i].width * 4,
                                                    NULL, /* don't destroy data */
                                                    NULL );
                
                
                entry = gnome_stock_pixmap_entry_new_from_gdk_pixbuf_at_size (pixbuf,
                                                                              entries_data[i].label,
                                                                              entries_data[i].scaled_width,
                                                                              entries_data[i].scaled_height);
                

                /* entry holds a ref */
                gdk_pixbuf_unref (pixbuf);

                hash_insert (hash, entries_data[i].icon, entries_data[i].state, entry);
		/* the above code has have reffed it in our database, so
		 * we need to get rid of the initial refcount */
		gnome_stock_pixmap_entry_unref(entry);
	}
        
	return hash;
}


static GnomeStockPixmapEntry **
lookup(const char *icon)
{
	GHashTable *hash = stock_pixmaps();
        HashEntry* he;

        he = g_hash_table_lookup(hash, icon);

        if (he == NULL)
                return NULL;
        else
                return he->entries;
}

void
gnome_stock_pixmap_register(const char *icon, GtkStateType state,
			    GnomeStockPixmapEntry *entry)
{
        GHashTable* hash;
        
	g_return_if_fail (icon != NULL);
	g_return_if_fail (entry != NULL);
	g_return_if_fail (GNOME_IS_STOCK_PIXMAP_ENTRY(entry));

        hash = stock_pixmaps();
        
        g_return_if_fail(g_hash_table_lookup(hash, icon) == NULL);
        
        hash_insert (hash, icon, state, entry);
}

void
gnome_stock_pixmap_change(const char *icon, GtkStateType state,
			  GnomeStockPixmapEntry *entry)
{
	GHashTable *hash;
        HashEntry *he;

	g_return_if_fail (icon != NULL);
	g_return_if_fail (entry != NULL);
	g_return_if_fail (GNOME_IS_STOCK_PIXMAP_ENTRY(entry));
        
	hash = stock_pixmaps();

        he = g_hash_table_lookup(hash, icon);

        if (he == NULL) {
                gnome_stock_pixmap_register(icon, state, entry);
        } else {
                hash_entry_set(he, state, entry);
        }
}



GnomeStockPixmapEntry *
gnome_stock_pixmap_lookup(const char *icon, GtkStateType state)
{
	GHashTable *hash;
        HashEntry *he;

	g_return_val_if_fail(icon != NULL, NULL);
        
	hash = stock_pixmaps();

        he = g_hash_table_lookup(hash, icon);

        if (he == NULL) {
                return NULL;
        } else {
                gnome_stock_pixmap_entry_ref(he->entries[state]);
                return he->entries[state];
        }
}

/*******************/
/*  stock buttons  */
/*******************/

GtkWidget *
gnome_pixmap_button(GtkWidget *pixmap, const char *text)
{
	GtkWidget *button, *label, *hbox, *w;
	gboolean use_icon, use_label;

	g_return_val_if_fail(text != NULL, NULL);

	button = gtk_button_new();
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(button), hbox);
	w = hbox;
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(w), hbox, TRUE, FALSE,
                           GNOME_STOCK_BUTTON_PADDING);

	use_icon = gnome_config_get_bool("/Gnome/Icons/ButtonUseIcons=true");
	use_label = gnome_config_get_bool("/Gnome/Icons/ButtonUseLabels=true");

	if ((use_label) || (!use_icon) || (!pixmap)) {
		label = gtk_label_new(_(text));
		gtk_widget_show(label);
		gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE,
                                 GNOME_STOCK_BUTTON_PADDING);
	}

	if ((use_icon) && (pixmap)) {

		gtk_widget_show(pixmap);
		gtk_box_pack_start(GTK_BOX(hbox), pixmap,
				   FALSE, FALSE, 0);
	} else {
                gtk_widget_unref(pixmap);
        }

	return button;
}

static GtkWidget *
stock_button_from_entries (const char *type, GnomeStockPixmapEntry **entries)
{
	char *text;
	GtkWidget *pixmap;

        /* FIXME we should actually change the label on state changes,
           or else not store a label for every state. */
	if (entries[GTK_STATE_NORMAL]->any.label)
		text = dgettext(PACKAGE, entries[GTK_STATE_NORMAL]->any.label);
	else
		text = dgettext(PACKAGE, type);

	pixmap = gnome_stock_new_with_icon(type);
	return gnome_pixmap_button(pixmap, text);
}

/**
 * gnome_stock_button:
 * @type: gnome stock type code.
 *
 * Constructs a new #GtkButton which contains a stock icon and text
 * whose type is @type.
 *
 * Returns: A configured #GtkButton widget
 */
GtkWidget *
gnome_stock_button(const char *type)
{
	GnomeStockPixmapEntry **entries;

	entries = lookup(type);

        g_return_val_if_fail(entries != NULL, NULL);

	return stock_button_from_entries (type, entries);
}

/**
 * gnome_stock_or_ordinary_button:
 * @type: gnome stock type code.
 *
 * It @type contains a valid GNOME stock code, it constructs a new
 * #GtkButton which contains a stock icon and text for the matching
 * type, or if the type does not exist, it creates a button with the
 * label being the same as "type".
 *
 * The use of this routine is discouraged, given that on international 
 * setups it might break subtly.
 *
 * Returns: A #GtkButton.
 */
GtkWidget *
gnome_stock_or_ordinary_button (const char *type)
{
        GnomeStockPixmapEntry **entries;

	entries = lookup(type);
        
        if (entries == NULL)
                return gtk_button_new_with_label (type);
        else
                return stock_button_from_entries (type, entries);
}


/***********/
/*  menus  */
/***********/

GtkWidget *
gnome_stock_menu_item(const char *type, const char *text)
{
	GtkWidget *hbox, *w, *menu_item;

	g_return_val_if_fail(type != NULL, NULL);
	g_return_val_if_fail(text != NULL, NULL);

 	if(gnome_preferences_get_menus_have_icons()) {
		hbox = gtk_hbox_new(FALSE, 2);
		gtk_widget_show(hbox);
		w = gnome_stock_new_with_icon(type);
		gtk_widget_show(w);
		gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);

		menu_item = gtk_menu_item_new();

		w = gtk_accel_label_new (text);
		gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (w), menu_item);
		gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
		gtk_widget_show(w);
		gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 0);

		gtk_container_add(GTK_CONTAINER(menu_item), hbox);
	} else {
		menu_item = gtk_menu_item_new_with_label(text);
	}

	return menu_item;
}


/***********************/
/*  menu accelerators  */
/***********************/

typedef struct _AccelEntry AccelEntry;
struct _AccelEntry {
	guchar key;
	guint8 mod;
};

struct default_AccelEntry {
	const char *type;
	AccelEntry entry;
};

static const struct default_AccelEntry default_accel_hash[] = {
	{GNOME_STOCK_MENU_NEW, {'N', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_OPEN, {'O', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_CLOSE, {'W', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_SAVE, {'S', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_SAVE_AS, {'S', GDK_SHIFT_MASK | GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_QUIT, {'Q', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_CUT, {'X', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_COPY, {'C', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_PASTE, {'V', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_PROP, {0, 0}},
	{GNOME_STOCK_MENU_PREF, {'E', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_ABOUT, {'A', GDK_MOD1_MASK}},
	{GNOME_STOCK_MENU_SCORES, {0, 0}},
	{GNOME_STOCK_MENU_UNDO, {'Z', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_PRINT, {'P', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_SEARCH, {'S', GDK_MOD1_MASK}},
	{GNOME_STOCK_MENU_BACK, {'B', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_FORWARD, {'F', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_UP, {'U', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_DOWN, {'D', GDK_CONTROL_MASK}},
	{GNOME_STOCK_MENU_MAIL, {0, 0}},
	{GNOME_STOCK_MENU_MAIL_RCV, {0, 0}},
	{GNOME_STOCK_MENU_MAIL_SND, {0, 0}},
	{NULL, {0,0}}
};


static char *
accel_to_string(const AccelEntry *entry)
{
	static char s[30];

	s[0] = 0;
	if (!entry->key) return NULL;
	if (entry->mod & GDK_CONTROL_MASK)
		strcat(s, "Ctl+");
	if (entry->mod & GDK_MOD1_MASK)
		strcat(s, "Alt+");
	if (entry->mod & GDK_SHIFT_MASK)
		strcat(s, "Shft+");
	if (entry->mod & GDK_MOD2_MASK)
		strcat(s, "Mod2+");
	if (entry->mod & GDK_MOD3_MASK)
		strcat(s, "Mod3+");
	if (entry->mod & GDK_MOD4_MASK)
		strcat(s, "Mod4+");
	if (entry->mod & GDK_MOD5_MASK)
		strcat(s, "Mod5+");
	if ((entry->key >= 'a') && (entry->key <= 'z')) {
		s[strlen(s) + 1] = 0;
		s[strlen(s)] = entry->key - 'a' + 'A';
	} else if ((entry->key >= 'A') && (entry->key <= 'Z')) {
		s[strlen(s) + 1] = 0;
		s[strlen(s)] = entry->key;
	} else {
		return NULL;
	}
	return s;
}


static void
accel_from_string(char *s, guchar *key, guint8 *mod)
{
	char *p, *p1;

	*mod = 0;
	*key = 0;
	if (!s) return;
	p = s;
	do {
		p1 = p;
		while ((*p) && (*p != '+')) p++;
		if (*p == '+') {
			*p = 0;
			if (0 == g_strcasecmp(p1, "Ctl"))
				*mod |= GDK_CONTROL_MASK;
			else if (0 == g_strcasecmp(p1, "Alt"))
				*mod |= GDK_MOD1_MASK;
			else if (0 == g_strcasecmp(p1, "Shft"))
				*mod |= GDK_SHIFT_MASK;
			else if (0 == g_strcasecmp(p1, "Mod2"))
				*mod |= GDK_MOD2_MASK;
			else if (0 == g_strcasecmp(p1, "Mod3"))
				*mod |= GDK_MOD3_MASK;
			else if (0 == g_strcasecmp(p1, "Mod4"))
				*mod |= GDK_MOD4_MASK;
			else if (0 == g_strcasecmp(p1, "Mod5"))
				*mod |= GDK_MOD5_MASK;
			*p = '+';
			p++;
		}
	} while (*p);
	if (p1[1] == 0) {
		*key = *p1;
	} else {
		*key = 0;
		*mod = 0;
		return;
	}
}


static void
accel_read_rc(gpointer key, gpointer value, gpointer data)
{
	char *path, *s;
	AccelEntry *entry = value;
	gboolean got_default;

	path = g_strconcat(data, key, "=", accel_to_string(value), NULL);
	s = gnome_config_get_string_with_default(path, &got_default);
	g_free(path);
	if (got_default) {
		g_free(s);
		return;
	}
	accel_from_string(s, &entry->key, &entry->mod);
	g_free(s);
}


static GHashTable *
accel_hash(void) {
	static GHashTable *hash = NULL;
	const struct default_AccelEntry *p;

	if (!hash) {
		hash = g_hash_table_new(g_str_hash, g_str_equal);
		for (p = default_accel_hash; p->type; p++)
			g_hash_table_insert(hash, (gpointer) p->type, (gpointer) &p->entry);
		g_hash_table_foreach(hash, accel_read_rc,
				     "/Gnome/Accelerators/");
	}
	return hash;
}

gboolean
gnome_stock_menu_accel(const char *type, guchar *key, guint8 *mod)
{
	AccelEntry *entry;

	entry = g_hash_table_lookup(accel_hash(), (char *)type);
	if (!entry) {
		*key = 0;
		*mod = 0;
		return FALSE;
	}

	*key = entry->key;
	*mod = entry->mod;
	return (*key != 0);
}

void
gnome_stock_menu_accel_parse(const char *section)
{
	g_return_if_fail(section != NULL);
	g_hash_table_foreach(accel_hash(), accel_read_rc,
			     (char *)section);
}


/********************************/
/*  accelerator definition box  */
/********************************/

#include <libgnomeui/gnome-messagebox.h>
#include <libgnomeui/gnome-propertybox.h>

static void
accel_dlg_apply(GtkWidget *box, int n)
{
	GtkCList *clist;
	char *section, *key, *s;
	int i;

	if (n != 0) return;
	clist = gtk_object_get_data(GTK_OBJECT(box), "clist");
	section = gtk_object_get_data(GTK_OBJECT(box), "section");
	for (i = 0; i < clist->rows; i++) {
		gtk_clist_get_text(clist, i, 0, &s);
		key = g_strconcat(section, s, NULL);
		gtk_clist_get_text(clist, i, 1, &s);
		gnome_config_set_string(key, s);
		g_free(key);
	}
	gnome_config_sync();
}



static void
accel_dlg_help(GtkWidget *box, int n)
{
	GtkWidget *w;
	
	w = gnome_message_box_new("No help available yet!", "info",
				 GNOME_STOCK_BUTTON_OK, NULL);
	gtk_widget_show(w);
}



static void
accel_dlg_select_ok(GtkWidget *widget, GtkWindow *window)
{
	AccelEntry entry;
	GtkToggleButton *check;
	gchar *key, *s, *s2;
	int row;

	key = gtk_entry_get_text(GTK_ENTRY(gtk_object_get_data(GTK_OBJECT(window), "key")));
	if (!key) {
		entry.key = 0;
		entry.mod = 0;
	} else {
		accel_from_string(key, &entry.key, &entry.mod);
		entry.mod = 0;
		check = gtk_object_get_data(GTK_OBJECT(window), "shift");
		if (check->active)
			entry.mod |= GDK_SHIFT_MASK;
		check = gtk_object_get_data(GTK_OBJECT(window), "control");
		if (check->active)
			entry.mod |= GDK_CONTROL_MASK;
		check = gtk_object_get_data(GTK_OBJECT(window), "alt");
		if (check->active)
			entry.mod |= GDK_MOD1_MASK;
	}
	row = GPOINTER_TO_INT (gtk_object_get_data(GTK_OBJECT(window), "row"));
	gtk_clist_get_text(GTK_CLIST(gtk_object_get_data(GTK_OBJECT(window), "clist")),
			   row, 1, &s);
	if (!s) s = "";
	s2 = accel_to_string(&entry);
	if (!s2) s2 = "";
	if (g_strcasecmp(s2, s)) {
		gnome_property_box_changed(GNOME_PROPERTY_BOX(gtk_object_get_data(GTK_OBJECT(window), "box")));
		gtk_clist_set_text(GTK_CLIST(gtk_object_get_data(GTK_OBJECT(window),
								 "clist")),
					     row, 1, accel_to_string(&entry));
	}
}



static void
accel_dlg_select(GtkCList *widget, gint row, gint col, GdkEventButton *event)
{
	AccelEntry entry;
	GtkTable *table;
	GtkWidget *window, *w;
	char *s;

	gtk_clist_unselect_row(widget, row, col);
	s = NULL;
	gtk_clist_get_text(widget, row, col, &s);
	if (s) {
		accel_from_string(s, &entry.key, &entry.mod);
	} else {
		entry.key = 0;
		entry.mod = 0;
	}
	window = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(window), "Menu Accelerator");
	gtk_object_set_data(GTK_OBJECT(window), "clist", widget);
	gtk_object_set_data(GTK_OBJECT(window), "row", GINT_TO_POINTER (row));
	gtk_object_set_data(GTK_OBJECT(window), "col", GINT_TO_POINTER (col));
	gtk_object_set_data(GTK_OBJECT(window), "box",
			    gtk_object_get_data(GTK_OBJECT(widget), "box"));

	table = (GtkTable *)gtk_table_new(0, 0, FALSE);
	gtk_widget_show(GTK_WIDGET(table));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox),
			  GTK_WIDGET(table));

	gtk_clist_get_text(GTK_CLIST(widget), row, 0, &s);
	s = g_strconcat("Accelerator for ", s, NULL);
	w = gtk_label_new(s);
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 2, 0, 1);

	w = gtk_label_new("Key:");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 1, 1, 2);
	w = gtk_entry_new();
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 1, 2, 1, 2);
	gtk_object_set_data(GTK_OBJECT(window), "key", w);
	if (accel_to_string(&entry)) {
		s = g_strdup(accel_to_string(&entry));
		if (strrchr(s, '+'))
			gtk_entry_set_text(GTK_ENTRY(w), strrchr(s, '+') + 1);
		else
			gtk_entry_set_text(GTK_ENTRY(w), s);
		g_free(s);
	}

	w = gtk_check_button_new_with_label("Control");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 2, 2, 3);
	gtk_object_set_data(GTK_OBJECT(window), "control", w);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				    entry.mod & GDK_CONTROL_MASK);

	w = gtk_check_button_new_with_label("Shift");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 2, 3, 4);
	gtk_object_set_data(GTK_OBJECT(window), "shift", w);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				    entry.mod & GDK_SHIFT_MASK);

	w = gtk_check_button_new_with_label("Alt");
	gtk_widget_show(w);
	gtk_table_attach_defaults(table, w, 0, 2, 4, 5);
	gtk_object_set_data(GTK_OBJECT(window), "alt", w);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				    entry.mod & GDK_MOD1_MASK);

	w = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);
	gtk_widget_show(w);
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  (GtkSignalFunc)gtk_widget_destroy,
				  GTK_OBJECT(window));
	gtk_box_pack_end_defaults(GTK_BOX(GTK_DIALOG(window)->action_area), w);
	w = gnome_stock_button(GNOME_STOCK_BUTTON_OK);
	gtk_widget_show(w);
	gtk_signal_connect(GTK_OBJECT(w), "clicked",
			   (GtkSignalFunc)accel_dlg_select_ok,
			   window);
	gtk_signal_connect_object(GTK_OBJECT(w), "clicked",
				  (GtkSignalFunc)gtk_widget_destroy,
				  GTK_OBJECT(window));
	gtk_box_pack_end_defaults(GTK_BOX(GTK_DIALOG(window)->action_area), w);
	gtk_widget_show(window);
}


/* make the compiler happy */
void gnome_stock_menu_accel_dlg(char *section);

void
gnome_stock_menu_accel_dlg(char *section)
{
	GnomePropertyBox *box;
	GtkWidget *w, *label;
	const struct default_AccelEntry *p;
	char *titles[2];
	char *row_data[2];

	box = GNOME_PROPERTY_BOX(gnome_property_box_new());
	gtk_window_set_title(GTK_WINDOW(box), _("Menu Accelerator Keys"));

	label = gtk_label_new(_("Global"));
	gtk_widget_show(label);
	titles[0] = _("Menu Item");
	titles[1] = _("Accelerator");
	w = gtk_clist_new_with_titles(2, titles);
	gtk_object_set_data(GTK_OBJECT(box), "clist", w);
	gtk_widget_set_usize(w, -1, 170);
	gtk_clist_set_column_width(GTK_CLIST(w), 0, 100);
	gtk_clist_column_titles_passive(GTK_CLIST(w));
	gtk_widget_show(w);
	gtk_signal_connect(GTK_OBJECT(w), "select_row",
			   (GtkSignalFunc)accel_dlg_select,
			   NULL);
	gtk_object_set_data(GTK_OBJECT(w), "box", box);
	for (p = default_accel_hash; p->type; p++) {
		row_data[0] = (char*)p->type;
		row_data[1] = g_strdup(accel_to_string(&p->entry));
		gtk_clist_append(GTK_CLIST(w), row_data);
		g_free(row_data[1]);
	}
	gnome_property_box_append_page(box, w, label);

	if (!section) {
		gtk_object_set_data(GTK_OBJECT(box), "section",
				    "/Gnome/Accelerators/");
	} else {
		gtk_object_set_data(GTK_OBJECT(box), "section", section);
		/* TODO: maybe add another page for the app's menu accelerator
		 * config */
	}

	gtk_signal_connect(GTK_OBJECT(box), "apply",
			  (GtkSignalFunc)accel_dlg_apply, NULL);
	gtk_signal_connect(GTK_OBJECT(box), "help",
			  (GtkSignalFunc)accel_dlg_help, NULL);

	gtk_widget_show(GTK_WIDGET(box));
}

GtkWidget *
gnome_stock_transparent_window (const char *icon, GtkStateType state)
{
	GnomeStockPixmapEntry **entries = NULL;
	GtkWidget *window = NULL;
        GdkPixbuf *pixbuf = NULL;
        GdkBitmap *mask = NULL;
        GdkPixmap *pixmap = NULL;
        
	g_return_val_if_fail(icon != NULL, NULL);

        entries = lookup(icon);
	
	window = NULL;

        if (entries[state] == NULL)
                return NULL;
        
        pixbuf = gnome_stock_pixmap_entry_get_gdk_pixbuf(entries[state]);

        if (pixbuf == NULL)
                return NULL;
        
	/* Create the window on GdkRGB's visual */
	gtk_widget_push_visual (gdk_rgb_get_visual ());
	gtk_widget_push_colormap (gdk_rgb_get_cmap ());
	window = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();

	/* Force realization */
	gtk_widget_realize (window);

	/* Kids:  DO not do this.  I repeat.  Do not do this.
	 * we are trained professionals and we know what we are doing.
	 *
	 * Here is an explanation in case you care:
	 *   We break the GDK abstraction here, as older Gdks do not
	 *   support the Xserver SaveUnder flag.  We do set the
	 *   windows's realize bit and we create the window manually
	 *   setting the saveunder flag.
	 */
	/* Set proper size */
	gtk_widget_set_usize (window,
                              gdk_pixbuf_get_width(pixbuf),
                              gdk_pixbuf_get_height(pixbuf));


        /* generate mask */
        if (gdk_pixbuf_get_has_alpha(pixbuf)) {
                mask = gdk_pixmap_new(NULL,
                                      gdk_pixbuf_get_width(pixbuf),
                                      gdk_pixbuf_get_height(pixbuf),
                                      1);

                gdk_pixbuf_render_threshold_alpha(pixbuf, mask,
                                                  0, 0, 0, 0,
                                                  gdk_pixbuf_get_width(pixbuf),
                                                  gdk_pixbuf_get_height(pixbuf),
                                                  128); /* 50% alpha */
        }

        /* Draw to a pixmap */
        
        pixmap = gdk_pixmap_new(window->window,
                                gdk_pixbuf_get_width(pixbuf),
                                gdk_pixbuf_get_height(pixbuf),
                                -1);

        gdk_pixbuf_render_to_drawable(pixbuf, pixmap,
                                      window->style->white_gc,
                                      0, 0, 0, 0,
                                      gdk_pixbuf_get_width(pixbuf),
                                      gdk_pixbuf_get_height(pixbuf),
                                      GDK_RGB_DITHER_NORMAL,
                                      0, 0);
        
        /* Install pixmap/mask in the window */
	gdk_window_set_back_pixmap (window->window, pixmap, FALSE);

        gdk_pixmap_unref(pixmap);

        /* This appears to take ownership of the mask */
	gtk_widget_shape_combine_mask (window, mask, 0, 0);

        /* Destroy the pixbuf */
        gdk_pixbuf_unref(pixbuf);
	
	return window;
}

void 
gnome_stock_pixmap_gdk (const char    *icon,
                        GtkStateType   state,
			GdkPixmap    **pixmap,
			GdkPixmap    **mask)
{
	GnomeStockPixmapEntry **entries;
        GdkPixbuf *pixbuf;
        
	g_return_if_fail(icon != NULL);
	g_return_if_fail(pixmap != NULL);
	g_return_if_fail(mask != NULL);

        entries = lookup(icon);

        if (entries == NULL)
                return;

        pixbuf = gnome_stock_pixmap_entry_get_gdk_pixbuf(entries[state]);

        gdk_pixbuf_render_pixmap_and_mask(pixbuf,
                                          pixmap,
                                          mask,
                                          128);

        gdk_pixbuf_unref(pixbuf);
}

