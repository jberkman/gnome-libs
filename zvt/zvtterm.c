/*  zvtterm.c - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  The zvtterm widget.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <config.h>

/* need for 'gcc -ansi -pedantic' under GNU/Linux */
#ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE 1
#endif
#include <sys/types.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

#include "zvtterm.h"

#include <gdk/gdkx.h>
#include <gdk_imlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>


/* define to 'x' to enable copious debug output */
#define d(x)

/* default font */
#define DEFAULT_FONT "-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1"

#define PADDING 2

/* forward declararations */
static void zvt_term_init (ZvtTerm *term);
static void zvt_term_class_init (ZvtTermClass *class);
static void zvt_term_init (ZvtTerm *term);
static void zvt_term_destroy (GtkObject *object);
static void zvt_term_realize (GtkWidget *widget);
static void zvt_term_unrealize (GtkWidget *widget);
static void zvt_term_map (GtkWidget *widget);
static void zvt_term_unmap (GtkWidget *widget);
static void zvt_term_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void zvt_term_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint zvt_term_expose (GtkWidget *widget, GdkEventExpose *event);
static void zvt_term_draw (GtkWidget *widget, GdkRectangle *area);
static gint zvt_term_button_press (GtkWidget *widget, GdkEventButton *event);
static gint zvt_term_button_release (GtkWidget *widget, GdkEventButton *event);
static gint zvt_term_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static gint zvt_term_key_press (GtkWidget *widget, GdkEventKey *event);
static gint zvt_term_focus_in (GtkWidget *widget, GdkEventFocus *event);
static gint zvt_term_focus_out (GtkWidget *widget, GdkEventFocus *event);
static void zvt_term_selection_received (GtkWidget *widget, 
					 GtkSelectionData *selection_data, 
					 guint time);
static gint zvt_term_selection_clear (GtkWidget *widget, GdkEventSelection *event);
static void zvt_term_selection_get (GtkWidget *widget,
				    GtkSelectionData *selection_data_ptr, 
				    guint info, 
				    guint time);
static void zvt_term_child_died (ZvtTerm *term);
static void zvt_term_title_changed (ZvtTerm *term, VTTITLE_TYPE type, char *str);
static void zvt_term_title_changed_raise (void *user_data, VTTITLE_TYPE type, char *str);
static gint zvt_term_cursor_blink (gpointer data);
static void zvt_term_scrollbar_moved (GtkAdjustment *adj, GtkWidget *widget);
static void zvt_term_readdata (gpointer data, gint fd, GdkInputCondition condition);
static void zvt_term_readmsg (gpointer data, gint fd, GdkInputCondition condition);
static void zvt_term_fix_scrollbar (ZvtTerm *term);
static void vtx_unrender_selection (struct _vtx *vx);
static void zvt_term_scroll (ZvtTerm *term, int n);
static void zvt_term_scroll_by_lines (ZvtTerm *term, int n);


/* transparent terminal prototypes */
static Window     get_desktop_window (Window the_window);
static Pixmap     get_pixmap_prop (Window the_window, char *prop_id);
static GdkPixmap *load_pixmap_back (char *file, int shaded);
static GdkPixmap *create_shaded_pixmap (Pixmap p, int x, int y, int w, int h);
static void       draw_back_pixmap (GtkWidget *widget, int nx, int ny, int nw, int nh);
static gint       safe_strcmp(gchar *a, gchar *b);


/* static data */

/* The first 16 values are the ansi colors, the last
 * two are the default foreground and default background
 */
static gushort default_red[] = 
{0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,
 0x5555,0xffff,0x5555,0xffff,0x5555,0xffff,0x5555,0xffff,
 0x0000, 0xffff};

static gushort default_grn[] = 
{0x0000,0x0000,0xaaaa,0x5555,0x0000,0x0000,0xaaaa,0xaaaa,
 0x5555,0x5555,0xffff,0xffff,0x5555,0x5555,0xffff,0xffff,
 0x0000, 0xffff};

static gushort default_blu[] = 
{0x0000,0x0000,0x0000,0x0000,0xaaaa,0xaaaa,0xaaaa,0xaaaa,
 0x5555,0x5555,0x5555,0x5555,0xffff,0xffff,0xffff,0xffff,
 0x0000, 0xffff};


/* GTK signals */
enum 
{
  CHILD_DIED,
  TITLE_CHANGED,
  LAST_SIGNAL
};
static guint term_signals[LAST_SIGNAL] = { 0 };


/* GTK parent class */
static GtkWidgetClass *parent_class = NULL;


guint
zvt_term_get_type (void)
{
  static guint term_type = 0;
  
  if (!term_type)
    {
      GtkTypeInfo term_info =
      {
	"ZvtTerm",
	sizeof (ZvtTerm),
	sizeof (ZvtTermClass),
	(GtkClassInitFunc) zvt_term_class_init,
	(GtkObjectInitFunc) zvt_term_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      term_type = gtk_type_unique (gtk_widget_get_type (), &term_info);
    }

  return term_type;
}


static void
zvt_term_class_init (ZvtTermClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  ZvtTermClass *term_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  term_class = (ZvtTermClass*) class;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  term_signals[CHILD_DIED] =
    gtk_signal_new ("child_died",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (ZvtTermClass, child_died),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  term_signals[TITLE_CHANGED] =
    gtk_signal_new ("title_changed",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (ZvtTermClass, child_died),
                    gtk_marshal_NONE__INT_POINTER,
                    GTK_TYPE_NONE, 2,
		    GTK_TYPE_INT,
		    GTK_TYPE_STRING);

  gtk_object_class_add_signals (object_class, term_signals, LAST_SIGNAL);

  object_class->destroy = zvt_term_destroy;

  widget_class->realize = zvt_term_realize;
  widget_class->unrealize = zvt_term_unrealize;
  widget_class->map = zvt_term_map;
  widget_class->unmap = zvt_term_unmap;
  widget_class->draw = zvt_term_draw;
  widget_class->expose_event = zvt_term_expose;
  widget_class->focus_in_event = zvt_term_focus_in;
  widget_class->focus_out_event = zvt_term_focus_out;
  widget_class->size_request = zvt_term_size_request;
  widget_class->size_allocate = zvt_term_size_allocate;
  widget_class->key_press_event = zvt_term_key_press;
  widget_class->button_press_event = zvt_term_button_press;
  widget_class->button_release_event = zvt_term_button_release;
  widget_class->motion_notify_event = zvt_term_motion_notify;

  widget_class->selection_clear_event = zvt_term_selection_clear;
  widget_class->selection_received = zvt_term_selection_received;
  widget_class->selection_get = zvt_term_selection_get;

  term_class->child_died = zvt_term_child_died;
  term_class->title_changed = zvt_term_title_changed;
}


static void
zvt_term_init (ZvtTerm *term)
{
  struct _zvtprivate *zp;

  GTK_WIDGET_SET_FLAGS (term, GTK_CAN_FOCUS);

  /* create and configure callbacks for the teminal back-end */
  term->vx = vtx_new (80, 24, term);
  term->vx->vt.ring_my_bell = zvt_term_bell;
  term->vx->vt.change_my_name = zvt_term_title_changed_raise;

  term->shadow_type = GTK_SHADOW_NONE;
  term->term_window = NULL;
  term->cursor_bar = NULL;
  term->cursor_dot = NULL;
  term->cursor_current = NULL;
  term->font = NULL;
  term->font_bold = NULL;
  term->scroll_gc = NULL;
  term->fore_gc = NULL;
  term->back_gc = NULL;
  term->fore_last = 0;
  term->back_last = 0;
  term->color_ctx = 0;
  term->ic = NULL;

  /* background pixmap */
  term->pixmap_filename = NULL;
  term->background.pix = NULL;
  term->background.x = 0;
  term->background.y = 0;
  term->background.w = 0;
  term->background.h = 0;

  /* grid sizing */
  term->set_grid_size_pending = TRUE;
  term->grid_width = term->vx->vt.width;
  term->grid_height = term->vx->vt.height;

  /* input handlers */
  term->input_id = -1;
  term->msg_id = -1;
  term->timeout_id = -1;

  /* bitfield flags */
  term->cursor_on = 0;
  term->cursor_filled = 1;
  term->cursor_blink_state = 0;
  term->scroll_on_keystroke = 0;
  term->scroll_on_output = 0;
  term->blink_enabled = 1;
  term->ic = NULL;
  term->in_expose = 0;
  term->transparent = 0;
  term->shaded = 0;

  /* charwidth, charheight set here */
  zvt_term_set_font_name (term, DEFAULT_FONT);

  /* scrollback position adjustment */
  term->adjustment =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 1.0, 1.0, 1.0, 1.0));
  gtk_object_ref (GTK_OBJECT (term->adjustment));
  gtk_object_sink (GTK_OBJECT (term->adjustment));

  gtk_signal_connect (
      GTK_OBJECT (term->adjustment),
      "value_changed",
      GTK_SIGNAL_FUNC (zvt_term_scrollbar_moved),
      term);

  /* selection received */
  gtk_selection_add_target (
      GTK_WIDGET (term),
      GDK_SELECTION_PRIMARY,
      GDK_SELECTION_TYPE_STRING, 
      0);

  /* private data */
  zp = g_malloc(sizeof(*zp));
  if (zp) {
    zp->scrollselect_id = -1;
  }
  gtk_object_set_data(GTK_OBJECT (term), "_zvtprivate", zp);
}

/**
 * zvt_term_set_blink:
 * @term: A &ZvtTerm widget.
 * @state: The blinking state.  If %TRUE, the cursor will blink.
 *
 * Use this to control the way the cursor is displayed (blinking/solid)
 */
void
zvt_term_set_blink (ZvtTerm *term, int state)
{
  g_return_if_fail (term != NULL);                     
  g_return_if_fail (ZVT_IS_TERM (term));

  if (!(term->blink_enabled ^ (state?1:0)))
    return;

  if (term->blink_enabled)
    {
      if (term->timeout_id != -1){
	gtk_timeout_remove (term->timeout_id);
	term->timeout_id = -1;
      }

      if (GTK_WIDGET_REALIZED (term))
	vt_cursor_state (GTK_WIDGET (term), 1);
    
      term->blink_enabled = 0;
    } 
  else 
    {
      term->timeout_id = gtk_timeout_add (500, zvt_term_cursor_blink, term);
      term->blink_enabled = 1;
    }
}

