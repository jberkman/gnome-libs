/* gnome-druid-page-start.c
 * Copyright (C) 1999  Red Hat, Inc.
 * All rights reserved.
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
#include "gnome-druid-page-start.h"
#include "libgnomeui/gnome-canvas-pixbuf.h"
#include "libgnomeui/gnome-druid.h"

struct _GnomeDruidPageStartPrivate
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

static void gnome_druid_page_start_init 	 (GnomeDruidPageStart		  *druid_page_start);
static void gnome_druid_page_start_class_init	 (GnomeDruidPageStartClass	  *klass);
static void gnome_druid_page_start_destroy 	 (GtkObject                       *object);
static void gnome_druid_page_start_finalize 	 (GObject                         *object);
static void gnome_druid_page_start_construct     (GnomeDruidPageStart             *druid_page_start);
static void gnome_druid_page_start_configure_size(GnomeDruidPageStart             *druid_page_start,
						  gint                             width,
						  gint                             height);
static void gnome_druid_page_start_size_allocate (GtkWidget                       *widget,
						  GtkAllocation                   *allocation);
static void gnome_druid_page_start_prepare	 (GnomeDruidPage		  *page,
						  GtkWidget                       *druid,
						  gpointer 			  *data);
static GnomeDruidPageClass *parent_class = NULL;

#define LOGO_WIDTH 50.0
#define DRUID_PAGE_HEIGHT 318
#define DRUID_PAGE_WIDTH 516
#define DRUID_PAGE_LEFT_WIDTH 100.0
#define GDK_COLOR_TO_RGBA(color) GNOME_CANVAS_COLOR (color.red/256, color.green/256, color.blue/256)


GtkType
gnome_druid_page_start_get_type (void)
{
  static GtkType druid_page_start_type = 0;

  if (!druid_page_start_type)
    {
      static const GtkTypeInfo druid_page_start_info =
      {
        "GnomeDruidPageStart",
        sizeof (GnomeDruidPageStart),
        sizeof (GnomeDruidPageStartClass),
        (GtkClassInitFunc) gnome_druid_page_start_class_init,
        (GtkObjectInitFunc) gnome_druid_page_start_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      druid_page_start_type = gtk_type_unique (gnome_druid_page_get_type (), &druid_page_start_info);
    }

  return druid_page_start_type;
}

static void
gnome_druid_page_start_class_init (GnomeDruidPageStartClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass*) klass;
	gobject_class = (GObjectClass*) klass;
	object_class->destroy = gnome_druid_page_start_destroy;
	gobject_class->finalize = gnome_druid_page_start_finalize;
	widget_class = (GtkWidgetClass*) klass;
	widget_class->size_allocate = gnome_druid_page_start_size_allocate;
	parent_class = gtk_type_class (gnome_druid_page_get_type ());
}

static void
gnome_druid_page_start_init (GnomeDruidPageStart *druid_page_start)
{
	druid_page_start->_priv = g_new0(GnomeDruidPageStartPrivate, 1);

	/* initialize the color values */
	druid_page_start->background_color.red = 6400; /* midnight blue */
	druid_page_start->background_color.green = 6400;
	druid_page_start->background_color.blue = 28672;
	druid_page_start->textbox_color.red = 65280; /* white */
	druid_page_start->textbox_color.green = 65280;
	druid_page_start->textbox_color.blue = 65280;
	druid_page_start->logo_background_color.red = 65280; /* white */
	druid_page_start->logo_background_color.green = 65280;
	druid_page_start->logo_background_color.blue = 65280;
	druid_page_start->title_color.red = 65280; /* white */
	druid_page_start->title_color.green = 65280;
	druid_page_start->title_color.blue = 65280;
	druid_page_start->text_color.red = 0; /* black */
	druid_page_start->text_color.green = 0;
	druid_page_start->text_color.blue = 0;

	/* Set up the canvas */
	gtk_container_set_border_width (GTK_CONTAINER (druid_page_start), 0);
	druid_page_start->_priv->canvas = gnome_canvas_new ();
	gtk_widget_set_usize (druid_page_start->_priv->canvas, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_widget_show (druid_page_start->_priv->canvas);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (druid_page_start->_priv->canvas), 0.0, 0.0, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_container_add (GTK_CONTAINER (druid_page_start), druid_page_start->_priv->canvas);

}

