/*  zvtterm.c - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  The zvtterm widget.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <errno.h>
#include <unistd.h>

#include "zvtterm.h"

#define d(x)

/* Forward declararations */
static void zvt_term_init (ZvtTerm *term);
static void zvt_term_class_init (ZvtTermClass *class);
static void zvt_term_init (ZvtTerm *term);
GtkWidget* zvt_term_new (void);
static void zvt_term_destroy (GtkObject *object);
static void zvt_term_realize (GtkWidget *widget);
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
static gint zvt_term_selection_handler (GtkWidget *widget, GtkSelectionData *selection_data_ptr, gpointer *data);

static gint zvt_term_cursor_blink(gpointer data);
void zvt_term_scrollbar_moved (GtkAdjustment *adj, GtkWidget *widget);
void zvt_term_readdata(gpointer data, gint fd, GdkInputCondition condition);

/* Local data */

static GtkWidgetClass *parent_class = NULL;

guint
zvt_term_get_type ()
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

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  object_class->destroy = zvt_term_destroy;

  widget_class->realize = zvt_term_realize;
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
}

static void
zvt_term_init (ZvtTerm *term)
{
  GTK_WIDGET_SET_FLAGS (term, GTK_CAN_FOCUS);

  term->vx = vtx_new(term);

  term->cursor_on = 0;
  term->cursor_filled = 1;
  term->cursor_blink_state = 0;

  /* input handler */
  term->input_id = 0;

  /* load fonts */
  term->font = gdk_font_load("-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");
  term->font_bold = gdk_font_load("-misc-fixed-bold-r-semicondensed--13-120-75-75-c-60-iso8859-1");

  /* FIXME: should use font metrics to set the default size of letters.
     In practice, since this is fixed width, it wont make much difference ...
     Also, 'M' is usually the widest letter (well, in English anyway) */

  term->charwidth = gdk_string_width(term->font, "M");
  term->charheight = term->font->ascent+term->font->descent;

  /* scrollback position adjustment */
  term->adjustment = gtk_adjustment_new(0.0, 0.0, 24.0, 1.0, 24.0, 24.0);

  gtk_signal_connect (GTK_OBJECT (term->adjustment), "value_changed",
		      GTK_SIGNAL_FUNC (zvt_term_scrollbar_moved), NULL);

  /* selection received */
  gtk_selection_add_handler (GTK_WIDGET(term), GDK_SELECTION_PRIMARY,
			     GDK_SELECTION_TYPE_STRING,
			     zvt_term_selection_handler, term);
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

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
zvt_term_realize (GtkWidget *widget)
{
  ZvtTerm *term;
  GdkWindowAttr attributes;
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
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
    GDK_POINTER_MOTION_HINT_MASK |
    GDK_KEY_PRESS_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);

  /* create pixmaps for this window */
  term->cursor_dot_pm=gdk_pixmap_create_from_data(widget->window,
						"\0", 1, 1, 1,
						&widget->style->fg[GTK_STATE_ACTIVE],
						&widget->style->bg[GTK_STATE_ACTIVE]);

  /* Get I beam cursor, and also create a blank one based on the blank image */
  term->cursor_bar = gdk_cursor_new(GDK_XTERM);
  term->cursor_dot = gdk_cursor_new_from_pixmap(term->cursor_dot_pm, term->cursor_dot_pm,
					      &widget->style->fg[GTK_STATE_ACTIVE],
					      &widget->style->bg[GTK_STATE_ACTIVE],
					      0, 0);
  gdk_window_set_cursor(widget->window, term->cursor_bar);
  term->cursor_current = term->cursor_bar;

  /* setup blinking cursor */
  term->timeout_id = gtk_timeout_add(500, zvt_term_cursor_blink, term);

  /* FIXME: not sure if this is right or not? */
  if (!GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);
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
  if (term->timeout_id == -1)
    term->timeout_id = gtk_timeout_add(500, zvt_term_cursor_blink, term);

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
  if (term->timeout_id != -1) {
    gtk_timeout_remove(term->timeout_id);
    term->timeout_id = -1;
  }

  return FALSE;
}

static void 
zvt_term_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  ZvtTerm *term;

  term = widget;
  requisition->width = term->vx->vt.width * term->charwidth;	/* FIXME: base size on terminal size */
  requisition->height = term->vx->vt.height * term->charheight;
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
      
      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

      /* resize the virtual terminal buffer */
      vt_resize(&term->vx->vt,
		allocation->width/term->charwidth,
		allocation->height/term->charheight,
		allocation->width,
		allocation->height);
      vt_update(term->vx, UPDATE_REFRESH|UPDATE_SCROLLBACK);/* redraw everything, unconditionally */
      
      /* resize the scrollbar */
      GTK_ADJUSTMENT(term->adjustment)->upper = term->vx->vt.scrollbacklines + term->vx->vt.height-1;
      GTK_ADJUSTMENT(term->adjustment)->value = term->vx->vt.scrollbackoffset + term->vx->vt.scrollbacklines;
      GTK_ADJUSTMENT(term->adjustment)->page_increment = term->vx->vt.height-1;
      GTK_ADJUSTMENT(term->adjustment)->page_size = term->vx->vt.height-1;

      /* FIXME: notify? */
    }
}

