/*
  pixmap processing for gnome-terminal
*/

#include <gdk/gdk.h>
#include <gnome.h>

#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-drawable.h>

#include <libart_lgpl/art_rgb_pixbuf_affine.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>

#include "zvtterm.h"

static void zvt_background_set(ZvtTerm *term);
static void zvt_configure_window(GtkWidget *w, ZvtTerm *term);
static void zvt_background_set_translate(ZvtTerm *term);

/**
 * Utility functions
 */

static void
pixmap_free_atom(GdkPixmap *pp)
{
  gdk_xid_table_remove (GDK_WINDOW_XWINDOW(pp));
  g_dataset_destroy (pp);
  g_free (pp);
}

static GdkPixmap *
pixmap_from_atom(GdkWindow *win, GdkAtom pmap)
{
  unsigned char *data;
  GdkAtom type;
  GdkPixmap *pm = 0;

  printf("getting property off root window ...\n");

  if (gdk_property_get(win, pmap, 0, 0, 10, FALSE, &type, NULL, NULL, &data)) {
    if (type == GDK_TARGET_PIXMAP) {
      printf("converting to pixmap\n");
      pm = gdk_pixmap_foreign_new(*((Pixmap *)data));
    }
    g_free(data);
  }

  printf("could not get property\n");

  return pm;
}

static GdkPixbuf *
pixbuf_from_atom(GdkWindow *win, GdkAtom pmap)
{
  GdkPixbuf *pb;
  GdkPixmap *pp;
  int pwidth, pheight;

  pp = pixmap_from_atom(win, pmap);
  if (pp) {
    gdk_window_get_size(pp, &pwidth, &pheight);
    pb = gdk_pixbuf_rgba_from_drawable(pp, 0, 0, pwidth, pheight);
    pixmap_free_atom(pp);
    return pb;
  }
  return NULL;
}

static GdkPixbuf *
pixbuf_scale(GdkPixbuf *pb, int wwidth, int wheight)
{
  GdkPixbuf *pb2;
  double scale[6];
  int width = gdk_pixbuf_get_width(pb);
  int height = gdk_pixbuf_get_height(pb);

  if (wwidth==width && wheight==height)
    return pb;

  pb2 =  gdk_pixbuf_new(ART_PIX_RGB, 0, 8, wwidth, wheight);

  /* clear pixbuf dest */
  memset(gdk_pixbuf_get_pixels(pb2), 0, gdk_pixbuf_get_rowstride(pb2)*wheight);
  
  art_affine_scale(scale, (double)wwidth/(double)width, (double)wheight/(double)height);
  
  /* scale screen data */
  art_rgb_pixbuf_affine(gdk_pixbuf_get_pixels(pb2),
			0, 0, wwidth, wheight,
			gdk_pixbuf_get_rowstride(pb2),
			pb->art_pixbuf, scale, ART_FILTER_NEAREST, NULL);

  /* throw away the old one */
  gdk_pixbuf_unref(pb);
  pb = pb2;
  return pb;
}

static void
pixbuf_shade(GdkPixbuf *pb, int r, int g, int b, int a)
{
  unsigned char *buf = gdk_pixbuf_get_pixels(pb);
  int rowstride = gdk_pixbuf_get_rowstride(pb);
  int pbwidth = gdk_pixbuf_get_width(pb);
  int pbheight = gdk_pixbuf_get_height(pb);
  unsigned char *p;
  int i,j;
  int offset;
  
  printf("applying shading\n");

  if (gdk_pixbuf_get_has_alpha(pb))
    offset=4;
  else
    offset=3;

  printf("offset = %d\n", offset);
  printf("(r,g,b,a) = (%d, %d, %d, %d)\n", r, g, b, a);
  
  for (i=0;i<pbheight;i++) {
    p = buf;
    for (j=0;j<pbwidth;j++) {
      p[0] += ((r-p[0])*a)>>8;
      p[1] += ((g-p[1])*a)>>8;
      p[2] += ((b-p[2])*a)>>8;
      p+=offset;
    }
    buf+=rowstride;
  }
}


struct _watchwin {
  struct _watchwin *next;
  struct _watchwin *prev;
  GdkWindow *win;
  int propmask;
  struct vt_list watchlist;
};

struct _watchatom {
  struct _watchatom *next;
  struct _watchatom *prev;
  GdkAtom atom;
  void (*atom_changed)(GdkAtom atom, int state, void *data);
  void *data;
};

static struct vt_list watchlist = { (struct vt_listnode *)&watchlist.tail,
				    0,
				    (struct vt_listnode *)&watchlist.tailpred };

