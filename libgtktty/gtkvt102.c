/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * gtkvt102: VT102 implementation for GtkVtEmu
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
#include <stdio.h>
#include <string.h>
#include "gtkvt102.h"
#include "config.h"


/* --- limits --- */
#define	GTKVT102_MAX_PARAMS	(16)

/* --- report strings --- */
#define VT100ID		("\033[?1;2c")
#define	VT102ID		("\033[?6c")
#define	VT_OK		("\033[0n")
#define	VT_TERM		("vt102")

/* --- macros --- */
#define	gtk_vt102_gotoxry(vtemu,x,y)	gtk_vt102_gotoxy(&(vtemu)->pub, (x), (vtemu)->relative_origin ? (vtemu)->pub.term->top + (y) : (y))

/* --- typedefs --- */
typedef enum
{
  GTKVT102_ESC_Normal,
  GTKVT102_ESC_Escape,
  GTKVT102_ESC_NonStd,
  GTKVT102_ESC_Percent,
  GTKVT102_ESC_Square,
  GTKVT102_ESC_GetParams,
  GTKVT102_ESC_GotParams,
  GTKVT102_ESC_Set_G0,
  GTKVT102_ESC_Set_G1,
  GTKVT102_ESC_Hash,
  GTKVT102_ESC_Palette,
  GTKVT102_ESC_FuncKey
} GtkVt102EscStates;

typedef	struct	_GtkVt102Emu	GtkVt102Emu;

struct	_GtkVt102Emu
{
  GtkVtEmu		pub;
  
  GtkVt102EscStates	state;
  gboolean		toggle_meta;
  gboolean		disp_ctrl;
  guint32		display_bitmap;
  guint32		display_bitmap_restricted;
  gboolean		dec_priv_mode;
  gboolean		relative_origin;
  GtkCursorMode		cursor_mode;
  gboolean		cursor_blinking;
  gchar			*charset_table;
  gulong		tab_stop[5];
  guint			n_params;
  gulong		param[GTKVT102_MAX_PARAMS];
};

/* --- prototypes --- */
static	void	gtk_vt102_lf		(GtkVtEmu	*base_emu);
static	void	gtk_vt102_reverse_lf	(GtkVtEmu	*base_emu);
static	void	gtk_vt102_cr		(GtkVtEmu	*base_emu);
static	void	gtk_vt102_bs		(GtkVtEmu	*base_emu);
static	void	gtk_vt102_gotoxy	(GtkVtEmu	*base_emu,
					 guint		new_x,
					 guint		new_y);
static	void	gtk_vt102_set_mode	(GtkVtEmu	*base_emu,
					 gboolean	on_off);
static	void	gtk_vt102_set_term_mode	(GtkVtEmu	*base_emu);
static	void	gtk_vt102_linux_setterm (GtkVtEmu	*base_emu);


/* --- GtkVtEmu Entry definition --- */
static	gchar	*gtk_vt102_types[] =
{
#ifdef	HAVE_TERMCAP_LINUX
  "linux",
  "linux-m",
#endif	/* HAVE_TERMCAP_LINUX */
  "vt102",
  "vt100",
#ifndef HAVE_TERMCAP_LINUX
  "linux",
  "linux-m",
#endif	/* !HAVE_TERMCAP_LINUX */
};
GtkVtEmuTermEntry	gtk_vt102_entry =
{
  GTK_VTEMU_ID_LINUX,
  sizeof (GtkVt102Emu),
  gtk_vt102_input,
  gtk_vt102_reset,
  gtk_vt102_types,
  sizeof (gtk_vt102_types) / sizeof (gtk_vt102_types[0]),
};


/* --- charset/font conversion tables --- */
#include	"gtkvt102c2f.c"


/* --- functions --- */

