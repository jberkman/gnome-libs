/*  vt.c - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  A virtual terminal emulator.
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

/*
  This module handles the update of the 'off-screen' virtual terminal
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <termios.h>
#include <sys/ioctl.h>

#include "subshell.h"
#include "lists.h"
#include "memory.h"
#include "vt.h"

#define SCROLLBACK_BUFFER	/* use scrollback buffer? */
#define DOUBLE_BUFFER		/* use back buffer to represent screen
				   FIXME: THIS MUST BE DEFINED */

/* define to 'x' to enable copius debug of this module */
#define d(x)

/* draw selected text (if selected!) */
void vt_draw_text_select(int col, int row, char *text, int len, int attr);

struct vt_line *vt_newline(struct vt_em *vt);

void vt_scroll_up(struct vt_em *vt, int count);
void vt_scroll_down(struct vt_em *vt, int count);
void vt_insert_chars(struct vt_em *vt, int count);
void vt_delete_chars(struct vt_em *vt, int count);
void vt_insert_lines(struct vt_em *vt, int count);
void vt_delete_lines(struct vt_em *vt, int count);
void vt_clear_lines(struct vt_em *vt, int top, int count);
void vt_clear_line_portion(struct vt_em *vt, int start_col, int end_col);

static unsigned char vt_remap_dec[256];

void vt_dump(struct vt_em *vt)
{
  struct vt_line *wn, *nn;
  int i;

  wn = (struct vt_line *)vt->lines.head;
  nn = wn->next;
  (printf("dumping state of vt buffer:\n"));
  while (nn) {
    for (i=0;i<wn->width;i++) {
      (printf("%c", wn->data[i]&0xffff));
    }
    (printf("\n"));
    wn=nn;
    nn = nn->next;
  }
  (printf("done\n"));
}

/***********************************************************************
 Update functions
*/

/*
  set the screen, either
    screen=0 for main screen
          =1 for alternate screen
*/

static void vt_set_screen(struct vt_em *vt, int screen)
{
  struct vt_line *lh, *lt, *ah, *at;
  int line;

  d(printf("vt_set_screen(%d) called\n", screen));

  if (((vt->mode&VTMODE_ALTSCREEN)?1:0) ^ screen) {

    d(printf("vt_set_screen swapping buffers ... from %d\n", (vt->mode&VTMODE_ALTSCREEN)?1:0));

    /* need to swap 2 list headers.
       tricky bit is catering for all the back pointers? */
    lh = (struct vt_line *)vt->lines.head;
    lt = (struct vt_line *)vt->lines.tailpred;

    ah = (struct vt_line *)vt->lines_alt.head;
    at = (struct vt_line *)vt->lines_alt.tailpred;    

    /* set new head/tail pointers */
    vt->lines.head = (struct vt_listnode *)ah;
    vt->lines.tailpred = (struct vt_listnode *)at;

    vt->lines_alt.head = (struct vt_listnode *)lh;
    vt->lines_alt.tailpred = (struct vt_listnode *)lt;

    /* and link back links */
    ah->prev = (struct vt_line *)&vt->lines.head;
    at->next = (struct vt_line *)&vt->lines.tail;

    lh->prev = (struct vt_line *)&vt->lines_alt.head;
    lt->next = (struct vt_line *)&vt->lines_alt.tail;

    /* and mark all lines as changed/but un-moved */
    at = ah->next;
    line=0;
    while (at) {
      ah->modcount=ah->width;
      ah->line = line++;
      ah = at;
      at = at->next;
    }

    d({
      ah = &vt->lines.head;
      lh = &vt->lines_alt.head;
      while (ah) {
	printf("%p: %p %p   %p: %p %p\n", ah, ah->next, ah->prev, lh, lh->next, lh->prev);
	ah = ah->next;
	lh = lh->next;
      }
    });

    if (screen)
      vt_clear_lines(vt, 0, vt->height);

    vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
    vt->mode = (vt->mode&~VTMODE_ALTSCREEN) | (screen?VTMODE_ALTSCREEN:0);
  }
}  

#ifdef SCROLLBACK_BUFFER

/*
  set the scrollback buffer size
*/
void vt_scrollback_set(struct vt_em *vt, int lines)
{
  struct vt_line *ln;

  /* note that it is safe to call vt_mem_push() with null memory, in the
     (erronous) event that scrollbacklines != sizeof(scrollback list) */
  while (vt->scrollbacklines > lines) {
    ln = (struct vt_line *)vt_list_remhead(&vt->scrollback); /* remove the top of list line */
    vt_mem_push(&vt->mem_list, ln, sizeof(struct vt_line)+sizeof(uint32)*ln->width);
    vt->scrollbacklines--;
  }
  vt->scrollbackmax = lines;
}

/*
  clone the line 'line' and add it to the bottom of
  the scrollback buffer.

  if the scrollback buffer is full, discard the oldest line

  it is up-to the caller to free or 'discard' the old line
*/
void vt_scrollback_add(struct vt_em *vt, struct vt_line *wn)
{
  struct vt_line *ln;

  d(printf("vt_scrollback_add()\n"));

  /* create a new scroll-back line */
  ln = vt_mem_get(&vt->mem_list, sizeof(struct vt_line) + sizeof(uint32)*wn->width);/* always allocates 1 more 'char' */
  ln->width = wn->width;
  memcpy(ln->data, wn->data, wn->width*sizeof(uint32));
  /* add it to the scrollback buffer */
  vt_list_addtail(&vt->scrollback, (struct vt_listnode *)ln);
  
  d(printf("scrollback grow\n"));
  
  /* limit the total number of lines in scrollback */
  if (vt->scrollbacklines >= vt->scrollbackmax) {
    ln = (struct vt_line *)vt_list_remhead(&vt->scrollback); /* remove the top of list line */
    vt_mem_push(&vt->mem_list, ln, sizeof(struct vt_line)+sizeof(uint32)*ln->width);
  } else {
    vt->scrollbacklines++;
  }
}

#endif /* SCROLLBACK_BUFFER */

