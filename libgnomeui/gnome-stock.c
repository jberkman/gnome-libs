#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18n.h"
#include "gnome-stock.h"
#include "gnome-pixmap.h"

#include "../programs/gtt/tb_new.xpm"
#include "../programs/gtt/tb_save.xpm"
#include "../programs/gtt/tb_open.xpm"
#include "../programs/gtt/tb_cut.xpm"
#include "../programs/gtt/tb_copy.xpm"
#include "../programs/gtt/tb_paste.xpm"
#include "../programs/gtt/tb_properties.xpm"
#include "../programs/gtt/tb_unknown.xpm"
#include "../programs/gtt/tb_exit.xpm"
#include "gnome-stock-ok.xpm"
#include "gnome-stock-cancel.xpm"


#define STOCK_SEP '.'
#define STOCK_SEP_STR "."

/* The stock buttons suck at this time. So maybe we should undefine
   BUTTON_WANT_ICON. If it is undefined, the stock buttons will not
   include a pixmap. I leave this defined, so that ppl see a
   difference between stock buttons and application buttons. */
#define BUTTON_WANT_ICON

/* Hmmm, I'm not sure, if using color context can consume to much
   colors, so one can define this here. If defined, all color context
   modes other then true/pseudo color and gray lead to the old shading
   method for insensitive pixmaps. If undefined, the newer method will
   allways be used */
#undef NOT_ALLWAYS_SHADE


/**************************/
/* GnomeStockPixmapWidget */
/**************************/

static GtkVBoxClass *parent_class = NULL;

static void
gnome_stock_pixmap_widget_destroy(GtkObject *object)
{
	GnomeStockPixmapWidget *w;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_STOCK_PIXMAP_WIDGET (object));

	w = GNOME_STOCK_PIXMAP_WIDGET (object);
	
	/* free resources */
	if (w->regular) gtk_widget_destroy(GTK_WIDGET(w->regular));
	if (w->disabled) gtk_widget_destroy(GTK_WIDGET(w->disabled));
	if (w->focused) gtk_widget_destroy(GTK_WIDGET(w->focused));
        if (w->icon) g_free(w->icon);

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}



static void
gnome_stock_pixmap_widget_state_changed(GtkWidget *widget, guint prev_state)
{
        GnomeStockPixmapWidget *w = GNOME_STOCK_PIXMAP_WIDGET(widget);
        GtkPixmap *pixmap;

#ifdef DEBUG
        g_return_if_fail(GTK_WIDGET_REALIZED(widget));
#else /* DEBUG */
        if (!GTK_WIDGET_REALIZED(widget)) return;
#endif /* DEBUG */
        pixmap = NULL;
        if (GTK_WIDGET_HAS_FOCUS(widget)) {
                if (!w->focused) {
                        w->focused = gnome_stock_pixmap(w->window, w->icon, GNOME_STOCK_PIXMAP_FOCUSED);
                        gtk_widget_show(GTK_WIDGET(w->focused));
                }
                pixmap = w->focused;
        } else if (!GTK_WIDGET_IS_SENSITIVE(widget)) {
                if (!w->disabled) {
                        w->disabled = gnome_stock_pixmap(w->window, w->icon, GNOME_STOCK_PIXMAP_DISABLED);
                        gtk_widget_show(GTK_WIDGET(w->disabled));
                }
                pixmap = w->disabled;
        } else {
                if (!w->regular) {
                        w->regular = gnome_stock_pixmap(w->window, w->icon, GNOME_STOCK_PIXMAP_REGULAR);
                        gtk_widget_show(GTK_WIDGET(w->regular));
                }
                pixmap = w->regular;
        }
        if (pixmap == w->pixmap) return;
        if (w->pixmap) {
                gtk_container_remove(GTK_CONTAINER(w), GTK_WIDGET(w->pixmap));
                gtk_widget_queue_draw(widget);
        }
        w->pixmap = pixmap;
        if (pixmap) {
                gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(w->pixmap));
                gtk_widget_queue_draw(widget);
        }
}



