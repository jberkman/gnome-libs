/*  vt.h - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  Virtual terminal emulation definitions and structures
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

#ifndef _VT_H
#define _VT_H

#include "lists.h"

/* defines for screen update routine */
#define UPDATE_CHANGES 0x00	/* only update changed areas */
#define UPDATE_REFRESH 0x01	/* just refersh all */
#define UPDATE_SCROLLBACK 0x02	/* if in scrollback mode, make sure everything is redrawn */


typedef unsigned int uint32;	/* 32 bit unsigned int */
/* perhaps should be a bitfield ? */

/* defines for VT argument processing, also used for textual arguments */
#define VTPARAM_MAXARGS 5	/* maximum number of arguments */
#define VTPARAM_ARGMAX 20	/* number of characters in each arg maximum */


struct vt_line {
  struct vt_line *next;		/* next 'vt' line */
  struct vt_line *prev;		/* prev 'vt' line */
  int line;			/* the line number for this line */
  int width;			/* width of this line */
  int modcount;			/* how many modifications since last update */
  uint32 data[1];		/* the line data follows this structure */
};

#define VTATTR_BOLD       0x40000000
#define VTATTR_UNDERLINE  0x20000000
#define VTATTR_BLINK      0x10000000
#define VTATTR_REVERSE    0x08000000
#define VTATTR_CONCEALED  0x04000000
/*#define VTATTR_FORE_SET   0x08000000
  #define VTATTR_BACK_SET   0x10000000*/
#define VTATTR_CHANGED    0x80000000

/* bitmasks for colour map information */
#define VTATTR_FORECOLOURM 0x03e00000
#define VTATTR_BACKCOLOURM 0x001f0000
#define VTATTR_FORECOLOURB 21
#define VTATTR_BACKCOLOURB 16

/* 'clear' character and attributes of default clear character */
#define VTATTR_CLEAR (16<<VTATTR_FORECOLOURB)|(17<<VTATTR_BACKCOLOURB)|0x0000

struct vt_em {
  int cursorx, cursory;		/* cursor position in characters */
  int width, height;		/* width/height in characters */
  int scrolltop;		/* line from which scrolling occurs */
  int scrollbottom;		/* line after which scrolling occurs */

  int childpid;			/* child process id */
  int childfd;			/* child file descriptor (for read/write) */

  int savex,savey;		/* saved cursor position */
  struct vt_line *savethis;

  int cx, cy;			/* cursor position in pixels */
  int sx, sy;			/* width and height pixels */

  unsigned char *remaptable;	/* chracter remapping table. 0 = dont remap */

  int Gx;			/* current character set mapping */
  unsigned char *G[4];		/* Gx character set mappings */

  uint32 attr;			/* current char attributes.  This
				 is a bitfield.  bottom 16 bits = ' ' */

  uint32 mode;			/* vt modes.  see below */

  unsigned char *args[VTPARAM_MAXARGS];	/* run-time emulation arguments */
  char args_mem[VTPARAM_MAXARGS*VTPARAM_ARGMAX];
  unsigned char **argptr;
  char *outptr;
  char *outend;
  int argcnt;

  int state;			/* current parse state */

  struct vt_line *this;		/* the current line */

  struct vt_list lines;		/* double linked list of lines */
  struct vt_list lines_back;	/* 'last rendered' buffer.  used to optimise updates */
  struct vt_list lines_alt;	/* alternate screen */

  /* scroll back stuff */
  struct vt_list scrollback;	/* double linked list of scrollback lines */
  struct vt_list mem_list;	/* memory list for scrollback buffer */
  int scrollbacklines;		/* total scroll back lines */
  int scrollbackoffset;		/* viewing offset */
  int scrollbackold;		/* old scrollback offset */
  int scrollbackmax;		/* maximum scrollbacklines, after this total is reached,
				   old lines are discarded */

};

#define VTMODE_INSERT 0x00000001 /* insert mode active */
#define VTMODE_SEND_MOUSE 0x00000002 /* send mouse clicks */
#define VTMODE_APP_CURSOR 0x00000008 /* application cursor keys */

#define VTMODE_ALTSCREEN 0x80000000 /* on alternate screen? */

struct vt_em *vt_init(struct vt_em *vt, int width, int height);
void vt_destroy(struct vt_em *vt);
void vt_resize(struct vt_em *vt, int width, int height, int pixwidth, int pixheight);
void parse_vt(struct vt_em *vt, char *ptr, int length);

void vt_swap_buffers(struct vt_em *vt);

int vt_forkpty(struct vt_em *vt);
int vt_readchild(struct vt_em *vt, char *buffer, int len);
int vt_writechild(struct vt_em *vt, char *buffer, int len);
int vt_report_button(struct vt_em *vt, int button, int qual, int x, int y);
void vt_scrollback_set(struct vt_em *vt, int lines);
int vt_killchild(struct vt_em *vt, int signal);
int vt_closepty(struct vt_em *vt);

#endif