/* normal scroll */
void vt_scroll_up(struct vt_em *vt, int count)
{
  struct vt_line *wn, *nn;
  int i;
  int blank;

  d(printf("vt_scroll_up(%d) (top = %d bottom=%d)\n", count, vt->scrolltop, vt->scrollbottom));

  blank = vt->attr;

  while (count>0) {
    /* first, find the line to remove */
    wn=(struct vt_line *)vt_list_index(&vt->lines, vt->scrolltop);

    d(if(!wn) printf("ERROR: could not find line %d\n", vt->scrolltop));

    vt_list_remove((struct vt_listnode *)wn);

#ifdef SCROLLBACK_BUFFER
    if ((vt->scrolltop==0) && ((vt->mode&VTMODE_ALTSCREEN)==0)) {
      vt_scrollback_add(vt, wn);
    }
#endif /* SCROLLBACK_BUFFER */

    for (i=0;i<wn->width;i++)
      wn->data[i] = vt->attr|VTATTR_CHANGED;

    if (wn->line == -1) {
      wn->modcount = wn->width;	/* make sure a wrap-scrolled line isn't marked clean */
    } else {
      wn->modcount = 0;		/* this speeds it up (heaps) but doesn't work :( */
      wn->line=-1;		/* flag new line */
    }

    /* insert it .. (on bottom of scroll area) */
    nn=(struct vt_line *)vt_list_index(&vt->lines, vt->scrollbottom);
    vt_list_insert(&vt->lines, (struct vt_listnode *)nn, (struct vt_listnode *)wn);

    count--;
  }

  d(printf("vt_scroll_up() done\n"));
}

/* reverse scroll */
void vt_scroll_down(struct vt_em *vt, int count)
{
  struct vt_line *wn, *nn;
  int i;

  d(printf("vt_scroll_down(%d) (top = %d bottom=%d)\n", count, vt->scrolltop, vt->scrollbottom));
  /* FIXME: do this properly */

  while(count>0) {

    /* first, find the line to remove */
    wn=(struct vt_line *)vt_list_index(&vt->lines, vt->scrollbottom);
    vt_list_remove((struct vt_listnode *)wn);
    
    /* clear it */
    for (i=0;i<wn->width;i++) {
      wn->data[i] = vt->attr|VTATTR_CHANGED;
    }
    wn->modcount=0;
    wn->line = -1;		/* flag new line */
    
    /* insert it .. (on bottom of scroll area) */
    nn=(struct vt_line *)vt_list_index(&vt->lines, vt->scrolltop);
    vt_list_insert(&vt->lines, (struct vt_listnode *)nn, (struct vt_listnode *)wn);

    count--;
  }
}

void vt_insert_chars(struct vt_em *vt, int count)
{
  int i, j;
  struct vt_line *l;		/* FIXME: kludge */

  d(printf("vt_insert_chars(%d)\n", count));
  l=vt->this;

  /* scroll data over count bytes */
  j = (l->width-count)-vt->cursorx;
  for (i=l->width-1;j>0;i--,j--) {
    l->data[i] = l->data[i-count]|VTATTR_CHANGED;    
  }

  /* clear the rest of the line */
  for (i=vt->cursorx;i<vt->cursorx+count;i++) {
    l->data[i] = vt->attr|VTATTR_CHANGED;
  }
  l->modcount+=count;
}

void vt_delete_chars(struct vt_em *vt, int count)
{
  int i, j;
  struct vt_line *l;

  d(printf("vt_delete_chars(%d)\n", count));
  l=vt->this;

  /* scroll data over count bytes */
  j = (l->width-count)-vt->cursorx;
  for (i=vt->cursorx;j>0;i++,j--) {
    l->data[i] = l->data[i+count]|VTATTR_CHANGED;
  }

  /* clear the rest of the line */
  for (i=l->width-count;i<l->width;i++) {
    l->data[i] = vt->attr|VTATTR_CHANGED;
  }
  l->modcount+=count;

}

