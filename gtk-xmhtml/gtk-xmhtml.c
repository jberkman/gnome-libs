#define SetScrollBars(HTML)
#define AdjustVerticalScrollValue(VSB,VAL)

/*
 * Gdk does not have a visibility mask thingie *yet*
 */
#ifndef GDK_VISIBILITY_MASK 
#   define GDK_VISIBILITY_MASK 0
#endif

enum {
	GTK_XMHTML_TEST_SIGNAL,
	LAST_SIGNAL
};

static gint gtk_xmhtml_signals [LAST_SIGNAL] = { 0, };

/* prototypes for functions defined here */
static void gtk_xmhtml_map (GtkWidget *widget);

static
static void
gtk_xmhtml_init (GtkXmHTML *html, char *html_source)
{
	/* private TWidget resources */
	html->html.needs_vsb    = False;
	html->html.needs_hsb    = False;
	html->html.scroll_x     = 0;
	html->html.scroll_y     = 0;

	CheckAnchorUnderlining(html, html);
}

GtkWidget *
gtk_xmhtml_new (char *html_source)
{
	GtkXmHTML *html;
	
	html = gtk_type_new (gtk_xmhtml_get_type ());
	XmHTML_Initialize (html, html_source);
	return GTK_WIDGET (html);
}

static void
gtk_xmhtml_class_init (GtkXmHTMLClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class
		
	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;
	
	gtk_xmhtml_signals [GTK_XMHTML_TEST_SIGNAL] = gtk_signal_new ("test-gtkxmhtml",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GtkXmHTMLClass, testsignal),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, gtk_xmhtml_signals, LAST_SIGNAL);
	class->testsignal = NULL;

	widget_class->map = gtk_xmhtml_map;
}

guint
gtk_xmhtml_get_type ()
{
	static guint gtk_xmhtml_type = 0;

	if (!gtk_xmhtml_type){
		GtkTypeInfo gtk_xmhtml_info =
		{
			"GtkXmHTML",
			sizeof (GtkXmHTML),
			sizeof (GtkXmHTMLClass),
			(GtkClassInitFunc) gtk_xmhtml_class_init,
			(GtkObjectInitFunc) gtk_xmhtml_init,
			(GtkArgFunc) NULL
		}

		gtk_xmhtml_type = gtk_type_unique (gtk_xmhtml_get_type (), &gtk_xmhtml_info);
	}
	return gtk_xmhtml_type;
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
	TPixmap shape, mask;
	TColor white_def, black_def;
	TWindow window = Toolkit_Widget_Window((TWidget)html);
	Display *display = Toolkit_Display((TWidget)html);
	Screen *screen = XtScreen((TWidget)html);
	GdkColormap *colormap;
	GdkColor fg, bg, white, black;

	fg.pixel = 1;
	bg.pixel = 0;
	if (html->html.anchor_cursor != None)
		return;

	if(!window)
		window = Toolkit_Default_Root_Window (display);

	shape = gdk_pixmap_create_from_data (window, fingers_bits, fingers_width, fingers_height,
					     1, &fg, &bg);
	
	mask = gdk_pixmap_create_from_data (window, fingers_m_bits, fingers_m_width, fingers_m_height,
					    1, &fg, &bg);

	colormap = gtk_widget_get_colormap (GTK_WIDGET (html));
	gdk_color_white (colormap, &white);
	gdk_color_black (colormap, &black);
	html->html.anchor_cursor = gdk_cursor_new_from_pixmap (display, shape, mask,
							       &white, &black,
							       fingers_x_hot, fingers_y_hot);
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
FormScroll (XmHTMLWidget html)
{
	fprintf ("FormScroll called\n");
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
	Display *dpy = Toolkit_Display (html->html.work_area);
	TWindow win =  Toolkit_Widget_Window (html->html.work_area);

	_XmHTMLDebug(1, ("XmHTML.c: ClearArea Start, x: %i, y: %i, width: %i "
		"height: %i.\n", x, y, width, height));

	/* first scroll form TWidgets if we have them */
	if(html->html.form_data)
	{
		GdkRectangle rect;

		rect.x = x;
		rect.y = y;
		rect.width  = width;
		rect.height = height;
		FormScroll(html);
		gdk_window_clear_area (win, x, y, width, height);
		gtk_widget_draw (html, &rect);
	}
	else
		gdk_window_clear_area_e (win, x, y, width, height);

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
	fprintf ("Move to pos called %d\n", value);
}

/*
 * html_work_areea_callback:
 *
 * Description:         Handles all of the events generated on an html->work_area
 */
gint
html_work_area_callback (GtkWidget *widget, GdkEvent *event, void *d)
{
	GtkXmHTML *html = GTK_XMHTML (d);

	switch (event->type){
	case GDK_EXPOSE:
		/* DrawRedisplay */
		return TRUE;
		
	case 999999 + GDK_VISIBILITY_MASK:
		/* Visibility Handler, 999999 is because there is no GDK_VIS yet */
		html->html.visibility = event->xvisibility.state;
		return TRUE;

	case XXX:
		
		
	}
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
	_XmHTMLDebug(1, ("XmHTML.c: CheckGC Start\n"));

	/* sanity check */
	if(!GTK_WIDGET_REALIZED (GTK_WIDGET (html)))
		return;

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

*****
 * Name: 		CreateHTMLWidget
 * Return Type: 	void
 * Description: 	creates the HTML TWidget
 *			The actual area we use to draw into is a drawingAreaWidget.
 * In: 
 *	html:		TWidget to be created.
 * Returns:
 *	nothing
 *****/
static void
CreateHTMLWidget(XmHTMLWidget html)
{
	_XmHTMLDebug(1, ("XmHTML.c: CreateHTMLWidget Start\n"));

	/* Check if user provided a work area */
	if(html->html.work_area == NULL)
	{
		GtkWidget *draw_area;
		draw_area = gtk_drawing_area_new ();
		gtk_drawing_area_size (GTK_DRAWING_AREA (draw_area),
				       html->core.width,
				       html->core.height);
	}
	/* catch all exposure events on the render window */
	gtk_widget_set_events (html->html.work_area,
			       gtk_widget_get_events (html->html.work_area) |
			       GDK_EXPOSURE_MASK | GDK_VISIBILITY_MASK);

	gtk_signal_connect (GTK_OBJECT(html->html.work_area),
			    "event",
			    (GtkSignalFunc) html_work_area_callback, (gpointer) html);

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

