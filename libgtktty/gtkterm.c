/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkTerm: Provide a widget with basic terminal output facilities
 * Copyright (C) 1997 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <config.h>
#include "gtkterm.h"
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkselection.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

/* --- limits & defaults --- */
#define	BLINK_TIMEOUT			(333)
#define	REFRESH_TIMEOUT			(25 * 0 /* FIXME ;) */)
#define	MIN_WIDTH			(15)
#define	MAX_WIDTH			(1024)
#define	MIN_HEIGHT			(1)
#define	MAX_HEIGHT			(1024)
#define	UNDERLINE_THIKNESS		(1)
#define	CURSOR_THIKNESS			(3)
#define INNER_BORDER			(0)
#define	DEFAULT_CURSOR_MODE		(GTK_CURSOR_UNDERLINE)
#define	DEFAULT_CURSOR_BLINK		(TRUE)


/* --- constants --- */
enum
{
  FLAG_NONE		= 0,
  FLAG_DIM		= 1 << 0,
  FLAG_BOLD		= 1 << 1,
  FLAG_UNDERLINE	= 1 << 2,
  FLAG_REVERSE		= 1 << 3,
  FLAG_SELECTION	= 1 << 4,
  FLAG_DIRTY		= 1 << 5
};

enum
{
  TEXT_RESIZE,
  BELL,
  LAST_SIGNAL
};


/* --- prototypes --- */

typedef	void	(*GtkTermSignalTextResize)	(GtkObject	*object,
						 gpointer	new_width_guint_p,
						 gpointer	new_height_guint_p,
						 gpointer	data);
typedef	gint	(*GtkTermSignalBell)		(GtkObject	*object,
						 gpointer	data);
typedef	void	(*GtkTermSignal3)		(GtkObject	*object,
						 guint		arg1,
						 guint		arg2,
						 gulong		arg3,
						 gpointer	data);

static	void	gtk_term_marshal_text_resize	(GtkObject	*object,
						 GtkSignalFunc	func,
						 gpointer	func_data,
						 GtkArg		*args);
static	void	gtk_term_marshal_bell		(GtkObject	*object,
						 GtkSignalFunc	func,
						 gpointer	func_data,
						 GtkArg		*args);
static	void	gtk_term_marshal_signal_3	(GtkObject	*object,
						 GtkSignalFunc	func,
						 gpointer	func_data,
						 GtkArg		*args);


static	void	gtk_term_class_init		(GtkTermClass	*class);
static	gint	gtk_term_class_timeout		(GtkTermClass	*class);
static	gint	gtk_term_timeout		(GtkTerm	*term);
static	void	gtk_term_init			(GtkTerm	*term);
static	void	gtk_term_destroy		(GtkObject	*object);
static	void	gtk_term_realize		(GtkWidget	*widget);
static	void	gtk_term_unrealize		(GtkWidget	*widget);
static	void	gtk_term_draw_focus		(GtkWidget	*widget);
static	void	gtk_term_size_request		(GtkWidget	*widget,
						 GtkRequisition	*requisition);
static	void	gtk_term_size_allocate		(GtkWidget	*widget,
						 GtkAllocation	*allocation);
static	void	gtk_term_draw			(GtkWidget	*widget,
						 GdkRectangle	*area);
static	gint	gtk_term_expose			(GtkWidget	*widget,
						 GdkEventExpose	*event);
static	gint	gtk_term_button_press		(GtkWidget	*widget,
						 GdkEventButton	*event);
static	gint	gtk_term_button_release		(GtkWidget	*widget,
						 GdkEventButton	*event);
static	gint	gtk_term_motion_notify		(GtkWidget	*widget,
						 GdkEventMotion	*event);
static	gint	gtk_term_focus_in		(GtkWidget	*widget,
						 GdkEventFocus	*event);
static	gint	gtk_term_focus_out		(GtkWidget	*widget,
						 GdkEventFocus	*event);
static	gint	gtk_term_selection_clear	(GtkWidget	*widget,
						 GdkEventSelection *event);
static	void	gtk_entry_selection_handler	(GtkWidget	*widget,
						 GtkSelectionData *selection_data,
						 gpointer	data);


/* --- static variables --- */

static GtkWidgetClass	*parent_class = NULL;
static gint		 term_signals[LAST_SIGNAL] = { 0 };
static gchar		 gtk_term_blank_char = ' ';
static GtkTermAttrib	 gtk_term_blank_attrib = { FLAG_NONE, GTK_TERM_MAX_COLORS - 1, 0 };


/* --- internal stuff (macros/functions) --- */

#include "gtkterm_internal.c"


/* --- functions --- */

