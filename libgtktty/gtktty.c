/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkTty: Terminal emulation widget
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
#include "gtktty.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gdk/gdkkeysyms.h>


/* --- limits --- */
#define	INPUT_BUFFER_LEN	(8192 * 4)
#define	MAX_SCROLL_BACK		(999)

/* --- typedefs --- */
enum {
  /* FIXME: led-update */
  KEY_PRESS,
  PROGRAM_EXEC,
  PROGRAM_EXIT,
  LAST_SIGNAL
};


typedef	gint	(*GtkTtySignalKeyPress)		(GtkObject	*object,
						 gpointer	arg1,
						 guint		arg2,
						 guint		arg3,
						 guint		arg4,
						 gpointer	data);
typedef	void	(*GtkTtySignalExec)		(GtkObject	*object,
						 gpointer	arg1,
						 gpointer	arg2,
						 gpointer	arg3,
						 gpointer	data);
typedef	void	(*GtkTtySignalExit)		(GtkObject	*object,
						 gpointer	arg1,
						 gchar		arg2,
						 guint		arg3,
						 gpointer	data);


/* --- prototypes --- */
static	void	gtk_tty_marshal_signal_key_press (GtkObject		*object,
						  GtkSignalFunc	func,
						  gpointer		func_data,
						  GtkArg		*args);
static	void	gtk_tty_marshal_signal_exec	(GtkObject		*object,
						 GtkSignalFunc		func,
						 gpointer		func_data,
						 GtkArg			*args);
static	void	gtk_tty_marshal_signal_exit	(GtkObject		*object,
						 GtkSignalFunc		func,
						 gpointer		func_data,
						 GtkArg			*args);

static	void	gtk_tty_class_init		(GtkTtyClass		*class);
static	void	gtk_tty_init			(GtkTty			*tty);
static	void	gtk_tty_destroy			(GtkObject		*object);
static	gint	gtk_tty_key_press_event		(GtkWidget		*widget,
						 GdkEventKey		*event);
static	gint	gtk_tty_key_release_event	(GtkWidget		*widget,
						 GdkEventKey		*event);
static  gint	gtk_tty_focus_in_event		(GtkWidget		*widget,
						 GdkEventFocus		*event);
static  gint	gtk_tty_focus_out_event		(GtkWidget		*widget,
						 GdkEventFocus		*event);
static  void	gtk_tty_query_keystate		(GtkTty			*tty);
     
static	void	gtk_tty_input_func		(gpointer		data,
						 gint			source,
						 GdkInputCondition	condition);
static	void	gtk_tty_text_resize		(GtkTerm		*term,
						 guint			*new_width,
						 guint			*new_height);

/* --- prototypes for os specific stuff --- */
static	gpointer	gtk_tty_os_get_hintp	(void);
static	void		gtk_tty_os_open_pty	(GtkTty		*tty);
static	void		gtk_tty_os_close_pty	(GtkTty		*tty);
static	void		gtk_tty_os_close_except (gint		ex_fd);
static	void		gtk_tty_os_setup_tty	(GtkTty		*tty,
						 FILE		*f_tty,
						 gpointer	os_data);
static	gint		gtk_tty_os_winsize	(GtkTty		*tty,
						 gpointer	os_data,
						 gint		tty_fd,
						 guint		width,
						 guint		height,
						 guint		xpix,
						 guint		ypix);
static	void		gtk_tty_os_wait		(GtkTty		*tty);


/* --- external variables --- */
/* hm, seems like some systems miss the automatic declaration
 * of the environment char array, so we explicitly declare it
 * here, and implicitly assume every system has it...
 */
extern char **environ;

/* --- static variables --- */
static GtkTermClass	*parent_class = NULL;
static gint		 tty_signals[LAST_SIGNAL] = { 0 };

guint
gtk_tty_get_type ()
{
  static guint tty_type = 0;
  
  if (!tty_type)
  {
    GtkTypeInfo tty_info =
    {
      "GtkTty",
      sizeof (GtkTty),
      sizeof (GtkTtyClass),
      (GtkClassInitFunc) gtk_tty_class_init,
      (GtkObjectInitFunc) gtk_tty_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL,
    };
    
    tty_type = gtk_type_unique (gtk_term_get_type (), &tty_info);
  }
  
  return tty_type;
}

