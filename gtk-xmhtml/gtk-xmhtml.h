#ifndef __GTK_XMHTML_H__
#define __GTK_XMHTML_H__

#include <gdk/gdk.h>
#include <gtk/gtkobject.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_XMHTML(obj)          GTK_CHECK_CAST (obj, gtk_data_get_type (), GtkXmHTML)
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
};

struct _GtkXmHTMLClass
{
	GtkContainer parent_class;

	void (* testsignal) (GtkXmHTML *xmhtml);
};

#endif
