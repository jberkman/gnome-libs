/*  update.c - Zed's Virtual Terminal
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
  This module handles the update of the 'on-screen' virtual terminal
  from the 'off-screen' buffer.

  It also handles selections, and making them visible, etc.
*/

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "lists.h"
#include "memory.h"
#include "vt.h"
#include "vtx.h"

#define d(x)

/*
  update line 'line' (node 'l') of the vt

  always==1  assumes all line data is stale
  start/end columns to update
*/
static void vt_line_update(struct _vtx *vx, struct vt_line *l, int line, int always,
			   int start, int end)
{
  int i;
  int run;
  int runstart;
  char *runbuffer, *p;
  uint32 attr, newattr;
  uint32 c;
  int ch;
  struct vt_line *bl;
  int sx, ex;			/* start/end selection */

  d(printf("updating line %d: ", line));
  d(fwrite(l->data, l->width, 1, stdout));
  d(printf("\n"));

  d(printf("updating line from (%d-%d) ->", start, end));

  /* for 'scrollback' where the window has changed size -
     create a proper-length line (this is easier than special case code below */
  bl = (struct vt_line *)vt_list_index(&vx->vt.lines_back, line);

  /* some sanity checks */
  g_return_if_fail (bl != NULL);
  g_return_if_fail (bl->next != NULL);

  if (bl->width > l->width) {
    struct vt_line *newline;

    d(printf("line size mismatch %d != %d\n", bl->width, l->width));

    /* create a new temp line, and set it to a clear
       last-character (attributes) of the new line */
    newline = alloca(sizeof(*l)+bl->width*sizeof(bl->data[0]));
    memcpy(newline, l, sizeof(*l)+l->width*sizeof(l->data[0]));
    c = l->data[l->width-1]&0xffff0000;
    for (i=l->width;i<bl->width;i++) {
      newline->data[i] = c;
    }
    /* over-write the 'l' pointer */
    l = newline;
  }

  /* check range conditions */
  if (end>l->width) {
    end = l->width;
  }
  if (start>=end) {
    return;
  }

  runbuffer = alloca(vx->vt.width * sizeof(char));

  /* work out if selections are being rendered */
  if (vx->selected &&
      (((line >= (vx->selstarty - vx->vt.scrollbackoffset)) && /* normal order select */
      (line <= (vx->selendy - vx->vt.scrollbackoffset))) ||
      ((line <= (vx->selstarty - vx->vt.scrollbackoffset)) && /* start<end */
      (line >= (vx->selendy - vx->vt.scrollbackoffset)))) ) {
    
    /* work out range of selections */
    sx = 0;
    ex = l->width;
    
    if (vx->selstarty<=vx->selendy) {
      if (line == (vx->selstarty-vx->vt.scrollbackoffset))
	sx = vx->selstartx;
      if (line == (vx->selendy-vx->vt.scrollbackoffset))
	ex = vx->selendx;
    } else {
      if (line == (vx->selendy-vx->vt.scrollbackoffset))
	sx = vx->selendx;
      if (line == (vx->selstarty-vx->vt.scrollbackoffset)) {
	ex = vx->selstartx;
      }
    }

    /* check startx<endx, if on the same line, swap if so */
    if ( (sx>ex) &&
	 (line == vx->selstarty-vx->vt.scrollbackoffset) &&
	 (line == vx->selendy-vx->vt.scrollbackoffset) ) {
      int tmp;
      tmp=sx; sx=ex; ex=tmp;
    }
  } else {
    sx = -1;
    ex = -1;
  }
  /* ^^ at this point sx -- ex needs to be inverted (it is selected) */

  /* work out the minimum range of change */
  if (!always) {
    
    while (start<end) {
      c = l->data[start]&0x7fffffff;
      if (start>=sx && start<ex)
	c ^= VTATTR_REVERSE;
      if (c != (bl->data[start]&0x7fffffff))
	break;
      start++;
    }
    
    while (end>start) {
      c = l->data[end-1]&0x7fffffff;
      if (end>=sx && end<ex)
	c ^= VTATTR_REVERSE;
      if (c != (bl->data[end-1]&0x7fffffff))
	break;
      end--;
    }
  }  /* else, update everything */

  /* see if on-screen is all background ... */
  /* (this is messy, and probably slow?) */
  vx->back_match=1;
  for(i=start;i<end;i++) {
    /* check for clear of the same background colour */
    c = bl->data[i]&0x7fffffff;
    ch = c & 0xffff;
    if (i>=sx && i<ex)
      c ^= VTATTR_REVERSE;
    if ((ch!=0 && ch!=9 && ch!=' ')
	|| ((c & (0x7c000000|VTATTR_BACKCOLOURM))
	    != (l->data[i] & (0x7c000000|VTATTR_BACKCOLOURM)))) {
      d(printf("bl->data[i] = %08x != %08x, c=%d (back now=%d not %d)\n", bl->data[i], l->data[i], c, (l->data[i]&VTATTR_BACKCOLOURM)>>VTATTR_BACKCOLOURB, (bl->data[i]&VTATTR_BACKCOLOURM)>>VTATTR_BACKCOLOURB));
      vx->back_match=0;
      break;
    }
  }

  d(printf("actually (%d-%d)?\n", start, end));

  run=0;
  attr=0;
  runstart = 0;
  p = 0;
  for(i=start;i<end;i++) {
    /* map on 'selected' areas, and copy to screen buffer */
    newattr = l->data[i] & 0x7fff0000;
    if (i>=sx && i<ex) {
      newattr ^= VTATTR_REVERSE;
      bl->data[i]=l->data[i]^VTATTR_REVERSE;
    } else {
      bl->data[i]=l->data[i];
    }

    if (run==0) {
      runstart = i;
      p = runbuffer;
      attr = newattr;
    } else {
      if (attr != newattr) { /* check run of same type ... */
	vt_draw_text(vx->user_data, runstart, line, runbuffer, run, attr);
	runstart = i;
	p = runbuffer;
	run=0;
	attr = newattr;
      }
    }
    c = l->data[i] & 0xff;
    /* FIXME: this is needed for alternate charsets
       will need to do something else to mark empty-space and tabs? */
    if ((c==0) || (c==9))
      c=' ';
    *p++=c;
    run++;
  }
  if (run) {
    d(printf("found a run of %d characters from %d: '", run, runstart));
    d(fwrite(runbuffer, run, 1, stdout));
    vt_draw_text(vx->user_data, runstart, line, runbuffer, run, attr);
    d(printf("'\n"));
  }
  l->modcount = 0;
  bl->line = line;
}

