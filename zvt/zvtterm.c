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

#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "zvtterm.h"

/* define to 'x' to enable copious debug output */
#define d(x)

/* Forward declararations */
static void zvt_term_init (ZvtTerm *term);
static void zvt_term_class_init (ZvtTermClass *class);
static void zvt_term_init (ZvtTerm *term);
GtkWidget* zvt_term_new (void);
static void zvt_term_destroy (GtkObject *object);
static void zvt_term_realize (GtkWidget *widget);
static void zvt_term_unrealize (GtkWidget *widget);
static void zvt_term_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void zvt_term_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint zvt_term_expose (GtkWidget *widget, GdkEventExpose *event);
static void zvt_term_draw (GtkWidget *widget, GdkRectangle *area);
static gint zvt_term_button_press (GtkWidget *widget, GdkEventButton *event);
static gint zvt_term_button_release (GtkWidget *widget, GdkEventButton *event);
static gint zvt_term_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static gint zvt_term_key_press (GtkWidget *widget, GdkEventKey *event);
static gint zvt_term_focus_in(GtkWidget *widget, GdkEventFocus *event);
static gint zvt_term_focus_out(GtkWidget *widget, GdkEventFocus *event);

static void zvt_term_selection_received (GtkWidget *widget, GtkSelectionData *selection_data);
static gint zvt_term_selection_clear (GtkWidget *widget, GdkEventSelection *event);
static void zvt_term_selection_handler (GtkWidget *widget, GtkSelectionData *selection_data_ptr, gpointer data);

static void zvt_term_child_died(ZvtTerm *term);

static gint zvt_term_cursor_blink(gpointer data);
static void zvt_term_scrollbar_moved (GtkAdjustment *adj, GtkWidget *widget);
static void zvt_term_readdata(gpointer data, gint fd, GdkInputCondition condition);
static void zvt_term_readmsg(gpointer data, gint fd, GdkInputCondition condition);

static void zvt_term_fix_scrollbar(ZvtTerm *term);
static void vtx_unrender_selection (struct _vtx *vx);

/* Local data */

enum {
  CHILD_DIED,
  LAST_SIGNAL
};

static guint term_signals[LAST_SIGNAL] = { 0 };

static GtkWidgetClass *parent_class = NULL;

guint
zvt_term_get_type (void )
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

  gtk_object_class_add_signals (object_class, term_signals, LAST_SIGNAL);

  object_class->destroy = zvt_term_destroy;

  widget_class->realize = zvt_term_realize;
  widget_class->unrealize = zvt_term_unrealize;
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

  term_class->child_died = zvt_term_child_died;
}

static void
zvt_term_init (ZvtTerm *term)
{
  GTK_WIDGET_SET_FLAGS (term, GTK_CAN_FOCUS);

  term->vx = vtx_new(term);

  term->cursor_on = 0;
  term->cursor_filled = 1;
  term->cursor_blink_state = 0;
  term->scroll_on_keystroke = 0;
  term->scroll_on_output = 0;
  term->blink_enabled = 1;
  term->ic = NULL;
  term->in_expose = 0;

  /* input handlers */
  term->input_id = -1;
  term->msg_id = -1;

  zvt_term_set_font_name(term, "-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");

  /* scrollback position adjustment */
  term->adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 24.0, 1.0, 24.0, 24.0));

  gtk_signal_connect (GTK_OBJECT (term->adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (zvt_term_scrollbar_moved), term);

  /* selection received */
  gtk_selection_add_handler (GTK_WIDGET(term), GDK_SELECTION_PRIMARY,
			     GDK_SELECTION_TYPE_STRING,
			     zvt_term_selection_handler, term);
}

void
zvt_term_set_blink (ZvtTerm *term, int state)
{
  if (!(term->blink_enabled ^ state))
    return;
  if (term->blink_enabled){
    if (term->timeout_id != -1){
      gtk_timeout_remove (term->timeout_id);
      term->timeout_id = -1;
    }
    if (GTK_WIDGET_REALIZED (term))
      vt_cursor_state (GTK_WIDGET (term), 1);
    term->blink_enabled = 0;
  } else {
    term->timeout_id = gtk_timeout_add (500, zvt_term_cursor_blink, term);
    term->blink_enabled = 1;
  }
}

