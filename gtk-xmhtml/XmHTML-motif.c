/* Static resources */
#include "resources.h"

/* manage scrollbars if necessary */
#define SetScrollBars(HTML) { \
	if((HTML)->html.needs_hsb && !XtIsManaged((HTML)->html.hsb)) \
		XtManageChild(html->html.hsb); \
	if((HTML)->html.needs_vsb && !XtIsManaged((HTML)->html.vsb)) \
		XtManageChild((html)->html.vsb); \
}

/* check slider value and adjust if necessary */
#define AdjustVerticalScrollValue(VSB, VAL) { \
	int max = 0, size = 0; \
	XtVaGetValues(VSB, \
		XmNmaximum, &max, \
		XmNsliderSize, &size, \
		NULL); \
	if(VAL > (max - size)) \
		VAL = (max - size); \
}

/***
* Class methods
***/
/* Primary ClassInitialize method */
static void ClassInitialize(void);

/* class initialize method */
static void Initialize(Widget request, Widget init, ArgList args,
	Cardinal *num_args);

/* class resize method */
static void Resize(Widget w);

/* class expose method */
static void Redisplay(Widget w, XEvent *event, Region region);

/* Expose event handler for the work area */
static void DrawRedisplay(Widget w, XmHTMLWidget html, XEvent *event);

/* VisibilityNotify event handler for the work area */
static void VisibilityHandler(Widget w, XmHTMLWidget html, XEvent *event);

/* MapNotify action routine for the work area */
static void Mapped(Widget w, XmHTMLWidget html, XEvent *event); 

/* class set_values method */
static Boolean SetValues(Widget current, Widget request, Widget set,
	ArgList args, Cardinal *num_args);

/* class get_values_hook method */
static void GetValues(Widget w, ArgList args, Cardinal *num_args);

/* class geometry_manager method */
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request,
	XtWidgetGeometry *geometry_return);

/* class destroy method */
static void Destroy(Widget w);