static void
gnome_stock_pixmap_widget_realize(GtkWidget *widget)
{
        GnomeStockPixmapWidget *p;

        g_return_if_fail(widget != NULL);
        if (parent_class)
                (* GTK_WIDGET_CLASS(parent_class)->realize)(widget);
        p = GNOME_STOCK_PIXMAP_WIDGET(widget);
        if (p->pixmap) return;
        g_return_if_fail(p->window != NULL);
        gnome_stock_pixmap_widget_state_changed(widget, 0);
}



static void
gnome_stock_pixmap_widget_class_init(GnomeStockPixmapWidgetClass *klass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
	object_class->destroy = gnome_stock_pixmap_widget_destroy;
        ((GtkWidgetClass *)klass)->state_changed =
                gnome_stock_pixmap_widget_state_changed;
        ((GtkWidgetClass *)klass)->realize =
                gnome_stock_pixmap_widget_realize;
}



static void
gnome_stock_pixmap_widget_init(GtkObject *obj)
{
        GnomeStockPixmapWidget *w;

        g_return_if_fail(obj != NULL);
        g_return_if_fail(GNOME_IS_STOCK_PIXMAP_WIDGET(obj));

        w = GNOME_STOCK_PIXMAP_WIDGET(obj);
        w->icon = NULL;
        w->window = NULL;
        w->pixmap = NULL;
        w->regular = NULL;
        w->disabled = NULL;
        w->focused = NULL;
}



guint
gnome_stock_pixmap_widget_get_type(void)
{
	static guint new_type = 0;
	if (!new_type) {
		GtkTypeInfo type_info = {
			"GnomeStockPixmapWidget",
			sizeof(GnomeStockPixmapWidget),
			sizeof(GnomeStockPixmapWidgetClass),
			(GtkClassInitFunc)gnome_stock_pixmap_widget_class_init,
			(GtkObjectInitFunc)gnome_stock_pixmap_widget_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};
		new_type = gtk_type_unique(gtk_vbox_get_type(), &type_info);
		parent_class = gtk_type_class(gtk_vbox_get_type());
	}
	return new_type;
}



GtkWidget *
gnome_stock_pixmap_widget_new(GtkWidget *window, char *icon)
{
	GtkWidget *w;
        GnomeStockPixmapWidget *p;

        g_return_val_if_fail(icon != NULL, NULL);
        g_return_val_if_fail(gnome_stock_pixmap_checkfor(icon, GNOME_STOCK_PIXMAP_REGULAR), NULL);

	w = gtk_type_new(gnome_stock_pixmap_widget_get_type());
        p = GNOME_STOCK_PIXMAP_WIDGET(w);
        p->icon = g_strdup(icon);
        p->window = window;

	return w;
}



/****************/
/* some helpers */
/****************/


struct _default_entries_data {
        char *icon, *subtype;
        gchar **xpm_data;
};

struct _default_entries_data entries_data[] = {
        {GNOME_STOCK_PIXMAP_NEW, GNOME_STOCK_PIXMAP_REGULAR, tb_new_xpm},
        {GNOME_STOCK_PIXMAP_SAVE, GNOME_STOCK_PIXMAP_REGULAR, tb_save_xpm},
        {GNOME_STOCK_PIXMAP_OPEN, GNOME_STOCK_PIXMAP_REGULAR, tb_open_xpm},
        {GNOME_STOCK_PIXMAP_CUT, GNOME_STOCK_PIXMAP_REGULAR, tb_cut_xpm},
        {GNOME_STOCK_PIXMAP_COPY, GNOME_STOCK_PIXMAP_REGULAR, tb_copy_xpm},
        {GNOME_STOCK_PIXMAP_PASTE, GNOME_STOCK_PIXMAP_REGULAR, tb_paste_xpm},
        {GNOME_STOCK_PIXMAP_PROPERTIES, GNOME_STOCK_PIXMAP_REGULAR, tb_properties_xpm},
/*         {GNOME_STOCK_PIXMAP_PROPERTIES, GNOME_STOCK_PIXMAP_DISABLED, tb_prop_dis_xpm}, */
        {GNOME_STOCK_PIXMAP_HELP, GNOME_STOCK_PIXMAP_REGULAR, tb_unknown_xpm},
        {GNOME_STOCK_BUTTON_OK, GNOME_STOCK_PIXMAP_REGULAR, gnome_stock_ok_xpm},
        {GNOME_STOCK_BUTTON_APPLY, GNOME_STOCK_PIXMAP_REGULAR, gnome_stock_ok_xpm},
        {GNOME_STOCK_BUTTON_CLOSE, GNOME_STOCK_PIXMAP_REGULAR, tb_exit_xpm},
        {GNOME_STOCK_BUTTON_CANCEL, GNOME_STOCK_PIXMAP_REGULAR, gnome_stock_cancel_xpm},
};
static int entries_data_num = sizeof(entries_data) / sizeof(entries_data[0]);


