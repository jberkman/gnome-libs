/***
* Class methods 
***/
/* class initialize method */
static void Initialize(Widget request, Widget init, ArgList args, Cardinal *num_args);

/* class set_values method */
static Boolean SetValues(Widget current, Widget request, Widget set,
	ArgList args, Cardinal *num_args);

/* class get_values_hook method */
static void GetValues(Widget w, ArgList args, Cardinal *num_args);

/* class destroy method */
static void Destroy(Widget w);

/* resources */
static XtResource resources [] = 
{
	{
		XmNmimeType, 
		XmCString, XtRString,  sizeof(String),
		XtOffset(XmHTMLParserObject, parser.mime_type),
		XtRString, "text/html"
	},
	{
		XmNstrictHTMLChecking, 
		XmCBoolean, XtRBoolean,  sizeof(Boolean),
		XtOffset(XmHTMLParserObject, parser.strict),
		XtRString, "False"
	},
	{
		XmNparserIsProgressive, 
		XmCBoolean, XtRBoolean,  sizeof(Boolean),
		XtOffset(XmHTMLParserObject, parser.progressive),
		XtRString, "False"
	},
	{
		XmNretainSource, 
		XmCBoolean, XtRBoolean,  sizeof(Boolean),
		XtOffset(XmHTMLParserObject, parser.retain_source),
		XtRString, "False"
	},
	{
		XmNuserData, 
		XmCUserData, XtRPointer,  sizeof(XtPointer),
		XtOffset(XmHTMLParserObject, parser.user_data),
		XtRImmediate, (XtPointer)NULL
	},
	{
		XmNparserCallback,
		XtCCallback, XtRCallback, sizeof(XtCallbackList),
		XtOffset(XmHTMLParserObject, parser.parser_callback),
		XtRImmediate, (XtPointer)NULL
	},
	{
		XmNmodifyVerifyCallback,
		XtCCallback, XtRCallback, sizeof(XtCallbackList),
		XtOffset(XmHTMLParserObject, parser.modify_verify_callback),
		XtRImmediate, (XtPointer)NULL
	},
	{
		XmNdocumentCallback,
		XtCCallback, XtRCallback, sizeof(XtCallbackList),
		XtOffset(XmHTMLParserObject, parser.document_callback),
		XtRImmediate, (XtPointer)NULL
	}
};

/*****
* Define the object class record
*****/
XmHTMLParserClassRec xmHTMLParserClassRec = {
										/* object class fields */
{
	(WidgetClass)&objectClassRec,		/* superclass */
	"XmHTMLParser",						/* class name */
	sizeof(XmHTMLParserRec),			/* widget_size */
	NULL,								/* class_initialize */
	NULL,								/* class_part_initialize */
	FALSE,								/* class_inited */
	(XtInitProc)Initialize,				/* initialize */
	NULL,								/* initialize_hook */
	NULL,								/* obj1 */
	NULL,								/* obj2 */
	0,									/* obj3 */
	resources,							/* resources */
	XtNumber(resources),				/* num_resources */
	NULLQUARK,							/* xrm_class */
	FALSE,								/* obj4 */
	0,									/* obj5 */
	FALSE,								/* obj6 */
	FALSE,								/* obj7 */
	Destroy,							/* destroy */
	NULL,								/* obj8 */
	NULL,								/* obj9 */
	(XtSetValuesFunc)SetValues,			/* set_values */
	NULL,								/* set_values_hook */
	NULL,								/* obj10 */
	GetValues,							/* get_values_hook */
	NULL,								/* obj11 */
	XtVersion,							/* version */
	NULL,								/* callback_private */
	NULL,								/* obj12 */
	NULL,								/* obj13 */
	NULL,								/* obj14 */
	NULL,								/* extension */
},
	/* html_parser_class fields */
{
	0											/* none */
}
};

/* Establish the object class name as an externally accessible symbol */
WidgetClass xmHTMLParserObjectClass = (WidgetClass)&xmHTMLParserClassRec;