/**
 * zvt_term_set_scroll_on_keystroke:
 * @term: A &ZvtTerm widget.
 * @state: Desired state.
 * 
 * If @state is %TRUE, forces the terminal to jump out of the
 * scrollback buffer whenever a keypress is received.
 **/
void 
zvt_term_set_scroll_on_keystroke(ZvtTerm *term, int state)
{
  term->scroll_on_keystroke = (state != 0);
}

/**
 * zvt_term_set_scroll_on_output:
 * @term: A &ZvtTerm widget.
 * @state: Desired state.
 *
 * If @state is %TRUE, forces the terminal to scroll on output
 * being generated by a child process or by zvt_term_feed().
 */
void 
zvt_term_set_scroll_on_output   (ZvtTerm *term, int state)
{
  g_return_if_fail (term != NULL);                     
  g_return_if_fail (ZVT_IS_TERM (term));

  term->scroll_on_output = (state != 0);
}

/**
 * zvt_term_set_wordclass:
 * @term: A &ZvtTerm widget.
 * @class: A string of characters to consider a "word" character.
 *
 * Sets the list of characters (character class) that are considered
 * part of a word, when selecting by word.  The @class is defined
 * the same was as a regular expression character class (as normally
 * defined using []'s, but without those included).  A leading or trailing
 * hypen (-) is used to include a hyphen in the character class.
 *
 * Passing a %NULL @class restores the default behaviour of alphanumerics
 * plus "_"  (i.e. "A-Za-z0-9_").
 */
void zvt_term_set_wordclass(ZvtTerm *term, unsigned char *class)
{
  g_return_if_fail (term != NULL);                     
  g_return_if_fail (ZVT_IS_TERM (term));

  vt_set_wordclass(term->vx, class);
}

/**
 * zvt_term_new_with_size:
 * @cols: Number of columns required.
 * @rows: Number of rows required.
 *
 * Creates a new ZVT Terminal widget of the given character dimentions.
 * If the encompassing widget is resizable, then this size may change
 * afterwards, but should be correct at realisation time.
 *
 * Return Value: A pointer to a &ZvtTerm widget is returned, or %NULL
 * on error.
 */
GtkWidget*
zvt_term_new_with_size (int cols, int rows)
{
  ZvtTerm *term;
  term = gtk_type_new (zvt_term_get_type ());

  /* fudge the pixel size, not (really) used anyway */
  vt_resize (&term->vx->vt, cols, rows, cols*8, rows*8);
  term->grid_width = cols;
  term->grid_height = rows;
  term->set_grid_size_pending = TRUE;

  return GTK_WIDGET (term);
}


/**
 * zvt_term_new:
 *
 * Creates a new ZVT Terminal widget.  By default the terminal will be
 * setup as 80 colmns x 24 rows, but it will size automatically to its
 * encompassing widget, and may be smaller or larger upon realisation.
 *
 * Return Value: A pointer to a &ZvtTerm widget is returned, or %NULL
 * on error.
 */
GtkWidget*
zvt_term_new (void)
{
  ZvtTerm *term;
  term = gtk_type_new (zvt_term_get_type ());
  return GTK_WIDGET (term);
}

static void
zvt_term_destroy (GtkObject *object)
{
  ZvtTerm *term;
  struct _zvtprivate *zp;

  g_return_if_fail (object != NULL);
  g_return_if_fail (ZVT_IS_TERM (object));

  term = ZVT_TERM (object);
  zp = gtk_object_get_data (GTK_OBJECT (term), "_zvtprivate");

  zvt_term_closepty (term);
  vtx_destroy (term->vx);

  if (term->font)
    {
      gdk_font_unref (term->font);
      term->font = NULL;
    }

  if (term->font_bold)
    {
      gdk_font_unref (term->font_bold);
      term->font_bold = NULL;
    }

  /* release the adjustment */
  if (term->adjustment)
    {
      gtk_signal_disconnect_by_data (GTK_OBJECT (term->adjustment), term);
      gtk_object_unref (GTK_OBJECT (term->adjustment));
      term->adjustment = NULL;
    }

  if (term->ic) 
    {
      gdk_ic_destroy(term->ic);
      term->ic = NULL;
    }

  /* release private data */
  if (zp) {
    g_free(zp);
    gtk_object_set_data (GTK_OBJECT (term), "_zvtprivate", 0);
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/**
 * zvt_term_reset:
 * @term: A &ZvtTerm widget.
 * @hard: If %TRUE, then perform a HARD reset.
 * 
 * Performs a complete reset on the terminal.  Resets all
 * attributes, and if @hard is %TRUE, also clears the screen.
 **/
void
zvt_term_reset (ZvtTerm *term, int hard)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));

  vt_cursor_state (term, 0);
  vt_reset_terminal(&term->vx->vt, hard);
  vt_update (term->vx, UPDATE_CHANGES);
  vt_cursor_state (term, 1);
}


/**
 * zvt_term_set_color_scheme:
 * @term: A &ZvtTerm widget.
 * @red:  pointer to a gushort array of 18 elements with red values.
 * @grn:  pointer to a gushort array of 18 elements with green values.
 * @blu:  pointer to a gushort array of 18 elements with blue values.
 *
 * This function sets the colour palette for the terminal @term.  Each
 * pointer points to a gushort array of 18 elements.  White is 0xffff
 * in all elements.
 *
 * The elements 0 trough 15 are the first 16 colours for the terminal,
 * with element 16 and 17 the default foreground and background colour
 * respectively.
 */
void
zvt_term_set_color_scheme (ZvtTerm *term, gushort *red, gushort *grn, gushort *blu)
{
  int  nallocated;
  GdkColor c;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));
  g_return_if_fail (red != NULL);
  g_return_if_fail (grn != NULL);
  g_return_if_fail (blu != NULL);
  
  memset (term->colors, 0, sizeof (term->colors));
  nallocated = 0;
  gdk_color_context_get_pixels (term->color_ctx, red, grn, blu, 18, 
				term->colors, &nallocated);
  c.pixel = term->colors [17];
  gdk_window_set_background (term->term_window, &c);
  gdk_window_set_background (GTK_WIDGET (term)->window, &c);
  gdk_window_clear (GTK_WIDGET (term)->window);
}

/**
 * zvt_term_set_default_color_scheme:
 * @term: A &ZvtTerm widget.
 *
 * Resets the color values to the default color scheme.
 */
void
zvt_term_set_default_color_scheme (ZvtTerm *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));
  
  zvt_term_set_color_scheme (term, default_red, default_grn, default_blu);
}

/**
 * zvt_term_set_size:
 * @term: A &ZvtTerm widget.
 * @width: Width of terminal, in columns.
 * @height: Height of terminal, in rows.
 * 
 * Causes the terminal to attempt to resize to the absolute character
 * size of @width rows by @height columns.
 **/
void
zvt_term_set_size (ZvtTerm *term, guint width, guint height)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));

  term->set_grid_size_pending = TRUE;
  term->grid_width = width;
  term->grid_height = height;

  gtk_widget_queue_resize (GTK_WIDGET (term));
}

static void
zvt_term_realize (GtkWidget *widget)
{
  ZvtTerm *term;
  GdkWindowAttr attributes;
  GdkPixmap *cursor_dot_pm;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  term = ZVT_TERM (widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  /* main window */
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gdk_window_set_user_data (widget->window, widget);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

  /* terminal window */
  attributes.x = widget->style->klass->xthickness + PADDING;
  attributes.y = widget->style->klass->ythickness;
  attributes.width = widget->allocation.width - (2 * widget->style->klass->xthickness) - PADDING;
  attributes.height = widget->allocation.height - (2 * widget->style->klass->ythickness);
  attributes.event_mask = gtk_widget_get_events (widget) |
    GDK_EXPOSURE_MASK | 
    GDK_BUTTON_PRESS_MASK | 
    GDK_BUTTON_MOTION_MASK |
    GDK_POINTER_MOTION_MASK | 
    GDK_BUTTON_RELEASE_MASK | 
    GDK_KEY_PRESS_MASK;

  term->term_window = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (term->term_window, term);
  gtk_style_set_background (widget->style, term->term_window, GTK_STATE_ACTIVE);

  /* create pixmaps for this window */
  cursor_dot_pm = 
    gdk_pixmap_create_from_data(term->term_window,
				"\0", 1, 1, 1,
				&widget->style->fg[GTK_STATE_ACTIVE],
				&widget->style->bg[GTK_STATE_ACTIVE]);

  /* Get I beam cursor, and also create a blank one based on the blank image */
  term->cursor_bar = gdk_cursor_new(GDK_XTERM);
  term->cursor_dot = 
    gdk_cursor_new_from_pixmap(cursor_dot_pm, cursor_dot_pm,
			       &widget->style->fg[GTK_STATE_ACTIVE],
			       &widget->style->bg[GTK_STATE_ACTIVE],
			       0, 0);
  gdk_window_set_cursor(term->term_window, term->cursor_bar);
  gdk_pixmap_unref (cursor_dot_pm);
  term->cursor_current = term->cursor_bar;

  /* setup blinking cursor */
  if (term->blink_enabled)
    term->timeout_id = gtk_timeout_add(500, zvt_term_cursor_blink, term);

  /* setup scrolling gc */
  term->scroll_gc = gdk_gc_new (term->term_window);
  gdk_gc_set_exposures (term->scroll_gc, TRUE);

  /* Colors */
  term->fore_gc = gdk_gc_new (term->term_window);
  term->back_gc = gdk_gc_new (term->term_window);
  term->color_ctx = 
      gdk_color_context_new (gtk_widget_get_visual (GTK_WIDGET (term)),
			     gtk_widget_get_colormap (GTK_WIDGET (term)));

  /* Allocate default color set */
  zvt_term_set_default_color_scheme (term);
  
  /* set the initial colours */
  term->back_last = -1;
  term->fore_last = -1;

  /* input context */
  if (gdk_im_ready () && !term->ic)
    {
      GdkICAttr attr;
      
      /* FIXME: do we have any window yet? */
      attr.style = GDK_IM_PREEDIT_NOTHING | GDK_IM_STATUS_NOTHING;
      attr.client_window = attr.focus_window = term->term_window;
      term->ic = gdk_ic_new(&attr, GDK_IC_ALL_REQ);
      
      if (!term->ic) 
	{
	  g_warning("Can't create input context.");
	}
    }
}

static void
zvt_term_unrealize (GtkWidget *widget)
{
  ZvtTerm *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  term = ZVT_TERM (widget);

  /* free resources */
  gdk_cursor_destroy (term->cursor_bar);
  term->cursor_bar = NULL;

  gdk_cursor_destroy (term->cursor_dot);
  term->cursor_dot = NULL;

  term->cursor_current = NULL;

  gdk_color_context_free (term->color_ctx);
  term->color_ctx = NULL;

  gdk_gc_destroy (term->scroll_gc);
  term->scroll_gc = NULL;

  gdk_gc_destroy (term->back_gc);
  term->back_gc = NULL;

  gdk_gc_destroy (term->fore_gc);
  term->fore_gc = NULL;

  /* destroy the terminal window */
  gdk_window_set_user_data (term->term_window, NULL);
  gdk_window_destroy (term->term_window);
  term->term_window = NULL;

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    (*GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}


static void
zvt_term_map (GtkWidget *widget)
{
  ZvtTerm *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  term = ZVT_TERM (widget);

  if (!GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

      gdk_window_show (widget->window);
      gdk_window_show (term->term_window);

      /* XXX: is this right? --JMP */
      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);
    }
}


static void
zvt_term_unmap (GtkWidget *widget)
{
  ZvtTerm *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  term = ZVT_TERM (widget);

  if (GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

      gdk_window_hide (term->term_window);
      gdk_window_hide (widget->window);
    }
}


static gint
zvt_term_focus_in(GtkWidget *widget, GdkEventFocus *event)
{
  ZvtTerm *term;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);

  vt_cursor_state(term, 0);
  term->cursor_filled = 1;
  vt_cursor_state(term, 1);

  /* setup blinking cursor */
  if (term->blink_enabled && term->timeout_id == -1)
    term->timeout_id = gtk_timeout_add (500, zvt_term_cursor_blink, term);

  if (term->ic)
    gdk_im_begin (term->ic, term->term_window);

  return FALSE;
}


