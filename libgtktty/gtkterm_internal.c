/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkTerm (gtkterm_internal.c): internal functions for GtkTerm
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

/* --- macros --- */
#define	VIEW_PORT_WIDTH(term)		((term)->term_width * (term)->char_width)
#define	VIEW_PORT_HEIGHT(term)		((term)->term_height * (term)->char_height)
#define	TEXT_AREA_WIDTH(term)		((term)->max_term_width * (term)->char_width)
#define	TEXT_AREA_HEIGHT(term)		((term)->max_term_height * (term)->char_height)

#define FONT_HEIGHT(font)		((font)->ascent + (font)->descent)
#define ATTRIB_EQ(a1,a2)		( (a1).flags == (a2).flags   && \
					  (a1).i_fore == (a2).i_fore && \
					  (a1).i_back == (a2).i_back	)

#define	SEL_CHECK_RANGE(term)		\
{									      \
  guint tmp;								      \
  if ((term)->sel_e_y < (term)->sel_b_y)				      \
  {									      \
    tmp = (term)->sel_b_y; (term)->sel_b_y = (term)->sel_e_y;		      \
    (term)->sel_e_y = tmp;						      \
    tmp = (term)->sel_b_x; (term)->sel_b_x = (term)->sel_e_x;		      \
    (term)->sel_e_x = tmp;						      \
  }									      \
  else if ((term)->sel_b_y == (term)->sel_e_y &&			      \
	   (term)->sel_e_x < (term)->sel_b_x)				      \
  {									      \
    tmp = (term)->sel_b_x; (term)->sel_b_x = (term)->sel_e_x;		      \
    (term)->sel_e_x = tmp;						      \
  }									      \
}
#define	SEL_MAKE_VOID(term)		\
{									      \
  if (term->sel_valid)							      \
  {									      \
    term->sel_valid = FALSE;						      \
    if (GTK_WIDGET_DRAWABLE ((term)))					      \
      gtk_term_update_new_sel (term,					      \
			       term->sel_b_x,				      \
			       term->sel_b_y,				      \
			       term->sel_e_x,				      \
			       term->sel_e_y,				      \
			       term->sel_b_x,				      \
			       term->sel_b_y);				      \
  }									      \
}

#define	NEW_INPUT(term)		{\
  SEL_MAKE_VOID (term);\
}

#define	CURSOR_OFF(term)		{\
  if (GTK_WIDGET_DRAWABLE ((term)))\
    gtk_term_update_char ((term), (term)->cur_x, (term)->cur_y);\
}

#define	CURSOR_ON(term)			{\
  if ((term)->cursor_mode != GTK_CURSOR_INVISIBLE && GTK_WIDGET_DRAWABLE ((term)))\
    gtk_term_update_cursor ((term));\
}


#define	XY_2_I(mul_base,x,y)		( (guint) ( ((guint) (y)) * ((guint) (mul_base)) + ((guint) (x)) ) )
#define	I_2_Y(mul_base,i)		( (guint) ( ((guint) (i)) / ((guint) (mul_base)) ) )
#define	I_2_X(mul_base,i,y)		( (guint) ( ((guint) (i)) - ((guint) (y)) * ((guint) (mul_base)) ) )

#define	gtk_term_draw_char(t,x,y,a)	(gtk_term_draw_line_seg ((t), (y), (x), (x), (a)))
#define	gtk_term_update_char(t,x,y)	(gtk_term_draw_char ((t), (x), (y), &(t)->attrib_buffer[(y)][(x)]))


static	gboolean gtk_term_update_new_sel	(GtkTerm	*term,
						 const guint	origin_x,
						 const guint	origin_y,
						 const guint	end_x,
						 const guint	end_y,
						 const guint	new_x,
						 const guint	new_y);
static	void	gtk_term_line_init		(GtkTerm	*term,
						 guint		line);
static	void	gtk_term_update_cursor		(GtkTerm	*term);
static	void	gtk_term_update_area		(GtkTerm	*term,
						 GdkRectangle	*area);
static	void	gtk_term_update_line		(GtkTerm	*term,
						 guint		line,
						 guint		first_char,
						 guint		last_char);