guint
gtk_term_get_type ()
{
  static guint term_type = 0;
  
  if (!term_type)
  {
    GtkTypeInfo term_info =
    {
      "GtkTerm",
      sizeof (GtkTerm),
      sizeof (GtkTermClass),
      (GtkClassInitFunc) gtk_term_class_init,
      (GtkObjectInitFunc) gtk_term_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    
    term_type = gtk_type_unique (gtk_widget_get_type (), &term_info);
  }
  
  return term_type;
}

static void
gtk_term_class_init (GtkTermClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  
  if (!parent_class)
    parent_class = gtk_type_class (gtk_widget_get_type ());
  
  term_signals[0] = gtk_signal_new ("text_resize",
				    GTK_RUN_LAST,
				    object_class->type,
				    GTK_SIGNAL_OFFSET (GtkTermClass, text_resize),
				    gtk_term_marshal_text_resize,
				    GTK_TYPE_NONE, 2,
				    GTK_TYPE_POINTER,
				    GTK_TYPE_POINTER);
  term_signals[1] = gtk_signal_new ("bell",
				    GTK_RUN_LAST,
				    object_class->type,
				    GTK_SIGNAL_OFFSET (GtkTermClass, bell),
				    gtk_term_marshal_bell,
				    GTK_TYPE_INT, 0);
  
  gtk_object_class_add_signals (object_class, term_signals, LAST_SIGNAL);
  
  object_class->destroy = gtk_term_destroy;
  
  widget_class->realize = gtk_term_realize;
  widget_class->unrealize = gtk_term_unrealize;
  widget_class->draw_focus = gtk_term_draw_focus;
  widget_class->size_request = gtk_term_size_request;
  widget_class->size_allocate = gtk_term_size_allocate;
  widget_class->draw = gtk_term_draw;
  widget_class->expose_event = gtk_term_expose;
  widget_class->button_press_event = gtk_term_button_press;
  widget_class->button_release_event = gtk_term_button_release;
  widget_class->motion_notify_event = gtk_term_motion_notify;
  widget_class->focus_in_event = gtk_term_focus_in;
  widget_class->focus_out_event = gtk_term_focus_out;
  
  class->blink_handler = 0;
  class->term_widgets = NULL;
  class->blink_state = TRUE;
  class->text_resize = NULL;
  class->bell = NULL;
}

static gint
gtk_term_class_timeout (GtkTermClass   *class)
{
  GList *list;
  
  class->blink_state = !class->blink_state;
  
  list = class->term_widgets;
  while (list)
  {
    GtkTerm *term;
    
    term = GTK_TERM(list->data);
    
    if (GTK_WIDGET_DRAWABLE (term) &&
	term->cursor_blinking &&
	term->cursor_mode != GTK_CURSOR_INVISIBLE)
      gtk_term_update_cursor (term);
    
    list = list->next;
  }
  
  return TRUE;
}

static void
gtk_term_init (GtkTerm *term)
{
  guint			i;
  static GdkAtom	atom_text = GDK_NONE;
  static GdkAtom	atom_string = GDK_TARGET_STRING;
  
  if (atom_text == GDK_NONE)
    atom_text = gdk_atom_intern ("TEXT", FALSE);
  if (atom_string == GDK_NONE)
    atom_string = gdk_atom_intern ("STRING", FALSE);
  if (atom_text != GDK_NONE)
    gtk_selection_add_handler (GTK_WIDGET (term),
			       GDK_SELECTION_PRIMARY,
			       atom_text,
			       gtk_entry_selection_handler,
			       NULL);
  if (atom_string != GDK_NONE)
    gtk_selection_add_handler (GTK_WIDGET (term),
			       GDK_SELECTION_PRIMARY,
			       atom_string,
			       gtk_entry_selection_handler,
			       NULL);
  
  
  GTK_WIDGET_SET_FLAGS (term, GTK_CAN_FOCUS);
  
  term->view_port = NULL;
  term->text_area = NULL;
  term->text_gc = NULL;
  term->refresh_blocked = FALSE;
  term->inverted = FALSE;
  
  term->max_term_width = 0;
  term->max_term_height = 0;
  term->term_width = 0;
  term->term_height = 0;
  term->first_line = 0;
  term->first_used_line = 0;
  term->scroll_offset = 0;
  
  term->dim = FALSE;
  term->bold = FALSE;
  term->underline = FALSE;
  term->reverse = FALSE;
  term->i_fore = GTK_TERM_MAX_COLORS - 1;
  term->i_back = 0;
  
  term->cursor_mode = DEFAULT_CURSOR_MODE;
  term->cursor_blinking = DEFAULT_CURSOR_BLINK;
  term->cur_x = 0;
  term->cur_y = 0;
  term->top = 0;
  term->bottom = 0;
  
  term->sel_valid = FALSE;
  term->sel_b_x = 0;
  term->sel_b_y = 0;
  term->sel_e_x = 0;
  term->sel_e_y = 0;
  term->sel_buffer = NULL;
  term->sel_len = 0;
  
  for (i = 0; i < GTK_TERM_MAX_COLORS; i++)
  {
    term->back[i] = 0;
    term->fore[i] = 0;
    term->fore_dim[i] = 0;
    term->fore_bold[i] = 0;
  }

  term->font_normal = NULL;
  term->font_bold = NULL;
  term->font_dim = NULL;
  term->font_underline = NULL;
  term->font_reverse = NULL;

  /* try fonts in following order:
   * -misc-fixed- size 20,
   * -adobe-courier- (if we got this we can use medium/bold pair),
   * -misc-fixed- any size,
   * else complain ;)
   */
  term->font_normal = gdk_font_load ("-misc-fixed-*-*-*-*-20-*-*-*-*-*-*-*");
  if (!term->font_normal)
  {
    term->font_normal = gdk_font_load ("-adobe-courier-medium-r-*-*-*-*-*-*-*-*-*-*-");
    if (term->font_normal)
      term->font_bold = gdk_font_load ("-adobe-courier-bold-r-*-*-*-*-*-*-*-*-*-*-");

    /* now fall back to -misc-fixed- */
    if (!term->font_normal)
      term->font_normal = gdk_font_load ("-misc-fixed-*-*-*-*-*-*-*-*-*-*-*-*");

    if (!term->font_normal)
    {
      g_warning ("GtkTerm: could not load default font `-misc-fixed-*'");
      term->font_normal = gtk_widget_get_default_style ()->font;
    }
  }
  if (!term->font_bold)
  {
    term->font_bold = term->font_normal;
    gdk_font_ref (term->font_bold);
    term->overstrike_bold = TRUE;
  }
  else
    term->overstrike_bold = FALSE;
  term->font_dim = term->font_normal;
  gdk_font_ref (term->font_dim);
  term->font_underline = term->font_normal;
  gdk_font_ref (term->font_underline);
  term->font_reverse = term->font_normal;
  gdk_font_ref (term->font_reverse);
  term->draw_underline = TRUE;
  term->colors_reversed = TRUE;
  
  term->char_height = FONT_HEIGHT (term->font_normal);
  term->char_descent = term->font_normal->descent;
  term->char_vorigin = term->char_height - term->char_descent;
  term->char_width = 0;
  /* this is ugly, but we need the sizes for the character grid
   */
  for (i = 0; i < 256; i++)
  {
    register guint width;
    
    width = gdk_char_width (term->font_normal, i);
    term->char_width = MAX (term->char_width, width);
  }
  
  term->char_buffer = NULL;
  term->attrib_buffer = NULL;

  term->flags_dirty = FALSE;
  term->refresh_handler = 0;
}

void
gtk_term_block_refresh (GtkTerm        *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));

  term->refresh_blocked = TRUE;
}

static gint
gtk_term_timeout (GtkTerm   *term)
{
  if (term->flags_dirty)
    gtk_term_force_refresh (term);
  
  return TRUE;
}

void
gtk_term_force_refresh (GtkTerm        *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));

  if (GTK_WIDGET_DRAWABLE (term) && term->flags_dirty)
  {
    register guint y;
    gboolean refresh_blocked_saved;
    
    refresh_blocked_saved = term->refresh_blocked;
    term->refresh_blocked = FALSE;
    
    for (y = term->first_line; y < term->max_term_height; y++)
    {
      register guint x;
      
      for (x = 0; x < term->term_width; x++)
      {
	if (term->attrib_buffer[y][x].flags & FLAG_DIRTY)
	{
	  register guint first;
	  
	  first = x;
	  do
	  {
	    x++;
	  }
	  while (x < term->term_width && term->attrib_buffer[y][x].flags & FLAG_DIRTY);
	  
	  gtk_term_update_line (term, y, first, x - 1);

	  if (term->cur_y == y &&
	      term->cur_x >= first &&
	      term->cur_x <= x - 1)
	    gtk_term_update_cursor (term);
	}
      }
    }
    term->refresh_blocked = refresh_blocked_saved;
    term->flags_dirty = FALSE;
  }
}

