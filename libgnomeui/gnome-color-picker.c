/* Color picker button for GNOME
 *
 * Copyright (C) 1998 Red Hat Software, Inc.
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkcolorsel.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkframe.h>
#include <gtk/gtksignal.h>
#include "gnome-color-picker.h"
#include <libgnome/gnome-i18nP.h>


/* These are the dimensions of the color sample in the color picker */
#define COLOR_PICKER_WIDTH  20
#define COLOR_PICKER_HEIGHT 12
#define COLOR_PICKER_PAD    1

/* Size of checks and gray levels for alpha compositing checkerboard*/
#define CHECK_SIZE  4
#define CHECK_DARK  (1.0 / 3.0)
#define CHECK_LIGHT (2.0 / 3.0)



enum {
	COLOR_SET,
	LAST_SIGNAL
};

typedef void (* GnomeColorPickerSignal1) (GtkObject *object,
					  gint       arg1,
					  gint       arg2,
					  gint       arg3,
					  gint       arg4,
					  gpointer   data);

static void gnome_color_picker_marshal_signal_1 (GtkObject     *object,
						 GtkSignalFunc  func,
						 gpointer       func_data,
						 GtkArg        *args);

static void gnome_color_picker_class_init (GnomeColorPickerClass *class);
static void gnome_color_picker_init       (GnomeColorPicker      *cp);
static void gnome_color_picker_destroy    (GtkObject             *object);
static void gnome_color_picker_clicked    (GtkButton             *button);


static guint color_picker_signals[LAST_SIGNAL] = { 0 };

static GtkButtonClass *parent_class;


GtkType
gnome_color_picker_get_type (void)
{
	static GtkType cp_type = 0;

	if (!cp_type) {
		GtkTypeInfo cp_info = {
			"GnomeColorPicker",
			sizeof (GnomeColorPicker),
			sizeof (GnomeColorPickerClass),
			(GtkClassInitFunc) gnome_color_picker_class_init,
			(GtkObjectInitFunc) gnome_color_picker_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		cp_type = gtk_type_unique (gtk_button_get_type (), &cp_info);
	}

	return cp_type;
}

static void
gnome_color_picker_class_init (GnomeColorPickerClass *class)
{
	GtkObjectClass *object_class;
	GtkButtonClass *button_class;

	object_class = (GtkObjectClass *) class;
	button_class = (GtkButtonClass *) class;

	parent_class = gtk_type_class (gtk_button_get_type ());

	color_picker_signals[COLOR_SET] =
		gtk_signal_new ("color_set",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GnomeColorPickerClass, color_set),
				gnome_color_picker_marshal_signal_1,
				GTK_TYPE_NONE, 4,
				GTK_TYPE_UINT,
				GTK_TYPE_UINT,
				GTK_TYPE_UINT,
				GTK_TYPE_UINT);

	gtk_object_class_add_signals (object_class, color_picker_signals, LAST_SIGNAL);

	object_class->destroy = gnome_color_picker_destroy;

	button_class->clicked = gnome_color_picker_clicked;

	class->color_set = NULL;
}