static gint
zvt_term_focus_out(GtkWidget *widget, GdkEventFocus *event)
{
  ZvtTerm *term;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

  vt_cursor_state(term, 0);
  term->cursor_filled = 0;
  vt_cursor_state(term, 1);

  /* setup blinking cursor */
  if (term->blink_enabled && term->timeout_id != -1)
    {
      gtk_timeout_remove(term->timeout_id);
      term->timeout_id = -1;
    }

  if (term->ic)
    gdk_im_end ();

  return FALSE;
}


static void 
zvt_term_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  ZvtTerm *term;
  guint grid_width, grid_height;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));
  g_return_if_fail (requisition != NULL);

  term = ZVT_TERM (widget);

  if (term->set_grid_size_pending)
    {
      grid_width = term->grid_width;
      grid_height = term->grid_height;
      
      if (grid_width <= 0)
	grid_width = 8;
      
      if (grid_height <= 0)
	grid_height = 2;
      term->set_grid_size_pending = FALSE;
    }
  else
    {
      grid_width = term->grid_width;
      grid_height = term->grid_height;
    }

  requisition->width = (grid_width * term->charwidth) + 
    (widget->style->klass->xthickness * 2) + PADDING;
  requisition->height = (grid_height * term->charheight) + 
    (widget->style->klass->ythickness * 2);

  /* debug ouput */
  d( printf("zvt_term_size_request x=%d y=%d\n", grid_width, grid_height) );
}


static void
zvt_term_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  gint term_width, term_height;
  guint grid_width, grid_height;
  ZvtTerm *term;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED (widget))
    {
      term = ZVT_TERM (widget);

      d( printf("zvt_term_size_allocate old grid x=%d y=%d\n", 
		term->vx->vt.width,
		term->vx->vt.height) );
      
      gdk_window_move_resize (widget->window,
			      allocation->x,
			      allocation->y,
			      allocation->width,
			      allocation->height);

      term_width = allocation->width - (2 * widget->style->klass->xthickness);
      term_height = allocation->height - (2 * widget->style->klass->ythickness);

      gdk_window_move_resize (term->term_window,
			      widget->style->klass->xthickness + PADDING,
			      widget->style->klass->ythickness,
			      term_width,
			      term_height);

      /* resize the virtual terminal buffer, minimal size is 1x1 */
      grid_width = MAX(term_width / term->charwidth, 1);
      grid_height = MAX(term_height / term->charheight, 1);
      vt_resize (&term->vx->vt, grid_width, grid_height, term_width, term_height);
      vt_update (term->vx, UPDATE_REFRESH|UPDATE_SCROLLBACK);

      d( printf("zvt_term_size_allocate grid calc x=%d y=%d\n", 
		grid_width,
		grid_height) );
      
      /* resize the scrollbar */
      zvt_term_fix_scrollbar (term);

      d( printf("zvt_term_size_allocate new grid x=%d y=%d\n", 
		term->vx->vt.width,
		term->vx->vt.height) );
    }
}


static void 
zvt_term_draw (GtkWidget *widget, GdkRectangle *area)
{
  gint width, height;
  ZvtTerm *term;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));
  g_return_if_fail (area != NULL);

  term = ZVT_TERM (widget);

  d( printf("zvt_term_draw x=%d y=%d\n", 
	    term->vx->vt.width,
	    term->vx->vt.height) );

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      d( printf("zvt_term_draw is drawable\n") );

      term->in_expose = 1;

      /* draw shadow/border */
      if (term->shadow_type != GTK_SHADOW_NONE)
	      gtk_draw_shadow (
		      widget->style, widget->window,
		      GTK_STATE_NORMAL, term->shadow_type, 0, 0, 
		      widget->allocation.width,
		      widget->allocation.height);

      gdk_window_get_size (term->term_window, &width, &height);

      if (term->transparent || term->pixmap_filename)
	{
	  draw_back_pixmap(widget, 0, 0, width, height);
	}
      else
	{
	  gdk_window_clear_area (term->term_window, 0, 0, width, height);
	}

      vt_update_rect (term->vx, 0, 0,
		      width / term->charwidth,
		      height / term->charheight);

      term->in_expose = 0;
    }	  
}


static gint
zvt_term_expose (GtkWidget      *widget,
		 GdkEventExpose *event)
{
  ZvtTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  /* d(printf("exposed!\n")); */

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      term = ZVT_TERM (widget);

      /* FIXME: may update 1 more line/char than needed */
      term->in_expose = 1;

      if (event->window == widget->window && term->shadow_type != GTK_SHADOW_NONE)
	gtk_draw_shadow (
	    widget->style, widget->window,
	    GTK_STATE_NORMAL, term->shadow_type, 0, 0, 
	    widget->allocation.width,
	    widget->allocation.height);

      if (event->window == term->term_window)
	{
	  if(term->transparent || term->pixmap_filename)
	    draw_back_pixmap(widget,
			     event->area.x, 
			     event->area.y,
			     event->area.width, 
			     event->area.height);

	  vt_update_rect (
              term->vx,
	      event->area.x / term->charwidth,
	      event->area.y / term->charheight,
	      (event->area.x + event->area.width) / term->charwidth+1,
	      (event->area.y + event->area.height) / term->charheight+1);
	}

      term->in_expose = 0;
    }

  return FALSE;
}


/**
 * zvt_term_show_pointer:
 * @term: A &ZvtTerm widget.
 *
 * Show the default I beam pointer.
 */
void
zvt_term_show_pointer (ZvtTerm *term)
{
  g_return_if_fail (term != NULL);

  if (term->cursor_current != term->cursor_bar)
    {
      gdk_window_set_cursor(term->term_window, term->cursor_bar);
      term->cursor_current = term->cursor_bar;
    }
}

/**
 * zvt_term_show_pointer:
 * @term: A &ZvtTerm widget.
 *
 * Hide the pointer.  In reality the pointer is changed to a
 * single-pixel black dot.
 */
void
zvt_term_hide_pointer (ZvtTerm *term)
{
  g_return_if_fail (term != NULL);

  if (term->cursor_current != term->cursor_dot)
    {
      gdk_window_set_cursor(term->term_window, term->cursor_dot);
      term->cursor_current = term->cursor_dot;
    }
}

/**
 * zvt_term_set_scrollback:
 * @term: A &ZvtTerm widget.
 * @lines: Number of lines desired for the scrollback buffer.
 *
 * Set the maximum number of scrollback lines for the widget @term to
 * @lines lines.
 */
void
zvt_term_set_scrollback (ZvtTerm *term, int lines)
{
  g_return_if_fail (term != NULL);

  vt_scrollback_set (&term->vx->vt, lines);
  zvt_term_fix_scrollbar (term);
}


/* Load a set of fonts into the terminal.
 * These fonts should be the same size, otherwise it could get messy ...
 */
static void
zvt_term_set_fonts_internal(ZvtTerm *term, GdkFont *font, GdkFont *font_bold)
{
  /* set up a size pending request so that no matter
   * what the queue resize will resize the widget to
   * have the same grid size
   */
  if (!term->set_grid_size_pending)
    {
      term->set_grid_size_pending = TRUE;
      term->grid_width = term->vx->vt.width;
      term->grid_height = term->vx->vt.height;
    }

  if (font) 
    {
      if (term->font)
	{
	  gdk_font_unref (term->font);
	}
      term->font = font;
    }

  if (font_bold)
    {
      if (term->font_bold)
	{
	  gdk_font_unref (term->font_bold);
	}
      term->font_bold = font_bold;
    }

  /* re-size window */
  term->charwidth = gdk_string_width (term->font, "M");
  term->charheight = term->font->ascent + term->font->descent;

  gtk_widget_queue_resize (GTK_WIDGET (term));
}

/**
 * zvt_term_set_fonts:
 * @term: A &ZvtTerm widget.
 * @font: Font used for regular text.
 * @font_bold: Font used for bold text.
 *
 * Load a set of fonts into the terminal.
 * 
 * These fonts should be the same size, otherwise it could get messy ...
 */