void
gtk_vt102_reset (GtkVtEmu	*base_emu,
		 gboolean	blank_screen)
{
  GtkVt102Emu *vtemu;
  guint i;
  
  g_assert (base_emu != NULL);
  g_assert (base_emu->internal_id == GTK_VTEMU_ID_LINUX);
  vtemu = (GtkVt102Emu*) base_emu;
  
  base_emu->led_override = FALSE;
  base_emu->cur_x = 0;
  base_emu->cur_y = 0;
  base_emu->insert_mode = FALSE;
  base_emu->lf_plus_cr = FALSE;
  base_emu->need_wrap = FALSE;

  if (base_emu->term_inverted)
    gtk_vtemu_invert (base_emu);
  
  vtemu->state = GTKVT102_ESC_Normal;
  vtemu->toggle_meta = FALSE;
  vtemu->disp_ctrl = FALSE;
  /* the control char display bitmap (and the restricted one)
   * are taken from the linux console
   */
  vtemu->display_bitmap
    = 0xffffffff &
    ~(0x00000000 |
      ( 1 << '\0') |
      ( 1 << '\b') |
      ( 1 << '\n') |
      ( 1 << '\f') |
      ( 1 << '\r') |
      ( 1 <<  14 ) | /* shift out */
      ( 1 <<  15 ) | /* shift in */
      ( 1 <<  27 ) | /* escape */
      0);
  vtemu->display_bitmap_restricted
    = 0xffffffff &
    ~(0x00000000 |
      ( 1 << '\0') |
      ( 1 << '\a') |
      ( 1 << '\b') |
      ( 1 << '\t') |
      ( 1 << '\n') |
      ( 1 << '\v') |
      ( 1 << '\f') |
      ( 1 << '\r') |
      ( 1 <<  14 ) | /* shift out */
      ( 1 <<  15 ) | /* shift in */
      ( 1 <<  24 ) | /* CAN */
      ( 1 <<  26 ) | /* SUB */
      ( 1 <<  27 ) | /* escape */
      0);
  vtemu->dec_priv_mode = FALSE;
  vtemu->relative_origin = FALSE;
  vtemu->cursor_mode = base_emu->dfl_cursor_mode;
  vtemu->cursor_blinking = base_emu->dfl_cursor_blinking;
  vtemu->charset_table = NULL;
  vtemu->tab_stop[0] = 0x01010100;
  vtemu->tab_stop[1] = 0x01010101;
  vtemu->tab_stop[2] = 0x01010101;
  vtemu->tab_stop[3] = 0x01010101;
  vtemu->tab_stop[4] = 0x01010101;
  vtemu->n_params = 0;
  for (i = 0; i < GTKVT102_MAX_PARAMS; i++)
    vtemu->param[i] = 0;
  
  gtk_term_set_cursor_mode (base_emu->term, vtemu->cursor_mode, vtemu->cursor_blinking);
  gtk_term_reset (base_emu->term);
  if (blank_screen)
    gtk_term_clear (base_emu->term, TRUE, TRUE);
}