void vt_insert_lines(struct vt_em *vt, int count)
{
  struct vt_line *wn, *nn;
  int i;

  d(printf("vt_insert_lines(%d) (top = %d bottom = %d cursory = %d)\n", count, vt->scrolltop, vt->scrollbottom, vt->cursory));

  /* FIXME: Do this properly */

  while(count>0) {
    /* first, find the line to remove */
    wn=(struct vt_line *)vt_list_index(&vt->lines, vt->scrollbottom);
    vt_list_remove((struct vt_listnode *)wn);
    
    /* clear it */
    for (i=0;i<wn->width;i++) {
      if ((wn->data[i]&0xff) && ((wn->data[i] & 0xff) != ' ')) {
	wn->data[i] = vt->attr|VTATTR_CHANGED; /* FIXME: include attributes */
      }
				/* NOTE: This should not have
				   ATTR_CHANGED - taken care of by
				   scroll update */
    }
    wn->modcount=0;		/* set as 'unchanged' so the scroll
				   routine can update it.
				   but, if anyone else changes this line, make
				   sure the rest of the screen is updated */
    /*wn->line=vt->cursory;*/		/* also 'fool' it into thinking the
					   line is unchanged */
    wn->line=-1;

    /* insert it .. */
    nn=(struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
    vt_list_insert(&vt->lines, (struct vt_listnode *)nn, (struct vt_listnode *)wn);

    count--;
  }

  vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
}

void vt_delete_lines(struct vt_em *vt, int count)
{
  struct vt_line *wn, *nn;
  int i;

  d(printf("vt_delete_lines(%d)\n", count));
  /* FIXME: do this properly */

  while (count>0) {
    /* first, find the line to remove */
    wn=(struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
    vt_list_remove((struct vt_listnode *)wn);
    
    /* clear it */
    for (i=0;i<wn->width;i++) {
      if ((wn->data[i]&0xff) && (((wn->data[i] & 0xff) != ' ') || 
		((wn->data[i] & (0x7ffff)) != vt->attr))) {
	wn->data[i] = vt->attr|VTATTR_CHANGED; /* FIXME: include attributes */
      }
    }
    wn->modcount=0;
    /*wn->line=vt->scrollbottom;*/
    wn->line=-1;
    
    /* insert it .. (on bottom of scroll area) */
    nn=(struct vt_line *)vt_list_index(&vt->lines, vt->scrollbottom);
    vt_list_insert(&vt->lines, (struct vt_listnode *)nn, (struct vt_listnode *)wn);
    
    count--;
  }
  vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
}

void vt_clear_lines(struct vt_em *vt, int top, int count)
{
  struct vt_line *wn, *nn;
  int i;

  d(printf("vt_clear_lines(%d, %d)\n", top, count));
  i=1;
  wn=(struct vt_line *)vt_list_index(&vt->lines, top);
  nn=wn->next;
  while(nn && count>=0) {
    for(i=0;i<wn->width;i++) {
      wn->data[i] = vt->attr|VTATTR_CHANGED;
    }
    wn->modcount = wn->width;
    count--;
    wn=nn;
    nn=nn->next;
  }
}

void vt_clear_line_portion(struct vt_em *vt, int start_col, int end_col)
{
  struct vt_line *this;
  int i;

  d(printf("vt_clear_line_portion()\n"));

  this = vt->this;
  for(i=start_col;i<end_col;i++) {
    this->data[i] = vt->attr | VTATTR_CHANGED;
  }
  this->modcount+=(this->width-vt->cursorx);
}




/********************************************************************************\
 PARSER Callbacks
\********************************************************************************/
static void vt_bell(struct vt_em *vt)
{
  d(printf("bell\n"));
}

/* carriage return */
static void vt_cr(struct vt_em *vt)
{
  d(printf("cr \n"));
  vt->cursorx = 0;
}

/* line feed */
static void vt_lf(struct vt_em *vt)
{
  d(printf("lf \n"));
  if (vt->cursory >= vt->scrollbottom) {
    d(printf("must scroll\n"));
    vt_scroll_up(vt, 1);
    vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
  } else {
    vt->cursory++;
    d(printf("new ypos = %d\n", vt->cursory));
    vt->this = vt->this->next;
  }
}

/* jump to next tab stop, wrap if needed */
static void vt_tab(struct vt_em *vt)
{
  int nx;
  unsigned char c;
  struct vt_line *l;
  int i;

  d(printf("tab\n"));

  l = vt->this;
  c=l->data[vt->cursorx]&0xff;
  /* dont store tab over a space - will affect attributes */
  if (c==0) {
    l->data[vt->cursorx] = 9|vt->attr|VTATTR_CHANGED; /* store 'tab' if we can ... helps cut/paste */
  }
  
  /* expand data after tab to be 'empty', but dont change attributes */
  nx = (vt->cursorx+8) & ~7;
  for(i=vt->cursorx+1;i<nx;i++) {
    if ( ((c=l->data[i]&0xff)==' ')) {
      l->data[i] = (l->data[i]&0xffff0000)|VTATTR_CHANGED;
    } else {
      break;
    }
  }

  /*nx = (vt->cursorx+8) & ~7;*/
  vt->cursorx = (vt->cursorx + 8) & 0xfff8;
  if (vt->cursorx >= vt->width) {
    vt->cursorx = 0;
    vt_lf(vt);			/* goto next line */
  }
}

static void vt_backspace(struct vt_em *vt)
{
  d(printf("bs \n"));
  if (vt->cursorx>0) {
    vt->cursorx--;
  }
}

static void vt_alt_start(struct vt_em *vt)
{
  d(printf("alternate charset on\n"));
  /* swap in a 'new' charset mapping */
  vt->remaptable = vt->G[1];		/* no character remapping */
}

static void vt_alt_end(struct vt_em *vt)
{
  d(printf("alternate charset off\n"));
  /* restore 'old' charset mapping */
  vt->remaptable = vt->G[0];		/* no character remapping */
}

/* line editing */
static void vt_insert_char(struct vt_em *vt)
{
  d(printf("insert char(s)\n"));
  if (vt->argcnt==0) {
    vt_insert_chars(vt, 1);		/* insert single char */
  } else if (vt->argcnt==1) {
    vt_insert_chars(vt, atoi(vt->args[0]));/* insert multiple characters */
  } else {
    d(printf("insert characters got >1 parameters\n"));
  }
}

static void vt_delete_char(struct vt_em *vt)
{
  d(printf("delete char(s)\n"));
  if (vt->argcnt==0) {
    vt_delete_chars(vt, 1);		/* insert single char */
  } else if (vt->argcnt==1) {
    vt_delete_chars(vt, atoi(vt->args[0]));/* insert multiple characters */
  } else {
    d(printf("delete characters got %d parameters\n", vt->argcnt));
  }
}

/* insert lines and scroll down */
static void vt_insert_line(struct vt_em *vt)
{
  d(printf("insert line(s)\n"));
  if (vt->argcnt==0) {
    vt_insert_lines(vt, 1);		/* insert single char */
  } else if (vt->argcnt==1) {
    vt_insert_lines(vt, atoi(vt->args[0]));/* insert multiple characters */
  } else {
    d(printf("insert lines got >1 parameters\n"));
  }
}

/* delete lines and scroll up */
static void vt_delete_line(struct vt_em *vt)
{
  d(printf("delete line(s)\n"));
  if (vt->argcnt==0) {
    if (vt->cursory>vt->scrolltop) {	/* reverse line feed, not delete */
      vt->cursory--;
    } else {
      vt_scroll_down(vt, 1);
    }
  } else if (vt->argcnt==1) {
    vt_delete_lines(vt, atoi(vt->args[0]));/* insert multiple characters */
  } else {
    d(printf("delete characters got >1 parameters\n"));
  }
  vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
}

/* clear from current cursor position to end of screen */
static void vt_cleareos(struct vt_em *vt)
{
  d(printf("clear screen/end of screen\n"));
  vt_clear_line_portion(vt, vt->cursorx, vt->this->width);
  if (vt->cursory<vt->height) {
    vt_clear_lines(vt, vt->cursory+1, vt->height);
  }
}

static void vt_clear_lineportion(struct vt_em *vt)
{
  d(printf("Clear part of line\n"));
  if (vt->argcnt>1) {
    /* eat the command */
  } else if (vt->argcnt==0 || *vt->args[0] == '0') {
    vt_clear_line_portion(vt, vt->cursorx, vt->this->width);
  } else if (*vt->args[0] == '1') {
    vt_clear_line_portion(vt, 0, vt->cursorx + 1);
  } else if (*vt->args[0] == '2') {
    vt_clear_line_portion(vt, 0, vt->this->width);
  }
  /* eat bad parameters */
}


/* cursor save/restore */
static void vt_save_cursor(struct vt_em *vt)
{
  d(printf("save cursor\n"));
  vt->savex = vt->cursorx;
  vt->savey = vt->cursory;
}

static void vt_restore_cursor(struct vt_em *vt)
{
  d(printf("restore cursor\n"));
  vt->cursorx = vt->savex;
  vt->cursory = vt->savey;

  /* incase the save/restore was across re-sizes */
  if (vt->cursorx >= vt->width)
    vt->cursorx = vt->width-1;
  if (vt->cursory >= vt->height)
    vt->cursory = vt->height-1;

  vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
  d(printf("found line %d, %p\n", vt->cursory, vt->this));
}

/* cursor movement */
/* in app-cursor mode, this will cause the screen to scroll ? */
static void vt_up(struct vt_em *vt)
{
  int count=1;

  d(printf("\n----------------------\n cursor up\n"));
  if (vt->argcnt==1)
    count=atoi(vt->args[0]);

  d(printf("cursor up %d, from %d\n", count, vt->cursory));

  vt->cursory-=count;
  if (vt->cursory<vt->scrolltop) {
    vt->cursory=vt->scrolltop;
  }

  vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
}

static void vt_down(struct vt_em *vt)
{
  int count=1;

  d(printf("cursor down\n"));
  if (vt->argcnt==1)
    count=atoi(vt->args[0]);

  vt->cursory+=count;
  if (vt->cursory>=vt->scrollbottom)
    vt->cursory=vt->scrollbottom-1;

  vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
}

static void vt_right(struct vt_em *vt)
{
  int count=1;

  d(printf("cursor right\n"));
  if (vt->argcnt==1)
    count=atoi(vt->args[0]);

  vt->cursorx+=count;
  if (vt->cursorx>=vt->width)	/* wrapping? end of line? */
    vt->cursorx=vt->width-1;
}

static void vt_left(struct vt_em *vt)
{
  int count=1;

  d(printf("cursor left\n"));
  if (vt->argcnt==1)
    count=atoi(vt->args[0]);

  vt->cursorx-=count;
  if (vt->cursorx < 0)
    vt->cursorx=0;
}

/* jump to a cursor position */
static void vt_goto(struct vt_em *vt)
{
  d(printf("goto position\n"));
  if (vt->argcnt==0) {
    vt->cursorx=0;
    vt->cursory=0;
    /*vt->this = vt->first;*/
  } else if (vt->argcnt==1) {
    vt->cursory = atoi(vt->args[0])-1;
    vt->cursorx=0;
    /*vt->this = vt_list_index(&vt->first, vt->cursory);*/
  } else if (vt->argcnt==2) {
    vt->cursory = atoi(vt->args[0])-1;
    vt->cursorx = atoi(vt->args[1])-1;
    /*vt->this = vt_list_index(&vt->first, vt->cursory);*/
  } else {
    d(printf("position had too many parameters\n"));
  }
  if (vt->cursorx < 0)
    vt->cursorx = 0;
  if (vt->cursory < 0)
    vt->cursory = 0;
  if (vt->cursorx >= vt->width)
    vt->cursorx = vt->width-1;
  if (vt->cursory >= vt->height)
    vt->cursory = vt->height-1;
  d(printf("pos = %d %d\n", vt->cursory, vt->cursorx));

  vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);
}

/* various mode stuff */
static void vt_modeh(struct vt_em *vt)
{
  d(printf("mode h set\n"));
  if (vt->argcnt==1) {
    switch(*(vt->args[0])) {
    case '4':
      d(printf("begin insert mode\n"));
      vt->mode |= VTMODE_INSERT;
      break;
    case '?':
      switch(atoi(vt->args[0]+1)) {
      case 1:			/* turn on application cursor keys */
	vt->mode |= VTMODE_APP_CURSOR;
	break;
      case 47:
	vt_set_screen(vt, 1);
	break;
      case 1000:
	d(printf("sending mouse events\n"));
	vt->mode |= VTMODE_SEND_MOUSE;
	break;
      }
    }
  } else {
    d(printf("Unknown args to mode h\n"));
  }
}


static void vt_model(struct vt_em *vt)
{
d(printf("mode l set\n"));
  if (vt->argcnt==1) {
    switch(*(vt->args[0])) {
    case '4':
      d(printf("end insert mode\n"));
      vt->mode &= ~VTMODE_INSERT;
      break;
    case '?':
      switch(atoi(vt->args[0]+1)) {
      case 1:
	vt->mode &= ~VTMODE_APP_CURSOR;
	break;
      case 47:
	vt_set_screen(vt, 0);
	break;
      case 1000:
	vt->mode &= ~VTMODE_SEND_MOUSE;
	break;
      }
    }
  } else {
    d(printf("Unknown args to mode l\n"));
  }
}

static void vt_modek(struct vt_em *vt)
{
  d(printf("mode k set\n"));
  if (vt->argcnt==1) {
    switch(*(vt->args[0])) {
    case '3':
      d(printf("clear tabs\n"));
      /* FIXME: do it */
      break;
    }
  } else {
    d(printf("Unknown args to mode k\n"));
  }
}


static void vt_mode(struct vt_em *vt)
{
  int i, j;
  int mode_map[] = {0, VTATTR_BOLD, 0, 0,
		    VTATTR_UNDERLINE, VTATTR_BLINK, 0,
		    VTATTR_REVERSE, VTATTR_CONCEALED};

  d(printf("draw mode called\n"));
  if (vt->argcnt==0) {
    vt->attr=VTATTR_CLEAR;	/* clear all attributes */
  } else {
    for (j = 0; j < vt->argcnt; j++)
      {
	i = atoi(vt->args[j]);
	if (i==0) {
	  vt->attr=VTATTR_CLEAR;
	} else if (i<9) {
	  vt->attr |= mode_map[i];	/* add a mode */
	} else if (i>=30 && i <=37) {
	  vt->attr = (vt->attr & ~VTATTR_FORECOLOURM) | ((i-30) << VTATTR_FORECOLOURB);
	} else if (i>=40 && i <=47) {
	  vt->attr = (vt->attr & ~VTATTR_BACKCOLOURM) | ((i-40) << VTATTR_BACKCOLOURB);
	}
      }
  }
}

/*
  set g0-g3 charset
*/
static void vt_gx_set(struct vt_em *vt)
{
  int index;
  unsigned char *table;

  index = vt->args[0][0]-'(';
  if (index<=3 && index>=0) {
    switch (vt->args[0][1]) {
    case '0':
      table = vt_remap_dec;
      break;
    case 'A':			/* turn mapping off for uk or us */
    case 'B':			/* not strictly correct, but easy to fix */
      table = 0;
      break;
    default:			/* unknown char mapping - turn off*/
      table = 0;
    }
    vt->G[index] = table;
    if (index==vt->Gx) {
      vt->remaptable = table;
    }
  } /* else ignore */
}

/*
  device status report
*/
static void vt_dsr(struct vt_em *vt)
{
  char status[16];

  if (vt->argcnt==1) {
    switch(*(vt->args[0])) {
    case '5':			/* report 'ok' status */
      sprintf(status, "\033[0n");
      break;
    case '6':			/* report cursor position */
      sprintf(status, "\033[%d;%dR", vt->cursory+1, vt->cursorx+1);
      break;
    default:
      status[0]=0;
    }
    vt_writechild(vt, status, strlen(status));
  }
}

static void vt_scroll(struct vt_em *vt)
{
  d(printf("set scroll region called\n"));
  if (vt->argcnt==0) {
    vt->scrolltop = 0;
    vt->scrollbottom = vt->height-1;
  } else if (vt->argcnt==1) {
    vt->scrolltop = atoi(vt->args[0])-1;
    vt->scrollbottom = vt->height-1;
  } else if (vt->argcnt ==2) {
    vt->scrolltop = atoi(vt->args[0])-1;
    vt->scrollbottom = atoi(vt->args[1])-1;
  }
  if (vt->scrolltop < 0)
    vt->scrolltop = 0;
  if (vt->scrollbottom >= vt->height)
    vt->scrollbottom = vt->height-1;

  d(printf("new region = (%d - %d)\n", vt->scrolltop, vt->scrollbottom));
}

/*
  master soft reset ^[c
*/
static void vt_reset(struct vt_em *vt)
{
  vt->cursorx=0;
  vt->cursory=0;
  vt->this = (struct vt_line *)vt->lines.head;
  vt->attr=VTATTR_CLEAR;	/* reset attributes */
  vt->remaptable = 0;		/* no character remapping */
  vt_set_screen(vt, 0);
  vt->mode = 0;
  vt_cleareos(vt);

  vt->Gx=0;			/* reset all fonts */
  vt->G[0]=0;
  vt->G[1]=vt_remap_dec;
  vt->G[2]=0;
  vt->G[3]=0;
}

/* function keys */
static void vt_func(struct vt_em *vt)
{
  int i;

  d(printf("function keys\n"));
  if (vt->argcnt==1) {
    i=atoi(vt->args[0]);
    switch (i) {
    case 2:
      d(printf("insert pressed\n"));
      break;
    case 5:
      d(printf("page up pressed\n"));
      break;
    case 6:
      d(printf("page down pressed\n"));
      break;
    default:
      if (i>=11 && i <=20) {
	d(printf("function key %d pressed\n", i-10));
      } else {
	d(printf("Unknown function %d\n", i));
      }
    }
  }
}

struct vt_jump {
  void (*process)(struct vt_em *vt);	/* process function */
  int modes;			/* modes appropriate */
};

#define VT_LIT 0x01		/* literal */
#define VT_CON 0x02		/* control character */
#define VT_EXB 0x04		/* escape [ sequence */
#define VT_EXO 0x08		/* escape O sequence */
#define VT_ESC 0x10		/* escape "x" sequence */
#define VT_ARG 0x20		/* character is a possible argument to function */
#define VT_EXA 0x40		/* escape x "x" sequence */

#define VT_EBL (VT_EXB|VT_LIT)	/* escape [ or literal */
#define VT_EXL (VT_ESC|VT_LIT)	/* escape "x" or literal */
#define VT_BOL (VT_EBL|VT_EXO)	/* escape [, escape O, or literal */

struct vt_jump vtjumps[] = {
  {0,0}, {0,0}, {0,0}, {0,0},	/* 0: ^@ ^C */
  {0,0}, {0,0}, {0,0}, {vt_bell,VT_CON},	/* 4: ^D ^G */
  {vt_backspace,VT_CON}, {vt_tab,VT_CON}, {vt_lf,VT_CON}, {0,0},	/* 8: ^H ^K */
  {0,0}, {vt_cr,VT_CON}, {vt_alt_start,VT_CON}, {vt_alt_end,VT_CON},	/* c: ^L ^O */
  {0,0}, {0,0}, {0,0}, {0,0},	/* 10: ^P ^S */
  {0,0}, {0,0}, {0,0}, {0,0},	/* 14: ^T ^W */
  {0,0}, {0,0}, {0,0}, {0,0},	/* 18: ^X ^[ */
  {0,0}, {0,0}, {0,0}, {0,0},	/* 1c: ^\ ^] ^^ ^_  */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/*  !"# */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* $%&' */
  {vt_gx_set,VT_LIT|VT_EXA}, {vt_gx_set,VT_LIT|VT_EXA}, {vt_gx_set,VT_LIT|VT_EXA}, {vt_gx_set,VT_LIT|VT_EXA},	/* ()*+ */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* ,-./ */
  {0,VT_LIT|VT_ARG}, {0,VT_LIT|VT_ARG}, {0,VT_LIT|VT_ARG}, {0,VT_LIT|VT_ARG},	/* 0123 */
  {0,VT_LIT|VT_ARG}, {0,VT_LIT|VT_ARG}, {0,VT_LIT|VT_ARG}, {vt_save_cursor,VT_EXL|VT_ARG},	/* 4567 */
  {vt_restore_cursor,VT_EXL|VT_ARG}, {0,VT_LIT|VT_ARG}, {0,VT_LIT}, {0,VT_LIT},	/* 89:; */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT|VT_ARG},	/* <=>? */
  {vt_insert_char,VT_EBL}, {vt_up,VT_BOL}, {vt_down,VT_BOL}, {vt_right,VT_BOL},	/* @ABC */
  {vt_left,VT_BOL}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* DEFG */
  {vt_goto,VT_EBL}, {0,VT_LIT}, {vt_cleareos,VT_EBL}, {vt_clear_lineportion,VT_EBL},	/* HIJK */
  {vt_insert_line,VT_EBL}, {vt_delete_line,VT_EBL|VT_ESC}, {0,VT_LIT}, {0,VT_LIT},	/* LMNO */
  {vt_delete_char,VT_EBL}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* PQRS */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* TUVW */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* XYZ[ */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* \]^_ */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {vt_reset,VT_EXL},	/* `abc */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* defg */
  {vt_modeh,VT_EBL}, {0,VT_LIT}, {0,VT_LIT}, {vt_modek,VT_EBL},	/* hijk */
  {vt_model,VT_EBL}, {vt_mode,VT_EBL}, {vt_dsr,VT_EBL}, {0,VT_LIT},	/* lmno */
  {0,VT_LIT}, {0,VT_LIT}, {vt_scroll,VT_EBL}, {0,VT_LIT},	/* pqrs */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* tuvw */
  {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT}, {0,VT_LIT},	/* xyz{ */
  {0,VT_LIT}, {0,VT_LIT}, {vt_func,VT_EBL}, {0,VT_LIT},	/* |}~? */
};

  /* parse vt commands:
     states:
     0: normal escape mode
     1: escape mode.  switch on next character.
     2: '[' escape mode (keep finding numbers until a command code found)
     3: 'O' escape mode.  switch on next character.
     4: ']' escape mode.  swallows until following bell (or newline).
     needs a little work on '[' mode (with parameter grabbing)
  */
