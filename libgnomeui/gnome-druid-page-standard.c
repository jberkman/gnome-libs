/* gnome-druid-page-standard.c
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

#include <gnome.h>
#include "gnome-druid.h"
#include "gnome-druid-page-standard.h"
#include "gnome-canvas-pixbuf.h"

struct _GnomeDruidPageStandardPrivate
{
	GtkWidget *canvas;
	GtkWidget *side_bar;
	GnomeCanvasItem *logoframe_item;
	GnomeCanvasItem *logo_item;
	GnomeCanvasItem *title_item;
	GnomeCanvasItem *background_item;
	GtkWidget *bottom_bar;
	GtkWidget *right_bar;
};


static void gnome_druid_page_standard_init	    (GnomeDruidPageStandard          *druid_page_standard);
static void gnome_druid_page_standard_class_init    (GnomeDruidPageStandardClass     *klass);
static void gnome_druid_page_standard_destroy 	    (GtkObject                       *object);
static void gnome_druid_page_standard_setup         (GnomeDruidPageStandard          *druid_page_standard);
static void gnome_druid_page_standard_finalize      (GObject                         *widget);
static void gnome_druid_page_standard_size_allocate (GtkWidget                       *widget,
						     GtkAllocation                   *allocation);
static void gnome_druid_page_standard_prepare       (GnomeDruidPage                  *page,
						     GtkWidget                       *druid,
						     gpointer                        *data);

static void gnome_druid_page_standard_set_sidebar_shown (GnomeDruidPage              *druid_page,
						     gboolean			      sidebar_shown);
static void gnome_druid_page_standard_configure_canvas (GnomeDruidPage		     *druid_page);

static GnomeDruidPageClass *parent_class = NULL;

#define LOGO_WIDTH 50.0
#define DRUID_PAGE_WIDTH 516
#define GDK_COLOR_TO_RGBA(color) GNOME_CANVAS_COLOR ((color).red/256, (color).green/256, (color).blue/256)

GtkType
gnome_druid_page_standard_get_type (void)
{
	static GtkType druid_page_standard_type = 0;

	if (druid_page_standard_type == 0) {
		static const GtkTypeInfo druid_page_standard_info = {
			"GnomeDruidPageStandard",
			sizeof (GnomeDruidPageStandard),
			sizeof (GnomeDruidPageStandardClass),
			(GtkClassInitFunc) gnome_druid_page_standard_class_init,
			(GtkObjectInitFunc) gnome_druid_page_standard_init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL
		};

		druid_page_standard_type =
			gtk_type_unique (gnome_druid_page_get_type (),
					 &druid_page_standard_info);
	}

	return druid_page_standard_type;
}

static void
gnome_druid_page_standard_class_init (GnomeDruidPageStandardClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GnomeDruidPageClass *page_class;

	object_class = (GtkObjectClass*) klass;
	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;
	page_class = (GnomeDruidPageClass*) klass;

	parent_class = gtk_type_class (gnome_druid_page_get_type ());

	page_class->configure_canvas = gnome_druid_page_standard_configure_canvas;
	page_class->set_sidebar_shown = gnome_druid_page_standard_set_sidebar_shown;

	object_class->destroy = gnome_druid_page_standard_destroy;
	gobject_class->finalize = gnome_druid_page_standard_finalize;
	widget_class->size_allocate = gnome_druid_page_standard_size_allocate;

}
static void
gnome_druid_page_standard_init (GnomeDruidPageStandard *druid_page_standard)
{
	druid_page_standard->_priv = g_new0(GnomeDruidPageStandardPrivate, 1);

	/* initialize the color values */
	druid_page_standard->background_color.red = 6400; /* midnight blue */
	druid_page_standard->background_color.green = 6400;
	druid_page_standard->background_color.blue = 28672;
	druid_page_standard->logo_background_color.red = 65280; /* white */
	druid_page_standard->logo_background_color.green = 65280;
	druid_page_standard->logo_background_color.blue = 65280;
	druid_page_standard->title_color.red = 65280; /* white */
	druid_page_standard->title_color.green = 65280;
	druid_page_standard->title_color.blue = 65280;
}