guint
gtk_vt102_input (GtkVtEmu	*base_emu,
		 const guchar	*buffer,
		 guint		count)
{
  /* this function is modeled much after
   * linux-2.0.29/drivers/char/console.c:con_write().
   * for the moment we skip unicode and charset conversions
   */
  GtkVt102Emu *vtemu;
  GtkTerm *term;
  guint n_processed;
  guint old_x, old_y;
  
  g_assert (base_emu != NULL);
  g_assert (base_emu->internal_id == GTK_VTEMU_ID_LINUX);
  vtemu = (GtkVt102Emu*) base_emu;
  
  term = base_emu->term;

  /*
   * printf("buffer_count: %d\n", count);
   */
  
  gtk_term_block_refresh (term);
  
  gtk_term_set_cursor_mode (term, GTK_CURSOR_INVISIBLE, vtemu->cursor_blinking);
  
  gtk_term_get_cursor (term, &base_emu->cur_x, &base_emu->cur_y);
  old_x = base_emu->cur_x;
  old_y = base_emu->cur_y;
  
  n_processed = 0;
  while (n_processed < count)
  {
    guchar ch;
    gboolean display_ok;
    
    if (base_emu->cur_x != old_x ||
	base_emu->cur_y != old_y)
    {
      gtk_term_set_cursor (term, base_emu->cur_x, base_emu->cur_y);
      old_x = base_emu->cur_x;
      old_y = base_emu->cur_y;
    }
    
    ch = buffer[n_processed++];
    display_ok = (ch
		  && (ch >= 32 ||
		      (((vtemu->disp_ctrl ?
			 vtemu->display_bitmap :
			 vtemu->display_bitmap_restricted) >> ch) & 1))
		  && (ch != 127 || vtemu->disp_ctrl)
		  && (ch != 128+27));
    
    /* write normal char
     */
    if (vtemu->state == GTKVT102_ESC_Normal && display_ok)
    {
      if (base_emu->need_wrap)
      {
	gtk_vt102_cr (base_emu);
	gtk_vt102_lf (base_emu);
      }
      
      gtk_term_set_scroll_offset (term, 0);
      if (vtemu->charset_table)	/* FIXME charset conversion */
      {
	ch = vtemu->charset_table[vtemu->toggle_meta ? ch | 0x80 : ch];
      }

      base_emu->need_wrap = gtk_term_putc (term, ch, base_emu->insert_mode);

      gtk_term_get_cursor (term, &base_emu->cur_x, &base_emu->cur_y);
      
      continue;
    }
    
    /* control characters can be used in the middle of an escape sequence
     */
    switch (ch)
    {
    case  0:
      continue;
      
    case  '\a': /* audible bell */
      gtk_term_bell (term);
      continue;
      
    case  '\b': /* backspace */
      gtk_vt102_bs (base_emu);
      continue;
      
    case  '\t': /* tabulator */
      while (base_emu->cur_x < MIN (160, term->term_width) - 1)
      {
	base_emu->cur_x++;
	if (vtemu->tab_stop[base_emu->cur_x >> 5] & (1 << (base_emu->cur_x & 31)))
	{
	  base_emu->need_wrap = FALSE;
	  break;
	}
      }
      gtk_term_set_cursor (term, base_emu->cur_x, base_emu->cur_y);
      continue;
      
    case  '\n': /* linefeed */
    case  '\v': /* vertical tabulator */
    case  '\f': /* formfeed */
      gtk_vt102_lf (base_emu);
      if (base_emu->lf_plus_cr)
	gtk_vt102_cr (base_emu);
      continue;
      
    case  '\r': /* carriage return */
      gtk_vt102_cr (base_emu);
      continue;
      
    case  14: /* shift out */
      vtemu->charset_table = c2f_vt102_any; /* FIXME */
      vtemu->charset_table = c2f_vt102_misc;
      continue;
      
    case  15: /* shift in */
      vtemu->charset_table = NULL;
      continue;
      
    case  24: /* cancel */
    case  26: /* SUB */
      vtemu->state = GTKVT102_ESC_Normal;
      continue;
      
    case  27: /* escape */
      vtemu->state = GTKVT102_ESC_Escape;
      continue;
      
    case  128 + 27: /* ??? */
      vtemu->state = GTKVT102_ESC_Square;
      continue;
    }
    
    /* handle various escape states
     */
    switch (vtemu->state)
    {
    case  GTKVT102_ESC_Escape:
      vtemu->state = GTKVT102_ESC_Normal;
      switch (ch)
      {
      case  '[':
	vtemu->state = GTKVT102_ESC_Square;
	continue;
	
      case  ']':
	vtemu->state = GTKVT102_ESC_NonStd;
	continue;
	
      case  '%':
	vtemu->state = GTKVT102_ESC_Percent;
	continue;
	
      case  'E':
	gtk_vt102_cr (base_emu);
	gtk_vt102_lf (base_emu);
	continue;
	
      case  'M':
	gtk_vt102_reverse_lf (base_emu);
	continue;
	
      case  'D':
	gtk_vt102_lf (base_emu);
	continue;
	
      case 'H':
	vtemu->tab_stop[MIN (160, base_emu->cur_x) >> 5] |= (1 << (base_emu->cur_x & 31));
	continue;
	
      case 'Z':
	gtk_vtemu_report (base_emu, VT102ID, strlen (VT102ID));
	continue;
	
      case '7':
	gtk_term_save_cursor (term);
	continue;
	
      case '8':
	gtk_term_restore_cursor (term);
	gtk_term_get_cursor (term, &base_emu->cur_x, &base_emu->cur_y);
	base_emu->need_wrap = FALSE;
	continue;
	
      case '(':
	vtemu->state = GTKVT102_ESC_Set_G0;
	continue;
	
      case ')':
	vtemu->state = GTKVT102_ESC_Set_G1;
	continue;
	
      case '#':
	vtemu->state = GTKVT102_ESC_Hash;
	continue;
	
      case 'c':
	gtk_vt102_reset (base_emu, TRUE);
	continue;
	
      case '>':
	/* FIXME: numeric keypad */
	continue;
	
      case '=':
	/* FIXME: application keypad */
	continue;
      }
      continue;
      
    case  GTKVT102_ESC_NonStd:
      if (ch == 'P') /* palette escape sequence */
      {
	for (vtemu->n_params = 0; vtemu->n_params < GTKVT102_MAX_PARAMS; vtemu->n_params++)
	  vtemu->param[vtemu->n_params] = 0;
	vtemu->n_params = 0;
	vtemu->state = GTKVT102_ESC_Palette;
	continue;
      }
      else if (ch == 'R') /* reset palette */
	vtemu->state = GTKVT102_ESC_Normal;
      else
	vtemu->state = GTKVT102_ESC_Normal;
      continue;
      
    case GTKVT102_ESC_Palette:
      if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' &&ch <= 'f'))
      {
	vtemu->param[vtemu->n_params++] = (ch > '9' ? (ch & 0xDF) - 'A' + 10 : ch - '0');
	if (vtemu->n_params == 7)
	  vtemu->state = GTKVT102_ESC_Normal;
      }
      else
	vtemu->state = GTKVT102_ESC_Normal;
      continue;
      
    case GTKVT102_ESC_Square:
      for (vtemu->n_params = 0; vtemu->n_params < GTKVT102_MAX_PARAMS; vtemu->n_params++)
	vtemu->param[vtemu->n_params] = 0;
      vtemu->n_params = 0;
      vtemu->state = GTKVT102_ESC_GetParams;
      if (ch == '[') /* function key */
      {
	vtemu->state = GTKVT102_ESC_FuncKey;
	continue;
      }
      vtemu->dec_priv_mode = ch == '?';
      if (vtemu->dec_priv_mode)
	continue;
    case  GTKVT102_ESC_GetParams:
      if (ch == ';' && vtemu->n_params < GTKVT102_MAX_PARAMS - 1)
      {
	vtemu->n_params++;
	continue;
      }
      else if (ch >= '0' && ch <= '9')
      {
	vtemu->param[vtemu->n_params] *= 10;
	vtemu->param[vtemu->n_params] += ch - '0';
	continue;
      }
      else
	vtemu->state = GTKVT102_ESC_GotParams;
    case  GTKVT102_ESC_GotParams:
      vtemu->state = GTKVT102_ESC_Normal;
      switch (ch)
      {
	
      case  'h':
	gtk_vt102_set_mode (base_emu, TRUE);
	continue;
	
      case 'l':
	gtk_vt102_set_mode (base_emu, FALSE);
	continue;
	
      case 'n':
	if (!vtemu->dec_priv_mode)
	  {
	    if (vtemu->param[0] == 5)
	      gtk_vtemu_report (base_emu, VT_OK, strlen (VT_OK));
	    else if (vtemu->param[0] == 6)
	      {
		guchar string[40];
		
		sprintf (string,
			 "\033[%d;%dR",
			 base_emu->cur_y + (vtemu->relative_origin ?
					           term->top + 1 : 1),
			 base_emu->cur_x + 1);
		gtk_vtemu_report (base_emu, string, strlen (string));
	      }
	  }
	continue;
      }
      if (vtemu->dec_priv_mode)
      {
	vtemu->dec_priv_mode = FALSE;
	continue;
      }
      switch(ch)
      {
	
      case  'G':
      case  '`':
	if (vtemu->param[0])
	  vtemu->param[0]--;
	gtk_vt102_gotoxy (base_emu, vtemu->param[0], base_emu->cur_y);
	continue;
	
      case  'A':
	if (!vtemu->param[0])
	  vtemu->param[0] = 1;
	if (vtemu->param[0] > base_emu->cur_y)
	  vtemu->param[0] = base_emu->cur_y;
	gtk_vt102_gotoxy (base_emu, base_emu->cur_x, base_emu->cur_y - vtemu->param[0]);
	continue;
	
      case  'B':
      case  'e':
	if (!vtemu->param[0])
	  vtemu->param[0] = 1;
	gtk_vt102_gotoxy (base_emu, base_emu->cur_x, base_emu->cur_y + vtemu->param[0]);
	continue;
	
      case  'C':
      case  'a':
	if (!vtemu->param[0])
	  vtemu->param[0] = 1;
	gtk_vt102_gotoxy (base_emu, base_emu->cur_x + vtemu->param[0], base_emu->cur_y);
	continue;
	
      case  'D':
	if (!vtemu->param[0])
	  vtemu->param[0] = 1;
	if (vtemu->param[0] > base_emu->cur_x)
	  vtemu->param[0] = base_emu->cur_x;
	gtk_vt102_gotoxy (base_emu, base_emu->cur_x - vtemu->param[0], base_emu->cur_y);
	continue;
	
      case  'E':
	if (!vtemu->param[0])
	  vtemu->param[0] = 1;
	gtk_vt102_gotoxy (base_emu, 0, base_emu->cur_y + vtemu->param[0]);
	continue;
	
      case  'F':
	if (!vtemu->param[0])
	  vtemu->param[0] = 1;
	gtk_vt102_gotoxy (base_emu, 0, base_emu->cur_y - vtemu->param[0]);
	continue;
	
      case  'd':
	if (vtemu->param[0])
	  vtemu->param[0]--;
	gtk_vt102_gotoxry (vtemu, base_emu->cur_x, vtemu->param[0]);
	continue;
	
      case  'H':
      case  'f':
	if (vtemu->param[0])
	  vtemu->param[0]--;
	if (vtemu->param[1])
	  vtemu->param[1]--;
	gtk_vt102_gotoxry (vtemu, vtemu->param[1], vtemu->param[0]);
	continue;
	
      case  'J':
	gtk_term_clear (term,
			vtemu->param[0] == 1 ||
			vtemu->param[0] == 2,
			vtemu->param[0] == 0 ||
			vtemu->param[0] == 2);
	base_emu->need_wrap = FALSE;
	continue;
	
      case  'K':
	gtk_term_clear_line (term,
			     vtemu->param[0] == 1 ||
			     vtemu->param[0] == 2,
			     vtemu->param[0] == 0 ||
			     vtemu->param[0] == 2);
	base_emu->need_wrap = FALSE;
	continue;
	
      case  'L':
	gtk_term_insert_lines (term, vtemu->param[0]);
	base_emu->need_wrap = FALSE;
	continue;
	
      case  'M':
	gtk_term_delete_lines (term, vtemu->param[0]);
	base_emu->need_wrap = FALSE;
	continue;
	
      case  'P':
	gtk_term_delete_chars (term, vtemu->param[0]);
	base_emu->need_wrap = FALSE;
	continue;
	
      case  'c':
	if (!vtemu->param[0])
	  gtk_vtemu_report (base_emu, VT102ID, strlen (VT102ID));
	continue;
	
      case  'g':
	if (!vtemu->param[0])
	  vtemu->tab_stop[MIN (base_emu->cur_x, 160) >> 5] &= ~(1 << (MIN (base_emu->cur_x, 160) & 31));
	else if (vtemu->param[0] == 3)
	{
	  vtemu->tab_stop[0] = 0;
	  vtemu->tab_stop[1] = 0;
	  vtemu->tab_stop[2] = 0;
	  vtemu->tab_stop[3] = 0;
	  vtemu->tab_stop[4] = 0;
	}
	continue;
	
      case  'm':
	gtk_vt102_set_term_mode (base_emu);
	continue;
	
      case  'q':
	if (vtemu->param[0] < 8)
	{
	  base_emu->led_override = TRUE;
	  if (!vtemu->param[0])
	    base_emu->led_states = 0;
	  else
	    base_emu->led_states = 1 << (vtemu->param[0] - 1);
	}
	continue;
	
      case  'r':
	if (!vtemu->param[0])
	  vtemu->param[0] = 1;
	if (!vtemu->param[1])
	  vtemu->param[1] = term->term_height;
	if (vtemu->param[0] < vtemu->param[1] &&
	    vtemu->param[1] <= term->term_height)
	{
	  gtk_term_set_scroll_reg (term, vtemu->param[0] - 1, vtemu->param[1] - 1);
	  gtk_vt102_gotoxry (vtemu, 0, 0);
	}
	continue;
	
      case  's':
	gtk_term_save_cursor (term);
	continue;
	
      case  'u':
	gtk_term_restore_cursor (term);
	gtk_term_get_cursor (term, &base_emu->cur_x, &base_emu->cur_y);
	base_emu->need_wrap = FALSE;
	continue;
	
      case  'X':
	gtk_term_erase_chars (term, vtemu->param[0]);
	base_emu->need_wrap = FALSE;
	continue;
	
      case  '@':
	gtk_term_insert_chars (term, vtemu->param[0]);
	continue;
	
      case  ']':
	gtk_vt102_linux_setterm (base_emu);
	continue;
      }
      continue;
      
    case  GTKVT102_ESC_Percent:
      vtemu->state = GTKVT102_ESC_Normal;
      continue;
      
    case  GTKVT102_ESC_FuncKey:
      vtemu->state = GTKVT102_ESC_Normal;
      continue;
      
    case  GTKVT102_ESC_Hash:
      vtemu->state = GTKVT102_ESC_Normal;
      continue;
      
    case  GTKVT102_ESC_Set_G0:
      vtemu->state = GTKVT102_ESC_Normal;
      if (ch == '0')
      {
	/* FIXME: charset_G0 = GRAF_MAP */
      }
      else if (ch == 'B')
      {
	/* FIXME: charset_G0 = LAT1_MAP */
      }
      else if (ch == 'U')
      {
	/* FIXME: charset_G0 = IBMPC_MAP */
      }
      else if (ch == 'K')
      {
	/* FIXME: charset_G0 = USER_MAP */
      }
      continue;
      
    case  GTKVT102_ESC_Set_G1:
      vtemu->state = GTKVT102_ESC_Normal;
      if (ch == '0')
      {
	/* FIXME: charset_G1 = GRAF_MAP */
      }
      else if (ch == 'B')
      {
	/* FIXME: charset_G1 = LAT1_MAP */
      }
      else if (ch == 'U')
      {
	/* FIXME: charset_G1 = IBMPC_MAP */
      }
      else if (ch == 'K')
      {
	/* FIXME: charset_G1 = USER_MAP */
      }
      continue;
      
    case  GTKVT102_ESC_Normal:
      continue;
      
    default:
      vtemu->state = GTKVT102_ESC_Normal;
    }
  }
  
  gtk_term_set_cursor_mode (term, vtemu->cursor_mode, vtemu->cursor_blinking);

  gtk_term_unblock_refresh (term);
  
  return n_processed;
}