static void
gtk_tty_class_init (GtkTtyClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkTermClass *term_class;
  
  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  term_class = (GtkTermClass*) class;
  
  if (!parent_class)
    parent_class = gtk_type_class (gtk_term_get_type ());
  
  tty_signals[KEY_PRESS] = gtk_signal_new ("key_press",
					   GTK_RUN_LAST,
					   object_class->type,
					   GTK_SIGNAL_OFFSET (GtkTtyClass, key_press),
					   gtk_tty_marshal_signal_key_press,
					   GTK_TYPE_INT, 4,
					   GTK_TYPE_STRING,
					   GTK_TYPE_UINT,
					   GTK_TYPE_UINT,
					   GTK_TYPE_UINT);
  tty_signals[PROGRAM_EXEC] = gtk_signal_new ("program_exec",
					      GTK_RUN_LAST,
					      object_class->type,
					      GTK_SIGNAL_OFFSET (GtkTtyClass, program_exec),
					      gtk_tty_marshal_signal_exec,
					      GTK_TYPE_NONE, 3,
					      GTK_TYPE_STRING,
					      GTK_TYPE_POINTER,
					      GTK_TYPE_POINTER);
  tty_signals[PROGRAM_EXIT] = gtk_signal_new ("program_exit",
					      GTK_RUN_LAST,
					      object_class->type,
					      GTK_SIGNAL_OFFSET (GtkTtyClass, program_exit),
					      gtk_tty_marshal_signal_exit,
					      GTK_TYPE_NONE, 3,
					      GTK_TYPE_STRING,
					      GTK_TYPE_CHAR,
					      GTK_TYPE_UINT);
  gtk_object_class_add_signals (object_class, tty_signals, LAST_SIGNAL);
  
  object_class->destroy = gtk_tty_destroy;
  
  widget_class->key_press_event = gtk_tty_key_press_event;
  widget_class->key_release_event = gtk_tty_key_release_event;
  /*  widget_class->focus_in_event = gtk_tty_focus_in_event; */
  /*  widget_class->focus_out_event = gtk_tty_focus_out_event; */
  
  term_class->text_resize = gtk_tty_text_resize;
  
  class->os_hintp = gtk_tty_os_get_hintp ();

#ifdef	HAVE_WAIT4
  class->meassure_time = TRUE;
#else	/* !HAVE_WAIT4 */
  class->meassure_time = FALSE;
#endif	/* !HAVE_WAIT4 */

  class->key_press = NULL;
  class->program_exec = NULL;
  class->program_exit = NULL;
}

static void
gtk_tty_init (GtkTty *tty)
{
  tty->key_states = GTK_TTY_STATE_NONE;

  tty->ignore_scroll_lock = FALSE;
  tty->ignore_num_lock = FALSE;
  tty->ignore_caps_lock = FALSE;
  tty->freeze_leds = FALSE;
  tty->leds = tty->key_states;
  tty->update_leds = NULL;
  
  tty->key_pad_enter = FALSE;
  tty->key_pad_char = 0;
  
  tty->pty_fd = -1;
  tty->tty_name = NULL;
  tty->input_tag = 0;
  
  tty->vtemu = NULL;

  tty->pid = 0;
  tty->prg_name = NULL;
  tty->exit_status = 0;
  tty->exit_signal = 0;
  tty->sys_usec = 0;
  tty->sys_sec = 0;
  tty->user_usec = 0;
  tty->user_sec = 0;
}


GtkWidget*
gtk_tty_new (guint width,
	     guint height,
	     guint scrollback)
{
  GtkTty *tty;
  GList	 *list;
  
  tty = gtk_type_new (gtk_tty_get_type ());
  scrollback = MIN (scrollback, MAX_SCROLL_BACK);
  gtk_term_setup (GTK_TERM (tty), width, height, width, scrollback);

  list = gtk_vtemu_create_type_list ();
  tty->vtemu = gtk_vtemu_new (GTK_TERM (tty), list->data);
  g_list_free (list);
  
  return GTK_WIDGET (tty);
}