void
gnome_druid_page_standard_construct (GnomeDruidPageStandard *druid_page_standard,
				     gboolean		     antialiased,
				     const gchar	    *title,
				     GdkPixbuf		    *logo)
{
	GnomeCanvas *canvas;
	GtkRcStyle *rc_style;
	GtkWidget *vbox;
	GtkWidget *hbox;
	gboolean sidebar_shown;

	gnome_druid_page_construct (GNOME_DRUID_PAGE (druid_page_standard), antialiased);

	druid_page_standard->title = g_strdup (title ? title : "");

	if (logo != NULL)
		gdk_pixbuf_ref (logo);
	druid_page_standard->logo_image = logo;

	canvas = gnome_druid_page_get_canvas (GNOME_DRUID_PAGE (druid_page_standard));

	/* Set up the widgets */
	vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	druid_page_standard->vbox = gtk_vbox_new (FALSE, 0);
	druid_page_standard->_priv->side_bar = gtk_drawing_area_new ();
	druid_page_standard->_priv->bottom_bar = gtk_drawing_area_new ();
	druid_page_standard->_priv->right_bar = gtk_drawing_area_new ();
	gtk_drawing_area_size (GTK_DRAWING_AREA (druid_page_standard->_priv->side_bar),
			       15, 10);
	gtk_drawing_area_size (GTK_DRAWING_AREA (druid_page_standard->_priv->bottom_bar),
			       10, 1);
	gtk_drawing_area_size (GTK_DRAWING_AREA (druid_page_standard->_priv->right_bar),
			       1, 10);
	rc_style = gtk_rc_style_new ();
	rc_style->bg[GTK_STATE_NORMAL].red = 6400;
	rc_style->bg[GTK_STATE_NORMAL].green = 6400;
	rc_style->bg[GTK_STATE_NORMAL].blue = 28672;
	rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
	gtk_rc_style_ref (rc_style);
	gtk_widget_modify_style (druid_page_standard->_priv->side_bar, rc_style);
	gtk_rc_style_ref (rc_style);
	gtk_widget_modify_style (druid_page_standard->_priv->bottom_bar, rc_style);
	gtk_rc_style_ref (rc_style);
	gtk_widget_modify_style (druid_page_standard->_priv->right_bar, rc_style);

	/* FIXME: can I just ref the old style? */
	rc_style = gtk_rc_style_new ();
	rc_style->bg[GTK_STATE_NORMAL].red = 6400;
	rc_style->bg[GTK_STATE_NORMAL].green = 6400;
	rc_style->bg[GTK_STATE_NORMAL].blue = 28672;
	rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
	gtk_widget_modify_style (GTK_WIDGET (canvas), rc_style);
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (canvas), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), druid_page_standard->_priv->bottom_bar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), druid_page_standard->_priv->side_bar, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), druid_page_standard->vbox, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), druid_page_standard->_priv->right_bar, FALSE, FALSE, 0);
	gtk_widget_set_usize (GTK_WIDGET (canvas), 508, LOGO_WIDTH + GNOME_PAD * 2);
	gtk_container_set_border_width (GTK_CONTAINER (druid_page_standard), 0);
	gtk_container_add (GTK_CONTAINER (druid_page_standard), vbox);
	gtk_widget_show_all (vbox);

	sidebar_shown = gnome_druid_page_get_sidebar_shown (GNOME_DRUID_PAGE (druid_page_standard));
	if ( ! sidebar_shown)
		gtk_widget_hide (druid_page_standard->_priv->side_bar);

	gnome_druid_page_standard_setup (druid_page_standard);
}

static void
gnome_druid_page_standard_destroy(GtkObject *object)
{
	GnomeDruidPageStandard *druid_page_standard = GNOME_DRUID_PAGE_STANDARD(object);

	/* remember, destroy can be run multiple times! */

	if (druid_page_standard->logo_image != NULL)
		gdk_pixbuf_unref (druid_page_standard->logo_image);
	druid_page_standard->logo_image = NULL;

	g_free (druid_page_standard->title);
	druid_page_standard->title = NULL;

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(object);
}

static void
gnome_druid_page_standard_finalize (GObject *object)
{
	GnomeDruidPageStandard *druid_page_standard = GNOME_DRUID_PAGE_STANDARD(object);

	g_free(druid_page_standard->_priv);
	druid_page_standard->_priv = NULL;

	if (G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(object);
}


static void
gnome_druid_page_standard_configure_canvas (GnomeDruidPage *druid_page)
{
	GnomeDruidPageStandard *druid_page_standard;
	int width, height;

	g_return_if_fail (druid_page != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page));

	druid_page_standard = GNOME_DRUID_PAGE_STANDARD (druid_page);

	width = GTK_WIDGET(druid_page)->allocation.width;
	height = GTK_WIDGET(druid_page)->allocation.height;

	gnome_canvas_item_set (druid_page_standard->_priv->background_item,
			       "x1", 0.0,
			       "y1", 0.0,
			       "x2", (gfloat) width,
			       "y2", (gfloat) LOGO_WIDTH + GNOME_PAD * 2,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_standard->_priv->logoframe_item,
			       "x1", (gfloat) width - LOGO_WIDTH - GNOME_PAD,
			       "y1", (gfloat) GNOME_PAD,
			       "x2", (gfloat) width - GNOME_PAD,
			       "y2", (gfloat) GNOME_PAD + LOGO_WIDTH,
			       "width_units", 1.0, NULL);
	gnome_canvas_item_set (druid_page_standard->_priv->logo_item,
			       "x", (gfloat) width - GNOME_PAD - LOGO_WIDTH,
			       "y", (gfloat) GNOME_PAD,
			       "width", (gfloat) LOGO_WIDTH,
			       "height", (gfloat) LOGO_WIDTH, NULL);
}