void
gtk_term_unblock_refresh (GtkTerm        *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));

  term->refresh_blocked = FALSE;

  if (!REFRESH_TIMEOUT && term->flags_dirty)
    gtk_term_force_refresh (term);
}

void
gtk_term_set_fonts (GtkTerm        *term,
		    GdkFont        *font_normal,
		    GdkFont        *font_dim,
		    GdkFont        *font_bold,
		    gboolean       overstrike_bold,
		    GdkFont        *font_underline,
		    gboolean       draw_underline,
		    GdkFont        *font_reverse,
		    gboolean       colors_reversed)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));

  if (font_normal)
  {
    gdk_font_ref (font_normal);
    gdk_font_unref (term->font_normal);
    term->font_normal = font_normal;
  }
  if (font_bold)
  {
    gdk_font_ref (font_bold);
    gdk_font_unref (term->font_bold);
    term->font_bold = font_bold;
  }
  if (font_dim)
  {
    gdk_font_ref (font_dim);
    gdk_font_unref (term->font_dim);
    term->font_dim = font_dim;
  }
  if (font_underline)
  {
    gdk_font_ref (font_underline);
    gdk_font_unref (term->font_underline);
    term->font_underline = font_underline;
  }
  if (font_reverse)
  {
    gdk_font_ref (font_reverse);
    gdk_font_unref (term->font_reverse);
    term->font_reverse = font_reverse;
  }
  term->overstrike_bold = overstrike_bold != FALSE;
  term->draw_underline = draw_underline != FALSE;
  term->colors_reversed = colors_reversed != FALSE;
}

void
gtk_term_setup (GtkTerm	      *term,
		guint	       width,
		guint	       height,
		guint	       max_width,
		guint	       scrollback)
{
  guint i;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  g_return_if_fail (width > 0 && height > 0);
  g_return_if_fail (width >= max_width);
  
  g_return_if_fail (term->max_term_width == 0 && term->max_term_height == 0);
  g_return_if_fail (term->char_buffer == NULL && term->attrib_buffer == NULL);
  
  term->max_term_width = MIN (max_width, MAX_WIDTH);
  term->max_term_height = MIN (height + scrollback, MAX_HEIGHT);
  term->term_width = MIN (width, term->max_term_width);
  term->term_height = MIN (height, term->max_term_height);
  term->first_line = term->max_term_height - term->term_height;
  term->first_used_line = term->first_line;
  term->scroll_offset = 0;
  
  term->cur_y = term->first_line;
  
  for (i = 0; i < GTK_TERM_MAX_COLORS; i++)
  {
    term->back[i] = gtk_widget_get_style (GTK_WIDGET (term))->black.pixel;
    term->fore[i] = gtk_widget_get_style (GTK_WIDGET (term))->white.pixel;
    term->fore_dim[i] = gtk_widget_get_style (GTK_WIDGET (term))->mid[GTK_STATE_NORMAL].pixel;
    term->fore_bold[i] = gtk_widget_get_style (GTK_WIDGET (term))->white.pixel;
  }
  
  term->char_buffer = g_new (gchar*, term->max_term_height);
  term->attrib_buffer = g_new (GtkTermAttrib*, term->max_term_height);
  
  for (i = 0; i < term->max_term_height; i++)
  {
    term->char_buffer[i] = g_new (gchar, term->max_term_width);
    term->attrib_buffer[i] = g_new (GtkTermAttrib, term->max_term_width);
    
    gtk_term_line_init (term, i);
  }
  
  gtk_term_reset (term);
}

static void
gtk_term_realize (GtkWidget *widget)
{
  GtkTerm *term;
  GtkTermClass *class;
  GdkWindowAttr attributes;
  gint attributes_mask;
  GdkColor color = { 0, 0, 0, 0 };
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TERM (widget));
  
  term = GTK_TERM (widget);
  class = GTK_TERM_CLASS (GTK_OBJECT (widget)->klass);
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_BUTTON_PRESS_MASK |
			    GDK_BUTTON_RELEASE_MASK |
			    GDK_BUTTON1_MOTION_MASK |
			    GDK_BUTTON3_MOTION_MASK |
			    GDK_POINTER_MOTION_HINT_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK |
			    GDK_KEY_PRESS_MASK |
			    GDK_KEY_RELEASE_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, term);
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
  
  
  attributes.x = 1 + widget->style->klass->xthickness + INNER_BORDER;
  attributes.y = 1 + widget->style->klass->ythickness + INNER_BORDER;
  attributes.width -= (1 + widget->style->klass->xthickness + INNER_BORDER) * 2;
  attributes.height -= (1 + widget->style->klass->xthickness + INNER_BORDER) * 2;
  
  term->view_port = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (term->view_port, term);
  gtk_style_set_background (widget->style, term->view_port, GTK_STATE_NORMAL);
  gdk_window_show (term->view_port);
  
  
  attributes.x = 0;
  attributes.y = - term->first_line * term->char_height;
  attributes.width = TEXT_AREA_WIDTH (term);
  attributes.height = TEXT_AREA_HEIGHT (term);
  
  term->text_area = gdk_window_new (term->view_port, &attributes, attributes_mask);
  gdk_window_set_user_data (term->text_area, term);
  color.pixel = term->back[term->inverted ? GTK_TERM_MAX_COLORS - 1 : 0];
  gdk_window_set_background (term->text_area, &color);
  gdk_window_show (term->text_area);
  term->scroll_offset = 0;
  
  term->text_gc = gdk_gc_new (term->text_area);
  gdk_gc_set_exposures (term->text_gc, TRUE);
  gdk_gc_set_exposures (term->text_gc, FALSE);
  gdk_gc_set_fill (term->text_gc, GDK_SOLID);

  if (REFRESH_TIMEOUT)
    term->refresh_handler = gtk_timeout_add (REFRESH_TIMEOUT, (GtkFunction) gtk_term_timeout, term);
  
  if (class->term_widgets == NULL)
    class->blink_handler = gtk_timeout_add (BLINK_TIMEOUT, (GtkFunction) gtk_term_class_timeout, class);
  class->term_widgets = g_list_append  (class->term_widgets, term);
}