/*****************************************************************************
* Chapter 1
* Object methods
*****************************************************************************/

/*****
* Name:			Initialize
* Return Type:	void
* Description:	called when the parser is instantiated
* In:
*	request:	widget with resource values set as requested by the argument
*				list, resource database and widget defaults
*	init:		same widget with values as modified by superclass initialize()
*				methods
*	args:		argument list passed to XtCreateWidget
*	num_args:	number of entries in the argument list
* Returns:
*	nothing, but init is updated with checked/updated resource values.
*****/
static void
Initialize(Widget request, Widget init, ArgList args, Cardinal *num_args)
{
	XmHTMLParserObject parser = (XmHTMLParserObject)init;
	XmHTMLParserObject req    = (XmHTMLParserObject)request;

	ATTR(alias_table) = (XmHTMLAliasTable)NULL;
	ATTR(nalias)      = 0;
	ATTR(source)      = (String)NULL;
	ATTR(source_len)  = 0;

	/* list of objects is always initialized to contain a head text element */
	ATTR(objects)     = (XmHTMLObject*)NULL;
	ATTR(head)        = newElement(parser, HT_ZTEXT, NULL, NULL, False, False);
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
	ATTR(automatic)   = (parser->parser.parser_callback ? True:False);
	ATTR(unbalanced)  = False;
	ATTR(html32)      = True;
	ATTR(have_body)   = False;
	ATTR(reset)       = True;
	ATTR(active)      = False;
	ATTR(terminated)  = False;

	/****
	* Following elements are set by default resources or by caller
	*
	* ATTR(mime_type), default is text/html
	* ATTR(strict_checking), default is False
	* ATTR(handle_shorttags), default is True
	* ATTR(progressive), default is False
	*****/
}

/*****
* Name:			SetValues
* Return Type:	Boolean
* Description:	XmHTMLParserObjectClass SetValues method.
* In:
*	current:	copy of widget before any set_values methods are called
*	request:	copy of widget after resources have changed but before any
*				set_values methods are called
*	set:		widget with resources set and as modified by any superclass
*				methods that have called XtSetValues()
*	args:		argument list passed to XtSetValues, done by programmer
*	num_args:	no of args
* Returns:
*	True if a changed resource requires a redisplay, False otherwise.
*****/
static Boolean
SetValues(Widget current, Widget request, Widget set, ArgList args,
	Cardinal *num_args)
{
	/* nothing to do */
	return(False);
}

/*****
* Name: 		GetValues
* Return Type: 	void
* Description: 	XmHTMLParserObjectClass get_values_hook method
* In: 
*
* Returns:
*	nothing
*****/
static void
GetValues(Widget w, ArgList args, Cardinal *num_args)
{
	/* nothing to do */
}

/*****
* Name: 		XmCreateHTMLParser
* Return Type: 	Widget
* Description: 	creates a XmHTMLParserObjectClass widget
* In: 
*	parent:		widget to act as parent for this new XmHTMLParserObject
*	name:		name for the new widget
*	arglist:	arguments for this new XmHTMLParserObject
*	argcount:	no of arguments
* Returns:
*	a newly created widget. This routine exits if parent is NULL or a subclass
*	of XmHTMLParserObject.
*****/
Widget
XmCreateHTMLParser(Widget parent, String name, ArgList arglist, Cardinal argcount)
{
	if(parent && !XmIsHTMLParser(parent))
		return(XtCreateWidget(name, xmHTMLParserObjectClass, parent, 
			arglist, argcount));

	if(!parent)
		_XmHTMLWarning(__WFUNC__(NULL, "XmCreateHTMLParser"),
			"NULL parent passed to XmCreateHTMLParser.");
	else 
		_XmHTMLError(__WFUNC__(NULL, "XmCreateHTMLParser"),
			"XmCreateHTMLParser: htmlParserObject can not be subclassed.");
	return(NULL);
}
