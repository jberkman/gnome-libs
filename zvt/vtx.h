/*  vtx.h - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  Screen definitions and structures
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

#ifndef _VTX_H
#define _VTX_H

#include <zvt/lists.h>
#include <zvt/vt.h>

/* include the toolkit */
#include <gtk/gtk.h>

/* defines for screen update routine */
#define UPDATE_CHANGES 0x00	/* only update changed areas */
#define UPDATE_REFRESH 0x01	/* just refersh all */
#define UPDATE_SCROLLBACK 0x02	/* if in scrollback mode, make sure everything is redrawn */

typedef enum {
  VT_SELTYPE_NONE=0,		/* selection inactive */
  VT_SELTYPE_CHAR,		/* selection by char */
  VT_SELTYPE_WORD,		/* selection by word */
  VT_SELTYPE_LINE,		/* selection by whole line */
  VT_SELTYPE_MAGIC		/* 'magic' or 'active tag' select */
} VT_SELTYPE;

#define VT_SELTYPE_MASK 0xff
#define VT_SELTYPE_BYEND 0x8000
#define VT_SELTYPE_BYSTART 0x4000

/* 'X' extension data for VT terminal emulation */
struct _vtx {
  struct vt_em vt;

  /* selection stuff */
  char *selection_data;		/* actual selection */
  int selection_size;
  
  int selected;			/* true if something selected */
  VT_SELTYPE selectiontype;	/* if selection active, what type? (by char/word/line)*/

  int selstartx, selstarty;
  int selendx, selendy;

  /* previously rendered values */
  int selstartxold, selstartyold;
  int selendxold, selendyold;

  /* needed to pass args to render functions */
  void *user_data;

#ifdef STAND_ALONE
  /* all tool-kit specific data goes here ... */
  GtkWidget *drawingarea;	/* rendering area */
  GtkObject *adjustment;	/* scrollback position adjustement */
  GdkCursor *cursor_bar,	/* I beam cursor */
    *cursor_dot,		/* the blank cursor */
    *cursor_current;		/* current active cursor */
  GdkPixmap *cursor_dot_pm;	/* 'dot' pixmap, for dot cursor (invisible) */
  guint timeout_id;		/* id of timeout function */
  GdkFont *font,		/* current normal font */
    *font_bold;			/* current bold font */
				/* (add italic font?) */
#endif
};


void vt_draw_text_select(struct _vtx *vx, int col, int row, char *text, int len, int attr);

/* from update.c */
char *vt_get_selection(struct _vtx *vx, int *len);
void vt_clear_selection(struct _vtx *vx);
void vt_fix_selection(struct _vtx *vx);
void vt_draw_selection(struct _vtx *vx);
void vt_update_rect(struct _vtx *vx, int sx, int sy, int ex, int ey);
void vt_update(struct _vtx *vt, int state);

struct _vtx *vtx_new(void *user_data);
void vtx_destroy(struct _vtx *vx);
void vtx_set_fontsize(struct _vtx *vx, int width, int height);

/* defined by caller */
void vt_draw_text(void *user_data, int col, int row, char *text, int len, int attr);
void vt_scroll_area(void *user_data, int firstrow, int count, int offset);
int vt_cursor_state(void *user_data, int state);
void vt_hightlight_block(void *user_data, int col, int row, int width, int height);

#endif

