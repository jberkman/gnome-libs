#ifndef __GTK_XMHTML_H__
#define __GTK_XMHTML_H__

#include <gdk/gdk.h>
#include <gtk/gtkobject.h>
#include "XmHTMLP.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_XMHTML(obj)          GTK_CHECK_CAST (obj, gtk_xmhtml_get_type (), GtkXmHTML)
#define GTK_XMHTML_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_xmhtml_get_type (), GtkXmHTMLClass)
#define GTK_IS_XMHTML(obj)       GTK_CHECK_TYPE (obj, gtk_xmhtml_get_type ())

/* For compatibility and consistency functions */
#define XmIsHTML(obj)             GTK_IS_XMHTML(obj)
#define XmHTML(obj)               GTK_XMHTML(obj)

typedef struct _GtkXmHTML       GtkXmHTML;
typedef struct _GtkXmHTMLClass  GtkXmHTMLClass;

struct _GtkXmHTML
{
	GtkContainer widget;

	XmHTMLPart html;

	/* Scrollbar adjustements */
	GtkObject *vsba;
	GtkObject *hsba;
};

struct _GtkXmHTMLClass
{
	GtkContainerClass parent_class;

	void (*activate)     (GtkXmHTML *, void *);
	void (*arm)          (GtkXmHTML *, void *);
	void (*anchor_track) (GtkXmHTML *, void *);
	void (*frame)        (GtkXmHTML *, void *);
	void (*form)         (GtkXmHTML *, void *);
	void (*input)        (GtkXmHTML *, void *);
	void (*link)         (GtkXmHTML *, void *);
	void (*motion)       (GtkXmHTML *, void *);
	void (*imagemap)     (GtkXmHTML *, void *);
	void (*document)     (GtkXmHTML *, void *);
	void (*focus)        (GtkXmHTML *, void *);
	void (*losing_focus) (GtkXmHTML *, void *);
	void (*motion_track) (GtkXmHTML *, void *);
};

GtkWidget *gtk_xmhtml_new (char *html_source);

/* For compatibility with the Motif sources */
typedef struct {
	int reason;
	GdkEvent *event;
} gtk_xmhtml_callback_info;

#endif