void 
zvt_term_set_scroll_on_keystroke(ZvtTerm *term, int state)
{
  term->scroll_on_keystroke = (state != 0);
}

void 
zvt_term_set_scroll_on_output   (ZvtTerm *term, int state)
{
  term->scroll_on_output = (state != 0);
}

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

  g_return_if_fail (object != NULL);
  g_return_if_fail (ZVT_IS_TERM (object));

  term = ZVT_TERM (object);
  gdk_cursor_destroy (term->cursor_bar);
  gdk_cursor_destroy (term->cursor_dot);

  zvt_term_closepty(term);
  vtx_destroy(term->vx);

  if (term->ic) {
    gdk_ic_destroy(term->ic);
    term->ic = NULL;
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* The first 16 values are the ansi colors, the last two are the default foreground
 * and default background
 */
static gushort default_red[] = {0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,0x0000,0xaaaa,
				0x5555,0xffff,0x5555,0xffff,0x5555,0xffff,0x5555,0xffff,
                                0xaaaa, 0x0000};
static gushort default_grn[] = {0x0000,0x0000,0xaaaa,0x5555,0x0000,0x0000,0xaaaa,0xaaaa,
				0x5555,0x5555,0xffff,0xffff,0x5555,0x5555,0xffff,0xffff,
				0xaaaa, 0x0000};

static gushort default_blu[] = {0x0000,0x0000,0x0000,0x0000,0xaaaa,0xaaaa,0xaaaa,0xaaaa,
				0x5555,0x5555,0x5555,0x5555,0xffff,0xffff,0xffff,0xffff,
				0xaaaa, 0x0000};

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
  gdk_color_context_get_pixels (term->color_ctx, red, grn, blu,
				18, term->colors, &nallocated);
  c.pixel = term->colors [17];
  gdk_window_set_background (GTK_WIDGET (term)->window, &c);
}

void
zvt_term_set_default_color_scheme (ZvtTerm *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));
  
  zvt_term_set_color_scheme (term, default_red, default_grn, default_blu);
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
  attributes.event_mask = gtk_widget_get_events (widget) | 
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_MOTION_MASK |
    GDK_POINTER_MOTION_MASK |
    GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
				   &attributes, attributes_mask);
  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);

  /* create pixmaps for this window */
  cursor_dot_pm=gdk_pixmap_create_from_data(widget->window,
					    "\0", 1, 1, 1,
					    &widget->style->fg[GTK_STATE_ACTIVE],
					    &widget->style->bg[GTK_STATE_ACTIVE]);

  /* Get I beam cursor, and also create a blank one based on the blank image */
  term->cursor_bar = gdk_cursor_new(GDK_XTERM);
  term->cursor_dot = gdk_cursor_new_from_pixmap(cursor_dot_pm, cursor_dot_pm,
					      &widget->style->fg[GTK_STATE_ACTIVE],
					      &widget->style->bg[GTK_STATE_ACTIVE],
					      0, 0);
  gdk_window_set_cursor(widget->window, term->cursor_bar);
  gdk_pixmap_unref (cursor_dot_pm);
  term->cursor_current = term->cursor_bar;

  /* setup blinking cursor */
  if (term->blink_enabled)
    term->timeout_id = gtk_timeout_add(500, zvt_term_cursor_blink, term);

  /* setup scrolling gc */
  term->scroll_gc = gdk_gc_new (GTK_WIDGET(term)->window);
  gdk_gc_set_exposures (term->scroll_gc, TRUE);

  /* Colors */
  term->fore_gc = gdk_gc_new (term->widget.window);
  term->back_gc = gdk_gc_new (term->widget.window);
  term->color_ctx = gdk_color_context_new (gtk_widget_get_visual (GTK_WIDGET (term)),
					   gtk_widget_get_colormap (GTK_WIDGET (term)));
  /* Allocate default color set */
  zvt_term_set_default_color_scheme (term);
  
  /* set the initial colours */
  term->back_last = -1;
  term->fore_last = -1;

  /* input context */
  if (gdk_im_ready() && !term->ic) {
    GdkIMStyle style = GDK_IM_PREEDIT_NOTHING | GDK_IM_STATUS_NOTHING;
    /* FIXME: do we have any window yet? */
    term->ic = gdk_ic_new(widget->window, widget->window, style, NULL);
    if (!term->ic) {
      g_warning("Can't create input context.");
    }
  }

  /* FIXME: not sure if this is right or not? */
  if (!GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);
}

