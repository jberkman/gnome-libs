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

static GtkContainer *parent_class = NULL;

static gint gtk_xmhtml_signals [LAST_SIGNAL] = { 0, };

/* prototypes for functions defined here */
static void gtk_xmhtml_map (GtkWidget *widget);
guint gtk_xmhtml_get_type (void);

static void CheckScrollBars(XmHTMLWidget html);
static void GetScrollDim(XmHTMLWidget html, int *hsb_height, int *vsb_width);

static Pixel
pixel_color (char *color_name)
{
	GdkColor c;
	
	gdk_color_parse (color_name, &c);
	return c.pixel;
}

/* These are initialized in the Motif sources with the resources */
gtk_xmhtml_resource_init (GtkXmHTML *html)
{
	/* The strings */
	html->html.mime_type             = g_strdup ("text/html");
	html->html.charset               = g_strdup ("iso8859-1");
	html->html.font_family           = g_strdup ("adobe-times-normal-*");
	html->html.font_family_fixed     = g_strdup ("adobe-courier-normal-*");
	html->html.font_sizes            = g_strdup ("14,8,24,18,14,12,10,8");
	html->html.font_sizes_fixed      = g_strdup ("12,8");
	html->html.zCmd                  = g_strdup ("gunzip");
				         
	html->html.needs_vsb       	 = FALSE;
	html->html.needs_hsb       	 = FALSE;
	html->html.scroll_x        	 = 0;
	html->html.scroll_y        	 = 0;
	html->html.alignment             = TALIGNMENT_BEGINNING;
	html->html.anchor_cursor         = NULL;
	html->html.anchor_display_cursor = TRUE;
	html->html.anchor_buttons        = TRUE;
	
	html->html.anchor_fg             = pixel_color ("blue1");
	html->html.anchor_visited_fg     = pixel_color ("red");
	html->html.anchor_target_fg      = pixel_color ("blue1");
	html->html.anchor_activated_fg   = pixel_color ("red");
	html->html.anchor_activated_bg   = pixel_color ("white");
	
	html->html.highlight_on_enter    = TRUE;
	html->html.anchor_underline_type = LINE_SOLID | LINE_UNDER | LINE_SINGLE;
	html->html.anchor_target_underline_type = LINE_DASHED | LINE_UNDER | LINE_SINGLE;
	html->html.anchor_visited_underline_type = LINE_SOLID | LINE_UNDER | LINE_SINGLE;
	
	html->html.anchor_visited_proc   = NULL;
	html->html.image_proc            = NULL;
	html->html.gif_proc              = NULL;
	html->html.anchor_track_callback = NULL;
	html->html.activate_callback     = NULL;
	html->html.arm_callback          = NULL;
	html->html.frame_callback        = NULL;
	html->html.form_callback         = NULL;
	html->html.focus_callback        = NULL;
	html->html.losing_focus_callback = NULL;
	html->html.link_callback         = NULL;
	html->html.input_callback        = NULL;
	html->html.motion_track_callback = NULL;
	html->html.imagemap_callback     = NULL;
	html->html.document_callback     = NULL;
	
	html->html.hsb                   = NULL;
	html->html.vsb                   = NULL;

	html->html.images_enabled        = TRUE;
	html->html.max_image_colors      = 0;
	html->html.screen_gamma          = 2.2;
	html->html.get_data              = NULL;
	html->html.end_data              = NULL;
	html->html.plc_delay             = PLC_DEFAULT_DELAY;
	html->html.plc_min_delay         = PLC_MIN_DELAY;
	html->html.plc_max_delay         = PLC_MAX_DELAY;
	html->html.perfect_colors        = 0; /* XmAUTOMATIC; */
	html->html.resize_height         = FALSE;
	html->html.resize_width          = FALSE;
	html->html.sb_policy             = GTK_POLICY_AUTOMATIC;
	html->html.sb_placement          = 0; /* FIXME */
	html->html.margin_height         = DEFAULT_MARGIN;
	html->html.margin_width          = DEFAULT_MARGIN;
	html->html.string_direction      = TSTRING_DIRECTION_L_TO_R;
	html->html.strict_checking       = FALSE;
	html->html.enable_outlining      = FALSE;
	html->html.top_line              = 0;
	html->html.value                 = NULL;
	html->html.work_area             = NULL;
	html->html.bad_html_warnings     = TRUE;
	html->html.body_colors_enabled   = TRUE;
	html->html.body_images_enabled   = TRUE;
	html->html.allow_color_switching = TRUE;
	html->html.allow_font_switching  = TRUE;
	html->html.allow_form_coloring   = TRUE;
	html->html.imagemap_fg           = pixel_color ("White");
	html->html.imagemap_draw         = FALSE;
	html->html.repeat_delay          = 25;
	html->html.freeze_animations     = FALSE;
	html->html.map_to_palette        = XmDISABLED;
	html->html.palette               = NULL;
	html->html.def_body_image_url    = NULL;
	html->html.alpha_processing      = XmALWAYS;
	html->html.rgb_conv_mode         = XmBEST;
#ifdef DEBUG
	html->html.debug_disable_warnings= FALSE;
	html->html.debug_full_output     = FALSE;
	html->html.debug_save_clipmasks  = FALSE;
	html->html.debug_prefix          = NULL;
	html->html.debug_no_loopcount    = FALSE;
	html->html.debug_levels          = NULL;
#endif
}