static int
zvt_filter_prop_change(XEvent *xevent, GdkEvent *event, void *data)
{
  struct _watchwin *w = data;
  struct _watchatom *a;

  printf("got window event ... %d\n", xevent->type);
  if (xevent->type == PropertyNotify) {

    a = (struct _watchatom *)w->watchlist.head;
    while (a->next) {
      if (a->atom == xevent->xproperty.atom) {
	printf("atom %ld has changed\n", a->atom);
	a->atom_changed(a->atom, xevent->xproperty.state, a->data);
      }
      a = a->next;
    }

  }
  return GDK_FILTER_REMOVE;
}

static void
add_winwatch(GdkWindow *win, GdkAtom atom, void *callback, void *data)
{
  struct _watchwin *w;
  struct _watchatom *a;

  w = (struct _watchwin *)watchlist.head;
  while (w->next) {
    if (w->win == win) {
      goto got_win;
    }
    w = w->next;
  }
  w = g_malloc0(sizeof(*w));
  vt_list_new(&w->watchlist);
  gdk_window_ref(win);
  w->win = win;
  w->propmask = gdk_window_get_events(win);
  gdk_window_add_filter(win, zvt_filter_prop_change, w);
  gdk_window_set_events(win, w->propmask | GDK_PROPERTY_CHANGE_MASK);

 got_win:
  a = g_malloc0(sizeof(*a));
  a->atom = atom;
  a->data = data;
  a->atom_changed = callback;
  vt_list_addtail(&w->watchlist, (struct vt_listnode *)a);
}

static void
del_winwatch(GdkWindow *win, void *data)
{
  struct _watchwin *w, *wn;
  struct _watchatom *a, *an;

  w = (struct _watchwin *)watchlist.head;
  wn = w->next;
  while (wn) {
    a = (struct _watchatom *)w->watchlist.head;
    an = a->next;
    while (an) {
      if (a->data == data) {
	g_free(a);
      }
      a = an;
      an = an->next;
    }
    if (vt_list_empty(&w->watchlist)) {
      gdk_window_set_events(w->win, w->propmask);
      gdk_window_remove_filter(w->win, zvt_filter_prop_change, w);
      gdk_window_unref(w->win);
      g_free(w);
    }
    w = wn;
    wn = wn->next;
  }
}

static void
zvt_watch_move(ZvtTerm *term)
{
  gtk_signal_connect (
      GTK_OBJECT (gtk_widget_get_toplevel(GTK_WIDGET(term))),
      "configure_event",
      GTK_SIGNAL_FUNC (zvt_configure_window),
      term);
}

/**
 * Main (xternal) functions
 */
struct zvt_background *
zvt_term_background_new(ZvtTerm *t)
{
  struct zvt_background *b = g_malloc0(sizeof(*b));
  b->refcount=1;
  return b;
}

void
zvt_term_background_unref(struct zvt_background *b)
{
  if (b->refcount==1) {
    zvt_term_background_set_pixmap(b, 0);
    g_free(b);
  } else {
    b->refcount--;
  }
}

void
zvt_term_background_ref(struct zvt_background *b)
{
  b->refcount++;
}

void
zvt_term_background_set_pixmap(struct zvt_background *b, GdkPixmap *p)
{
  switch (b->type) {
  case ZVT_BGTYPE_NONE:		/* no background */
    break;
  case ZVT_BGTYPE_ATOM:		/* pixmap id contained in atom */
    gdk_window_unref(b->data.atom.window);
    break;
  case ZVT_BGTYPE_PIXMAP:	/* normal pixmap */
    if (b->data.pixmap)
      gdk_pixmap_unref(b->data.pixmap);
    break;
  case ZVT_BGTYPE_FILE:		/* file */
    g_free(b->data.pixmap_file);
    break;
  case ZVT_BGTYPE_PIXBUF:		/* pixbuf */
    gdk_pixbuf_unref(b->data.pixbuf);
    break;
  }
  b->data.pixmap = p;
  b->type = ZVT_BGTYPE_PIXMAP;
}

void
zvt_term_background_set_pixmap_atom(struct zvt_background *b, GdkWindow *win, GdkAtom atom)
{
  zvt_term_background_set_pixmap(b, 0);
  b->data.atom.atom = atom;
  gdk_window_ref(win);
  b->data.atom.window = win;
  b->type = ZVT_BGTYPE_ATOM;
}

void
zvt_term_background_set_pixmap_file(struct zvt_background *b, char *filename)
{
  zvt_term_background_set_pixmap(b, 0);
  b->data.pixmap_file = g_strdup(filename);
  b->type = ZVT_BGTYPE_FILE;
}

void
zvt_term_background_set_pixbuf(struct zvt_background *b, GdkPixbuf *pb)
{
  zvt_term_background_set_pixmap(b, 0);
  gdk_pixbuf_ref(pb);
  b->data.pixbuf = pb;
  b->type = ZVT_BGTYPE_PIXBUF;
}