int vt_get_attr_at (struct _vtx *vx, int col, int row)
{
  struct vt_line *line;
  
  line = (struct vt_line *) vt_list_index (&vx->vt.lines_back, row);
  return line->data [col];
}

/*
  scroll/update a section of lines
  from firstline (fn), scroll count lines 'offset' lines
*/
static void vt_scroll_update(struct _vtx *vx, int firstline, int count, int offset)
{
  struct vt_line *tn, *bn, *dn;	/* top/bottom of scroll area */
  int i;

  d(printf("scrolling %d lines from %d, by %d\n", count, firstline, offset));

  vt_scroll_area(vx->user_data, firstline, count, offset);

  d({
    printf("before:\n");
    dn = &vx->vt.lines_back.head;
    while(dn) {
      printf("node %p:  n: %p  p:%p\n", dn, dn->next, dn->prev);
      dn = dn->next;
    }
  });


  /* find start/end of scroll area */
  if (offset>0) {
    /* grab n lines at bottom of scroll area, and move them to above it */
    tn = (struct vt_line *)vt_list_index(&vx->vt.lines_back, firstline);
    bn = (struct vt_line *)vt_list_index(&vx->vt.lines_back, firstline+offset-1);
    dn = (struct vt_line *)vt_list_index(&vx->vt.lines_back, firstline+count+offset);
  } else {
    /* grab n lines at top of scroll area, and move them to below it */
    /* FIXME: this is wrong */
    tn = (struct vt_line *)vt_list_index(&vx->vt.lines_back, firstline+count+offset);
    bn = (struct vt_line *)vt_list_index(&vx->vt.lines_back, firstline+count-1);
    dn = (struct vt_line *)vt_list_index(&vx->vt.lines_back, firstline+offset);
  }

  /*    
    0->dn->4->tn->1->2->bn->5
    0->dn->4->5
    0->tn->1->2->bn->dn->4->5
  */

  /* remove the scroll segment */
  tn->prev->next = bn->next;
  bn->next->prev = tn->prev;
  
  /* insert it again at the destination */
  tn->prev = dn->prev;
  dn->prev->next = tn;
  bn->next = dn;
  dn->prev = bn;

  d({
    printf("After\n");
    dn = &vx->vt.lines_back.head;
    while(dn) {
      printf("node %p:  n: %p  p:%p\n", dn, dn->next, dn->prev);
      dn = dn->next;
    }
  });

  /* need to clear tn->bn lines */
  /* FIXME: maybe if we didn't 'clear' this, and we dont 'clear' the screen after
     a scroll, it will make things faster ... but we'd have to 'copy' stuff here
     so it matches the result of a scroll */
  do {
    d(printf("clearning line %d\n", tn->line));
    for(i=0;i<tn->width;i++) {
      tn->data[i]=vx->vt.attr; /* not sure if this is the correct thing to 'clear' with */
    }
  } while ((tn!=bn) && (tn=tn->next));
}