static void zvt_term_draw (GtkWidget *widget, GdkRectangle *area)
{
  d(printf("zvt_term_draw called\n"));
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
  vt_update_rect(term->vx,
		 event->area.x/term->charwidth,
		 event->area.y/term->charheight,
		 (event->area.x+event->area.width)/term->charwidth+1,
		 (event->area.y+event->area.height)/term->charheight+1);

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
  static GdkAtom string_atom = GDK_NONE;
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

  if (vt_report_button(&vx->vt, event->button, event->state, x, y)) {
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
    /* Get the atom corresonding to the string "TARGETS" */
    if (string_atom == GDK_NONE)
      string_atom = gdk_atom_intern ("STRING", FALSE);
    
    /* And request the "TARGETS" target for the primary selection */
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

  if (vt_report_button(&vx->vt, 0, event->state, x, y)) {
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
    
    vt_draw_selection(vx);

    vx->selectiontype = VT_SELTYPE_NONE; /* 'turn off' selecting */
    
    vt_get_selection(vx, 0);

    gtk_selection_owner_set (widget,
			     GDK_SELECTION_PRIMARY,
			     GDK_CURRENT_TIME);
  }

  return FALSE;
}

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

  printf("Motion notify\n");

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

  term = ZVT_TERM (widget);
  vx = term->vx;

				/* FIXME: this is used elsewhere - modularise! */
  if (vx->selected) {
    vx->selstartx = vx->selendx;
    vx->selstarty = vx->selendy;
    vt_draw_selection(vx);
    vx->selected = 0;
  }

  vt_clear_selection(vx);
  return TRUE;
}