static gint
gtk_tty_key_press_event (GtkWidget	*widget,
			 GdkEventKey	*event)
{
  GtkTty *tty;
  guchar buffer[64];
  guint count;
  guchar old_key_states;
  gint	return_val;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TTY (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  if (!GTK_WIDGET_HAS_FOCUS (widget))
    return FALSE;
  
  tty = GTK_TTY (widget);
  
  memset (buffer, 0, sizeof (buffer));
  count = 0;
  
  /* ok, at this point we still have the old state settings in event->state,
   * therefore we need to override the states in the switch() statement
   */
  old_key_states = tty->key_states;
  tty->key_states &= ~(GTK_TTY_STATE_SHIFT	|
		       GTK_TTY_STATE_CAPS_LOCK	|
		       GTK_TTY_STATE_CONTROL	|
		       GTK_TTY_STATE_ALT	|
		       GTK_TTY_STATE_NUM_LOCK);
  tty->key_states |= ((event->state & GDK_SHIFT_MASK   ? GTK_TTY_STATE_SHIFT	: 0) |
		      (event->state & GDK_LOCK_MASK    ? GTK_TTY_STATE_CAPS_LOCK : 0) |
		      (event->state & GDK_CONTROL_MASK ? GTK_TTY_STATE_CONTROL	: 0) |
		      (event->state & GDK_MOD1_MASK    ? GTK_TTY_STATE_ALT	: 0) |
		      (event->state & GDK_MOD2_MASK    ? GTK_TTY_STATE_NUM_LOCK	: 0));
  switch (event->keyval)
  {
  case	0 ... 31: /* do these occour? */
    if (event->state & GDK_MOD1_MASK)
      buffer[count++] = '\e';
    buffer[count++] = event->keyval;
    break;
    
  case	' ' ... '/':
  case	'0' ... '9':
  case	':' ... '?':
  case	'`':
  case	'{' ... '~':
  case	GDK_nobreakspace ... GDK_ydiaeresis:
    if (event->state & GDK_MOD1_MASK)
      buffer[count++] = '\e';
    buffer[count++] = event->keyval;
    break;
    
  case	'@':
  case	'A' ... 'Z':
  case	'[' ... '_':
    if (event->state & GDK_MOD1_MASK)
      buffer[count++] = '\e';
    if (event->state & GDK_CONTROL_MASK)
      buffer[count++] = event->keyval - '@';
    else
      buffer[count++] = event->keyval;
    break;
    
  case	'a' ... 'z':
    if (event->state & GDK_MOD1_MASK)
      buffer[count++] = '\e';
    if (event->state & GDK_CONTROL_MASK)
      buffer[count++] = event->keyval - '`';
    else
      buffer[count++] = event->keyval;
    break;
    
  case	GDK_KP_0:
  case	GDK_KP_1:
  case	GDK_KP_2:
  case	GDK_KP_3:
  case	GDK_KP_4:
  case	GDK_KP_5:
  case	GDK_KP_6:
  case	GDK_KP_7:
  case	GDK_KP_8:
  case	GDK_KP_9:
    if (tty->key_states & GTK_TTY_STATE_ALT)
    {
      tty->key_pad_enter = TRUE;
      tty->key_pad_char *= 10;
      switch (event->keyval)
      {
      case  GDK_KP_1:	tty->key_pad_char += 1; break;
      case  GDK_KP_2:	tty->key_pad_char += 2; break;
      case  GDK_KP_3:	tty->key_pad_char += 3; break;
      case  GDK_KP_4:	tty->key_pad_char += 4; break;
      case  GDK_KP_5:	tty->key_pad_char += 5; break;
      case  GDK_KP_6:	tty->key_pad_char += 6; break;
      case  GDK_KP_7:	tty->key_pad_char += 7; break;
      case  GDK_KP_8:	tty->key_pad_char += 8; break;
      case  GDK_KP_9:	tty->key_pad_char += 9; break;
      }
      break;
    }
  case	GDK_KP_Add:
  case	GDK_KP_Subtract:
  case	GDK_KP_Multiply:
  case	GDK_KP_Divide:
  case	GDK_KP_Equal:
  case	GDK_KP_Decimal:
  case	GDK_KP_Space:
  case	GDK_KP_Tab:
  case	GDK_KP_Enter:
    if (event->state & GDK_MOD1_MASK)
      buffer[count++] = '\e';
    switch (event->keyval)
    {
    case  GDK_KP_0:		buffer[count++] = '0'; break;
    case  GDK_KP_1:		buffer[count++] = '1'; break;
    case  GDK_KP_2:		buffer[count++] = '2'; break;
    case  GDK_KP_3:		buffer[count++] = '3'; break;
    case  GDK_KP_4:		buffer[count++] = '4'; break;
    case  GDK_KP_5:		buffer[count++] = '5'; break;
    case  GDK_KP_6:		buffer[count++] = '6'; break;
    case  GDK_KP_7:		buffer[count++] = '7'; break;
    case  GDK_KP_8:		buffer[count++] = '8'; break;
    case  GDK_KP_9:		buffer[count++] = '9'; break;
    case  GDK_KP_Add:		buffer[count++] = '+'; break;
    case  GDK_KP_Subtract:	buffer[count++] = '-'; break;
    case  GDK_KP_Multiply:	buffer[count++] = '*'; break;
    case  GDK_KP_Divide:	buffer[count++] = '/'; break;
    case  GDK_KP_Equal:		buffer[count++] = '='; break;
    case  GDK_KP_Decimal:	buffer[count++] = '.'; break;
    case  GDK_KP_Space:		buffer[count++] = ' '; break;
    case  GDK_KP_Tab:		buffer[count++] = '\t'; break;
    case  GDK_KP_Enter:		buffer[count++] = '\n'; break;
    }
    break;
    
  case	GDK_Escape:
    buffer[count++] = '\033';
    break;
    
  case	GDK_Return:
    buffer[count++] = '\r';
    break;
    
  case	GDK_BackSpace:
    buffer[count++] = '\177';
    break;
    
  case	GDK_Tab:
    buffer[count++] = '\t';
    break;
    
  case	GDK_KP_Right:
  case	GDK_Right:
    count = sprintf (buffer, "\033[C");
    break;
    
  case	GDK_KP_Left:
  case	GDK_Left:
    count = sprintf (buffer, "\033[D");
    break;
    
  case	GDK_KP_Up:
  case	GDK_Up:
    count = sprintf (buffer, "\033[A");
    break;
    
  case	GDK_KP_Down:
  case	GDK_Down:
    count = sprintf (buffer, "\033[B");
    break;
    
  case	GDK_KP_Insert:
  case	GDK_Insert:
    count = sprintf (buffer, "\033[2~");
    break;
    
  case	GDK_KP_Delete:
  case	GDK_Delete:
    count = sprintf (buffer, "\033[3~");
    break;
    
  case	GDK_KP_Home:
  case	GDK_Home:
    count = sprintf (buffer, "\033[1~");
    break;
    
  case	GDK_KP_End:
  case	GDK_End:
    count = sprintf (buffer, "\033[4~");
    break;
    
  case	GDK_KP_Page_Up:		/* GDK_KP_Prior */
  case	GDK_Page_Up:		/* GDK_Prior */
    if (tty->key_states & GTK_TTY_STATE_SHIFT)
      gtk_term_set_scroll_offset (GTK_TERM (tty),
				  GTK_TERM (tty)->scroll_offset -
				  GTK_TERM (tty)->term_height / 2);
    else
      count = sprintf (buffer, "\033[5~");
    break;
    
  case	GDK_KP_Page_Down:	/* GDK_KP_Next */
  case	GDK_Page_Down:		/* GDK_Next */
    if (tty->key_states & GTK_TTY_STATE_SHIFT)
      gtk_term_set_scroll_offset (GTK_TERM (tty),
				  GTK_TERM (tty)->scroll_offset +
				  GTK_TERM (tty)->term_height / 2);
    else
      count = sprintf (buffer, "\033[6~");
    break;
    
  case	GDK_KP_F1:
  case	GDK_F1:
    count = sprintf (buffer, "\033[[A");
    break;
    
  case	GDK_KP_F2:
  case	GDK_F2:
    count = sprintf (buffer, "\033[[B");
    break;
    
  case	GDK_KP_F3:
  case	GDK_F3:
    count = sprintf (buffer, "\033[[C");
    break;
    
  case	GDK_KP_F4:
  case	GDK_F4:
    count = sprintf (buffer, "\033[[D");
    break;
    
  case	GDK_F5:
    count = sprintf (buffer, "\033[[E");
    break;
    
  case	GDK_F6:
    count = sprintf (buffer, "\033[17~");
    break;
    
  case	GDK_F7:
    count = sprintf (buffer, "\033[18~");
    break;
    
  case	GDK_F8:
    count = sprintf (buffer, "\033[19~");
    break;
    
  case	GDK_F9:
    count = sprintf (buffer, "\033[20~");
    break;
    
  case	GDK_F10:
    count = sprintf (buffer, "\033[21~");
    break;
    
  case	GDK_F11:
    count = sprintf (buffer, "\033[23~");
    break;
    
  case	GDK_F12:
    count = sprintf (buffer, "\033[24~");
    break;
    
  case	GDK_Num_Lock:
    if (!(old_key_states & GTK_TTY_STATE_NUM_LOCK))
      tty->ignore_num_lock = TRUE;
    tty->key_states |= GTK_TTY_STATE_NUM_LOCK;
    break;
    
  case	GDK_Scroll_Lock:
    if (!(old_key_states & GTK_TTY_STATE_SCROLL_LOCK))
      tty->ignore_scroll_lock = TRUE;
    tty->key_states |= GTK_TTY_STATE_SCROLL_LOCK;
    break;
    
  case	GDK_Shift_L:
  case	GDK_Shift_R:
    tty->key_states |= GTK_TTY_STATE_SHIFT;
    break;
    
  case	GDK_Shift_Lock:
  case	GDK_Caps_Lock:
    if (!(old_key_states & GTK_TTY_STATE_CAPS_LOCK))
      tty->ignore_caps_lock = TRUE;
    tty->key_states |= GTK_TTY_STATE_CAPS_LOCK;
    break;
    
  case	GDK_Control_L:
  case	GDK_Control_R:
    tty->key_states |= GTK_TTY_STATE_CONTROL;
    break;
    
  case	GDK_Alt_L:
  case	GDK_Alt_R:
  case	GDK_Meta_L:
  case	GDK_Meta_R:
    tty->key_states |= GTK_TTY_STATE_ALT;
    break;
    
  case	GDK_Super_L:
  case	GDK_Super_R:
    tty->key_states |= GTK_TTY_STATE_SUPER;
    break;
    
  case	GDK_Hyper_L:
  case	GDK_Hyper_R:
    tty->key_states |= GTK_TTY_STATE_HYPER;
    break;
    
  case	GDK_Mode_switch:
  case	GDK_Multi_key:
    break;
    
  default:
    count = sprintf (buffer, "[%p]", (gpointer) event->keyval);
    break;
  }
  
  buffer[count] = 0;
  
  if (old_key_states != tty->key_states && !tty->freeze_leds)
    gtk_tty_leds_changed (tty);
  
  return_val = FALSE;
  gtk_signal_emit (GTK_OBJECT (tty),
		   tty_signals[KEY_PRESS],
		   buffer,
		   count,
		   event->keyval,
		   event->state,
		   &return_val);
  
  if (!return_val)
    gtk_tty_put_in (tty, buffer, count);
  
  return TRUE;
}

static gint
gtk_tty_key_release_event (GtkWidget	  *widget,
			   GdkEventKey	  *event)
{
  GtkTty *tty;
  guint old_key_states;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TTY (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  
  tty = GTK_TTY (widget);
  
  /* ok, at this point we still have the old state settings in event->state
   */
  old_key_states = tty->key_states;
  tty->key_states &= ~(GTK_TTY_STATE_SHIFT	|
		       GTK_TTY_STATE_CONTROL	|
		       GTK_TTY_STATE_ALT);
  tty->key_states |= ((event->state & GDK_SHIFT_MASK   ? GTK_TTY_STATE_SHIFT	 : 0) |
		      (event->state & GDK_CONTROL_MASK ? GTK_TTY_STATE_CONTROL	 : 0) |
		      (event->state & GDK_MOD1_MASK    ? GTK_TTY_STATE_ALT	 : 0));
  switch (event->keyval)
  {
  case	GDK_Num_Lock:
    if (tty->ignore_num_lock)
      tty->ignore_num_lock = FALSE;
    else
      tty->key_states &= ~GTK_TTY_STATE_NUM_LOCK;
    break;
    
  case	GDK_Scroll_Lock:
    if (tty->ignore_scroll_lock)
      tty->ignore_scroll_lock = FALSE;
    else
      tty->key_states &= ~GTK_TTY_STATE_SCROLL_LOCK;
    break;
    
  case	GDK_Shift_L:
  case	GDK_Shift_R:
    tty->key_states &= ~GTK_TTY_STATE_SHIFT;
    break;
    
  case	GDK_Shift_Lock:
  case	GDK_Caps_Lock:
    if (tty->ignore_caps_lock)
      tty->ignore_caps_lock = FALSE;
    else
      tty->key_states &= ~GTK_TTY_STATE_CAPS_LOCK;
    break;
    
  case	GDK_Control_L:
  case	GDK_Control_R:
    tty->key_states &= ~GTK_TTY_STATE_CONTROL;
    break;
    
  case	GDK_Alt_L:
  case	GDK_Alt_R:
  case	GDK_Meta_L:
  case	GDK_Meta_R:
    tty->key_states &= ~GTK_TTY_STATE_ALT;
    break;
    
  case	GDK_Super_L:
  case	GDK_Super_R:
    tty->key_states &= ~GTK_TTY_STATE_SUPER;
    break;
    
  case	GDK_Hyper_L:
  case	GDK_Hyper_R:
    tty->key_states &= ~GTK_TTY_STATE_HYPER;
    break;
  }
  
  if (tty->key_pad_enter && !(tty->key_states & GTK_TTY_STATE_ALT))
  {
    tty->key_pad_enter = FALSE;
    gtk_tty_put_in (tty, &tty->key_pad_char, 1);
    tty->key_pad_char = 0;
  }
  
  if (old_key_states != tty->key_states && !tty->freeze_leds)
    gtk_tty_leds_changed (tty);
  
  return TRUE;
}

static void
gtk_tty_query_keystate (GtkTty *tty)
{
  guint old_key_states;
  GdkModifierType mods;

  mods = 0;
  gdk_window_get_pointer (NULL, NULL, NULL, &mods);

  old_key_states = tty->key_states;
  tty->key_states &= ~(GTK_TTY_STATE_SHIFT      |
		       GTK_TTY_STATE_CAPS_LOCK  |
		       GTK_TTY_STATE_CONTROL    |
		       GTK_TTY_STATE_ALT        |
		       GTK_TTY_STATE_NUM_LOCK);
  tty->key_states |= ((mods & GDK_SHIFT_MASK   ? GTK_TTY_STATE_SHIFT    : 0) |
		      (mods & GDK_LOCK_MASK    ? GTK_TTY_STATE_CAPS_LOCK : 0) |
		      (mods & GDK_CONTROL_MASK ? GTK_TTY_STATE_CONTROL  : 0) |
		      (mods & GDK_MOD1_MASK    ? GTK_TTY_STATE_ALT      : 0) |
		      (mods & GDK_MOD2_MASK    ? GTK_TTY_STATE_NUM_LOCK : 0));

  if (old_key_states != tty->key_states && !tty->freeze_leds)
    gtk_tty_leds_changed (tty);
}

static gint
gtk_tty_focus_in_event (GtkWidget          *widget,
			GdkEventFocus      *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TTY (widget), FALSE);

  gtk_tty_query_keystate (GTK_TTY (widget));

  if (GTK_WIDGET_CLASS (parent_class)->focus_in_event)
    return GTK_WIDGET_CLASS (parent_class)->focus_in_event (widget, event);
  return FALSE;
}

static gint
gtk_tty_focus_out_event (GtkWidget          *widget,
			 GdkEventFocus      *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_TTY (widget), FALSE);

  gtk_tty_query_keystate (GTK_TTY (widget));

  if (GTK_WIDGET_CLASS (parent_class)->focus_out_event)
    return GTK_WIDGET_CLASS (parent_class)->focus_out_event (widget, event);
  return FALSE;
}

