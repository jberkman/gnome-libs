/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
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
/* Rectangle and ellipse item types for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <math.h>
#include "gnome-canvas-rect-ellipse.h"
#include "gnome-canvas-util.h"

#include "libart_lgpl/art_vpath.h"
#include "libart_lgpl/art_svp.h"
#include "libart_lgpl/art_svp_vpath.h"
#include "libart_lgpl/art_rgb_svp.h"

/* Base class for rectangle and ellipse item types */

#define noVERBOSE

enum {
	PROP_0,
	PROP_X1,
	PROP_Y1,
	PROP_X2,
	PROP_Y2,
	PROP_FILL_COLOR,
	PROP_FILL_COLOR_GDK,
	PROP_FILL_COLOR_RGBA,
	PROP_OUTLINE_COLOR,
	PROP_OUTLINE_COLOR_GDK,
	PROP_OUTLINE_COLOR_RGBA,
	PROP_FILL_STIPPLE,
	PROP_OUTLINE_STIPPLE,
	PROP_WIDTH_PIXELS,
	PROP_WIDTH_UNITS
};


static void gnome_canvas_re_class_init (GnomeCanvasREClass *class);
static void gnome_canvas_re_init       (GnomeCanvasRE      *re);
static void gnome_canvas_re_destroy    (GtkObject          *object);
static void gnome_canvas_re_set_property (GObject              *object,
					  guint                 param_id,
					  const GValue         *value,
					  GParamSpec           *pspec,
					  const gchar          *trailer);
static void gnome_canvas_re_get_property (GObject              *object,
					  guint                 param_id,
					  GValue               *value,
					  GParamSpec           *pspec,
					  const gchar          *trailer);

static void gnome_canvas_re_update_shared      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void gnome_canvas_re_realize     (GnomeCanvasItem *item);
static void gnome_canvas_re_unrealize   (GnomeCanvasItem *item);
static void gnome_canvas_re_translate   (GnomeCanvasItem *item, double dx, double dy);
static void gnome_canvas_re_bounds      (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);

static void gnome_canvas_re_render      (GnomeCanvasItem *item, GnomeCanvasBuf *buf);

static void gnome_canvas_rect_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void gnome_canvas_ellipse_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);

static GnomeCanvasItemClass *re_parent_class;


