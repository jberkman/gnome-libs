/*  update.c - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  A virtual terminal emulator.
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

/*
  This module handles the update of the 'on-screen' virtual terminal
  from the 'off-screen' buffer.

  It also handles selections, and making them visible, etc.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "lists.h"
#include "memory.h"
#include "vt.h"
#include "vtx.h"

#define d(x)

/*
  invert the reverse attribute if the text is selected
  and then draw as usual.

  this is unecessarily complicated by the fact selstarty not guaranteed
  to be < selendy (and same for selstartx/selendx)
*/
void vt_draw_text_select(struct _vtx *vx, int col, int row, char *text, int len, int attr)
{
  int newattr;
  int startlen,midlen,endlen;	/* start/mid/end sections */
  int sx, ex;

  if (vx->selected &&
      (((row >= (vx->selstarty - vx->vt.scrollbackoffset)) && /* normal order select */
      (row <= (vx->selendy - vx->vt.scrollbackoffset))) ||
      ((row <= (vx->selstarty - vx->vt.scrollbackoffset)) && /* start<end */
      (row >= (vx->selendy - vx->vt.scrollbackoffset)))) ) {

    d(printf("drawing text from %d for %d\n", col, len));

    newattr = attr ^ VTATTR_REVERSE;
    sx = 0;
    ex = col+len;

    if (vx->selstarty<=vx->selendy) {
      if (row == (vx->selstarty-vx->vt.scrollbackoffset))
	sx = vx->selstartx;
      if (row == (vx->selendy-vx->vt.scrollbackoffset))
	ex = vx->selendx;
    } else {
      if (row == (vx->selendy-vx->vt.scrollbackoffset))
	sx = vx->selendx;
      if (row == (vx->selstarty-vx->vt.scrollbackoffset)) {
	ex = vx->selstartx;
      }
    }

    /* check startx<endx, if on the same line */
    if ( (sx>ex) &&
	 (row == vx->selstarty-vx->vt.scrollbackoffset) &&
	 (row == vx->selendy-vx->vt.scrollbackoffset) ) {
      startlen=sx; sx=ex; ex=startlen; /* use startlen as a temp */
    }

    /* now calculate which bits of the line are selected/otherwise */
    startlen = (sx-col)<0?0:(sx-col);
    endlen = (col+len-ex)<0?0:(col+len-ex);
    if (startlen>len)
      startlen = len;
    if (endlen>len)
      endlen = len;
    midlen = (len-startlen-endlen);

    d(printf("startx = %d, endx = %d\n", sx, ex));
    d(printf("drawing text <%d> normal <%d> selected <%d> normal\n", startlen, midlen, endlen));

    if (startlen)
      vt_draw_text(vx->user_data, col, row, text, startlen, attr);
    if (midlen)
      vt_draw_text(vx->user_data, col+startlen, row, text+startlen, midlen, newattr);
    if (endlen)
      vt_draw_text(vx->user_data, col+startlen+midlen, row, text+startlen+midlen, endlen, attr);
  } else {
    vt_draw_text(vx->user_data, col, row, text, len, attr);
  }
}

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
  int attr;
  uint32 c;
  struct vt_line *bl;
  int noclear;			/* clear before writing text? */

  d(printf("updating line %d: ", line));
  d(fwrite(l->data, l->width, 1, stdout));
  d(printf("\n"));

  d(printf("updating line from (%d-%d) ->", start, end));

  /* check range conditions */
  if (end>l->width) {
    end = l->width;
  }
  if (start>=end) {
    return;
  }

  runbuffer = alloca(vx->vt.width * sizeof(char));

  bl = (struct vt_line *)vt_list_index(&vx->vt.lines_back, line);

  /* some sanity checks */
  g_return_if_fail (bl != NULL);
  g_return_if_fail (runbuffer != NULL);
  g_return_if_fail (bl->width == l->width);

  /* work out the minimum range of change */
  if (!always) {
    
    while (start<end) {
      if ((l->data[start]&0x7fffffff) != (bl->data[start]&0x7fffffff))
	break;
      start++;
    }
    
    while (end>start) {
      if ((l->data[end-1]&0x7fffffff) != (bl->data[end-1]&0x7fffffff))
	break;
      end--;
    }
  }  /* else, update everything */

  /* see if on-screen is all background ... */
  vx->back_match=1;
  for(i=start;i<end;i++) {
    if ((bl->data[i] & ~VTATTR_CHANGED) != (vx->vt.attr&~VTATTR_CHANGED)) {
      vx->back_match=0;
      break;
    }
  }

  /* copy changed section of line to destination line */
  for(i=start;i<end;i++) {
    bl->data[i]=l->data[i];
  }
  bl->line = line;

  d(printf("actually (%d-%d)?\n", start, end));

  run=0;
  attr=0;
  runstart = 0;
  p = 0;
  for(i=start;i<end;i++) {
#if 0
    if (always || ((l->data[i] & VTATTR_CHANGED)) ) {
#endif
      if (run==0) {
	runstart = i;
	p = runbuffer;
	attr = l->data[i] & 0x7fff0000;
      } else {
	if (attr != (l->data[i] & 0x7fff0000)) { /* check run of same type ... */
	  vt_draw_text_select(vx, runstart, line, runbuffer, run, attr);
	  runstart = i;
	  p = runbuffer;
	  run=0;
	  attr = l->data[i] & 0x7fff0000;
	}
      }
      c = l->data[i] & 0xff;
      /* FIXME: this is needed for alternate charsets
	 will need to do something else to mark empty-space and tabs? */
      if ((c==0) || (c==9))
	c=' ';
      /*if (c<32)*/			/* all control chars ->space */
      /*c=' ';*/
      *p++=c;
      run++;
#if 0
    } else {
      if (run) {
	d(printf("found a run of %d characters from %d: '", run, runstart));
	d(fwrite(runbuffer, run, 1, stdout));
	vt_draw_text_select(vx, runstart, line, runbuffer, run, attr);
	d(printf("'\n"));
	run=0;
      }
    }
#endif
    l->data[i] &= ~VTATTR_CHANGED;
  }
  if (run) {
    d(printf("found a run of %d characters from %d: '", run, runstart));
    d(fwrite(runbuffer, run, 1, stdout));
    vt_draw_text_select(vx, runstart, line, runbuffer, run, attr);
    d(printf("'\n"));
  }
  l->modcount = 0;
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

  FIXME: check this