static void
gtk_tty_text_resize (GtkTerm	*term,
		     guint		*new_width,
		     guint		*new_height)
{
  GtkTty *tty;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TTY (term));
  
  tty = GTK_TTY (term);
  
  if (parent_class->text_resize)
    (* parent_class->text_resize) (term, new_width, new_height);
  
  if (tty->pty_fd >= 0)
  {
    gint	err;
    
    err = gtk_tty_os_winsize (tty,
			      GTK_TTY_CLASS (GTK_OBJECT (tty)->klass)->os_hintp,
			      tty->pty_fd,
			      *new_width, *new_height,
			      0, 0);
    if (err || errno)
      g_warning ("setting new tty dimensions failed: %s\n", g_strerror (err));
  }
}

static void
gtk_tty_destroy (GtkObject *object)
{
  GtkTty *tty;
  GList *list;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_TTY (object));
  
  tty = GTK_TTY (object);

  if (tty->vtemu)
    gtk_vtemu_destroy (tty->vtemu);
  
  gtk_tty_os_close_pty (tty);
  
  list = tty->update_leds;
  while (list)
  {
    GtkTtyUpdateLed *update_led;
    
    update_led = list->data;
    list = list->next;
    
    gtk_widget_unref (GTK_WIDGET (update_led->led));
    g_free (update_led);
  }
  g_list_free (tty->update_leds);
  
  g_free (tty->prg_name);
  tty->prg_name = NULL;
  
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gtk_tty_marshal_signal_key_press (GtkObject		*object,
				  GtkSignalFunc	func,
				  gpointer		func_data,
				  GtkArg		*args)
{
  GtkTtySignalKeyPress rfunc;
  gint *return_val;
  
  rfunc = (GtkTtySignalKeyPress) func;
  return_val = GTK_RETLOC_BOOL (args[4]);
  
  *return_val = (* rfunc) (object,
			   GTK_VALUE_STRING (args[0]),
			   GTK_VALUE_UINT (args[1]),
			   GTK_VALUE_UINT (args[2]),
			   GTK_VALUE_UINT (args[3]),
			   func_data);
}