static void
gtk_vt102_lf (GtkVtEmu		*base_emu)
{
  g_assert (base_emu != NULL && base_emu->internal_id == GTK_VTEMU_ID_LINUX);
  
  if (base_emu->cur_y == base_emu->term->bottom)
    gtk_term_scroll (base_emu->term, 1, FALSE);
  else if (base_emu->cur_y < base_emu->term->term_height - 1)
  {
    base_emu->cur_y++;
    gtk_term_set_cursor (base_emu->term, base_emu->cur_x, base_emu->cur_y);
  }
  base_emu->need_wrap = FALSE;
}

static void
gtk_vt102_reverse_lf (GtkVtEmu		*base_emu)
{
  if (base_emu->cur_y == base_emu->term->top)
    gtk_term_scroll (base_emu->term, 1, TRUE);
  else if (base_emu->cur_y > 0)
  {
    base_emu->cur_y--;
    gtk_term_set_cursor (base_emu->term, base_emu->cur_x, base_emu->cur_y);
  }
  base_emu->need_wrap = FALSE;
}

static void
gtk_vt102_cr (GtkVtEmu	*base_emu)
{
  if (base_emu->cur_x != 0)
  {
    base_emu->cur_x = 0;
    gtk_term_set_cursor (base_emu->term, base_emu->cur_x, base_emu->cur_y);
  }
  base_emu->need_wrap = FALSE;
}