static void
gnome_druid_page_start_destroy(GtkObject *object)
{
	/* remember, destroy can be run multiple times! */
	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void
gnome_druid_page_start_finalize(GObject *object)
{
	GnomeDruidPageStart *druid_page_start = GNOME_DRUID_PAGE_START(object);

	g_free(druid_page_start->_priv);
	druid_page_start->_priv = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(object);
}

static void
gnome_druid_page_start_configure_size (GnomeDruidPageStart *druid_page_start, gint width, gint height)
{
	gfloat watermark_width = DRUID_PAGE_LEFT_WIDTH;
	gfloat watermark_height = (gfloat) height - LOGO_WIDTH + GNOME_PAD * 2.0;
	gfloat watermark_ypos = LOGO_WIDTH + GNOME_PAD * 2.0;

	if (druid_page_start->watermark_image) {
		watermark_width = gdk_pixbuf_get_width (druid_page_start->watermark_image);
		watermark_height = gdk_pixbuf_get_height (druid_page_start->watermark_image);
		watermark_ypos = (gfloat) height - watermark_height;
		if (watermark_width < 1)
			watermark_width = 1.0;
		if (watermark_height < 1)
			watermark_height = 1.0;
	}

	gnome_canvas_item_set (druid_page_start->_priv->background_item,
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (gfloat) width,
			       "y2", (gfloat) height,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_start->_priv->textbox_item,
			       "x1", watermark_width,
			       "y1", LOGO_WIDTH + GNOME_PAD * 2.0,
			       "x2", (gfloat) width,
			       "y2", (gfloat) height,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_start->_priv->logoframe_item,
			       "x1", (gfloat) width - LOGO_WIDTH -GNOME_PAD,
			       "y1", (gfloat) GNOME_PAD,
			       "x2", (gfloat) width - GNOME_PAD,
			       "y2", (gfloat) GNOME_PAD + LOGO_WIDTH,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_start->_priv->logo_item,
			       "x", (gfloat) width - GNOME_PAD - LOGO_WIDTH,
			       "y", (gfloat) GNOME_PAD,
			       "width", (gfloat) LOGO_WIDTH,
			       "height", (gfloat) LOGO_WIDTH, NULL);
	gnome_canvas_item_set (druid_page_start->_priv->watermark_item,
			       "x", 0.0,
			       "y", watermark_ypos,
			       "width", watermark_width,
			       "height", watermark_height,
			       NULL);
	gnome_canvas_item_set (druid_page_start->_priv->title_item,
			       "x", 15.0,
			       "y", (gfloat) GNOME_PAD + LOGO_WIDTH / 2.0,
			       "anchor", GTK_ANCHOR_WEST,
			       NULL);
	gnome_canvas_item_set (druid_page_start->_priv->text_item,
			       "x", ((width - watermark_width) * 0.5) + watermark_width,
			       "y", LOGO_WIDTH + GNOME_PAD * 2.0 + (height - (LOGO_WIDTH + GNOME_PAD * 2.0))/ 2.0,
			       "anchor", GTK_ANCHOR_CENTER,
			       NULL);
}

static void
gnome_druid_page_start_construct (GnomeDruidPageStart *druid_page_start)
{
	guint32 fill_color, outline_color;

	/* set up the rest of the page */
	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->background_color);
	druid_page_start->_priv->background_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_start->_priv->canvas)),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       NULL);

	outline_color = fill_color;
	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->textbox_color);
	druid_page_start->_priv->textbox_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_start->_priv->canvas)),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       "outline_color_rgba", outline_color,
				        NULL);

	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->logo_background_color);
	druid_page_start->_priv->logoframe_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_start->_priv->canvas)),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       NULL);

	druid_page_start->_priv->logo_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_start->_priv->canvas)),
				       gnome_canvas_pixbuf_get_type (),
				       "x_set", TRUE, "y_set", TRUE,
				       NULL);

	if (druid_page_start->logo_image != NULL)
		gnome_canvas_item_set (druid_page_start->_priv->logo_item,
				       "pixbuf", druid_page_start->logo_image, NULL);

	druid_page_start->_priv->watermark_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_start->_priv->canvas)),
				       gnome_canvas_pixbuf_get_type (),
				       "x_set", TRUE, "y_set", TRUE,
				       NULL);

	if (druid_page_start->watermark_image != NULL)
		gnome_canvas_item_set (druid_page_start->_priv->watermark_item,
				       "pixbuf", druid_page_start->watermark_image, NULL);

	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->title_color);
	druid_page_start->_priv->title_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_start->_priv->canvas)),
				       gnome_canvas_text_get_type (),
				       "text", druid_page_start->title,
				       "fill_color_rgba", fill_color,
				       "fontset", _("-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-*-*,*-r-*"),
				       NULL);

	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->text_color);
	druid_page_start->_priv->text_item =
		gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (druid_page_start->_priv->canvas)),
				       gnome_canvas_text_get_type (),
				       "text", druid_page_start->text,
				       "justification", GTK_JUSTIFY_LEFT,
				       "fontset", _("-adobe-helvetica-medium-r-normal-*-*-120-*-*-p-*-*-*,*-r-*"),
				       "fill_color_rgba", fill_color,
				       NULL);

	gnome_druid_page_start_configure_size (druid_page_start, DRUID_PAGE_WIDTH, DRUID_PAGE_HEIGHT);
	gtk_signal_connect (GTK_OBJECT (druid_page_start),
			    "prepare",
			    gnome_druid_page_start_prepare,
			    NULL);
}
static void
gnome_druid_page_start_prepare (GnomeDruidPage *page,
				GtkWidget *druid,
				gpointer *data)
{
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), FALSE, TRUE, TRUE);
	gnome_druid_set_show_finish (GNOME_DRUID (druid), FALSE);
	gtk_widget_grab_default (GNOME_DRUID (druid)->next);
}