void
zvt_term_set_fonts (ZvtTerm *term, GdkFont *font, GdkFont *font_bold)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));
  g_return_if_fail (font != NULL);
  g_return_if_fail (font_bold != NULL);

  zvt_term_set_fonts_internal (term, font, font_bold);

  gdk_font_ref (font);
  gdk_font_ref (font_bold);
}

/**
 * zvt_term_set_font_name:
 * @term: A &ZvtTerm widget.
 * @name: A full X11 font name string.
 *
 * Set a font by name only.  If font aliases such as 'fixed' or
 * '10x20' are passed to this function, then both the bold and
 * non-bold font will be identical.  In colour mode bold fonts are
 * always the top 8 colour scheme entries, and so bold is still
 * rendered.
 *
 * Tries to calculate bold font name from the base name.  This only
 * works with fonts where the names are alike.
 */
void
zvt_term_set_font_name (ZvtTerm *term, char *name)
{
  int count;
  char c, *ptr, *rest;
  GString *newname, *outname;
  GdkFont *font, *font_bold;

  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));
  g_return_if_fail (name != NULL);

  newname = g_string_new (name);
  outname = g_string_new ("");

  rest = 0;
  ptr = newname->str;

  for (count = 0; (c = *ptr++);)
    {
      if (c == '-')
	{
	  count++;

	  d(printf("scanning (%c) (%d)\n", c, count));
	  d(printf("newname = %s ptr = %s\n", newname->str, ptr));

	  switch (count) 
	    {
	    case 3:
	      ptr[-1] = 0;
	      break;

	    case 5:
	      rest = ptr - 1;
	      break;
	    }
	}
    }

  if (rest)
    {
      g_string_sprintf (outname, "%s-medium-r%s", newname->str, rest);
      font = gdk_font_load (outname->str);
      d( printf("loading normal font %s\n", outname->str) );
     
      g_string_sprintf (outname, "%s-bold-r%s", newname->str, rest); 
      font_bold = gdk_font_load (outname->str);
      d( printf("loading bold font %s\n", outname->str) );
      
      zvt_term_set_fonts_internal (term, font, font_bold);
    } 
  else
    {
      font = gdk_font_load (name);
      font_bold = gdk_font_load (name);
      zvt_term_set_fonts_internal (term, font, font_bold);
    }

  g_string_free (newname, TRUE);
  g_string_free (outname, TRUE);
}


/*
  Called when something has changed, size of window or scrollback.

  Fixes the adjustment and notifies the system.
*/
static void 
zvt_term_fix_scrollbar (ZvtTerm *term)
{
  GTK_ADJUSTMENT(term->adjustment)->upper = 
    term->vx->vt.scrollbacklines + term->vx->vt.height - 1;

  GTK_ADJUSTMENT(term->adjustment)->value = 
    term->vx->vt.scrollbackoffset + term->vx->vt.scrollbacklines;

  GTK_ADJUSTMENT(term->adjustment)->page_increment = 
    term->vx->vt.height - 1;

  GTK_ADJUSTMENT(term->adjustment)->page_size =
    term->vx->vt.height - 1;

  gtk_signal_emit_by_name (GTK_OBJECT (term->adjustment), "changed");
}

static void
request_paste (GtkWidget *widget)
{
  GdkAtom string_atom;

  /* Get the atom corresonding to the target "STRING" */
  string_atom = gdk_atom_intern ("STRING", FALSE);

  if (string_atom == GDK_NONE) {
    g_warning("WARNING: Could not get string atom\n");
  }

  /* And request the "STRING" target for the primary selection */
  gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, string_atom,
			 GDK_CURRENT_TIME);
}

/*
  perhaps most of the button press stuff could be shifted
  to the update file.  as for the report_button function
  shifted to the vt file ?

*/

static gint
zvt_term_button_press (GtkWidget      *widget,
		       GdkEventButton *event)
{
  gint x,y;
  GdkModifierType mask;
  struct _vtx *vx;
  ZvtTerm *term;
  struct _zvtprivate *zp;

  d(printf("button pressed\n"));

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  vx = term->vx;
  zp = gtk_object_get_data (GTK_OBJECT (term), "_zvtprivate");

  zvt_term_show_pointer (term);


  gdk_window_get_pointer(term->term_window, &x, &y, &mask);
  x /= term->charwidth;
  y = y / term->charheight + vx->vt.scrollbackoffset;

  if (zp && zp->scrollselect_id!=-1) {
    gtk_timeout_remove(zp->scrollselect_id);
    zp->scrollselect_id=-1;
  }

  /* Shift is an overwrite key for the reporting of the buttons */
  if (!(event->state & GDK_SHIFT_MASK))
    if (vt_report_button(&vx->vt, event->button, event->state, x, y)) 
      return FALSE;

  /* ignore all control-clicks' at this level */
  if (event->state & GDK_CONTROL_MASK) 
    {
      return FALSE;
    }
    
  switch(event->button) {
  case 1:			/* left button */

    /* set selection type, and from which end we are selecting */
    switch(event->type) {
    case GDK_BUTTON_PRESS:
      vx->selectiontype = VT_SELTYPE_CHAR|VT_SELTYPE_BYSTART;
      break;
    case GDK_2BUTTON_PRESS:
      vx->selectiontype = VT_SELTYPE_WORD|VT_SELTYPE_BYSTART|VT_SELTYPE_MOVED;
      break;
    case GDK_3BUTTON_PRESS:
      vx->selectiontype = VT_SELTYPE_LINE|VT_SELTYPE_BYSTART|VT_SELTYPE_MOVED;
      break;
    default:
      break;
    }
    
    /* reset selection */
    vx->selstartx = x;
    vx->selstarty = y;
    vx->selendx = x;
    vx->selendy = y;
    
    /* reset 'drawn' screen (to avoid mis-refreshes) */
    if (!vx->selected) {
      vx->selstartxold = x;
      vx->selstartyold = y;
      vx->selendxold = x;
      vx->selendyold = y;
      vx->selected =1;
    }

    if (event->type != GDK_BUTTON_PRESS) {
      vt_fix_selection(vx);
      vt_draw_selection(vx);	/* handles by line/by word update */
    }

    d( printf("selection starting %d %d\n", x, y) );

    gtk_grab_add (widget);
    gdk_pointer_grab (term->term_window, TRUE,
		      GDK_BUTTON_RELEASE_MASK |
		      GDK_BUTTON_MOTION_MASK |
		      GDK_POINTER_MOTION_HINT_MASK,
		      NULL, NULL, 0);
    break;

  case 2:			/* middle button - paste */
    request_paste (widget);
    break;

  case 3:			/* right button - select extend? */
    if (vx->selected) {
	
      switch(event->type) {
      case GDK_BUTTON_PRESS:
	vx->selectiontype = VT_SELTYPE_CHAR;
	break;
      case GDK_2BUTTON_PRESS:
	vx->selectiontype = VT_SELTYPE_WORD;
	break;
      case GDK_3BUTTON_PRESS:
	vx->selectiontype = VT_SELTYPE_LINE;
	break;
      default:
	break;
      }

      /* FIXME: the way this works out which end to select from, and
	 what happens to the other end is a little different to Xterm -
	 but it kinda works ok */
      if (y<=vx->selstarty) {
	vx->selstarty=y;
	vx->selstartx=x;
	vx->selectiontype |= VT_SELTYPE_BYEND;
      } else {
	vx->selendy=y;
	vx->selendx=x;
	vx->selectiontype |= VT_SELTYPE_BYSTART;
      }
      
      vt_fix_selection(vx);
      vt_draw_selection(vx);
      
      gtk_grab_add (widget);
      gdk_pointer_grab (term->term_window, TRUE,
			GDK_BUTTON_RELEASE_MASK |
			GDK_BUTTON_MOTION_MASK |
			GDK_POINTER_MOTION_HINT_MASK,
			NULL, NULL, 0);
      }
    break;

  case 4:
    zvt_term_scroll_by_lines (term, -12);
    break;

  case 5:
    zvt_term_scroll_by_lines (term, 12);
    break;
  }
  return FALSE;
}

static gint
zvt_term_button_release (GtkWidget      *widget,
			 GdkEventButton *event)
{
  ZvtTerm *term;
  gint x, y;
  GdkModifierType mask;
  struct _vtx *vx;
  struct _zvtprivate *zp;

  d(printf("button released\n"));

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  vx = term->vx;
  zp = gtk_object_get_data (GTK_OBJECT (term), "_zvtprivate");

  gdk_window_get_pointer (term->term_window, &x, &y, &mask);

  x = x/term->charwidth;
  y = y/term->charheight + vx->vt.scrollbackoffset;

  /* reset scrolling selection timer if it is enabled */
  if (zp && zp->scrollselect_id!=-1) {
    gtk_timeout_remove(zp->scrollselect_id);
    zp->scrollselect_id=-1;
  }

  /* ignore wheel mice buttons (4 and 5) */
  /* otherwise they affect the selection */
  if (event->button == 4 || event->button == 5)
    return FALSE;
  
  /* report mouse to terminal */
  if (!(event->state & GDK_SHIFT_MASK))
    if (vt_report_button(&vx->vt, 0, event->state, x, y))
      return FALSE;

  /* ignore all control-clicks' at this level */
  if (event->state & GDK_CONTROL_MASK) {
    return FALSE;
  }

  if (vx->selectiontype & VT_SELTYPE_BYSTART) {
    vx->selendx = x;
    vx->selendy = y;
  } else {
    vx->selstartx = x;
    vx->selstarty = y;
  }

  switch(event->button) {
  case 1:
  case 3:
    d(printf("select from (%d,%d) to (%d,%d)\n", vx->selstartx, vx->selstarty,
	     vx->selendx, vx->selendy));

    gtk_grab_remove (widget);
    gdk_pointer_ungrab (0);

    if (vx->selectiontype & VT_SELTYPE_MOVED) {
      vt_fix_selection(vx);
      vt_draw_selection(vx);
          
      vt_get_selection(vx, 0);
      
      gtk_selection_owner_set (widget,
			       GDK_SELECTION_PRIMARY,
			       event->time);
    }

    vx->selectiontype = VT_SELTYPE_NONE; /* 'turn off' selecting */

  }

  return FALSE;
}