static void
gtk_vt102_bs (GtkVtEmu	*base_emu)
{
  if (base_emu->cur_x)
  {
    base_emu->cur_x--;
    gtk_term_set_cursor (base_emu->term, base_emu->cur_x, base_emu->cur_y);
    base_emu->need_wrap = FALSE;
  }
}

static void
gtk_vt102_gotoxy (GtkVtEmu	*base_emu,
		  guint		new_x,
		  guint		new_y)
{
  GtkVt102Emu *vtemu;
  guint min_y, max_y;
  guint height, width;
  
  g_assert (base_emu != NULL);
  g_assert (base_emu->internal_id == GTK_VTEMU_ID_LINUX);
  vtemu = (GtkVt102Emu*) base_emu;
  
  height = base_emu->term->term_height;
  width = base_emu->term->term_width;
  
  if (new_x >= width)
    base_emu->cur_x = width - 1;
  else
    base_emu->cur_x = new_x;
  
  if (vtemu->relative_origin)
  {
    min_y = base_emu->term->top;
    max_y = base_emu->term->bottom;
  }
  else
  {
    min_y = 0;
    max_y = height - 1;
  }
  
  if (new_y < min_y)
    base_emu->cur_y = min_y;
  else if (new_y > max_y)
    base_emu->cur_y = max_y;
  else
    base_emu->cur_y = new_y;
  base_emu->need_wrap = FALSE;
  
  gtk_term_set_cursor (base_emu->term, base_emu->cur_x, base_emu->cur_y);
}