static void
gtk_term_unrealize (GtkWidget *widget)
{
  GtkTerm *term;
  GtkTermClass *class;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TERM (widget));
  
  term = GTK_TERM (widget);
  class = GTK_TERM_CLASS (GTK_OBJECT (widget)->klass);
  
  GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED);
  
  gtk_style_detach (widget->style);
  
  if (widget->window)
  {
    gdk_window_set_user_data (widget->window, NULL);
    gdk_window_destroy (widget->window);
    widget->window = NULL;
  }
  
  if (term->view_port)
  {
    gdk_window_set_user_data (term->view_port, NULL);
    gdk_window_destroy (term->view_port);
    term->view_port = NULL;
  }
  
  if (term->text_area)
  {
    gdk_window_set_user_data (term->text_area, NULL);
    gdk_window_destroy (term->text_area);
    term->text_area = NULL;
  }
  
  if (term->text_gc)
  {
    gdk_gc_destroy (term->text_gc);
    term->text_gc = NULL;
  }

  if (term->refresh_handler)
    gtk_timeout_remove (term->refresh_handler);
  
  class->term_widgets = g_list_remove (class->term_widgets, term);
  if (class->term_widgets == NULL)
  {
    gtk_timeout_remove (class->blink_handler);
    class->blink_handler = 0;
    class->blink_state = TRUE;
  }
}

static void
gtk_term_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkTerm *term;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TERM (widget));
  g_return_if_fail (requisition != NULL);
  
  term = GTK_TERM (widget);
  
  requisition->width = VIEW_PORT_WIDTH (term) +
		       (1 + widget->style->klass->xthickness + INNER_BORDER) * 2;
  requisition->height = VIEW_PORT_HEIGHT (term) +
			(1 + widget->style->klass->ythickness + INNER_BORDER) * 2;
}

static void
gtk_term_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkTerm *term;
  guint new_width;
  guint new_height;
  guint cursor_x_saved, cursor_y_inv_saved;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TERM (widget));
  g_return_if_fail (allocation != NULL);
  
  term = GTK_TERM (widget);
  
  new_width = (allocation->width - (1 + widget->style->klass->xthickness + INNER_BORDER) * 2) / term->char_width;
  new_height = (allocation->height - (1 + widget->style->klass->ythickness + INNER_BORDER) * 2) / term->char_height;
  new_width = MIN (new_width, term->max_term_width);
  new_height = MIN (new_height, term->max_term_height);
  
  gtk_signal_emit (GTK_OBJECT (term), term_signals[TEXT_RESIZE],
		   &new_width, &new_height);
  
  SEL_MAKE_VOID (term);
  
  cursor_x_saved = term->cur_x;
  cursor_y_inv_saved = term->max_term_height - term->cur_y;
  term->cur_x = 0;
  term->cur_y = term->max_term_height - 1;
  if (GTK_WIDGET_DRAWABLE (widget))
    gtk_term_update_char (term, cursor_x_saved, term->max_term_height - cursor_y_inv_saved);
  
  
  new_width = CLAMP (new_width, MIN_WIDTH, term->max_term_width);
  new_height = CLAMP (new_height, MIN_HEIGHT, term->max_term_height);
  term->term_width = new_width;
  term->term_height = new_height;
  term->first_line = term->max_term_height - term->term_height;
  
  widget->allocation.x = allocation->x;
  widget->allocation.y = allocation->y;
  widget->allocation.width = VIEW_PORT_WIDTH (term) +
			     (1 + widget->style->klass->xthickness + INNER_BORDER) * 2;
  widget->allocation.height = VIEW_PORT_HEIGHT (term) +
			      (1 + widget->style->klass->ythickness + INNER_BORDER) * 2;
  
  if (GTK_WIDGET_REALIZED (widget))
  {
    gdk_window_move_resize (widget->window,
			    widget->allocation.x,
			    widget->allocation.y,
			    widget->allocation.width,
			    widget->allocation.height);
    gdk_window_move_resize (term->view_port,
			    1 + widget->style->klass->xthickness + INNER_BORDER,
			    1 + widget->style->klass->ythickness + INNER_BORDER,
			    term->term_width * term->char_width,
			    term->term_height * term->char_height);
    
    term->cur_x = MIN (cursor_x_saved, term->term_width - 1);
    term->cur_y = MAX (term->max_term_height - cursor_y_inv_saved, term->first_line);

    gtk_term_set_scroll_offset (term, 0);
  }
  
  gtk_term_reset (term);
  if (GTK_WIDGET_DRAWABLE (widget))
    gtk_term_update_cursor (term);

  gtk_term_force_refresh (term);
}

gint
gtk_term_set_scroll_offset (GtkTerm	*term,
			    gint	offset)
{
  gint lower_bound;
  
  g_return_val_if_fail (term != NULL, 0);
  g_return_val_if_fail (GTK_IS_TERM (term), 0);
  
  lower_bound = - (term->first_line - term->first_used_line);
  
  if (offset < lower_bound)
    term->scroll_offset = lower_bound;
  else if (offset > 0)
    term->scroll_offset = 0;
  else
    term->scroll_offset = offset;

  if (GTK_WIDGET_REALIZED (term))
    gdk_window_move (term->text_area,
		     0,
		     - (term->first_line + term->scroll_offset) * term->char_height);
  
  return term->scroll_offset;
}

static void
gtk_term_draw_focus (GtkWidget *widget)
{
  gint width, height;
  gint x, y;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TERM (widget));
  
  if (GTK_WIDGET_DRAWABLE (widget))
  {
    x = 0;
    y = 0;
    gdk_window_get_size (widget->window, &width, &height);
    
    if (GTK_WIDGET_HAS_FOCUS (widget))
      gdk_draw_rectangle (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
			  FALSE,
			  0,
			  0,
			  width - 1,
			  height - 1);
    else
      gdk_draw_rectangle (widget->window, widget->style->bg_gc[GTK_STATE_NORMAL],
			  FALSE,
			  0,
			  0,
			  width - 1,
			  height - 1);
    
    gtk_draw_shadow (widget->style, widget->window,
		     GTK_STATE_NORMAL, GTK_SHADOW_IN,
		     x + 1,
		     y + 1,
		     width - 1 - 1,
		     height - 1 - 1);
  }
}

static void
gtk_term_draw (GtkWidget    *widget,
	       GdkRectangle *area)
{
  GtkTerm *term;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TERM (widget));
  /* g_return_if_fail (area != NULL); hm, isn't this saver? */
  
  term = GTK_TERM (widget);
  
  if (GTK_WIDGET_DRAWABLE (widget))
  {
    gtk_widget_draw_focus (widget);
    gtk_term_update_area (term, NULL);
  }
}

static gint
gtk_term_expose (GtkWidget	*widget,
		 GdkEventExpose *event)
{
  GtkTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  term = GTK_TERM (widget);
  
  if (widget->window == event->window)
    gtk_widget_draw_focus (widget);
  else if (term->text_area == event->window)
    gtk_term_update_area (term, &event->area);
  
  return FALSE;
}