static void
gtk_tty_marshal_signal_exec (GtkObject		*object,
			     GtkSignalFunc	func,
			     gpointer		func_data,
			     GtkArg		*args)
{
  GtkTtySignalExec rfunc;
  
  rfunc = (GtkTtySignalExec) func;
  
  (* rfunc) (object,
	     GTK_VALUE_STRING (args[0]),
	     GTK_VALUE_POINTER (args[1]),
	     GTK_VALUE_POINTER (args[2]),
	     func_data);
}

static void
gtk_tty_marshal_signal_exit (GtkObject		*object,
			     GtkSignalFunc	func,
			     gpointer		func_data,
			     GtkArg		*args)
{
  GtkTtySignalExit rfunc;
  
  rfunc = (GtkTtySignalExit) func;
  
  (* rfunc) (object,
	     GTK_VALUE_STRING (args[0]),
	     GTK_VALUE_CHAR (args[1]),
	     GTK_VALUE_UINT (args[2]),
	     func_data);
}

void
gtk_tty_add_update_led (GtkTty		   *tty,
			GtkLed		   *led,
			GtkTtyKeyStateBits mask)
{
  GtkTtyUpdateLed *update_led;
  
  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  g_return_if_fail (led != NULL);
  g_return_if_fail (GTK_IS_LED (led));
  
  update_led = g_new (GtkTtyUpdateLed, 1);
  
  update_led->led = led;
  update_led->mask = mask;
  
  gtk_widget_ref (GTK_WIDGET (update_led->led));
  
  tty->update_leds = g_list_append (tty->update_leds, update_led);
  
  gtk_led_set_state (update_led->led,
		     GTK_STATE_SELECTED,
		     tty->leds & update_led->mask);
}