void
zvt_term_background_set_shade(struct zvt_background *bg, int r, int g, int b, int a)
{
  bg->shade.r = r>>8;
  bg->shade.g = g>>8;
  bg->shade.b = b>>8;
  bg->shade.a = a>>8;
}

void
zvt_term_background_set_scale(struct zvt_background *b, zvt_background_scale_t type, int x, int y)
{
  b->scale.x = x;
  b->scale.y = y;
  b->scale.type = type;
}

void
zvt_term_background_set_translate(struct zvt_background *b, zvt_background_translate_t type, int x, int y)
{
  b->offset.x = x;
  b->offset.y = y;
  b->offset.type = type;
}

/* called when the root pixmap atom changes */
static void
zvt_root_atom_changed(GdkAtom atom, int state, ZvtTerm *term)
{
  if (state == GDK_PROPERTY_NEW_VALUE) {
    zvt_background_set(term);
    gtk_widget_queue_draw(GTK_WIDGET(term));
  }
  /* FIXME: If GDK_PROPERTY_DELETE, must remove root pixmap option
     from terminal */
}

void
zvt_term_background_unload(ZvtTerm *term)
{
  struct _zvtprivate *zp = _ZVT_PRIVATE(term);
  struct zvt_background *b = zp->background;

  if (b) {
    switch(b->type) {
    case ZVT_BGTYPE_ATOM:		/* pixmap id contained in atom */
      del_winwatch(b->data.atom.window, term);
      break;
    case ZVT_BGTYPE_NONE:
    case ZVT_BGTYPE_PIXMAP:
    case ZVT_BGTYPE_FILE:
    case ZVT_BGTYPE_PIXBUF:
      break;
    }
    zvt_term_background_unref(b);
    zp->background = 0;
  }

  /* free pixmap also ... must take into account root pixmaps */
  /* *FIXME FIXME* */
  zp->background_pixmap = 0;
  gtk_widget_queue_draw(GTK_WIDGET(term));
}

int
zvt_term_background_load(ZvtTerm *term, struct zvt_background *b)
{
  struct _zvtprivate *zp = _ZVT_PRIVATE(term);
  int watchatom=0;
  int watchmove=0;

  if (zp->background)
    zvt_term_background_unref(zp->background);
  zp->background = b;
  if (b) {
    if (b->type == ZVT_BGTYPE_ATOM)
      watchatom = 1;
    if (b->scale.type == ZVT_BGSCALE_WINDOW
	|| b->offset.type == ZVT_BGTRANSLATE_ROOT)
      watchmove = 1;
    if (watchatom) {
      add_winwatch(b->data.atom.window,
		   b->data.atom.atom,
		   zvt_root_atom_changed,
		   term);
    }
    if (watchmove) {
      zvt_watch_move(term);
    }
  }
  zvt_background_set(term);
  gtk_widget_queue_draw(GTK_WIDGET(term));
  return 0;
}