static	void	gtk_term_draw_line_seg		(GtkTerm	*term,
						 guint		line,
						 guint		first_char,
						 guint		last_char,
						 GtkTermAttrib	*attrib);
static	void	gtk_term_update_region		(GtkTerm	*term,
						 guint		b_x,
						 guint		b_y,
						 guint		e_x,
						 guint		e_y);
static	void	gtk_term_scroll_up		(GtkTerm	*term,
						 guint		first_line,
						 guint		last_line,
						 guint		n_lines);
static	void	gtk_term_scroll_down		(GtkTerm	*term,
						 guint		first_line,
						 guint		last_line,
						 guint		n_lines);



static gboolean
gtk_term_update_new_sel (GtkTerm     *term,
			 const guint  origin_x,
			 const guint  origin_y,
			 const guint  end_x,
			 const guint  end_y,
			 const guint  new_x,
			 const guint  new_y)
{
  guint i_origin, i_end, i_new, i_clear1, i_clear2, i_mark1, i_mark2;
  guint pos, line, backwards;
  
  /* clear (.) :  i_clear1 - i_clear2
   * mark (#)  :   i_mark1 - i_mark2
   * new selection (const): i_origin, i_new_end
   *
   * forwrd selection:		   OOOOOO
   * need mark:			   OOOOOO####
   * need clear:		   OOO...
   *
   * need clear & mark:	      #####O.....
   *
   * backward selection:      OOOOOO
   * need mark:		  ####OOOOOO
   * need clear:	      ...OOO
   *
   * need clear & mark:	      .....######
   *
   * also, in general: clear if !term->sel_valid
   *
   * returns wether selection works backwards
   */
  
  i_origin = XY_2_I (term->term_width, origin_x, origin_y);
  i_end = XY_2_I (term->term_width, end_x, end_y);
  if (i_origin > i_end)
  {
    guint tmp;
    
    tmp = i_end;
    i_end = i_origin;
    i_origin = tmp;
    
    backwards = TRUE;
  }
  else
    backwards = FALSE;
  
  i_new = XY_2_I (term->term_width, new_x, new_y);
  
  if (!backwards)
  {
    if (i_new <= i_origin)
    {
      i_mark1 = i_new;
      i_mark2 = i_origin;
    }
    else if (i_new > i_end)
    {
      i_mark1 = i_end;
      i_mark2 = i_new;
    }
    else
    {
      i_mark2 = 0;
      i_mark1 = i_mark2 + 1;
    }
  }
  else
  {
    if (i_new <= i_origin)
    {
      i_mark1 = i_new;
      i_mark2 = i_origin;
    }
    else if (i_new > i_end)
    {
      i_mark1 = i_end;
      i_mark2 = i_new;
    }
    else
    {
      i_mark2 = 0;
      i_mark1 = i_mark2 + 1;
    }
  }
  
  if (!backwards)
  {
    i_clear2 = i_end;
    if (!term->sel_valid)
      i_clear1 = i_origin;
    else if (i_new < i_end)
      i_clear1 = MAX (i_new + 1, i_origin + 1);
    else
      i_clear1 = i_clear2 + 1;
  }
  else
  {
    i_clear1 = i_origin;
    if (!term->sel_valid)
      i_clear2 = i_end;
    else if (i_new > i_origin)
      i_clear2 = MIN (i_new - 1, i_end - 1);
    else
    {
      i_clear2 = 0;
      i_clear1 = i_clear2 + 1;
    }
  }
  
  /* clear FLAG_SELECTION
   */
  line = I_2_Y (term->term_width, i_clear1);
  pos = I_2_X (term->term_width, i_clear1, line);
  
  while (XY_2_I(term->term_width, pos, line) <= i_clear2)
  {
    term->attrib_buffer[line][pos].flags &= ~FLAG_SELECTION;
    pos++;
    if (pos > term->term_width - 1)
    {
      pos = 0;
      line++;
    }
  }
  if (i_clear1 <= i_clear2)
  {
    guint b_x, b_y, e_x, e_y;
    
    b_y = I_2_Y (term->term_width, i_clear1);
    b_x = I_2_X (term->term_width, i_clear1, b_y);
    e_y = I_2_Y (term->term_width, i_clear2);
    e_x = I_2_X (term->term_width, i_clear2, e_y);
    
    gtk_term_update_region (term, b_x, b_y, e_x, e_y);
  }
  
  /* mark FLAG_SELECTION
   */
  if (term->sel_valid)
  {
    line = I_2_Y (term->term_width, i_mark1);
    pos = I_2_X (term->term_width, i_mark1, line);
    
    while (XY_2_I(term->term_width, pos, line) <= i_mark2)
    {
      term->attrib_buffer[line][pos].flags |= FLAG_SELECTION;
      pos++;
      if (pos > term->term_width - 1)
      {
	pos = 0;
	line++;
      }
    }
    if (i_mark1 <= i_mark2)
    {
      guint b_x, b_y, e_x, e_y;
      
      b_y = I_2_Y (term->term_width, i_mark1);
      b_x = I_2_X (term->term_width, i_mark1, b_y);
      e_y = I_2_Y (term->term_width, i_mark2);
      e_x = I_2_X (term->term_width, i_mark2, e_y);
      
      gtk_term_update_region (term, b_x, b_y, e_x, e_y);
    }
  }
  
  return backwards;
}