/* Action routines */
static void	ExtendStart(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void	ExtendAdjust(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void	ExtendEnd(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void TrackMotion(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void HTMLProcessInput(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void HTMLPageUpOrLeft(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void HTMLPageDownOrRight(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void HTMLIncrementUpOrLeft(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void HTMLIncrementDownOrRight(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void HTMLTopOrBottom(Widget w, XEvent *event, String *params, 
		Cardinal *num_params);
static void HTMLTraverseCurrent(Widget w, XEvent *event, String *params,
		Cardinal *num_params);
static void HTMLTraverseNext(Widget w, XEvent *event, String *params,
		Cardinal *num_params);
static void HTMLTraversePrev(Widget w, XEvent *event, String *params,
		Cardinal *num_params);
static void HTMLTraverseNextOrPrev(Widget w, XEvent *event, String *params,
		Cardinal *num_params);

/*** Private Variable Declarations ***/
static XmRepTypeId underline_repid, sb_policy_repid;
static XmRepTypeId sb_placement_repid, string_repid;
static XmRepTypeId enable_repid, conv_repid;

/*****
* default translations
* Order of key translations is important: placing c <Key>osfPageUp after
* <key>osfPageUp will mask of the Ctrl key.
* XmHTML explicitly masks off all key modifiers it does not need for a
* certain action. This allows application programmers to use the same keys
* with modifiers for their own purposes and prevents that these key sequences
* are handled by these specific XmHTML action routines.
* This looks ugly, since it's a static block of text it doesn't take up that
* much data space.
*****/
static char translations[] = 
"Ctrl <Key>osfPageUp: page-up-or-left(1)\n\
Ctrl <Key>osfPageDown: page-down-or-right(1)\n\
Ctrl <Key>osfBeginLine: top-or-bottom(0)\n\
Ctrl <Key>osfEndLine: top-or-bottom(1)\n\
~Shift ~Meta ~Alt <Btn1Down>: extend-start() ManagerGadgetArm()\n\
~Shift ~Meta ~Alt <Btn1Motion>: extend-adjust() ManagerGadgetButtonMotion()\n\
~Shift ~Meta ~Alt <Btn1Up>: extend-end(PRIMARY, CUT_BUFFER0) ManagerGadgetActivate() traverse-current()\n\
~Shift ~Meta ~Alt <Btn2Down>:extend-start()\n\
~Shift ~Meta ~Alt <Btn2Motion>: extend-adjust()\n\
~Shift ~Meta ~Alt <Btn2Up>: extend-end(PRIMARY, CUT_BUFFER0)\n\
~Shift ~Meta ~Alt <Key>osfPageUp: page-up-or-left(0)\n\
~Shift ~Meta ~Alt <Key>osfPageDown: page-down-or-right(0)\n\
~Shift ~Meta ~Alt <Key>osfUp: increment-up-or-left(0)\n\
~Shift ~Meta ~Alt <Key>osfLeft: increment-up-or-left(1)\n\
~Shift ~Meta ~Alt <Key>osfDown: increment-down-or-right(0)\n\
~Shift ~Meta ~Alt <Key>osfRight: increment-down-or-right(1)\n\
<Key>osfHelp: ManagerGadgetHelp()\n\
Shift Ctrl <Key>Tab: ManagerGadgetPrevTabGroup()\n\
Ctrl <Key>Tab: ManagerGadgetNextTabGroup()\n\
<Key>Tab: traverse-next()\n\
<Motion>: track-motion()\n\
<Leave>: track-motion()\n\
<FocusIn>: track-motion()\n\
<FocusOut>: track-motion()\n\
<Expose>: track-motion()\n\
<KeyDown>: process-html-input()\n\
<KeyUp>: process-html-input()";

/* Action routines provided by XmHTML */
static XtActionsRec actions[] = 
{
	{"extend-start",            (XtActionProc)ExtendStart},
	{"extend-adjust",           (XtActionProc)ExtendAdjust},
	{"extend-end",              (XtActionProc)ExtendEnd},
	{"page-up-or-left",         (XtActionProc)HTMLPageUpOrLeft},
	{"page-down-or-right",      (XtActionProc)HTMLPageDownOrRight},
	{"increment-up-or-left",    (XtActionProc)HTMLIncrementUpOrLeft},
	{"increment-down-or-right", (XtActionProc)HTMLIncrementDownOrRight},
	{"top-or-bottom",           (XtActionProc)HTMLTopOrBottom},
	{"track-motion",            (XtActionProc)TrackMotion},
	{"process-html-input",      (XtActionProc)HTMLProcessInput},
	{"traverse-current",        (XtActionProc)HTMLTraverseCurrent},
	{"traverse-next",           (XtActionProc)HTMLTraverseNext},
	{"traverse-prev",           (XtActionProc)HTMLTraversePrev},
	{"traverse-next-or-prev",   (XtActionProc)HTMLTraverseNextOrPrev}
};

/* 
* copy of original list. Motif destroys the original list and therefore
* XmHTML crashes when we try to use the above list again.
*/
static XtActionsRec spareActions[] = 
{
	{"extend-start",            (XtActionProc)ExtendStart},
	{"extend-adjust",           (XtActionProc)ExtendAdjust},
	{"extend-end",              (XtActionProc)ExtendEnd},
	{"page-up-or-left",         (XtActionProc)HTMLPageUpOrLeft},
	{"page-down-or-right",      (XtActionProc)HTMLPageDownOrRight},
	{"increment-up-or-left",    (XtActionProc)HTMLIncrementUpOrLeft},
	{"increment-down-or-right", (XtActionProc)HTMLIncrementDownOrRight},
	{"top-or-bottom",           (XtActionProc)HTMLTopOrBottom},
	{"track-motion",            (XtActionProc)TrackMotion},
	{"process-html-input",      (XtActionProc)HTMLProcessInput},
	{"traverse-current",        (XtActionProc)HTMLTraverseCurrent},
	{"traverse-next",           (XtActionProc)HTMLTraverseNext},
	{"traverse-prev",           (XtActionProc)HTMLTraversePrev},
	{"traverse-next-or-prev",   (XtActionProc)HTMLTraverseNextOrPrev}
};

/****
* Define the CompositeClassExtension record so we can accept objects.
****/
static CompositeClassExtensionRec htmlCompositeExtension = {
	NULL,									/* next_extension */
	NULLQUARK,								/* record_type */
	XtCompositeExtensionVersion,			/* version */
	sizeof(CompositeClassExtensionRec),		/* record_size */
	True									/* accept_objects */
#if XtSpecificationRelease >= 6
	, False									/* allows_change_managed_set */
#endif
};

/****
* Define the widget class record.
****/
XmHTMLClassRec xmHTMLClassRec = {
											/* core class fields	*/
{
	(WidgetClass) &xmManagerClassRec,		/* superclass			*/
	"XmHTML",								/* class_name			*/
	sizeof(XmHTMLRec),						/* widget_size			*/
	ClassInitialize,						/* class_initialize	 	*/
	NULL,									/* class_part_init		*/
	FALSE,									/* class_inited		 	*/
	(XtInitProc)Initialize,					/* initialize		 	*/
	NULL,									/* initialize_hook		*/
	XtInheritRealize,						/* realize				*/
	actions,								/* actions				*/
	XtNumber(actions),						/* num_actions			*/
	resources,								/* resources			*/
	XtNumber(resources),					/* num_resources		*/
	NULLQUARK,								/* xrm_class			*/
	TRUE,									/* compress_motion		*/
	XtExposeCompressMaximal,				/* compress_exposure	*/
	TRUE,									/* compress_enterleave 	*/
	FALSE,									/* visible_interest	 	*/
	Destroy,								/* destroy				*/
	(XtWidgetProc)Resize,					/* resize			 	*/
	(XtExposeProc)Redisplay,				/* expose			 	*/
	(XtSetValuesFunc)SetValues,				/* set_values		 	*/
	NULL,									/* set_values_hook		*/
	XtInheritSetValuesAlmost,				/* set_values_almost	*/
	GetValues,								/* get_values_hook		*/
	XtInheritAcceptFocus,					/* accept_focus		 	*/
	XtVersion,								/* version				*/
	NULL,									/* callback_private	 	*/
	translations,							/* tm_table			 	*/
	XtInheritQueryGeometry,					/* query_geometry	 	*/
	XtInheritDisplayAccelerator,			/* display_accelerator	*/
	NULL									/* extension			*/
},
											/* composite_class fields */
{
	GeometryManager, 						/* geometry_manager	 	*/
	NULL,									/* change_managed	 	*/
	XtInheritInsertChild,					/* insert_child		 	*/
	XtInheritDeleteChild,					/* delete_child			*/
	(XtPointer)&htmlCompositeExtension		/* extension			*/
},
											/* constraint_class fields */
{
	NULL,									/* resource list		*/	 
	0,										/* num resources		*/	 
	0,										/* constraint size		*/	 
	NULL,									/* init proc			*/	 
	NULL,									/* destroy proc			*/	 
	NULL,									/* set values proc		*/	 
	NULL									/* extension			*/
},
											/* manager_class fields */
{
	XtInheritTranslations,					/* translations			*/
	NULL,									/* syn_resources		*/
	0,										/* num_syn_resources 	*/
	NULL,									/* syn_cont_resources	*/
	0,										/* num_syn_cont_resources*/
	XmInheritParentProcess,					/* parent_process		*/
	NULL									/* extension 			*/	
},
											/* html_class fields */	 
{	
	0										/* none					*/
}	
};


/* Establish the widget class name as an externally accessible symbol. */
WidgetClass xmHTMLWidgetClass = (WidgetClass) &xmHTMLClassRec;

static void
TestRepId(XmRepTypeId id, String name)
{
	if(id == XmREP_TYPE_INVALID)
 		_XmHTMLWarning(__WFUNC__(NULL, "TestRepId"), "Representation "
			"type resource convertor %s not found/installed.\n"
			"    Please contact ripley@xs4all.nl", name);
}

/*****
* Name:			ClassInitialize
* Return Type:	void
* Description:	Called by Intrinsics the first time a widget of this class
*				is instantiated
* In:
*	nothing
* Returns:
*	nothing
*****/
static void
ClassInitialize(void)
{
	static char *enable_models[] = {"automatic", "always", "never"};
	static char *conv_models[] = {"quick", "best", "fast", "slow", "disabled"};
	static char *line_styles[] = {"no_line", "single_line", "double_line",
								"single_dashed_line", "double_dashed_line"};

	/* Get appropriate representation type convertor id's */

	/* ScrollBar converters. */
	sb_policy_repid = XmRepTypeGetId(XmCScrollBarDisplayPolicy);
	TestRepId(sb_policy_repid, XmCScrollBarDisplayPolicy);

	sb_placement_repid = XmRepTypeGetId(XmCScrollBarPlacement);
	TestRepId(sb_placement_repid, XmCScrollBarPlacement);

	/* string direction converter */
	string_repid = XmRepTypeGetId(XmCAlignment);
	TestRepId(string_repid, XmCAlignment);

	/* XmCEnableMode resource class converter */
	XmRepTypeRegister(XmCEnableMode, enable_models, NULL, 3);
	enable_repid = XmRepTypeGetId(XmCEnableMode);
	TestRepId(enable_repid, XmCEnableMode);

	/* XmCConversionMode resource class converter */
	XmRepTypeRegister(XmCConversionMode, conv_models, NULL, 5);
	conv_repid = XmRepTypeGetId(XmCConversionMode);
	TestRepId(conv_repid, XmCConversionMode);

	/* XmCAnchorUnderlineType resource class converter */
	XmRepTypeRegister(XmCAnchorUnderlineType, line_styles, NULL, 5);
	underline_repid = XmRepTypeGetId(XmCAnchorUnderlineType);
	TestRepId(underline_repid, XmCAnchorUnderlineType);
}

static void
Initialize(TWidget request, TWidget init, ArgList args, Cardinal *num_args)
{
	XmHTMLWidget html = XmHTML (init);
	XmHTMLWidget req  = XmHTML (request);

	/* select debug levels */
	_XmHTMLSelectDebugLevels (req->html.debug_levels);
	_XmHTMLSetFullDebug(req->html.debug_full_output);

#ifdef DEBUG
	if(req->html.debug_disable_warnings)
		debug_disable_warnings = True;
	else
		debug_disable_warnings = False;
#endif

	/* Initialize the global HTMLpart */
	_XmHTMLDebug(1, ("XmHTML.c: Initialize Start\n"));

	/* private TWidget resources */
	html->html.needs_vsb    = False;
	html->html.needs_hsb    = False;
	html->html.scroll_x     = 0;
	html->html.scroll_y     = 0;

	CheckAnchorUnderlining(html, html);

	/* ScrollBarDisplayPolicy */
	if(!XmRepTypeValidValue(sb_policy_repid, html->html.sb_policy, 
		(TWidget)html))
		html->html.sb_policy = XmAS_NEEDED;
	else if(html->html.sb_policy == XmSTATIC)
		html->html.needs_vsb = True;

	/* ScrollBarPlacement */
	if(!XmRepTypeValidValue(sb_placement_repid, html->html.sb_placement, 
		(TWidget)html))
		html->html.sb_placement = XmBOTTOM_RIGHT;

	/* perfectColors */
	if(!XmRepTypeValidValue(enable_repid, html->html.perfect_colors,
		(TWidget)html))
		html->html.perfect_colors = XmAUTOMATIC;

	/* AlphaChannelProcessing */
	if(!XmRepTypeValidValue(enable_repid, html->html.alpha_processing,
		(TWidget)html))
		html->html.alpha_processing = XmALWAYS;

	/* ImageRGBConversion */
	if(!XmRepTypeValidValue(conv_repid, html->html.rgb_conv_mode,
		(TWidget)html) || html->html.rgb_conv_mode == XmDISABLED)
		html->html.rgb_conv_mode = XmBEST;

	/* ImageMapToPalette */
	if(!XmRepTypeValidValue(conv_repid, html->html.map_to_palette,
		(TWidget)html))
		html->html.map_to_palette = XmDISABLED;

	XmHTML_Initialize (html, req->html.value);
}

/*****
* Name: 		CreateAnchorCursor
* Return Type: 	void
* Description: 	creates the built-in anchor cursor
* In: 
*	html:		XmHTMLWidget for which to create a cursor
* Returns:
*	nothing.
*****/
static void 
CreateAnchorCursor(XmHTMLWidget html)
{
	_XmHTMLDebug(1, ("XmHTML.c: CreateAnchorCursor Start\n"));

	if(html->html.anchor_cursor == None)
	{
		Pixmap shape, mask;
		XColor white_def, black_def;
		Window window = XtWindow((TWidget)html);
		Display *display = XtDisplay((TWidget)html);
		Screen *screen = XtScreen((TWidget)html);

		if(!window)
			window = RootWindowOfScreen(screen);

		shape = XCreatePixmapFromBitmapData(display, window,
			fingers_bits, fingers_width, fingers_height, 1, 0, 1);

		mask = XCreatePixmapFromBitmapData(display, window,
			fingers_m_bits, fingers_m_width, fingers_m_height, 1, 0, 1);

		(void)XParseColor(display, html->core.colormap, "white", 
			&white_def);

		(void)XParseColor(display, html->core.colormap, "black", 
			&black_def);

		html->html.anchor_cursor = XCreatePixmapCursor(display, shape, mask, 
			&white_def, &black_def, fingers_x_hot, fingers_y_hot);
	}
	_XmHTMLDebug(1, ("XmHTML.c: CreateAnchorCursor End\n"));
}

/*****
* Name:			OverrideExposure
* Return Type: 	void
* Description: 	expose event filter when HTML form TWidgets are being scrolled.
* In: 
*	w:			unused;
*	client_..:	unused;
*	event:		XEvent info;
*	continu..:	flag to tell X whether or not to propagate this event; 
* Returns:
*	nothing.
* Note:
*	this routine is only activated when XmHTML is moving TWidgets on it's own
*	display area. It filters out any Exposure events that are generated by
*	moving these TWidgets around.
*****/
static void
OverrideExposure(TWidget w, XtPointer client_data, TEvent *event,
	Boolean *continue_to_dispatch)
{
	if(event->xany.type == Expose || event->xany.type == GraphicsExpose)
	{
		_XmHTMLDebug(1, ("XmHTML.c: OverrideExposure, ignoring %s event\n",
			(event->xany.type == Expose ? "Expose" : "GraphicsExpose"))); 
		*continue_to_dispatch = False;
	}
#ifdef DEBUG
	else
		_XmHTMLDebug(1, ("XmHTML.c: OverrideExposure, wrong event %i\n",
			(int)(event->xany.type)));
#endif
}

/*****
* Name: 		FormScroll
* Return Type: 	void
* Description: 	scrolls all TWidgets of all forms in the current document.
* In: 
*	html:		XmHTML TWidget id
* Returns:
*	nothing.
*****/
static void
FormScroll(XmHTMLWidget html)
{
	int x, y, xs, ys;
	XmHTMLFormData *form;
	XmHTMLForm *entry;
	Boolean did_anything = False;

	_XmHTMLDebug(1, ("XmHTML.c: FormScroll, Start\n"));

	/*****
	* To prevent the X exposure handling from going haywire, we simply
	* override *any* exposure events generated by moving the TWidgets 
	* around.
	*****/
	XtInsertEventHandler(html->html.work_area, ExposureMask, True,
		(XtEventHandler)OverrideExposure, NULL, XtListHead);

	for(form = html->html.form_data; form != NULL; form = form->next)
	{
		for(entry = form->components; entry != NULL; entry = entry->next)
		{
			if(entry->w)
			{
				/* save current TWidget position */
				x = entry->x;
				y = entry->y;

				/* compute new TWidget position */
				xs = entry->data->x - html->html.scroll_x;
				ys = entry->data->y - html->html.scroll_y;

				/* check if we need to show this TWidget */
				if(xs + entry->width > 0 && xs < html->html.work_width &&
					ys + entry->height > 0 && ys < html->html.work_height)
				{
					_XmHTMLDebug(1, ("XmHTML.c: FormScroll, moving "
						"TWidget %s to %ix%i\n", entry->name, xs, ys));

					/* save new TWidget position */
					entry->x = xs;
					entry->y = ys;

					/* and move to it */
					XtMoveWidget(entry->w, xs, ys);

					/* show it */
					if(!entry->mapped)
					{
						XtSetMappedWhenManaged(entry->w, True);
						entry->mapped = True;
					}

					/* restore background at previously obscured position */
					Refresh(html, x, y, entry->width, entry->height);

					did_anything = True;
				}
				else
				{
					/* hide by unmapping it */
					if(entry->mapped)
					{
						_XmHTMLDebug(1, ("XmHTML.c: FormScroll, hiding "
							"TWidget %s\n", entry->name));

						XtSetMappedWhenManaged(entry->w, False);
						entry->mapped = False;

						/* restore background at previously obscured position */
						Refresh(html, x, y, entry->width, entry->height);

						did_anything = True;
					}
				}
			}
		}
	}
	/* only do this if we actually did something */
	if(did_anything)
	{
		XSync(XtDisplay((TWidget)html), False);
		XmUpdateDisplay((TWidget)html);
	}

	XtRemoveEventHandler(html->html.work_area, ExposureMask, True,
		(XtEventHandler)OverrideExposure, NULL);

	_XmHTMLDebug(1, ("XmHTML.c: FormScroll, End\n"));
}

/*****
* Name: 		ClearArea
* Return Type: 	void
* Description: 	XClearArea wrapper. Does form component updating as well.
* In: 
*	html:		XmHTMLWidget id;
*	x,y:		upper left corner of region to be updated;
*	width:		width of region;
*	height:		height of region;
* Returns:
*
*****/
static void
ClearArea(XmHTMLWidget html, int x, int y, int width, int height)
{
	Display *dpy = XtDisplay(html->html.work_area);
	Window win = XtWindow(html->html.work_area);

	_XmHTMLDebug(1, ("XmHTML.c: ClearArea Start, x: %i, y: %i, width: %i "
		"height: %i.\n", x, y, width, height));

	/* first scroll form TWidgets if we have them */
	if(html->html.form_data)
	{
		FormScroll(html);
		XClearArea(dpy, win, x, y, width, height, False);
		Refresh(html, x, y, width, height);
	}
	else
		XClearArea(dpy, win, x, y, width, height, True);

	_XmHTMLDebug(1, ("XmHTML.c: ClearArea End.\n"));
}

/*****
* Name: 		_XmHTMLMoveToPos
* Return Type: 	void
* Description: 	scroll the working area with the given value
* In: 
*	w:			originator
*	html:		XmHTMLWidget
*	value:		position to scroll to
* Returns:
*	nothing
*****/
void
_XmHTMLMoveToPos(TWidget w, XmHTMLWidget html, int value)
{
	int inc, x, y, width, height;
	Display *dpy = Toolkit_Display(html->html.work_area);
	TWindow win = Toolkit_Widget_Window(html->html.work_area);
	TGC gc = html->html.gc;
	int vsb_width = 0, hsb_height = 0;

	/* sanity check */
	if(value < 0)
		return;

	/* default exposure region */
	x = y = 0;
	width = html->core.width;
	height = html->core.height;

	/* 
	* need to adjust slider position since we may not be called from
	* the scrollbar callback handler.
	*/
	XtVaSetValues(w, XmNvalue, value, NULL);

	/* vertical scrolling */
	if(w == html->html.vsb)
	{
		/* 
		* clicking on the slider causes activation of the scrollbar
		* callbacks. Since there is no real movement, just return.
		* Not doing this will cause an entire redraw of the window.
		*/
		if(value == html->html.scroll_y)
			return;		/* fix 01/20/97-01 kdh */

		/* save line number */
		SetCurrentLineNumber(html, value);

		/* moving down (text moving up) */
		if(value > html->html.scroll_y)
		{
			inc = value - html->html.scroll_y;

			/* save new value */
			html->html.scroll_y = value;

			/* save new paint engine start */
			html->html.paint_start = html->html.paint_end;

			/* small increment */
			if(inc < html->html.work_height)
			{
				/*****
				* See if we have a hsb. If we have one, we need to add
				* the height of the hsb to the region requiring updating.
				*****/
				if(html->html.needs_hsb)
#ifdef NO_XLIB_ILLEGAL_ACCESS
					GetScrollDim(html, &hsb_height, &vsb_width);
#else
					hsb_height = html->html.hsb->core.height;
#endif
				/* copy visible part upward */
				XCopyArea(dpy, win, win, gc, 0, inc,
					html->html.work_width + html->html.margin_width, 
					html->html.work_height - inc - hsb_height, 0, 0);

				/* clear area below */
				x = 0;
				y = html->html.work_height - inc - hsb_height;
				width = html->core.width;
				height = inc + hsb_height;
			}
			/* large increment, use default area */
		}
		/* moving up (text moving down) */
		else
		{
			inc = html->html.scroll_y - value;

			/* save new value */
			html->html.scroll_y = value;

			/* small increment */
			if(inc < html->html.work_height)
			{
				/* copy area down */
				XCopyArea(dpy, win, win, gc, 0, 0, 
					html->html.work_width + html->html.margin_width, 
					html->html.work_height - inc, 0, inc);

				/* save paint engine end */
				html->html.paint_end = html->html.paint_start;

				/* clear area above */
				x = y = 0;
				width = html->core.width;
				height = inc;
			}
			/* large increment, use default area */
		}
	}
	/* horizontal scrolling */
	else if(w == html->html.hsb)
	{
		/* 
		* clicking on the slider causes activation of the scrollbar
		* callbacks. Since there is no real movement, just return.
		* Not doing this will cause an entire redraw of the window.
		*/
		if(value == html->html.scroll_x)
			return;		/* fix 01/20/97-01 kdh */

		/* moving right (text moving left) */
		if (value > html->html.scroll_x)
		{
			inc = value - html->html.scroll_x;

			/* save new value */
			html->html.scroll_x = value;

			/* small increment */
			if(inc < html->html.work_width)
			{
				/*
				* See if we have a vsb. If we have, no additional offset
				* required, otherwise we also have to clear the space that
				* has been reserved for it.
				*/
				if(!html->html.needs_vsb)
#ifdef NO_XLIB_ILLEGAL_ACCESS
					GetScrollDim(html, &hsb_height, &vsb_width);
#else
					vsb_width = html->html.vsb->core.width;
#endif

				/* copy area to the left */
				XCopyArea(dpy, win, win, gc, inc, 0, 
					html->html.work_width - inc, html->html.work_height, 0, 0);

				/* clear area on right */
				x = html->html.work_width - inc;
				y = 0;
				width = inc + html->html.margin_width + vsb_width;
				height = html->html.work_height;
			}
			/* large increment, use default area */
		}
		/* moving left (text moving right) */
		else 
		{
			inc = html->html.scroll_x - value;

			/* save new value */
			html->html.scroll_x = value;

			/* small increment */
			if(inc < html->html.work_width)
			{
				if(!html->html.needs_vsb)
#ifdef NO_XLIB_ILLEGAL_ACCESS
					GetScrollDim(html, &hsb_height, &vsb_width);
#else
					vsb_width = html->html.vsb->core.width;
#endif

				/* copy area to the right */
				/* fix 01/24/97-01, kdh */
				XCopyArea(dpy, win, win, gc, 0, 0, 
					html->html.work_width - inc + html->html.margin_width +
					vsb_width, html->html.work_height, inc, 0); 

				/* clear area on left */
				x = y = 0;
				width = inc;
				height = html->html.work_height;
			}
			/* large increment, use default area */
		}
	}
	else
	{
		_XmHTMLWarning(__WFUNC__(html, "_XmHTMLMoveToPos"), 
			"Internal Error: unknown scrollbar!");
		return;
	}

	/* update display */
	ClearArea(html, x, y, width, height);
}

/*****
* Name: 		ScrollCB
* Return Type: 	void
* Description: 	callback procedure for scrollbar movement
* In: 
*	w:			originator
*	arg1:		client_data, in this case a XmHTMLWidget
*	arg2:		event specific callback structure.
* Returns:
*	nothing
*****/
static void
ScrollCB(TWidget w, XtPointer arg1, XtPointer arg2)
{
	XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)arg2;

	_XmHTMLDebug(1, ("XmHTML.c: ScrollCB, calling _XmHTMLMoveToPos\n"));
	_XmHTMLMoveToPos(w, XmHTML(arg1), cbs->value);
}

/*****
* Name: 		CreateHTMLWidget
* Return Type: 	void
* Description: 	creates the HTML TWidget
*				The actual area we use to draw into is a drawingAreaWidget.
* In: 
*	html:		TWidget to be created.
* Returns:
*	nothing
*****/
static void
CreateHTMLWidget(XmHTMLWidget html)
{
	Arg args[15];
	Dimension argc = 0;
	int vsb_width, hsb_height;
	static XtTranslations trans = NULL;

	_XmHTMLDebug(1, ("XmHTML.c: CreateHTMLWidget Start\n"));

	/* Check if user provided a work area */
	if(html->html.work_area == NULL)
	{
		html->html.work_area = XtVaCreateWidget("workWindow",
			xmDrawingAreaWidgetClass, (TWidget)html,
			XmNwidth, html->core.width,
			XmNheight, html->core.height,
			NULL);
	}
	/* catch all exposure events on the render window */
	XtAddEventHandler((TWidget)html->html.work_area, ExposureMask, True,
		(XtEventHandler)DrawRedisplay, (XtPointer)html);

	/* we want to know when to handle GraphicsExpose events */
	XtAddEventHandler((TWidget)html->html.work_area, VisibilityChangeMask, True,
		(XtEventHandler)VisibilityHandler, (XtPointer)html);

	XtAddEventHandler((TWidget)html, SubstructureNotifyMask, 
		True, (XtEventHandler)Mapped, (XtPointer)html);

	/* 
	* For some reason, Motif fucks up the original action list, so we
	* need to use a fallback copy instead.
	* Crash happens in XrmStringToQuark().
	*/
	XtAppAddActions(XtWidgetToApplicationContext(html->html.work_area),
		spareActions, XtNumber(spareActions));

	/* add translations for the actions */
	if(trans == NULL)
		trans = XtParseTranslationTable(translations);
	XtSetArg(args[0], XtNtranslations, trans);
	XtSetValues(html->html.work_area, args, 1);

	argc = 0;
	XtManageChild(html->html.work_area);

	if(html->html.vsb == NULL)
	{
		argc = 0;
		XtSetArg(args[argc], XmNorientation, XmVERTICAL); argc++;
		XtSetArg(args[argc], XmNrepeatDelay, html->html.repeat_delay); argc++;
		/* make them a little bit more responsive */
		XtSetArg(args[argc], XmNinitialDelay, 100); argc++;
		html->html.vsb = XtCreateWidget("verticalScrollBar", 
			xmScrollBarWidgetClass, (TWidget)html, args, argc);
	}
	XtManageChild(html->html.vsb);
	/* Catch vertical scrollbar movement */
	XtAddCallback(html->html.vsb, XmNvalueChangedCallback,
		(XtCallbackProc)ScrollCB, (XtPointer)html);
	XtAddCallback(html->html.vsb, XmNdragCallback,
		(XtCallbackProc)ScrollCB, (XtPointer)html);

	if(html->html.hsb == NULL)
	{
		argc = 0;
		XtSetArg(args[argc], XmNorientation, XmHORIZONTAL); argc++;
		XtSetArg(args[argc], XmNrepeatDelay, html->html.repeat_delay); argc++;
		/* make them a little bit more responsive */
		XtSetArg(args[argc], XmNinitialDelay, 100); argc++;
		html->html.hsb = XtCreateWidget("horizontalScrollBar", 
			xmScrollBarWidgetClass, (TWidget)html, args, argc);
	}
	XtManageChild(html->html.hsb);
	/* Catch horizontal scrollbar movement */
	XtAddCallback(html->html.hsb, XmNvalueChangedCallback,
		(XtCallbackProc)ScrollCB, (XtPointer)html);
	XtAddCallback(html->html.hsb, XmNdragCallback,
		(XtCallbackProc)ScrollCB, (XtPointer)html);

	/* 
	* subtract margin_width once to minimize number of calcs in
	* the paint routines: every thing rendered starts at an x position
	* of margin_width.
	*/
	GetScrollDim(html, &hsb_height, &vsb_width);

	html->html.work_width = html->core.width-html->html.margin_width-vsb_width;
	html->html.work_height= html->core.height;

	_XmHTMLDebug(1, ("XmHTML.c: CreateHTMLWidget End\n"));
	return;
}

/*****
* Name:			VisibilityHandler
* Return Type: 	void
* Description: 	VisibilityChangeMask event handler. Used to store the
*				visibility state of the work_area so we know when to
*				serve or ignore GraphicsExpose requests.
* In: 
*	w:			owner of this eventhandler 
*	html:		client data, XmHTMLWidget to which w belongs
*	event:		VisibilityNotify event data
* Returns:
*	nothing, but sets the visibility field in the TWidget's instance structure.
*****/
/*ARGSUSED*/
static void
VisibilityHandler(TWidget w, XmHTMLWidget html, XEvent *event)
{
	if(event->type != VisibilityNotify)
		return;

	_XmHTMLDebug(1, ("XmHTML.c: VisibilityHandler start\n"));

	html->html.visibility = event->xvisibility.state;

	_XmHTMLDebug(1, ("XmHTML.c: VisibilityHandler end\n"));
}

/*****
* Name: 		Mapped
* Return Type: 	void
* Description: 	event handler for CreateNotify events.
* In: 
*	w:			owner of this eventhandler 
*	html:		client data, XmHTMLWidget to which w belongs
*	event:		CreateNotify event data
* Returns:
*	nothing
* Note:
*	We want to be notified when the window gets created. Motif seems to block
*	the CreateNotify event, so we work with the MapNotify event. This is
*	required to get the text rendered correctly when it has been
*	set inside the Xt[Va]Create[Managed]TWidget and before XtRealizeWidget
*	has been called: we do not have a window yet and thus no valid gc. Bad 
*	things happen otherwise.
*****/
/*ARGSUSED*/
static void
Mapped(TWidget w, XmHTMLWidget html, XEvent *event)
{
	/* wrong event, just return */
	if(event->type != MapNotify)
		return;

	_XmHTMLDebug(1, ("XmHTML.c: Mapped start\n"));

	_XmHTMLDebug(1, ("XmHTML.c: Mapped, work area dimensions: %ix%i\n",
		html->html.work_width, html->html.work_height));

	CheckGC(html);

	/* save new height */
	html->html.work_height = html->core.height;
	/* and width as well, fix 10/26/97-01, kdh */
	html->html.work_width = html->core.width - html->html.margin_width -
							html->html.vsb->core.width;

	_XmHTMLDebug(1, ("XmHTML.c: Mapped, new work area dimensions: %ix%i\n",
		html->html.work_width, html->html.work_height));

	/* configure the scrollbars, will also resize work_area */
	CheckScrollBars(html);

	Layout(html);

	/* no longer needed now, so remove it */ 
	XtRemoveEventHandler(w, SubstructureNotifyMask, True,
		(XtEventHandler)Mapped, (XtPointer)html); 	

	_XmHTMLDebug(1, ("XmHTML.c: Mapped end.\n"));
}

/*****
* Name: 		CheckGC
* Return Type: 	void
* Description: 	creates a Graphics Context to be used for rendering
* In: 
*	html:		XmHTMLWidget
* Returns:
*	nothing, but a GC is created and stored in the TWidget's internal data
*	structure. If background images are allowed, a seperate GC is created
*	which is used in PaintBackground to do tiling of the background with an
*	image.
*****/
static void
CheckGC(XmHTMLWidget html)
{
	Display *dpy;

	_XmHTMLDebug(1, ("XmHTML.c: CheckGC Start\n"));

	/* sanity check */
	if(!XtIsRealized((TWidget)html))
		return;

	dpy = XtDisplay((TWidget)html);

	/* main gc */
	if(html->html.gc == NULL)
	{
		XGCValues xgc;

		xgc.function = GXcopy;
		xgc.plane_mask = AllPlanes;
		xgc.foreground = html->manager.foreground;
		xgc.background = html->core.background_pixel;
		html->html.gc = XCreateGC(dpy, XtWindow(html),
			GCFunction | GCPlaneMask | GCForeground | GCBackground, &xgc);

		_XmHTMLRecomputeColors(html);

		_XmHTMLDebug(1, ("XmHTML.c: CheckGC, gc created\n"));
	}
	/* background image gc */
	if(html->html.body_images_enabled && html->html.bg_gc == NULL)
	{
		html->html.bg_gc = XCreateGC(dpy, XtWindow(html), 0, NULL);
		XCopyGC(dpy, html->html.gc, 0xFFFF, html->html.bg_gc);
	}

	_XmHTMLDebug(1, ("XmHTML.c: CheckGC End\n"));
}

/*****
* Name: 		CheckScrollBars
* Return Type: 	void
* Description: 	(re)configures scrollbars
* In: 
*	html:		HTML TWidget to configure
* Returns:
*	nothing.
*****/
static void
CheckScrollBars(XmHTMLWidget html)
{
	int dx, dy, hsb_height, vsb_width, st;
	Boolean hsb_on_top, vsb_on_left;
	/* forced display of scrollbars: XmSTATIC or frames with scrolling = yes */
	Boolean force_vsb = False, force_hsb = False;
	Arg args[10];
	Dimension argc = 0;

	_XmHTMLDebug(1, ("XmHTML.c: CheckScrollBars, start\n"));

	/* don't do a thing if we aren't managed yet */
	if(!(XtIsManaged((TWidget)html)))
		return;

	/* Initial work area offset */
	st = dx = dy = html->manager.shadow_thickness;
	GetScrollDim(html, &hsb_height, &vsb_width);

 	/* check if we need a vertical scrollbar */
	if(html->html.formatted_height < html->core.height)
	{
		html->html.needs_vsb = False;
		/* don't forget! */
		html->html.scroll_y = 0;
		XtUnmanageChild(html->html.vsb);
	}
	else
		html->html.needs_vsb = True;

	/* add a scrollbar if we must and it isn't already here */
	if(!html->html.needs_vsb && html->html.sb_policy == XmSTATIC)
	{
		html->html.needs_vsb = True;
		force_vsb = True;
	}

	/*
	* check if we need a horizontal scrollbar. If we have a vertical
	* scrollbar, we must add it's width or text might be lost.
	*/
	if(html->html.formatted_width < html->core.width -
		(html->html.needs_vsb ? vsb_width : 0))	/* fix 04/27/97-01, kdh */
	{
		html->html.needs_hsb = False;
		/* don't forget! */
		html->html.scroll_x = 0;
		XtUnmanageChild(html->html.hsb);
	}
	else
		html->html.needs_hsb = True;

	/* add a scrollbar if we must and it isn't already here */
	if(!html->html.needs_hsb && html->html.sb_policy == XmSTATIC)
	{
		html->html.needs_hsb = True;
		force_hsb = True;
	}

	/* if this is a frame, check what type of scrolling is requested */
	if(html->html.is_frame)
	{
		if(html->html.scroll_type == FRAME_SCROLL_NONE)
		{
			html->html.needs_hsb = False;
			html->html.needs_vsb = False;
			html->html.scroll_x = 0;
			html->html.scroll_y = 0;
			XtUnmanageChild(html->html.hsb);
			XtUnmanageChild(html->html.vsb);
		}
		else if(html->html.scroll_type == FRAME_SCROLL_YES)
		{
			html->html.needs_vsb = True;
			html->html.needs_hsb = True;
			force_vsb = True;
			force_hsb = True;
		}
		/* else scrolling is auto, just proceed */
	}

	/* return if we don't need any scrollbars */
	if(!html->html.needs_hsb && !html->html.needs_vsb)
	{
		_XmHTMLDebug(1, ("XmHTML.c: CheckScrollBars, end, no bars needed.\n"));
		/* move work_area to it's correct position */
		XtMoveWidget(html->html.work_area, dx, dy);
		XtResizeWidget(html->html.work_area, html->core.width,
			html->core.height, html->html.work_area->core.border_width);
		return;
	}

	/* see if we have to put hsb on top */
	hsb_on_top = (html->html.sb_placement == XmTOP_LEFT ||
		html->html.sb_placement == XmTOP_RIGHT);
	/* see if we have top put vsb on left */
	vsb_on_left = (html->html.sb_placement == XmTOP_LEFT ||
		html->html.sb_placement == XmBOTTOM_LEFT);

	/* horizontal sb on top */
	if(html->html.needs_hsb && hsb_on_top)
		dy += hsb_height;

	/* vertical sb on left */
	if(html->html.needs_vsb && vsb_on_left)
		dx += vsb_width;

	/* move work_area to it's correct position */
	XtMoveWidget(html->html.work_area, dx, dy);

	/* See what space we have to reserve for the scrollbars */
	if(html->html.needs_hsb && hsb_on_top == False)
		dy += hsb_height;
	if(html->html.needs_vsb && vsb_on_left == False)
		dx += vsb_width;

	XtResizeWidget(html->html.work_area, 
		html->core.width - dx, html->core.height - dy, 
		html->html.work_area->core.border_width);

	if(html->html.needs_hsb == True)
	{
		int pinc;

		_XmHTMLDebug(1, ("XmHTML.c: CheckScrollBars, setting hsb\n"));

		/* Set hsb size; adjust x-position if we have a vsb */
		dx = (html->html.needs_vsb ? vsb_width : 0);
		XtResizeWidget(html->html.hsb,
			html->core.width - dx - 2*st,
			html->html.hsb->core.height,
			html->html.hsb->core.border_width);

		/* pageIncrement == sliderSize */
		pinc = html->html.work_width - 2*(html->html.default_font ? 
			html->html.default_font->xfont->max_bounds.width :
			HORIZONTAL_INCREMENT);
		/* sanity check */
		if(pinc < 1)
			pinc = HORIZONTAL_INCREMENT;

		/* adjust horizontal scroll if necessary */
		if(html->html.scroll_x > html->html.formatted_width - pinc)
			html->html.scroll_x = html->html.formatted_width - pinc;
		/* fix 01/23/97-02, kdh */

		/*
		* Adjust if a horizontal scrollbar has been forced
		* (can only happen for frames with scrolling = yes)
		*/
		if(force_hsb && pinc > html->html.formatted_width)
		{
			pinc = html->html.formatted_width;
			html->html.scroll_x = 0;
		}

		argc = 0;
		XtSetArg(args[argc], XmNminimum, 0); argc++;
		XtSetArg(args[argc], XmNmaximum, html->html.formatted_width); argc++;
		XtSetArg(args[argc], XmNvalue, html->html.scroll_x); argc++;
		XtSetArg(args[argc], XmNsliderSize, pinc); argc++;
		XtSetArg(args[argc], XmNincrement, (html->html.default_font ? 
			html->html.default_font->xfont->max_bounds.width :
			HORIZONTAL_INCREMENT));
		argc++;
		XtSetArg(args[argc], XmNpageIncrement, pinc); argc++;
		XtSetValues(html->html.hsb, args, argc);

		/* adjust x-position if vsb is on left */
 		dx = (html->html.needs_vsb && vsb_on_left ? vsb_width : 0);

		/* place it */
		if(hsb_on_top)
			XtMoveWidget(html->html.hsb, dx, 0);
		else
			XtMoveWidget(html->html.hsb, dx, (html->core.height - hsb_height));
	}
	if(html->html.needs_vsb == True)
	{
		int pinc;
		
		_XmHTMLDebug(1, ("XmHTML.c: CheckScrollBars, setting vsb\n"));

		/* Set vsb size; adjust y-position if we have a hsb */
		dy = (html->html.needs_hsb ? hsb_height : 0);
		XtResizeWidget(html->html.vsb, 
			html->html.vsb->core.width,
			html->core.height - dy - 2*st,
			html->html.vsb->core.border_width);

		/* pageIncrement == sliderSize */
		pinc = html->html.work_height - 2*(html->html.default_font ? 
			html->html.default_font->height : VERTICAL_INCREMENT);
		/* sanity check */
		if(pinc < 1)
			pinc = VERTICAL_INCREMENT;

		/* adjust vertical scroll if necessary */
		if(html->html.scroll_y > html->html.formatted_height - pinc)
			html->html.scroll_y = html->html.formatted_height - pinc;

		/*
		* Adjust if a vertical scrollbar has been forced
		* (can only happen if scrollBarDisplayPolicy == XmSTATIC)
		*/
		if(force_vsb && pinc > html->html.formatted_height)
		{
			pinc = html->html.formatted_height;
			html->html.scroll_y = 0;
		}

		argc = 0;
		XtSetArg(args[argc], XmNminimum, 0); argc++;
		XtSetArg(args[argc], XmNmaximum, html->html.formatted_height); argc++;
		XtSetArg(args[argc], XmNvalue, html->html.scroll_y); argc++;
		XtSetArg(args[argc], XmNsliderSize, pinc); argc++;
		XtSetArg(args[argc], XmNincrement, (html->html.default_font ? 
			html->html.default_font->height : VERTICAL_INCREMENT)); argc++;
		XtSetArg(args[argc], XmNpageIncrement, pinc); argc++;
		XtSetValues(html->html.vsb, args, argc);

		/* adjust y-position if hsb is on top */
 		dy = (html->html.needs_hsb && hsb_on_top ? hsb_height : 0);

		/* place it */
		if(vsb_on_left)
			XtMoveWidget(html->html.vsb, 0, dy);
		else
			XtMoveWidget(html->html.vsb, (html->core.width - vsb_width), dy);
	}
	_XmHTMLDebug(1, ("XmHTML.c: CheckScrollBars, end\n"));
}

/*****
* Name: 		GeometryManager
* Return Type: 	XtGeometryResult
* Description:	XmHTMLWidgetClass geometry_manager method
* In: 
*
* Returns:
*	Don't care. Just pass everything on.
*****/
static XtGeometryResult 
GeometryManager(TWidget w, XtWidgetGeometry *request,
	XtWidgetGeometry *geometry_return)
{
	_XmHTMLDebug(1, ("XmHTML.c: GeometryManager Start\n"));

	if(request->request_mode & CWX)
		geometry_return->x = request->x;
	if(request->request_mode & CWY)
		geometry_return->y = request->y;
	if(request->request_mode & CWWidth)
		geometry_return->width = request->width;
	if(request->request_mode & CWHeight)
		geometry_return->height = request->height;
	if(request->request_mode & CWBorderWidth)
		geometry_return->border_width = request->border_width;
	geometry_return->request_mode = request->request_mode;

	_XmHTMLDebug(1, ("XmHTML.c: GeometryManager End\n"));

	return(XtGeometryYes);
}

/*****
* Name: 		Destroy
* Return Type: 	void
* Description: 	XmHTMLWidgetClass destroy method. Frees up allocated resources.
* In: 
*	w:			TWidget to destroy
* Returns:
*	nothing
*****/
static void 
Destroy(TWidget w)
{
	XmHTMLWidget html = (XmHTMLWidget) w;

	_XmHTMLDebug(1, ("XmHTML.c: Destroy Start\n"));
	
	XmHTML_Destroy (html);

	/* remove all callbacks */
	XtRemoveAllCallbacks(w, XmNactivateCallback);
	XtRemoveAllCallbacks(w, XmNarmCallback);
	XtRemoveAllCallbacks(w, XmNanchorTrackCallback);
	XtRemoveAllCallbacks(w, XmNframeCallback);
	XtRemoveAllCallbacks(w, XmNformCallback);
	XtRemoveAllCallbacks(w, XmNinputCallback);
	XtRemoveAllCallbacks(w, XmNlinkCallback);
	XtRemoveAllCallbacks(w, XmNmotionTrackCallback);
	XtRemoveAllCallbacks(w, XmNimagemapCallback);
	XtRemoveAllCallbacks(w, XmNdocumentCallback);
	XtRemoveAllCallbacks(w, XmNfocusCallback);
	XtRemoveAllCallbacks(w, XmNlosingFocusCallback);

	/* invalidate this TWidget */
	w = NULL;


	_XmHTMLDebug(1, ("XmHTML.c: Destroy End\n"));
}	

/*****
* Name: 		HTMLProcessInput
* Return Type: 	void
* Description: 	handles keyboard input for the HTML TWidget.
* In: 
*	w:			XmHTMLWidget
*	event:		ButtonEvent structure
*	params:		additional args, unused
*	num_params:	no of addition args, unused
* Returns:
*	nothing
* Note:
*	This routine calls any installed XmNinputCallback callback resources.
*****/
static void 
HTMLProcessInput(TWidget w, XEvent *event, String *params, Cardinal *num_params)
{
	XmHTMLWidget html;
	/*
	* If this action proc is called directly from within application code,
	* w is a html TWidget. In all other cases this action proc is called 
	* for the translations installed on the work_area, and thus we need to
	* use XtParent to get our html TWidget.
	*/
	if(XmIsHTML(w))
		html = XmHTML (w);
	else
		html = XmHTML (XtParent(w));

	/* pass down if callback is installed */
	if(html->html.input_callback)
	{
		XmAnyCallbackStruct cbs;
		cbs.reason = XmCR_INPUT;
		cbs.event = event;
		XtCallCallbackList((TWidget)html, html->html.input_callback,
			&cbs);
	}
	_XmHTMLDebug(1, ("XmHTML.c: ProcessInput End\n"));
}

/*****
* Name: 		HTMLPageUpOrLeft
* Return Type: 	void
* Description: 	keyboard navigation action routine
* In: 
*	w:			TWidget id; XmHTMLWidget id if called from within application
*				code, work_area if handled by XmHTML itself;
*	event:		key event;
*	params:		0 for pageUp, 1 for pageLeft
*	num_params:	always 1
* Returns:
*	nothing
* Note:
*	This routine also honors the repeatDelay resource.
*****/
static void
HTMLPageUpOrLeft(TWidget w, XEvent *event, String *params, 
		Cardinal *num_params)
{
	int which;
	XmHTMLWidget html;
	static Time prev_time = 0;

	if(XmIsHTML(w))
		html = XmHTML (w);
	else
		html = XmHTML (XtParent(w));

	if(*num_params != 1 || !XtIsRealized(w))
	{
		if(*num_params != 1)
			_XmHTMLWarning(__WFUNC__(w, "HTMLPageUpOrLeft"),
				"page-up-or-left: invalid num_params. Must be exactly 1.");
		return;
	}

	/* check repeat delay */
	if(event->xkey.time - prev_time < html->html.repeat_delay)
		return;
	prev_time = event->xkey.time;

	which = atoi(params[0]);

	_XmHTMLDebug(1, ("XmHTML.c: HTMLPageUpOrLeft, which = %i\n", which));

	if(which == 0 && XtIsManaged(html->html.vsb))
		XtCallActionProc(html->html.vsb, "PageUpOrLeft", event, params, 1);
	else if(which == 1 && XtIsManaged(html->html.hsb))
		XtCallActionProc(html->html.hsb, "PageUpOrLeft", event, params, 1);
}

/*****
* Name: 		HTMLDownOrRight
* Return Type: 	void
* Description: 	keyboard navigation action routine
* In: 
*	w:			TWidget id; XmHTMLWidget id if called from within application
*				code, work_area if handled by XmHTML itself;
*	event:		key event;
*	params:		0 for pageDown, 1 for pageRight
*	num_params:	always 1
* Returns:
*	nothing
* Note:
*	This routine also honors the repeatDelay resource.
*****/
static void
HTMLPageDownOrRight(TWidget w, XEvent *event, String *params, 
		Cardinal *num_params)
{
	int which;
	XmHTMLWidget html;
	static Time prev_time = 0;

	if(XmIsHTML(w))
		html = XmHTML (w);
	else
		html = XmHTML (XtParent(w));

	if(*num_params != 1 || !XtIsRealized(w))
	{
		if(*num_params != 1)
			_XmHTMLWarning(__WFUNC__(w, "HTMLPageDownOrRight"),
				"page-down-or-right: invalid num_params. Must be exactly 1.");
		return;
	}

	/* check repeat delay */
	if(event->xkey.time - prev_time < html->html.repeat_delay)
		return;
	prev_time = event->xkey.time;

	which = atoi(params[0]);

	_XmHTMLDebug(1, ("XmHTML.c: HTMLPageDownOrRight, which = %i\n", which));

	if(which == 0 && XtIsManaged(html->html.vsb))
		XtCallActionProc(html->html.vsb, "PageDownOrRight", event, params, 1);
	else if(which == 1 && XtIsManaged(html->html.hsb))
		XtCallActionProc(html->html.hsb, "PageDownOrRight", event, params, 1);
}

/*****
* Name: 		HTMLIncrementUpOrLeft
* Return Type: 	void
* Description: 	keyboard navigation action routine
* In: 
*	w:			TWidget id; XmHTMLWidget id if called from within application
*				code, work_area if handled by XmHTML itself;
*	event:		key event;
*	params:		0 for IncrementUp, 1 for IncrementLeft
*	num_params:	always 1
* Returns:
*	nothing
* Note:
*	This routine also honors the repeatDelay resource.
*****/
static void
HTMLIncrementUpOrLeft(TWidget w, XEvent *event, String *params, 
		Cardinal *num_params)
{
	int which;
	XmHTMLWidget html;
	static Time prev_time = 0;

	if(XmIsHTML(w))
		html = XmHTML (w);
	else
		html = XmHTML (XtParent(w));

	if(*num_params != 1 || !XtIsRealized(w))
	{
		if(*num_params != 1)
			_XmHTMLWarning(__WFUNC__(w, "HTMLIncrementUpOrLeft"),
				"increment-up-or-left: invalid num_params. Must be exactly 1.");
		return;
	}

	/* check repeat delay */
	if(event->xkey.time - prev_time < html->html.repeat_delay)
		return;
	prev_time = event->xkey.time;

	which = atoi(params[0]);

	_XmHTMLDebug(1, ("XmHTML.c: HTMLIncrementUpOrLeft, which = %i\n", which));

	if(which == 0 && XtIsManaged(html->html.vsb))
		XtCallActionProc(html->html.vsb, "IncrementUpOrLeft", event,
			params, 1);
	else if(which == 1 && XtIsManaged(html->html.hsb))
		XtCallActionProc(html->html.hsb, "IncrementUpOrLeft", event,
			params, 1);
}

/*****
* Name: 		HTMLIncrementDownOrRight
* Return Type: 	void
* Description: 	keyboard navigation action routine
* In: 
*	w:			TWidget id; XmHTMLWidget id if called from within application
*				code, work_area if handled by XmHTML itself;
*	event:		key event;
*	params:		0 for IncrementDown, 1 for IncrementRight
*	num_params:	always 1
* Returns:
*	nothing
* Note:
*	This routine also honors the repeatDelay resource.
*****/
static void
HTMLIncrementDownOrRight(TWidget w, XEvent *event, String *params, 
		Cardinal *num_params)
{
	int which;
	XmHTMLWidget html;
	static Time prev_time = 0;

	if(XmIsHTML(w))
		html = XmHTML (w);
	else
		html = XmHTML (XtParent(w));

	if(*num_params != 1 || !XtIsRealized(w))
	{
		if(*num_params != 1)
			_XmHTMLWarning(__WFUNC__(w, "HTMLIncrementDownOrRight"),
				"increment-down-or-right: invalid num_params. Must be "
				"exactly 1.");
		return;
	}

	/* check repeat delay */
	if(event->xkey.time - prev_time < html->html.repeat_delay)
		return;
	prev_time = event->xkey.time;

	which = atoi(params[0]);

	_XmHTMLDebug(1, ("XmHTML.c: HTMLIncrementDownOrRight, which = %i\n",
		which));

	if(which == 0 && XtIsManaged(html->html.vsb))
		XtCallActionProc(html->html.vsb, "IncrementDownOrRight", event, 
			params, 1);
	else if(which == 1 && XtIsManaged(html->html.hsb))
		XtCallActionProc(html->html.hsb, "IncrementDownOrRight", event, 
			params, 1);
}

/*****
* Name: 		HTMLTopOrBottom
* Return Type: 	void
* Description: 	keyboard navigation action routine
* In: 
*	w:			TWidget id; XmHTMLWidget id if called from within application
*				code, work_area if handled by XmHTML itself;
*	event:		key event;
*	params:		0 for top, 1 for bottom
*	num_params:	always 1
* Returns:
*	nothing
* Note:
*	no repeatDelay by this action routine, it only moves from top to bottom
*	or vice-versa
*****/
static void
HTMLTopOrBottom(TWidget w, XEvent *event, String *params, 
		Cardinal *num_params)
{
	int which;
	XmHTMLWidget html;

	if(XmIsHTML(w))
		html = XmHTML (w);
	else
		html = XmHTML (XtParent(w));

	if(*num_params != 1 || !XtIsRealized(w))
	{
		if(*num_params != 1)
			_XmHTMLWarning(__WFUNC__(w, "HTMLTopOrBottom"),
				"top-or-bottom: invalid num_params. Must be exactly 1.");
		return;
	}
	which = atoi(params[0]);

	_XmHTMLDebug(1, ("XmHTML.c: HTMLTopOrBottom, which = %i\n", which));

	if(which == 0 && XtIsManaged(html->html.vsb))
	{
		/* no move if already on top */
		if(html->html.top_line == 0)
			return;

		html->html.top_line = 0;
		_XmHTMLMoveToPos(html->html.vsb, html, 0);
	}
	else if(which == 1 && XtIsManaged(html->html.vsb))
	{
		int value;

		/* no move if already on bottom */
		if(html->html.top_line == html->html.nlines)
			return;

		html->html.top_line = html->html.nlines;
		value = html->html.formatted_height;

		/* fix 01/30/97-04, kdh */
		AdjustVerticalScrollValue(html->html.vsb, value);

		_XmHTMLMoveToPos(html->html.vsb, html, value);
	}
}

static void
HTMLTraverseCurrent(TWidget w, XEvent *event, String *params,
	Cardinal *num_params)
{
	if(!XtIsRealized(w))
		return;
	_XmHTMLProcessTraversal(w, XmTRAVERSE_CURRENT);
}

static void
HTMLTraverseNext(TWidget w, XEvent *event, String *params,
	Cardinal *num_params)
{
	if(!XtIsRealized(w))
		return;
	_XmHTMLProcessTraversal(w, XmTRAVERSE_NEXT);
}

static void
HTMLTraversePrev(TWidget w, XEvent *event, String *params,
	Cardinal *num_params)
{
	if(!XtIsRealized(w))
		return;

	_XmHTMLProcessTraversal(w, XmTRAVERSE_PREV);
}

static void
HTMLTraverseNextOrPrev(TWidget w, XEvent *event, String *params,
	Cardinal *num_params)
{
	int which;

	if(*num_params != 1 || !XtIsRealized(w))
	{
		if(*num_params != 1)
			_XmHTMLWarning(__WFUNC__(w, "HTMLTraverseNextOrPrev"),
				"traverse-next-or-prev: invalid num_params. Must be "
				"exactly 1.");
		return;
	}
	which = atoi(params[0]);
	if(which == 0)
		_XmHTMLProcessTraversal(w, XmTRAVERSE_NEXT_TAB_GROUP);
	else
		_XmHTMLProcessTraversal(w, XmTRAVERSE_PREV_TAB_GROUP);
}

/*****
* Name: 		XmCreateHTML
* Return Type: 	TWidget
* Description: 	creates a XmHTML TWidget
* In: 
*	parent:		TWidget to act as parent for this new XmHTMLWidget
*	name:		name for the new TWidget
*	arglist:	arguments for this new XmHTMLWidget
*	argcount:	no of arguments
* Returns:
*	a newly created TWidget. This routine exits if parent is NULL or a subclass
*	of XmGadget.
*****/
TWidget
XmCreateHTML(TWidget parent, String name, ArgList arglist, Cardinal argcount)
{
	if(parent && !XmIsGadget(parent))
		return(XtCreateWidget(name, xmHTMLWidgetClass, parent, 
			arglist, argcount));

	_XmHTMLError(__WFUNC__(NULL, "XmCreateHTML"), "XmHTML requires "
		"a non-%s parent", parent ? "gadget" : "NULL");

	/* keep compiler happy */
	return(NULL);
}

static void
XmHTML_Frontend_Redisplay (XmHTMLWidget html)
{
	ClearArea(html, 0, 0, html->core.width, html->core.height);

	/* sync so the display is updated */
	XSync(XtDisplay((TWidget)html), False);
	
	XmUpdateDisplay((TWidget)html);
	if(XtIsManaged(html->html.vsb))
		XmUpdateDisplay(html->html.vsb);
	if(XtIsManaged(html->html.hsb))
		XmUpdateDisplay(html->html.hsb);
}