static void
gnome_druid_page_standard_setup (GnomeDruidPageStandard *druid_page_standard)
{
	GnomeCanvas *canvas;
	static guint32 fill_color = 0;

	canvas = gnome_druid_page_get_canvas (GNOME_DRUID_PAGE (druid_page_standard));

	/* set up the rest of the page */
	fill_color = GDK_COLOR_TO_RGBA (druid_page_standard->background_color);
	druid_page_standard->_priv->background_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       NULL);

	fill_color = GDK_COLOR_TO_RGBA (druid_page_standard->logo_background_color);
	druid_page_standard->_priv->logoframe_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_rect_get_type (),
				       "fill_color_rgba", fill_color,
				       NULL);

	druid_page_standard->_priv->logo_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_pixbuf_get_type (),
				       "x_set", TRUE, "y_set", TRUE,
				       NULL);

	if (druid_page_standard->logo_image != NULL) {
		gnome_canvas_item_set (druid_page_standard->_priv->logo_item,
				       "pixbuf", druid_page_standard->logo_image, NULL);
	}

	fill_color = GDK_COLOR_TO_RGBA (druid_page_standard->title_color);
	druid_page_standard->_priv->title_item =
		gnome_canvas_item_new (gnome_canvas_root (canvas),
				       gnome_canvas_text_get_type (),
				       "text", druid_page_standard->title,
				       "fontset", _("-adobe-helvetica-bold-r-normal-*-*-180-*-*-p-*-*-*,*-r-*"),
				       "fill_color_rgba", fill_color,
				       NULL);

	gnome_canvas_item_set (druid_page_standard->_priv->title_item,
			       "x", 15.0,
			       "y", (gfloat) GNOME_PAD + LOGO_WIDTH / 2.0,
			       "anchor", GTK_ANCHOR_WEST,
			       NULL);

	gtk_signal_connect (GTK_OBJECT (druid_page_standard),
			    "prepare",
			    gnome_druid_page_standard_prepare,
			    NULL);

}
static void
gnome_druid_page_standard_prepare (GnomeDruidPage *page,
				   GtkWidget *druid,
				   gpointer *data)
{
	gnome_druid_set_buttons_sensitive (GNOME_DRUID (druid), TRUE, TRUE, TRUE);
	gnome_druid_set_show_finish (GNOME_DRUID (druid), FALSE);
	gtk_widget_grab_default (GNOME_DRUID (druid)->next);
}

static void
gnome_druid_page_standard_size_allocate (GtkWidget *widget,
					 GtkAllocation *allocation)
{
	GnomeCanvas *canvas;

	canvas = gnome_druid_page_get_canvas (GNOME_DRUID_PAGE (widget));

	GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

	gnome_canvas_set_scroll_region (canvas,
					0.0, 0.0,
					allocation->width,
					allocation->height);
	gnome_druid_page_configure_canvas (GNOME_DRUID_PAGE (widget));
}

GtkWidget *
gnome_druid_page_standard_new (void)
{
	GnomeDruidPageStandard *retval;

	retval = gtk_type_new (gnome_druid_page_standard_get_type ());

	gnome_druid_page_standard_construct (retval,
					     FALSE,
					     NULL,
					     NULL);

	return GTK_WIDGET (retval);
}

GtkWidget *
gnome_druid_page_standard_new_aa (void)
{
	GnomeDruidPageStandard *retval;

	retval = gtk_type_new (gnome_druid_page_standard_get_type ());

	gnome_druid_page_standard_construct (retval,
					     TRUE,
					     NULL,
					     NULL);

	return GTK_WIDGET (retval);
}

GtkWidget *
gnome_druid_page_standard_new_with_vals (gboolean antialiased, const gchar *title, GdkPixbuf *logo)
{
	GnomeDruidPageStandard *retval;

	retval = gtk_type_new (gnome_druid_page_standard_get_type ());

	gnome_druid_page_standard_construct (retval,
					     antialiased,
					     title,
					     logo);

	return GTK_WIDGET (retval);
}