void parse_vt(struct vt_em *vt, char *ptr, int length)
{
  register int c;
  register int state;
  register int mode;
  int handled;
  struct vt_jump *modes = vtjumps;
  char *ptrend;
  void (*process)(struct vt_em *vt);	/* process function */

  state = vt->state;
  ptrend = ptr+length;
  while (ptr<ptrend) {
    c=(*ptr++) & 0xff;		/* convert to unsigned byte */
    mode = modes[c & 0x7f].modes;
    process = modes[c & 0x7f].process;
    do {
      handled = 1;
      switch(state) {
      case 0:
	if (mode & VT_LIT) {
	  /* remap character? */
	  if (vt->remaptable)
	    c=vt->remaptable[c];

	  /* insert mode? */
	  if (vt->mode & VTMODE_INSERT)
	    vt_insert_chars(vt, 1);

	  /* need to wrap? */
	  if (vt->cursorx>=vt->width) {
	    vt_lf(vt);
	    vt->cursorx=0;
	  }

	  /* output character */
	  vt->this->data[vt->cursorx] = ((vt->attr | VTATTR_CHANGED) & 0xffff0000) | c;
	  vt->this->modcount++;
	  d(printf("literal %c\n", c));
	  vt->cursorx++;
	} else if (mode & VT_CON) {
	  process(vt);
	} else if (c==27) {
	  state=1;
	}
	/* else ignore */
	break;
      case 1:			/* received 'esc', next byte */
	if (mode & VT_ESC) {	/* got a \Ex sequence */
	  vt->argcnt = 0;
	  process(vt);
	  state=0;
	} else if (c=='[') {
	  vt->argptr = vt->args;	/* initialise output arg pointers */
	  vt->outptr = vt->argptr[0];
	  vt->outend = vt->outptr+VTPARAM_ARGMAX;
	  state = 2;
	} else if (c=='O') {
	  state = 3;
	} else if (c==']') {	/* set text parameters, read parameters */
	  state = 4;
	  vt->argptr = vt->args;
	  vt->outptr = vt->argptr[0];
	  vt->outend = vt->outptr+VTPARAM_ARGMAX*VTPARAM_MAXARGS-1;
	} else if (mode & VT_EXA) {
	  vt->args[0][0]=c;
	  state = 5;
	} else {
	  state = 0;		/* dont understand input */
	}
	break;
      case 2:
	if (mode & VT_ARG) {
	  if (vt->outptr<vt->outend) /* truncate excessive args */
	    *(vt->outptr)++=c;
	} else if (c==';' || c==':') {	/* looking for 'arg;arg;...' */
	  if (vt->argcnt<VTPARAM_MAXARGS) { /* goto next argument */
	    *(vt->outptr)=0;
	    vt->argptr++;
	    vt->outptr = vt->argptr[0];
	    vt->outend = vt->outptr+VTPARAM_ARGMAX;
	  }
	  /* others are ignored */
	} else if (mode & VT_EXB) {
	  if (vt->outptr!=(char *) vt->argptr[0])
	    vt->argptr++;
	  *(vt->outptr)=0;
	  vt->argcnt = (vt->argptr-vt->args);
	  process(vt);
	  state=0;
	} else {
	  d(printf("unknown option '%c'\n", c));
	  state=0;		/* unexpected escape sequence - ignore */
	}
	break;
      case 3:			/* \EOx */
	if (mode & VT_EXO) {
	  process(vt);
	}	/* ignore otherwise */
	state=0;
	break;
      case 4:			/* \E]..;...BEL */
	if (c==0x07) {
	  /* handle output */
	  *(vt->outptr)=0;
	  d(printf("received text mode: %s\n", vt->argptr[0]));
	  state = 0;
	} else if (c==0x0a) {
	  state = 0;		/* abort command */
	} else {
	  if (vt->outptr<vt->outend) /* truncate excessive args */
	    *(vt->outptr)++=c;
	}
	break;
      case 5:			/* \E?x */
	vt->args[0][1]=c;
	if (vt->args[0][0] < 0x7f) {
	  process = modes[vt->args[0][0]].process;
	  if (process) {		/* check, if doesn't exist, then ignore and lose */
	    process(vt);
	  }
	} else {
	  handled = 0;
	}
	state=0;
	break;
      }
    } while (!handled);
  }
  vt->state = state;
}

