#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "XmHTMLP.h"
#include "XmHTMLfuncs.h"

void _XmHTMLFormReset(XmHTMLWidget html, XmHTMLForm *entry)
{
	fprintf (stderr, "Fatal: function %s called\n", __FUNCTION__);
}

void
_XmHTMLFormActivate(XmHTMLWidget html, TEvent *event, XmHTMLForm *entry)
{
	fprintf (stderr, "Fatal: function %s called\n", __FUNCTION__);
}

void 
_XmHTMLFreeForm(XmHTMLWidget html, XmHTMLFormData *form)
{
}

XmHTMLForm*
_XmHTMLFormAddTextArea(XmHTMLWidget html, String attributes, String text)
{
	return NULL;
}

void
_XmHTMLFormSelectClose(XmHTMLWidget html, XmHTMLForm *entry)
{
}

void
_XmHTMLFormSelectAddOption(XmHTMLWidget html, XmHTMLForm *entry,
	String attributes, String label)
{
}

XmHTMLForm*
_XmHTMLFormAddSelect(XmHTMLWidget html, String attributes)
{
	return 0;
}

XmHTMLForm*
_XmHTMLFormAddInput(XmHTMLWidget html, String attributes)
{
}

void
_XmHTMLEndForm(XmHTMLWidget html)
{
}

void
_XmHTMLStartForm(XmHTMLWidget html, String attributes)
{
}
