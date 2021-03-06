/* gnome-druid-page-finish.c
 * Copyright (C) 1999  Red Hat, Inc.
 *
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/

#include <config.h>

#include <glib.h>

#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"

#include "libgnomeui/gnome-uidefs.h"
#include "libgnomeui/gnome-canvas.h"
#include "libgnomeui/gnome-canvas-text.h"
#include "libgnomeui/gnome-canvas-rect-ellipse.h"
#include "gnome-druid-page-finish.h"
#include "libgnomeui/gnome-canvas-pixbuf.h"
#include "libgnomeui/gnome-druid.h"

struct _GnomeDruidPageFinishPrivate
{
	GtkWidget *canvas;
	GnomeCanvasItem *background_item;
	GnomeCanvasItem *textbox_item;
	GnomeCanvasItem *text_item;
	GnomeCanvasItem *logo_item;
	GnomeCanvasItem *logoframe_item;
	GnomeCanvasItem *watermark_item;
	GnomeCanvasItem *title_item;
};

static void gnome_druid_page_finish_init 	  (GnomeDruidPageFinish		 *druid_page_finish);
static void gnome_druid_page_finish_class_init	  (GnomeDruidPageFinishClass	 *klass);
static void gnome_druid_page_finish_destroy 	  (GtkObject                     *object);
static void gnome_druid_page_finish_finalize 	  (GObject                       *object);
static void gnome_druid_page_finish_construct     (GnomeDruidPageFinish          *druid_page_finish);
static void gnome_druid_page_finish_configure_size(GnomeDruidPageFinish          *druid_page_finish,
						   gint                           width,
						   gint                           height);
static void gnome_druid_page_finish_size_allocate (GtkWidget                     *widget,
						   GtkAllocation                 *allocation);
static void gnome_druid_page_finish_realize       (GtkWidget                     *widget);
static void gnome_druid_page_finish_prepare	  (GnomeDruidPage		 *page,
						   GtkWidget                     *druid,
						   gpointer 			 *data);
static GnomeDruidPageClass *parent_class = NULL;

#define LOGO_WIDTH 50.0
#define DRUID_PAGE_HEIGHT 318
#define DRUID_PAGE_WIDTH 516
#define DRUID_PAGE_LEFT_WIDTH 100.0

GtkType
gnome_druid_page_finish_get_type (void)
{
  static GtkType druid_page_finish_type = 0;

  if (!druid_page_finish_type)
    {
      static const GtkTypeInfo druid_page_finish_info =
      {
        "GnomeDruidPageFinish",
        sizeof (GnomeDruidPageFinish),
        sizeof (GnomeDruidPageFinishClass),
        (GtkClassInitFunc) gnome_druid_page_finish_class_init,
        (GtkObjectInitFunc) gnome_druid_page_finish_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      druid_page_finish_type = gtk_type_unique (gnome_druid_page_get_type (), &druid_page_finish_info);
    }

  return druid_page_finish_type;
}

static void
gnome_druid_page_finish_class_init (GnomeDruidPageFinishClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) klass;
	gobject_class = (GObjectClass*) klass;
	object_class->destroy = gnome_druid_page_finish_destroy;
	gobject_class->finalize = gnome_druid_page_finish_finalize;
	widget_class = (GtkWidgetClass*) klass;
	widget_class->size_allocate = gnome_druid_page_finish_size_allocate;
	widget_class->realize = gnome_druid_page_finish_realize;
	parent_class = gtk_type_class (gnome_druid_page_get_type ());
}

static void
gnome_druid_page_finish_init (GnomeDruidPageFinish *druid_page_finish)
{
	druid_page_finish->_priv = g_new0(GnomeDruidPageFinishPrivate, 1);

	/* initialize the color values */
	druid_page_finish->background_color.red = 6400; /* midnight blue */
	druid_page_finish->background_color.green = 6400;
	druid_page_finish->background_color.blue = 28672;
	druid_page_finish->textbox_color.red = 65280; /* white */
	druid_page_finish->textbox_color.green = 65280;
	druid_page_finish->textbox_color.blue = 65280;
	druid_page_finish->logo_background_color.red = 65280; /* white */
	druid_page_finish->logo_background_color.green = 65280;
	druid_page_finish->logo_background_color.blue = 65280;
	druid_page_finish->title_color.red = 65280; /* white */
	druid_page_finish->title_color.green = 65280;
	druid_page_finish->title_color.blue = 65280;
	druid_page_finish->text_color.red = 0; /* black */
	druid_page_finish->text_color.green = 0;
	druid_page_finish->text_color.blue = 0;

	/* Set up the canvas */ 
	gtk_container_set_border_width (GTK_CONTAINER (druid_page_finish), 0);
	druid_page_finish->_priv->canvas = gnome_canvas_new ();
	gtk_widget_set_usize (druid_page_finish->_priv->canvas, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_widget_show (druid_page_finish->_priv->canvas);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (druid_page_finish->_priv->canvas), 0.0, 0.0, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_container_add (GTK_CONTAINER (druid_page_finish), druid_page_finish->_priv->canvas);
}

