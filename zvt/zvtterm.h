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
#include "vtx.h"

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

    int cursor_on:1;		/* on/off cursor */
    int cursor_filled:1;		/* is the cursor filled? */
    int cursor_blink_state:1;	/* cursor blink state */

    int charwidth;		/* size of characters */
    int charheight;

    gint input_id;		/* input handler id */

    /* sub-objects required */
    GtkAdjustment *adjustment;	/* scrollback position adjustement */

    /* internal data */
    GdkCursor *cursor_bar,	/* I beam cursor */
      *cursor_dot,		/* the blank cursor */
      *cursor_current;		/* current active cursor */
    GdkPixmap *cursor_dot_pm;	/* 'dot' pixmap, for dot cursor (invisible) */
    guint timeout_id;		/* id of timeout function */
    GdkFont *font,		/* current normal font */
      *font_bold;		/* current bold font */
  };

  struct _ZvtTermClass
  {
    GtkWidgetClass parent_class;
  };


  GtkWidget*    zvt_term_new                    (void);
  int		zvt_term_forkpty		(ZvtTerm *term);

  guint         zvt_term_get_type               (void);

  void          zvt_term_set_scrollback         (ZvtTerm *term, int lines);

  void          zvt_term_set_font_name          (ZvtTerm *term, char *name);
  void          zvt_term_set_fonts              (ZvtTerm *term,
						 GdkFont *font, GdkFont *font_bold);

  void          zvt_term_hide_pointer           (ZvtTerm *term);
  void          zvt_term_show_pointer           (ZvtTerm *term);

  /*GtkAdjustment* zvt_term_get_adjustment         (ZvtTerm      *terminal);
    void           zvk_term_set_adjustment         (ZvtTerm      *dial,
    GtkAdjustment *adjustment);*/
#ifdef __cplusplus
	   }
#endif /* __cplusplus */


#endif /* __ZVT_TERM_H__ */