/*
  do an optimised update of the screen
  performed in 3 passes -
    first a scroll update pass, for down scrolling
    then a scroll update pass, for up scrolling
  finally a character update pass

  if state==0, perform optimised update
  otherwise perform refresh update (update whole screen)
*/
void vt_update(struct _vtx *vx, int update_state)
{
  int line=0;
  int offset;
  int oldoffset=0;
  struct vt_line *wn, *nn, *fn;
  int firstline;
  int old_state;
  int update_start=-1;	/* where to start/stop update */
  int update_end=-1;

  d(printf("updating screen\n"));

  old_state = vt_cursor_state(vx->user_data, 0);

  /* find first line of visible screen, take into account scrollback */
  offset = vx->vt.scrollbackoffset;
  if (offset<0) {
    wn = (struct vt_line *)vt_list_index(&vx->vt.scrollback, offset);
    if (wn==0) {
      /* check for error condition */
      printf("LINE UNDERFLOW!\n");
      wn = (struct vt_line *)vx->vt.scrollback.head;
    }
  } else {
    wn=(struct vt_line *)vx->vt.lines.head;
  }
  
  /* updated scrollback if we can ... otherwise dont */
  if (!(update_state&UPDATE_REFRESH)) {

    /* if scrollback happening, then force update of newly exposed areas - calculate here */
    offset = vx->vt.scrollbackoffset-vx->vt.scrollbackold;
    if (offset>0) {
      d(printf("scrolling up, by %d\n", offset));
      update_start = vx->vt.height-offset-1;
      update_end = vx->vt.height;
    } else {
      d(printf("scrolling down, by %d\n", -offset));
      update_start = -1;
      update_end = -offset;
    }
    d(printf("forced updated from %d - %d\n", update_start, update_end));
    
    nn = wn->next;
    firstline = 0;		/* this isn't really necessary (quietens compiler) */
    fn = wn;
    while (nn && line<vx->vt.height) {

      /* enforce full update for 'scrollback' areas */
      if (line>update_start && line<update_end) {
	d(printf("forcing scroll update refresh (line %d)\n", line));
	wn->line = -1;
      }

      d(printf("%p: scanning line %d, offset=%d : line = %d\n ", wn, line, (wn->line-line), wn->line));
      d(printf("(%d - %d)\n", line, wn->line));
      if ((wn->line!=-1) && ((offset = wn->line-line) >0)) {
	wn->line = line;
	if (offset == oldoffset) {
	  /* accumulate "update" lines */
	} else {
	  if (oldoffset != 0) {
	    d(printf("scrolling (1.1)\n"));
	    vt_scroll_update(vx, firstline, line-firstline, oldoffset); /* scroll/update a line */
	  }
	  d(printf("found a scrolled line\n"));
	  firstline = line;	/* first line to scroll */
	  fn = wn;
	}
      } else {
	if (oldoffset != 0) {
	  d(printf("scrolling (1.2)\n"));
	  vt_scroll_update(vx, firstline, line-firstline, oldoffset); /* scroll/update a line */
	}
	offset=0;
      }

      /* goto next logical line */
      if (wn==(struct vt_line *)vx->vt.scrollback.tailpred) {
	d(printf("skipping to real lines at line %d\n", line));
	wn = (struct vt_line *)vx->vt.lines.head;
      } else {
	d(printf("going to next line ...\n"));
	wn = nn;
      }

      oldoffset = offset;
      line ++;
      nn = wn->next;
    }
    if (oldoffset != 0) {
      d(printf("scrolling (1.3)\n"));
      d(printf("oldoffset = %d, must scroll\n", oldoffset));
      vt_scroll_update(vx, firstline, line-firstline, oldoffset); /* scroll/update a line */
    }

    /* now scan backwards ! */
    d(printf("scanning backwards now\n"));
    
      /* does this need checking for overflow? */
    wn = wn->prev;
    nn = wn->prev;
    line = vx->vt.height;
    oldoffset = 0;
    while (nn && line) {
      line--;
      d(printf("%p: scanning line %d, offset=%d : line = %d, oldoffset=%d\n ", wn, line, (wn->line-line), wn->line, oldoffset));
      if ((wn->line !=-1) && ((offset = wn->line-line) < 0)) {
	wn->line = line;
	if (offset == oldoffset) {
	  /* accumulate "update" lines */
	} else {
	  if (oldoffset != 0) {
	    d(printf("scrolling (2.1)\n"));
	    vt_scroll_update(vx, line, firstline-line+1, oldoffset); /* scroll/update a line */
	  }
	  d(printf("found a scrolled line\n"));
	  firstline = line;	/* first line to scroll */
	  fn = wn;
	}
      } else {
	if (oldoffset != 0) {
	  d(printf("scrolling (2.2)\n"));
	  vt_scroll_update(vx, line+1, firstline-line, oldoffset); /* scroll/update a line */
	}
	offset=0;
      }

      /* goto previous logical line */
      if (wn==(struct vt_line *)vx->vt.lines.head) {
	wn = (struct vt_line *)vx->vt.scrollback.tailpred;
	d(printf("going to scrollback buffer line ...\n"));
      } else {
	d(printf("going to prev line ...\n"));
	wn = nn;
      }

      nn = wn->prev;
      oldoffset = offset;
      d(printf("nn = %p\n", nn));
    }
    if (oldoffset != 0) {
      d(printf("scrolling (2.3)\n"));
      d(printf("oldoffset = %d, must scroll 2\n", oldoffset));
      vt_scroll_update(vx, 0, firstline-line+1, oldoffset); /* scroll/update a line */
    }

    /* have to align the pointer properly for the last pass */
    if (wn==(struct vt_line *)vx->vt.scrollback.tailpred) {
      d(printf("++ skipping to real lines at line %d\n", line));
      wn = (struct vt_line *)vx->vt.lines.head;
    } else {
      d(printf("++ going to next line ...\n"));
      wn = wn->next;
    }

  }

  /* now, re-scan, since all lines should be at the right position now,
     and update as required */
  d(printf("scanning from top again\n"));

  nn = wn->next;
  firstline = 0;		/* this isn't really necessary */
  fn = wn;
  line=0;
  while (nn && line<vx->vt.height) {
    d(printf("%p: scanning line %d, was %d\n", wn, line, wn->line));
    if (wn->line==-1) {
      vt_line_update(vx, wn, line, 0, 0, wn->width);
    } else if (wn->modcount || update_state&UPDATE_REFRESH) {
      vt_line_update(vx, wn, line, update_state, 0, wn->width);
    }
    wn->line = line;		/* make sure line is reset */
    line++;

    /* goto next logical line */
    if (wn==(struct vt_line *)vx->vt.scrollback.tailpred) {
      d(printf("skipping to real lines at line %d\n", line));
      wn = (struct vt_line *)vx->vt.lines.head;
    } else {
      wn = nn;
    }
    
    nn = wn->next;
    d(printf("  -- wn = %p, wn->next = %p\n", wn, wn->next));
  }

  vx->vt.scrollbackold = vx->vt.scrollbackoffset;

  /* some debug */
#if 0
  {
    struct vt_line *wb, *nb;
    int i;

    wb = (struct vt_line *)vx->vt.lines_back.head;
    nb = wb->next;
    printf("on-screen buffer contains:\n");
    while (nb) {
      printf("%d: ", wb->line);
      for (i=0;i<wb->width;i++) {
	(printf("%c", (wb->data[i]&0xffff))); /*>=32?(wb->data[i]&0xffff):' '));*/
      }
      (printf("\n"));
      wb=nb;
      nb=nb->next;
    }
  }
#endif

  vt_cursor_state(vx->user_data, old_state);
}