static void
zvt_term_unrealize (GtkWidget *widget)
{
  ZvtTerm *term = ZVT_TERM (widget);
  /* free resources */
  gdk_color_context_free (term->color_ctx);
  gdk_gc_destroy (term->scroll_gc);
  gdk_gc_destroy (term->back_gc);
  gdk_gc_destroy (term->fore_gc);

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    (*GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static gint zvt_term_focus_in(GtkWidget *widget, GdkEventFocus *event)
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
    term->timeout_id = gtk_timeout_add(500, zvt_term_cursor_blink, term);

  if (term->ic)
    gdk_im_begin (term->ic, widget->window);

  return FALSE;
}
static gint zvt_term_focus_out(GtkWidget *widget, GdkEventFocus *event)
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
  if (term->blink_enabled && term->timeout_id != -1) {
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

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));
  g_return_if_fail (requisition != NULL);

  term = ZVT_TERM (widget);
  requisition->width = term->vx->vt.width * term->charwidth;	/* FIXME: base size on terminal size */
  requisition->height = term->vx->vt.height * term->charheight;

  d(printf("size request\n");
    printf("term is %dx%d\n", term->vx->vt.width, term->vx->vt.height));

}

static void
zvt_term_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  ZvtTerm *term;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      term = ZVT_TERM (widget);

      d(printf("size allocate\n");
	printf("term is %dx%d\n", term->vx->vt.width, term->vx->vt.height));
      
      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      /* resize the virtual terminal buffer, minimal size is 1x1 */
      vt_resize(&term->vx->vt,
		MAX(allocation->width/term->charwidth,1),
		MAX(allocation->height/term->charheight,1),
		allocation->width,
		allocation->height);
      vt_update(term->vx, UPDATE_REFRESH|UPDATE_SCROLLBACK);/* redraw everything, unconditionally */
      
      /* resize the scrollbar */
      zvt_term_fix_scrollbar(term);
    }
}

static void zvt_term_draw (GtkWidget *widget, GdkRectangle *area)
{
  ZvtTerm *term;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  term = ZVT_TERM (widget);
  vt_update_rect (term->vx,
		  area->x/term->charwidth,
		  area->y/term->charheight,
		  (area->x+area->width)/term->charwidth+1,
		  (area->y+area->height)/term->charheight+1);
		  
}

static gint
zvt_term_expose (GtkWidget      *widget,
		 GdkEventExpose *event)
{
  ZvtTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);

  d(printf("exposed!\n"));

  /* FIXME: may update 1 more line/char than needed */
  term->in_expose = 1;
  vt_update_rect(term->vx,
		 event->area.x/term->charwidth,
		 event->area.y/term->charheight,
		 (event->area.x+event->area.width)/term->charwidth+1,
		 (event->area.y+event->area.height)/term->charheight+1);
  term->in_expose = 0;

  return FALSE;
}

/*
  show/hide the pointer
*/
void zvt_term_show_pointer(ZvtTerm *term)
{
  if (term->cursor_current != term->cursor_bar) {
    gdk_window_set_cursor(GTK_WIDGET(term)->window, term->cursor_bar);
    term->cursor_current = term->cursor_bar;
  }
}

void zvt_term_hide_pointer(ZvtTerm *term)
{
  if (term->cursor_current != term->cursor_dot) {
    gdk_window_set_cursor(GTK_WIDGET(term)->window, term->cursor_dot);
    term->cursor_current = term->cursor_dot;
  }
}

/*
  set the number of scollback lines
*/
void zvt_term_set_scrollback(ZvtTerm *term, int lines)
{
  vt_scrollback_set(&term->vx->vt, lines);
  zvt_term_fix_scrollbar(term);
}