void
gnome_druid_page_standard_set_bg_color      (GnomeDruidPageStandard *druid_page_standard,
					     GdkColor *color)
{
	guint32 fill_color;

	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->background_color.red = color->red;
	druid_page_standard->background_color.green = color->green;
	druid_page_standard->background_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_standard->background_color);

	gnome_canvas_item_set (druid_page_standard->_priv->background_item,
			       "fill_color_rgba", fill_color,
			       NULL);

	if (GTK_WIDGET_REALIZED (druid_page_standard)) {

		GtkStyle *style;

		style = gtk_style_copy (gtk_widget_get_style (druid_page_standard->_priv->side_bar));
		style->bg[GTK_STATE_NORMAL].red = color->red;
		style->bg[GTK_STATE_NORMAL].green = color->green;
		style->bg[GTK_STATE_NORMAL].blue = color->blue;
		gtk_widget_set_style (druid_page_standard->_priv->side_bar, style);
		gtk_widget_set_style (druid_page_standard->_priv->bottom_bar, style);
		gtk_widget_set_style (druid_page_standard->_priv->right_bar, style);
	} else {
		GtkRcStyle *rc_style;

		rc_style = gtk_rc_style_new ();
		rc_style->bg[GTK_STATE_NORMAL].red = color->red;
		rc_style->bg[GTK_STATE_NORMAL].green = color->green;
		rc_style->bg[GTK_STATE_NORMAL].blue = color->blue;
		rc_style->color_flags[GTK_STATE_NORMAL] = GTK_RC_BG;
		gtk_rc_style_ref (rc_style);
		gtk_widget_modify_style (druid_page_standard->_priv->side_bar, rc_style);
		gtk_rc_style_ref (rc_style);
		gtk_widget_modify_style (druid_page_standard->_priv->bottom_bar, rc_style);
		gtk_rc_style_ref (rc_style);
		gtk_widget_modify_style (druid_page_standard->_priv->right_bar, rc_style);
	}
}

void
gnome_druid_page_standard_set_logo_bg_color (GnomeDruidPageStandard *druid_page_standard,
					     GdkColor *color)
{
	guint32 fill_color;

	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->logo_background_color.red = color->red;
	druid_page_standard->logo_background_color.green = color->green;
	druid_page_standard->logo_background_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_standard->logo_background_color);
	gnome_canvas_item_set (druid_page_standard->_priv->logoframe_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}
void
gnome_druid_page_standard_set_title_color   (GnomeDruidPageStandard *druid_page_standard,
					  GdkColor *color)
{
	guint32 fill_color;

	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));
	g_return_if_fail (color != NULL);

	druid_page_standard->title_color.red = color->red;
	druid_page_standard->title_color.green = color->green;
	druid_page_standard->title_color.blue = color->blue;

	fill_color = GDK_COLOR_TO_RGBA (druid_page_standard->title_color);
	gnome_canvas_item_set (druid_page_standard->_priv->title_item,
			       "fill_color_rgba", fill_color,
			       NULL);
}

void
gnome_druid_page_standard_set_title         (GnomeDruidPageStandard *druid_page_standard,
					     const gchar *title)
{
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	g_free (druid_page_standard->title);
	druid_page_standard->title = g_strdup (title ? title : "");
	gnome_canvas_item_set (druid_page_standard->_priv->title_item,
			       "text", druid_page_standard->title,
			       NULL);
}
void
gnome_druid_page_standard_set_logo          (GnomeDruidPageStandard *druid_page_standard,
					     GdkPixbuf*logo_image)
{
	g_return_if_fail (druid_page_standard != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page_standard));

	if (druid_page_standard->logo_image)
		gdk_pixbuf_unref (druid_page_standard->logo_image);

	druid_page_standard->logo_image = logo_image;
	if (logo_image != NULL)
		gdk_pixbuf_ref (logo_image);
	gnome_canvas_item_set (druid_page_standard->_priv->logo_item,
			       "pixbuf", druid_page_standard->logo_image, NULL);
}


static void
gnome_druid_page_standard_set_sidebar_shown (GnomeDruidPage *druid_page, gboolean sidebar_shown)
{
	GnomeDruidPageStandard *druid_page_standard;

	g_return_if_fail (druid_page != NULL);
	g_return_if_fail (GNOME_IS_DRUID_PAGE_STANDARD (druid_page));

	druid_page_standard = GNOME_DRUID_PAGE_STANDARD (druid_page);

	if(GNOME_DRUID_PAGE_GET_CLASS(druid_page)->set_sidebar_shown)
		GNOME_DRUID_PAGE_CLASS(druid_page)->set_sidebar_shown(druid_page, sidebar_shown);

	if (druid_page_standard->_priv->side_bar) {
		if (sidebar_shown)
			gtk_widget_show (druid_page_standard->_priv->side_bar);
		else
			gtk_widget_hide (druid_page_standard->_priv->side_bar);
	}
}