/* Renders the pixmap for the case of dithered or use_alpha */
static void
render_dither (GnomeColorPicker *cp)
{
	int dark_r, dark_g, dark_b;
	int light_r, light_g, light_b;
	int x, y;
	int c1[3], c2[3];
	guchar *p;
	GdkPixmap *pixmap;

	/* Compute dark and light check colors */

	if (cp->use_alpha) {
		dark_r = (int) ((CHECK_DARK + (cp->r - CHECK_DARK) * cp->a) * 255.0 + 0.5);
		dark_g = (int) ((CHECK_DARK + (cp->g - CHECK_DARK) * cp->a) * 255.0 + 0.5);
		dark_b = (int) ((CHECK_DARK + (cp->b - CHECK_DARK) * cp->a) * 255.0 + 0.5);

		light_r = (int) ((CHECK_LIGHT + (cp->r - CHECK_LIGHT) * cp->a) * 255.0 + 0.5);
		light_g = (int) ((CHECK_LIGHT + (cp->g - CHECK_LIGHT) * cp->a) * 255.0 + 0.5);
		light_b = (int) ((CHECK_LIGHT + (cp->b - CHECK_LIGHT) * cp->a) * 255.0 + 0.5);
	} else {
		dark_r = light_r = (int) (cp->r * 255.0 + 0.5);
		dark_g = light_g = (int) (cp->g * 255.0 + 0.5);
		dark_b = light_b = (int) (cp->b * 255.0 + 0.5);
	}

	/* Fill image buffer */

	p = cp->im->rgb_data;

	for (y = 0; y < COLOR_PICKER_HEIGHT; y++) {
		if ((y / CHECK_SIZE) & 1) {
			c1[0] = dark_r;
			c1[1] = dark_g;
			c1[2] = dark_b;

			c2[0] = light_r;
			c2[1] = light_g;
			c2[2] = light_b;
		} else {
			c1[0] = light_r;
			c1[1] = light_g;
			c1[2] = light_b;

			c2[0] = dark_r;
			c2[1] = dark_g;
			c2[2] = dark_b;
		}

		for (x = 0; x < COLOR_PICKER_WIDTH; x++)
			if ((x / CHECK_SIZE) & 1) {
				*p++ = c1[0];
				*p++ = c1[1];
				*p++ = c1[2];
			} else {
				*p++ = c2[0];
				*p++ = c2[1];
				*p++ = c2[2];
			}
	}

	/* Render the image and copy it to our pixmap */

	gdk_imlib_changed_image (cp->im);
	gdk_imlib_render (cp->im, COLOR_PICKER_WIDTH, COLOR_PICKER_HEIGHT);
	pixmap = gdk_imlib_move_image (cp->im);

	gdk_draw_pixmap (cp->pixmap,
			 cp->gc,
			 pixmap,
			 0, 0,
			 0, 0,
			 COLOR_PICKER_WIDTH, COLOR_PICKER_HEIGHT);

	gdk_imlib_free_pixmap (pixmap);
}

/* Renders the pixmap with the contents of the color sample */
static void
render (GnomeColorPicker *cp)
{
	GdkColor c;

	if (cp->dither || cp->use_alpha)
		render_dither (cp);
	else {
		c.red   = (int) (cp->r * 65535.0 + 0.5);
		c.green = (int) (cp->g * 65535.0 + 0.5);
		c.blue  = (int) (cp->b * 65535.0 + 0.5);

		gdk_imlib_best_color_get (&c);
		gdk_gc_set_foreground (cp->gc, &c);
		gdk_draw_rectangle (cp->pixmap,
				    cp->gc,
				    TRUE,
				    0, 0,
				    COLOR_PICKER_WIDTH, COLOR_PICKER_HEIGHT);
	}
}

/* Handle exposure events for the color picker's drawing area */
static gint
expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	GnomeColorPicker *cp;

	cp = data;

	gdk_draw_pixmap (widget->window,
			 cp->gc,
			 cp->pixmap,
			 event->area.x,
			 event->area.y,
			 event->area.x,
			 event->area.y,
			 event->area.width,
			 event->area.height);

	return FALSE;
}

static void
gnome_color_picker_init (GnomeColorPicker *cp)
{
	GtkWidget *alignment;
	GtkWidget *frame;
	guchar *buf;

	/* Create the widgets */

	alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_container_border_width (GTK_CONTAINER (alignment), COLOR_PICKER_PAD);
	gtk_container_add (GTK_CONTAINER (cp), alignment);
	gtk_widget_show (alignment);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (alignment), frame);
	gtk_widget_show (frame);

	gtk_widget_push_visual (gdk_rgb_get_visual ());
	gtk_widget_push_colormap (gdk_rgb_get_cmap ());

	cp->da = gtk_drawing_area_new ();

	gtk_widget_pop_colormap ();
	gtk_widget_pop_visual ();

	gtk_drawing_area_size (GTK_DRAWING_AREA (cp->da), COLOR_PICKER_WIDTH, COLOR_PICKER_HEIGHT);
	gtk_signal_connect (GTK_OBJECT (cp->da), "expose_event",
			    (GtkSignalFunc) expose_event,
			    cp);
	gtk_container_add (GTK_CONTAINER (frame), cp->da);
	gtk_widget_show (cp->da);

	cp->title = g_strdup (_("Pick a color")); /* default title */

	/* Create the buffer for the image so that we can create an imlib image.  Also create the
	 * picker's pixmap.
	 */

	buf = g_malloc0 (COLOR_PICKER_WIDTH * COLOR_PICKER_HEIGHT * 3 * sizeof (guchar));
	cp->im = gdk_imlib_create_image_from_data (buf, NULL, COLOR_PICKER_WIDTH, COLOR_PICKER_HEIGHT);
	g_free (buf);

	gdk_imlib_render (cp->im, COLOR_PICKER_WIDTH, COLOR_PICKER_HEIGHT);
	cp->pixmap = gdk_imlib_copy_image (cp->im);
	cp->gc = gdk_gc_new (cp->pixmap);

	/* Start with opaque black, dither on, alpha disabled */

	cp->r = 0.0;
	cp->g = 0.0;
	cp->b = 0.0;
	cp->a = 1.0;
	cp->dither = TRUE;
	cp->use_alpha = FALSE;
	render (cp);
}