/*
  Load a set of fonts into the terminal.

  These fonts should be the same size, otherwise it could get messy ...

  FIXME: This should trigger a re-size
*/
static void zvt_term_set_fonts_internal(ZvtTerm *term, GdkFont *font, GdkFont *font_bold)
{
  if (font) {
    if (term->font)
      gdk_font_unref(term->font);
    term->font = font;
  }
  if (font_bold) {
    if (term->font_bold)
      gdk_font_unref(term->font_bold);
    term->font_bold = font_bold;
  }

  /* re-size window etc */
  term->charwidth = gdk_string_width(term->font, "M");
  term->charheight = term->font->ascent+term->font->descent;

#if 0
  gdk_window_resize(GTK_WIDGET(term)->window,
		    term->charwidth * term->vx->vt.width,
		    term->charheight * term->vx->vt.height);
#endif

#if 0
  /* force a re-draw */
  vt_update(term->vx, UPDATE_REFRESH|UPDATE_SCROLLBACK);/* redraw everything, unconditionally */
#endif

}

/*
  Load a set of fonts into the terminal.

  These fonts should be the same size, otherwise it could get messy ...
*/

void zvt_term_set_fonts(ZvtTerm *term, GdkFont *font, GdkFont *font_bold)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));
  g_return_if_fail (font != NULL);
  g_return_if_fail (font_bold != NULL);

  zvt_term_set_fonts_internal(term, font, font_bold);

  gdk_font_ref(font);
  gdk_font_ref(font_bold);
}

/*
  set a font by name

  Tries to calculate bold fonts from the base name.
  This only seems to sort of work in practice :-/.
*/
void zvt_term_set_font_name(ZvtTerm *term, char *name)
{
  char *newname, *rest, *outname;
  int count;
  char c, *ptr;
  GdkFont *font;
  GdkFont *font_bold;

  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));
  g_return_if_fail (name != NULL);

  newname = alloca(strlen(name)+1);
  outname = alloca(strlen(name)+16);

  strcpy(newname, name);
  rest = 0;
  ptr = newname;
  for(count=0;(c=*ptr++);) {
    if (c=='-') {
      count++;
      d(printf("scanning (%c) (%d)\n", c, count));
      d(printf("newname = %s ptr = %s\n", newname, ptr));
      switch (count) {
	/* e.g. "-schumacher-clean-medium-r-normal--10-100-75-75-c-80-iso646.1991-irv"*/
      case 3:
	ptr[-1]=0;
	break;
      case 5:
	rest = ptr-1;
	break;
      }
    }
  }
  if (rest) {
    sprintf(outname, "%s-medium-r%s", newname, rest);
    d(printf("loading normal font %s\n", outname));
    font = gdk_font_load(outname);

    sprintf(outname, "%s-bold-r%s", newname, rest);
    d(printf("loading bold font %s\n", outname));
    font_bold = gdk_font_load(outname);

    zvt_term_set_fonts_internal(term, font, font_bold);
  } else {
    font = gdk_font_load(name);
    font_bold = gdk_font_load(name);
    zvt_term_set_fonts_internal(term, font, font_bold);
  }
}