static void
zvt_term_scroll_by_lines (ZvtTerm *term, int n)
{
  gfloat new_value = 0;

  if (n > 0)
    new_value = MIN (term->adjustment->value + n, term->adjustment->upper - n);
  else if (n < 0)
    new_value = MAX (term->adjustment->value + n, term->adjustment->lower);
  else
    return;

  gtk_adjustment_set_value (term->adjustment, new_value);
}

static gint zvt_selectscroll(gpointer data)
{
  ZvtTerm *term;
  GtkWidget *widget;
  struct _zvtprivate *zp;

  widget = data;
  term = ZVT_TERM (widget);
  zp = gtk_object_get_data (GTK_OBJECT (term), "_zvtprivate");

  if (zp) {
    zvt_term_scroll_by_lines(term, zp->scrollselect_dir);
  }
  return TRUE;
}

/*
  mouse motion notify.
  only gets called for the first motion?  why?
*/

static gint
zvt_term_motion_notify (GtkWidget      *widget,
			GdkEventMotion *event)
{
  struct _vtx *vx;
  gint x, y;
  ZvtTerm *term;
  struct _zvtprivate *zp;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  vx = term->vx;
  zp = gtk_object_get_data (GTK_OBJECT (term), "_zvtprivate");

  if (vx->selectiontype != VT_SELTYPE_NONE){
    x=(((int)event->x))/term->charwidth;
    y=(((int)event->y))/term->charheight;
    
    /* move end of selection, and draw it ... */
    if (vx->selectiontype & VT_SELTYPE_BYSTART) {
      vx->selendx = x;
      vx->selendy = y + vx->vt.scrollbackoffset;
    } else {
      vx->selstartx = x;
      vx->selstarty = y + vx->vt.scrollbackoffset;
    }

    vx->selectiontype |= VT_SELTYPE_MOVED;
    
    vt_fix_selection(vx);
    vt_draw_selection(vx);

    if (zp) {
      if (zp->scrollselect_id!=-1) {
	gtk_timeout_remove(zp->scrollselect_id);
	zp->scrollselect_id = -1;
      }
      
      if (y<0 || y>vx->vt.height) {
	if (y<0)
	  zp->scrollselect_dir = -1;
	else
	  zp->scrollselect_dir = 1;
	zp->scrollselect_id = gtk_timeout_add(100, zvt_selectscroll, term);
      }
    }
  }
  /* otherwise, just a mouse event */
  /* make sure the pointer is visible! */
  zvt_term_show_pointer(term);

  return FALSE;
}

/*
 * zvt_term_selection_clear: [internal]
 * @term: A &ZvtTerm widget.
 * @event: Event that triggered the selection claim.
 *
 * Called when another application claims the selection.
 */
gint
zvt_term_selection_clear (GtkWidget *widget, GdkEventSelection *event)
{
  struct _vtx *vx;
  ZvtTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  /* Let the selection handling code know that the selection
   * has been changed, since we've overriden the default handler */
  if (!gtk_selection_clear (widget, event))
    return FALSE;

  term = ZVT_TERM (widget);
  vx = term->vx;

  vtx_unrender_selection(vx);
  vt_clear_selection(vx);
  return TRUE;
}

/* supply the current selection to the caller */
static void
zvt_term_selection_get (GtkWidget        *widget, 
			GtkSelectionData *selection_data_ptr,
			guint             info,
			guint             time)
{
  struct _vtx *vx;
  ZvtTerm *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));
  g_return_if_fail (selection_data_ptr != NULL);

  term = ZVT_TERM (widget);
  vx = term->vx;

  gtk_selection_data_set (selection_data_ptr, GDK_SELECTION_TYPE_STRING,
			  8, vx->selection_data, vx->selection_size);
}

/* receive a selection */
/* Signal handler called when the selections owner returns the data */
static void
zvt_term_selection_received (GtkWidget *widget, GtkSelectionData *selection_data, 
			     guint time)
{
  struct _vtx *vx;
  ZvtTerm *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));
  g_return_if_fail (selection_data != NULL);

  term = ZVT_TERM (widget);
  vx = term->vx;

  d(printf("got selection from system\n"));

  /* **** IMPORTANT **** Check to see if retrieval succeeded  */
  if (selection_data->length < 0)
    {
      g_print ("Selection retrieval failed\n");
      return;
    }

  /* Make sure we got the data in the expected form */
  if (selection_data->type != GDK_SELECTION_TYPE_STRING)
    {
      g_print ("Selection \"STRING\" was not returned as strings!\n");
      return;
    }

  /* paste selection into window! */
  if (selection_data->length)
    {
      int i;
      char *ctmp = selection_data->data;

      for(i = 0; i < selection_data->length; i++)
	if(ctmp[i] == '\n') ctmp[i] = '\r';

      if (term->scroll_on_keystroke)
	zvt_term_scroll (term, 0);
      vt_writechild(&vx->vt, selection_data->data, selection_data->length);
    }
}  


static gint
zvt_term_cursor_blink(gpointer data)
{
  ZvtTerm *term;
  GtkWidget *widget;

  widget = data;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);

  term = ZVT_TERM (widget);

  term->cursor_blink_state ^= 1;
  vt_cursor_state(data, term->cursor_blink_state);

  return TRUE;
}

/*
 * Callback for when the adjustment changes - i.e., the scrollbar
 * moves.
 */
static void 
zvt_term_scrollbar_moved (GtkAdjustment *adj, GtkWidget *widget)
{
  int line;
  ZvtTerm *term;
  struct _vtx *vx;
  struct _zvtprivate *zp;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  term = ZVT_TERM (widget);
  vx = term->vx;
  zp = gtk_object_get_data (GTK_OBJECT (term), "_zvtprivate");

  line = term->vx->vt.scrollbacklines - (int)adj->value;

  /* needed for floating point errors in slider code */
  if (line < 0)
    line = 0;

  term->vx->vt.scrollbackoffset = -line;

  d(printf("zvt_term_scrollbar_moved adj->value=%f\n", adj->value));

  /* will redraw if scrollbar moved */
  vt_update (term->vx, UPDATE_SCROLLBACK);

  /* for scrolling selection */
  if (zp && zp->scrollselect_id != -1) {
    int x,y;

    if (zp->scrollselect_dir>0) {
      x = vx->vt.width-1;
      y = vx->vt.height-1;
    } else {
      x = 0;
      y = 0;
    }

    if (vx->selectiontype & VT_SELTYPE_BYSTART) {
      vx->selendx = x;
      vx->selendy = y + vx->vt.scrollbackoffset;
    } else {
      vx->selstartx = x;
      vx->selstarty = y + vx->vt.scrollbackoffset;
    }

    vt_fix_selection(vx);
    vt_draw_selection(vx);
  }
}


/**
 * zvt_term_forkpty:
 * @term: A &ZvtTerm widget.
 * @do_uwtmp_log: If %TRUE, then log the session in wtmp(4) and utmp(4).
 *
 * Fork a child process, with a master controlling terminal.
 */
int
zvt_term_forkpty (ZvtTerm *term, int do_uwtmp_log)
{
  int pid;

  g_return_val_if_fail (term != NULL, -1);
  g_return_val_if_fail (ZVT_IS_TERM (term), -1);

  /* cannot fork twice! */
  if (term->input_id != -1)
    {
      return -1;
    }

  pid = vt_forkpty (&term->vx->vt, do_uwtmp_log);
  if (pid > 0) 
    {
      term->input_id = 
	gdk_input_add(term->vx->vt.childfd, GDK_INPUT_READ, zvt_term_readdata, term);
      term->msg_id =
	gdk_input_add(term->vx->vt.msgfd, GDK_INPUT_READ, zvt_term_readmsg, term);
    }

  return pid;
}

/**
 * zvt_term_killchild:
 * @term: A &ZvtTerm widget.
 * @signal: A signal number.
 *
 * Send the signal @signal to the child process.  Note that a child
 * process must have first been started using zvt_term_forkpty().
 *
 * Return Value: See kill(2).
 * See Also: signal(5).
 */
int
zvt_term_killchild (ZvtTerm *term, int signal)
{
  g_return_val_if_fail (term != NULL, -1);
  g_return_val_if_fail (ZVT_IS_TERM (term), -1);

  return vt_killchild (&term->vx->vt, signal);
}

/**
 * zvt_term_closepty:
 * @term: A &ZvtTerm widget.
 *
 * Close master pty to the child process.  It is upto the child to
 * recognise its pty has been closed, and to exit appropriately.
 *
 * Note that a child process must have first been started using
 * zvt_term_forkpty().
 *
 * Return Value: See close(2).
 */
int
zvt_term_closepty (ZvtTerm *term)
{
  g_return_val_if_fail (term != NULL, -1);
  g_return_val_if_fail (ZVT_IS_TERM (term), -1);

  if (term->input_id != -1)
    {
      gdk_input_remove (term->input_id);
      term->input_id = -1;
    }

  if (term->msg_id != -1) 
    {
      gdk_input_remove (term->msg_id);
      term->msg_id = -1;
    }

  return vt_closepty (&term->vx->vt);
}

static void
zvt_term_scroll (ZvtTerm *term, int n)
{
  gfloat new_value = 0;
  
  if (n)
    {
      new_value = term->adjustment->value + (n * term->adjustment->page_size);
    }
  else if (new_value == (term->adjustment->upper - term->adjustment->page_size))
    {
      return;
    }
  else
    {
      new_value = term->adjustment->upper - term->adjustment->page_size;
    }

  gtk_adjustment_set_value (
      term->adjustment,
      n > 0 ? MIN(new_value, term->adjustment->upper- term->adjustment->page_size) :
      MAX(new_value, term->adjustment->lower));
}

/*
 * Keyboard input callback
 */