static void
gtk_vt102_linux_setterm (GtkVtEmu	*base_emu)
{
  GtkVt102Emu *vtemu;
  
  g_assert (base_emu != NULL);
  g_assert (base_emu->internal_id == GTK_VTEMU_ID_LINUX);
  vtemu = (GtkVt102Emu*) base_emu;
  
  switch(vtemu->param[0])
  {
    
  case 1:
    /* set color for underline mode */
    break;
    
  case 2:
    /* set color for half intensity mode */
    break;
    
  case 8:
    /* store colors as defaults */
    break;
    
  case 9:
    /* set blanking interval */
    break;
    
  case 10:
    /* set bell frequency in Hz from param[1] */
    break;
    
  case 11:
    /* set bell duration in msec from param[1] */
    break;
    
  case 12:
    /* bring specified console to the front,
     * we could "misuse" this for notebooks... ;)
     */
    break;
    
  case 13:
    /* unblank the screen */
    break;
    
  case 14:
    /* set vesa powerdown interval */
    break;
  }
}

static void
gtk_vt102_set_mode (GtkVtEmu		*base_emu,
		    gboolean		on_off)
{
  GtkVt102Emu *vtemu;
  guint i;
  
  g_assert (base_emu != NULL);
  g_assert (base_emu->internal_id == GTK_VTEMU_ID_LINUX);
  vtemu = (GtkVt102Emu*) base_emu;
  
  on_off = on_off != FALSE;
  
  for (i = 0; i <= vtemu->n_params; i++)
    if (vtemu->dec_priv_mode)
    {
      switch (vtemu->param[i]) /* DEC private modes set/reset */
      {
      case  1:
	/* FIXME: Cursor keys send ^[Ox/^[[x */
	break;
	
      case  3:
	/* 80/132 mode switch unimplemented */
	break;
	
      case  5:
	/* Inverted screen on/off */
	if (base_emu->term_inverted != on_off)
	  gtk_vtemu_invert (base_emu);
	break;
	
      case  6:
	/* Origin relative/absolute */
	vtemu->relative_origin = on_off;
	gtk_vt102_gotoxry (vtemu, base_emu->cur_x, base_emu->cur_y);
	break;
	
      case  7:
	/* FIXME: autowrap hint */
	/* vtemu->autowrap_hint = on_off; */
	break;
	
      case  8:
	/* Autorepeat on/off unimplemented */
	break;
	
      case  9:
	/* FIXME: report_mouse on/off -> 1/0 */
	break;
	
      case  25:
	if (on_off)
	  vtemu->cursor_mode = base_emu->dfl_cursor_mode;
	else
	  vtemu->cursor_mode = GTK_CURSOR_INVISIBLE;
	gtk_term_set_cursor_mode (base_emu->term, vtemu->cursor_mode, vtemu->cursor_blinking);
	break;
	
      case  1000:
	/* FIXME: report_mouse on/off -> 2/0 */
	break;
      }
    }
    else
    {
      switch (vtemu->param[i]) /* ANSI modes set/reset */
      {
      case  3:
	vtemu->disp_ctrl = on_off;
	break;
	
      case  4:
	/* Insert Mode on/off */
	base_emu->insert_mode = on_off;
	break;
	
      case  5: /* stolen from dec_priv_mode FEATURE */
	/* Inverted screen on/off */
	if (base_emu->term_inverted != on_off)
	  gtk_vtemu_invert (base_emu);
	break;

      case  20:
	/* Lf, Enter == CrLf/Lf */
	if (on_off)
	  base_emu->lf_plus_cr = TRUE;
	else
	  base_emu->lf_plus_cr = FALSE;
	break;
      }
    }
}