/*
  Called when something has changed, size of window or scrollback.

  Fixes the adjustment and notifies the system.
*/
static void zvt_term_fix_scrollbar(ZvtTerm *term)
{
  GTK_ADJUSTMENT(term->adjustment)->upper = term->vx->vt.scrollbacklines + term->vx->vt.height-1;
  GTK_ADJUSTMENT(term->adjustment)->value = term->vx->vt.scrollbackoffset + term->vx->vt.scrollbacklines;
  GTK_ADJUSTMENT(term->adjustment)->page_increment = term->vx->vt.height-1;
  GTK_ADJUSTMENT(term->adjustment)->page_size = term->vx->vt.height-1;

  gtk_signal_emit_by_name (GTK_OBJECT (term->adjustment), "changed");
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
  GdkAtom string_atom;
  struct _vtx *vx;
  ZvtTerm *term;

  d(printf("button pressed\n"));

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  vx = term->vx;

  zvt_term_show_pointer(term);

  gdk_window_get_pointer(widget->window, &x, &y, &mask);
  x/=term->charwidth;
  y=y/term->charheight+vx->vt.scrollbackoffset;

  /* Shift is an overwrite key for the reporting of the buttons */
  if (!(event->state & GDK_SHIFT_MASK))
    if (vt_report_button(&vx->vt, event->button, event->state, x, y)) 
      return FALSE;
    
  switch(event->button) {
  case 1:			/* left button */

    /* set selection type, and from which end we are selecting */
    switch(event->type) {
    case GDK_BUTTON_PRESS:
      vx->selectiontype = VT_SELTYPE_CHAR|VT_SELTYPE_BYSTART;
      break;
    case GDK_2BUTTON_PRESS:
      vx->selectiontype = VT_SELTYPE_WORD|VT_SELTYPE_BYSTART;
      break;
    case GDK_3BUTTON_PRESS:
      vx->selectiontype = VT_SELTYPE_LINE|VT_SELTYPE_BYSTART;
      break;
    default:
      break;
    }
    
    vx->selstartx = x;		/* reset selection */
    vx->selstarty = y;
    vx->selendx = x;
    vx->selendy = y;
    
    if (!vx->selected) {	/* reset 'drawn' screen (to avoid mis-refreshes) */
      vx->selstartxold = x;
      vx->selstartyold = y;
      vx->selendxold = x;
      vx->selendyold = y;
      vx->selected =1;
    }

    vt_fix_selection(vx);
    vt_draw_selection(vx);	/* handles by line/by word update */

    d(printf("selection starting %d %d\n", x, y));

    gtk_grab_add (widget);
    gdk_pointer_grab (widget->window, TRUE,
		      GDK_BUTTON_RELEASE_MASK |
		      GDK_BUTTON_MOTION_MASK |
		      GDK_POINTER_MOTION_HINT_MASK,
		      NULL, NULL, 0);
    break;

  case 2:			/* middle button - paste */
    /* Get the atom corresonding to the target "STRING" */
    string_atom = gdk_atom_intern ("STRING", FALSE);

    if (string_atom == GDK_NONE) {
      printf("WARNING: Could not get string atom\n");
    }

    /* And request the "STRING" target for the primary selection */
    gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, string_atom,
			   GDK_CURRENT_TIME);
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
      gdk_pointer_grab (widget->window, TRUE,
			GDK_BUTTON_RELEASE_MASK |
			GDK_BUTTON_MOTION_MASK |
			GDK_POINTER_MOTION_HINT_MASK,
			NULL, NULL, 0);
      }
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

  d(printf("button released\n"));

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  vx = term->vx;

  gdk_window_get_pointer(widget->window, &x, &y, &mask);

  x = x/term->charwidth;
  y = y/term->charheight + vx->vt.scrollbackoffset;

  if (!(event->state & GDK_SHIFT_MASK))
    if (vt_report_button(&vx->vt, 0, event->state, x, y))
      return FALSE;

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
    
    vt_fix_selection(vx);
    vt_draw_selection(vx);

    vx->selectiontype = VT_SELTYPE_NONE; /* 'turn off' selecting */
    
    vt_get_selection(vx, 0);

    gtk_selection_owner_set (widget,
			     GDK_SELECTION_PRIMARY,
			     event->time);
  }

  return FALSE;
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
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  vx = term->vx;

  d(printf("Motion notify\n"));

  if (vx->selectiontype != VT_SELTYPE_NONE) {
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
    
    vt_fix_selection(vx);
    vt_draw_selection(vx);
  }
  /* otherwise, just a mouse event */
  /* make sure the pointer is visible! */
  zvt_term_show_pointer(term);

  return FALSE;
}

/*
  called when another app claims the selection

  Make sure the display is refreshed to remove the selection 
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
zvt_term_selection_handler (GtkWidget *widget, 
			    GtkSelectionData *selection_data_ptr, gpointer data)
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
zvt_term_selection_received (GtkWidget *widget, GtkSelectionData *selection_data)
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
  vt_writechild(&vx->vt, selection_data->data, selection_data->length);
}  


static gint zvt_term_cursor_blink(gpointer data)
{
  ZvtTerm *term;
  GtkWidget *widget;

  widget = data;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);

  term = ZVT_TERM (widget);

  d(printf("Cursor blinked\n"));
  term->cursor_blink_state ^= 1;
  vt_cursor_state(data, term->cursor_blink_state);

  return TRUE;
}

/*
  Callback for when the adjustment changes - i.e., the scrollbar
  moves.
*/
static void zvt_term_scrollbar_moved (GtkAdjustment *adj, GtkWidget *widget)
{
  int line;
  ZvtTerm *term;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  term = ZVT_TERM (widget);

  line = term->vx->vt.scrollbacklines - (int)adj->value;
  if (line<0)			/* needed for floating point errors in slider code */
    line=0;

  d(printf("scrollbar moved to: %f\n\n", adj->value);
  printf("scroll offset is %d\n", line));

  term->vx->vt.scrollbackoffset = -line;

  vt_update(term->vx, UPDATE_SCROLLBACK); /* will redraw if scrollbar moved */
}