*/
void vt_scroll_update(struct _vtx *vx, int firstline, struct vt_line *fn, int count, int offset)
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

    /*
1
2
3
4
5
6
7
8
9

scroll(3, 4, 1);

1
2
-7
3
4
5
6
8
9

scroll(3, 4, -1);
 

1
3
4
5
6
-2
7
8
    */
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
  int count;
  int old_state;

  d(printf("updating screen\n"));

  old_state = vt_cursor_state(vx->user_data, 0);

  if (vx->vt.scrollbackoffset != 0) {
    if (vx->vt.scrollbackoffset != vx->vt.scrollbackold ||
	update_state&UPDATE_SCROLLBACK) {

      /* still experimental */
#ifdef OPT_SCROLLBACK
      /* FIXME: this could be optimised, based on vt->scrollbackold! */
      if (!(update_state&UPDATE_SCROLLBACK)) {
	/*
	  offset>0 -> scroll down
	    scroll from line offset, (total-offset) lines, by offset lines
	  offset<0 -> scroll up
	    scroll from line total-offset
	*/
	offset = vt->scrollbackold-vt->scrollbackoffset;
	if (offset>0) {		/* scrolled .. down */
	  if (offset < vt->height-8) { /* otherwise just do all of it */
	    (printf("scrolling down\n"));	/* need to add on top */
	    vt_scroll_area(0, vt->height, -offset);
	  }
	} else {		/* scrolled .. up */
	  if ((-offset) < vt->height-8) {
	    printf("scrolling up\n"); /* need to add on bottom */
	    vt_scroll_area(0, vt->height, -offset);
	    if ( (-vt->scrollbackoffset) < vt->height) {
	      printf("*** scrolls into normal screen (%d %d)\n", vt->scrollbackoffset, vt->height+vt->scrollbackoffset);
	      count = vt->scrollbackoffset+vt->height;
	      firstline=0;
	      /*line=vt->height+offset;*/ /* FIXME: must also specify the number of lines to skip ... */
	      line=-vt->scrollbackoffset;
	    } else {
	      count = -offset;	/* how many lines to add */
	      firstline= (- vt->scrollbackoffset); /* how many lines to go back ... */
	      line=0;
	    }

	    printf("updating: count=%d, firstline=%d, line=%d\n", count, firstline, line);

	    goto skip_countset;
	  }
	}
      }

#endif
      /* we are in scroll back mode ... */
      /* must first print any scroll-back buffer lines that need to be displayed */
      /* find the scrollback lines visible, and display them .. */
      count=vx->vt.height;		/* lines left to draw */
      firstline= (- vx->vt.scrollbackoffset); /* how many lines to go back ... */
      line=0;
#ifdef OPT_SCROLLBACK
	skip_countset:
#endif

      wn = (struct vt_line *)vx->vt.scrollback.tailpred;
      nn = wn->prev;
      while (nn && count>0 && firstline>0) {
	if (firstline <= vx->vt.height) { /* are we on the screen yet? */
	  vt_line_update(vx, wn, firstline-1, 1, 0, wn->width);
	  count--;
	  line++;
	}
	firstline--;
	wn=nn;
	nn=nn->prev;
      }
      /* now, find working buffer lines visible, and display them */
      wn = (struct vt_line *)vx->vt.lines.head;
      nn = wn->next;
      while (nn && count>0) {
	vt_line_update(vx, wn, line, 1, 0, wn->width);
	wn=nn;
	nn=nn->next;
	line++;
	count--;
      }
      vx->vt.scrollbackold = vx->vt.scrollbackoffset;
    }
  } else {
    if (update_state==0) {
				/* perform simple whole-screen update? */
				/* perform optimised update */
      wn = (struct vt_line *)vx->vt.lines.head;
      nn = wn->next;
      firstline = 0;		/* this isn't really necessary */
      fn = wn;
      while (nn) {
	d(printf("scanning line %d, offset=%d : ", line, (wn->line-line)));
	d(printf("(%d - %d)\n", line, wn->line));
	if ((wn->line!=-1) && ((offset = wn->line-line) >0)) {
	  wn->line = line;
#ifdef CHANGE_CHECK
	  if (wn->modcount > (wn->width>>4)) { /* simple update metric, not much changed? */
	    if (oldoffset != 0) {
	      d(printf("scrolling (1.0)\n"));
	      vt_scroll_update(vx, firstline, fn, line-firstline, oldoffset); /* scroll/update a line */
	    }
	    offset=0;
	  } else {
#endif
	    if (offset == oldoffset) {
	      /* accumulate "update" lines */
	    } else {
	      if (oldoffset != 0) {
		d(printf("scrolling (1.1)\n"));
		vt_scroll_update(vx, firstline, fn, line-firstline, oldoffset); /* scroll/update a line */
	      }
	      d(printf("found a scrolled line\n"));
	      firstline = line;	/* first line to scroll */
	      fn = wn;
	    }
#ifdef CHANGE_CHECK
	  }
#endif
	} else {
	  if (oldoffset != 0) {
	    d(printf("scrolling (1.2)\n"));
	    vt_scroll_update(vx, firstline, fn, line-firstline, oldoffset); /* scroll/update a line */
	  }
	  offset=0;
	}

	oldoffset = offset;
	line ++;
	wn = nn;
	nn = nn->next;
      }
      if (oldoffset != 0) {
	d(printf("scrolling (1.3)\n"));
	d(printf("oldoffset = %d, must scroll\n", oldoffset));
	vt_scroll_update(vx, firstline, fn, line-firstline, oldoffset); /* scroll/update a line */
      }

      /* now scan backwards ! */
      wn = wn->prev;
      nn = wn->prev;
      line = vx->vt.height;
      oldoffset = 0;
      while (nn) {
	line--;
	d(printf("scanning line %d, offset=%d : ", line, (wn->line-line)));
	if ((wn->line !=-1) && ((offset = wn->line-line) < 0)) {
	  wn->line = line;
#ifdef CHANGE_CHECK
	  if (wn->modcount > (wn->width>>4)) { /* simple update metric, not much changed? */
	    d(printf("changed too much - ignoring\n"));
	    if (oldoffset != 0) {
	      d(printf("scrolling (2.0)\n"));
	      vt_scroll_update(vx, line, wn->next, firstline-line+1, oldoffset); /* scroll/update a line, from this line *down* */
	    }
	    offset=0;
	  } else {
#endif
	    if (offset == oldoffset) {
	      /* accumulate "update" lines */
	    } else {
	      if (oldoffset != 0) {
		d(printf("scrolling (2.1)\n"));
		vt_scroll_update(vx, line, wn->next, firstline-line+1, oldoffset); /* scroll/update a line */
	      }
	      d(printf("found a scrolled line\n"));
	      firstline = line;	/* first line to scroll */
	      fn = wn;
	    }
#ifdef CHANGE_CHECK
	  }
#endif
	} else {
	  if (oldoffset != 0) {
	    d(printf("scrolling (2.2)\n"));
	    vt_scroll_update(vx, line+1, wn->next, firstline-line, oldoffset); /* scroll/update a line */
	  }
	  offset=0;
	}

	oldoffset = offset;
	wn = nn;
	nn = nn->prev;
      }
      if (oldoffset != 0) {
	d(printf("scrolling (2.3)\n"));
	d(printf("oldoffset = %d, must scroll 2\n", oldoffset));
	vt_scroll_update(vx, 0, wn, firstline-line+1, oldoffset); /* scroll/update a line */
      }
    }

    /* now, re-scan, since all lines should be at the right position now,
     and update as required */

    wn = (struct vt_line *)vx->vt.lines.head;
    nn = wn->next;
    firstline = 0;		/* this isn't really necessary */
    fn = wn;
    line=0;
    while (nn) {
      if (wn->line==-1) {
	vt_line_update(vx, wn, line, 0, 0, wn->width);
      } else if (wn->modcount || update_state) {
	vt_line_update(vx, wn, line, update_state, 0, wn->width);
      }
      wn->line = line;		/* make sure line is reset */
      line++;
      wn=nn;
      nn=nn->next;
    }
  }

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

  lines = cey-csy;
  wn = (struct vt_line *)vt_list_index(&vx->vt.lines, csy);
  if (wn) {
    nn = wn->next;
    while ((csy<=cey) && nn) {
      d(printf("updating line %d\n", csy));
      vt_line_update(vx, wn, csy, 1, csx, cex);
      csy++;
      wn = nn;
      nn = nn->next;
    }
  }

  vt_cursor_state(vx->user_data, old_state);
}

/*
  returns true if 'c' is in a 'wordclass' (for selections)
*/
int vt_in_wordclass(uint32 c)
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
    while ((sx>0) && ((s->data[sx]&0xff) == 0))
      sx--;

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
char *vt_expand_line(struct vt_line *l, int start, int end, char *out)
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
void vt_draw_selection_part(struct _vtx *vx, int sx, int sy, int ex, int ey)
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

  if (vx->vt.scrollbackoffset == 0) {
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