/*
  updates only a rectangle of the screen

  pass in rectangle of screen to draw (in pixel coordinates)
   - assumes screen order is up-to-date.
*/
void vt_update_rect(struct _vtx *vx, int csx, int csy, int cex, int cey)
{
  int lines;
  struct vt_line *wn, *nn;
  int old_state;

  old_state = vt_cursor_state(vx->user_data, 0);	/* ensure cursor is really off */

  d(printf("updating (%d,%d) - (%d,%d)\n", csx, csy, cex, cey));

  /* bounds check */
  if (cex>vx->vt.width)
    cex = vx->vt.width;
  if (csx>vx->vt.width)
    csx = vx->vt.width;

  if (cey>=vx->vt.height)
    cey = vx->vt.height-1;
  if (csy>=vx->vt.height)
    csy = vx->vt.height-1;

  lines = cey-csy;

  /* check scrollback for current line */
  if ((vx->vt.scrollbackoffset+csy)<0) {
    wn = (struct vt_line *)vt_list_index(&vx->vt.scrollback, vx->vt.scrollbackoffset+csy);
  } else {
    wn = (struct vt_line *)vt_list_index(&vx->vt.lines, vx->vt.scrollbackoffset+csy);
  }
  if (wn) {
    nn = wn->next;
    while ((csy<=cey) && nn) {
      d(printf("updating line %d\n", csy));
      vt_line_update(vx, wn, csy, 1, csx, cex);
      csy++;

      /* skip out of scrollback buffer if need be */
      if (wn==(struct vt_line *)vx->vt.scrollback.tailpred) {
	wn = (struct vt_line *)vx->vt.lines.head;
      } else {
	wn = nn;
      }

      nn = wn->next;
    }
  }

  vt_cursor_state(vx->user_data, old_state);
}