static char *
build_hash_key(char *icon, char *subtype)
{
        char *s;

        s = g_malloc(strlen(icon) + strlen(subtype) + 2);
        strcpy(s, icon);
        strcat(s, STOCK_SEP_STR);
        if (subtype)
                strcat(s, subtype);
        else
                strcat(s, GNOME_STOCK_PIXMAP_REGULAR);
        return s;
}



static GHashTable *
stock_pixmaps(void)
{
        static GHashTable *hash = NULL;
        GnomeStockPixmapEntry *entry;
        int i;

        if (hash) return hash;

        hash = g_hash_table_new(g_str_hash, g_str_equal);

        for (i = 0; i < entries_data_num; i++) {
                entry = g_malloc(sizeof(GnomeStockPixmapEntry));
                entry->type = GNOME_STOCK_PIXMAP_TYPE_DATA;
                entry->data.xpm_data = entries_data[i].xpm_data;
                g_hash_table_insert(hash,
                                    build_hash_key(entries_data[i].icon,
                                                   entries_data[i].subtype),
                                    entry);
        }

        return hash;
}


static GnomeStockPixmapEntry *
lookup(char *icon, char *subtype, int fallback)
{
        char *s;
        GHashTable *hash = stock_pixmaps();
        GnomeStockPixmapEntry *entry;

        s = build_hash_key(icon, subtype);
        entry = (GnomeStockPixmapEntry *)g_hash_table_lookup(hash, s);
        if (!entry) {
                if (!fallback) return NULL;
                g_free(s);
                s = build_hash_key(icon, GNOME_STOCK_PIXMAP_REGULAR);
                entry = (GnomeStockPixmapEntry *)
                        g_hash_table_lookup(hash, s);
        }
        g_free(s);
        return entry;
}



static GtkPixmap *
create_pixmap_from_data(GtkWidget *window, GnomeStockPixmapEntryData *data)
{
        GtkPixmap *pixmap;

	pixmap = GTK_PIXMAP(gnome_create_pixmap_widget_d(window, window,
                                                         data->xpm_data));
        return pixmap;
}



static void
build_disabled_pixmap(GtkWidget *window, GtkPixmap **inout_pixmap)
{
        GdkGC *gc;
        GdkWindow *pixmap = (*inout_pixmap)->pixmap;
        GtkStyle *style;
        gint w, h, x, y;
        GdkGCValues vals;
        GdkVisual *visual;
        GdkImage *image;
        GdkColorContext *cc;
        GdkColor color;
        GdkColormap *cmap;
        gint32 red, green, blue;

        g_return_if_fail(window->window != NULL);
        style = gtk_widget_get_style(window);
        gc = style->bg_gc[GTK_STATE_INSENSITIVE];
        gdk_window_get_size(pixmap, &w, &h);
        visual = gdk_window_get_visual(window->window);
        cmap = gdk_window_get_colormap(window->window);
        cc = gdk_color_context_new(visual, cmap);
#ifdef NOT_ALLWAYS_SHADE
        if ((cc->mode != GDK_CC_MODE_TRUE) &&
            (cc->mode != GDK_CC_MODE_MY_GRAY)) {
                /* preserve colors */
                gdk_color_context_free(cc);
                for (y = 0; y < h; y ++) {
                        for (x = y % 2; x < w; x += 2) {
                                gdk_draw_point(pixmap, gc, x, y);
                        }
                }
                return;
        }
#endif
        image = gdk_image_get(pixmap, 0, 0, w, h);
        gdk_gc_get_values(gc, &vals);
        color.pixel = vals.foreground.pixel;
        gdk_color_context_query_color(cc, &color);
        red = color.red;
        green = color.green;
        blue = color.blue;
        for (y = 0; y < h; y++) {
                for (x = 0; x < w; x++) {
                        GdkColor c;
                        int failed;
                        c.pixel = gdk_image_get_pixel(image, x, y);
                        gdk_color_context_query_color(cc, &c);
                        c.red = (((gint32)c.red - red) >> 1)
                                + red;
                        c.green = (((gint32)c.green - green) >> 1)
                                + green;
                        c.blue = (((gint32)c.blue - blue) >> 1)
                                + blue;
                        c.pixel = gdk_color_context_get_pixel(cc, c.red,
                                                              c.green,
                                                              c.blue,
                                                              &failed);
                        gdk_image_put_pixel(image, x, y, c.pixel);
                }
        }
        gdk_draw_image(pixmap, gc, image, 0, 0, 0, 0, w, h);
        gdk_image_destroy(image);
        gdk_color_context_free(cc);
}