static gint
gtk_term_button_press (GtkWidget      *widget,
		       GdkEventButton *event)
{
  GtkTerm *term;
  guint x, y;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  term = GTK_TERM (widget);
  x = event->x / term->char_width;
  y = event->y / term->char_height;

  if (x >= term->term_width) x = term->term_width - 1;
  if (y >= term->term_height + term->first_line) y = term->term_height + term->first_line - 1;
  
  gtk_term_selection_clear (widget, NULL);
  
  if (!GTK_WIDGET_HAS_FOCUS (widget))
    gtk_widget_grab_focus (widget);
  
  if (event->button == 1)
  {
    switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      term->sel_b_x = x;
      term->sel_b_y = y;
      term->sel_e_x = term->sel_b_x;
      term->sel_e_y = term->sel_b_y;
      gtk_grab_add (widget);
      break;
      
    case GDK_2BUTTON_PRESS:
      printf ("FIXME: need a word selection\n");
      break;
      
    case GDK_3BUTTON_PRESS:
      term->sel_b_x = 0;
      term->sel_b_y = y;
      term->sel_e_x = term->term_width - 1;
      term->sel_e_y = y;
      term->sel_valid = TRUE;
      gtk_term_update_new_sel (term,
			       term->sel_b_x,
			       term->sel_b_y,
			       term->sel_b_x,
			       term->sel_b_y,
			       term->sel_e_x,
			       term->sel_e_y);
      break;
      
    default:
      break;
    }
  }
  else if (event->button == 2)
  {
    /* selection requested */
  }
  else if (event->button == 3)
  {
    if (event->type == GDK_BUTTON_PRESS)
    {
      term->sel_valid = TRUE;
      if (XY_2_I (MAX_WIDTH, x, y) < XY_2_I (MAX_WIDTH, term->sel_b_x, term->sel_b_y) &&
	  XY_2_I (MAX_WIDTH, term->sel_b_x, term->sel_b_y) <
	  XY_2_I (MAX_WIDTH, term->sel_e_x, term->sel_e_y))
      {
	term->sel_b_x = term->sel_e_x;
	term->sel_b_y = term->sel_e_y;
      }
      gtk_term_update_new_sel (term,
			       term->sel_b_x,
			       term->sel_b_y,
			       term->sel_b_x,
			       term->sel_b_y,
			       x,
			       y);
      term->sel_e_x = x;
      term->sel_e_y = y;
      gtk_grab_add (widget);
    }
  }
  
  return FALSE;
}

static gint
gtk_term_button_release (GtkWidget	*widget,
			 GdkEventButton *event)
{
  GtkTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  term = GTK_TERM (widget);
  
  if (event->button == 1 || event->button == 3)
  {
    gtk_grab_remove (widget);
    
    if (term->sel_valid)
      gtk_selection_owner_set (GTK_WIDGET (term), GDK_SELECTION_PRIMARY, event->time);
  }
  
  return FALSE;
}

static gint
gtk_term_motion_notify (GtkWidget      *widget,
			GdkEventMotion *event)
{
  GtkTerm *term;
  gint x, y, x_r;
  gboolean backwards, was_valid;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  term = GTK_TERM (widget);
  
  x = event->x;
  y = event->y;
  if (event->is_hint ||
      (event->window != term->text_area))
    gdk_window_get_pointer (term->text_area, &x, &y, NULL);
  
  x_r = x % term->char_width;
  
  if (x > 0)
    x = x / (gint) term->char_width;
  else
    x = - MAX_WIDTH;
  
  if (y > 0)
    y = y / (gint) term->char_height;
  else
    y = - MAX_HEIGHT;
  
  if (x < (gint) 0)
  {
    if (y <= (gint) term->sel_b_y)
      x = 0;
    else
    {
      x = term->term_width - 1;
      y -= 1;
    }
  }
  else if (x > (gint) (term->term_width - 1))
  {
    if (y >= (gint) term->sel_b_y)
      x = term->term_width - 1;
    else
    {
      x = 0;
      y += 1;
    }
  }
  
  if (y < (gint) term->first_line)
    y = term->first_line;
  else if (y > (gint) (term->max_term_height - 1))
    y = term->max_term_height - 1;
  
  was_valid = term->sel_valid;
  if (term->sel_b_x == x &&
      term->sel_b_y == y &&
      x_r < term->char_width * (2.0 / 3) &&
      x_r > term->char_width * (1.0 / 3))
    term->sel_valid = FALSE;
  else
    term->sel_valid = TRUE;
  
  backwards = gtk_term_update_new_sel (term,
				       term->sel_b_x,
				       term->sel_b_y,
				       was_valid == term->sel_valid ? term->sel_e_x : term->sel_b_x,
				       was_valid == term->sel_valid ? term->sel_e_y : term->sel_b_y,
				       x,
				       y);
  
  term->sel_e_x = x;
  term->sel_e_y = y;
  
  /*
    printf ("(%u, %u, %u, %u)\n", term->sel_b_x, term->sel_b_y, term->sel_e_x, term->sel_e_y);
    */
  
  return FALSE;
}

static gint
gtk_term_focus_in (GtkWidget	 *widget,
		   GdkEventFocus *event)
{
  GtkTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  term = GTK_TERM (widget);
  
  GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
  gtk_widget_draw_focus (widget);
  
  gtk_term_update_cursor (term);
  gtk_term_force_refresh (term);
  
  return FALSE;
}

static gint
gtk_term_focus_out (GtkWidget	  *widget,
		    GdkEventFocus *event)
{
  GtkTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TERM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  term = GTK_TERM (widget);
  
  GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);
  gtk_widget_draw_focus (widget);
  
  gtk_term_update_cursor (term);
  gtk_term_force_refresh (term);
  
  return FALSE;
}

static gint
gtk_term_selection_clear (GtkWidget		*widget,
			  GdkEventSelection	*event)
{
  GtkTerm *term;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TERM (widget), FALSE);
  /* g_return_val_if_fail (event != NULL, FALSE); */
  
  term = GTK_TERM (widget);
  
  if (term->sel_buffer)
  {
    g_free (term->sel_buffer);
    term->sel_buffer = NULL;
    term->sel_len = 0;
  }
  
  if (term->sel_valid)
  {
    gtk_selection_owner_set (NULL, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
    term->sel_valid = FALSE;
    
    if (GTK_WIDGET_DRAWABLE (widget))
      gtk_term_update_new_sel (term,
			       term->sel_b_x,
			       term->sel_b_y,
			       term->sel_e_x,
			       term->sel_e_y,
			       term->sel_b_x,
			       term->sel_b_y);
  }
  
  return FALSE;
}

