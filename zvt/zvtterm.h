/*  zvtterm.h - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  The zvtterm widget definitions.
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
 
#ifndef __ZVT_TERM_H__
#define __ZVT_TERM_H__

#include <gdk/gdk.h>
#include <gdk_imlib.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>
#include <zvt/vtx.h>

#include <X11/X.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* if one doesn't want to compile in transparency one would define this */
/* #define ZVT_NO_TRANSPARENT 1 */

#define ZVT_TERM(obj)          GTK_CHECK_CAST (obj, zvt_term_get_type (), ZvtTerm)
#define ZVT_TERM_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, zvt_term_get_type (), ZvtTermClass)
#define ZVT_IS_TERM(obj)       GTK_CHECK_TYPE (obj, zvt_term_get_type ())
	
/*capabilities, it's for the pixmap/transparency stuff only for now*/
enum
{
  ZVT_TERM_PIXMAP_SUPPORT = 1<<0
};

typedef struct _ZvtTerm        ZvtTerm;
typedef struct _ZvtTermClass   ZvtTermClass;

struct _ZvtTerm
{
  GtkWidget widget;

  /* scrollback position adjustement */
  GtkAdjustment *adjustment;

  GtkShadowType shadow_type;
  GdkWindow *term_window;

  /* zvt emulator */
  struct _vtx *vx;

  /* size of characters */
  int charwidth;
  int charheight;

  /* input handler, message handler, cursor blink timeout */
  gint input_id;
  gint msg_id;
  gint timeout_id;

  /* resizing */
  gboolean set_grid_size_pending;
  guint grid_width;
  guint grid_height;

  /* internal data */
  GdkCursor *cursor_bar;	/* I beam cursor */
  GdkCursor *cursor_dot;       	/* the blank cursor */
  GdkCursor *cursor_current;	/* current active cursor */
  GdkFont *font;		/* current normal font */
  GdkFont *font_bold;		/* current bold font */
  GdkGC *scroll_gc;		/* special GC used for scrolling */
  GdkGC *fore_gc, *back_gc;	/* GCs for the foreground and background colors */
  int fore_last, back_last;	/* last colour for foreground/background gc's */
  GdkColorContext *color_ctx;	/* Color context in use, where we allocate our colors */
  gulong colors [18];		/* Our colors, pixel values. */
  GdkIC *ic;			/* input context */

 /* file name of a pixmap, if NULL, none is loaded
  * and normal mode is used
  */
  char *pixmap_filename;

  /* transparency stuff, it's left in even if we don't compile
   * transparency/background pixmaps, if we don't, it will just be ignored
   */
  struct
  {
    GdkPixmap *pix; /* background pixmap */
    int x,y,w,h;    /* these are used to know if the position changed
		     * and we need to get new shaded transparent pixmap
		     */
  } background;

  /* bitfield flags -- keep at end of structure */
  unsigned int cursor_on:1;	      /* on/off cursor */
  unsigned int cursor_filled:1;	      /* is the cursor filled? */
  unsigned int cursor_blink_state:1;  /* cursor blink state */
  unsigned int blink_enabled:1;       /* Set to on if we do blinking */
  unsigned int in_expose:1;	      /* updating from within expose events */
  unsigned int scroll_on_keystroke:1;
  unsigned int scroll_on_output:1;
  unsigned int transparent:1;	      /*transparent background*/
  unsigned int shaded:1;	      /*transparent background with shade*/
  unsigned int swap_del_key:1;        /* swap the del and backspace keys */
};

struct _ZvtTermClass
{
  GtkWidgetClass parent_class;

  void (* child_died) (ZvtTerm *term);    
  void (* title_changed) (ZvtTerm *term, VTTITLE_TYPE type, char *newtitle);
};

/* private data structure, stored under "_zvtprivate" */
/* Yes, this data *really*is* private to the widget! */
struct _zvtprivate
{
  gint scrollselect_id;
  int scrollselect_dir;		/* scrolling selection direction/step */  
};

#define ZVT_TERM_DO_UTMP_LOG 1
#define ZVT_TERM_DO_WTMP_LOG 2

GtkWidget*   zvt_term_new                      (void);
GtkWidget*   zvt_term_new_with_size            (int cols, int rows);
void	     zvt_term_reset		       (ZvtTerm *term, int hard);
void         zvt_term_feed                     (ZvtTerm *term,
						char *text, int len);
int	     zvt_term_forkpty		       (ZvtTerm *term, int do_uwtmp_log);
int          zvt_term_closepty                 (ZvtTerm *term);
int          zvt_term_killchild                (ZvtTerm *term, int signal);
void	     zvt_term_bell		       (void *zvt_term);
guint        zvt_term_get_type                 (void);
void         zvt_term_set_scrollback           (ZvtTerm *term, int lines);
char        *zvt_term_get_buffer	       (ZvtTerm *term, int *len,
						int type,
						int sx, int sy,
						int ex, int ey);
void         zvt_term_set_font_name            (ZvtTerm *term, char *name);
void         zvt_term_set_fonts                (ZvtTerm *term,
						GdkFont *font,
						GdkFont *font_bold);
void         zvt_term_hide_pointer             (ZvtTerm *term);
void         zvt_term_show_pointer             (ZvtTerm *term);
void	     zvt_term_set_bell		       (ZvtTerm *term, int state);
gboolean     zvt_term_get_bell		       (ZvtTerm *term);
void         zvt_term_set_blink                (ZvtTerm *term, int state);
void         zvt_term_set_scroll_on_keystroke  (ZvtTerm *term, int state);
void         zvt_term_set_scroll_on_output     (ZvtTerm *term, int state);
void         zvt_term_set_color_scheme         (ZvtTerm *term,
						gushort *red,
						gushort *grn,
						gushort *blu);
void         zvt_term_set_default_color_scheme (ZvtTerm *term);
void         zvt_term_set_del_key_swap         (ZvtTerm *term, int state);
void	     zvt_term_set_wordclass	       (ZvtTerm *term, unsigned char *klass);

/* transparency stuff, it's left in even if we don't compile
 * transparency/backround pixmaps, if we don't, it will just be ignored,
 * setting pixmap_file to NULL disables the background pixmap
 */
void	     zvt_term_set_background		(ZvtTerm       *terminal,
						 char          *pixmap_file,
						 int		transparent,
						 int		shaded);
void         zvt_term_set_shadow_type           (ZvtTerm       *term,
						 GtkShadowType  type);
void         zvt_term_set_size                  (ZvtTerm       *term,
						 guint          width,
						 guint          height);

  
/* returns an bitmask of the capabilities compiled into ZvtTerm */
guint32	     zvt_term_get_capabilities	        (ZvtTerm       *term);




#if 0
GtkAdjustment* zvt_term_get_adjustment         (ZvtTerm      *terminal);
void           zvk_term_set_adjustment         (ZvtTerm      *dial,
                                                GtkAdjustment *adjustment);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __ZVT_TERM_H__ */