/*
  begin a child process, with a master controlling terminal
*/
int zvt_term_forkpty(ZvtTerm *term)
{
  int pid;

  g_return_val_if_fail (term != NULL, -1);
  g_return_val_if_fail (ZVT_IS_TERM (term), -1);

  if (term->input_id!=-1)	/* cannot fork twice! */
    return -1;

  pid = vt_forkpty(&term->vx->vt);
  if (pid>0) {
    term->input_id = gdk_input_add(term->vx->vt.childfd, GDK_INPUT_READ, zvt_term_readdata, term);
    term->msg_id = gdk_input_add(term->vx->vt.msgfd, GDK_INPUT_READ, zvt_term_readmsg, term);
  }
  return pid;
}

/*
  send a signal to the child process
*/
int zvt_term_killchild(ZvtTerm *term, int signal)
{
  g_return_val_if_fail (term != NULL, -1);
  g_return_val_if_fail (ZVT_IS_TERM (term), -1);

  return vt_killchild(&term->vx->vt, signal);
}

/*
  Close the child's pty.

  (and presumably the child process)
*/
int zvt_term_closepty(ZvtTerm *term)
{
  g_return_val_if_fail (term != NULL, -1);
  g_return_val_if_fail (ZVT_IS_TERM (term), -1);

  if (term->input_id!=-1) {
    gdk_input_remove(term->input_id);
    term->input_id=-1;
  }
  if (term->msg_id!=-1) {
    gdk_input_remove(term->msg_id);
    term->msg_id=-1;
  }
  return vt_closepty(&term->vx->vt);
}

static void
zvt_term_scroll (ZvtTerm *term, int n)
{
	gfloat new_value = 0;
	
	if (n)
		new_value = term->adjustment->value + (n * term->adjustment->page_size);
	else if (new_value == (term->adjustment->upper - term->adjustment->page_size))
		return;
	else
		new_value = term->adjustment->upper - term->adjustment->page_size;

	gtk_adjustment_set_value (term->adjustment,
				  n > 0 ? MIN(new_value, term->adjustment->upper
					      - term->adjustment->page_size)
				  : MAX(new_value, term->adjustment->lower));
}