static void
gnome_druid_page_start_size_allocate   (GtkWidget               *widget,
					GtkAllocation           *allocation)
{
	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
	gnome_canvas_set_scroll_region (GNOME_CANVAS (GNOME_DRUID_PAGE_START (widget)->_priv->canvas),
					0.0, 0.0,
					allocation->width,
					allocation->height);
	gnome_druid_page_start_configure_size (GNOME_DRUID_PAGE_START (widget),
					       allocation->width,
					       allocation->height);
}

#if 0
/* Fragment left behind by CVS.  Don't get it. */
	gnome_canvas_item_set (druid_page_start->_priv->background_item,
			       "fill_color_gdk", &(druid_page_start->background_color),
			       NULL);
	gnome_canvas_item_set (druid_page_start->_priv->textbox_item,
			       "fill_color_gdk", &(druid_page_start->textbox_color),
			       "outline_color_gdk", &(druid_page_start->background_color),
			       NULL);
	gnome_canvas_item_set (druid_page_start->_priv->logoframe_item,
			       "fill_color_gdk", &druid_page_start->logo_background_color,
			       NULL);
	gnome_canvas_item_set (druid_page_start->_priv->title_item,
			       "fill_color_gdk", &druid_page_start->title_color,
			       NULL);
	gnome_canvas_item_set (druid_page_start->_priv->text_item,
			       "fill_color_gdk", &druid_page_start->text_color,
			       NULL);
	GTK_WIDGET_CLASS (parent_class)->realize (widget);
}
#endif
/**
 * gnome_druid_page_start_new:
 *
 * Creates a new GnomeDruidPageStart widget.
 *
 * Return value: Pointer to new GnomeDruidPageStart
 **/
/* Public functions */
GtkWidget *
gnome_druid_page_start_new (void)
{
	GtkWidget *retval =  GTK_WIDGET (gtk_type_new (gnome_druid_page_start_get_type ()));
	GNOME_DRUID_PAGE_START (retval)->title = g_strdup ("");
	GNOME_DRUID_PAGE_START (retval)->text = g_strdup ("");
	GNOME_DRUID_PAGE_START (retval)->logo_image = NULL;
	GNOME_DRUID_PAGE_START (retval)->watermark_image = NULL;
	gnome_druid_page_start_construct (GNOME_DRUID_PAGE_START (retval));
	return retval;
}
/**
 * gnome_druid_page_start_new_with_vals:
 * @title: The title.
 * @text: The introduction text.
 * @logo: The logo in the upper right corner.
 * @watermark: The watermark on the left.
 *
 * This will create a new GNOME Druid start page, with the values
 * given.  It is acceptable for any of them to be %NULL.
 *
 * Return value: GtkWidget pointer to new GnomeDruidPageStart.
 **/
GtkWidget *
gnome_druid_page_start_new_with_vals (const gchar *title, const gchar* text,
				      GdkPixbuf *logo, GdkPixbuf *watermark)
{
	GtkWidget *retval =  GTK_WIDGET (gtk_type_new (gnome_druid_page_start_get_type ()));
	GNOME_DRUID_PAGE_START (retval)->title = g_strdup (title);
	GNOME_DRUID_PAGE_START (retval)->text = g_strdup (text);
	GNOME_DRUID_PAGE_START (retval)->logo_image = logo;
	GNOME_DRUID_PAGE_START (retval)->watermark_image = watermark;
	gnome_druid_page_start_construct (GNOME_DRUID_PAGE_START (retval));
	return retval;
}

