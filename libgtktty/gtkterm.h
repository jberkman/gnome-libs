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
#ifndef __GTK_TERM_H__
#define __GTK_TERM_H__


#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TERM(obj)	       (GTK_CHECK_CAST (obj, gtk_term_get_type (), GtkTerm))
#define GTK_TERM_CLASS(klass)  (GTK_CHECK_CLASS_CAST (klass, gtk_term_get_type (), GtkTermClass))
#define GTK_IS_TERM(obj)       (GTK_CHECK_TYPE (obj, gtk_term_get_type ()))

#define GTK_TERM_MAX_COLORS    (8)

typedef enum
{
  GTK_CURSOR_INVISIBLE,
  GTK_CURSOR_UNDERLINE,
  GTK_CURSOR_BLOCK
} GtkCursorMode;

typedef struct _GtkTermAttrib GtkTermAttrib;
typedef struct _GtkTerm	      GtkTerm;
typedef struct _GtkTermClass  GtkTermClass;

struct	_GtkTermAttrib
{
  guchar flags;
  guchar i_fore;
  guchar i_back;
};


struct	_GtkTerm
{
  GtkWidget	widget;
  
  GdkWindow	*view_port;
  GdkWindow	*text_area;
  GdkGC		*text_gc;
  gboolean	refresh_blocked;
  gboolean	inverted;
  
  /* term dimensions in character
   */
  guint		max_term_width;
  guint		max_term_height;
  guint		term_width;
  guint		term_height;
  guint		first_line;
  guint		first_used_line;
  gint		scroll_offset;
  
  /* current text mode
   */
  guint		dim : 1;
  guint		bold : 1;
  guint 	underline : 1;
  guint		reverse : 1;
  guint		i_fore;
  guint		i_back;
  
  /* cursor position (absolute into buffer!)
   */
  GtkCursorMode	cursor_mode;
  guint		cursor_blinking : 1;
  guint		cur_x;
  guint		cur_y;
  guint		top;
  guint		bottom;
  
  /* cursor saving
   */
  guint		s_cur_x;
  guint		s_cur_y;
  guint		s_text_mode;
  guint		s_i_fore;
  guint		s_i_back;
  
  /* selections
   */
  guint		sel_valid : 1;
  guint		sel_b_x;
  guint		sel_b_y;
  guint		sel_e_x;
  guint		sel_e_y;
  guchar	*sel_buffer;
  guchar	sel_len;
  
  gulong	back[GTK_TERM_MAX_COLORS];
  gulong	fore[GTK_TERM_MAX_COLORS];
  gulong	fore_dim[GTK_TERM_MAX_COLORS];
  gulong	fore_bold[GTK_TERM_MAX_COLORS];
  
  /* mode appearance
   */
  GdkFont	*font_normal;
  GdkFont	*font_dim;
  GdkFont	*font_bold;
  GdkFont	*font_underline;
  GdkFont	*font_reverse;
  guint		overstrike_bold : 1;
  guint		draw_underline : 1;
  guint		colors_reversed : 1;
  
  /* character pixel dimensions
   */
  guint		char_width;
  guint		char_height;
  guint		char_vorigin;
  guint		char_descent;
  
  /* screen buffer
   */
  gchar		**char_buffer;
  GtkTermAttrib	**attrib_buffer;

  guint		flags_dirty : 1;
  gint		refresh_handler;
};

struct _GtkTermClass
{
  GtkWidgetClass		parent_class;
  
  gint				blink_handler;
  GList				*term_widgets;
  guint				blink_state : 1;
  
  void (* text_resize)		(GtkTerm	*term,
				 guint		*new_width,
				 guint		*new_height);
  gint	(* bell)		(GtkTerm	*term);
};


GtkType		gtk_term_get_type	(void);
void		gtk_term_construct	(GtkTerm	*term,
					 guint		width,
					 guint		height,
					 guint		max_width,
					 guint		scrollback);
gint		gtk_term_set_scroll_offset (GtkTerm	*term,
					    gint	offset);

