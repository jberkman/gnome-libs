/*  vtx.h - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  Screen definitions and structures
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

#ifndef _VTX_H
#define _VTX_H

#include <zvt/lists.h>
#include <zvt/vt.h>

/* include the toolkit */
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
#define VT_SELTYPE_MOVED 0x2000 /* has motion occured? */

/* 'X' extension data for VT terminal emulation */
struct _vtx
{
  struct vt_em vt;

  /* when updating, background colour matches for whole contents of line */
  unsigned int back_match:1;

  /* selection stuff */
  char *selection_data;		/* actual selection */
  int selection_size;

  /* 256 bits of word class characters (assumes a char is 8 bits or more) */
  unsigned char wordclass[32];

  /* rendering buffer, for building output strings */
  char *runbuffer;
  int runbuffer_size;

  /* true if something selected */
  int selected;

  /* if selection active, what type? (by char/word/line) */
  VT_SELTYPE selectiontype;

  int selstartx, selstarty;
  int selendx, selendy;

  /* previously rendered values */
  int selstartxold, selstartyold;
  int selendxold, selendyold;
};


/* from update.c */
char *vt_get_selection   (struct _vtx *vx, int *len);
void vt_clear_selection  (struct _vtx *vx);
void vt_fix_selection    (struct _vtx *vx);
void vt_draw_selection   (struct _vtx *vx);
void vt_update_rect      (struct _vtx *vx, int sx, int sy, int ex, int ey);
void vt_update           (struct _vtx *vt, int state);
void vt_draw_cursor      (struct _vtx *vx, int state);
void vt_set_wordclass    (struct _vtx *vx, unsigned char *s);
			 
struct _vtx *vtx_new     (int width, int height, void *user_data);
void vtx_destroy         (struct _vtx *vx);
void vtx_set_fontsize    (struct _vtx *vx, int width, int height);
			 
/* defined by caller */	 
void vt_draw_text        (void *user_data, int col, int row, char *text, int len, int attr);
void vt_scroll_area      (void *user_data, int firstrow, int count, int offset, int fill);
int  vt_cursor_state     (void *user_data, int state);
void vt_hightlight_block (void *user_data, int col, int row, int width, int height);
int  vt_get_attr_at      (struct _vtx *vx, int col, int row);


#ifdef __cplusplus
	   }
#endif /* __cplusplus */


#endif