/*
  returns true if 'c' is in a 'wordclass' (for selections)
*/
static int vt_in_wordclass(uint32 c)
{
  int ch;

  ch = c&0xff;
  return (isalnum(ch) || ispunct(ch));
}

/*
  fixes up the selection boundaries made by the mouse, so they
  match the screen data (tabs, newlines, etc)

  should this also fix the up/down ordering?
*/
void vt_fix_selection(struct _vtx *vx)
{
  struct vt_line *s, *e;

  int sx, sy, ex, ey;

  /* range check vertical limits */
  if (vx->selendy >= vx->vt.height)
    vx->selendy = vx->vt.height-1;
  if (vx->selstarty >= vx->vt.height)
    vx->selstarty = vx->vt.height-1;

  if (vx->selendy < (-vx->vt.scrollbacklines))
    vx->selendy = -vx->vt.scrollbacklines;
  if (vx->selstarty < (-vx->vt.scrollbacklines))
    vx->selstarty = -vx->vt.scrollbacklines;

  /* make sure 'start' is at the top/left */
  if ( ((vx->selstarty == vx->selendy) && (vx->selstartx > vx->selendx)) ||
       (vx->selstarty > vx->selendy) ) {
    ex = vx->selstartx;		/* swap start/end */
    ey = vx->selstarty;
    sx = vx->selendx;
    sy = vx->selendy;
  } else {			/* keep start/end same */
    sx = vx->selstartx;      
    sy = vx->selstarty;
    ex = vx->selendx;
    ey = vx->selendy;
  }

  d(printf("fixing selection, starting at (%d,%d) (%d,%d)\n", sx, sy, ex, ey));

  /* check if it is 'on screen' or in the scroll back memory */
  s = (struct vt_line *)vt_list_index(sy<0?(&vx->vt.scrollback):(&vx->vt.lines), sy);
  e = (struct vt_line *)vt_list_index(ey<0?(&vx->vt.scrollback):(&vx->vt.lines), ey);

  /* if we didn't find it ... umm? FIXME: do something? */
  switch(vx->selectiontype & VT_SELTYPE_MASK) {
  case VT_SELTYPE_LINE:
    d(printf("selecting by line\n"));
    sx=0;
    ex=e->width;
    break;
  case VT_SELTYPE_WORD:
    d(printf("selecting by word\n"));
    /* scan back over word chars */
    d(printf("startx = %d %p-> \n", sx, s->data));

    if (ex==sx && ex<e->width)
      ex++;

    if ((s->data[sx]&0xff)==0) {
      while ((sx>0) && ((s->data[sx]&0xff) == 0))
	sx--;
      if (sx &&
	  (( ((s->data[sx])&0xff)!=0x09))) /* 'compress' tabs */
	sx++;
    } else {
      while ((sx>0) &&
	     (( (vt_in_wordclass(s->data[sx])))))
	sx--;

      if (sx &&
	  (!vt_in_wordclass(s->data[sx]&0xff)) )
	sx++;
    }
    d(printf("%d\n", sx));

    /* scan forward over word chars */
    /* special cases for tabs and 'blank' character select */
    if ( !((ex >0) && ((e->data[ex-1]&0xff) != 0)) )
      while ((ex<e->width) && ((e->data[ex]&0xff) == 0))
	ex++;
    if ( !((ex >0) && (!vt_in_wordclass(e->data[ex-1]))) )
      while ((ex<e->width) && 
	     ( (vt_in_wordclass(e->data[ex]))))
	ex++;

    break;
  case VT_SELTYPE_CHAR:
  default:
    d(printf("selecting by char\n"));

    if (ex==sx && ex<e->width)
      ex++;

    if ((s->data[sx]&0xff)==0) {
      while ((sx>0) && ((s->data[sx]&0xff) == 0))
	sx--;
      if (sx &&
	  (( ((s->data[sx])&0xff)!=0x09))) /* 'compress' tabs */
	sx++;
    }

    /* special cases for tabs and 'blank' character select */
    if ( !((ex >0) && ((e->data[ex-1]&0xff) != 0)) )
      while ((ex<e->width) && ((e->data[ex]&0xff) == 0))
	ex++;
  }
  
  if ( ((vx->selstarty == vx->selendy) && (vx->selstartx > vx->selendx)) ||
       (vx->selstarty > vx->selendy) ) {
    vx->selstartx = ex;		/* swap end/start values */
    vx->selendx = sx;
  } else {
    vx->selstartx = sx;		/* swap end/start values */
    vx->selendx = ex;
  }

  d(printf("fixed selection, now    at (%d,%d) (%d,%d)\n", sx, sy, ex, ey));
}