static gint
zvt_term_key_press (GtkWidget *widget, GdkEventKey *event)
{
  char buffer[64];
  char *p=buffer;
  struct _vtx *vx;
  ZvtTerm *term;
  int handled;
  char *cursor;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  vx = term->vx;

  zvt_term_hide_pointer(term);
  
  d(printf("keyval = %04x state = %x\n", event->keyval, event->state));
  handled = TRUE;
  switch (event->keyval) {
  case GDK_BackSpace:
    if (event->state & GDK_MOD1_MASK)
      *p++ = '\033';

    if (term->swap_del_key)
      *p++ = '\177';
    else
      *p++ = 8;
    break;
  case GDK_KP_Right:
  case GDK_Right:
    cursor ="C";
    goto do_cursor;
  case GDK_KP_Left:
  case GDK_Left:
    cursor = "D";
    goto do_cursor;
  case GDK_KP_Up:
  case GDK_Up:
    cursor = "A";
    goto do_cursor;
  case GDK_KP_Down:
  case GDK_Down:
    cursor = "B";
    do_cursor:
    if (vx->vt.mode & VTMODE_APP_CURSOR)
      p+=sprintf (p, "\033O%s", cursor);
    else
      p+=sprintf (p, "\033[%s", cursor);
    break;
  case GDK_KP_Insert:
  case GDK_Insert:
    if ((event->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK){
        request_paste (widget);
    } else {
      p+=sprintf (p, "\033[2~");
    }
    break;
  case GDK_Delete:
    if (event->state & GDK_MOD1_MASK)
      *p++ = '\033';
    if (term->swap_del_key)
      *p++ = 8;
    else
      *p++ = '\177';
    break;
  case GDK_KP_Delete:
    p+=sprintf (p, "\033[3~");
    break;
  case GDK_KP_Home:
  case GDK_Home:
    p+=sprintf (p, "\033[1~");
    break;
  case GDK_KP_End:
  case GDK_End:
    p+=sprintf (p, "\033[4~");
    break;
  case GDK_KP_Page_Up:
  case GDK_Page_Up:
    if (event->state & GDK_SHIFT_MASK){
      zvt_term_scroll (term, -1);
    } else
      p+=sprintf (p, "\033[5~");
    break;
  case GDK_KP_Page_Down:
  case GDK_Page_Down:
    if (event->state & GDK_SHIFT_MASK){
      zvt_term_scroll (term, 1);
    } else
      p+=sprintf (p, "\033[6~");
    break;

    /*
        key_enter=\EOM, key_f1=\E[11~, key_f10=\E[21~, 
        key_f11=\E[23~, key_f12=\E[24~, key_f2=\E[12~, 
        key_f3=\E[13~, key_f4=\E[14~, key_f5=\E[15~, 
        key_f6=\E[17~, key_f7=\E[18~, key_f8=\E[19~, 
        key_f9=\E[20~, key_home=\EO\200, key_ic=\E[2~, 
        key_left=\EOD, key_mouse=\E[M, key_npage=\E[6~, 
    */

  case GDK_KP_F1:
  case GDK_F1:
    p+=sprintf (p, "\033OP");
    break;
  case GDK_KP_F2:
  case GDK_F2:
    p+=sprintf (p, "\033OQ");
    break;
  case GDK_KP_F3:
  case GDK_F3:
    p+=sprintf (p, "\033OR");
    break;
  case GDK_KP_F4:
  case GDK_F4:
    p+=sprintf (p, "\033OS");
    break;
  case GDK_F5:
    p+=sprintf (p, "\033[15~");
    break;
  case GDK_F6:
    p+=sprintf (p, "\033[17~");
    break;
  case GDK_F7:
    p+=sprintf (p, "\033[18~");
    break;
  case GDK_F8:
    p+=sprintf (p, "\033[19~");
    break;
  case GDK_F9:
    p+=sprintf (p, "\033[20~");
    break;
  case GDK_F10:
    p+=sprintf (p, "\033[21~");
    break;
  case GDK_F11:
    p+=sprintf (p, "\033[23~");
    break;
  case GDK_F12:
    p+=sprintf (p, "\033[24~");
    break;
  case GDK_KP_Begin:
    /* ? middle key of keypad */
  case GDK_Print:
  case GDK_Scroll_Lock:
  case GDK_Pause:
    /* control keys */
  case GDK_Shift_Lock:
  case GDK_Num_Lock:
  case GDK_Caps_Lock:
    /* ignore - for now FIXME: do something here*/
    break;
  case GDK_Control_L:
  case GDK_Control_R:
    break;
  case GDK_Shift_L:
  case GDK_Shift_R:
    break;
  case GDK_Alt_L:
  case GDK_Alt_R:
  case GDK_Meta_L:
  case GDK_Meta_R:
    break;
  case GDK_Mode_switch:
  case GDK_Multi_key:
    break;
  case ' ':
    /* maps single characters to correct control and alt versions */
    if (event->state & GDK_CONTROL_MASK)
      *p++=event->keyval & 0x1f;
    else if (event->state & GDK_MOD1_MASK)
      *p++=event->keyval + 0x80; /* this works for space at least */
    else
      *p++=event->keyval;
    break;
  default:
      if (event->length > 0){
	if (event->state & GDK_MOD1_MASK){
	   *p++ = '\033';
        }
	memcpy(p, event->string, event->length*sizeof(char));
	p += event->length;
      } else {
	handled = FALSE;
      }
      d(printf ("[%s,%d,%d]\n", event->string, event->length, handled));
  }
  if (handled && p>buffer) {
    vt_writechild(&vx->vt, buffer, (p-buffer));
    if (term->scroll_on_keystroke) zvt_term_scroll (term, 0);
  }

  return handled;
}

/* dummy default signal handler */
static void
zvt_term_child_died (ZvtTerm *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));

  /* perhaps we should do something here? */
}

/* dummy default signal handler for title_changed */
static void
zvt_term_title_changed (ZvtTerm *term, VTTITLE_TYPE type, char *str)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));

  /* perhaps we should do something here? */
}

/* raise the title_changed signal */
static void
zvt_term_title_changed_raise (void *user_data, VTTITLE_TYPE type, char *str)
{
  ZvtTerm *term = user_data;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));

  gtk_signal_emit(GTK_OBJECT(term), term_signals[TITLE_CHANGED], type, str);
}

static void
vtx_unrender_selection (struct _vtx *vx)
{
  /* need to 'un-render' selected area */
  if (vx->selected)
    {
      vx->selstartx = vx->selendx;
      vx->selstarty = vx->selendy;
      vt_draw_selection(vx);	/* un-render selection */
      vx->selected = 0;
    }
}

/*
 * this callback is called when data is ready on the child's file descriptor
 *
 * Read all data waiting on the file descriptor, updating the virtual
 * terminal buffer, until there is no more data to read, and then render it.
 *
 * NOTE: this may impact on slow machines, but it may not also ...!
 */
static void
zvt_term_readdata (gpointer data, gint fd, GdkInputCondition condition)
{
  gboolean update;
  gchar buffer[4096];
  gint count, saveerrno;
  struct _vtx *vx;
  ZvtTerm *term;

  d( printf("zvt_term_readdata\n") );

  term = (ZvtTerm *) data;

  if (term->input_id == -1)
    return;

  update = FALSE;
  vx = term->vx;
  vtx_unrender_selection (vx);
  saveerrno = EAGAIN;

  vt_cursor_state (term, 0);
  while ( (saveerrno == EAGAIN) && (count = read (fd, buffer, 4096)) > 0) 
    {
      update = TRUE;
      saveerrno = errno;
      vt_parse_vt (&vx->vt, buffer, count);
    }

  if (update)
    {
      if (GTK_WIDGET_DRAWABLE (term))
	{
	  d( printf("zvt_term_readdata: update from read\n") );
	  vt_update (vx, UPDATE_CHANGES);
	}
      else
	{
	  d( printf("zvt_term_readdata: suspending update -- not drawable\n") );
	}
    } 
  else 
    {
      saveerrno = errno;
    }

  /* *always* turn the cursor back on */
  vt_cursor_state (term, 1);
  
  /* fix scroll bar */
  if (term->scroll_on_output)
    {
      zvt_term_scroll (term, 0);
    }

  /* flush all X events - this is really necessary to stop X queuing up
   * lots of screen updates and reducing interactivity on a busy terminal
   */
  gdk_flush ();

  /* read failed? oh well, that's life -- we handle dead children via
   * SIGCHLD
   */
  zvt_term_fix_scrollbar (term);
}

/**
 * zvt_term_feed:
 * @term: A &ZvtTerm widget.
 * @text: The text to feed.
 * @len:  The text length.
 *
 * This makes the terminal emulator process the stream of
 * characters in @text for @len bytes.  The text is interpreted
 * by the terminal emulator as if it were generated by a child
 * process.
 *
 * This is used by code that needs a terminal emulator, but
 * does not use a child process.
 */
void
zvt_term_feed (ZvtTerm *term, char *text, int len)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));
  g_return_if_fail (text != NULL);
  
  vt_cursor_state (term, 0);
  vtx_unrender_selection (term->vx);
  vt_parse_vt (&term->vx->vt, text, len);
  vt_update (term->vx, UPDATE_CHANGES);
  
  /* *always* turn the cursor back on */
  vt_cursor_state (term, 1);
  
  /* fix scroll bar */
  if (term->scroll_on_output)
    zvt_term_scroll (term, 0);
  
  /* flush all X events - this is really necessary to stop X queuing up
   * lots of screen updates and reducing interactivity on a busy terminal
   */
  gdk_flush ();
  
  /* read failed? oh well, that's life -- we handle dead children via
   *  SIGCHLD
   */
  zvt_term_fix_scrollbar (term);
}

static void
zvt_term_readmsg (gpointer data, gint fd, GdkInputCondition condition)
{
  ZvtTerm *term = (ZvtTerm *)data;

  /* I suppose I should bother reading the message from the fd, but
   * it doesn't seem worth the trouble <shrug>
   */
  if (term->input_id != -1)
    {
      gdk_input_remove (term->input_id);
      term->input_id=-1;
    }

  zvt_term_closepty (term);

  /* signal application FIXME: include error/non error code */
  gtk_signal_emit (GTK_OBJECT(term), term_signals[CHILD_DIED]);
}

/**
 * zvt_term_set_background:
 * @terminal: A &ZvtTerm widget.
 * @pixmap_file: file containing the pixmap image
 * @transparent: true if we want to run in transparent mode
 * @shaded:      if true, we want to run the terminal in shade mode
 */