static void
gtk_entry_selection_handler (GtkWidget		*widget,
			     GtkSelectionData	*selection_data,
			     gpointer		data)
{
  GtkTerm *term;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_TERM (widget));
  
  term = GTK_TERM (widget);
  
  if (!term->sel_buffer && term->sel_valid)
  {
    guint   b_x, b_y, e_x, e_y, len;
    guint   y, i;
    guchar  *sel_buffer;
    
    if ((term->sel_e_y < term->sel_b_y) ||
	(term->sel_b_y == term->sel_e_y && term->sel_e_x < term->sel_b_x))
    {
      e_x = term->sel_b_x;
      e_y = term->sel_b_y;
      b_x = term->sel_e_x;
      b_y = term->sel_e_y;
    }
    else
    {
      b_x = term->sel_b_x;
      b_y = term->sel_b_y;
      e_x = term->sel_e_x;
      e_y = term->sel_e_y;
    }
    
    len = e_x - b_x + 1 + (e_y - b_y) * term->term_width;
    
    sel_buffer = g_new (guchar, len + (e_y - b_y + 1) + 1);
    
    y = b_y;
    i = 0;
    for (; y <= e_y; y++)
    {
      guint e, b;
      
      if (y == e_y)
	e = e_x;
      else
	e = term->term_width - 1;
      while (e &&
	     (term->char_buffer[y][e] == ' ' ||
	      term->char_buffer[y][e] == '\t'))
	e--;
      
      if (y == b_y)
	b = b_x;
      else
	b = 0;
      
      if (e == 0 &&
	  (term->char_buffer[y][e] == ' ' ||
	   term->char_buffer[y][e] == '\t'))
	sel_buffer[i++] = '\n';
      else
	do
	{
	  sel_buffer[i++] = term->char_buffer[y][b++];
	}
	while (b <= e);
      
      if (y != e_y ||
	  e_x >= term->term_width - 1)
	sel_buffer[i++] = '\n';
    }
    
    sel_buffer[i] = 0;
    term->sel_buffer = g_strdup (sel_buffer);
    term->sel_len = i;
    g_free (sel_buffer);
  }
  
  if (term->sel_buffer)
  {
    gtk_selection_data_set (selection_data,
			    GDK_SELECTION_TYPE_STRING,
			    sizeof (gchar) * 8,
			    term->sel_buffer,
			    term->sel_len);
    printf ("selction: \"%s\"\n", term->sel_buffer);
  }
}

static void
gtk_term_marshal_text_resize (GtkObject	    *object,
			      GtkSignalFunc func,
			      gpointer	    func_data,
			      GtkArg	    *args)
{
  GtkTermSignalTextResize rfunc;
  
  rfunc = (GtkTermSignalTextResize) func;
  
  (* rfunc) (object,
	     GTK_VALUE_POINTER (args[0]),
	     GTK_VALUE_POINTER (args[1]),
	     func_data);
}

static void
gtk_term_marshal_bell (GtkObject	*object,
		       GtkSignalFunc	func,
		       gpointer		func_data,
		       GtkArg		*args)
{
  GtkTermSignalBell rfunc;
  gint *return_val;
  
  rfunc = (GtkTermSignalBell) func;
  return_val = GTK_RETLOC_BOOL (args[0]);
  
  *return_val = (* rfunc) (object, func_data);
}

static void
gtk_term_marshal_signal_3 (GtkObject	  *object,
			   GtkSignalFunc  func,
			   gpointer	  func_data,
			   GtkArg	  *args)
{
  GtkTermSignal3 rfunc;
  
  rfunc = (GtkTermSignal3) func;
  
  (* rfunc) (object,
	     GTK_VALUE_UINT (args[0]),
	     GTK_VALUE_UINT (args[1]),
	     GTK_VALUE_ULONG (args[2]),
	     func_data);
}

static void
gtk_term_destroy (GtkObject *object)
{
  GtkTerm *term;
  guint i;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_TERM (object));
  
  term = GTK_TERM (object);
  
  /* this is done in gtk_real_widget_destroy ()
   * gtk_grab_remove (GTK_WIDGET (term));
   */
  
  if (term->sel_valid)
    gtk_term_selection_clear (GTK_WIDGET (term), NULL);
  
  for (i = 0; i < term->max_term_height; i++)
  {
    g_free (term->attrib_buffer[i]);
    g_free (term->char_buffer[i]);
  }

  gdk_font_unref (term->font_normal);
  gdk_font_unref (term->font_dim);
  gdk_font_unref (term->font_bold);
  gdk_font_unref (term->font_underline);
  gdk_font_unref (term->font_reverse);
  
  
  g_free (term->attrib_buffer);
  term->attrib_buffer = NULL;
  g_free (term->char_buffer);
  term->char_buffer = NULL;
  
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


/* --- terminal actions --- */
void
gtk_term_set_scroll_reg (GtkTerm	*term,
			 guint		top,
			 guint		bottom)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  term->top = MIN (top, term->term_height - 2);
  term->bottom = CLAMP (bottom, term->top + 1, term->term_height - 1);
}

void
gtk_term_scroll (GtkTerm	*term,
		 guint		n,
		 gboolean	downwards)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  
  n = CLAMP (n, 1, term->bottom - term->top + 1);
  
  if (downwards)
    gtk_term_scroll_down (term, term->first_line + term->top, term->first_line + term->bottom, n);
  else
    gtk_term_scroll_up (term, term->first_line + term->top, term->first_line + term->bottom, n);
}

void
gtk_term_delete_lines (GtkTerm	*term,
		       guint	n)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  
  n = MIN (n ? n : 1, term->max_term_height - term->cur_y);
  
  gtk_term_scroll_up (term, term->cur_y, term->first_line + term->bottom, n);
}

void
gtk_term_insert_lines (GtkTerm	*term,
		       guint	n)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  
  n = MIN (n ? n : 1, term->max_term_height - term->cur_y);
  
  gtk_term_scroll_down (term, term->cur_y, term->first_line + term->bottom, n);
}

void
gtk_term_set_color (GtkTerm	   *term,
		    guint	    index,
		    gulong	    back,
		    gulong	    fore,
		    gulong	    fore_dim,
		    gulong	    fore_bold)
{
  GdkColor color = { 0, 0, 0, 0 };
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  g_return_if_fail (index < GTK_TERM_MAX_COLORS);

  /*
    if (!fore_dim)
    fore_dim = fore;
    if (!fore_bold)
    fore_bold = fore;
    */
    
  term->back[index] = back;
  term->fore[index] = fore;
  term->fore_dim[index] = fore_dim;
  term->fore_bold[index] = fore_bold;

  color.pixel = term->back[term->inverted ? GTK_TERM_MAX_COLORS - 1 : 0];

  if (index == 0 && term->text_area)
    gdk_window_set_background (term->text_area, &color);

  if (GTK_WIDGET_DRAWABLE (term))
    gtk_term_update_area (term, NULL);
}