/**********************/
/* utitlity functions */
/**********************/



GtkPixmap *
gnome_stock_pixmap(GtkWidget *window, char *icon, char *subtype)
{
        GnomeStockPixmapEntry *entry;
        GtkPixmap *pixmap;

        g_return_val_if_fail(icon != NULL, NULL);
        g_return_val_if_fail(window != NULL, NULL);
        g_return_val_if_fail(GTK_IS_WIDGET(window), NULL);
        /* subtype can be NULL, so not checked */

        entry = lookup(icon, subtype, TRUE);
        if (!entry) return NULL;
        pixmap = NULL;
        switch (entry->type) {
        case GNOME_STOCK_PIXMAP_TYPE_DATA:
                pixmap = create_pixmap_from_data(window, &(entry->data));
                break;
        default:
                g_assert_not_reached();
                break;
        }
        /* check if we have to draw our own disabled pixmap */
        /* TODO: should be optimized a bit */
        if ((NULL == lookup(icon, subtype, 0)) && (pixmap) &&
            (0 == strcmp(subtype, GNOME_STOCK_PIXMAP_DISABLED))) {
                build_disabled_pixmap(window, &pixmap);
        }
        return pixmap;
}



GtkWidget *
gnome_stock_pixmap_widget(GtkWidget *window, char *icon)
{
        GtkWidget *w;

        w = gnome_stock_pixmap_widget_new(window, icon);
        return w;
}


gint
gnome_stock_pixmap_register(char *icon, char *subtype,
                            GnomeStockPixmapEntry *entry)
{
        g_return_val_if_fail(NULL == lookup(icon, subtype, 0), 0);
        g_return_val_if_fail(entry != NULL, 0);
        g_hash_table_insert(stock_pixmaps(), build_hash_key(icon, subtype),
                            entry);
        return 1;
}



gint
gnome_stock_pixmap_change(char *icon, char *subtype,
                          GnomeStockPixmapEntry *entry)
{
        GHashTable *hash;
        char *key;

        g_return_val_if_fail(NULL != lookup(icon, subtype, 0), 0);
        g_return_val_if_fail(entry != NULL, 0);
        hash = stock_pixmaps();
        key = build_hash_key(icon, subtype);
        g_hash_table_remove(hash, key);
        g_hash_table_insert(hash, key, entry);
        /* TODO: add some method to change all pixmaps (or at least
           the pixmap_widgets) to the new icon */
        return 1;
}



GnomeStockPixmapEntry *
gnome_stock_pixmap_checkfor(char *icon, char *subtype)
{
        return lookup(icon, subtype, 0);
}



GtkWidget *
gnome_stock_button(char *type)
{
        GtkWidget *button, *label, *hbox;
#ifdef BUTTON_WANT_ICON
        GtkWidget *pixmap;
#endif /* BUTTON_WANT_ICON */

        button = gtk_button_new();
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_widget_show(hbox);
        gtk_container_add(GTK_CONTAINER(button), hbox);
        label = gtk_label_new(gettext(type));
        gtk_widget_show(label);
        gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 7);

#ifdef BUTTON_WANT_ICON
        pixmap = gnome_stock_pixmap_widget(button, type);
        if (pixmap) {
                gtk_widget_show(pixmap);
                gtk_box_pack_start_defaults(GTK_BOX(hbox), pixmap);
        }
#endif /* BUTTON_WANT_ICON */
        return button;
}
