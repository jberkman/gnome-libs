#ifndef __GTK_XMHTML_PARSER_H__
#define __GTK_XMHTML_PARSER_H__

#include <gdk/gdk.h>
#include <gtk/gtkobject.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GTK_XMHTML_PARSER(obj)          GTK_CHECK_CAST (obj, gtk_data_get_type (), GtkXmHTMLParser)
#define GTK_XMHTML_PARSER_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_xmhtml_parser_get_type (), \
						GtkXmHTMLParserClass)
#define GTK_IS_XMHTML_PARSER(obj)       GTK_CHECK_TYPE (obj, gtk_xmhtml_parser_get_type ())

/* For compatibility and consistency functions */
#define XmIsHTMLParser(obj)             GTK_IS_XMHTML_PARSER(obj)
#define XmHTMLParser(obj)               GTK_XMHTML_PARSER(obj)

typedef struct _GtkXmHTMLParser       GtkXmHTMLParser;
typedef struct _GtkXmHTMLParserClass  GtkXmHTMLParserClass;

struct _GtkXmHTMLParser
{
	GtkObject object;

	XmHTMLParserPart parser;
};

struct _GtkXmHTMLParserClass
{
	GtkObjectClass parent_class;

	void (* testsignal) (GtkData *data);
};

typedef GtkXmHTMLParser *XmHTMLParserObject;
#endif