static void
gnome_druid_page_finish_destroy(GtkObject *object)
{
	/* remember, destroy can be run multiple times! */

	if (GNOME_DRUID_PAGE_FINISH (object)->logo_image)
		gdk_pixbuf_unref (GNOME_DRUID_PAGE_FINISH (object)->logo_image);
	GNOME_DRUID_PAGE_FINISH (object)->logo_image = NULL;

	if (GNOME_DRUID_PAGE_FINISH (object)->watermark_image)
		gdk_pixbuf_unref (GNOME_DRUID_PAGE_FINISH (object)->watermark_image);
	GNOME_DRUID_PAGE_FINISH (object)->watermark_image = NULL;

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void
gnome_druid_page_finish_finalize(GObject *object)
{
	GnomeDruidPageFinish *druid_page_finish = GNOME_DRUID_PAGE_FINISH(object);

	g_free(druid_page_finish->_priv);
	druid_page_finish->_priv = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(object);
}

static void
gnome_druid_page_finish_configure_size (GnomeDruidPageFinish *druid_page_finish, gint width, gint height)
{
	gfloat watermark_width = DRUID_PAGE_LEFT_WIDTH;
	gfloat watermark_height = (gfloat) height - LOGO_WIDTH + GNOME_PAD * 2.0;
	gfloat watermark_ypos = LOGO_WIDTH + GNOME_PAD * 2.0;

	if (druid_page_finish->watermark_image) {
		watermark_width = gdk_pixbuf_get_width (druid_page_finish->watermark_image);
		watermark_height = gdk_pixbuf_get_width (druid_page_finish->watermark_image);
		watermark_ypos = (gfloat) height - watermark_height;
		if (watermark_width < 1)
			watermark_width = 1.0;
		if (watermark_height < 1)
			watermark_height = 1.0;
	}

	gnome_canvas_item_set (druid_page_finish->_priv->background_item,
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (gfloat) width,
			       "y2", (gfloat) height,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->textbox_item,
			       "x1", (gfloat) watermark_width,
			       "y1", (gfloat) LOGO_WIDTH + GNOME_PAD * 2.0,
			       "x2", (gfloat) width,
			       "y2", (gfloat) height,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->logoframe_item,
			       "x1", (gfloat) width - LOGO_WIDTH -GNOME_PAD,
			       "y1", (gfloat) GNOME_PAD,
			       "x2", (gfloat) width - GNOME_PAD,
			       "y2", (gfloat) GNOME_PAD + LOGO_WIDTH,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->logo_item,
			       "x", (gfloat) width - GNOME_PAD - LOGO_WIDTH,
			       "y", (gfloat) GNOME_PAD,
			       "anchor", GTK_ANCHOR_NORTH_WEST,
			       "width", (gfloat) LOGO_WIDTH,
			       "height", (gfloat) LOGO_WIDTH, NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->watermark_item,
			       "x", 0.0,
			       "y", watermark_ypos,
			       "anchor", GTK_ANCHOR_NORTH_WEST,
			       "width", watermark_width,
			       "height", watermark_height,
			       NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->title_item,
			       "x", 15.0, 
			       "y", (gfloat) GNOME_PAD + LOGO_WIDTH / 2.0,
			       "anchor", GTK_ANCHOR_WEST,
			       NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->text_item,
			       "x", ((width - watermark_width) * 0.5) + watermark_width,
			       "y", LOGO_WIDTH + GNOME_PAD * 2.0 + (height - (LOGO_WIDTH + GNOME_PAD * 2.0))/ 2.0,
			       "anchor", GTK_ANCHOR_CENTER,
			       NULL);
}

static void
gnome_druid_page_finish_construct (GnomeDruidPageFinish *druid_page_finish)
{
	/* set up the rest of the page */
	druid_page_finish->_priv->background_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_finish->_priv->canvas)),
				       gnome_canvas_rect_get_type (), NULL);
	druid_page_finish->_priv->textbox_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_finish->_priv->canvas)),
				       gnome_canvas_rect_get_type (), NULL);
	druid_page_finish->_priv->logoframe_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_finish->_priv->canvas)),
				       gnome_canvas_rect_get_type (), NULL);
	druid_page_finish->_priv->logo_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_finish->_priv->canvas)),
				       gnome_canvas_pixbuf_get_type (), NULL);
	if (druid_page_finish->logo_image != NULL)
		gnome_canvas_item_set (druid_page_finish->_priv->logo_item,
				       "pixbuf", druid_page_finish->logo_image, NULL);
	druid_page_finish->_priv->watermark_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_finish->_priv->canvas)),
				       gnome_canvas_pixbuf_get_type (),
				       "pixbuf", druid_page_finish->watermark_image, NULL);
	druid_page_finish->_priv->title_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_finish->_priv->canvas)),
				       gnome_canvas_text_get_type (), 
				       "text", druid_page_finish->title,
				       "font", _("-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-*-*"),
				       "fontset", _("-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-*-*,*-r-*"),
				       NULL);
	druid_page_finish->_priv->text_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_finish->_priv->canvas)),
				       gnome_canvas_text_get_type (),
				       "text", druid_page_finish->text,
				       "justification", GTK_JUSTIFY_LEFT,
				       "font", _("-adobe-helvetica-medium-r-normal-*-*-120-*-*-p-*-*-*"),
				       "fontset", _("-adobe-helvetica-medium-r-normal-*-*-120-*-*-p-*-*-*,*-r-*"),
				       NULL);

	gnome_druid_page_finish_configure_size (druid_page_finish, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_signal_connect (GTK_OBJECT (druid_page_finish),
			    "prepare",
			    gnome_druid_page_finish_prepare,
			    NULL);
}
static void
gnome_druid_page_finish_prepare (GnomeDruidPage *page,
				GtkWidget *druid,
				gpointer *data)
{
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), TRUE, FALSE, TRUE);
	gnome_druid_set_show_finish (GNOME_DRUID (druid), TRUE);
	gtk_widget_grab_default (GNOME_DRUID (druid)->finish);
}


