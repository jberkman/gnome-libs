#define SetScrollBars(HTML)
#define AdjustVerticalScrollValue(VSB,VAL)

/*
 * Gdk does not have a visibility mask thingie *yet*
 */
#ifndef GDK_VISIBILITY_MASK 
#   define GDK_VISIBILITY_MASK 0
#endif

#define SCROLLBAR_SPACING 0

enum {
	GTK_XMHTML_TEST_SIGNAL,
	LAST_SIGNAL
};

static GtkContainer *parent_class = NULL;

static gint gtk_xmhtml_signals [LAST_SIGNAL] = { 0, };

/* prototypes for functions defined here */
static void gtk_xmhtml_realize (GtkWidget *widget);
static void gtk_xmhtml_unrealize (GtkWidget *widget);
static void gtk_xmhtml_map (GtkWidget *widget);
static void gtk_xmhtml_draw (GtkWidget *widget, GdkRectangle *area);
static gint gtk_xmhtml_expose (GtkWidget *widget, GdkEventExpose *event);
static void gtk_xmhtml_add (GtkContainer *container, GtkWidget *widget);
static void gtk_xmhtml_manage (GtkContainer *container, GtkWidget *widget);
static void gtk_xmhtml_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void gtk_xmhtml_size_request (GtkWidget *widget, GtkRequisition *requisition);
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
void
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
gtk_xmhtml_init (GtkXmHTML *html)
{
	gtk_xmhtml_resource_init (html);
}

GtkWidget *
gtk_xmhtml_new (char *html_source)
{
	GtkXmHTML *html;
	
	html = gtk_type_new (gtk_xmhtml_get_type ());
	GTK_WIDGET(html)->allocation.width  = 200;
	GTK_WIDGET(html)->allocation.height = 200;
	XmHTML_Initialize (html, html, html_source);
	return GTK_WIDGET (html);
}

static void
gtk_xmhtml_class_init (GtkXmHTMLClass *class)
{
	GtkObjectClass    *object_class;
	GtkWidgetClass    *widget_class;
	GtkContainerClass *container_class;
		
	object_class    = (GtkObjectClass *) class;
	widget_class    = (GtkWidgetClass *) class;
	container_class = (GtkContainerClass *) class;

	parent_class = gtk_type_class (gtk_container_get_type ());
	
	gtk_xmhtml_signals [GTK_XMHTML_TEST_SIGNAL] =
		gtk_signal_new ("test-gtkxmhtml",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkXmHTMLClass, testsignal),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, gtk_xmhtml_signals, LAST_SIGNAL);
	class->testsignal = NULL;

	widget_class->map           = gtk_xmhtml_map;
/*	widget_class->unmap         = gtk_xmhtml_unmap; */
	widget_class->draw          = gtk_xmhtml_draw;
	widget_class->size_request  = gtk_xmhtml_size_request;
	widget_class->size_allocate = gtk_xmhtml_size_allocate;
	widget_class->expose_event  = gtk_xmhtml_expose;
	
	container_class->add = gtk_xmhtml_add;
}

void
gtk_xmhtml_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	GtkXmHTML *html = GTK_XMHTML (widget);
	int extra_width, extra_height;
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (requisition != NULL);
	
	requisition->width  = 0;
	requisition->height = 0;

	if (GTK_WIDGET_VISIBLE (html->html.work_area)){
		gtk_widget_size_request (html->html.work_area, &widget->requisition);

		requisition->width  += widget->requisition.width;
		requisition->height += widget->requisition.height;
	}

	extra_width = extra_height = 0;

	/* Horizontal scroll bar */
	gtk_widget_size_request (html->html.hsb, &html->html.hsb->requisition);
	requisition->width = MAX (requisition->width, html->html.hsb->requisition.width);
	extra_height = SCROLLBAR_SPACING + html->html.hsb->requisition.height;
		
	/* Vertical scrollbarl */
	gtk_widget_size_request (html->html.vsb, &html->html.vsb->requisition);
	requisition->height = MAX (requisition->height, html->html.vsb->requisition.height);
	extra_width = SCROLLBAR_SPACING + html->html.vsb->requisition.width;

	requisition->width  += GTK_CONTAINER (widget)->border_width * 2 + extra_width;
	requisition->height += GTK_CONTAINER (widget)->border_width * 2 + extra_height;
}