static void
gtk_xmhtml_init (GtkXmHTML *html, char *html_source)
{
	gtk_xmhtml_resource_init (html);
	XmHTML_Initialize (html, html, html_source);
}

GtkWidget *
gtk_xmhtml_new (char *html_source)
{
	GtkXmHTML *html;
	
	html = gtk_type_new (gtk_xmhtml_get_type ());
	return GTK_WIDGET (html);
}

static void
gtk_xmhtml_class_init (GtkXmHTMLClass *class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
		
	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	parent_class = gtk_type_class (gtk_container_get_type ());
	
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
gtk_xmhtml_get_type (void)
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
		};

		gtk_xmhtml_type = gtk_type_unique (gtk_container_get_type (), &gtk_xmhtml_info);
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
	html->html.anchor_cursor = gdk_cursor_new_from_pixmap (shape, mask,
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
	fprintf (stderr, "FormScroll called\n");
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
		gtk_widget_draw (GTK_WIDGET (html), &rect);
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
	fprintf (stderr, "Move to pos called %d\n", value);
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
		html->html.visibility = 0; /* event->xvisibility.state; */
		return TRUE;

	}
	return FALSE;
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
	if (html->html.gc == NULL){
		GdkGCValues xgc;

		xgc.function = GDK_COPY;
		xgc.foreground = GTK_WIDGET(html)->style->fg[GTK_STATE_NORMAL];
		xgc.background = GTK_WIDGET(html)->style->bg[GTK_STATE_NORMAL];
		html->html.gc = gdk_gc_new_with_values (GTK_WIDGET (html)->window, &xgc,
							GDK_GC_FOREGROUND | GDK_GC_BACKGROUND |
							GDK_GC_FUNCTION);
		fprintf (stderr, "FIXME: missing call to XmHTMLRecomputeColors\n");
/*		_XmHTMLRecomputeColors(html); */

		_XmHTMLDebug(1, ("XmHTML.c: CheckGC, gc created\n"));
	}
	/* background image gc */
	if(html->html.body_images_enabled && html->html.bg_gc == NULL)
	{
		html->html.bg_gc = gdk_gc_new (GTK_WIDGET (html)->window);
		fprintf (stderr, "Gdk-gc-copy should be implemnenbted\n");
		/* XCopyGC(dpy, html->html.gc, 0xFFFF, html->html.bg_gc); */
	}

	_XmHTMLDebug(1, ("XmHTML.c: CheckGC End\n"));
}

/*****
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
	int vsb_width, hsb_height;

	_XmHTMLDebug(1, ("XmHTML.c: CreateHTMLWidget Start\n"));

	/* Check if user provided a work area */
	if(html->html.work_area == NULL)
	{
		GtkWidget *draw_area;
		draw_area = gtk_drawing_area_new ();
		gtk_drawing_area_size (GTK_DRAWING_AREA (draw_area),
				       GTK_WIDGET(html)->allocation.width,
				       GTK_WIDGET(html)->allocation.height);
	}
	/* catch all exposure events on the render window */
	gtk_widget_set_events (html->html.work_area,
			       gtk_widget_get_events (html->html.work_area) |
			       GDK_EXPOSURE_MASK | GDK_VISIBILITY_MASK);

	gtk_signal_connect (GTK_OBJECT(html->html.work_area),
			    "event",
			    (GtkSignalFunc) html_work_area_callback, (gpointer) html);

	fprintf (stderr, "XT MANAGE IS NOT PRESENT\n");
#if 0
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

#endif
	/* 
	* subtract margin_width once to minimize number of calcs in
	* the paint routines: every thing rendered starts at an x position
	* of margin_width.
	*/
	GetScrollDim(html, &hsb_height, &vsb_width);

	html->html.work_width = GTK_WIDGET (html)->allocation.width - html->html.margin_width-vsb_width;
	html->html.work_height= GTK_WIDGET (html)->allocation.height;
	
	_XmHTMLDebug(1, ("XmHTML.c: CreateHTMLWidget End\n"));
	return;
}

static void
gtk_xmhtml_map (GtkWidget *widget)
{
	GtkWidget *scrollbar;
	GtkXmHTML *html = GTK_XMHTML (widget);
	
	if (GTK_WIDGET_CLASS (parent_class)->map)
		(*GTK_WIDGET_CLASS (parent_class)->map)(widget);
	
	_XmHTMLDebug(1, ("XmHTML.c: Mapped start\n"));

	_XmHTMLDebug(1, ("XmHTML.c: Mapped, work area dimensions: %ix%i\n",
		html->html.work_width, html->html.work_height));

	CheckGC (html);

	scrollbar = html->html.vsb;
	
	html->html.work_height = widget->allocation.height;
	html->html.work_width = widget->allocation.width - 
		(html->html.margin_width + scrollbar->allocation.width);

	_XmHTMLDebug(1, ("XmHTML.c: Mapped, new work area dimensions: %ix%i\n",
			 html->html.work_width, html->html.work_height));

	/* configure the scrollbars, will also resize work_area */
	CheckScrollBars (html);

	Layout(html);
	
	_XmHTMLDebug(1, ("XmHTML.c: Mapped end.\n"));
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
	fprintf (stderr, "CheckScrollBars unimplemented\n");
}