static void
gnome_druid_page_finish_size_allocate   (GtkWidget               *widget,
					GtkAllocation           *allocation)
{
	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (GNOME_DRUID_PAGE_FINISH (widget)->_priv->canvas),
					0.0, 0.0,
					allocation->width,
					allocation->height);
	gnome_druid_page_finish_configure_size (GNOME_DRUID_PAGE_FINISH (widget),
					       allocation->width,
					       allocation->height);
}
static void
gnome_druid_page_finish_realize (GtkWidget *widget)
{
	GnomeDruidPageFinish *druid_page_finish;
	GdkColormap *cmap = gdk_rgb_get_cmap ();

	druid_page_finish = GNOME_DRUID_PAGE_FINISH (widget);
	gdk_colormap_alloc_color (cmap, &druid_page_finish->background_color, FALSE, TRUE);
	gdk_colormap_alloc_color (cmap, &druid_page_finish->textbox_color, FALSE, TRUE);
	gdk_colormap_alloc_color (cmap, &druid_page_finish->logo_background_color, FALSE, TRUE);
	gdk_colormap_alloc_color (cmap, &druid_page_finish->title_color, FALSE, TRUE);
	gdk_colormap_alloc_color (cmap, &druid_page_finish->text_color, FALSE, TRUE);

	gnome_canvas_item_set (druid_page_finish->_priv->background_item,
			       "fill_color_gdk", &(druid_page_finish->background_color),
			       NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->textbox_item,
			       "fill_color_gdk", &(druid_page_finish->textbox_color),
			       "outline_color_gdk", &(druid_page_finish->background_color),
			       NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->logoframe_item,
			       "fill_color_gdk", &druid_page_finish->logo_background_color,
			       NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->title_item,
			       "fill_color_gdk", &druid_page_finish->title_color,
			       NULL);
	gnome_canvas_item_set (druid_page_finish->_priv->text_item,
			       "fill_color_gdk", &druid_page_finish->text_color,
			       NULL);
	GTK_WIDGET_CLASS (parent_class)->realize (widget);
}