void
gtk_xmhtml_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GtkXmHTML *html = GTK_XMHTML (widget);
	
	widget->allocation = *allocation;
	gtk_container_disable_resize (GTK_CONTAINER (html));

	
	printf ("Size Allocate: (%d,%d) %d %d\n",
		allocation->x,
		allocation->y,
		allocation->height,
		allocation->width);
	Resize (widget);
	gtk_container_enable_resize (GTK_CONTAINER (html));
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
	TWindow window = Toolkit_Widget_Window((TWidget)html);
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

static void
gtk_xmhtml_draw (GtkWidget *widget, GdkRectangle *area)
{
	GtkXmHTML *html = GTK_XMHTML (widget);
	GdkRectangle na;

	if (!GTK_WIDGET_DRAWABLE (widget))
		return;

	if (gtk_widget_intersect (html->html.vsb, area, &na))
		gtk_widget_draw (html->html.vsb, &na);

	if (gtk_widget_intersect (html->html.hsb, area, &na))
		gtk_widget_draw (html->html.hsb, &na);

	if (gtk_widget_intersect (html->html.work_area, area, &na))
		gtk_widget_draw (html->html.work_area, &na);
}

static gint
gtk_xmhtml_expose (GtkWidget *widget, GdkEventExpose *event)
{
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

gint
work_area_expose (GtkWidget *widget, GdkEvent *event, gpointer closure)
{
	GtkXmHTML *html = closure;

/* 	fprintf (stderr, "->Expose %d %d!\n", widget->allocation.width, widget->allocation.height);*/

	Refresh(html, 0, 0, widget->allocation.width, widget->allocation.height);
/*	fprintf (stderr, "<-Expose!\n"); */
	return FALSE;
}

void
horizontal_scroll (GtkObject *obj, gpointer user_data)
{
	GtkAdjustment *adj = GTK_ADJUSTMENT (obj);
	GtkXmHTML *html = GTK_XMHTML (user_data);

	_XmHTMLDebug(1, ("XmHTML.c: ScrollCB, calling _XmHTMLMoveToPos\n"));
	_XmHTMLMoveToPos (html->html.hsb, html, adj->value);
}

void
vertical_scroll (GtkObject *obj, gpointer user_data)
{
	GtkAdjustment *adj = GTK_ADJUSTMENT (obj);
	GtkXmHTML *html = GTK_XMHTML (user_data);
	
	_XmHTMLDebug(1, ("XmHTML.c: ScrollCB, calling _XmHTMLMoveToPos\n"));
	_XmHTMLMoveToPos (html->html.vsb, html, adj->value);
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
	GtkWidget *draw_area;
	int events;
	int vsb_width, hsb_height;

	_XmHTMLDebug(1, ("XmHTML.c: CreateHTMLWidget Start\n"));

	/* Check if user provided a work area */
	if(html->html.work_area == NULL)
	{
		html->html.work_area = draw_area = gtk_drawing_area_new ();
		gtk_drawing_area_size (GTK_DRAWING_AREA (draw_area),
				       40, 40);
		gtk_xmhtml_manage (GTK_CONTAINER (html), draw_area);
		
		gtk_signal_connect (GTK_OBJECT (draw_area), "expose_event",
				    (GtkSignalFunc) work_area_expose, html);
		
		events = gtk_widget_get_events (draw_area);
		gtk_widget_set_events (draw_area, events | GDK_EXPOSURE_MASK);
		
		gtk_widget_show (draw_area);
	}

	/*
	 * Scrollbars for the widget
	 */
	if (html->html.hsb == NULL){
		html->hsba = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
		html->html.hsb = gtk_hscrollbar_new (GTK_ADJUSTMENT (html->hsba));
		gtk_xmhtml_manage (GTK_CONTAINER(html), GTK_WIDGET (html->html.hsb));
		gtk_widget_show (html->html.hsb);
		gtk_signal_connect (GTK_OBJECT(html->hsba), "value_changed",
				    (GtkSignalFunc) horizontal_scroll, html);
	}
	if (html->html.vsb == NULL){
		html->vsba = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
		html->html.vsb = gtk_vscrollbar_new (GTK_ADJUSTMENT (html->vsba));
		gtk_xmhtml_manage (GTK_CONTAINER(html), GTK_WIDGET (html->html.vsb));
		gtk_widget_show (html->html.vsb);
		gtk_signal_connect (GTK_OBJECT(html->vsba), "value_changed",
				    (GtkSignalFunc) vertical_scroll, html);
	}

#if 0
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
gtk_xmhtml_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	gint attributes_mask;
	
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_XMHTML (widget));
	
	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;
	
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	
	widget->window = gdk_window_new (widget->parent->window, &attributes, 
					 attributes_mask);
	gdk_window_set_user_data (widget->window, widget);
	
	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static void
gtk_xmhtml_unrealize (GtkWidget *widget)
{
	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_XMHTML (widget));

	GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED | GTK_MAPPED);

	gtk_style_detach (widget->style);
	gdk_window_destroy (widget->window);
	widget->window = NULL;
}