/*
  Allocate a new line
*/
struct vt_line *vt_newline(struct vt_em *vt)
{
  struct vt_line *n;
  int len, i, j=0;

  len = vt->width;
  d(printf("allocating %d bytes\n", sizeof(struct vt_line) + (sizeof(char)*2*len)));
  n = malloc(sizeof(struct vt_line) + (sizeof(uint32)*len)); /* always allocates 1 more 'char' entry */
  if (n) {
    n->width = vt->width;
    n->modcount = vt->width;
    for (i=0;i<len+1;i++) {
      n->data[i] = vt->attr | VTATTR_CHANGED;
    }
    n->line = j++;
  }
  return n;
}

/*
  Setup a new VT terminal

  FIXME: check allocations
*/
struct vt_em *vt_init(struct vt_em *vt, int width, int height)
{
  struct vt_line *vl;
  int i;

  vt_list_new(&vt->lines);
#ifdef DOUBLE_BUFFER
  vt_list_new(&vt->lines_back);
#endif
  vt_list_new(&vt->scrollback);

  vt_list_new(&vt->lines_alt);

  vt->width = width;
  vt->height = height;
  vt->scrolltop = 0;
  vt->scrollbottom = height-1;
  vt->attr = VTATTR_CLEAR;	/* default 'clear' character */
  vt->mode = 0;
  vt->remaptable = 0;		/* no character remapping */
  for (i=0;i<height;i++) {
    vl = vt_newline(vt);
    vl->line = i;
    vt_list_addtail(&vt->lines, (struct vt_listnode *)vl);
#ifdef DOUBLE_BUFFER
    vl = vt_newline(vt);
    vl->line = i;
    vt_list_addtail(&vt->lines_back, (struct vt_listnode *)vl);
#endif
    vl = vt_newline(vt);
    vl->line = i;
    vt_list_addtail(&vt->lines_alt, (struct vt_listnode *)vl);
  }
  vt->cursorx=0;
  vt->cursory=0;