static void
gtk_vt102_set_term_mode (GtkVtEmu	*base_emu)
{
  GtkVt102Emu *vtemu;
  guint i;
  
  g_assert (base_emu != NULL);
  g_assert (base_emu->internal_id == GTK_VTEMU_ID_LINUX);
  vtemu = (GtkVt102Emu*) base_emu;
  
  for (i = 0; i <= vtemu->n_params; i++)
    switch (vtemu->param[i])
    {
    case  0:
      /* all attributes off */
      gtk_term_select_color (base_emu->term, GTK_TERM_MAX_COLORS - 1, 0);
      gtk_term_set_bold (base_emu->term, FALSE);
      gtk_term_set_dim (base_emu->term, FALSE);
      gtk_term_set_underline (base_emu->term, FALSE);
      /* gtk_term_set_blink (base_emu->term, FALSE); */
      gtk_term_set_reverse (base_emu->term, FALSE);
      break;
      
    case  1:
      gtk_term_set_bold (base_emu->term, TRUE);
      break;
      
    case  2:
      gtk_term_set_dim (base_emu->term, TRUE);
      break;
      
    case  4:
      gtk_term_set_underline (base_emu->term, TRUE);
      break;
      
    case  5:
      /* we can't blink... ;( */
      gtk_term_set_bold (base_emu->term, TRUE);
      break;
      
    case  7:
      gtk_term_set_reverse (base_emu->term, TRUE);
      break;
      
    case  10:
      /* FIXME: charset_G0/charset_G1 */
      vtemu->charset_table = NULL;
      vtemu->disp_ctrl = FALSE;
      vtemu->toggle_meta = FALSE;
      break;
      
    case  11:
      /* FIXME: charset_G0/charset_G1 */
      vtemu->charset_table = c2f_linux_any; /* FIXME */
      vtemu->charset_table = c2f_linux_misc;
      vtemu->disp_ctrl = TRUE;
      vtemu->toggle_meta = FALSE;
      break;
      
    case  12:
      /* FIXME: charset_G0/charset_G1 */
      vtemu->charset_table = c2f_linux_any; /* FIXME */
      vtemu->charset_table = c2f_linux_misc;
      vtemu->disp_ctrl = TRUE;
      vtemu->toggle_meta = TRUE;
      break;
      
    case  21:
      gtk_term_set_bold (base_emu->term, FALSE);
      break;
      
    case  22:
      gtk_term_set_dim (base_emu->term, FALSE);
      break;
      
    case  24:
      gtk_term_set_underline (base_emu->term, FALSE);
      break;
      
    case  25:
      /* we can't blink... ;( */
      gtk_term_set_bold (base_emu->term, FALSE);
      break;
      
    case  27:
      gtk_term_set_reverse (base_emu->term, FALSE);
      break;
      
    case  30 ... 37:
      /* set foreground color */
      gtk_term_select_color (base_emu->term, vtemu->param[i] - 30, base_emu->term->i_back);
      break;
      
    case  38:
      /* enable underscore mode: white foreground + underline */
      gtk_term_set_underline (base_emu->term, TRUE);
      break;
      
    case  39:
      /* disable underscore mode */
      gtk_term_set_underline (base_emu->term, FALSE);
      break;
      
    case  40 ... 47:
      /* set background color */
      gtk_term_select_color (base_emu->term, base_emu->term->i_fore, vtemu->param[i] - 40);
      break;
      
    case  49:
      /* reset background color */
      gtk_term_select_color (base_emu->term, base_emu->term->i_fore, 0);
      break;
    }
}