static void
gtk_xmhtml_add (GtkContainer *container, GtkWidget *widget)
{
	fprintf (stderr, "GtkXmHTML: you should not use gtk_container_add on this container\n");
}

static void
gtk_xmhtml_manage (GtkContainer *container, GtkWidget *widget)
{
	GtkXmHTML *html;
	g_return_if_fail (container != NULL);
	g_return_if_fail (widget != NULL);

	html = GTK_XMHTML (container);
	gtk_widget_set_parent (widget, GTK_WIDGET (container));

	if (GTK_WIDGET_REALIZED (html) && !GTK_WIDGET_REALIZED (widget))
		gtk_widget_realize (widget);

	if (GTK_WIDGET_MAPPED (html) && !GTK_WIDGET_MAPPED (widget))
		gtk_widget_map (widget);

	if (GTK_WIDGET_VISIBLE (html) && GTK_WIDGET_VISIBLE (widget))
		gtk_widget_queue_resize (GTK_WIDGET (html));
}

static void
gtk_map_item (GtkWidget *w)
{
	if (GTK_WIDGET_VISIBLE (w) && !GTK_WIDGET_MAPPED (w))
		gtk_widget_map (w);
}

static void
gtk_xmhtml_map (GtkWidget *widget)
{
	GtkWidget *scrollbar;
	GtkXmHTML *html = GTK_XMHTML (widget);

	g_return_if_fail (widget != NULL);
	GTK_WIDGET_SET_FLAGS(widget, GTK_MAPPED);
	
	if (GTK_WIDGET_CLASS (parent_class)->map)
		(*GTK_WIDGET_CLASS (parent_class)->map)(widget);

	gtk_map_item (html->html.work_area);
	_XmHTMLDebug(1, ("XmHTML.c: Mapped start\n"));
	_XmHTMLDebug(1, ("XmHTML.c: Mapped, work area dimensions: %ix%i\n",
		html->html.work_width, html->html.work_height));

	CheckGC (html);

	scrollbar = html->html.vsb;
	
	html->html.work_height = widget->allocation.height;
	html->html.work_width = widget->allocation.width - 
		(html->html.margin_width + (scrollbar ? scrollbar->allocation.width : 0));

	_XmHTMLDebug(1, ("XmHTML.c: Mapped, new work area dimensions: %ix%i\n",
			 html->html.work_width, html->html.work_height));

	/* configure the scrollbars, will also resize work_area */
	CheckScrollBars (html);
	gtk_map_item (html->html.vsb);
	gtk_map_item (html->html.hsb);
	Layout(html);
	
	_XmHTMLDebug(1, ("XmHTML.c: Mapped end.\n"));
}