/* convert columns start to column end of line l, into
   a byte array, and store into out */
static char *vt_expand_line(struct vt_line *l, int start, int end, char *out)
{
  int i;
  char *o;
  char c;
  int state=0;
  int dataend=0;
  int lf=0;

  /* scan from the end of the line to the start, looking for the
     actual end of screen data on that line */
  for(dataend=l->width-1;dataend>0;dataend--) {
    if (l->data[dataend] & 0xff) {
      dataend++;
      break;
    }
  }

  o = out;
  if (end>dataend) {
    lf = 1;			/* we selected past the end of the line */
    end = dataend;
  }
 
  for (i=start;i<end;i++) {
    c = l->data[i] & 0xff;
    switch(state) {
    case 0:
      if (c==0x09)
	state=1;
      else if (c<32)
	c=' ';
      *o++ = c;
      break;
    case 1:
      if (c) {			/* in tab, keep skipping null bytes */
	if (c!=0x09)
	  state=0;
	*o++ = c;
      }
      break;
    }
    d(printf("%02x", c));
  }

  if (lf)			/* did we go past the end of the line? */
    *o++='\n';

  return o;
}

/*
  return currently selected text
*/
char *vt_get_selection(struct _vtx *vx, int *len)
{
  struct vt_line *wn, *nn;
  int line;
  char *out;
  int sx, sy, ex, ey;

  if ( ((vx->selstarty == vx->selendy) && (vx->selstartx > vx->selendx)) ||
       (vx->selstarty > vx->selendy) ) {
    ex = vx->selstartx;		/* swap start/end */
    ey = vx->selstarty;
    sx = vx->selendx;
    sy = vx->selendy;
  } else {			/* keep start/end same */
    sx = vx->selstartx;      
    sy = vx->selstarty;
    ex = vx->selendx;
    ey = vx->selendy;
  }

  /* FIXME: assumes buffer hasn't shrunk much horizontally */
  if (vx->selection_data)
    free(vx->selection_data);

  if ( (vx->selection_data = malloc((ey-sy+1)*(vx->vt.width+20))) == 0 ) {
    vx->selection_size = 0;
    printf("ERROR: Cannot malloc selection buffer\n");
    return 0;
  }

  out = vx->selection_data;

  line = sy;
  wn = (struct vt_line *)vt_list_index((line<0)?(&vx->vt.scrollback):(&vx->vt.lines), line);

  if (wn)
    nn = wn->next;
  else {
    vx->selection_size = 0;
    return 0;
  }

  /* FIXME: check 'wn' exists ... */

  /* only a single line selected? */
  if (sy == ey) {
    out = vt_expand_line(wn, sx, ex, out);
  } else {
    /* scan lines */
    while (nn && (line<ey)) {
      d(printf("adding selection from line %d\n", line));
      if (line == sy) {
	out = vt_expand_line(wn, sx, wn->width, out); /* first line */
      } else {
	out = vt_expand_line(wn, 0, wn->width, out);
      }
      line++;
      if (line==0) {		/* wrapped into 'on screen' area? */
	wn = (struct vt_line *)vt_list_index(&vx->vt.lines, 0);
	nn = wn->next;
      } else {
	wn = nn;
	nn = nn->next;
      }
    }

    /* last line (if it exists - shouldn't happen?) */
    if (nn)
      out = vt_expand_line(wn, 0, ex, out);
  }

  vx->selection_size = out-vx->selection_data;
  if (len)
    *len = vx->selection_size;

  d(printf("selected text = \n");
    fwrite(vx->selection_data, vx->selection_size, 1, stdout);
    printf("\n"));

  return vx->selection_data;
}