/*
  Keyboard input callback
*/
static gint
zvt_term_key_press (GtkWidget *widget, GdkEventKey *event)
{
  char buffer[64];
  char *p=buffer;
  struct _vtx *vx;
  ZvtTerm *term;
  int handled;
  
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
    if (event->state & GDK_MOD1_MASK){
      *p++ = '\033';
    }
    *p++ = 8;
    break;
  case GDK_KP_Right:
  case GDK_Right:
    p+=sprintf(p, "\033OC");
    break;
  case GDK_KP_Left:
  case GDK_Left:
    p+=sprintf (p, "\033OD");
    break;
  case GDK_KP_Up:
  case GDK_Up:
    p+=sprintf (p, "\033OA");
    break;
  case GDK_KP_Down:
  case GDK_Down:
    p+=sprintf (p, "\033OB");
    break;
  case GDK_KP_Insert:
  case GDK_Insert:
    p+=sprintf (p, "\033[2~");
    break;
  case GDK_Delete:
    if (event->state & GDK_MOD1_MASK){
      *p++ = '\033';
    }
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
    p+=sprintf (p, "\033[11~");
    break;
  case GDK_KP_F2:
  case GDK_F2:
    p+=sprintf (p, "\033[12~");
    break;
  case GDK_KP_F3:
  case GDK_F3:
    p+=sprintf (p, "\033[13~");
    break;
  case GDK_KP_F4:
  case GDK_F4:
    p+=sprintf (p, "\033[14~");
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

/*
  dummy default signal handler
*/
static void zvt_term_child_died(ZvtTerm *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (ZVT_IS_TERM (term));

  /* perhaps we should do something here? */
}

static void vtx_unrender_selection (struct _vtx *vx)
{
  /* need to 'un-render' selected area */
  if (vx->selected) {
    vx->selstartx = vx->selendx;
    vx->selstarty = vx->selendy;
    vt_draw_selection(vx);	/* un-render selection */
    vx->selected = 0;
  }
}

/*
  this callback is called when data is ready on the child's file descriptor

  Read all data waiting on the file descriptor, updating the virtual
  terminal buffer, until there is no more data to read, and then render it.

  NOTE: this may impact on slow machines, but it may not also ...!
*/
static void zvt_term_readdata(gpointer data, gint fd, GdkInputCondition condition)
{
  char buffer[4096];
  int count;
  int iter;
  int saveerrno;
  struct _vtx *vx;
  ZvtTerm *term;

  term = (ZvtTerm *)data;
  if (term->input_id == -1) return;
  vx = term->vx;

  vtx_unrender_selection (vx);

  iter=0;
  saveerrno = EAGAIN;

  vt_cursor_state(term, 0);
  while ( (saveerrno == EAGAIN) && (count=read(fd, buffer, 4096)) > 0) {
    saveerrno = errno;

    parse_vt(&vx->vt, buffer, count);

    /* use this for debugging */
    /*vt_update(vt, UPDATE_CHANGES);
      gdk_flush();
      usleep(500);*/
    
    iter++;
  }
  if (iter) {
    d(printf("update from read\n\n"));
    vt_update(vx, UPDATE_CHANGES);
  } else {
    saveerrno = errno;
  }

  /* *always* turn the cursor back on */
  vt_cursor_state (term, 1);
  
  /* fix scroll bar */
  if (term->scroll_on_output) zvt_term_scroll (term, 0);

  /*
   * flush all X events - this is really necessary to stop X queuing up
   * lots of screen updates and reducing interactivity on a busy terminal
   */
  gdk_flush();

  /* read failed? oh well, that's life -- we handle dead children via
     SIGCHLD */

  zvt_term_fix_scrollbar(term);

}

void
zvt_term_feed (ZvtTerm *term, char *text, int len)
{
	g_return_if_fail (term != NULL);
	g_return_if_fail (ZVT_IS_TERM (term));
	g_return_if_fail (text != NULL);

	
	vt_cursor_state (term, 0);
	vtx_unrender_selection (term->vx);
	parse_vt(&term->vx->vt, text, len);
	vt_update (term->vx, UPDATE_CHANGES);

	/* *always* turn the cursor back on */
	vt_cursor_state (term, 1);

	/* fix scroll bar */
	if (term->scroll_on_output) zvt_term_scroll (term, 0);

	/*
	 * flush all X events - this is really necessary to stop X queuing up
	 * lots of screen updates and reducing interactivity on a busy terminal
	 */
	gdk_flush();

	/*
	 * read failed? oh well, that's life -- we handle dead children via
	 *  SIGCHLD
	 */

	zvt_term_fix_scrollbar(term);
}

static void zvt_term_readmsg(gpointer data, gint fd, GdkInputCondition condition)
{
  ZvtTerm *term = (ZvtTerm *)data;

  /* I suppose I should bother reading the message from the fd, but
     it doesn't seem worth the trouble <shrug> */

  if (term->input_id!=-1) {
    gdk_input_remove(term->input_id);
    term->input_id=-1;
  }

  zvt_term_closepty(term);

  /* signal application FIXME: include error/non error code */
  gtk_signal_emit(GTK_OBJECT(term), term_signals[CHILD_DIED]);
}

/*
  external rendering functions called by vt_update, etc
*/
int vt_cursor_state(void *user_data, int state)
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
  if (old_state ^ state) {
    if (GTK_WIDGET_DRAWABLE (widget)){
      vt_draw_cursor(term->vx, state);
      term->cursor_on = state;
    }
  }
  return old_state;
}