static void
gtk_xmhtml_set_geometry (GtkWidget *widget, int x, int y, int width, int height)
{
	GtkAllocation allocation;
	
	allocation.x = x;
	allocation.y = y;
	allocation.width = width;
	allocation.height = height;
	
	gtk_widget_size_allocate (widget, &allocation);
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
	XmHTMLfont *f = html->html.default_font ? html->html.default_font : NULL;
	XFontStruct *xf;
	int dx, dy, hsb_height, vsb_width, st, nx, ny;
	int hsb_on_top, vsb_on_left;
	/* forced display of scrollbars: XmSTATIC or frames with scrolling = yes */
	int force_vsb = FALSE, force_hsb = FALSE;
	GtkAdjustment *hsba = GTK_ADJUSTMENT (html->hsba);
	GtkAdjustment *vsba = GTK_ADJUSTMENT (html->vsba);

	if (f)
		xf = (XFontStruct *) ((GdkFontPrivate *) f)->xfont;

	/* don't do a thing if we aren't managed yet */
	if (!GTK_WIDGET_MAPPED (html))
		return;

	/* Initial work area offset */
	st = dx = dy = 0;
	GetScrollDim (html, &hsb_height, &vsb_width);

	/* 1. Vertical scrollbar */
	
 	/*    check if we need a vertical scrollbar */
	if(html->html.formatted_height < Toolkit_Widget_Dim (html).height){
		html->html.needs_vsb = False;
		/* don't forget! */
		html->html.scroll_y = 0;
		gtk_widget_hide (html->html.vsb);
	} else
		html->html.needs_vsb = TRUE;

	/*     FIXME: Here: should we support Scrollbar policies, ie, always present,
	 *     or only on demand?  The XmHTML Motif code supports it.
	 */

	/* 2. Horizontal scrollbar */
	/*
	*     check if we need a horizontal scrollbar. If we have a vertical
	*     scrollbar, we must add it's width or text might be lost.
	*/
	if(html->html.formatted_width < Toolkit_Widget_Dim (html).width -
	   (html->html.needs_vsb ? vsb_width : 0))
	{
		html->html.needs_hsb = False;
		/* don't forget! */
		html->html.scroll_x = 0;
		gtk_widget_hide (html->html.hsb);
	} else
		html->html.needs_hsb = TRUE;

	/*     FIXME: same.  Should we support scrollbar policies? */

	/* if this is a frame, check what type of scrolling is requested */
	if(html->html.is_frame)
	{
		if(html->html.scroll_type == FRAME_SCROLL_NONE)
		{
			html->html.needs_hsb = False;
			html->html.needs_vsb = False;
			html->html.scroll_x = 0;
			html->html.scroll_y = 0;
			gtk_widget_hide (html->html.hsb);
			gtk_widget_hide (html->html.vsb);
		}
		else if(html->html.scroll_type == FRAME_SCROLL_YES)
		{
			html->html.needs_vsb = TRUE;
			html->html.needs_hsb = TRUE;
			force_vsb = TRUE;
			force_hsb = TRUE;
		}
		/* else scrolling is auto, just proceed */
	}

	/* return if we don't need any scrollbars */
	if(!html->html.needs_hsb && !html->html.needs_vsb)
	{
		_XmHTMLDebug(1, ("XmHTML.c: CheckScrollBars, end, no bars needed.\n"));

		gtk_xmhtml_set_geometry (html->html.work_area, dx, dy,
					 Toolkit_Widget_Dim (html).width,
					 Toolkit_Widget_Dim (html).height);
		return;
	}

	/* For now: we dont support this */
	hsb_on_top = 0;
	vsb_on_left = 0;

	/* horizontal sb on top */
	if(html->html.needs_hsb && hsb_on_top)
		dy += hsb_height;

	/* vertical sb on left */
	if(html->html.needs_vsb && vsb_on_left)
		dx += vsb_width;

	nx = dx;
	ny = dy;

	/* See what space we have to reserve for the scrollbars */
	if(html->html.needs_hsb && hsb_on_top == FALSE)
		dy += hsb_height;
	if(html->html.needs_vsb && vsb_on_left == FALSE)
		dx += vsb_width;

	gtk_xmhtml_set_geometry (html->html.work_area, nx, ny,
			      Toolkit_Widget_Dim (html).width - dx,
			      Toolkit_Widget_Dim (html).height - dy);

	if(html->html.needs_hsb == TRUE)
	{
		int pinc;
		int sb_width, sb_height;
		
		_XmHTMLDebug(1, ("XmHTML.c: CheckScrollBars, setting hsb\n"));

		/* Set hsb size; adjust x-position if we have a vsb */
		dx = (html->html.needs_vsb ? vsb_width : 0);
		
		sb_width  = Toolkit_Widget_Dim (html).width - dx - 2*st;
		sb_height = html->html.hsb->requisition.height;

		/* pageIncrement == sliderSize */
		pinc = html->html.work_width - 2*(f ? xf->max_bounds.width : HORIZONTAL_INCREMENT);
		
		/* sanity check */
		if(pinc < 1)
			pinc = HORIZONTAL_INCREMENT;

		/* adjust horizontal scroll if necessary */
		if (html->html.scroll_x > html->html.formatted_width - pinc)
			html->html.scroll_x = html->html.formatted_width - pinc;

		/*
		* Adjust if a horizontal scrollbar has been forced
		* (can only happen for frames with scrolling = yes)
		*/
		if(force_hsb && pinc > html->html.formatted_width)
		{
			pinc = html->html.formatted_width;
			html->html.scroll_x = 0;
		}

		hsba->upper          = (gfloat) html->html.formatted_width;
		hsba->value          = (gfloat) html->html.scroll_x;
		hsba->page_size      = (gfloat) pinc;
		hsba->page_increment = (gfloat) pinc;
		hsba->step_increment = (f ? xf->max_bounds.width : HORIZONTAL_INCREMENT);
		gtk_signal_emit_by_name (GTK_OBJECT (html->hsba), "changed");
		
		/* adjust x-position if vsb is on left */
 		dx = (html->html.needs_vsb && vsb_on_left ? vsb_width : 0);

		/* place it */
		gtk_xmhtml_set_geometry (html->html.hsb, dx,
					 hsb_on_top ? 0 : Toolkit_Widget_Dim(html).height - hsb_height,
					 sb_width, sb_height);
		gtk_widget_show (html->html.hsb);
	}
	if(html->html.needs_vsb == TRUE)
	{
		int pinc;
		int sb_width, sb_height;
		
		_XmHTMLDebug(1, ("XmHTML.c: CheckScrollBars, setting vsb\n"));

		/* Set vsb size; adjust y-position if we have a hsb */
		dy = (html->html.needs_hsb ? hsb_height : 0);
		sb_width  = html->html.vsb->requisition.width;
		sb_height = Toolkit_Widget_Dim (html).height - dy - 2*st;

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

		vsba->upper          = (gfloat) html->html.formatted_height;
		vsba->value          = (gfloat) html->html.scroll_y;
		vsba->page_size      = (gfloat) pinc;
		vsba->page_increment = (gfloat) pinc;
		vsba->step_increment = (html->html.default_font
					 ? html->html.default_font->height
					: VERTICAL_INCREMENT);
		gtk_signal_emit_by_name (GTK_OBJECT (html->vsba), "changed");
		
		/* adjust y-position if hsb is on top */
 		dy = (html->html.needs_hsb && hsb_on_top ? hsb_height : 0);

		gtk_xmhtml_set_geometry (html->html.vsb,
					 vsb_on_left ? 0 : Toolkit_Widget_Dim(html).width - vsb_width,
					 dy, sb_width, sb_height);
		gtk_widget_show (html->html.vsb);
	}
	_XmHTMLDebug(1, ("XmHTML.c: CheckScrollBars, end\n"));

}

static void
XmHTML_Frontend_Redisplay (XmHTMLWidget html)
{
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
		height = GTK_WIDGET (html->html.hsb)->requisition.height;
	
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
		width = GTK_WIDGET (html->html.vsb)->requisition.width;
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

/*****
* Name: 		autoSizeWidget
* Return Type: 	void
* Description: 	computes XmHTML's TWidget dimensions if we have to autosize
*				in either direction.
* In: 
*	html:		XmHTMLWidget id
* Returns:
*	nothing.
* Note:
*	This routine is fairly complicated due to the fact that the dimensions
*	of the work area are partly determined by the presence of possible
*	scrollbars.
*****/
static void
autoSizeWidget (XmHTMLWidget html)
{
}

/* XmImage configuration hook */
XmImageConfig *_xmimage_cfg;