/* clear the selection */
void vt_clear_selection(struct _vtx *vx)
{
  if (vx->selection_data) {
    free(vx->selection_data);
    vx->selection_data = 0;
    vx->selection_size = 0;
  }
}

/*
  Draw part of the selection

  Called by vt_draw_selection
*/
static void vt_draw_selection_part(struct _vtx *vx, int sx, int sy, int ex, int ey)
{
  int tmp;
  struct vt_line *l;
  int line;

  /* always draw top->bottom */
  if (sy>ey) {
    tmp = sy;sy=ey;ey=tmp;
    tmp = sx;sx=ex;ex=tmp;
  }

  d(printf("selecting from (%d,%d) to (%d,%d)\n", sx, sy, ex, ey));

  line = sy;
  if (line<0)
    l = (struct vt_line *)vt_list_index(&vx->vt.scrollback, line);
  else
    l = (struct vt_line *)vt_list_index(&vx->vt.lines, line);

  d(printf("selecting from (%d,%d) to (%d,%d)\n", sx, sy-vx->vt.scrollbackoffset, ex, ey-vx->vt.scrollbackoffset));
  while ((line<=ey) && (l->next) && ((line-vx->vt.scrollbackoffset)<vx->vt.height)) {
    d(printf("line %d = %p ->next = %p\n", line, l, l->next));
    if ((line-vx->vt.scrollbackoffset)>=0)
      vt_line_update(vx, l, line-vx->vt.scrollbackoffset, 1, 0, l->width);
    line++;
    if (line==0) {
      l = (struct vt_line *)vt_list_index(&vx->vt.lines, 0);
    } else {
      l=l->next;
    }
  }
}

