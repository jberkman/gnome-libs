/*  zvtterm.h - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  The zvtterm widget definitions.
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
 
#ifndef __ZVT_TERM_H__
#define __ZVT_TERM_H__

#include <gdk/gdk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>
#include <zvt/vtx.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define ZVT_TERM(obj)          GTK_CHECK_CAST (obj, zvt_term_get_type (), ZvtTerm)
#define ZVT_TERM_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, zvt_term_get_type (), ZvtTermClass)
#define ZVT_IS_TERM(obj)       GTK_CHECK_TYPE (obj, zvt_term_get_type ())


  typedef struct _ZvtTerm        ZvtTerm;
  typedef struct _ZvtTermClass   ZvtTermClass;

  struct _ZvtTerm
  {
    GtkWidget widget;

    struct _vtx *vx;		/* zvt emulator */

    unsigned int cursor_on:1;		/* on/off cursor */
    unsigned int cursor_filled:1;	/* is the cursor filled? */
    unsigned int cursor_blink_state:1;	/* cursor blink state */
    unsigned int blink_enabled:1;        /* Set to on if we do blinking */
    unsigned int in_expose:1;	/* updating from within expose events */
    unsigned int scroll_on_keystroke:1;
    unsigned int scroll_on_output:1;

    int charwidth;		/* size of characters */
    int charheight;

    gint input_id;		/* input handler id */
    gint msg_id;		/* message handler id */

    /* sub-objects required */
    GtkAdjustment *adjustment;	/* scrollback position adjustement */

    /* internal data */
    GdkCursor *cursor_bar,	/* I beam cursor */
      *cursor_dot,		/* the blank cursor */
      *cursor_current;		/* current active cursor */
    gint timeout_id;		/* id of timeout function */
    GdkFont *font,		/* current normal font */
      *font_bold;		/* current bold font */
    GdkGC *scroll_gc;		/* special GC used for scrolling */
    GdkGC *fore_gc, *back_gc;	/* GCs for the foreground and background colors */
    int fore_last, back_last;	/* last colour for foreground/background gc's */
    GdkColorContext *color_ctx;	/* The color context in use, where we allocate our colors */
    gulong colors [18];		/* Our colors, pixel values. */

    GdkIC ic;			/* input context */
  };

  struct _ZvtTermClass
  {
    GtkWidgetClass parent_class;

    void (* child_died) (ZvtTerm *term);    
  };


  GtkWidget*    zvt_term_new                    (void);
  void          zvt_term_feed                   (ZvtTerm *term, char *text, int len);
  int		zvt_term_forkpty		(ZvtTerm *term);
  int           zvt_term_closepty               (ZvtTerm *term);

  int           zvt_term_killchild              (ZvtTerm *term, int signal);

  guint         zvt_term_get_type               (void);

  void          zvt_term_set_scrollback         (ZvtTerm *term, int lines);

  void          zvt_term_set_font_name          (ZvtTerm *term, char *name);
  void          zvt_term_set_fonts              (ZvtTerm *term,
						 GdkFont *font, GdkFont *font_bold);

  void          zvt_term_hide_pointer           (ZvtTerm *term);
  void          zvt_term_show_pointer           (ZvtTerm *term);

  void          zvt_term_set_blink              (ZvtTerm *term, int state);
  void          zvt_term_set_scroll_on_keystroke(ZvtTerm *term, int state);
  void          zvt_term_set_scroll_on_output   (ZvtTerm *term, int state);
  void          zvt_term_set_color_scheme       (ZvtTerm *term, gushort *red, gushort *grn, gushort *blu);
  void          zvt_term_set_default_color_scheme (ZvtTerm *term);
	
  /*GtkAdjustment* zvt_term_get_adjustment         (ZvtTerm      *terminal);
    void           zvk_term_set_adjustment         (ZvtTerm      *dial,
    GtkAdjustment *adjustment);*/
#ifdef __cplusplus
	   }
#endif /* __cplusplus */


#endif /* __ZVT_TERM_H__ */