static void
XmHTML_Frontend_Redisplay (XmHTMLWidget html)
{
	GtkWidget *w = GTK_WIDGET (html);
	
	gtk_widget_draw (GTK_WIDGET (html), NULL);

	if (GTK_WIDGET_MAPPED (html->html.vsb))
		/* update_display html->html.vsb */;
	if (GTK_WIDGET_MAPPED (html->html.hsb))
		/* update_display html->html.hsb */;
}

/*****
* Name: 		GetScrollDim
* Return Type: 	void
* Description: 	retrieves width & height of the scrollbars
* In: 
*	html:		XmHTMLWidget for which to retrieve these values
*	hsb_height: thickness of horizontal scrollbar, filled upon return
*	vsb_width:	thickness of vertical scrollbar, filled upon return
* Returns:
*	nothing
*****/
static void
GetScrollDim(XmHTMLWidget html, int *hsb_height, int *vsb_width)
{
	int height = 0, width = 0;

	if(html->html.hsb)
		height = GTK_WIDGET (html->html.hsb)->allocation.height;
	
	/*
	 * Sanity check if the scrollbar dimensions exceed the TWidget dimensions
	 * Not doing this would lead to X Protocol errors whenever text is set
	 * into the TWidget: the size of the workArea will be less than or equal
	 * to zero when scrollbars are required.
	 * We need always need to do this check since it's possible that some
	 * user has been playing with the dimensions of the scrollbars.
	 */
	if(height >= GTK_WIDGET (html)->allocation.height){
		_XmHTMLWarning(__WFUNC__(html->html.hsb, "GetScrollDim"),
			       "Height of horizontal scrollbar (%i) exceeds height of parent "
			       "TWidget (%i).\n    Reset to 15.", height,
			       GTK_WIDGET(html)->allocation.height);
		height = 15;
		fprintf (stderr, "Die boy boy die\n");
		exit (1);
	}
	
	if(html->html.vsb){
		width = GTK_WIDGET (html->html.vsb)->allocation.width;
		if(width >= GTK_WIDGET (html)->allocation.width){
			_XmHTMLWarning(__WFUNC__(html->html.vsb, "GetScrollDim"),
				"Width of vertical scrollbar (%i) exceeds width of parent "
				"TWidget (%i).\n    Reset to 15.", width,
				       GTK_WIDGET (html)->allocation.width);
			width  = 15;
			fprintf (stderr, "Die vertical boy boy die!\n");
			exit (1);
		}
	}

	_XmHTMLDebug(1, ("XmHTML.c: GetScrollDim; height = %i, width = %i\n",
		height, width));

	*hsb_height = height;
	*vsb_width  = width;
}

void
_XmHTMLCheckXCC(XmHTMLWidget html)
{
	GtkWidget *htmlw = GTK_WIDGET (html);
	
	_XmHTMLDebug(1, ("XmHTML.c: _XmHTMLCheckXCC Start\n"));
	/*
	* CheckXCC is called each time an image is loaded, so it's quite
	* usefull if we have a GC around by the time the TWidget is being
	* mapped to the display.
	* Our SubstructureNotify event handler can fail in some cases leading to
	* a situation where we don't have a GC when images are about to be
	* rendered (especially background images can cause a problem, they
	* are at the top of the text).
	*/
	CheckGC (html);

	/*
	* Create an XCC. 
	* XmHTML never decides whether or not to use a private or standard
	* colormap. A private colormap can be supplied by setting it on the
	* TWidget's parent, we know how to deal with that.
	*/
	if(!html->html.xcc)
	{
		TVisual *visual = NULL;
		TColormap cmap;

		_XmHTMLDebug(1, ("XmHTML.c: _XmHTMLCheckXCC: creating an XCC\n"));

		cmap   = gtk_widget_get_colormap (htmlw);
		visual = gtk_widget_get_visual (htmlw);

		/* walk TWidget tree or get default visual */
		if(visual == NULL){
			/* visual = XCCGetParentVisual((TWidget)html);*/
			fprintf (stderr, "%s: should not happen\n", __FUNCTION__);
			exit (1);
		}
		/* create an xcc for this TWidget */
		html->html.xcc = XCCCreate((TWidget)html, visual, cmap);
	}
	
	_XmHTMLDebug(1, ("XmHTML.c: _XmHTMLCheckXCC End\n"));
}

static void
autoSizeWidget (XmHTMLWidget html)
{
	fprintf (stderr, "Autosize widget called\n");
}

/* XmImage configuration hook */
XmImageConfig *_xmimage_cfg;