/* --- terminal actions --- */
void		gtk_term_block_refresh	(GtkTerm	*term);
void		gtk_term_force_refresh	(GtkTerm	*term);
void		gtk_term_unblock_refresh(GtkTerm	*term);
void		gtk_term_set_fonts	(GtkTerm	*term,
					 GdkFont	*font_normal,
					 GdkFont	*font_dim,
					 GdkFont	*font_bold,
					 gboolean	overstrike_bold,
					 GdkFont	*font_underline,
					 gboolean	draw_underline,
					 GdkFont	*font_reverse,
					 gboolean	colors_reversed);
void		gtk_term_set_color	(GtkTerm	*term,
					 guint		index,
					 gulong		back,
					 gulong		fore,
					 gulong		fore_dim,
					 gulong		fore_bold);


void		gtk_term_select_color	(GtkTerm	*term,
					 guint		fore_index,
					 guint		back_index);
void		gtk_term_set_dim	(GtkTerm	*term,
					 gboolean	dim);
void		gtk_term_set_bold	(GtkTerm	*term,
					 gboolean	bold);
void		gtk_term_set_underline	(GtkTerm	*term,
					 gboolean	underline);
void		gtk_term_set_reverse	(GtkTerm	*term,
					 gboolean	reverse);
void		gtk_term_invert		(GtkTerm	*term);

/* termcap facility: AL
 * the line under cursor moves downwards n times, empty ones are inserted
 */
void		gtk_term_insert_lines	(GtkTerm	*term,
					 guint		n);
/* termcap facility: DL
 * delete lines under cursor, insert empty lines at bottom
 */
void		gtk_term_delete_lines	(GtkTerm	*term,
					 guint		n);
void		gtk_term_scroll		(GtkTerm	*term,
					 guint		n,
					 gboolean	downwards);
/* termcap facility: cb/ce
 * clear the cursor's character and optionaly surrounding parts of line
 */
void		gtk_term_clear_line	(GtkTerm	*term,
					 gboolean	before_cursor,
					 gboolean	after_cursor);
/* termcap facility: IC
 * insert n space(s) under cursors, the ones at end of line are skipped
 */
void		gtk_term_insert_chars	(GtkTerm	*term,
					 guint		n);
/* termcap facility: DC
 * delete character(s) under cursor and insert spaces at end of line
 */
void		gtk_term_delete_chars	(GtkTerm	*term,
					 guint		n);
/* termcap facility: bl
 * produce an audible bell (do we need that in here?)
 */
void		gtk_term_bell		(GtkTerm	*term);
/* termcap facility: cd
 * clear the cursor's character and optionaly surrounding parts of screen
 */
void		gtk_term_clear		(GtkTerm	*term,
					 gboolean	before_cursor,
					 gboolean	after_cursor);
/* termcap facility: cm
 * set the cursors coordinates, origin is upper left (0,0)
 */
void		gtk_term_set_cursor	(GtkTerm	*term,
					 guint		x,
					 guint		y);
void		gtk_term_get_cursor	(GtkTerm	*term,
					 guint		*x,
					 guint		*y);
void		gtk_term_set_cursor_mode(GtkTerm	*term,
					 GtkCursorMode	mode,
					 gboolean	blinking);
void		gtk_term_save_cursor	(GtkTerm	*term);
void		gtk_term_restore_cursor	(GtkTerm	*term);
void		gtk_term_set_scroll_reg	(GtkTerm	*term,
					 guint		top,
					 guint		bottom);
void		gtk_term_get_cursor	(GtkTerm	*term,
					 guint		*x,
					 guint		*y);
void		gtk_term_reset		(GtkTerm	*term);
void		gtk_term_get_size	(GtkTerm	*term,
					 guint		*width,
					 guint		*height);
/* termcap facility: ec
 * starting at cursor overtype n characters with space(s)
 */
void		gtk_term_erase_chars	(GtkTerm	*term,
					 guint		n);
/* put out char at cursor position, move cursor one to the right.
 * return wether cursor needs wrap (no cursor move in this case)
 */
gboolean	gtk_term_putc		(GtkTerm	*term,
					 guchar		ch,
					 gboolean	insert);






#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_TERM_H__ */