static void
gnome_color_picker_destroy (GtkObject *object)
{
	GnomeColorPicker *cp;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (object));

	cp = GNOME_COLOR_PICKER (object);

	gdk_imlib_free_pixmap (cp->pixmap);
	gdk_imlib_destroy_image (cp->im);
	gdk_gc_destroy (cp->gc);

	if (cp->cs_dialog)
		gtk_widget_destroy (cp->cs_dialog);

	if (cp->title)
		g_free (cp->title);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

GtkWidget *
gnome_color_picker_new (void)
{
	return GTK_WIDGET (gtk_type_new (gnome_color_picker_get_type ()));
}

/* Callback used when the color selection dialog is destroyed */
static gint
cs_destroy (GtkWidget *widget, gpointer data)
{
	GnomeColorPicker *cp;

	cp = data;

	cp->cs_dialog = NULL;

	return FALSE;
}

/* Callback for when the OK button in the color selection dialog is clicked */
static void
cs_ok_clicked (GtkWidget *widget, gpointer data)
{
	GnomeColorPicker *cp;
	gdouble color[4];
	gushort r, g, b, a;

	cp = data;

	gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (cp->cs_dialog)->colorsel),
				       color);
	gtk_widget_destroy (cp->cs_dialog);

	cp->r = color[0];
	cp->g = color[1];
	cp->b = color[2];
	cp->a = cp->use_alpha ? color[3] : 1.0;

	render (cp);
	gtk_widget_draw (cp->da, NULL);

	/* Notify the world that the color was set */

	gnome_color_picker_get_i16 (cp, &r, &g, &b, &a);
	gtk_signal_emit (GTK_OBJECT (cp), color_picker_signals[COLOR_SET],
			 r, g, b, a);
}

static void
gnome_color_picker_clicked (GtkButton *button)
{
	GnomeColorPicker *cp;
	GtkColorSelectionDialog *csd;
	gdouble color[4];

	g_return_if_fail (button != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (button));

	cp = GNOME_COLOR_PICKER (button);
	
	/*if dialog already exists, make sure it's shown and raised*/
	if(cp->cs_dialog) {
		gtk_widget_show(cp->cs_dialog);
		if(cp->cs_dialog->window)
			gdk_window_raise(cp->cs_dialog->window);
		return;
	}

	/* Create the dialog and connects its buttons */

	cp->cs_dialog = gtk_color_selection_dialog_new (cp->title);
	csd = GTK_COLOR_SELECTION_DIALOG (cp->cs_dialog);
	gtk_color_selection_set_opacity (GTK_COLOR_SELECTION (csd->colorsel), cp->use_alpha);

	color[0] = cp->r;
	color[1] = cp->g;
	color[2] = cp->b;
	color[3] = cp->use_alpha ? cp->a : 1.0;

	/* Hack: we set the color twice so that GtkColorSelection will remember its history */
	gtk_color_selection_set_color (GTK_COLOR_SELECTION (csd->colorsel), color);
	gtk_color_selection_set_color (GTK_COLOR_SELECTION (csd->colorsel), color);

	gtk_signal_connect (GTK_OBJECT (cp->cs_dialog), "destroy",
			    (GtkSignalFunc) cs_destroy,
			    cp);

	gtk_signal_connect (GTK_OBJECT (csd->ok_button), "clicked",
			    (GtkSignalFunc) cs_ok_clicked,
			    cp);

	gtk_signal_connect_object (GTK_OBJECT (csd->cancel_button), "clicked",
				   (GtkSignalFunc) gtk_widget_destroy,
				   GTK_OBJECT(cp->cs_dialog));

	/* FIXME: do something about the help button */

        gtk_window_position (GTK_WINDOW (cp->cs_dialog), GTK_WIN_POS_MOUSE);
        
        /* If there is a grabed window, set new dialog as modal */
        if (gtk_grab_get_current())
            gtk_window_set_modal(GTK_WINDOW(cp->cs_dialog),TRUE);
        
	gtk_widget_show (cp->cs_dialog);
}