/**
 * gnome_druid_page_start_set_bg_color:
 * @druid_page_start: A DruidPageStart.
 * @color: The new background color.
 *
 * This will set the background color to be the @color.  You do not
 * need to allocate the color, as the @druid_page_start will do it for
 * you.
 **/
void
gnome_druid_page_start_set_bg_color      (GnomeDruidPageStart *druid_page_start,
					  GdkColor *color)
{
	guint fill_color;

	g_return_if_fail (druid_page_start != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_START (druid_page_start));

	druid_page_start->background_color.red = color->red;
	druid_page_start->background_color.green = color->green;
	druid_page_start->background_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->background_color);

	gnome_canvas_item_set (druid_page_start->_priv->textbox_item,
			       "outline_color_rgba", fill_color,
			       NULL);
	gnome_canvas_item_set (druid_page_start->_priv->background_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}

void
gnome_druid_page_start_set_textbox_color (GnomeDruidPageStart *druid_page_start,
					  GdkColor *color)
{
	guint fill_color;

	g_return_if_fail (druid_page_start != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_START (druid_page_start));

	druid_page_start->textbox_color.red = color->red;
	druid_page_start->textbox_color.green = color->green;
	druid_page_start->textbox_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->textbox_color);
	gnome_canvas_item_set (druid_page_start->_priv->textbox_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}

void
gnome_druid_page_start_set_logo_bg_color (GnomeDruidPageStart *druid_page_start,
					  GdkColor *color)
{
	guint fill_color;

	g_return_if_fail (druid_page_start != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_START (druid_page_start));

	druid_page_start->logo_background_color.red = color->red;
	druid_page_start->logo_background_color.green = color->green;
	druid_page_start->logo_background_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->logo_background_color);
	gnome_canvas_item_set (druid_page_start->_priv->logoframe_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}
void
gnome_druid_page_start_set_title_color   (GnomeDruidPageStart *druid_page_start,
					  GdkColor *color)
{
	guint32 fill_color;

	g_return_if_fail (druid_page_start != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_START (druid_page_start));

	druid_page_start->title_color.red = color->red;
	druid_page_start->title_color.green = color->green;
	druid_page_start->title_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->title_color);
	gnome_canvas_item_set (druid_page_start->_priv->title_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}
void
gnome_druid_page_start_set_text_color    (GnomeDruidPageStart *druid_page_start,
					  GdkColor *color)
{
	guint32 fill_color;

	g_return_if_fail (druid_page_start != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_START (druid_page_start));

	druid_page_start->text_color.red = color->red;
	druid_page_start->text_color.green = color->green;
	druid_page_start->text_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_start->text_color);
	gnome_canvas_item_set (druid_page_start->_priv->text_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}
void
gnome_druid_page_start_set_text          (GnomeDruidPageStart *druid_page_start,
					  const gchar *text)
{
	g_return_if_fail (druid_page_start != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_START (druid_page_start));

	g_free (druid_page_start->text);
	druid_page_start->text = g_strdup (text);
	gnome_canvas_item_set (druid_page_start->_priv->text_item,
			       "text", druid_page_start->text,
			       NULL);
}
void
gnome_druid_page_start_set_title         (GnomeDruidPageStart *druid_page_start,
					  const gchar *title)
{
	g_return_if_fail (druid_page_start != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_START (druid_page_start));

	g_free (druid_page_start->title);
	druid_page_start->title = g_strdup (title);
	gnome_canvas_item_set (druid_page_start->_priv->title_item,
			       "text", druid_page_start->title,
			       NULL);
}
void
gnome_druid_page_start_set_logo          (GnomeDruidPageStart *druid_page_start,
					  GdkPixbuf *logo_image)
{
	g_return_if_fail (druid_page_start != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_START (druid_page_start));

	if (druid_page_start->logo_image)
		gdk_pixbuf_unref (druid_page_start->logo_image);

	druid_page_start->logo_image = logo_image;
	gdk_pixbuf_ref (logo_image);
	gnome_canvas_item_set (druid_page_start->_priv->logo_item,
			       "pixbuf", druid_page_start->logo_image, NULL);
}
void
gnome_druid_page_start_set_watermark     (GnomeDruidPageStart *druid_page_start,
					  GdkPixbuf *watermark)
{
	g_return_if_fail (druid_page_start != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_START (druid_page_start));

	if (druid_page_start->watermark_image)
		gdk_pixbuf_unref (druid_page_start->watermark_image);

	druid_page_start->watermark_image = watermark;
	gdk_pixbuf_ref (watermark);
	gnome_canvas_item_set (druid_page_start->_priv->watermark_item,
			       "pixbuf", druid_page_start->watermark_image, NULL);
}