void
gtk_term_select_color (GtkTerm	*term,
		       guint	 fore_index,
		       guint	 back_index)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  term->i_fore = MIN (fore_index, GTK_TERM_MAX_COLORS - 1);
  term->i_back = MIN (back_index, GTK_TERM_MAX_COLORS - 1);
}

void
gtk_term_set_cursor (GtkTerm *term,
		     guint    x,
		     guint    y)
{
  guint old_x, old_y;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  
  old_x = term->cur_x;
  old_y = term->cur_y;
  
  x = MIN (term->term_width - 1, x);
  y = MIN (term->term_height - 1, y);
  
  term->cur_x = x;
  term->cur_y = y + term->first_line;
  
  if (term->cursor_mode != GTK_CURSOR_INVISIBLE &&
      (term->cur_x != old_x || term->cur_y != old_y))
  {
    if (GTK_WIDGET_DRAWABLE (term))
    {
      gtk_term_update_cursor (term);
      gtk_term_update_char (term, old_x, old_y);
    }
  }
}

void
gtk_term_get_cursor (GtkTerm	    *term,
		     guint	    *x,
		     guint	    *y)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  if (x)
    *x = term->cur_x;
  
  if (y)
    *y = term->cur_y - term->first_line;
}

void
gtk_term_save_cursor (GtkTerm	     *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  term->s_cur_x = term->cur_x;
  term->s_cur_y = term->cur_y;
  term->s_text_mode = ((term->dim	? FLAG_DIM	 : 0) |
		       (term->bold	? FLAG_BOLD	 : 0) |
		       (term->underline	? FLAG_UNDERLINE : 0) |
		       (term->reverse	? FLAG_REVERSE	 : 0));
  term->s_i_fore = term->i_fore;
  term->s_i_back = term->i_back;
}

void
gtk_term_restore_cursor (GtkTerm	*term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  term->cur_x = term->s_cur_x;
  term->cur_y = term->s_cur_y;
  term->dim = term->s_text_mode & FLAG_DIM;
  term->bold = term->s_text_mode & FLAG_BOLD;
  term->underline = term->s_text_mode & FLAG_UNDERLINE;
  term->reverse = term->s_text_mode & FLAG_REVERSE;
  term->i_fore = term->s_i_fore;
  term->i_back = term->s_i_back;
}

void
gtk_term_set_cursor_mode (GtkTerm	 *term,
			  GtkCursorMode	 mode,
			  gboolean	 blinking)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  
  term->cursor_mode = CLAMP (mode, GTK_CURSOR_INVISIBLE, GTK_CURSOR_BLOCK);
  term->cursor_blinking = blinking != FALSE;
  
  if (GTK_WIDGET_DRAWABLE (term))
    gtk_term_update_cursor (term);
}

void
gtk_term_get_size (GtkTerm	  *term,
		   guint	  *width,
		   guint	  *height)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  if (width)
    *width = term->term_width;
  
  if (height)
    *height = term->term_height;
}

void
gtk_term_erase_chars (GtkTerm	     *term,
		      guint	     n)
{
  guint i;
  guint start, end;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  
  if (n > 0)
    n--;
  
  start = term->cur_x;
  end = MIN (term->max_term_width - 1, start + n);
  
  for (i = start; i <= end; i++)
  {
    term->char_buffer[term->cur_y][i] = gtk_term_blank_char;
    term->attrib_buffer[term->cur_y][i] = gtk_term_blank_attrib;
  }
  
  if (GTK_WIDGET_DRAWABLE (term))
  {
    gdk_window_clear_area (term->text_area,
			   term->char_width * start,
			   term->char_height * term->cur_y,
			   (end - start + 1) * term->char_width,
			   term->char_height);
    CURSOR_ON (term);
  }
}

void
gtk_term_delete_chars (GtkTerm	      *term,
		       guint	      n)
{
  guint gap;
  guint len;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  
  gap = MAX (n, 1);
  
  if (gap < term->max_term_width - term->cur_x)
  {
    guint i;
    
    len = term->max_term_width - term->cur_x - gap;
    
    for (i = 0; i < len; i++)
    {
      term->char_buffer[term->cur_y][term->cur_x + i] = term->char_buffer[term->cur_y][term->cur_x + gap + i];
      term->attrib_buffer[term->cur_y][term->cur_x + i] = term->attrib_buffer[term->cur_y][term->cur_x + gap + i];
    }
    
    for (i = term->term_width - 1; i >= term->term_width - gap; i--)
    {
      term->char_buffer[term->cur_y][i] = gtk_term_blank_char;
      term->attrib_buffer[term->cur_y][i] = gtk_term_blank_attrib;
    }
    
    if (GTK_WIDGET_DRAWABLE (term))
    {
      gdk_window_copy_area (term->text_area,
			    term->text_gc,
			    term->cur_x * term->char_width,
			    term->char_height * term->cur_y,
			    NULL,
			    (term->cur_x + gap) * term->char_width,
			    term->char_height * term->cur_y,
			    len * term->char_width,
			    term->char_height);
      gdk_window_clear_area (term->text_area,
			     (term->term_width - gap) * term->char_width,
			     term->cur_y * term->char_height,
			     gap * term->char_width,
			     term->char_height);
      CURSOR_ON (term);
    }
  }
}


void
gtk_term_insert_chars (GtkTerm	*term,
		       guint	n)
{
  guint i;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  CURSOR_OFF (term);
  
  n = MIN (n ? n : 1, term->term_width - term->cur_x);
  
  for (i = term->term_width - 1; i >= term->cur_x + n; i--)
  {
    term->char_buffer[term->cur_y][i] = term->char_buffer[term->cur_y][i - n];
    term->attrib_buffer[term->cur_y][i] = term->attrib_buffer[term->cur_y][i - n];
  }
  
  for (i = 0; i < n; i++)
  {
    term->char_buffer[term->cur_y][term->cur_x + i] = gtk_term_blank_char;
    term->attrib_buffer[term->cur_y][term->cur_x + i] = gtk_term_blank_attrib;
  }
  
  if (GTK_WIDGET_DRAWABLE (term))
  {
    gtk_term_update_line (term,
			  term->cur_y,
			  term->cur_x + n,
			  term->term_width);
    gdk_window_clear_area (term->text_area,
			   term->cur_x * term->char_width,
			   term->cur_y * term->char_height,
			   n * term->char_width,
			   term->char_height);
    CURSOR_ON (term);
  }
}