static void
gtk_term_line_init (GtkTerm *term,
		    guint    line)
{
  GtkTermAttrib *attrib_line;
  gchar *char_line;
  guint i;
  
  char_line = term->char_buffer[line];
  attrib_line = term->attrib_buffer[line];
  
  for (i = 0; i < term->max_term_width; i++)
  {
    char_line[i] = gtk_term_blank_char;
    attrib_line[i] = gtk_term_blank_attrib;
  }
}

static void
gtk_term_update_area (GtkTerm	 *term,
		      GdkRectangle *area)
{
  GdkRectangle tmp_area;
  gint width, height;
  guint first_line, last_line;
  guint first_char, last_char;
  guint i;
  
  g_return_if_fail (term != NULL);
  g_return_if_fail (GTK_IS_TERM (term));
  
  gdk_window_get_size (term->text_area, &width, &height);
  
  if (!area)
  {
    area = &tmp_area;
    
    area->x = 0;
    area->y = 0;
    area->width = width;
    area->height = height;
  }
  
  if (area->width > 1)
    area->width--;
  else if (area->x > 0)
    area->x--;
  
  if (area->height > 1)
    area->height--;
  else if (area->y > 0)
    area->y--;
  
  first_line = area->y / term->char_height;
  last_line = (area->y + area->height) / term->char_height;
  first_char = area->x / term->char_width;
  last_char = (area->x + area->width) / term->char_width;
  
#if 0
  printf("DEBUG: GtkTerm: (%u,%u, %d,%d) area(%d,%d,%u,%u) -> box(%u,%u,%u,%u)\n",
	 term->char_width,
	 term->char_height,
	 width,
	 height,
	 area->x,
	 area->y,
	 area->width,
	 area->height,
	 first_char,
	 first_line,
	 last_char,
	 last_line);
#endif
  
  g_return_if_fail (first_line < term->max_term_height);
  g_return_if_fail (last_line < term->max_term_height);
  g_return_if_fail (first_char < term->max_term_width);
  g_return_if_fail (last_char < term->max_term_width);
  
  for (i = first_line; i <= last_line; i++)
  {
    gtk_term_update_line (term, i, first_char, last_char);
  }
  if (term->cur_y == CLAMP (term->cur_y - term->first_line, first_line, last_line) &&
      term->cur_x == CLAMP (term->cur_x, first_char, last_char))
    gtk_term_update_cursor (term);
}

static void
gtk_term_update_region (GtkTerm *term,
			guint	b_x,
			guint	b_y,
			guint	e_x,
			guint	e_y)
{
  while (b_y <= e_y)
  {
    guint x;
    
    if (b_y == e_y)
      x = e_x;
    else
      x = term->term_width - 1;
    
    gtk_term_update_line (term, b_y, b_x, x);
    b_y++;
    b_x = 0;
  }
}