  vt->childfd = -1;
  vt->childpid = -1;

  for (i=0;i<VTPARAM_MAXARGS;i++) {
    vt->args[i]=&vt->args_mem[i*VTPARAM_ARGMAX];
  }

  vt->this = (struct vt_line *)vt->lines.head;

  vt->scrollbacklines=0;
  vt->scrollbackoffset=0;
  vt->scrollbackold=0;
  vt->scrollbackmax=50;		/* maximum scrollback lines */

  for(i=0;i<256;i++) {		/* initialise dec special char remapping */
    vt_remap_dec[i]=(i>95)&&(i<128)?(i-95):i;
  }

  vt->Gx=0;			/* reset all fonts */
  vt->G[0]=0;
  vt->G[1]=vt_remap_dec;
  vt->G[2]=0;
  vt->G[3]=0;

  vt_mem_init(&vt->mem_list);

  return vt;
}

/*
  start a child process running in the VT emulator

   fork the child process

*/
int vt_forkpty(struct vt_em *vt)
{
  char ttyname[256];

  vt->childpid = init_subshell(&vt->childfd, ttyname, &vt->msgfd);
  if (vt->childpid>0) {
    fcntl(vt->childfd, F_SETFL, O_NONBLOCK);
    resize_subshell(vt->childfd, vt->width, vt->height, vt->width*8, vt->height*8); /* FIXME: approx sizes only */
  }

  d(fprintf(stderr, "program started on pid %d, on tty %s\n", vt->childpid, ttyname));
  return vt->childpid;
}