static void
zvt_background_set(ZvtTerm *term)
{
  struct _zvtprivate *zp = _ZVT_PRIVATE(term);
  struct zvt_background *b = zp->background;
  GdkPixmap *pixmap = NULL;
  GdkPixbuf *pixbuf = NULL;
  int wwidth, wheight, wdepth;
  int process = 0;

  /* if we have no 'background image', use a solid colour */
  if (b == NULL) {
    GdkColor pen;
    gdk_gc_set_fill (term->back_gc, GDK_SOLID);
    pen.pixel = term->colors[17];
    gdk_gc_set_foreground (term->back_gc, &pen);
    return;
  }

  process = (b->shade.a != 0
	     || b->scale.type != ZVT_BGSCALE_NONE);
      
  switch (b->type) {
  case ZVT_BGTYPE_NONE:		/* no background */
    break;
  case ZVT_BGTYPE_ATOM:		/* pixmap id contained in atom */
    if (process)
      pixbuf = pixbuf_from_atom(b->data.atom.window, b->data.atom.atom);
    else
      pixmap = pixmap_from_atom(b->data.atom.window, b->data.atom.atom);
    break;
  case ZVT_BGTYPE_PIXMAP:	/* normal pixmap */
    pixmap = b->data.pixmap;
    break;
  case ZVT_BGTYPE_FILE:		/* file */
    pixbuf = gdk_pixbuf_new_from_file(b->data.pixmap_file);
    break;
  case ZVT_BGTYPE_PIXBUF:		/* pixbuf */
    pixbuf = b->data.pixbuf;
    break;
  }

  gdk_window_get_geometry(GTK_WIDGET(term)->window,NULL,NULL,&wwidth,&wheight,&wdepth);

  if (process) {
    int width, height;
    if (pixbuf==NULL) {
      int pwidth, pheight;
      gdk_window_get_size(pixmap, &pwidth, &pheight);
      pixbuf = gdk_pixbuf_rgb_from_drawable(pixmap, 0, 0, pwidth, pheight);
      /* free the pixmap? */
    }

    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);

    if (b->shade.a != 0) {
      pixbuf_shade(pixbuf, b->shade.r, b->shade.g, b->shade.b, b->shade.a);
    }

    switch (b->scale.type) {
    case ZVT_BGSCALE_NONE:		/* no scaling */
      break;
    case ZVT_BGSCALE_WINDOW:		/* scale to window */
      width = wwidth;
      height = wheight;
      break;
    case ZVT_BGSCALE_FIXED:		/* scale fixed amount */
      width = (width * b->scale.x) >> 16;
      height = (height * b->scale.y) >> 16;
      break;
    case ZVT_BGSCALE_ABSOLUTE:		/* scale absolute coords */
      width = b->scale.x;
      height = b->scale.y;
      break;
    }
    if (b->scale.type != ZVT_BGSCALE_NONE)
      pixbuf = pixbuf_scale(pixbuf, width, height);
  }

  /* if we have a pixbuf, then we need to convert it to a pixmap to actually
     use it ... */
  if (pixbuf!=NULL) {
    pixmap = gdk_pixmap_new(GTK_WIDGET(term)->window,
			    gdk_pixbuf_get_width(pixbuf),
			    gdk_pixbuf_get_height(pixbuf), wdepth);
    
    /* render to pixmap */
    gdk_pixbuf_render_to_drawable(pixbuf, pixmap, GTK_WIDGET(term)->style->white_gc,
				  0, 0, 0, 0,
				  gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf),
				  GDK_RGB_DITHER_MAX,
				  0, 0);
    /* free working area */
    gdk_pixbuf_unref(pixbuf);
  }

  zp->background_pixmap = pixmap;

  gdk_gc_set_tile (term->back_gc, pixmap);
  gdk_gc_set_fill (term->back_gc, GDK_TILED);

  zvt_background_set_translate(term);
}

static void
zvt_background_set_translate(ZvtTerm *term)
{
  int offx, offy, x, y;
  Window childret;
  struct _zvtprivate *zp = _ZVT_PRIVATE(term);
  struct zvt_background *b = zp->background;

  offx = b->offset.x;
  offy = b->offset.y;

  switch(b->offset.type) {
  case ZVT_BGTRANSLATE_NONE:
  case ZVT_BGTRANSLATE_SCROLL:
    break;
  case ZVT_BGTRANSLATE_ROOT:
    XTranslateCoordinates (GDK_WINDOW_XDISPLAY (GTK_WIDGET(term)->window),
			   GDK_WINDOW_XWINDOW (GTK_WIDGET(term)->window),
			   GDK_ROOT_WINDOW (),
			   0, 0,
			   &x, &y,
			   &childret);
    offx -= x;
    offy -= y;
    break;
  }
  gdk_gc_set_ts_origin(term->back_gc, offx, offy);
}

/*
 * If we configure window, work out if we have to reload the background
 * image, etc.
 */
static void
zvt_configure_window(GtkWidget *w, ZvtTerm *term)
{
  struct _zvtprivate *zp = _ZVT_PRIVATE(term);
  Window childret;
  int width, height, x, y;
  struct zvt_background *b = zp->background;
  int forcedraw = 0;

  XTranslateCoordinates (GDK_WINDOW_XDISPLAY (GTK_WIDGET(term)->window),
			 GDK_WINDOW_XWINDOW (GTK_WIDGET(term)->window),
			 GDK_ROOT_WINDOW (),
			 0, 0,
			 &x, &y,
			 &childret);
  gdk_window_get_size(GTK_WIDGET(term)->window, &width, &height);

  /* see if we need to reload (scale) the image */
  if (b->scale.type == ZVT_BGSCALE_WINDOW
      && (b->pos.h != height || b->pos.w != width)) {
    zvt_background_set(term);
    forcedraw = 1;
  }

  /* if we are relative absolute coords, and we have moved, we must catch up */
  if (b->offset.type == ZVT_BGTRANSLATE_ROOT
      && (b->pos.x != x || b->pos.y != y)) {
    zvt_background_set_translate(term);
    forcedraw = 1;
  }

  /* update last rendered position */
  b->pos.x = x;
  b->pos.y = y;
  b->pos.w = width;
  b->pos.h = height;

  if (forcedraw) {
    gtk_widget_queue_draw(GTK_WIDGET(term));
  }
}