void
gtk_term_clear_line (GtkTerm	*term,
		     gboolean	before_cursor,
		     gboolean	after_cursor)
{
  guint i;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  
  if (before_cursor)
    for (i = 0; i < term->cur_x; i++)
    {
      term->char_buffer[term->cur_y][i] = gtk_term_blank_char;
      term->attrib_buffer[term->cur_y][i] = gtk_term_blank_attrib;
    }
  
  if (after_cursor)
    for (i = term->cur_x + 1; i < term->term_width; i++)
    {
      term->char_buffer[term->cur_y][i] = gtk_term_blank_char;
      term->attrib_buffer[term->cur_y][i] = gtk_term_blank_attrib;
    }
  
  term->char_buffer[term->cur_y][term->cur_x] = gtk_term_blank_char;
  term->attrib_buffer[term->cur_y][term->cur_x] = gtk_term_blank_attrib;
  
  if (GTK_WIDGET_DRAWABLE (term))
  {
    guint b, e;
    
    b = before_cursor ? 0 : term->cur_x;
    e = after_cursor ? term->term_width - 1 : term->cur_x;
    gdk_window_clear_area (term->text_area,
			   b * term->char_width,
			   term->cur_y * term->char_height,
			   (e - b + 1) * term->char_width,
			   term->char_height);
    CURSOR_ON (term);
  }
}

void
gtk_term_clear (GtkTerm	   *term,
		gboolean   before_cursor,
		gboolean   after_cursor)
{
  guint i;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  NEW_INPUT (term);
  
  if (before_cursor)
  {
    for (i = term->first_line; i < term->cur_y; i++)
      gtk_term_line_init (term, i);
    
    if (GTK_WIDGET_DRAWABLE (term))
      gdk_window_clear_area (term->text_area,
			     0,
			     term->first_line * term->char_height,
			     term->max_term_width * term->char_width,
			     (term->cur_y - term->first_line) * term->char_height);
  }
  
  gtk_term_clear_line (term, before_cursor, after_cursor);
  
  if (after_cursor)
  {
    for (i = term->cur_y + 1; i < term->max_term_height; i++)
      gtk_term_line_init (term, i);
    
    if (GTK_WIDGET_DRAWABLE (term))
      gdk_window_clear_area (term->text_area,
			     0,
			     (term->cur_y + 1) * term->char_height,
			     term->max_term_width * term->char_width,
			     (term->max_term_height - term->cur_y) * term->char_height);
  }
}

gboolean
gtk_term_putc (GtkTerm	*term,
	       guchar	ch,
	       gboolean	insert)
{
  gboolean need_wrap;
  
  g_return_val_if_fail (term != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TERM (term), FALSE);
  NEW_INPUT (term);
  CURSOR_OFF (term);
  
  if (insert)
    gtk_term_insert_chars (term, 1);
  
  term->char_buffer[term->cur_y][term->cur_x] = ch;
  term->attrib_buffer[term->cur_y][term->cur_x].i_fore = term->i_fore;
  term->attrib_buffer[term->cur_y][term->cur_x].i_back = term->i_back;
  term->attrib_buffer[term->cur_y][term->cur_x].flags = FLAG_NONE;
  term->attrib_buffer[term->cur_y][term->cur_x].flags |= term->dim ? FLAG_DIM : 0;
  term->attrib_buffer[term->cur_y][term->cur_x].flags |= term->bold ? FLAG_BOLD : 0;
  term->attrib_buffer[term->cur_y][term->cur_x].flags |= term->underline ? FLAG_UNDERLINE : 0;
  term->attrib_buffer[term->cur_y][term->cur_x].flags |= term->reverse ? FLAG_REVERSE : 0;
  
  if (GTK_WIDGET_DRAWABLE (term))
    gtk_term_update_char (term, 
			  term->cur_x,
			  term->cur_y);
  
  term->cur_x++;
  if (term->cur_x >= term->term_width)
  {
    term->cur_x = term->term_width - 1;
    need_wrap = TRUE;
  }
  else
    need_wrap = FALSE;
  
  CURSOR_ON (term);
  
  return need_wrap;
}

void
gtk_term_bell (GtkTerm	      *term)
{
  gint return_val;

  return_val = TRUE;
  gtk_signal_emit (GTK_OBJECT (term), term_signals[BELL], &return_val);

  if (return_val)
    gdk_beep ();
}

void
gtk_term_reset (GtkTerm *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  term->dim = FALSE;
  term->bold = FALSE;
  term->underline = FALSE;
  term->reverse = FALSE;
  term->i_fore = GTK_TERM_MAX_COLORS - 1;
  term->i_back = 0;
  
  term->cursor_mode = DEFAULT_CURSOR_MODE;
  term->cursor_blinking = DEFAULT_CURSOR_BLINK;
  
  term->top = 0;
  term->bottom = term->term_height - 1;
  
  gtk_term_save_cursor (term);

  gtk_term_force_refresh (term);
}

void
gtk_term_set_dim (GtkTerm	 *term,
		  gboolean	 dim)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  term->dim = dim != FALSE;
}

void
gtk_term_set_bold (GtkTerm	  *term,
		   gboolean	  bold)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  term->bold = bold != FALSE;
}

void
gtk_term_set_underline (GtkTerm	       *term,
			gboolean	underline)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  term->underline = underline != FALSE;
}

void
gtk_term_set_reverse (GtkTerm	     *term,
		      gboolean	     reverse)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  term->reverse = reverse != FALSE;
}

void
gtk_term_invert (GtkTerm	      *term)
{
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));

  term->inverted = ! term->inverted;

  /*  for (i = 0; i < GTK_TERM_MAX_COLORS; i++)
  {
    GdkColor *tmp;

    tmp = term->back[i];
    term->back[i] = term->back[GTK_TERM_MAX_COLORS - 1];
    term->back[GTK_TERM_MAX_COLORS - 1] = tmp;
    
    tmp = term->fore[i];
    term->fore[i] = term->fore[GTK_TERM_MAX_COLORS - 1];
    term->fore[GTK_TERM_MAX_COLORS - 1] = tmp;
  }
  */
    
  if (term->text_area)
    {
      GdkColor color = { 0, 0, 0, 0 };

      color.pixel = term->back[term->inverted ? GTK_TERM_MAX_COLORS - 1 : 0];
      gdk_window_set_background (term->text_area, &color);
    }

  if (GTK_WIDGET_DRAWABLE (term))
    gtk_term_update_area (term, NULL);
}
