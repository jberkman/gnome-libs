#include <gtk/gtksignal.h>
#include "gtk-xmhtml-parser.h"

enum {
  TEST,
  LAST_SIGNAL
};

static gint xmhtml_parser_signals [LAST_SIGNAL] = { 0 };

static void Destroy (TWidget w);
	
static void
gtk_xmhtml_parser_class_init (GtkXmHTMLParserClass *class)
{
	GtkObjectClass *object_class;
	
	object_class = (GtkObjectClass*) class;

	object_class->destroy = (void (*)(GtkObject *)) Destroy;
	
	xmhtml_parser_signals [TEST] =
		gtk_signal_new ("testsignal",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkXmHTMLParserClass, testsignal),
				gtk_signal_default_marshaller,
				GTK_TYPE_NONE, 0);
	
	gtk_object_class_add_signals (object_class, xmhtml_parser_signals, LAST_SIGNAL);
}

static void
gtk_xmhtml_parser_init (GtkXmHTMLParser *parser)
{
	ATTR(alias_table) = (XmHTMLAliasTable)NULL;
	ATTR(nalias)      = 0;
	ATTR(source)      = (String)NULL;
	ATTR(source_len)  = 0;

	/* list of objects is always initialized to contain a head text element */
	ATTR(objects)     = (XmHTMLObject*)NULL;
	ATTR(head)        = newElement(parser, HT_ZTEXT, NULL, NULL, FALSE, FALSE);
	ATTR(current)     = ATTR(head);
	ATTR(nelements)   = 0;
	ATTR(ntext)       = 1;

	ATTR(loop_count)  = 0;
	ATTR(index)       = 0;
	ATTR(inserted)    = 0;
	ATTR(line_len)    = 0;
	ATTR(cnt)         = 0;
	ATTR(num_lines)   = 0;
	ATTR(err_count)   = 0;

	/* parser state stack is always initialized to HT_DOCTYPE */
	ATTR(base.id)     = HT_DOCTYPE;
	ATTR(base.next)   = (stateStack*)NULL;
	ATTR(stack)       = &(parser->parser.base);
	ATTR(depth)       = 0;

	/* automatic is True when XmNparserCallback is installed */
	ATTR(automatic)   = (parser->parser.parser_callback ? TRUE:FALSE);
	ATTR(unbalanced)  = FALSE;
	ATTR(html32)      = TRUE;
	ATTR(have_body)   = FALSE;
	ATTR(reset)       = TRUE;
	ATTR(active)      = FALSE;
	ATTR(terminated)  = FALSE;

	ATTR(mime_type)   = g_strdup ("text/html");
	ATTR(progressive) = FALSE;
}

guint
gtk_xmthml_parser_get_type ()
{
	static guint data_type = 0;
	
	if (!data_type){
		GtkTypeInfo data_info =
		{
			"GtkXmHTMLParser",
			sizeof (GtkXmHTMLParser),
			sizeof (GtkXmHTMLParserClass),
			(GtkClassInitFunc) gtk_xmhtml_parser_class_init,
			(GtkObjectInitFunc) gtk_xmhtml_parser_init,
			(GtkArgFunc) NULL,
		};
		data_type = gtk_type_unique (gtk_object_get_type (), &data_info);
	}
	return data_type;
}

GtkObject*
gtk_xmhtml_parser_new ()
{
	GtkXmHTMLParser *parser;

	parser = gtk_type_new (gtk_xmhtml_parser_get_type ());
	
	return GTK_OBJECT (parser);
}