/*
  draws selection from vt->selstartx,vt->selstarty to vt->selendx,vt->selendy
*/
void vt_draw_selection(struct _vtx *vx)
{
  /* draw in 2 parts
     difference between old start and new start
     difference between old end and new end
  */
  d(printf("start ... \n"));
  vt_draw_selection_part(vx,
			 vx->selstartx, vx->selstarty,
			 vx->selstartxold, vx->selstartyold);

  d(printf("end ..\n"));
  vt_draw_selection_part(vx,
			 vx->selendx, vx->selendy,
			 vx->selendxold, vx->selendyold);

  vx->selendxold = vx->selendx;
  vx->selendyold = vx->selendy;
  vx->selstartxold = vx->selstartx;
  vx->selstartyold = vx->selstarty;
}

/*
  draw/undraw the cursor
*/
void vt_draw_cursor(struct _vtx *vx, int state)
{
  unsigned char c;
  uint32 attr;

  if (vx->vt.scrollbackold == 0) {
    attr = vx->vt.this->data[vx->vt.cursorx];
    c = attr & 0xff;
    if (c==9 || c==0)			/* remap tab */
      c=' ';
    if (state) {			/* must swap fore/background colour */
      attr = (((attr & VTATTR_FORECOLOURM) >> VTATTR_FORECOLOURB) << VTATTR_BACKCOLOURB)
	| (((attr & VTATTR_BACKCOLOURM) >> VTATTR_BACKCOLOURB) << VTATTR_FORECOLOURB)
      | ( attr & ~(VTATTR_FORECOLOURM|VTATTR_BACKCOLOURM));
    }
    vx->back_match=0;		/* forces re-draw? */
    vt_draw_text(vx->user_data, vx->vt.cursorx, vx->vt.cursory, &c, 1, attr);
  }
}

struct _vtx *vtx_new(void *user_data)
{
  struct _vtx *vx;

  vx = calloc(1, sizeof(*vx));

  /* initial settings */
  vt_init(&vx->vt, 80,24);

  vx->selection_data = 0;
  vx->selection_size = 0;
  vx->selected = 0;
  vx->selectiontype = VT_SELTYPE_NONE;

  /* other parameters initialised to 0 by calloc */

  vx->user_data = user_data;

  return vx;
}

/*
  reverse of new!
*/
void vtx_destroy(struct _vtx *vx)
{
  if (vx) {
    vt_destroy(&vx->vt);
    if (vx->selection_data)
      free(vx->selection_data);
    free(vx);
  }
}