/* supply the current selection to the caller */
static gint
zvt_term_selection_handler (GtkWidget *widget, 
			    GtkSelectionData *selection_data_ptr, gpointer *data)
{
  struct _vtx *vx;
  ZvtTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (selection_data_ptr != NULL, FALSE);

  term = ZVT_TERM (widget);
  vx = term->vx;

  gtk_selection_data_set (selection_data_ptr, GDK_SELECTION_TYPE_STRING,
			  8, vx->selection_data, vx->selection_size);
  return FALSE;
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

  d(printf("got selection from system!\n"));

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

void zvt_term_scrollbar_moved (GtkAdjustment *adj, GtkWidget *widget)
{
  printf("Scrollbar moved\n");
}



/* FIXME: check argument */
int zvt_term_forkpty(ZvtTerm *term)
{
  int pid;

  pid = vt_forkpty(&term->vx->vt);
  if (pid>0) {
    if (!term->input_id) {
      gdk_input_add(term->vx->vt.childfd, GDK_INPUT_READ, zvt_term_readdata, term);
    }
  }
  return pid;
}


/*
  Keyboard input callback

  How do i stop the scrollback getting a look in on characters?
*/
static gint
zvt_term_key_press (GtkWidget *widget, GdkEventKey *event)
{
  char buffer[64];
  char *p=buffer;
  struct _vtx *vx;
  ZvtTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (ZVT_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  term = ZVT_TERM (widget);
  vx = term->vx;

  zvt_term_hide_pointer(term);
  
  d(printf("keyval = %04x state = %x\n", event->keyval, event->state));
  switch (event->keyval) {
  case GDK_BackSpace:
    *p++ = '\177';
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
#if 0
  case GDK_KP_Right:
  case GDK_Right:
    p+=sprintf(p, "\033[C");
    break;
  case GDK_KP_Left:
  case GDK_Left:
    p+=sprintf (p, "\033[D");
    break;
  case GDK_KP_Up:
  case GDK_Up:
    p+=sprintf (p, "\033[A");
    break;
  case GDK_KP_Down:
  case GDK_Down:
    p+=sprintf (p, "\033[B");
    break;
#endif
  case GDK_KP_Insert:
  case GDK_Insert:
    p+=sprintf (p, "\033[2~");
    break;
  case GDK_KP_Delete:
  case GDK_Delete:
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
    p+=sprintf (p, "\033[5~");
    break;
  case GDK_KP_Page_Down:
  case GDK_Page_Down:
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
  default:
    if (event->length > 0) {
      memcpy(buffer, event->string, event->length*sizeof(char));
      p=buffer+event->length;
    } else {
      /* FIXME: do something more intelligent with unknown keykodes */
      p+=sprintf(p, "[%x]", event->keyval);
    }
    break;
  }
  if (p>buffer)
    vt_writechild(&vx->vt, buffer, (p-buffer));

  return 1;
}


/*
  this callback is called when data is ready on the child's file descriptor

  Read all data waiting on the file descriptor, updating the virtual
  terminal buffer, until there is no more data to read, and then render it.

  NOTE: this may impact on slow machines, but it may not also ...!
*/
void zvt_term_readdata(gpointer data, gint fd, GdkInputCondition condition)
{
  char buffer[4096];
  int count;
  int iter;
  int saveerrno;
  struct _vtx *vx;
  ZvtTerm *term;

  term = (ZvtTerm *)data;
  vx = term->vx;

  /* need to 'un-render' selected area */
  if (vx->selected) {		/* FIXME: modularise */
    vx->selstartx = vx->selendx;
    vx->selstarty = vx->selendy;
    vt_draw_selection(vx);
    vx->selected = 0;
  }

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

  /* flush all X events - this is really necessary to stop X queuing up
     lots of screen updates and reducing interactivity on a busy terminal */
  gdk_flush();

  /* read failed?
      assume pipe cut - just quit */
  if (count<0 && saveerrno!=EAGAIN) {
    printf("errno = %d, saverrno = %d\n", errno, saveerrno);
    printf("out of data on read\n");

    /* FIXME: raise signal to caller ... */
    gtk_exit(1);
  }

  /* fix scroll bar */
  GTK_ADJUSTMENT(term->adjustment)->upper = vx->vt.scrollbacklines + vx->vt.height-1;
  GTK_ADJUSTMENT(term->adjustment)->value = vx->vt.scrollbackoffset + vx->vt.scrollbacklines;

  /* raise signal? */
}




/*
  external rendering functions called by vt_update, etc
*/
void vt_cursor_state(void *user_data, int state)
{
  ZvtTerm *term;
  GtkWidget *widget;

  widget = user_data;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (ZVT_IS_TERM (widget));

  term = ZVT_TERM (widget);

  gdk_gc_set_function(widget->style->fg_gc[GTK_WIDGET_STATE (widget)], GDK_INVERT);

  if ((state && !term->cursor_on) ||	/* turn it on, or*/
      (!state && term->cursor_on)) {	/* turn it off - then toggle it */
    gdk_draw_rectangle(widget->window,
		       widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		       term->cursor_filled,
		       term->vx->vt.cursorx*
		       term->charwidth,
		       (term->vx->vt.cursory-term->vx->vt.scrollbackoffset)*term->charheight,
		       term->charwidth, term->charheight);
    term->cursor_on ^= 1;
  }

  gdk_gc_set_function(widget->style->fg_gc[GTK_WIDGET_STATE (widget)], GDK_COPY);
}

void vt_draw_text(void *user_data, int col, int row, char *text, int len, int attr)
{
  GdkFont *f;
  GdkGC *gc1, *gc2;
  ZvtTerm *term;
  struct _vtx *vx;
  GtkWidget *widget;

  term = user_data;
  vx = term->vx;
  widget = term;

  f=term->font;
  if (attr&VTATTR_BOLD)
    f=term->font_bold;

  if (attr&VTATTR_REVERSE) {
    gc1=widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
    gc2=widget->style->bg_gc[GTK_WIDGET_STATE (widget)];
  } else {
    gc2=widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
    gc1=widget->style->bg_gc[GTK_WIDGET_STATE (widget)];
  }

  gdk_draw_rectangle(widget->window,
		     gc1,
		     1,
		     col*term->charwidth, row*term->charheight,
		     len*term->charwidth, term->charheight);
  gdk_draw_text(widget->window,
		f,
		gc2,
		col*term->charwidth, row*term->charheight+term->font->ascent,
		text, len);
}

void vt_scroll_area(void *user_data, int firstrow, int count, int offset)
{
  int width,height;
  GtkWidget *widget;
  ZvtTerm *term;

  term = user_data;
  widget = term;

  /* FIXME: check args */

  d(printf("scrolling %d rows from %d, by %d lines\n", count,firstrow,offset));

  width = widget->allocation.width;
  height = widget->allocation.height;

  gdk_window_copy_area(widget->window,
		       widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		       0, firstrow*term->charheight,
		       NULL, 
		       0, (firstrow+offset)*term->charheight,
		       width, count*term->charheight);

  /* clear the other part of the screen */
  if (offset>0) {
    gdk_draw_rectangle(widget->window,
		       widget->style->bg_gc[GTK_WIDGET_STATE (widget)],
		       1,
		       0, (firstrow+count)*term->charheight,
		       width, offset*term->charheight);
  } else {
    gdk_draw_rectangle(widget->window,
		       widget->style->bg_gc[GTK_WIDGET_STATE (widget)],
		       1,
		       0, (firstrow+offset)*term->charheight,
		       width, (-offset)*term->charheight);
  }
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