void
zvt_term_set_background (ZvtTerm *terminal, char *pixmap_file, 
			 int transparent, int shaded)
{
  if (!(zvt_term_get_capabilities (terminal) & ZVT_TERM_PIXMAP_SUPPORT))  
    return; 

  /* if there no changes return */
  if(shaded == terminal->shaded &&
     safe_strcmp(pixmap_file,terminal->pixmap_filename)==0 &&
     transparent == terminal->transparent)
    return;
  
  if (terminal->background.pix)
    {
      gdk_pixmap_unref(terminal->background.pix);
      terminal->background.pix = NULL;
    }

  terminal->transparent = transparent;
  terminal->shaded = shaded;
  g_free (terminal->pixmap_filename);
  
  if (pixmap_file)
    terminal->pixmap_filename = g_strdup (pixmap_file);
  else
    terminal->pixmap_filename = NULL;

  gtk_widget_queue_draw(GTK_WIDGET(terminal));
}

/*
 * external rendering functions called by vt_update, etc
 */
int
vt_cursor_state(void *user_data, int state)
{
  ZvtTerm *term;
  GtkWidget *widget;
  int old_state;

  widget = user_data;

  g_return_val_if_fail (widget != NULL, 0);
  g_return_val_if_fail (ZVT_IS_TERM (widget), 0);

  term = ZVT_TERM (widget);
  old_state = term->cursor_on;

  /* only call vt_draw_cursor if the state has changed */
  if (old_state ^ state)
    {
      if (GTK_WIDGET_DRAWABLE (widget))
	{
	  vt_draw_cursor(term->vx, state);
	  term->cursor_on = state;
	}
    }
  return old_state;
}

void
vt_draw_text(void *user_data, int col, int row, char *text, int len, int attr)
{
  GdkFont *f;
  struct _vtx *vx;
  ZvtTerm *term;
  GtkWidget *widget;
  int fore, back, or;
  GdkColor pen;
  GdkGC *fgc, *bgc;
  
  widget = user_data;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  if (!GTK_WIDGET_DRAWABLE (widget))
    {
      return;
    }

  term = ZVT_TERM (widget);

  vx = term->vx;

  if (attr & VTATTR_BOLD) 
    {
      or = 8;
      f = term->font_bold;
    } 
  else 
    {
      or = 0;
      f = term->font;
    }

  fore = (attr & VTATTR_FORECOLOURM) >> VTATTR_FORECOLOURB;
  back = (attr & VTATTR_BACKCOLOURM) >> VTATTR_BACKCOLOURB;

  if (fore < 8)
    fore |= or;

  /* set the right colour in the appropriate gc */
  fgc = term->fore_gc;
  bgc = term->back_gc;

  if (term->back_last != back)
    {
      pen.pixel = term->colors [back];
      gdk_gc_set_foreground (bgc, &pen);
      term->back_last = back;
    }
  
  if (term->fore_last != fore)
    {
      pen.pixel = term->colors [fore];
      gdk_gc_set_foreground (fgc, &pen);
      term->fore_last = fore;
    }

  /* for reverse, swap gc's */
  if (attr & VTATTR_REVERSE)
    {
      GdkGC *tmp = fgc;
      int tmp2 = fore;
      
      fgc = bgc;
      bgc = tmp;
      
      fore = back;
      back = tmp2;
    }

  /* optimise: dont 'clear' background if not in 
   * expose, and background colour == window colour
   * this may look a bit weird, it really does need to 
   * be this way - so dont touch!
   *
   * if the terminal is transparent we must redraw 
   * all the time as we can't optimize in that case
   */
  if (term->in_expose || vx->back_match == 0)
    {
      if ((!term->transparent && !term->pixmap_filename) || back < 17)
	{
	  gdk_draw_rectangle (
              term->term_window,
	      bgc, 1,
	      col * term->charwidth,
	      row * term->charheight,
	      len * term->charwidth,
	      term->charheight);
	  
	} 
      else if (!term->in_expose) 
	{
	  draw_back_pixmap (
              widget,
	      col * term->charwidth,
	      row * term->charheight,
	      len * term->charwidth,
	      term->charheight);
	}
    } 
  else 
    {
      d(printf("not clearing background in_expose = %d, back_match=%d\n", 
	       term->in_expose, vx->back_match));
      d(printf("txt = '%.*s'\n", len, text));
    }
  
  gdk_draw_text(
      term->term_window, f, fgc,
      col * term->charwidth,
      row * term->charheight + term->font->ascent,
      text, len);

  /* check for underline */
  if (attr&VTATTR_UNDERLINE)
    {
      gdk_draw_line(
          term->term_window, fgc,
	  col * term->charwidth,
	  row * term->charheight + term->font->ascent + 1,
	  (col + len) * term->charwidth,
	  row * term->charheight + term->font->ascent + 1);
    }
}





void
vt_scroll_area(void *user_data, int firstrow, int count, int offset, int fill)
{
  int width, off;
  ZvtTerm *term;
  GtkWidget *widget;
  
  widget = user_data;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  if (!GTK_WIDGET_DRAWABLE (widget))
    {
      return;
    }

  term = ZVT_TERM (widget);

  if(term->transparent || term->pixmap_filename)
    {
      /* FIXME: bad hack */ /* tell me about it... --JMP */
      gtk_widget_queue_draw(widget);
      return;
    }

  /* FIXME: check args */
  d(printf("scrolling %d rows from %d, by %d lines\n", count,firstrow,offset));

  width = term->charwidth * term->vx->vt.width;
  off = widget->style->klass->xthickness - PADDING;

  /* "scroll" area */
  gdk_draw_pixmap(term->term_window,
		  term->scroll_gc, /* must use this to generate expose events */
		  term->term_window,
		  off, (firstrow+offset)*term->charheight,
		  off, firstrow*term->charheight,
		  width, count*term->charheight);

  /* clear the other part of the screen */
  /* check fill colour is right */
  if (term->back_last != fill) 
    {
      GdkColor pen;

      pen.pixel = term->colors[fill];
      gdk_gc_set_foreground (term->back_gc, &pen);
      term->back_last = fill;
    }

  if (offset > 0) 
    {
      gdk_draw_rectangle(term->term_window,
  		         term->back_gc,
		         1,
		         0, (firstrow+count)*term->charheight,
		         width, offset*term->charheight);
    } 
  else 
    {
      gdk_draw_rectangle(term->term_window,
		         term->back_gc,
		         1,
		         0, (firstrow+offset)*term->charheight,
		         width, (-offset)*term->charheight);
    }
}

/**
 * zvt_term_set_del_key_swap:
 * @term:   A &ZvtTerm widget.
 * @state:  If true it swaps the del/backspace definitions
 * 
 * Sets the mode for interpreting the DEL and Backspace keys.
 **/
void
zvt_term_set_del_key_swap (ZvtTerm *term, int state)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));
  
  term->swap_del_key = state != 0;
}

/*
 * zvt_term_bell:
 * @term: Terminal.
 *
 * Generate a terminal bell.  Currently this is just a beep.
 **/
void
zvt_term_bell(void *user_data)
{
  gdk_beep();
}


/**
 * zvt_term_set_bell:
 * @term: A &ZvtTerm widget.
 * @state: New bell state.
 * 
 * Enable or disable the terminal bell.  If @state is %TRUE, then the
 * bell is enabled.
 **/
void
zvt_term_set_bell(ZvtTerm *term, int state)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));

  if (state)
    term->vx->vt.ring_my_bell = zvt_term_bell;
  else
    term->vx->vt.ring_my_bell = 0;
}

/**
 * zvt_term_get_bell:
 * @term: A &ZvtTerm widget.
 * 
 * get the terminal bell state.  If the bell on then %TRUE is
 * returned, otherwise %FALSE.
 **/
gboolean
zvt_term_get_bell(ZvtTerm *term)
{
  g_return_val_if_fail (term != NULL, 0);
  g_return_val_if_fail (ZVT_IS_TERM (term), 0);

  return (term->vx->vt.ring_my_bell)?TRUE:FALSE;
}

void
zvt_term_set_shadow_type(ZvtTerm  *term, GtkShadowType type)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));

  term->shadow_type = type;

  if (GTK_WIDGET_VISIBLE (term))
    gtk_widget_queue_resize (GTK_WIDGET (term));
}

/**
 * zvt_term_get_capabilities:
 * @term: A &ZvtTerm widget.
 * 
 * Description: Gets the compiled in capabilities of the terminal widget, for
 * now this is only pixmap support which %ZVT_TERM_PIXMAP_SUPPORT
 *
 * Returns: a bitmask of the capabilities
 **/
guint32
zvt_term_get_capabilities (ZvtTerm *term)
{
  guint32 out = 0;

  /* pixmap and transparency support */
  if (gdk_imlib_get_visual () == gtk_widget_get_default_visual ())
    out |= ZVT_TERM_PIXMAP_SUPPORT;

  return out;
}

/**
 * zvt_term_get_buffer:
 * @term: Valid &ZvtTerm widget.
 * @len: Placeholder to store the length of text selected.  May be
 *       %NULL in which case the value is not returned.
 * @type: Type of selection.  %VT_SELTYPE_LINE, select by line,
 *       %VT_SELTYPE_WORD, select by word, or %VT_SELTYPE_CHAR, select
 *       by character.
 * @sx: Start of selection, horizontal.
 * @sy: Start of selection, vertical.  0 is the top of the visible
 *      screen, <0 is scrollback lines, >0 is visible lines (upto the
 *      height of the window).
 * @ex: End of selection, horizontal.
 * @ey: End of selection, vertical, as above.
 * 
 * Convert the buffer memory into a contiguous array which may be
 * saved or processed.  Note that this is not gauranteed to match the
 * order of characters processed by the terminal, only the order in
 * which they were displayed.  Tabs will normally be preserved in
 * the output.
 *
 * All inputs are range-checked first, so it is possible to fudge
 * a full buffer grab.
 *
 * Examples:
 *  data = zvt_term_get_buffer(term, NULL, VT_SELTYPE_LINE,
 *       -term->vx->vt.scrollbackmax, 0,
 *        term->vx->vt.height, 0);
 *  or, as a rule -
 *  data = zvt_term_get_buffer(term, NULL, VT_SELTYPE_LINE,
 *       -10000, 0, 10000, 0);
 *
 * Will return the contents of the entire scrollback and on-screen
 * buffers, remembering that all inputs are range-checked first.
 *
 *  data = zvt_term_get_buffer(term, NULL, VT_SELTYPE_CHAR,
 *       0, 0, 5, 10);
 *
 * Will return the first 5 lines of the visible screen, and the 6th
 * line upto column 10.
 * 
 * Return value: A pointer to a %NUL terminated buffer containing the
 * raw text from the buffer.  If memory could not be allocated, then
 * returns %NULL.  Note that it is upto the caller to free the memory,
 * using g_free(3c).  If @len was supplied, then the length of data is
 * stored there.
 **/