GtkType
gnome_canvas_re_get_type (void)
{
	static GtkType re_type = 0;

	if (!re_type) {
		GtkTypeInfo re_info = {
			"GnomeCanvasRE",
			sizeof (GnomeCanvasRE),
			sizeof (GnomeCanvasREClass),
			(GtkClassInitFunc) gnome_canvas_re_class_init,
			(GtkObjectInitFunc) gnome_canvas_re_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		re_type = gtk_type_unique (gnome_canvas_item_get_type (), &re_info);
	}

	return re_type;
}

static void
gnome_canvas_re_class_init (GnomeCanvasREClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	re_parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gobject_class->set_property = gnome_canvas_re_set_property;
	gobject_class->get_property = gnome_canvas_re_get_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_X1,
                 g_param_spec_double ("x1", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y1,
                 g_param_spec_double ("y1", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_X2,
                 g_param_spec_double ("x2", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y2,
                 g_param_spec_double ("y2", NULL, NULL,
				      G_MINDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR,
                 g_param_spec_string ("fill_color", NULL, NULL,
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_GDK,
                 g_param_spec_boxed ("fill_color_gdk", NULL, NULL,
				     GTK_TYPE_GDK_COLOR,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_RGBA,
                 g_param_spec_uint ("fill_color_rgba", NULL, NULL,
				    0, G_MAXUINT, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_STIPPLE,
                 g_param_spec_object ("fill_stipple", NULL, NULL,
                                      GDK_TYPE_DRAWABLE,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_OUTLINE_COLOR,
                 g_param_spec_string ("outline_color", NULL, NULL,
                                      NULL,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_OUTLINE_COLOR_GDK,
                 g_param_spec_boxed ("outline_color_gdk", NULL, NULL,
				     GTK_TYPE_GDK_COLOR,
				     (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_OUTLINE_COLOR_RGBA,
                 g_param_spec_uint ("outline_color_rgba", NULL, NULL,
				    0, G_MAXUINT, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_OUTLINE_STIPPLE,
                 g_param_spec_object ("outline_stipple", NULL, NULL,
                                      GDK_TYPE_DRAWABLE,
                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_WIDTH_PIXELS,
                 g_param_spec_uint ("width_pixels", NULL, NULL,
				    0, G_MAXUINT, 0,
				    (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_WIDTH_UNITS,
                 g_param_spec_double ("width_units", NULL, NULL,
				      0.0, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->destroy = gnome_canvas_re_destroy;

	item_class->realize = gnome_canvas_re_realize;
	item_class->unrealize = gnome_canvas_re_unrealize;
	item_class->translate = gnome_canvas_re_translate;
	item_class->bounds = gnome_canvas_re_bounds;

	item_class->render = gnome_canvas_re_render;
}

static void
gnome_canvas_re_init (GnomeCanvasRE *re)
{
	re->x1 = 0.0;
	re->y1 = 0.0;
	re->x2 = 0.0;
	re->y2 = 0.0;
	re->width = 0.0;
	re->fill_svp = NULL;
	re->outline_svp = NULL;
}

static void
gnome_canvas_re_destroy (GtkObject *object)
{
	GnomeCanvasRE *re;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_RE (object));

	re = GNOME_CANVAS_RE (object);

	/* remember, destroy can be run multiple times! */

	if (re->fill_stipple)
		gdk_bitmap_unref (re->fill_stipple);
	re->fill_stipple = NULL;

	if (re->outline_stipple)
		gdk_bitmap_unref (re->outline_stipple);
	re->outline_stipple = NULL;

	if (re->fill_svp)
		art_svp_free (re->fill_svp);
	re->fill_svp = NULL;

	if (re->outline_svp)
		art_svp_free (re->outline_svp);
	re->outline_svp = NULL;

	if (GTK_OBJECT_CLASS (re_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (re_parent_class)->destroy) (object);
}

static void get_bounds (GnomeCanvasRE *re, double *px1, double *py1, double *px2, double *py2)
{
	GnomeCanvasItem *item;
	double x1, y1, x2, y2;
	int cx1, cy1, cx2, cy2;
	double hwidth;

#ifdef VERBOSE
	g_print ("re get_bounds\n");
#endif
	item = GNOME_CANVAS_ITEM (re);

	if (re->width_pixels)
		hwidth = (re->width / item->canvas->pixels_per_unit) / 2.0;
	else
		hwidth = re->width / 2.0;

	x1 = re->x1;
	y1 = re->y1;
	x2 = re->x2;
	y2 = re->y2;

	gnome_canvas_item_i2w (item, &x1, &y1);
	gnome_canvas_item_i2w (item, &x2, &y2);
	gnome_canvas_w2c (item->canvas, x1 - hwidth, y1 - hwidth, &cx1, &cy1);
	gnome_canvas_w2c (item->canvas, x2 + hwidth, y2 + hwidth, &cx2, &cy2);
	*px1 = cx1;
	*py1 = cy1;
	*px2 = cx2;
	*py2 = cy2;

	/* Some safety fudging */

	*px1 -= 2;
	*py1 -= 2;
	*px2 += 2;
	*py2 += 2;
}

/* deprecated */
static void
recalc_bounds (GnomeCanvasRE *re)
{
	GnomeCanvasItem *item;

#ifdef VERBOSE
	g_print ("re recalc_bounds\n");
#endif
	item = GNOME_CANVAS_ITEM (re);

	get_bounds (re, &item->x1, &item->y1, &item->x2, &item->y2);

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
}

/* Convenience function to set a GC's foreground color to the specified pixel value */
static void
set_gc_foreground (GdkGC *gc, gulong pixel)
{
	GdkColor c;

	if (!gc)
		return;

	c.pixel = pixel;
	gdk_gc_set_foreground (gc, &c);
}

/* Sets the stipple pattern for the specified gc */
static void
set_stipple (GdkGC *gc, GdkBitmap **internal_stipple, GdkBitmap *stipple, int reconfigure)
{
	if (*internal_stipple && !reconfigure)
		gdk_bitmap_unref (*internal_stipple);

	*internal_stipple = stipple;
	if (stipple && !reconfigure)
		gdk_bitmap_ref (stipple);

	if (gc) {
		if (stipple) {
			gdk_gc_set_stipple (gc, stipple);
			gdk_gc_set_fill (gc, GDK_STIPPLED);
		} else
			gdk_gc_set_fill (gc, GDK_SOLID);
	}
}

/* Recalculate the outline width of the rectangle/ellipse and set it in its GC */
static void
set_outline_gc_width (GnomeCanvasRE *re)
{
	int width;

	if (!re->outline_gc)
		return;

	if (re->width_pixels)
		width = (int) re->width;
	else
		width = (int) (re->width * re->item.canvas->pixels_per_unit + 0.5);

	gdk_gc_set_line_attributes (re->outline_gc, width,
				    GDK_LINE_SOLID, GDK_CAP_PROJECTING, GDK_JOIN_MITER);
}

static void
gnome_canvas_re_set_fill (GnomeCanvasRE *re, gboolean fill_set)
{
	if (re->fill_set != fill_set) {
		re->fill_set = fill_set;
		gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (re));
	}
}

static void
gnome_canvas_re_set_outline (GnomeCanvasRE *re, gboolean outline_set)
{
	if (re->outline_set != outline_set) {
		re->outline_set = outline_set;
		gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (re));
	}
}

static void
gnome_canvas_re_set_property (GObject              *object,
			      guint                 param_id,
			      const GValue         *value,
			      GParamSpec           *pspec,
			      const gchar          *trailer)
{
	GnomeCanvasItem *item;
	GnomeCanvasRE *re;
	GdkColor color = { 0, 0, 0, 0, };
	GdkColor *pcolor;
	int have_pixel;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_RE (object));

	item = GNOME_CANVAS_ITEM (object);
	re = GNOME_CANVAS_RE (object);
	have_pixel = FALSE;

	switch (param_id) {
	case PROP_X1:
		re->x1 = g_value_get_double (value);

		gnome_canvas_item_request_update (item);
		break;

	case PROP_Y1:
		re->y1 = g_value_get_double (value);

		gnome_canvas_item_request_update (item);
		break;

	case PROP_X2:
		re->x2 = g_value_get_double (value);

		gnome_canvas_item_request_update (item);
		break;

	case PROP_Y2:
		re->y2 = g_value_get_double (value);

		gnome_canvas_item_request_update (item);
		break;

	case PROP_FILL_COLOR:
	case PROP_FILL_COLOR_GDK:
	case PROP_FILL_COLOR_RGBA:
		switch (param_id) {
		case PROP_FILL_COLOR:
			if (g_value_get_string (value) &&
			    gdk_color_parse (g_value_get_string (value), &color))
				gnome_canvas_re_set_fill (re, TRUE);
			else
				gnome_canvas_re_set_fill (re, FALSE);

			re->fill_color = ((color.red & 0xff00) << 16 |
					  (color.green & 0xff00) << 8 |
					  (color.blue & 0xff00) |
					  0xff);
			break;

		case PROP_FILL_COLOR_GDK:
			pcolor = g_value_get_boxed (value);
			gnome_canvas_re_set_fill (re, pcolor != NULL);

			if (pcolor) {
				GdkColormap *colormap;

				color = *pcolor;
				colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
				gdk_rgb_find_color (colormap, &color);
				have_pixel = TRUE;
			}

			re->fill_color = ((color.red & 0xff00) << 16 |
					  (color.green & 0xff00) << 8 |
					  (color.blue & 0xff00) |
					  0xff);
			break;

		case PROP_FILL_COLOR_RGBA:
			gnome_canvas_re_set_fill (re, TRUE);
			re->fill_color = g_value_get_uint (value);
			break;
		}
#ifdef VERBOSE
		g_print ("re fill color = %08x\n", re->fill_color);
#endif
		if (have_pixel)
			re->fill_pixel = color.pixel;
		else
			re->fill_pixel = gnome_canvas_get_color_pixel (item->canvas, re->fill_color);

		if (!item->canvas->aa)
			set_gc_foreground (re->fill_gc, re->fill_pixel);

		gnome_canvas_item_request_redraw_svp (item, re->fill_svp);
		break;

	case PROP_OUTLINE_COLOR:
	case PROP_OUTLINE_COLOR_GDK:
	case PROP_OUTLINE_COLOR_RGBA:
		switch (param_id) {
		case PROP_OUTLINE_COLOR:
			if (g_value_get_string (value) &&
			    gdk_color_parse (g_value_get_string (value), &color))
				gnome_canvas_re_set_outline (re, TRUE);
			else
				gnome_canvas_re_set_outline (re, FALSE);

			re->outline_color = ((color.red & 0xff00) << 16 |
					     (color.green & 0xff00) << 8 |
					     (color.blue & 0xff00) |
					     0xff);
			break;

		case PROP_OUTLINE_COLOR_GDK:
			pcolor = g_value_get_boxed (value);
			gnome_canvas_re_set_outline (re, pcolor != NULL);

			if (pcolor) {
				GdkColormap *colormap;

				color = *pcolor;
				colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
				gdk_rgb_find_color (colormap, &color);

				have_pixel = TRUE;
			}

			re->outline_color = ((color.red & 0xff00) << 16 |
					     (color.green & 0xff00) << 8 |
					     (color.blue & 0xff00) |
					     0xff);
			break;

		case PROP_OUTLINE_COLOR_RGBA:
			gnome_canvas_re_set_outline (re, TRUE);
			re->outline_color = g_value_get_uint (value);
			break;
		}
#ifdef VERBOSE
		g_print ("re outline color %x %x %x\n", color.red, color.green, color.blue);
#endif
		if (have_pixel)
			re->outline_pixel = color.pixel;
		else
			re->outline_pixel = gnome_canvas_get_color_pixel (item->canvas,
									  re->outline_color);

		if (!item->canvas->aa)
			set_gc_foreground (re->outline_gc, re->outline_pixel);

		gnome_canvas_item_request_redraw_svp (item, re->outline_svp);
		break;

	case PROP_FILL_STIPPLE:
		if (!item->canvas->aa)
			set_stipple (re->fill_gc, &re->fill_stipple, (GdkBitmap *) g_value_get_object (value), FALSE);

		break;

	case PROP_OUTLINE_STIPPLE:
		if (!item->canvas->aa)
			set_stipple (re->outline_gc, &re->outline_stipple, (GdkBitmap *) g_value_get_object (value), FALSE);
		break;

	case PROP_WIDTH_PIXELS:
		re->width = g_value_get_uint (value);
		re->width_pixels = TRUE;
		if (!item->canvas->aa)
			set_outline_gc_width (re);

		gnome_canvas_item_request_update (item);
		break;

	case PROP_WIDTH_UNITS:
		re->width = fabs (g_value_get_double (value));
		re->width_pixels = FALSE;
		if (!item->canvas->aa)
			set_outline_gc_width (re);

		gnome_canvas_item_request_update (item);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* Allocates a GdkColor structure filled with the specified pixel, and puts it into the specified
 * value for returning it in the get_property method.
 */
static void
get_color_value (GnomeCanvasRE *re, gulong pixel, GValue *value)
{
	GdkColor *color;
	GdkColormap *colormap;

	color = g_new (GdkColor, 1);
	color->pixel = pixel;

	colormap = gtk_widget_get_colormap (GTK_WIDGET (re));
	gdk_rgb_find_color (colormap, color);
	g_value_set_boxed (value,  color);
}

static void
gnome_canvas_re_get_property (GObject              *object,
			      guint                 param_id,
			      GValue               *value,
			      GParamSpec           *pspec,
			      const gchar          *trailer)
{
	GnomeCanvasRE *re;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_RE (object));

	re = GNOME_CANVAS_RE (object);

	switch (param_id) {
	case PROP_X1:
		g_value_set_double (value,  re->x1);
		break;

	case PROP_Y1:
		g_value_set_double (value,  re->y1);
		break;

	case PROP_X2:
		g_value_set_double (value,  re->x2);
		break;

	case PROP_Y2:
		g_value_set_double (value,  re->y2);
		break;

	case PROP_FILL_COLOR_GDK:
		get_color_value (re, re->fill_pixel, value);
		break;

	case PROP_OUTLINE_COLOR_GDK:
		get_color_value (re, re->outline_pixel, value);
		break;

	case PROP_FILL_COLOR_RGBA:
		g_value_set_uint (value,  re->fill_color);
		break;

	case PROP_OUTLINE_COLOR_RGBA:
		g_value_set_uint (value,  re->outline_color);
		break;

	case PROP_FILL_STIPPLE:
		g_value_set_object (value,  (GObject *) re->fill_stipple);
		break;

	case PROP_OUTLINE_STIPPLE:
		g_value_set_object (value,  (GObject *) re->outline_stipple);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
gnome_canvas_re_update_shared (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasRE *re;

#ifdef VERBOSE
	g_print ("gnome_canvas_re_update_shared\n");
#endif
	re = GNOME_CANVAS_RE (item);

	if (re_parent_class->update)
		(* re_parent_class->update) (item, affine, clip_path, flags);

	if (!item->canvas->aa) {
		set_gc_foreground (re->fill_gc, re->fill_pixel);
		set_gc_foreground (re->outline_gc, re->outline_pixel);
		set_stipple (re->fill_gc, &re->fill_stipple, re->fill_stipple, TRUE);
		set_stipple (re->outline_gc, &re->outline_stipple, re->outline_stipple, TRUE);
		set_outline_gc_width (re);
	}

#ifdef OLD_XFORM
	recalc_bounds (re);
#endif
}

static void
gnome_canvas_re_realize (GnomeCanvasItem *item)
{
	GnomeCanvasRE *re;

#ifdef VERBOSE
	g_print ("gnome_canvas_re_realize\n");
#endif
	re = GNOME_CANVAS_RE (item);

	if (re_parent_class->realize)
		(* re_parent_class->realize) (item);

	if (!item->canvas->aa) {
		re->fill_gc = gdk_gc_new (item->canvas->layout.bin_window);
		re->outline_gc = gdk_gc_new (item->canvas->layout.bin_window);
	}

#ifdef OLD_XFORM
	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->update) (item, NULL, NULL, 0);
#endif
}

static void
gnome_canvas_re_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasRE *re;

	re = GNOME_CANVAS_RE (item);

	if (!item->canvas->aa) {
		gdk_gc_unref (re->fill_gc);
		re->fill_gc = NULL;
		gdk_gc_unref (re->outline_gc);
		re->outline_gc = NULL;
	}

	if (re_parent_class->unrealize)
		(* re_parent_class->unrealize) (item);
}

static void
gnome_canvas_re_translate (GnomeCanvasItem *item, double dx, double dy)
{
	GnomeCanvasRE *re;

#ifdef VERBOSE
	g_print ("gnome_canvas_re_translate %g %g\n", dx, dy);
#endif
	re = GNOME_CANVAS_RE (item);

	re->x1 += dx;
	re->y1 += dy;
	re->x2 += dx;
	re->y2 += dy;

	recalc_bounds (re);

#ifdef VERBOSE
	g_print ("translate\n");
#endif
	if (item->canvas->aa) {
		gnome_canvas_item_request_update (item);
	}
}

static void
gnome_canvas_re_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasRE *re;
	double hwidth;

#ifdef VERBOSE
	g_print ("gnome_canvas_re_bounds\n");
#endif
	re = GNOME_CANVAS_RE (item);

	if (re->width_pixels)
		hwidth = (re->width / item->canvas->pixels_per_unit) / 2.0;
	else
		hwidth = re->width / 2.0;

	*x1 = re->x1 - hwidth;
	*y1 = re->y1 - hwidth;
	*x2 = re->x2 + hwidth;
	*y2 = re->y2 + hwidth;
}

static void
gnome_canvas_re_render (GnomeCanvasItem *item,
			GnomeCanvasBuf *buf)
{
	GnomeCanvasRE *re;

	re = GNOME_CANVAS_RE (item);

#ifdef VERBOSE
	g_print ("gnome_canvas_re_render (%d, %d) - (%d, %d) fill=%08x outline=%08x\n",
		 buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1, re->fill_color, re->outline_color);
#endif

	if (re->fill_svp != NULL) {
		gnome_canvas_render_svp (buf, re->fill_svp, re->fill_color);
	}

	if (re->outline_svp != NULL) {
		gnome_canvas_render_svp (buf, re->outline_svp, re->outline_color);
	}
}

/* Rectangle item */


static void gnome_canvas_rect_class_init (GnomeCanvasRectClass *class);

static void   gnome_canvas_rect_draw   (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height);
static double gnome_canvas_rect_point  (GnomeCanvasItem *item, double x, double y, int cx, int cy,
				        GnomeCanvasItem **actual_item);


GtkType
gnome_canvas_rect_get_type (void)
{
	static GtkType rect_type = 0;

	if (!rect_type) {
		GtkTypeInfo rect_info = {
			"GnomeCanvasRect",
			sizeof (GnomeCanvasRect),
			sizeof (GnomeCanvasRectClass),
			(GtkClassInitFunc) gnome_canvas_rect_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		rect_type = gtk_type_unique (gnome_canvas_re_get_type (), &rect_info);
	}

	return rect_type;
}

static void
gnome_canvas_rect_class_init (GnomeCanvasRectClass *class)
{
	GnomeCanvasItemClass *item_class;

	item_class = (GnomeCanvasItemClass *) class;

	item_class->draw = gnome_canvas_rect_draw;
	item_class->point = gnome_canvas_rect_point;
	item_class->update = gnome_canvas_rect_update;
}

static void
gnome_canvas_rect_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	GnomeCanvasRE *re;
	double i2w[6], w2c[6], i2c[6];
	int x1, y1, x2, y2;
	ArtPoint i1, i2;
	ArtPoint c1, c2;

	re = GNOME_CANVAS_RE (item);

	/* Get canvas pixel coordinates */

	gnome_canvas_item_i2w_affine (item, i2w);
	gnome_canvas_w2c_affine (item->canvas, w2c);
	art_affine_multiply (i2c, i2w, w2c);

	i1.x = re->x1;
	i1.y = re->y1;
	i2.x = re->x2;
	i2.y = re->y2;
	art_affine_point (&c1, &i1, i2c);
	art_affine_point (&c2, &i2, i2c);
	x1 = c1.x;
	y1 = c1.y;
	x2 = c2.x;
	y2 = c2.y;

	if (re->fill_set) {
		if (re->fill_stipple)
			gnome_canvas_set_stipple_origin (item->canvas, re->fill_gc);

		gdk_draw_rectangle (drawable,
				    re->fill_gc,
				    TRUE,
				    x1 - x,
				    y1 - y,
				    x2 - x1 + 1,
				    y2 - y1 + 1);
	}

	if (re->outline_set) {
		if (re->outline_stipple)
			gnome_canvas_set_stipple_origin (item->canvas, re->outline_gc);

		gdk_draw_rectangle (drawable,
				    re->outline_gc,
				    FALSE,
				    x1 - x,
				    y1 - y,
				    x2 - x1,
				    y2 - y1);
	}
}

static double
gnome_canvas_rect_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasRE *re;
	double x1, y1, x2, y2;
	double hwidth;
	double dx, dy;
	double tmp;

#ifdef VERBOSE
	g_print ("gnome_canvas_rect_point\n");
#endif
	re = GNOME_CANVAS_RE (item);

	*actual_item = item;

	/* Find the bounds for the rectangle plus its outline width */

	x1 = re->x1;
	y1 = re->y1;
	x2 = re->x2;
	y2 = re->y2;

	if (re->outline_set) {
		if (re->width_pixels)
			hwidth = (re->width / item->canvas->pixels_per_unit) / 2.0;
		else
			hwidth = re->width / 2.0;

		x1 -= hwidth;
		y1 -= hwidth;
		x2 += hwidth;
		y2 += hwidth;
	} else
		hwidth = 0.0;

	/* Is point inside rectangle (which can be hollow if it has no fill set)? */

	if ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2)) {
		if (re->fill_set || !re->outline_set)
			return 0.0;

		dx = x - x1;
		tmp = x2 - x;
		if (tmp < dx)
			dx = tmp;

		dy = y - y1;
		tmp = y2 - y;
		if (tmp < dy)
			dy = tmp;

		if (dy < dx)
			dx = dy;

		dx -= 2.0 * hwidth;

		if (dx < 0.0)
			return 0.0;
		else
			return dx;
	}

	/* Point is outside rectangle */

	if (x < x1)
		dx = x1 - x;
	else if (x > x2)
		dx = x - x2;
	else
		dx = 0.0;

	if (y < y1)
		dy = y1 - y;
	else if (y > y2)
		dy = y - y2;
	else
		dy = 0.0;

	return sqrt (dx * dx + dy * dy);
}

static void
gnome_canvas_rect_update (GnomeCanvasItem *item, double affine[6], ArtSVP *clip_path, gint flags)
{
	GnomeCanvasRE *re;
	ArtVpath vpath[11];
	ArtVpath *vpath2;
	double x0, y0, x1, y1;
	double halfwidth;
	int i;

#ifdef VERBOSE
	{
		char str[128];

		art_affine_to_string (str, affine);
		g_print ("gnome_canvas_rect_update item %x %s\n", item, str);

		if (item->xform) {
		  g_print ("xform = %g %g\n", item->xform[0], item->xform[1]);
		}
	}
#endif
	gnome_canvas_re_update_shared (item, affine, clip_path, flags);

	re = GNOME_CANVAS_RE (item);

	if (item->canvas->aa) {
		x0 = re->x1;
		y0 = re->y1;
		x1 = re->x2;
		y1 = re->y2;

		gnome_canvas_item_reset_bounds (item);

		if (re->fill_set) {
			vpath[0].code = ART_MOVETO;
			vpath[0].x = x0;
			vpath[0].y = y0;
			vpath[1].code = ART_LINETO;
			vpath[1].x = x0;
			vpath[1].y = y1;
			vpath[2].code = ART_LINETO;
			vpath[2].x = x1;
			vpath[2].y = y1;
			vpath[3].code = ART_LINETO;
			vpath[3].x = x1;
			vpath[3].y = y0;
			vpath[4].code = ART_LINETO;
			vpath[4].x = x0;
			vpath[4].y = y0;
			vpath[5].code = ART_END;
			vpath[5].x = 0;
			vpath[5].y = 0;

			vpath2 = art_vpath_affine_transform (vpath, affine);

			gnome_canvas_item_update_svp_clip (item, &re->fill_svp, art_svp_from_vpath (vpath2), clip_path);
			art_free (vpath2);
		} else
			gnome_canvas_item_update_svp (item, &re->fill_svp, NULL);

		if (re->outline_set) {
			/* We do the stroking by hand because it's simple enough
			   and could save time. */

			if (re->width_pixels)
				halfwidth = re->width * 0.5;
			else
				halfwidth = re->width * item->canvas->pixels_per_unit * 0.5;

			if (halfwidth < 0.25)
				halfwidth = 0.25;

			i = 0;
			vpath[i].code = ART_MOVETO;
			vpath[i].x = x0 - halfwidth;
			vpath[i].y = y0 - halfwidth;
			i++;
			vpath[i].code = ART_LINETO;
			vpath[i].x = x0 - halfwidth;
			vpath[i].y = y1 + halfwidth;
			i++;
			vpath[i].code = ART_LINETO;
			vpath[i].x = x1 + halfwidth;
			vpath[i].y = y1 + halfwidth;
			i++;
			vpath[i].code = ART_LINETO;
			vpath[i].x = x1 + halfwidth;
			vpath[i].y = y0 - halfwidth;
			i++;
			vpath[i].code = ART_LINETO;
			vpath[i].x = x0 - halfwidth;
			vpath[i].y = y0 - halfwidth;
			i++;

			if (x1 - halfwidth > x0 + halfwidth &&
			    y1 - halfwidth > y0 + halfwidth) {
				vpath[i].code = ART_MOVETO;
				vpath[i].x = x0 + halfwidth;
				vpath[i].y = y0 + halfwidth;
				i++;
				vpath[i].code = ART_LINETO;
				vpath[i].x = x1 - halfwidth;
				vpath[i].y = y0 + halfwidth;
				i++;
				vpath[i].code = ART_LINETO;
				vpath[i].x = x1 - halfwidth;
				vpath[i].y = y1 - halfwidth;
				i++;
				vpath[i].code = ART_LINETO;
				vpath[i].x = x0 + halfwidth;
				vpath[i].y = y1 - halfwidth;
				i++;
				vpath[i].code = ART_LINETO;
				vpath[i].x = x0 + halfwidth;
				vpath[i].y = y0 + halfwidth;
				i++;
			}
			vpath[i].code = ART_END;
			vpath[i].x = 0;
			vpath[i].y = 0;

			vpath2 = art_vpath_affine_transform (vpath, affine);

			gnome_canvas_item_update_svp_clip (item, &re->outline_svp, art_svp_from_vpath (vpath2), clip_path);
			art_free (vpath2);
		} else
			gnome_canvas_item_update_svp (item, &re->outline_svp, NULL);
	} else {
		/* xlib rendering - just update the bbox */
		get_bounds (re, &x0, &y0, &x1, &y1);
		gnome_canvas_update_bbox (item, x0, y0, x1, y1);
	}
}

/* Ellipse item */


static void gnome_canvas_ellipse_class_init (GnomeCanvasEllipseClass *class);

static void   gnome_canvas_ellipse_draw   (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y,
					   int width, int height);
static double gnome_canvas_ellipse_point  (GnomeCanvasItem *item, double x, double y, int cx, int cy,
					   GnomeCanvasItem **actual_item);


GtkType
gnome_canvas_ellipse_get_type (void)
{
	static GtkType ellipse_type = 0;

	if (!ellipse_type) {
		GtkTypeInfo ellipse_info = {
			"GnomeCanvasEllipse",
			sizeof (GnomeCanvasEllipse),
			sizeof (GnomeCanvasEllipseClass),
			(GtkClassInitFunc) gnome_canvas_ellipse_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		ellipse_type = gtk_type_unique (gnome_canvas_re_get_type (), &ellipse_info);
	}

	return ellipse_type;
}

static void
gnome_canvas_ellipse_class_init (GnomeCanvasEllipseClass *class)
{
	GnomeCanvasItemClass *item_class;

	item_class = (GnomeCanvasItemClass *) class;

	item_class->draw = gnome_canvas_ellipse_draw;
	item_class->point = gnome_canvas_ellipse_point;
	item_class->update = gnome_canvas_ellipse_update;
}

static void
gnome_canvas_ellipse_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	GnomeCanvasRE *re;
	double i2w[6], w2c[6], i2c[6];
	int x1, y1, x2, y2;
	ArtPoint i1, i2;
	ArtPoint c1, c2;

	re = GNOME_CANVAS_RE (item);

	/* Get canvas pixel coordinates */

	gnome_canvas_item_i2w_affine (item, i2w);
	gnome_canvas_w2c_affine (item->canvas, w2c);
	art_affine_multiply (i2c, i2w, w2c);

	i1.x = re->x1;
	i1.y = re->y1;
	i2.x = re->x2;
	i2.y = re->y2;
	art_affine_point (&c1, &i1, i2c);
	art_affine_point (&c2, &i2, i2c);
	x1 = c1.x;
	y1 = c1.y;
	x2 = c2.x;
	y2 = c2.y;

	if (re->fill_set) {
		if (re->fill_stipple)
			gnome_canvas_set_stipple_origin (item->canvas, re->fill_gc);

		gdk_draw_arc (drawable,
			      re->fill_gc,
			      TRUE,
			      x1 - x,
			      y1 - y,
			      x2 - x1,
			      y2 - y1,
			      0 * 64,
			      360 * 64);
	}

	if (re->outline_set) {
		if (re->outline_stipple)
			gnome_canvas_set_stipple_origin (item->canvas, re->outline_gc);

		gdk_draw_arc (drawable,
			      re->outline_gc,
			      FALSE,
			      x1 - x,
			      y1 - y,
			      x2 - x1,
			      y2 - y1,
			      0 * 64,
			      360 * 64);
	}
}

static double
gnome_canvas_ellipse_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasRE *re;
	double dx, dy;
	double scaled_dist;
	double outline_dist;
	double center_dist;
	double width;
	double a, b;
	double diamx, diamy;

	re = GNOME_CANVAS_RE (item);

	*actual_item = item;

	if (re->outline_set) {
		if (re->width_pixels)
			width = re->width / item->canvas->pixels_per_unit;
		else
			width = re->width;
	} else
		width = 0.0;

	/* Compute the distance between the center of the ellipse and the point, with the ellipse
	 * considered as being scaled to a circle.
	 */

	dx = x - (re->x1 + re->x2) / 2.0;
	dy = y - (re->y1 + re->y2) / 2.0;
	center_dist = sqrt (dx * dx + dy * dy);

	a = dx / ((re->x2 + width - re->x1) / 2.0);
	b = dy / ((re->y2 + width - re->y1) / 2.0);
	scaled_dist = sqrt (a * a + b * b);

	/* If the scaled distance is greater than 1, then we are outside.  Compute the distance from
	 * the point to the edge of the circle, then scale back to the original un-scaled coordinate
	 * system.
	 */

	if (scaled_dist > 1.0)
		return (center_dist / scaled_dist) * (scaled_dist - 1.0);

	/* We are inside the outer edge of the ellipse.  If it is filled, then we are "inside".
	 * Otherwise, do the same computation as above, but also check whether we are inside the
	 * outline.
	 */

	if (re->fill_set)
		return 0.0;

	if (scaled_dist > GNOME_CANVAS_EPSILON)
		outline_dist = (center_dist / scaled_dist) * (1.0 - scaled_dist) - width;
	else {
		/* Handle very small distance */

		diamx = re->x2 - re->x1;
		diamy = re->y2 - re->y1;

		if (diamx < diamy)
			outline_dist = (diamx - width) / 2.0;
		else
			outline_dist = (diamy - width) / 2.0;
	}

	if (outline_dist < 0.0)
		return 0.0;

	return outline_dist;
}

#define N_PTS 126

static void
gnome_canvas_gen_ellipse (ArtVpath *vpath, double x0, double y0,
			  double x1, double y1)
{
	int i;

	for (i = 0; i < N_PTS + 1; i++) {
		double th;

		th = (2 * M_PI * i) / N_PTS;
		vpath[i].code = i == 0 ? ART_MOVETO : ART_LINETO;
		vpath[i].x = (x0 + x1) * 0.5 + (x1 - x0) * 0.5 * cos (th);
		vpath[i].y = (y0 + y1) * 0.5 - (y1 - y0) * 0.5 * sin (th);
	}
}

static void
gnome_canvas_ellipse_update (GnomeCanvasItem *item, double affine[6], ArtSVP *clip_path, gint flags)
{
	GnomeCanvasRE *re;
	ArtVpath vpath[N_PTS + N_PTS + 3];
	ArtVpath *vpath2;
	double x0, y0, x1, y1;
	int i;

#ifdef VERBOSE
	g_print ("gnome_canvas_rect_update item %x\n", item);
#endif

	gnome_canvas_re_update_shared (item, affine, clip_path, flags);
	re = GNOME_CANVAS_RE (item);

	if (item->canvas->aa) {

#ifdef VERBOSE
		{
			char str[128];

			art_affine_to_string (str, affine);
			g_print ("g_c_r_e affine = %s\n", str);
		}
		g_print ("item %x (%g, %g) - (%g, %g)\n",
			 item,
			 re->x1, re->y1, re->x2, re->y2);
#endif

		if (re->fill_set) {
			gnome_canvas_gen_ellipse (vpath, re->x1, re->y1, re->x2, re->y2);
			vpath[N_PTS + 1].code = ART_END;
			vpath[N_PTS + 1].x = 0;
			vpath[N_PTS + 1].y = 0;

			vpath2 = art_vpath_affine_transform (vpath, affine);

			gnome_canvas_item_update_svp_clip (item, &re->fill_svp, art_svp_from_vpath (vpath2), clip_path);
			art_free (vpath2);
		} else
			gnome_canvas_item_update_svp (item, &re->fill_svp, NULL);

		if (re->outline_set) {
			double halfwidth;

			if (re->width_pixels)
				halfwidth = (re->width / item->canvas->pixels_per_unit) * 0.5;
			else
				halfwidth = re->width * 0.5;

			if (halfwidth < 0.25)
				halfwidth = 0.25;

			i = 0;
			gnome_canvas_gen_ellipse (vpath + i,
						  re->x1 - halfwidth, re->y1 - halfwidth,
						  re->x2 + halfwidth, re->y2 + halfwidth);
			i = N_PTS + 1;
			if (re->x2 - halfwidth > re->x1 + halfwidth &&
			    re->y2 - halfwidth > re->y1 + halfwidth) {
				gnome_canvas_gen_ellipse (vpath + i,
							  re->x1 + halfwidth, re->y2 - halfwidth,
							  re->x2 - halfwidth, re->y1 + halfwidth);
				i += N_PTS + 1;
			}
			vpath[i].code = ART_END;
			vpath[i].x = 0;
			vpath[i].y = 0;

			vpath2 = art_vpath_affine_transform (vpath, affine);

			gnome_canvas_item_update_svp_clip (item, &re->outline_svp, art_svp_from_vpath (vpath2), clip_path);
			art_free (vpath2);
		} else
			gnome_canvas_item_update_svp (item, &re->outline_svp, NULL);
	} else {
		/* xlib rendering - just update the bbox */
		get_bounds (re, &x0, &y0, &x1, &y1);
		gnome_canvas_update_bbox (item, x0, y0, x1, y1);
	}

}