/*
  write to the child process
*/
int vt_writechild(struct vt_em *vt, char *buffer, int len)
{
  return write(vt->childfd, buffer, len);
}

/*
  read from the child process
*/
int vt_readchild(struct vt_em *vt, char *buffer, int len)
{
  return read(vt->childfd, buffer, len);
}

/*
  signal the child process
*/
int vt_killchild(struct vt_em *vt, int signal)
{
  if (vt->childpid!=-1) {
    return kill(vt->childpid, signal);
  }
  return -1;
}

/*
  close the child connection pty, and invalidates it.
*/
int vt_closepty(struct vt_em *vt)
{
  int ret;

  d(printf("vt_closepty called\n"));

  if (vt->childfd != -1){
	  ret = close(vt->childfd);
	  close_msgfd(vt->childpid);
	  vt->msgfd = vt->childfd = -1;
  } else
	  ret = 0;
  return ret;
}

/*
  destroy all data associated with a given 'vt' structure
*/
void vt_destroy(struct vt_em *vt)
{
  struct vt_line *wn;

  d(printf("vt_destroy called\n"));

  vt_closepty(vt);

#ifdef SCROLLBACK_BUFFER
  /* clear out all scrollback memory */
  vt_scrollback_set(vt, 0);
#endif

  /* clear all visible lines */
  while ( (wn = (struct vt_line *)vt_list_remhead(&vt->lines)) ) {
    free(wn);
  }

  /* and all alternate lines */
  while ( (wn = (struct vt_line *)vt_list_remhead(&vt->lines_alt)) ) {
    free(wn);
  }

  /* and all back lines */
  while ( (wn = (struct vt_line *)vt_list_remhead(&vt->lines_back)) ) {
    free(wn);
  }

  /* done */
}

