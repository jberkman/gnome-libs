typedef struct _XmHTMLParserClassRec *XmHTMLParserObjectClass;
typedef struct _XmHTMLParserRec *XmHTMLParserObject;

externalref WidgetClass xmHTMLParserObjectClass;

/* XmHTMLParser Object subclassing macro */
#ifndef XmIsHTMLParser
#define XmIsHTMLParser(w) XtIsSubclass(w, xmHTMLParserObjectClass)
#endif /* XmIsHTMLParser */

#define XmHTMLParser(x) (XmHTMLParserObject)x

/* Create a HTML Parser if parent is not null and not a XmHTMLParser itself */
extern Widget XmCreateHTMLParser(Widget parent, String name, ArgList arglist,
    Cardinal argcount);