void
gnome_color_picker_set_d (GnomeColorPicker *cp, gdouble r, gdouble g, gdouble b, gdouble a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));
	g_return_if_fail ((r >=	0.0) &&	(r <= 1.0));
	g_return_if_fail ((g >=	0.0) &&	(g <= 1.0));
	g_return_if_fail ((b >=	0.0) && (b <= 1.0));
	g_return_if_fail ((a >=	0.0) && (a <= 1.0));

	cp->r = r;
	cp->g = g;
	cp->b = b;
	cp->a = a;

	render (cp);
	gtk_widget_draw (cp->da, NULL);
}

void
gnome_color_picker_get_d (GnomeColorPicker *cp, gdouble *r, gdouble *g, gdouble *b, gdouble *a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	if (r)
		*r = cp->r;

	if (g)
		*g = cp->g;

	if (b)
		*b = cp->b;

	if (a)
		*a = cp->a;
}

void
gnome_color_picker_set_i8 (GnomeColorPicker *cp, guint8 r, guint8 g, guint8 b, guint8 a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));
	/* Don't check range of r,g,b,a since it's a 8 bit unsigned type. */

	cp->r = r / 255.0;
	cp->g = g / 255.0;
	cp->b = b / 255.0;
	cp->a = a / 255.0;

	render (cp);
	gtk_widget_draw (cp->da, NULL);
}

void
gnome_color_picker_get_i8 (GnomeColorPicker *cp, guint8 *r, guint8 *g, guint8 *b, guint8 *a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	if (r)
		*r = (guint8) (cp->r * 255.0 + 0.5);

	if (g)
		*g = (guint8) (cp->g * 255.0 + 0.5);

	if (b)
		*b = (guint8) (cp->b * 255.0 + 0.5);

	if (a)
		*a = (guint8) (cp->a * 255.0 + 0.5);
}

void
gnome_color_picker_set_i16 (GnomeColorPicker *cp, gushort r, gushort g, gushort b, gushort a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));
	/* Don't check range of r,g,b,a since it's a 16 bit unsigned type. */

	cp->r = r / 65535.0;
	cp->g = g / 65535.0;
	cp->b = b / 65535.0;
	cp->a = a / 65535.0;

	render (cp);
	gtk_widget_draw (cp->da, NULL);
}

void
gnome_color_picker_get_i16 (GnomeColorPicker *cp, gushort *r, gushort *g, gushort *b, gushort *a)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	if (r)
		*r = (gushort) (cp->r * 65535.0 + 0.5);

	if (g)
		*g = (gushort) (cp->g * 65535.0 + 0.5);

	if (b)
		*b = (gushort) (cp->b * 65535.0 + 0.5);

	if (a)
		*a = (gushort) (cp->a * 65535.0 + 0.5);
}

void
gnome_color_picker_set_dither (GnomeColorPicker *cp, gboolean dither)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	cp->dither = dither ? TRUE : FALSE;

	render (cp);
	gtk_widget_draw (cp->da, NULL);
}

void
gnome_color_picker_set_use_alpha (GnomeColorPicker *cp, gboolean use_alpha)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	cp->use_alpha = use_alpha ? TRUE : FALSE;

	render (cp);
	gtk_widget_draw (cp->da, NULL);
}

void
gnome_color_picker_set_title (GnomeColorPicker *cp, const char *title)
{
	g_return_if_fail (cp != NULL);
	g_return_if_fail (GNOME_IS_COLOR_PICKER (cp));

	if (cp->title)
		g_free (cp->title);

	cp->title = g_strdup (title);

	if (cp->cs_dialog)
		gtk_window_set_title (GTK_WINDOW (cp->cs_dialog), cp->title);
}

static void
gnome_color_picker_marshal_signal_1 (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args)
{
	GnomeColorPickerSignal1 rfunc;

	rfunc = (GnomeColorPickerSignal1) func;
	(* rfunc) (object,
		   GTK_VALUE_UINT (args[0]),
		   GTK_VALUE_UINT (args[1]),
		   GTK_VALUE_UINT (args[2]),
		   GTK_VALUE_UINT (args[3]),
		   func_data);
}