static void
gtk_term_update_cursor (GtkTerm *term)
{
  GtkTermAttrib attrib;
  gboolean colors_reversed_saved;
  gboolean cursor_on;
  
  memcpy(&attrib, &term->attrib_buffer[term->cur_y][term->cur_x], sizeof (attrib));
  
  if (!term->cursor_blinking ||
      (term->cursor_blinking &&
       (!GTK_WIDGET_HAS_FOCUS (term) ||
	(GTK_WIDGET_HAS_FOCUS (term) &&
	 GTK_TERM_CLASS (GTK_OBJECT (term)->klass)->blink_state))))
    cursor_on = TRUE;
  else
    cursor_on = FALSE;
  
  if (cursor_on && term->cursor_mode == GTK_CURSOR_BLOCK)
    attrib.flags ^= FLAG_REVERSE;
  
  colors_reversed_saved = term->colors_reversed;
  term->colors_reversed = TRUE;
  
  gtk_term_draw_char (term, term->cur_x, term->cur_y, &attrib);
  
  term->colors_reversed = colors_reversed_saved;
  
  if (cursor_on && term->cursor_mode == GTK_CURSOR_UNDERLINE)
    gdk_draw_rectangle (term->text_area,
			term->text_gc,
			TRUE,
			term->char_width * term->cur_x,
			term->char_height * term->cur_y + term->char_vorigin,
			term->char_width,
			CURSOR_THIKNESS);
}

static void
gtk_term_update_line (GtkTerm *term,
		      guint    line,
		      guint    first_char,
		      guint    last_char)
{
  guint len, i;
  
  len = last_char - first_char + 1;
  
  i = first_char;
  while (i <= last_char) {
    GtkTermAttrib *attrib;
    guint first;
    
    first = i;
    
    attrib = &term->attrib_buffer[line][first];
    
    while (i < last_char && ATTRIB_EQ(term->attrib_buffer[line][i + 1], *attrib))
      i++;
    
    gtk_term_draw_line_seg (term, line, first, i, attrib);
    i++;
  }
}

static void
gtk_term_draw_line_seg (GtkTerm	      *term,
			guint	       line,
			guint	       first_char,
			guint	       last_char,
			GtkTermAttrib *attrib)
{
  guint len;
  GdkFont *font;
  GdkColor *fore;
  GdkColor *back;
  
  len = last_char - first_char + 1;
  
  back = term->back[attrib->i_back];
  if (attrib->flags & FLAG_SELECTION)
  {
    fore = & GTK_WIDGET (term)->style->fg[GTK_STATE_SELECTED];
    back = & GTK_WIDGET (term)->style->bg[GTK_STATE_SELECTED];
  }
  else if (attrib->flags & FLAG_BOLD)
    fore = term->fore_bold[attrib->i_fore];
  else if (attrib->flags & FLAG_DIM)
    fore = term->fore_dim[attrib->i_fore];
  else
    fore = term->fore[attrib->i_fore];
  
  if (attrib->flags & FLAG_REVERSE)
  {
    GdkColor *tmp;
    
    tmp = fore;
    fore = back;
    back = tmp;
  }
  
  if (attrib->flags & FLAG_DIM)
    font = term->font_dim;
  else if (attrib->flags & FLAG_BOLD)
    font = term->font_bold;
  else if (attrib->flags & FLAG_UNDERLINE)
    font = term->font_underline;
  else if (attrib->flags & FLAG_REVERSE)
    font = term->font_reverse;
  else
    font = term->font_normal;
  
  gdk_gc_set_foreground (term->text_gc, back);
  
  gdk_draw_rectangle (term->text_area,
		      term->text_gc,
		      TRUE,
		      term->char_width * first_char,
		      term->char_height * line,
		      term->char_width * len,
		      term->char_height);
  
  gdk_gc_set_foreground (term->text_gc, fore);
  
  gdk_draw_text (term->text_area,
		 font,
		 term->text_gc,
		 term->char_width * first_char,
		 term->char_height * line + term->char_vorigin,
		 &term->char_buffer[line][first_char],
		 len);
  
  if (attrib->flags & FLAG_UNDERLINE && term->draw_underline)
    gdk_draw_rectangle (term->text_area,
			term->text_gc,
			TRUE,
			term->char_width * first_char,
			term->char_height * line + term->char_vorigin + 1 - UNDERLINE_THIKNESS / 2,
			term->char_width * len,
			UNDERLINE_THIKNESS);
  
  if (attrib->flags & FLAG_BOLD && term->overstrike_bold)
    gdk_draw_text (term->text_area,
		   font,
		   term->text_gc,
		   term->char_width * first_char + 1,
		   term->char_height * line + term->char_vorigin,
		   &term->char_buffer[line][first_char],
		   len);
}

