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
#endif /* __cplusplus */


#define GTK_TTY(obj)		(GTK_CHECK_CAST (obj, gtk_tty_get_type (), GtkTty))
#define GTK_TTY_CLASS(klass)	(GTK_CHECK_CLASS_CAST (klass, gtk_tty_get_type (), GtkTtyClass))
#define GTK_IS_TTY(obj)		(GTK_CHECK_TYPE (obj, gtk_tty_get_type ()))


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

typedef enum
{
  /* direct conversion
   */
  GTK_TKA_DC_MASK		= 0x000000ff,

  /* vt-emu sequence insertion
   */
  GTK_TKA_SEQ_0			=  0 << 8,
  GTK_TKA_SEQ_ESCAPE		=  1 << 8,
  GTK_TKA_SEQ_RETURN		=  2 << 8,
  GTK_TKA_SEQ_ENTER /* SEND */	=  3 << 8,
  GTK_TKA_SEQ_LINE_FEED		=  4 << 8,
  GTK_TKA_SEQ_LEFT		=  5 << 8,
  GTK_TKA_SEQ_RIGHT		=  6 << 8,
  GTK_TKA_SEQ_UP		=  7 << 8,
  GTK_TKA_SEQ_DOWN		=  8 << 8,
  GTK_TKA_SEQ_SCROLL_FORWARD	=  9 << 8,
  GTK_TKA_SEQ_SCROLL_BACKWARD	= 10 << 8,
  GTK_TKA_SEQ_PAGE_UP		= 11 << 8,
  GTK_TKA_SEQ_PAGE_DOWN		= 12 << 8,
  GTK_TKA_SEQ_BEGIN		= 13 << 8,
  GTK_TKA_SEQ_END		= 14 << 8,
  GTK_TKA_SEQ_EXIT		= 15 << 8,
  GTK_TKA_SEQ_F1		= 16 << 8,
  GTK_TKA_SEQ_F2		= 17 << 8,
  GTK_TKA_SEQ_F3		= 18 << 8,
  GTK_TKA_SEQ_F4		= 19 << 8,
  GTK_TKA_SEQ_F5		= 20 << 8,
  GTK_TKA_SEQ_F6		= 21 << 8,
  GTK_TKA_SEQ_F7		= 22 << 8,
  GTK_TKA_SEQ_F8		= 23 << 8,
  GTK_TKA_SEQ_F9		= 24 << 8,
  GTK_TKA_SEQ_F10		= 25 << 8,
  GTK_TKA_SEQ_F11		= 26 << 8,
  GTK_TKA_SEQ_F12		= 27 << 8,
  GTK_TKA_SEQ_SET_TAB		= 28 << 8,
  GTK_TKA_SEQ_CLEAR_TAB		= 29 << 8,
  GTK_TKA_SEQ_CLEAR_TABS	= 30 << 8,
  GTK_TKA_SEQ_BACK_TAB_STOP	= 31 << 8,
  GTK_TKA_SEQ_NEXT_TAB_STOP	= 32 << 8,
  GTK_TKA_SEQ_INSERT		= 33 << 8,
  GTK_TKA_SEQ_EXIT_INSERT	= 34 << 8,
  GTK_TKA_SEQ_BACKSPACE		= 35 << 8,
  GTK_TKA_SEQ_DELETE		= 36 << 8,
  GTK_TKA_SEQ_INSERT_LINE	= 37 << 8,
  GTK_TKA_SEQ_CLEAR_BOL		= 38 << 8,
  GTK_TKA_SEQ_CLEAR_LINE	= 39 << 8,
  GTK_TKA_SEQ_CLEAR_EOL		= 40 << 8,
  GTK_TKA_SEQ_CLEAR_BOS		= 41 << 8,
  GTK_TKA_SEQ_CLEAR_SCREEN	= 42 << 8,
  GTK_TKA_SEQ_CLEAR_EOS		= 43 << 8,
  GTK_TKA_SEQ_KP_ON		= 44 << 8,
  GTK_TKA_SEQ_KP_UPPER_LEFT	= 45 << 8,
  GTK_TKA_SEQ_KP_CENTER_KEY	= 46 << 8,
  GTK_TKA_SEQ_KP_UPPER_RIGHT	= 47 << 8,
  GTK_TKA_SEQ_KP_BOTTOM_LEFT	= 48 << 8,
  GTK_TKA_SEQ_KP_BOTTOM_RIGHT	= 49 << 8,
  GTK_TKA_SEQ_KP_OFF		= 50 << 8,
  GTK_TKA_SEQ_SELECT		= 51 << 8,
  GTK_TKA_SEQ_FIND		= 52 << 8,
  GTK_TKA_SEQ_UNDO		= 53 << 8,
  GTK_TKA_SEQ_REDO		= 54 << 8,
  GTK_TKA_SEQ_HELP		= 55 << 8,
  GTK_TKA_SEQ_MARK		= 56 << 8,
  GTK_TKA_SEQ_MESSAGE		= 57 << 8,
  GTK_TKA_SEQ_MOVE		= 58 << 8,
  GTK_TKA_SEQ_PREV_OBJECT	= 59 << 8,
  GTK_TKA_SEQ_NEXT_OBJECT	= 60 << 8,
  GTK_TKA_SEQ_OPEN		= 61 << 8,
  GTK_TKA_SEQ_SAVE		= 62 << 8,
  GTK_TKA_SEQ_CANCEL		= 63 << 8,
  GTK_TKA_SEQ_CREATE		= 64 << 8,
  GTK_TKA_SEQ_CLOSE		= 65 << 8,
  GTK_TKA_SEQ_COPY		= 66 << 8,
  GTK_TKA_SEQ_COMMAND		= 67 << 8,
  GTK_TKA_SEQ_OPTIONS		= 68 << 8,
  GTK_TKA_SEQ_PRINT		= 69 << 8,
  GTK_TKA_SEQ_REFRESH		= 70 << 8,
  GTK_TKA_SEQ_REFERENCE		= 71 << 8,
  GTK_TKA_SEQ_REPLACE		= 72 << 8,
  GTK_TKA_SEQ_RESTART		= 73 << 8,
  GTK_TKA_SEQ_SUSPEND		= 74 << 8,
  GTK_TKA_SEQ_RESUME		= 75 << 8,
  GTK_TKA_SEQ_MASK		= 0x0000ff00,

  /* primary action
   */
  GTK_TKA_P_0			=  0 << 16,
  GTK_TKA_P_SCROLL_UP		=  1 << 16,
  GTK_TKA_P_SCROLL_DOWN		=  2 << 16,
  GTK_TKA_P_LITERAL_INSERT	=  3 << 16,
  GTK_TKA_P_NUMERICAL_COMPOSE	=  4 << 16,
  GTK_TKA_P_KILL_PROGRAM	=  5 << 16,
  GTK_TKA_P_MAIN_QUIT		=  5 << 16,
  GTK_TKA_P_MASK		= 0x000f0000,

  /* action flags
   */
  GTK_TKA_DIRECT_CONVERSION	=  1 << 24,
  GTK_TKA_AT_SUBTRACT		=  1 << 25,
  GTK_TKA_ADD_META_ESCAPE	=  1 << 26,

  /* internal tag
   */
  GTK_TKA_TAG			=  1 << 31
} GtkTtyKeyActionType;

#define GTK_TKA(dc_char,vt_seq,p_action,zero,flags)      ( \
  GTK_TKA_SEQ_ ## vt_seq | \
  (dc_char) | \
  GTK_TKA_P_ ## p_action | \
  (flags))


struct	_GtkTty
{
  GtkTerm	term;
  
  /* the key_states should always be up to date!
   * this is actually of type GtkTtyKeyStateBits.
   */
  guchar		key_states;
  
  guchar		leds;
  GList			*update_leds;
  guint			ignore_scroll_lock : 1;
  guint			ignore_num_lock : 1;
  guint			ignore_caps_lock : 1;
  guint			freeze_leds : 1;
  
  /* hehe, for the ones reading the source:
   * this supports <ALT>+keypad_ascii_code_entering if NumLock is on
   */
  guint			key_pad_enter : 1;
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

  guint		meassure_time : 1;
  
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

GtkType		gtk_tty_get_type	(void);
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
}
#endif /* __cplusplus */


#endif /* __GTK_TTY_H__ */