void vt_draw_text(void *user_data, int col, int row, char *text, int len, int attr)
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

  term = ZVT_TERM (widget);

  vx = term->vx;

  if (attr&VTATTR_BOLD) {
    or = 8;
    f=term->font_bold;
  } else {
    or = 0;
    f=term->font;
  }

  fore = (attr & VTATTR_FORECOLOURM) >> VTATTR_FORECOLOURB;
  back = (attr & VTATTR_BACKCOLOURM) >> VTATTR_BACKCOLOURB;

  if (fore<8)
    fore|=or;

  /* set the right colour in the appropriate gc */
  fgc = term->fore_gc;
  bgc = term->back_gc;

  if (term->back_last != back){
    pen.pixel = term->colors [back];
    gdk_gc_set_foreground (bgc, &pen);
    term->back_last = back;
  }
  
  if (term->fore_last != fore){
    pen.pixel = term->colors [fore];
    gdk_gc_set_foreground (fgc, &pen);
    term->fore_last = fore;
  }

  /* for reverse, swap gc's */
  if (attr & VTATTR_REVERSE){
    GdkGC *tmp = fgc;
    int tmp2 = fore;

    fgc = bgc;
    bgc = tmp;

    fore = back;
    back = tmp2;
  }

  /* optimise: dont 'clear' background if in expose, and background colour == window colour */
  /* this may look a bit weird, it really does need to be this way - so dont touch!*/
  if (term->in_expose==0 && vx->back_match==0) {
    gdk_draw_rectangle(widget->window,
		       bgc,
		       1,
		       col*term->charwidth, row*term->charheight,
		       len*term->charwidth, term->charheight);
  } else {
    d(printf("not clearing background\n"));
  }

  gdk_draw_text(widget->window,
		f,
		fgc,
		col*term->charwidth, row*term->charheight+term->font->ascent,
		text, len);

  /* check for underline */
  if (attr&VTATTR_UNDERLINE) {
    gdk_draw_line(widget->window,
		  fgc,
		  col*term->charwidth, row*term->charheight+term->font->ascent+1,
		  (col+len)*term->charwidth, row*term->charheight+term->font->ascent+1);
  }
}

void vt_scroll_area(void *user_data, int firstrow, int count, int offset)
{
  int width,height;
  ZvtTerm *term;
  GtkWidget *widget;
#if 0
  GdkEvent *event;
#endif

  widget = user_data;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  term = ZVT_TERM (widget);

  /* FIXME: check args */

  d(printf("scrolling %d rows from %d, by %d lines\n", count,firstrow,offset));

  width = widget->allocation.width;
  height = widget->allocation.height;

  /* "scroll" area */
  gdk_draw_pixmap(widget->window,
		  term->scroll_gc, /* must use this to generate expose events */
		  widget->window,
		  0, (firstrow+offset)*term->charheight,
		  0, firstrow*term->charheight,
		  width, count*term->charheight);


#if 1
  /* clear the other part of the screen */
  if (offset>0) {
    gdk_draw_rectangle(widget->window,
		       term->back_gc,
		       1,
		       0, (firstrow+count)*term->charheight,
		       width, offset*term->charheight);
  } else {
    gdk_draw_rectangle(widget->window,
		       term->back_gc,
		       1,
		       0, (firstrow+offset)*term->charheight,
		       width, (-offset)*term->charheight);
  }
#endif

#if 0
  /* this seems to slow it down, maybe, but I can't tell.
     it is also *not* necessary in this case, 'cause zvt
     will never try and scroll the same area twice, between
     full updates

     however, leaving it in cleans up rendering a little */

  /* fix up the screen display after a scroll */
  while ((event = gdk_event_get_graphics_expose (widget->window)) 
	 != NULL) {
    gtk_widget_event (widget, event);
    if (event->expose.count == 0)
      {
	gdk_event_free (event);
	break;
      }
    gdk_event_free (event);
  }
#endif

}

/*
  high-light a block of text
*/
void vt_hightlight_block(void *user_data, int col, int row, int width, int height)
{
  ZvtTerm *term;
  GtkWidget *widget;

  widget = user_data;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  term = ZVT_TERM (widget);

  gdk_gc_set_function(widget->style->fg_gc[GTK_WIDGET_STATE (widget)], GDK_INVERT);

  gdk_draw_rectangle(widget->window,
		     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		     1,
		     col*term->charwidth,
		     row*term->charheight,
		     width*term->charwidth,
		     height*term->charheight);
  
  gdk_gc_set_function(widget->style->fg_gc[GTK_WIDGET_STATE (widget)], GDK_COPY);  
}  