/*
  resize the window to a new window size

  handles tty re-size too.
*/
void vt_resize(struct vt_em *vt, int width, int height, int pixwidth, int pixheight)
{
  int count;
  struct vt_line *wn, *nn;
  int i;
#ifdef DOUBLE_BUFFER
  int pass;
#endif
  int old_width;

  old_width = vt->width;
  vt->width = width;

  /* update scroll bottom constant */
  if (vt->scrollbottom == (vt->height-1)) {
    vt->scrollbottom = height-1;
  }

  /* if we just got smaller, discard unused lines (from top of window - move to scrollback) */
  if (height< vt->height) {
    count = (vt->height-height);
    d(printf("removing %d lines to smaller window\n", count));
    while (count>0) {
      if ( (wn = (struct vt_line *)vt_list_remhead(&vt->lines)) ) {
	if ((vt->mode & VTMODE_ALTSCREEN)==0)
	  vt_scrollback_add(vt, wn); /* add it to scrollback buffer */
	free(wn);
      }
      /* and for 'alternate' screen */
      if ( (wn = (struct vt_line *)vt_list_remhead(&vt->lines_alt)) ) {
	if ((vt->mode & VTMODE_ALTSCREEN)!=0)
	  vt_scrollback_add(vt, wn); /* add it to scrollback buffer */
	free(wn);
      }
#ifdef DOUBLE_BUFFER
      /* repeat for backbuffer */
      if ( (wn = (struct vt_line *)vt_list_remhead(&vt->lines_back)) )
	free(wn);
#endif
      count--;
    }

    if (vt->cursory>=height) {	/* fix up cursors on resized window */
      vt->cursory=height-1;
      d(printf("cursor too big, reset\n"));
    }
    if (vt->scrollbottom>=height) {
      vt->scrollbottom=height-1;
      d(printf("scroll bottom too big, reset\n"));
    }
    if (vt->scrolltop>=height) {
      vt->scrolltop=height-1;
      d(printf("scroll top too big, reset\n"));
    }
  } else if (height>vt->height) {
    /* if we just got bigger, add new lines to the bottom */
    /* FIXME: this should look in the scrollback buffer if it exists 
      no - would upset the memory allocation of the scrollback buffer */
    count = (height-vt->height);
    d(printf("adding %d lines to buffer window\n", count));
    for(i=0;i<count;i++) {
      vt_list_addtail(&vt->lines, (struct vt_listnode *)vt_newline(vt));
#ifdef DOUBLE_BUFFER
      vt_list_addtail(&vt->lines_back, (struct vt_listnode *)vt_newline(vt));
#endif
      vt_list_addtail(&vt->lines_alt, (struct vt_listnode *)vt_newline(vt));
    }
  } /* otherwise width may have changed? */

  vt->height=height;

  if (vt->cursorx>=width) {	/* fix up cursor on resized window */
    vt->cursorx=width-1;
    d(printf("cursor x too wide, reset\n"));
  }

  /* now, scan all lines visible, and make them the right width */
  /* for all 3 'buffers', onscreen, offscreen and alternate */
#ifdef DOUBLE_BUFFER
  for(pass=0, wn = (struct vt_line *)vt->lines.head;
      pass<3;pass++) {
#else
    wn = (struct vt_line *)vt->lines.head;
#endif
    nn = wn->next;
    count=height;
    while (nn) {
      if (wn->width != width) {
	wn = realloc(wn, sizeof(struct vt_line) + (sizeof(uint32)*width)); /* always allocates 1 more 'char' pos */
	if (!wn) {
	  printf("WARNING: memory failure.  expect the unexpected!\n");
	  break;			/* memory failure ... quit? */
	}
	wn->next->prev = wn;	/* re-link line into linked list */
	wn->prev->next = wn;
	for(i=wn->width;i<width;i++) { /* if the line got bigger, fix it up */
	  wn->data[i]=vt->attr | VTATTR_CHANGED;
	  wn->modcount++;		/* FIXME: optimise? */
	}
	wn->width = width;
      }
      wn = nn;
      nn = nn->next;
    }
#ifdef DOUBLE_BUFFER
    if (pass==0)
      wn = (struct vt_line *)vt->lines_back.head;
    else if (pass==1)
      wn = (struct vt_line *)vt->lines_alt.head;
  }
#endif

  /* re-fix 'this line' pointer */
  vt->this = (struct vt_line *)vt_list_index(&vt->lines, vt->cursory);

  resize_subshell(vt->childfd, width, height, pixwidth, pixheight);

  d(printf("resized to %d,%d, this = %p\n", vt->width, vt->height, vt->this));
}

/*
  reports a button click to the terminal, (if it is asking
  for them at the moment).

  button = 1, 2, 3.  If button=0, then signifies button up
  qual = qualifiers (shift/control/etc)
      bit 

  returns true if the button event should be swallowed

  FIXME: handle qualifiers
*/
int vt_report_button(struct vt_em *vt, int button, int qual, int x, int y)
{
  char mouse_info[16];

  if ( (vt->mode & VTMODE_SEND_MOUSE) &&
       (y>=0) /*&&
		!(event->state & GDK_SHIFT_MASK)*/
       ) {
    sprintf(mouse_info, "\033[M%c%c%c",
	    ' ' + ((button - 1)&3) /*+ 
				     ((event->state&GDK_SHIFT_MASK)?4:0) +
				     ((event->state&GDK_MOD1_MASK)?8:0) +
				     ((event->state&GDK_CONTROL_MASK)?16:0)*/,
	    x+' '+1,
	    y+' '+1);
    vt_writechild(vt, mouse_info, strlen(mouse_info));
    return 1;
  }
  return 0;
}