void
gtk_tty_leds_changed (GtkTty	     *tty)
{
  GList *list;
  
  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  
  if (!tty->freeze_leds)
    tty->leds = tty->key_states;
  
  /* FIXME: invoke signal handler */
  
  list = tty->update_leds;
  while (list)
  {
    GtkTtyUpdateLed *update_led;
    
    update_led = list->data;
    list = list->next;
    
    gtk_led_switch (update_led->led, tty->leds & update_led->mask);
  }
}

static void
gtk_tty_input_func (gpointer		data,
		    gint		source,
		    GdkInputCondition	condition)
{
  GtkTty	*tty;
  gint		len;
  gchar		buffer[INPUT_BUFFER_LEN];
  guint		c;
  
  g_return_if_fail (data != NULL);
  g_return_if_fail (GTK_IS_TTY (data));
  
  tty = data;

  c = 0;
  do
  {
    len = read (tty->pty_fd, buffer, INPUT_BUFFER_LEN);
    
    if (len > 0)
    {
      gtk_tty_put_out (tty, buffer, len);
      /* gtk_term_force_refresh (GTK_TERM (tty)); */
    }
    c++;
  }
  while (len > 0 && c < 3);
  
  if (len < 0)
  {

#ifdef	EAGAIN
    if (errno != EAGAIN)
    {
#endif	/* EAGAIN */

      /* we assume the child died....
       */
      
      gdk_input_remove (tty->input_tag);
      tty->input_tag = 0;
      
      gtk_tty_os_wait (tty);
      
      gtk_signal_emit (GTK_OBJECT (tty),
		       tty_signals[PROGRAM_EXIT],
		       tty->prg_name,
		       tty->exit_status,
		       tty->exit_signal);
#ifdef	EAGAIN
    }
#endif	/* EAGAIN */

  }
  /*  else
   *    g_warning ("hum, zero characters to read on input ready for read?");
   */
}