/* Public functions */
GtkWidget *
gnome_druid_page_finish_new (void)
{
	GtkWidget *retval = GTK_WIDGET (gtk_type_new (gnome_druid_page_finish_get_type ()));
	GNOME_DRUID_PAGE_FINISH (retval)->title = g_strdup ("");
	GNOME_DRUID_PAGE_FINISH (retval)->text = g_strdup ("");
	GNOME_DRUID_PAGE_FINISH (retval)->logo_image = NULL;
	GNOME_DRUID_PAGE_FINISH (retval)->watermark_image = NULL;
	gnome_druid_page_finish_construct (GNOME_DRUID_PAGE_FINISH (retval));
	return retval;
}
GtkWidget *
gnome_druid_page_finish_new_with_vals (const gchar *title, const gchar* text, GdkPixbuf *logo, GdkPixbuf *watermark)
{
	GtkWidget *retval = GTK_WIDGET (gtk_type_new (gnome_druid_page_finish_get_type ()));
	GNOME_DRUID_PAGE_FINISH (retval)->title = g_strdup (title);
	GNOME_DRUID_PAGE_FINISH (retval)->text = g_strdup (text);
	GNOME_DRUID_PAGE_FINISH (retval)->logo_image = logo;
	if (logo)
		gdk_pixbuf_ref (logo);
	GNOME_DRUID_PAGE_FINISH (retval)->watermark_image = watermark;
	gnome_druid_page_finish_construct (GNOME_DRUID_PAGE_FINISH (retval));
	return retval;
}
void
gnome_druid_page_finish_set_bg_color      (GnomeDruidPageFinish *druid_page_finish,
					  GdkColor *color)
{
	g_return_if_fail (druid_page_finish != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_FINISH (druid_page_finish));

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();

		gdk_colormap_free_colors (cmap, &druid_page_finish->background_color, 1);
	}
	druid_page_finish->background_color.red = color->red;
	druid_page_finish->background_color.green = color->green;
	druid_page_finish->background_color.blue = color->blue;

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();
		gdk_colormap_alloc_color (cmap, &druid_page_finish->background_color, FALSE, TRUE);
		gnome_canvas_item_set (druid_page_finish->_priv->textbox_item,
				       "outline_color_gdk", &druid_page_finish->background_color,
				       NULL);
		gnome_canvas_item_set (druid_page_finish->_priv->background_item,
				       "fill_color_gdk", &druid_page_finish->background_color,
				       NULL);
	}
}
void
gnome_druid_page_finish_set_textbox_color (GnomeDruidPageFinish *druid_page_finish,
					  GdkColor *color)
{
	g_return_if_fail (druid_page_finish != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_FINISH (druid_page_finish));

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();

		gdk_colormap_free_colors (cmap, &druid_page_finish->textbox_color, 1);
	}
	druid_page_finish->textbox_color.red = color->red;
	druid_page_finish->textbox_color.green = color->green;
	druid_page_finish->textbox_color.blue = color->blue;

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();
		gdk_colormap_alloc_color (cmap, &druid_page_finish->textbox_color, FALSE, TRUE);
		gnome_canvas_item_set (druid_page_finish->_priv->textbox_item,
				       "fill_color_gdk", &druid_page_finish->textbox_color,
				       NULL);
	}
}
void
gnome_druid_page_finish_set_logo_bg_color (GnomeDruidPageFinish *druid_page_finish,
					  GdkColor *color)
{
	g_return_if_fail (druid_page_finish != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_FINISH (druid_page_finish));

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();

		gdk_colormap_free_colors (cmap, &druid_page_finish->logo_background_color, 1);
	}
	druid_page_finish->logo_background_color.red = color->red;
	druid_page_finish->logo_background_color.green = color->green;
	druid_page_finish->logo_background_color.blue = color->blue;

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();
		gdk_colormap_alloc_color (cmap, &druid_page_finish->logo_background_color, FALSE, TRUE);
		gnome_canvas_item_set (druid_page_finish->_priv->logoframe_item,
				       "fill_color_gdk", &druid_page_finish->logo_background_color,
				       NULL);
	}
}
void
gnome_druid_page_finish_set_title_color   (GnomeDruidPageFinish *druid_page_finish,
					  GdkColor *color)
{
	g_return_if_fail (druid_page_finish != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_FINISH (druid_page_finish));

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();

		gdk_colormap_free_colors (cmap, &druid_page_finish->title_color, 1);
	}
	druid_page_finish->title_color.red = color->red;
	druid_page_finish->title_color.green = color->green;
	druid_page_finish->title_color.blue = color->blue;

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();
		gdk_colormap_alloc_color (cmap, &druid_page_finish->title_color, FALSE, TRUE);
		gnome_canvas_item_set (druid_page_finish->_priv->title_item,
				       "fill_color_gdk", &druid_page_finish->title_color,
				       NULL);
	}

}
void
gnome_druid_page_finish_set_text_color    (GnomeDruidPageFinish *druid_page_finish,
					  GdkColor *color)
{
	g_return_if_fail (druid_page_finish != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_FINISH (druid_page_finish));

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();

		gdk_colormap_free_colors (cmap, &druid_page_finish->text_color, 1);
	}
	druid_page_finish->text_color.red = color->red;
	druid_page_finish->text_color.green = color->green;
	druid_page_finish->text_color.blue = color->blue;

	if (GTK_WIDGET_REALIZED (druid_page_finish)) {
		GdkColormap *cmap = gdk_rgb_get_cmap ();
		gdk_colormap_alloc_color (cmap, &druid_page_finish->text_color, FALSE, TRUE);
		gnome_canvas_item_set (druid_page_finish->_priv->text_item,
				       "fill_color_gdk", &druid_page_finish->text_color,
				       NULL);
	}

}
void
gnome_druid_page_finish_set_text          (GnomeDruidPageFinish *druid_page_finish,
					  const gchar *text)
{
	g_return_if_fail (druid_page_finish != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_FINISH (druid_page_finish));

	g_free (druid_page_finish->text);
	druid_page_finish->text = g_strdup (text);
	gnome_canvas_item_set (druid_page_finish->_priv->text_item,
			       "text", druid_page_finish->text,
			       NULL);
}
void
gnome_druid_page_finish_set_title         (GnomeDruidPageFinish *druid_page_finish,
					  const gchar *title)
{
	g_return_if_fail (druid_page_finish != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_FINISH (druid_page_finish));

	g_free (druid_page_finish->title);
	druid_page_finish->title = g_strdup (title);
	gnome_canvas_item_set (druid_page_finish->_priv->title_item,
			       "text", druid_page_finish->title,
			       NULL);
}
void
gnome_druid_page_finish_set_logo          (GnomeDruidPageFinish *druid_page_finish,
					  GdkPixbuf *logo_image)
{
	g_return_if_fail (druid_page_finish != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_FINISH (druid_page_finish));

	druid_page_finish->logo_image = logo_image;
	if (logo_image)
		gdk_pixbuf_ref(logo_image);
	gnome_canvas_item_set (druid_page_finish->_priv->logo_item,
			       "pixbuf", druid_page_finish->logo_image,
			       NULL);
}
void
gnome_druid_page_finish_set_watermark     (GnomeDruidPageFinish *druid_page_finish,
					  GdkPixbuf *watermark)
{
	g_return_if_fail (druid_page_finish != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_FINISH (druid_page_finish));

	druid_page_finish->watermark_image = watermark;
	if (watermark)
		gdk_pixbuf_ref(watermark);
	gnome_canvas_item_set (druid_page_finish->_priv->watermark_item,
			       "pixbuf", druid_page_finish->watermark_image,
			       NULL);
}
