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
#ifndef __GTK_TTY_H__
#define __GTK_TTY_H__


#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>
#include "gtkvtemu.h"
#include "gtkled.h"


#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */


#define GTK_TTY(obj)		GTK_CHECK_CAST (obj, gtk_tty_get_type (), GtkTty)
#define GTK_TTY_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gtk_tty_get_type (), GtkTtyClass)
#define GTK_IS_TTY(obj)		GTK_CHECK_TYPE (obj, gtk_tty_get_type ())


typedef	struct	_GtkTty			GtkTty;
typedef	struct	_GtkTtyClass		GtkTtyClass;
typedef	struct	_GtkTtyUpdateLed	GtkTtyUpdateLed;

typedef	enum
{
  GTK_TTY_STATE_NONE		=   0,
  GTK_TTY_STATE_SCROLL_LOCK	=   1,
  GTK_TTY_STATE_NUM_LOCK	=   2,
  GTK_TTY_STATE_SHIFT		=   4,
  GTK_TTY_STATE_CAPS_LOCK	=   8,
  GTK_TTY_STATE_CONTROL		=  16,
  GTK_TTY_STATE_ALT		=  32,
  GTK_TTY_STATE_SUPER		=  64,
  GTK_TTY_STATE_HYPER		= 128,
} GtkTtyKeyStateBits;


struct	_GtkTty
{
  GtkTerm	term;
  
  /* the key_states should always be up to date!
   * this is actually of type GtkTtyKeyStateBits.
   */
  guchar		key_states;
  
  gboolean		ignore_scroll_lock;
  gboolean		ignore_num_lock;
  gboolean		ignore_caps_lock;
  gboolean		freeze_leds;
  guchar		leds;
  GList			*update_leds;
  
  /* hehe, for the ones reading the source:
   * this supports <ALT>+keypad_ascii_code_entering if NumLock is on
   */
  gboolean		key_pad_enter;
  guchar		key_pad_char;
  
  gint			pty_fd;
  gchar			*tty_name;
  gint			input_tag;

  GtkVtEmu		*vtemu;

  /* child process
   * pid + exit information
   * FIXME: these fields could also be allocated on demand...
   */
  gint			pid;
  gchar			*prg_name;
  guchar		exit_status;
  guint			exit_signal;
  gulong		sys_usec;
  gulong		sys_sec;
  gulong		user_usec;
  gulong		user_sec;
  /* FIXME: child execution time measurement
   * gulong		total_usec;
   * gulong		total_sec;
   */
};

struct _GtkTtyClass
{
  GtkTermClass	parent_class;
  
  gpointer	os_hintp;
  
  gint (* key_press)	(GtkTty		*tty,
			 const gchar	*char_code,
			 guint		length,
			 guint		keyval,
			 guint		key_state);
  void (* program_exec)	(GtkTty		*tty,
			 const gchar	*prg_name,
			 gchar * const	argv[],
			 gchar * const	envp[]);
  void (* program_exit)	(GtkTty		*tty,
			 const gchar	*prg_name,
			 gchar		exit_status,
			 guint		exit_signal);
};

struct	_GtkTtyUpdateLed
{
  GtkLed		*led;
  GtkTtyKeyStateBits	mask;
};

guint		gtk_tty_get_type	(void);
GtkWidget	*gtk_tty_new		(guint		width,
					 guint		height,
					 guint		scrollback);
void		gtk_tty_put_out		(GtkTty		*tty,
					 const guchar	*buffer,
					 guint		count);
void		gtk_tty_put_in		(GtkTty		*tty,
					 const guchar	*buffer,
					 guint		count);
void		gtk_tty_leds_changed	(GtkTty		*tty);
void		gtk_tty_add_update_led	(GtkTty		*tty,
					 GtkLed		*led,
					 GtkTtyKeyStateBits mask);
void		gtk_tty_execute		(GtkTty		*tty,
					 const gchar	*prg_name,
					 gchar * const	argv[],
					 gchar * const	envp[]);
void		gtk_tty_exec		(GtkTty		*tty,
					 const gchar	*prg_name,
					 const gchar	*arg,
					 ...);
void		gtk_tty_change_emu	(GtkTty		*tty,
					 GtkVtEmu	*vtemu);






#ifdef __cplusplus
#pragma {
}
#endif /* __cplusplus */


#endif /* __GTK_TTY_H__ */