static void
gtk_term_scroll_up (GtkTerm	*term,
		    guint	first_line,
		    guint	last_line,
		    guint	n_lines)
{
  gchar	**char_buffer;
  GtkTermAttrib ** attrib_buffer;
  guint i;
  
  if (first_line == term->first_line)
    first_line = 0;
  
  if (term->first_used_line > first_line)
  {
    if (term->first_used_line > n_lines)
      term->first_used_line -= n_lines;
    else
      term->first_used_line = first_line;
  }
  
  char_buffer = g_new (gchar*, n_lines);
  attrib_buffer = g_new (GtkTermAttrib*, n_lines);
  
  for (i = 0; i < n_lines; i++)
  {
    char_buffer[i] = term->char_buffer[first_line + i];
    attrib_buffer[i] = term->attrib_buffer[first_line + i];
  }
  
  CURSOR_OFF (term);
  
  for (i = first_line; i <= last_line - n_lines; i++)
  {
    term->char_buffer[i] = term->char_buffer[i + n_lines];
    term->attrib_buffer[i] = term->attrib_buffer[i + n_lines];
  }
  
  for (i = 0; i < n_lines; i++)
  {
    term->char_buffer[last_line - i] = char_buffer[i];
    term->attrib_buffer[last_line - i] = attrib_buffer[i];
    
    gtk_term_line_init (term, last_line - i);
  }
  
  g_free (char_buffer);
  g_free (attrib_buffer);
  
  if (GTK_WIDGET_DRAWABLE (term))
  {
    gdk_window_copy_area (term->text_area,
			  term->text_gc,
			  0,
			  first_line * term->char_height,
			  NULL,
			  0,
			  (first_line + n_lines) * term->char_height,
			  term->term_width * term->char_width,
			  (1 + last_line - first_line - n_lines) * term->char_height);
    gdk_window_clear_area (term->text_area,
			   0,
			   (1 + last_line - n_lines) * term->char_height,
			   term->term_width * term->char_width,
			   n_lines * term->char_height);
    CURSOR_ON (term);
  }
}

static void
gtk_term_scroll_down (GtkTerm	*term,
		      guint	first_line,
		      guint	last_line,
		      guint	n_lines)
{
  gchar	**char_buffer;
  GtkTermAttrib ** attrib_buffer;
  guint i;
  
  char_buffer = g_new (gchar*, n_lines);
  attrib_buffer = g_new (GtkTermAttrib*, n_lines);
  
  for (i = 0; i < n_lines; i++)
  {
    char_buffer[i] = term->char_buffer[last_line - i];
    attrib_buffer[i] = term->attrib_buffer[last_line - i];
  }
  
  CURSOR_OFF (term);
  
  for (i = last_line; i >= first_line + n_lines; i--)
  {
    term->char_buffer[i] = term->char_buffer[i - n_lines];
    term->attrib_buffer[i] = term->attrib_buffer[i - n_lines];
  }
  
  for (i = 0; i < n_lines; i++)
  {
    term->char_buffer[first_line + i] = char_buffer[i];
    term->attrib_buffer[first_line + i] = attrib_buffer[i];
    
    gtk_term_line_init (term, first_line + i);
  }
  
  g_free (char_buffer);
  g_free (attrib_buffer);
  
  if (GTK_WIDGET_DRAWABLE (term))
  {
    gdk_window_copy_area (term->text_area,
			  term->text_gc,
			  0,
			  (first_line + n_lines) * term->char_height,
			  NULL,
			  0,
			  first_line * term->char_height,
			  term->term_width * term->char_width,
			  (1 + last_line - first_line - n_lines) * term->char_height);
    gdk_window_clear_area (term->text_area,
			   0,
			   first_line * term->char_height,
			   term->term_width * term->char_width,
			   n_lines * term->char_height);
    CURSOR_ON (term);
  }
}