void
gtk_tty_put_out (GtkTty		*tty,
		 const guchar	*buffer,
		 guint		count)
{
  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  
  if (tty->vtemu)
    while (count)
    {
      count -= gtk_vtemu_input (tty->vtemu, buffer, count);
    }
}

void
gtk_tty_put_in (GtkTty		*tty,
		const guchar	*buffer,
		guint		count)
{
  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  
  if (tty->pty_fd < 0 || !tty->pid)
    gtk_tty_put_out (tty, buffer, count);
  else
    write (tty->pty_fd, buffer, count);
}

void
gtk_tty_change_emu (GtkTty	*tty,
		    GtkVtEmu	*vtemu)
{
  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));

  if (tty->vtemu)
    gtk_vtemu_destroy (tty->vtemu);

  tty->vtemu = vtemu;
}

void
gtk_tty_execute (GtkTty		*tty,
		 const gchar	*prg_name,
		 gchar * const	argv[],
		 gchar * const	envp[])
{
  FILE *f_tty;
  
  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  g_return_if_fail (tty->pid == 0);
  
  tty->exit_status = 0;
  tty->exit_signal = 0;
  
  
  g_free (tty->prg_name);
  tty->prg_name = NULL;
  
  if (!strchr (prg_name, '/'))
  {
    gchar *p;
    gchar *path, *free_path;
    
    path = getenv ("PATH");
    if (!path)
      path = "/bin:/usr/bin";
    free_path = g_strdup (path);
    path = free_path;
    
    p = strchr (path, ':');
    while (p || path)
    {
      gchar *test_name;
      
      if (p)
	*p = 0;
      
      test_name = g_new (gchar, strlen (path) + 1 + strlen (prg_name) + 1);
      test_name[0] = 0;
      strcat (test_name, path);
      strcat (test_name, "/");
      strcat (test_name, prg_name);
      if (access (test_name, X_OK) >= 0)
      {
	tty->prg_name = test_name;
	break;
      }
      g_free (test_name);
      
      if (p)
      {
	path = p + 1;
	p = strchr (path, ':');
      }
      else
	path = NULL;
    }
    g_free (free_path);
  }
  if (!tty->prg_name)
    tty->prg_name = g_strdup (prg_name);
  
  
  if (tty->vtemu)
    gtk_vtemu_reset (tty->vtemu, FALSE);
  
  if (!tty->tty_name)
  {
    g_assert (tty->pty_fd == -1); /* paranoid */
    
    gtk_tty_os_open_pty (tty);
    
    if (!tty->tty_name)
    {
      g_warning ("could not find a free pseudo teletype");
      return ;
    }
  }
  
  f_tty = fopen (tty->tty_name, "r+");
  if (!f_tty)
  {
    g_warning ("open(\"%s\") failed: %s", tty->tty_name, g_strerror (errno));
    gtk_tty_os_close_pty (tty);
    return ;
  }
  
  tty->pid = fork ();
  if (tty->pid < 0)
  {
    g_warning ("hum? fork() failed: %s", g_strerror (errno));
    fclose (f_tty);
    return ;
  }
  
  if (tty->pid == 0) /* child process */
  {
    if (envp == environ)
    {
      gchar *empty_term = "TERM=";

      if (!tty->vtemu)
	putenv (empty_term);
      else
      {
	gchar *buffer;
	
	buffer = g_new (gchar, strlen (empty_term) + strlen (tty->vtemu->terminal_type) + 1);
	buffer[0] = 0;
	strcat (buffer, empty_term);
	strcat (buffer, tty->vtemu->terminal_type);

	if (putenv (buffer) < 0)
	{
	  putenv (empty_term);
	}
      }

      envp = environ;
    }
    
    /* this function will _exit(-1) on errors*/
    gtk_tty_os_setup_tty (tty, f_tty, GTK_TTY_CLASS (GTK_OBJECT (tty)->klass)->os_hintp);
    
    execve (tty->prg_name, argv, envp);
    
    fprintf (stderr, "gtktty-clone: %s: %s\n", tty->prg_name, g_strerror (errno));
    _exit (-1);
  }
  
  /* parent process */
  
  fclose (f_tty);
  
  tty->input_tag = gdk_input_add (tty->pty_fd, GDK_INPUT_READ, gtk_tty_input_func, tty);
  
  gtk_signal_emit (GTK_OBJECT (tty),
		   tty_signals[PROGRAM_EXEC],
		   tty->prg_name,
		   argv,
		   envp);
}

void
gtk_tty_exec (GtkTty	     *tty,
	      const gchar    *filename,
	      const gchar    *arg,
	      ...)
{
  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  
  /* FIXME: call gtk_tty_execute() */
}

/* include the os specific functions */
#include "gtkttyos.c"