char *
zvt_term_get_buffer(ZvtTerm *term, int *len, int type, int sx, int sy, int ex, int ey)
{
  struct _vtx *vx;
  int ssx, ssy, sex, sey, stype, slen;
  char *sdata;
  char *data;

  g_return_val_if_fail (term != NULL, 0);
  g_return_val_if_fail (ZVT_IS_TERM (term), 0);

  vx = term->vx;

  /* this is a bit messy - we save the current selection state,
   * override it, 'select' the new text, then restore the old
   *  selection state but return the new ...
   *  api should be more separated.  vt_get_block() is a start on this.
   */

  ssx = vx->selstartx;
  ssy = vx->selstarty;
  sex = vx->selendx;
  sey = vx->selendy;
  sdata = vx->selection_data;
  slen = vx->selection_size;
  stype = vx->selectiontype;

  vx->selstartx = sx;
  vx->selstarty = sy;
  vx->selendx = ex;
  vx->selendy = ey;
  vx->selection_data = 0;
  vx->selectiontype = type & VT_SELTYPE_MASK;

  vt_fix_selection(vx);

  data = vt_get_selection(vx, len);
  
  vx->selstartx = ssx;
  vx->selstarty = ssy;
  vx->selendx = sex;
  vx->selendy = sey;
  vx->selection_data = sdata;
  vx->selection_size = slen;
  vx->selectiontype = stype;

  return data;
}

/******************************/
/**** TRANSPARENT TERMINAL ****/


/* kind of stolen from Eterm and needs heavy cleanup */
static Window desktop_window = None;

static Window
get_desktop_window (Window the_window)
{
  Atom prop, type, prop2;
  int format;
  unsigned long length, after;
  unsigned char *data;
  unsigned int nchildren;
  Window w, root, *children, parent;
  Window last_desktop_window = desktop_window;
  
  prop = XInternAtom(GDK_DISPLAY(), "_XROOTPMAP_ID", True);
  prop2 = XInternAtom(GDK_DISPLAY(), "_XROOTCOLOR_PIXEL", True);
  
  if (prop == None && prop2 == None)
    {
      return None;
    }
  
#ifdef WATCH_DESKTOP_OPTION
  if (Options & Opt_watchDesktop) 
    {
      if (TermWin.wm_parent != None)
	{
	  XSelectInput(GDK_DISPLAY(), TermWin.wm_parent, None);
	}

      if (TermWin.wm_grandparent != None)
	{
	  XSelectInput(GDK_DISPLAY(), TermWin.wm_grandparent, None);
	}
    }
#endif
  
  for (w = the_window; w; w = parent) 
    {
      if ((XQueryTree(GDK_DISPLAY(), w, &root, &parent, &children, &nchildren)) == False) 
	{
	  return None;
	}
      
      if (nchildren) 
	{
	  XFree(children);
	}

#ifdef WATCH_DESKTOP_OPTION
    if (Options & Opt_watchDesktop) 
      {
	if (w == TermWin.parent) 
	  {
	    TermWin.wm_parent = parent;
	    XSelectInput(GDK_DISPLAY(), TermWin.wm_parent, StructureNotifyMask);
	  } 
	else if (w == TermWin.wm_parent)
	  {
	    TermWin.wm_grandparent = parent;
	    XSelectInput(GDK_DISPLAY(), TermWin.wm_grandparent, StructureNotifyMask);
	  }
      }
#endif
    
    if (prop != None) 
      {
	XGetWindowProperty(GDK_DISPLAY(), w, prop, 0L, 1L, False, AnyPropertyType,
			   &type, &format, &length, &after, &data);
      }
    else if (prop2 != None) 
      {
	XGetWindowProperty(GDK_DISPLAY(), w, prop2, 0L, 1L, False, AnyPropertyType,
			   &type, &format, &length, &after, &data);
      } 
    else 
      {
	continue;
      }

    if (type != None) 
      {
	return (desktop_window = w);
      }
    }
  
  return (desktop_window = None);
}

static Pixmap
get_pixmap_prop (Window the_window, char *prop_id)
{
  Atom prop, type;
  int format;
  unsigned long length, after;
  unsigned char *data;
  register unsigned long i=0;
  
  /*this should be changed when desktop changes I guess*/
  if(desktop_window == None)
    desktop_window = get_desktop_window(the_window);
  if(desktop_window == None)
    desktop_window = GDK_ROOT_WINDOW();
  
  prop = XInternAtom(GDK_DISPLAY(), prop_id, True);
  
  if (prop == None)
    return None;
  
  XGetWindowProperty(GDK_DISPLAY(), desktop_window, prop, 0L, 1L, False,
		     AnyPropertyType, &type, &format, &length, &after,
		     &data);

  if (type == XA_PIXMAP)
    return *((Pixmap *)data);

  return None;
}

static GdkPixmap *
load_pixmap_back(char *file, int shaded)
{
  GdkPixmap *pix;
  GdkImlibColorModifier mod;
  GdkImlibImage *iim;
  
  if(!file)
    return NULL;
  
  iim = gdk_imlib_load_image(file);
  if(!iim)
    return NULL;
  mod.contrast=256;
  mod.gamma=256;
  if(shaded)
    mod.brightness=190;
  else
    mod.brightness=256;
  
  gdk_imlib_set_image_modifier(iim,&mod);
  gdk_imlib_render(iim, iim->rgb_width,iim->rgb_height);
  pix = gdk_imlib_move_image(iim);
  gdk_imlib_destroy_image(iim);
  
  return pix;
}

static GdkPixmap *
create_shaded_pixmap (Pixmap p, int x, int y, int w, int h)
{
  GdkPixmap *pix;
  GdkImlibColorModifier mod;
  GdkImlibImage *iim;
  GdkWindowPrivate pw;
  
  if (p == None)
    return NULL;
  
  pw.xwindow = (Window) p;
  iim = gdk_imlib_create_image_from_drawable(
	    (GdkPixmap*) &pw, NULL, x, y, w, h);

  if(!iim)
    return NULL;

  mod.contrast = 256;
  mod.brightness = 190;
  mod.gamma = 256;
  gdk_imlib_set_image_modifier (iim, &mod);
  gdk_imlib_render (iim, iim->rgb_width, iim->rgb_height);
  pix = gdk_imlib_move_image (iim);
  gdk_imlib_destroy_image (iim);
  
  return pix;
}


static void
draw_back_pixmap (GtkWidget *widget, int nx, int ny, int nw, int nh)
{
  int x,y;
  Window childret;
  ZvtTerm *term;
  GdkGC *bgc;
  Pixmap p;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  if (!GTK_WIDGET_DRAWABLE (widget))
    {
      return;
    }

  term = ZVT_TERM (widget);
  bgc = term->back_gc;
  
  /* we will be doing a background pixmap, not transparency */
  if (term->pixmap_filename)
    {
      GdkGCValues vals;

    if (!term->background.pix) 
      {
	term->background.pix =
	  load_pixmap_back (term->pixmap_filename, term->shaded);
      }

    if (!term->background.pix && !term->transparent)
      {
	g_free (term->pixmap_filename);
	term->pixmap_filename = NULL;
	return;
      } 
    else if (term->background.pix)
      {
	gdk_gc_get_values (bgc, &vals);
	gdk_gc_set_tile (bgc, term->background.pix);
	gdk_gc_set_fill (bgc, GDK_TILED);
	gdk_draw_rectangle (term->term_window, bgc, TRUE, nx, ny, nw, nh);
	gdk_gc_set_fill (bgc, vals.fill);
	return;
      }
 
    /* if we can't load a pixmap and transparency is set,
     * default to the transparency setting
     */
    }
  
  p = get_pixmap_prop (GDK_WINDOW_XWINDOW(term->term_window), "_XROOTPMAP_ID");
  if (p == None) 
    {
      term->transparent = 0;
      return;
    }
  
  XTranslateCoordinates (
      GDK_WINDOW_XDISPLAY (term->term_window),
      GDK_WINDOW_XWINDOW (term->term_window),
      GDK_ROOT_WINDOW (),
      0, 0,
      &x, &y,
      &childret);
  
  if (term->shaded)
    {
      if (term->background.pix == NULL ||
	  term->background.x != x ||
	  term->background.y != y ||
	  term->background.w < nw+nx ||
	  term->background.h < nh+ny)
	{
	  int width,height;

	  gdk_window_get_size(term->term_window, &width, &height);

	  if (term->background.pix)
	    {
	      gdk_pixmap_unref(term->background.pix);
	    }
	  
	  term->background.pix = create_shaded_pixmap (p, x, y, width, height);
	  term->background.x = x;
	  term->background.y = y;
	  term->background.w = width;
	  term->background.h = height;
	}

      gdk_draw_pixmap (term->term_window,
		       bgc,
		       term->background.pix,
		       nx,ny,
		       nx,ny,
		       nw,nh);
    } 
  else 
    {
      /*non-shaded is simple*/

      XCopyArea (GDK_WINDOW_XDISPLAY(term->term_window),
		 p,
		 GDK_WINDOW_XWINDOW(term->term_window),
		 GDK_GC_XGC(bgc),
		 x+nx,y+ny,
		 nw,nh,
		 nx,ny);
    }
}

static gint
safe_strcmp(gchar *a, gchar *b)
{
  if (!a && !b)
    return 0;

  if (a && b)
    return strcmp(a,b);

  return 1;
}
