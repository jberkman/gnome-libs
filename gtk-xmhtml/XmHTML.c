#ifndef lint
static char rcsId[]="$Header$";
#endif
/*****
* XmHTML.c : XmHTML main routines
*
* This file Version	$Revision$
*
* Creation date:		Thu Nov 21 05:02:44 GMT+0100 1996
* Last modification: 	$Date$
* By:					$Author$
* Current State:		$State$
*
* Author:				newt
* (C)Copyright 1995-1996 Ripley Software Development
* All Rights Reserved
*
* This file is part of the XmHTML Widget Library.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the Free
* Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/
/*****
* ChangeLog 
* $Log$
* Revision 1.8  1997/12/18 00:39:20  unammx
* It compiles and links -miguel
*
* Revision 1.7  1997/12/17 04:40:27  unammx
* Your daily XmHTML code is here.  It almost links.  Only the
* images.c file is left to port.  Once this is ported we are all
* set to start debugging this baby.
*
* btw, Dickscrape is a Motif based web browser that is entirely
* based on this widget, I just tested it today, very impressive.
*
* Miguel.
*
* Revision 1.6  1997/12/16 00:34:47  unammx
* More progress on the XmHTML work.  This time, I did frames.c, not
* as nice as I would like it to be.
*
* It still does not link.
*
* Revision 1.5  1997/12/14 23:20:59  unammx
* it compiles.  Now we need to make it link and work -mig
*
* Revision 1.4  1997/12/13 01:49:11  unammx
* your daily dose of ported XmHTML code, non functional as usual -mig
*
* Revision 1.3  1997/12/12 00:58:40  unammx
* Most of the Motif dependant code is now splitted.
*
* There are some bits still remaining in XmHTML.c, but will be
* soonish gone.
*
* As usual, it does not compile yet :-)
*
* Miguel
*
* Revision 1.2  1997/12/11 21:20:19  unammx
* Step 2: more gtk/xmhtml code, still non-working - mig
*
* Revision 1.17  1997/10/26 23:49:18  newt
* Bugfixes 10/22/97-01, 10/26/97-01
*
* Revision 1.16  1997/08/31 17:32:33  newt
* Again some fixes in _XmHTMLMoveToPos
*
* Revision 1.15  1997/08/30 00:38:58  newt
* Anchor Highlighting, form component traversal action routines, alpha
* channel support, loads of fixes in _XmHTMLMoveToPos, DrawRedisplay and
* SetValues.
*
* Revision 1.14  1997/08/01 12:54:48  newt
* Progressive image loading changes.
*
* Revision 1.13  1997/05/28 01:38:30  newt
* Added a check on the value of the XmNmaxImageColors resource. Added a check
* on the scrollbar dimensions. Modified the SetValues method to properly deal
* with the XmNbodyImage resource. Modified XmHTMLImageFreeImageInfo to call
* _XmHTMLFreeImageInfo.
*
* Revision 1.12  1997/04/29 14:22:14  newt
* A lot of changes in SetValues, Layout, XmHTMLTextSetString.
* Added XmHTMLXYToInfo.
*
* Revision 1.11  1997/04/03 05:31:02  newt
* Modified XmHTMLImageReplace and XmHTMLImageUpdate to immediatly render an
* image whenever possible. Added the XmHTMLFrameGetChild convenience function.
*
* Revision 1.10  1997/03/28 07:05:28  newt
* Frame interface added. 
* Horizontal scrolling bugfix. 
* Added all remaining action routines and adjusted translation table.
*
* Revision 1.9  1997/03/20 08:04:38  newt
* XmHTMLImageFreeImageInfo, XmNrepeatDelay, changes in public image functions
*
* Revision 1.8  1997/03/11 19:49:53  newt
* Fix 03/11/97-01, SetValues. 
* Added XmHTMLImageGetType, updated XmHTMLImageReplace and XmHTMLImageUpdate
*
* Revision 1.7  1997/03/04 18:45:15  newt
* ?
*
* Revision 1.6  1997/03/04 00:55:54  newt
* Delayed Image Loading changes: XmHTMLReplaceImage, XmHTMLUpdateImage 
* and XmHTMLRedisplay added
*
* Revision 1.5  1997/03/02 23:06:35  newt
* Added image/imagemap support; changed expose method; added background 
* image painting
*
* Revision 1.4  1997/02/11 02:05:44  newt
* Changes related to autosizing, exposure and scrolling
*
* Revision 1.3  1997/01/09 06:54:53  newt
* expanded copyright marker
*
* Revision 1.2  1997/01/09 06:42:40  newt
* XmNresizeWidth and XmNresizeHeight updates
*
* Revision 1.1  1996/12/19 02:17:03  newt
* Initial Revision
*
*****/ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WITH_MOTIF
#   include <X11/IntrinsicP.h>	/* Fast macros */
#    include <X11/keysym.h>		/* for keycodes in HTMLProcessInput */
#    include <X11/Xmu/StdSel.h>

#    include <Xm/XmP.h>			/* XmField, XmPartOffset and private motif funcs. */
#    include <Xm/DrawP.h>
#    include <Xm/XmStrDefs.h>	/* For motif XmN macros */

/***** 
* Test if these macros are present. They are required for conversion of the
* enumeration resource values. I don't know if they are present in Motif 1.2.X. 
* If not here, spit out an error message.
* If you can't compile this because this error occurs, please contact us
* at the address given below and state what version of Motif you are using.
* Note: these are explicit syntax errors...
*****/
#ifndef XmPartOffset
	XmPartOffset macro undefined. Please contact ripley@xs4all.nl
#endif
#ifndef XmField
	XmField macro undefined. Please contact ripley@xs4all.nl
#endif

#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/RepType.h>

#endif

/* Our private header files */
#include "XmHTMLP.h"
#include "XmHTMLfuncs.h"

/* This is the anchor cursor */
#include "bitmaps/fingers.xbm"
#include "bitmaps/fingers_m.xbm"
	
/*** External Function Prototype Declarations ***/
/* 
* undocumented motif functions used: (declared in XmP.h): 
* _XmRedisplayGadgets (Widget, XEvent*, Region);
*/

/*** Public Variable Declarations ***/

/*** Private Datatype Declarations ****/
/*** 
* button press & release must occur within half a second of each other to 
* trigger an anchor activation
***/
#define MAX_RELEASE_TIME	500		

/* Default margin offsets */
#define DEFAULT_MARGIN		20

/* initial horizontal & vertical increment when a scrollbar is moved */
#define HORIZONTAL_INCREMENT 12 /* average char width */
#define VERTICAL_INCREMENT 18	/* average line height */

static void XmHTML_Destroy(XmHTMLWidget html);
static void XmHTML_Initialize (XmHTMLWidget html, XmHTMLWidget init, char *html_source);
static void Layout(XmHTMLWidget html);
static void CheckMaxColorSetting(XmHTMLWidget html);
static void CheckPLCIntervals(XmHTMLWidget html);

#ifdef WITH_MOTIF
#    include "XmHTML-motif.c"
#else
#    include "gtk-xmhtml.c"
#endif
	
/*****
* Name: 		XmHTML_Initialize
* Return Type: 	void
* Description: 	Called when the TWidget is instantiated
* In: 
*	html:	        Widget
*       init:           Optionally the original widget (Motifish support)
*       html_source:    html source code.
* Returns:
*	nothing, but init is updated with checked/updated resource values.	
*****/
static void
XmHTML_Initialize (XmHTMLWidget html, XmHTMLWidget init, char *html_source)
{
	/* Initialize the global HTMLpart */
	_XmHTMLDebug(1, ("XmHTML.c: Initialize Start\n"));

	/* private TWidget resources */
	html->html.needs_vsb    = False;
	html->html.needs_hsb    = False;
	html->html.scroll_x     = 0;
	html->html.scroll_y     = 0;

#ifdef WITH_MOTIF
	CheckAnchorUnderlining(html, html);
#endif
	/* repeat delay. Must be positive */
	if(html->html.repeat_delay < 1)
	{
		_XmHTMLWarning(__WFUNC__(html, "Initialize"),
			"The specified value for XmNrepeatDelay (%i) is too small.\n"
			"    Reset to 25", html->html.repeat_delay);
		html->html.repeat_delay = 25;
	}

#ifdef WITH_MOTIF
	/* Set default text alignment */
	CheckAlignment(html, html);
#endif
	
	/****
	* Initialize private resources.
	****/
	/* Formatted document resources */
	html->html.formatted_width   = 1;
	html->html.formatted_height  = 1;
	html->html.elements          = (XmHTMLObject*)NULL;
	html->html.formatted         = (XmHTMLObjectTable*)NULL;
	html->html.paint_start       = (XmHTMLObjectTable*)NULL;
	html->html.paint_end         = (XmHTMLObjectTable*)NULL;
	html->html.nlines            = 0;

	/* body image */
	html->html.body_image        = (XmHTMLImage*)NULL;
	html->html.body_image_url    = (String)NULL;

	/* layout & paint engine resources */
	html->html.in_layout         = False;
	html->html.paint_x           = 0;
	html->html.paint_y           = 0;
	html->html.paint_width       = 0;
	html->html.paint_height      = 0;
#ifdef WITH_MOTIF
	html->html.body_fg           = html->manager.foreground;
	html->html.body_bg           = html->core.background_pixel;
#else
	html->html.body_fg           = GTK_WIDGET(html)->style->fg [GTK_STATE_NORMAL].pixel;
	html->html.body_bg           = GTK_WIDGET(html)->style->bg [GTK_STATE_NORMAL].pixel;
#endif
	html->html.images            = (XmHTMLImage*)NULL;
	html->html.image_maps        = (XmHTMLImageMap*)NULL;
	html->html.xcc               = (XCC)NULL;
	html->html.bg_gc             = (TGC)NULL;
	html->html.form_data         = (XmHTMLFormData*)NULL;
	html->html.delayed_creation  = False;	/* no delayed image creation */

	/***** 
	* Original colors must be stored. They can be altered by the
	* <BODY> element, so if we get a body without any or some of these
	* colors specified, we can use the proper default values for the 
	* unspecified elements.
	*****/
	html->html.body_fg_save             = html->html.body_fg;
	html->html.body_bg_save             = html->html.body_bg;
	html->html.anchor_fg_save           = html->html.anchor_fg;
	html->html.anchor_target_fg_save    = html->html.anchor_target_fg;
	html->html.anchor_visited_fg_save   = html->html.anchor_visited_fg;
	html->html.anchor_activated_fg_save = html->html.anchor_activated_fg;
	html->html.anchor_activated_bg_save = html->html.anchor_activated_bg;

	/* anchor resources */
	html->html.anchor_position_x             = 0;
	html->html.anchor_position_y             = 0;
	html->html.anchor_current_cursor_element = (XmHTMLAnchor*)NULL;
	html->html.armed_anchor                  = (XmHTMLObjectTable*)NULL;
	html->html.current_anchor                = (XmHTMLObjectTable*)NULL;
	html->html.num_anchors                   = 0;
	html->html.num_named_anchors             = 0;
	html->html.anchors                       = (XmHTMLWord*)NULL;
	html->html.anchor_words                  = 0;
	html->html.named_anchors                 = (XmHTMLObjectTable*)NULL;
	html->html.anchor_data                   = (XmHTMLAnchor*)NULL;
	html->html.press_x                       = 0;
	html->html.press_y                       = 0;
	html->html.pressed_time                  = 0;
	html->html.selected_time                 = 0;
	html->html.selected                      = (XmHTMLAnchor*)NULL;

	/* Text selection resources */
	html->html.selection    = (XmHTMLObjectTable*)NULL;
	html->html.select_start = 0;
	html->html.select_end   = 0;

	/* HTML Frame resources */
	html->html.nframes  = 0;
	html->html.frames   = NULL;
	html->html.is_frame = False;

	/* PLC resources */
	html->html.plc_buffer    = (PLCPtr)NULL;
	html->html.num_plcs      = 0;
	html->html.plc_proc_id   = None;
	html->html.plc_suspended = True;
	html->html.plc_gc        = (TGC)NULL;

	/* initial mimetype */
	if(!(strcasecmp(html->html.mime_type, "text/html")))
		html->html.mime_id = XmPLC_DOCUMENT;
	else if(!(strcasecmp(html->html.mime_type, "text/html-perfect")))
		html->html.mime_id = XmPLC_DOCUMENT;
	else if(!(strcasecmp(html->html.mime_type, "text/plain")))
		html->html.mime_id = XmNONE;
	else if(!(strncasecmp(html->html.mime_type, "image/", 6)))
		html->html.mime_id = XmPLC_IMAGE;

	/* alpha channel stuff */
	html->html.alpha_buffer  = (AlphaPtr)NULL;

	if(!html->html.anchor_track_callback && !html->html.anchor_cursor &&
		!html->html.highlight_on_enter && !html->html.motion_track_callback &&
		!html->html.focus_callback && !html->html.losing_focus_callback)
		html->html.need_tracking = False;
	else
		html->html.need_tracking = True;

	/* verify plc timing intervals */
	CheckPLCIntervals(html);

	/* Misc. resources */
	html->html.gc = (TGC)NULL;

	/* set maximum amount of colors for this display (also creates the XCC) */
	CheckMaxColorSetting(html);

	/* Create the anchor cursor (if any) */
	if(html->html.anchor_display_cursor && !(html->html.anchor_cursor))
		CreateAnchorCursor(XmHTML(init));

	/* set cursor to None if we don't have to use or have a cursor */
	if(!html->html.anchor_display_cursor || !html->html.anchor_cursor)
		html->html.anchor_cursor = None;

	/* Select & initialize appropriate font cache */
	html->html.default_font = _XmHTMLSelectFontCache(html, True);

	/*****
	* if no width or height was specified, default to the width of 20 em
	* (TeX measure for average character width) in the default font and the
	* height of a single line. We need to do this check since the Core
	* initialize() method doesn't do it.
	*****/
#ifdef WITH_MOTIF
#   define XF(font) (font)
#else
#   define XF(font) ((XFontStruct *)((GdkFontPrivate *)font))
#endif
	if(Toolkit_Widget_Dim (html).width <= 0)
	{
		unsigned long value = 0;
		if(!(XGetFontProperty(XF (html->html.default_font->xfont), XA_QUAD_WIDTH, &value)))
		{
			XCharStruct ch;
			int dir, ascent, descent;
			XTextExtents(XF (html->html.default_font->xfont), "m", 1, &dir, &ascent, 
				&descent, &ch);
			value = (Cardinal)ch.width;
			/* sanity for non-ISO fonts */
			if(value <= 0)
				value = 16;
		}
		Toolkit_Widget_Dim (html).width = (Dimension)(20*(Dimension)value +
							      2*html->html.margin_width);
	}
	if(Toolkit_Widget_Dim (html).height <= 0)
		Toolkit_Widget_Dim (html).height = html->html.default_font->lineheight +
			2*html->html.margin_height;

	/*****
	* Now create all private TWidgets: drawing area and scrollbars.
	* We couldn't do this until we knew for sure the TWidget dimensions were
	* set; creation of the work_area uses them.
	*****/
	CreateHTMLWidget(html);

	/* Parse the raw HTML text */
	if(html_source)
	{
		html->html.source   = strdup(html_source);
		html->html.elements = _XmHTMLparseHTML(html, NULL, html_source, html);

		/* check for frames */
		html->html.nframes = _XmHTMLCheckForFrames(html, html->html.elements);

		/* and create them */
		if(!_XmHTMLCreateFrames(NULL, html))
		{
			html->html.frames = NULL;
			html->html.nframes = 0;
		}
		/* Trigger link callback */
		if(html->html.link_callback)
			_XmHTMLLinkCallback(html);

		/* do initial document markup */
		html->html.formatted = _XmHTMLformatObjects(NULL, NULL,
			XmHTML(init));

		/* check for possible delayed external imagemaps */
		_XmHTMLCheckImagemaps(html);
	}
	else
	{
		html->html.source = (String)NULL;
		html->html.elements = (XmHTMLObject*)NULL;
		html->html.nframes = 0;
		html->html.formatted = (XmHTMLObjectTable*)NULL;
	}

	/* reset scrollbars (this will also resize the work_area) */
	CheckScrollBars(html);

	/* Final step: add a palette if we must dither */
	if(html->html.map_to_palette != XmDISABLED)
		_XmHTMLAddPalette(html);

	_XmHTMLDebug(1, ("XmHTML.c: Initialize End.\n"));
}


/*****
* Name: 		_XmHTMLGetLineObject
* Return Type: 	void
* Description: 	get the object located at the given y position.
* In: 
*	html:		XmHTMLWidget 
*	y_pos:		current text y position.
* Returns:
*	located element.
*****/
static XmHTMLObjectTableElement
_XmHTMLGetLineObject(XmHTMLWidget html, int y_pos)
{
	register XmHTMLObjectTableElement tmp = NULL;

	/*
	* y_pos given must fall in the bounding box of an element.
	* We try to be a little bit smart here: 
	* If we have a paint engine end and it's y position is below the
	* requested position, walk forwards until we find a match.
	* If we have a paint engine start and it's y position is below the
	* requested position, walk forwards. If it's above the requested position,
	* walk backwards. We are always bound to find a matching element.
	*/
	if(html->html.paint_end || html->html.paint_start)
	{
		/* located above paint engine end, walk forwards */
		if(html->html.paint_end && html->html.paint_end->y < y_pos)
		{
			for(tmp = html->html.paint_end; tmp != NULL; tmp = tmp->next)
				if(y_pos >= tmp->y && y_pos < tmp->y + tmp->height)
					break;
		}
		/* not found or no paint engine end */
		else if(html->html.paint_start)
		{
			/* located above paint engine start, walk forwards */
			if(html->html.paint_start->y < y_pos)
			{
				for(tmp = html->html.paint_start; tmp != NULL; tmp = tmp->next)
					if(y_pos >= tmp->y && y_pos < tmp->y + tmp->height)
						break;
			}
			/* located under paint engine start, walk backwards */
			else
			{
				for(tmp = html->html.paint_start; tmp != NULL; tmp = tmp->prev)
					if(y_pos >= tmp->y && y_pos < tmp->y + tmp->height)
						break;
			}
		}
	}
	/* neither paint engine start or end */
	else
		for(tmp = html->html.formatted; tmp != NULL; tmp = tmp->next)
			if(y_pos >= tmp->y && y_pos < tmp->y + tmp->height)
				break;

	/* top or bottom element */
	if(tmp == NULL || tmp->prev == NULL)
	{
		/* bottom element */
		if(tmp == NULL)
			return(html->html.formatted);
		/* top element otherwise */
		return(NULL);
	}
	return((tmp->y > y_pos ? tmp->prev : tmp));
}

/*****
* Name: 		SetCurrentLineNumber
* Return Type: 	void
* Description: 	get & set the linenumber of the line at the top of the
*				working area.
* In: 
*	html:		XmHTMLWidget 
*	y_pos:		current text y position.
* Returns:
*	nothing, but the top_line field of the htmlRec is updated.
*****/
static void
SetCurrentLineNumber(XmHTMLWidget html, int y_pos)
{
	XmHTMLObjectTableElement tmp;

	_XmHTMLDebug(1, ("XmHTML.c: SetCurrentLineNumber, y_pos = %i\n", y_pos));

	if((tmp = _XmHTMLGetLineObject(html, y_pos)) != NULL)
	{

		_XmHTMLDebug(1, ("XmHTML.c: SetCurrentLineNumber, object found, "
			"y_pos = %i, linenumber = %i\n", tmp->y, tmp->line));

		/* set line number for the found object */
		html->html.top_line = tmp->line;
		/* 
		* If the current element has got more than one word in it, and these 
		* words span accross a number of lines, adjust the linenumber.
		*/
		if(tmp->n_words > 1 && tmp->words[0].y != tmp->words[tmp->n_words-1].y)
		{
			int i;
			for(i = 0 ; i < tmp->n_words && tmp->words[i].y < y_pos; i++);
			if(i != tmp->n_words)
				html->html.top_line = tmp->words[i].line;
		}
	}
	else
		html->html.top_line = 0;

	_XmHTMLDebug(1, ("XmHTML.c: SetCurrentLineNumber, top_line = %i\n",
		html->html.top_line));
}


/*****
* Name: 		Layout
* Return Type: 	void
* Description: 	main layout algorithm. 
*				computes text layout and configures the scrollbars.
*				Also does handles image recreation.
* In: 
*	html:		TWidget to layout
* Returns:
*	nothing
*****/
static void
Layout(XmHTMLWidget html)
{
	XmHTMLObjectTableElement curr_ele = NULL;

	_XmHTMLDebug(1, ("XmHTML.c: Layout Start\n"));

	/* set blocking flag */
	html->html.in_layout = True;

	/* remember current vertical position if we have been scrolled */
	if(html->html.scroll_y)
		curr_ele = _XmHTMLGetLineObject(html, html->html.scroll_y);

	/* make a resize request if we have to do auto-sizing in either direction */
	if(html->html.resize_width || html->html.resize_height)
		autoSizeWidget(html);
	else
		_XmHTMLComputeLayout(html);

	/* set new vertical scrollbar positions */
	if(curr_ele != NULL)
		html->html.scroll_y = curr_ele->y;
	else
		html->html.scroll_y = 0;

	/* configure the scrollbars, will also resize work_area */
	CheckScrollBars(html);

	html->html.in_layout = False;
	_XmHTMLDebug(1, ("XmHTML.c: Layout End\n"));
	return;
}

/*****
* Name: 		Resize
* Return Type: 	void
* Description: 	xmHTMLWidgetClass resize method.
* In: 
*	w:			resized TWidget.
* Returns:
*	nothing
*****/
static void 
Resize(TWidget w)
{
	Boolean do_expose;
	XmHTMLWidget html = XmHTML(w);
	int foo, vsb_width;
	Display *dpy;
	TWindow win;

	_XmHTMLDebug(1, ("XmHTML.c: Resize Start\n"));

	/* No needless resizing */
	if(!Toolkit_Widget_Is_Realized(w))
	{
		_XmHTMLDebug(1, ("XmHTML.c: Resize end, TWidget not realized.\n"));
		return;
	}

	if(html->html.in_layout)
	{
		_XmHTMLDebug(1, ("XmHTML.c: Resize end, layout flag is set.\n"));
		return;
	}

	GetScrollDim(html, &foo, &vsb_width);

	/* No change in size, return */
	if((Toolkit_Widget_Dim (html).height == html->html.work_height) && 
	   (Toolkit_Widget_Dim (html).width == (html->html.work_width + html->html.margin_width +
			vsb_width)))
	{
		_XmHTMLDebug(1, ("XmHTML.c: Resize End, no change in size\n"));
		return;
	}

	dpy = Toolkit_Display (html->html.work_area); 
	win = Toolkit_Widget_Window (html->html.work_area);

	/*
	* Check if we have to do layout and generate an expose event.
	* When the TWidget shrinks, X does not generate an expose event. 
	* We want to recompute layout and generate an expose event when the 
	* width changes.
	* When the height increases, we only want to generate a partial
	* exposure (this gets handled in Redisplay).
	*/
	do_expose = (Toolkit_Widget_Dim (html).width != (html->html.work_width + 
		html->html.margin_width + vsb_width));

	_XmHTMLDebug(1, ("XmHTML.c: Resize, new window dimensions: %ix%i.\n",
		Toolkit_Widget_Dim (html).width - html->html.margin_width, html->html.work_height));
	_XmHTMLDebug(1, ("XmHTML.c: Resize, generating expose event : %s.\n",
		(do_expose ? "yes" : "no")));

	/* Clear current visible text */
	if(do_expose)
	{
		/* 
		* save new height & width of visible area.
		* subtract margin_width once to minimize number of calcs in
		* the paint routines: every thing rendered starts at an x position
		* of margin_width.
		*/
		html->html.work_width = Toolkit_Widget_Dim (html).width - html->html.margin_width - 
			vsb_width;
		html->html.work_height= Toolkit_Widget_Dim (html).height;

		/* Recompute layout */
		Layout(html);

		/* Clear current text area and generate an expose event */
		Toolkit_Widget_Repaint (html);
	}
	/* change in height */
	else
	{
		/* 
		* Get new start & end points for the paint engine 
		* We have two cases: shrink or stretch. 
		* When stretched, we generate an exposure event for the added
		* area and let DrawRedisplay figure it out. If shrunk, adjust 
		* end point for the paint engine.
		*/

		/* Window has been stretched */
		if(html->html.work_height < Toolkit_Widget_Dim (html).height)
		{
			/* 
			* formatted_height has some formatting offsets in it. Need
			* to subtract them first.
			*/
			int max = html->html.formatted_height - html->html.margin_height - 
				html->html.default_font->xfont->descent;
			/*
			* If the stretch is so large that the entire text will fit
			* in the new window height, remove the scrollbars by resetting
			* the vertical scrollbar position.
			*/
			if(Toolkit_Widget_Dim (html).height > max)
				html->html.scroll_y = 0;

			/* save new height */
			html->html.work_height = Toolkit_Widget_Dim (html).height;

			/* reset scrollbars (this will also resize the work_area) */
			CheckScrollBars(html);

			/* 
			* just clear the entire area. Will generate a double exposure
			* but everything will be painted as it should.
			*/
			Toolkit_Widget_Repaint (html);
		}
		/* window has been shrunk */
		else
		{
			XmHTMLObjectTable *start, *end;
			int y; 

			/* get new y maximum */
			y = html->html.scroll_y + Toolkit_Widget_Dim (html).height;

			/* Starting point is end of previous stream */
			start = (html->html.paint_end == NULL ? html->html.formatted:
				html->html.paint_end);

			/* Walk backwards until we reach the desired height */
			for(end = start; end != NULL && y >= end->y; end = end->prev);

			/* save end point */
			html->html.paint_end = end;

			/* save new height */
			html->html.work_height = Toolkit_Widget_Dim (html).height;

			/* reset scrollbars (this will also resize the work_area) */
			CheckScrollBars(html);

			/* no need to paint */
		}
	}
	/* resize XmHTML's frame childs */
	if(html->html.nframes)
	{
		_XmHTMLDebug(1, ("XmHTML.c: Resize, calling ReconfigureFrames\n"));
		_XmHTMLReconfigureFrames(html);
	}

	SetScrollBars(html);

	_XmHTMLDebug(1, ("XmHTML.c: Resize End\n"));

	return;
}

/*****
* Name: 		PaintBackground
* Return Type: 	void
* Description:	update background with the given region
* In: 
*	html:		XmHTMLWidget for which to do background painting.
*	x,y:		origin of region to update
*	width,height: dimensions of region to update.
* Returns:
*	nothing.
* Note:
*	A simple and beautiful routine that does it's job perfectly!
*****/
static void
PaintBackground(XmHTMLWidget html, int x, int y, int width, int height)
{
	XGCValues values;
	unsigned long valuemask;
	Display *dpy;
	int tile_width, tile_height, x_dist, y_dist, ntiles_x, ntiles_y;
	int x_offset, y_offset, tsx, tsy;

	_XmHTMLDebug(1, ("XmHTML.c: PaintBackground start, x = %i, y = %i, "
		"width = %i, height = %i\n", x, y, width, height));

	/***** 
	* We need to figure out a correct starting point for the first
	* tile to be drawn (ts_[x,y]_origin in the GC).
	* We know the region to update. First we need to get the number of tiles
	* drawn so far. Since we want the *total* number of tiles drawn, we must 
	* add the scroll offsets to the region origin.
	*****/
	tile_width  = html->html.body_image->width;
	tile_height = html->html.body_image->height;

	_XmHTMLDebug(1, ("XmHTML.c: PaintBackground, tile width = %i, "
		"tile height = %i\n", tile_width, tile_height));

	x_dist = html->html.scroll_x + x;
	y_dist = html->html.scroll_y + y;

	ntiles_x = (int)(x_dist/tile_width);
	ntiles_y = (int)(y_dist/tile_height);

	_XmHTMLDebug(1, ("XmHTML.c: PaintBackground, no of full horizontal "
		"tiles: %i (x_dist = %i)\n", ntiles_x, x_dist));
	_XmHTMLDebug(1, ("XmHTML.c: PaintBackground, no of full vertical "
		"tiles  : %i (y_dist = %i)\n", ntiles_y, y_dist));
	/*
	* Now we know how many full tiles have been drawn, we can calculate
	* the horizontal and vertical shifts required to start tiling on a
	* tile boundary.
	*/
	x_offset = x_dist - ntiles_x * tile_width;
	y_offset = y_dist - ntiles_y * tile_height;

	_XmHTMLDebug(1, ("XmHTML.c: PaintBackground, computed horizontal "
		"offset: %i\n", x_offset));
	_XmHTMLDebug(1, ("XmHTML.c: PaintBackground, computed vertical "
		"offset  : %i\n", y_offset));
	/*
	* Now we can compute the x and y tile origins. Note that these can
	* be negative.
	*/
	tsx = x - x_offset;
	tsy = y - y_offset;

	_XmHTMLDebug(1, ("XmHTML.c: PaintBackground, computed horizontal tile "
		"origin: %i (x = %i)\n", tsx, x));
	_XmHTMLDebug(1, ("XmHTML.c: PaintBackground, computed vertical tile "
		"origin  : %i (y = %i)\n", tsy, y));

	dpy = Toolkit_Display (html->html.work_area);

#if WITH_MOTIF
	valuemask = GCTile | GCFillStyle | GCTileStipXOrigin | GCTileStipYOrigin;
	values.fill_style = FillTiled;
	values.tile = html->html.body_image->pixmap;
	values.ts_x_origin = tsx;
	values.ts_y_origin = tsy;

	XChangeGC(dpy, html->html.bg_gc, valuemask, &values);
#else
	gdk_gc_set_fill (html->html.bg_gc, GDK_TILED);
	gdk_gc_set_tile (html->html.bg_gc, html->html.body_image->pixmap);
	gdk_gc_set_ts_origin (html->html.bg_gc, tsx, tsy);
#endif
	
	/* a plain fillrect will redraw the background portion */
	Toolkit_Fill_Rectangle(dpy, Toolkit_Widget_Window(html->html.work_area),
			       html->html.bg_gc, x, y, width, height);

	_XmHTMLDebug(1, ("XmHTML.c: PaintBackground end\n"));
}

/*****
* Name: 		Refresh
* Return Type: 	void
* Description: 	main screen refresher: given an exposure rectangle, this
*				routine determines the proper paint engine start and end
*				points and calls the painter.
* In: 
*	html:		XmHTMLWidget id
*	x,y:		upper-left corner of exposure region
*	width:		width of exposure region
*	height:		height of exposure region
* Returns:
*	nothing.
*****/
static void
Refresh(XmHTMLWidget html, int x, int y, int width, int height)
{
	int x1, x2, y1, y2, dy;
	XmHTMLObjectTable *start, *end;

	_XmHTMLDebug(1, ("XmHTML.c: Refresh Start\n"));

	x1 = x;
	x2 = x1 + width;

	/*
	* Update background with the given region. Must check if body image
	* hasn't been delayed or is being loaded progressively or we will get
	* some funny effects...
	*/
	if(html->html.body_image && !ImageDelayedCreation(html->html.body_image) &&
		BodyImageLoaded(html->html.body_image->html_image))
		PaintBackground(html, x, y, width, height);

	/*
	* We add the fontheight to the height of the exposure region. This will
	* ensure that the line right under the exposure region is also redrawn
	* properly. Same for topmost position, but subtraction instead of addition.
	*/
	dy = html->html.default_font->lineheight;
	y1 = (y - dy > 0 ? y - dy : y);
	y2 = y1 + height + 1.5*dy;

	_XmHTMLDebug(1, ("XmHTML.c: Refresh, initial y1: %i, y2: %i\n", y1, y2));

	/* add vertical scroll and core offsets */
	y1 += html->html.scroll_y - Toolkit_Widget_Dim (html).y;
	y2 += html->html.scroll_y + Toolkit_Widget_Dim (html).y;

	_XmHTMLDebug(1, ("XmHTML.c: Refresh, using y1: %i, y2: %i (scroll_y = %i, "
		"core.y = %i)\n", y1, y2, html->html.scroll_y, Toolkit_Widget_Dim (html).y));

	/*
	* If the offset of the top of the exposed region is higher than
	* the max height of the text to display, the exposure region
	* is empty, so we just return here and leave everything untouched.
	*/
	if(y1 > html->html.formatted_height)
	{
		_XmHTMLDebug(1, ("XmHTML.c: Refresh End, y1 > maximum document "
			" height\n"));
		html->html.top_line = html->html.nlines;
		return;
	}

	/*
	* Get paint stream start & end for the obtained exposure region
	* We have to take the height of the object into account as well.
	* We try to be a little bit smart here.
	* paint_start == NULL is a valid stream command, so check it.
	*/
	start = (html->html.paint_start ? 
		html->html.paint_start : html->html.formatted);

	/* below current paint engine start */
	if(y1 > start->y)
	{
		/* already in region, get first object in it */
		if(y1 < (start->y + start->height))
		{
			_XmHTMLDebug(1, ("XmHTML.c: Refresh, walking bottom-up, "
				"y_start = %i\n", start->y));
			while(start && y1 > start->y && y1 < (start->y + start->height))
				start = start->prev;
		}
		/* below region, walk forward until we hit first object */
		else
		{
			_XmHTMLDebug(1, ("XmHTML.c: Refresh, walking bottom-down, "
				"y_start = %i\n", start->y));
			while(start)
			{
				if(y1 > start->y && y1 < (start->y + start->height))
					break;
				start = start->next;
			}
		}
	}
	/* above current paint engine start */
	else
	{
		_XmHTMLDebug(1, ("XmHTML.c: Refresh, walking top-up, "
			"y_start = %i\n", start->y));
		while(start && y1 <= start->y)
			start = start->prev;
		/* get first object with same y position */
		while(start && start->prev && start->y == start->prev->y)
			start = start->prev;
	}

	/* sanity check */
	if(start == NULL)
		start = html->html.formatted;
	end = start;

	/* get first point at bottom of exposed region */
	while(end && y2 > end->y)
		end = end->next;
	/* now walk to the last point still inside the region */
	while(end && y2 > end->y && y2 < (end->y + end->height))
		end = end->next;

	/* set proper paint engine start & end */
	html->html.paint_start = start;
	html->html.paint_end = end;

	/* Set horizontal painting positions */
	html->html.paint_x = x1 + html->html.scroll_x - Toolkit_Widget_Dim (html).x;
	html->html.paint_width = x2 + html->html.scroll_x + Toolkit_Widget_Dim (html).x;

	/* Set vertical painting positions */
	html->html.paint_y = y1;
	html->html.paint_height = y2;

	_XmHTMLDebug(1, ("XmHTML.c: Refresh, x1 = %i, x2 = %i\n", x1, x2));
	_XmHTMLDebug(1, ("XmHTML.c: Refresh, y1 = %i, y2 = %i\n", y1, y2));
	_XmHTMLDebug(1, ("XmHTML.c: Refresh, paint_start->x = %i, paint_start->y "
		"= %i\n", start->x, start->y));
#ifdef DEBUG
	if(end)
		_XmHTMLDebug(1, ("XmHTML.c: Refresh, paint_end->x = %i, paint_end->y "
			"= %i\n", end->x, end->y));
	else
		_XmHTMLDebug(1, ("XmHTML.c: Refresh, paint_end is NULL!\n"));
#endif

	if(html->html.gc == NULL)
		return;

	_XmHTMLDebug(1, ("XmHTML.c: Refresh, calling _XmHTMLPaint\n"));
	_XmHTMLPaint(html, html->html.paint_start, html->html.paint_end);

#if 0
	/* doesn't work yet */
	if(html->html.is_frame && html->html.frame_border)
		_XmHTMLDrawFrameBorder(html);
#endif

	/* display scrollbars */
	SetScrollBars(html);
}

/*****
* Name: 		ResetWidget
* Return Type: 	void
* Description: 	resets all non-persistent resources to their defaults
* In: 
*	html:		XmHTMLWidget id
*	free_img:	true when images should be freed. This will only be True
*				when the TWidget has received a new source.
* Returns:
*	nothing
*****/
static void
ResetWidget(XmHTMLWidget html, Boolean free_img)
{
	/* reset some important vars */
	html->html.anchors             = (XmHTMLWord*)NULL;
	html->html.anchor_words        = 0;
	html->html.named_anchors       = (XmHTMLObjectTable*)NULL;
	html->html.num_named_anchors   = 0;
	html->html.anchor_current_cursor_element = (XmHTMLAnchor*)NULL;
	html->html.armed_anchor        = (XmHTMLObjectTable*)NULL;
	html->html.current_anchor      = (XmHTMLObjectTable*)NULL;
	html->html.selected            = (XmHTMLAnchor*)NULL;
	html->html.selection           = (XmHTMLObjectTable*)NULL;
	html->html.select_start        = 0;
	html->html.select_end          = 0;
	html->html.scroll_x            = 0;
	html->html.scroll_y            = 0;
	html->html.formatted_width     = 1;
	html->html.formatted_height    = 1;
	html->html.paint_start         = (XmHTMLObjectTable*)NULL;
	html->html.paint_end           = (XmHTMLObjectTable*)NULL;
	html->html.paint_x             = 0;
	html->html.paint_y             = 0;
	html->html.paint_width         = 0;
	html->html.paint_height        = 0;
	html->html.scroll_x            = 0;
	html->html.scroll_y            = 0;
	html->html.top_line            = 0;
	/* fix 02/26/97-01, kdh */
	html->html.paint_start         = (XmHTMLObjectTable*)NULL;
	html->html.paint_end           = (XmHTMLObjectTable*)NULL;

	/* Reset all colors */
	html->html.body_fg             = html->html.body_fg_save;
	html->html.body_bg             = html->html.body_bg_save;
	html->html.anchor_fg           = html->html.anchor_fg_save;
	html->html.anchor_target_fg    = html->html.anchor_target_fg_save;
	html->html.anchor_visited_fg   = html->html.anchor_visited_fg_save;
	html->html.anchor_activated_fg = html->html.anchor_activated_fg_save;
	html->html.anchor_activated_bg = html->html.anchor_activated_bg_save;
	html->html.image_maps          = (XmHTMLImageMap*)NULL;

	/* and reset image stuff if it was freed */
	if(free_img)
	{
		/* must reset body_image as well since it also has been freed */
		html->html.body_image          = (XmHTMLImage*)NULL;
		html->html.images              = (XmHTMLImage*)NULL;
		html->html.body_image_url      = (String)NULL;
		html->html.alpha_buffer        = (AlphaPtr)NULL;
		/* only reset when we aren't dithering */
		if(html->html.map_to_palette == XmDISABLED)
			html->html.xcc                 = (XCC)NULL;
	}
}

/*****
* Name: 		FreeExpendableResources
* Return Type: 	void
* Description: 	frees all non-persistent resources of a TWidget
* In: 
*	html:		XmHTMLWidget id
*	free_img:	true when images should be freed. This will only be True
*				when the TWidget has received a new source.
* Returns:
*	nothing
*****/
static void
FreeExpendableResources(XmHTMLWidget html, Boolean free_img)
{
	/* Free anchor worddata */
	if(html->html.anchor_words)
		free(html->html.anchors);
	html->html.anchors = (XmHTMLWord*)NULL;

	/* Free named anchor data */
	if(html->html.num_named_anchors)
		free(html->html.named_anchors);
	html->html.named_anchors = (XmHTMLObjectTable*)NULL;

	/*****
	* Always free imagemaps, anchor data becomes invalid!!
	* (fix 09/17/97-02, kdh)
	*****/
	_XmHTMLFreeImageMaps(html);
	html->html.image_maps = (XmHTMLImageMap*)NULL;

	/* clear the images if we have to */
	if(free_img)
	{
		/* Free all images (also clears xcc & alpha channel stuff) */
		XmHTMLImageFreeAllImages((TWidget)html);

		/* must reset body_image as well since it also has been freed */
		html->html.body_image          = (XmHTMLImage*)NULL;
		html->html.images              = (XmHTMLImage*)NULL;
		html->html.delayed_creation    = False; /* no delayed image creation */
		html->html.alpha_buffer        = (AlphaPtr)NULL;
		/* only reset when we aren't dithering */
		if(html->html.map_to_palette == XmDISABLED)
			html->html.xcc                 = (XCC)NULL;
	}
	else
	{
		/*****
		* We need to orphan all images: the formatter will be called shortly
		* after this routine returns and as a result of that the owner
		* of each image will become invalid. Not orphanizing them would
		* lead to a lot of image copying.
		* Info structures with the XmIMAGE_DELAYED_CREATION bit need to
		* propagate this info to their parent, or chances are that alpha
		* channeling will *not* be redone when required.
		* Must not forget to set the global delayed_creation flag or nothing
		* will happen.
		*****/
		register XmHTMLImage *img;
		for(img = html->html.images; img != NULL; img = img->next)
		{
			img->owner = NULL;	/* safety */
			img->options |= IMG_ORPHANED;
			if(!ImageInfoFreed(img) &&
				ImageInfoDelayedCreation(img->html_image))
			{
				img->options |= IMG_DELAYED_CREATION;
				html->html.delayed_creation = True;
			}
		}
	}
	/* Free any allocated document colors */
	_XmHTMLFreeColors(html);
}

/*****
* Name: 		XmHTML_Destroy
* Return Type: 	void
* Description: 	Frontend indepentent destroy routine.
* In: 
*	w:			TWidget to destroy
* Returns:
*	nothing
*****/
static void 
XmHTML_Destroy(XmHTMLWidget html)
{
	_XmHTMLDebug(1, ("XmHTML.c: XmHTML_Destroy Start\n"));

	/* First kill any outstanding PLC's */
	_XmHTMLKillPLCCycler(html);

	/* Free list of parsed HTML elements */
	html->html.elements = _XmHTMLparseHTML(html, html->html.elements, NULL, NULL);

	/* Free list of formatted HTML elements */
	html->html.formatted = _XmHTMLformatObjects(html->html.formatted, 
		html->html.anchor_data, html);

	/* Free list of form data */
	_XmHTMLFreeForm(html, html->html.form_data);
	html->html.form_data = (XmHTMLFormData*)NULL;

	/* free all non-persitent resources and destroy the images */
	FreeExpendableResources(html, True);

	/* free the XColorContext (can be here if we are using a fixed palette) */
	XCCFree(html->html.xcc);

	/*****
	* Free list of frames. It is important that the images are destroyed
	* *before* the frames themselves get destroyed: frames can also have
	* images, and destroying the frame before destroying the image data
	* causes havoc. _XmHTMLDestroyFrames invokes XtDestroyWidget to destroy
	* each of XmHTML's frame childs, which invokes this routine and so on.
	*****/
	if(html->html.nframes)
		_XmHTMLDestroyFrames(html, html->html.nframes);

	/* free all fonts for this TWidget's instance */
	_XmHTMLUnloadFonts(html);

	/* free cursors */
	if(html->html.anchor_cursor != None)
		Toolkit_Free_Cursor (XtDisplay(w), html->html.anchor_cursor);

	/* Free GC's */
	if(html->html.gc)
		Toolkit_GC_Free (XtDisplay(html), html->html.gc);
	if(html->html.bg_gc)
		Toolkit_GC_Free (XtDisplay(html), html->html.bg_gc);

	_XmHTMLDebug(1, ("XmHTML.c: XmHTML_Destroy End\n"));
	return;
}

/*****
* Name: 		GetAnchor
* Return Type: 	XmHTMLWord*
* Description: 	determines if the given x and y positions are within the
*				bounding rectangle of an anchor.
* In: 
*	w:			HTML TWidget to check
*	x,y:		position to validate
* Returns:
*	A ptr. to the anchor data or NULL.
* Note:
*	anchor_words is an array that _only_ contains anchor data. Although
*	it requires a bit more memory, it's worth it since it will be a fast
*	lookup.
*****/
static XmHTMLWord*
GetAnchor(XmHTMLWidget html, int x, int y)
{
	XmHTMLWord *anchor_word = NULL;
	int ys, xs;
	register int i;

	xs = x + html->html.scroll_x;
	ys = y + html->html.scroll_y;

	for(i = 0 ; i < html->html.anchor_words; i++)
	{
		anchor_word = &(html->html.anchors[i]);
		if((xs >= anchor_word->x && xs<=(anchor_word->x+anchor_word->width)) &&
			(ys>=anchor_word->y && ys<=(anchor_word->y+anchor_word->height)))
		{
			_XmHTMLFullDebug(1, ("XmHTML.c: GetAnchor, anchor is: %s\n",
				anchor_word->owner->anchor->href));

			/* store line number */
			anchor_word->owner->anchor->line = anchor_word->line;
			return(anchor_word);
		}
	}
	_XmHTMLFullDebug(1, ("XmHTML.c: GetAnchor, no match found\n"));

	return(NULL);
}

/*****
* Name: 		GetImageAnchor
* Return Type: 	XmHTMLAnchor*
* Description: 	determines if the given x and y positions lie upon an image
*				that has an imagemap
* In: 
*	html:		HTML TWidget to check
*	x,y:		position to validate
* Returns:
*	A ptr. to the anchor data or NULL.
*****/
static XmHTMLAnchor*
GetImageAnchor(XmHTMLWidget html, int x, int y)
{
	XmHTMLImage *image = html->html.images;
	XmHTMLAnchor *anchor = NULL;
	int ys, xs;
	XmHTMLImageMap *imagemap = NULL;

	xs = x + html->html.scroll_x;
	ys = y + html->html.scroll_y;

	/* don't do this if we haven't got any imagemaps */
	if(html->html.image_maps == NULL)
		return(NULL);

	_XmHTMLFullDebug(1, ("XmHTML.c: GetImageAnchor, xs = %i, ys = %i\n",
		xs, ys));

	for(image = html->html.images; image != NULL; image = image->next)
	{
#ifdef DEBUG
		if(image->owner)
		{
			_XmHTMLFullDebug(1, ("XmHTML.c: GetImageAnchor, checking %s, "
				"x = %i, y = %i\n", image->url, image->owner->x,
				image->owner->y));
		}
		else
		{
			_XmHTMLFullDebug(1, ("XmHTML.c: GetImageAnchor, checking %s "
				"(no owner).", image->url));
		}
#endif
		if(image->owner && (xs >= image->owner->x && 
				xs <= (image->owner->x + image->owner->width)) &&
			(ys >= image->owner->y && 
				ys <= (image->owner->y + image->owner->height)))
		{
			if(image->map_type != XmMAP_NONE)
			{
				if(image->map_type == XmMAP_SERVER)
				{
					_XmHTMLWarning(__WFUNC__(html, "GetImageAnchor"),
						"server side imagemaps not supported yet.");
					return(NULL);
				}
				if((imagemap = _XmHTMLGetImagemap(html, image->map_url)) != NULL)
				{
					if((anchor = _XmHTMLGetAnchorFromMap(html, x, y, image, 
						imagemap)) != NULL)
					{
						_XmHTMLDebug(1, ("XmHTML.c: GetImageAnchor, anchor "
							"is: %s\n", anchor->href));
						return(anchor);
					}
				}
				
			}
		}
	}
	_XmHTMLFullDebug(1, ("XmHTML.c: GetImageAnchor, no match found\n"));

	return(NULL);
}

/*****
* Name: 		PaintAnchorUnselected
* Return Type: 	void
* Description:  paints the currently active anchor in an unactivated state.
*				_XmHTMLPaint will do the proper rendering.
* In: 
*	w:			HTML TWidget of which to unset the current anchor
* Returns:
*	nothing.
*****/
static void
PaintAnchorUnSelected(XmHTMLWidget html)
{
	XmHTMLObjectTable *start, *end;

	start = html->html.current_anchor;

	/* pick up the anchor end. An anchor ends when the raw worddata changes. */
	for (end = start; end != NULL && end->object == start->object; end = end->next)
		end->anchor_state = ANCHOR_UNSELECTED;

	_XmHTMLFullDebug(1, ("XmHTML.c: PaintAnchorUnselected, unselecting "
			"anchor: %s\n", start->anchor->href));

	/* paint it... */
	_XmHTMLPaint(html, start, end);

	/* ...and invalidate current selection */
	html->html.current_anchor = NULL;
}

/*****
* Name: 		PaintAnchorSelected
* Return Type: 	void
* Description:  paints the current active in an activated state.
*				_XmHTMLPaint will do the proper rendering.
* In: 
*	html:		HTML TWidget of which to set the current anchor
* Returns:
*	nothing.
*****/
static void
PaintAnchorSelected(XmHTMLWidget html, XmHTMLWord *anchor)
{
	XmHTMLObjectTable *start, *end;

	/* save as the current active anchor */
	start = html->html.current_anchor = anchor->owner;

	start = anchor->owner;

	/* pick up anchor end. */
	for(end = start; end != NULL && end->object == start->object; end = end->next)
		end->anchor_state = ANCHOR_SELECTED;

	_XmHTMLFullDebug(1, ("XmHTML.c: PaintAnchorSelected, selecting anchor: "
		"%s\n", start->anchor->href));

	/* paint it */
	_XmHTMLPaint(html, start, end);
}

/*****
* Name: 		LeaveAnchor
* Return Type: 	void
* Description:  remove anchor highlighting.
* In: 
*	html:		HTML TWidget of which to set the current anchor
* Returns:
*	nothing.
*****/
static void
LeaveAnchor(XmHTMLWidget html)
{
	XmHTMLObjectTable *start, *end;

	/* save as the current active anchor */
	start = html->html.armed_anchor;

	/* pick up the anchor end. */
	for(end = start; end != NULL && end->object == start->object; end = end->next)
		end->anchor_state = ANCHOR_UNSELECTED;

	_XmHTMLFullDebug(1, ("XmHTML.c: LeaveAnchor, leaving anchor: %s\n", start->anchor->href));

	/* paint it */
	_XmHTMLPaint(html, start, end);

	html->html.armed_anchor = NULL;
}

/*****
* Name: 		EnterAnchor
* Return Type: 	void
* Description: 	paints a highlight on the given anchor.
* In: 
*	html:		XmHTMLWidget id;
*	anchor:		anchor to receive highlighting.
* Returns:
*	nothing.
*****/
static void
EnterAnchor(XmHTMLWidget html, XmHTMLObjectTable *anchor)
{
	XmHTMLObjectTable *start, *end;

	/* save as the current active anchor */
	start = html->html.armed_anchor = anchor;

	/* pick up anchor end */
	for(end = start; end != NULL && end->object == start->object; end = end->next)
		end->anchor_state = ANCHOR_INSELECT;

	_XmHTMLFullDebug(1, ("XmHTML.c: EnterAnchor, entering anchor: %s\n", start->anchor->href));

	/* paint it */
	_XmHTMLPaint(html, start, end);
}

/*****
* Name: 		OnImage
* Return Type: 	XmHTMLImage*
* Description: 	checks whether the given positions fall within an image
* In: 
*	html:		XmHTMLWidget id
*	x:			pointer x-position
*	y:			pointer y-position
* Returns:
*	The selected image if a match was found, NULL if not.
*****/
static XmHTMLImage*
OnImage(XmHTMLWidget html, int x, int y)
{
	XmHTMLImage *image;
	int xs, ys;

	xs = x + html->html.scroll_x;
	ys = y + html->html.scroll_y;

	_XmHTMLDebug(1, ("XmHTML.c: OnImage, xs = %i, ys = %i\n", xs, ys));

	for(image = html->html.images; image != NULL; image = image->next)
	{
		if(image->owner && (xs >= image->owner->x && 
				xs <= (image->owner->x + image->owner->width)) &&
			(ys >= image->owner->y && 
				ys <= (image->owner->y + image->owner->height)))
		{
			_XmHTMLDebug(1, ("XmHTML.c: OnImage, image selected: %s\n",
				image->url));
			return(image);
		}
	}
	_XmHTMLDebug(1, ("XmHTML.c: OnImage, no match found\n"));
	return(NULL);
}

/*****
* Name: 		CheckMaxColorSetting
* Return Type: 	void
* Description: 	checks value of the XmNmaxImageColors resource against 
*				maximum number of colors allowed for this display.
* In: 
*	html:		XmHTMLWidget
* Returns:
*	nothing;
*****/
static void
CheckMaxColorSetting(XmHTMLWidget html)
{
	int max_colors;

	/* check for an XCC */
	if(html->html.xcc == NULL)
		_XmHTMLCheckXCC(html);
	
	/* get maximum allowable colors */
	max_colors = html->html.xcc->num_colors;

	/* limit to 256 colors */
	if(max_colors > MAX_IMAGE_COLORS)
		max_colors = MAX_IMAGE_COLORS;

	/* verify */
	if(html->html.max_image_colors > max_colors)
	{
		_XmHTMLWarning(__WFUNC__(html, "CheckMaxColorSetting"),
			"Bad value for XmNmaxImageColors: %i colors selected while "
			"display only\n    supports %i colors. Reset to %i",
			html->html.max_image_colors, max_colors, max_colors);
		html->html.max_image_colors = max_colors;
	}
	/* plop maximum colors in */
	else if(html->html.max_image_colors == 0)
		html->html.max_image_colors = max_colors;
}

/*****
* Name: 		CheckPLCIntervals
* Return Type: 	void
* Description: 	validates the delays for the PLC Cycler.
* In: 
*	html:		XmHTMLWidget id
* Returns:
*	nothing, but the PLC delay values can have changed when this function
*	returns.
*****/
static void
CheckPLCIntervals(XmHTMLWidget html)
{
	int delay, min_delay, max_delay, new_delay;
	Boolean delay_reset = False;

	delay = html->html.plc_delay;
	min_delay = html->html.plc_min_delay;
	max_delay = html->html.plc_max_delay;

	if(min_delay <= 0)
	{
		_XmHTMLWarning(__WFUNC__(html, "CheckPLCIntervals"),
			"Invalid value for XmNprogressiveMinimumDelay (%i)\n"
			"    Reset to %i", min_delay, PLC_MIN_DELAY);
		min_delay = PLC_MIN_DELAY;
	}

	if(delay < min_delay)
	{
		if(min_delay < PLC_DEFAULT_DELAY)
			new_delay = PLC_DEFAULT_DELAY;
		else
			new_delay = min_delay * 50;

		_XmHTMLWarning(__WFUNC__(html, "CheckPLCIntervals"),
			"Invalid value for XmNprogressiveInitialDelay (%i).\n"
			"    Set to %i", delay, new_delay);
		delay = new_delay;
		delay_reset = True;
	}

	if(max_delay <= min_delay)
	{
		new_delay = min_delay <= PLC_MAX_DELAY ?
					PLC_MAX_DELAY : min_delay * 100;

		_XmHTMLWarning(__WFUNC__(html, "CheckPLCIntervals"),
			"XmNprogressiveMaximumDelay (%i) less than "
			"XmNprogressiveMinimumDelay (%i).\n    Set to %i",
			max_delay, min_delay, new_delay);
		max_delay = new_delay;
	}

	/* can't do anything with this, reset to default values */
	if(max_delay <= delay && !delay_reset)
	{
		_XmHTMLWarning(__WFUNC__(html, "CheckPLCIntervals"),
			"XmNprogressiveMaximumDelay (%i) smaller than "
			"XmNprogressiveInitialDelay (%i).\n    Reset to default values",
			max_delay, min_delay);

		delay     = PLC_DEFAULT_DELAY;
		min_delay = PLC_MIN_DELAY;
		max_delay = PLC_MAX_DELAY;
	}

	html->html.plc_delay = html->html.plc_def_delay = delay;
	html->html.plc_min_delay = min_delay;
	html->html.plc_max_delay = max_delay;
}

/*****
* Name: 		_XmHTMLGetAnchorByName
* Return Type: 	XmHTMLObjectTableElement
* Description: 	returns the named anchor data.
* In: 
*	html:		XmHTMLWidget
*	anchor:		anchor to locate, with a leading hash sign.
* Returns:
*	anchor data upon success, NULL on failure.
*****/
XmHTMLObjectTableElement
_XmHTMLGetAnchorByName(XmHTMLWidget html, String anchor)
{
	XmHTMLObjectTableElement anchor_data;	
	int i;
	String chPtr = NULL;

	/* see if it is indeed a named anchor */
	if(!anchor || !*anchor || anchor[0] != '#')
		return(NULL);	/* fix 02/03/97-04, kdh */

	/* we start right after the leading hash sign */
	chPtr = &anchor[1];

	_XmHTMLDebug(1, ("XmHTML.c: _XmHTMLGetAnchorByName Start\n"));

	for(i = 0 ; i < html->html.num_named_anchors; i++)
	{
		anchor_data = &(html->html.named_anchors[i]);
		if(anchor_data->anchor && anchor_data->anchor->name &&  
			strstr(anchor_data->anchor->name, chPtr))
		{
			_XmHTMLDebug(1, ("XmHTML.c: _XmHTMLGetAnchorByName End, "
				"match found.\n"));
			return(anchor_data);
		}
	}
	_XmHTMLDebug(1, ("XmHTML.c: _XmHTMLGetAnchorByName End\n"));
	return(NULL);
}

/*****
* Name: 		_XmHTMLGetAnchorByValue
* Return Type: 	XmHTMLObjectTableElement
* Description: 	returns the named anchor data.
* In: 
*	w:			XmHTMLWidget
*	anchor_id:	internal anchor id.
* Returns:
*	anchor data upon success, NULL on failure.
*****/
XmHTMLObjectTableElement
_XmHTMLGetAnchorByValue(XmHTMLWidget html, int anchor_id)
{
	XmHTMLObjectTableElement anchor_data;	
	int i;

	_XmHTMLDebug(1, ("XmHTML.c: _XmHTMLGetAnchorByValue Start\n"));

	/* this should always match */
	anchor_data = &(html->html.named_anchors[anchor_id]);
	if(anchor_data->id == anchor_id)
		return(anchor_data);

	/* hmm, something went wrong, search the whole list of named anchors */
	_XmHTMLWarning(__WFUNC__(html, "_XmHTMLGetAnchorByValue"),
		"Misaligned anchor stack (id=%i), trying to recover.", anchor_id);

	for(i = 0 ; i < html->html.num_named_anchors; i++)
	{
		anchor_data = &(html->html.named_anchors[i]);
		if(anchor_data->id == anchor_id)
		{
			_XmHTMLDebug(1, ("XmHTML.c: _XmHTMLGetAnchorByValue End, "	
				"match found.\n"));
			return(anchor_data);
		}
	}
	_XmHTMLDebug(1, ("XmHTML.c: _XmHTMLGetAnchorByValue End\n"));
	return(NULL);
}

/*****
* Name: 		VerticalPosToLine
* Return Type: 	int
* Description: 	translates a vertical position to the current line number
*				in the currently displayed document.
* In: 
*	html:		XmHTMLWidget id;
*	y:			absolute document y position.
* Returns:
*	line number of the object found at the given position. nlines if not found.
*****/
static int
VerticalPosToLine(XmHTMLWidget html, int y)
{
	XmHTMLObjectTableElement tmp;

	/* sanity check */
	if(!html->html.formatted)
		return(0);

	if((tmp = _XmHTMLGetLineObject(html, y)) != NULL)
	{

		_XmHTMLDebug(1, ("XmHTML.c: VerticalPosToLine, object found, "
			"y_pos = %i, linenumber = %i\n", tmp->y, tmp->line));

		/* 
		* If the current element has got more than one word in it, and these 
		* words span accross a number of lines, adjust the linenumber.
		*/
		if(tmp->n_words > 1 && tmp->words[0].y != tmp->words[tmp->n_words-1].y)
		{
			int i;
			for(i = 0 ; i < tmp->n_words && tmp->words[i].y < y; i++);
			if(i != tmp->n_words)
				return(tmp->words[i].line);
			else
				return(tmp->line);
		}
		else
			return(tmp->line);
	}
	return(0);
}

/*****
* Name: 		ScrollToLine
* Return Type: 	void
* Description: 	scrolls the TWidget to the given line number.
* In: 
*	html:		XmHTMLWidget id
*	line:		line number to scroll to.
* Returns:
*	nothing.
*****/
static void
ScrollToLine(XmHTMLWidget html, int line)
{
	XmHTMLObjectTableElement tmp = NULL;

	if(line > html->html.nlines)
	{
		int value;

		_XmHTMLDebug(1, ("XmHTML.c: ScrollToLine, "
			"calling _XmHTMLMoveToPos\n"));

		html->html.top_line = html->html.nlines;
		value = html->html.formatted_height;

		/* fix 01/30/97-04, kdh */
		AdjustVerticalScrollValue(html->html.vsb, value);

		_XmHTMLMoveToPos(html->html.vsb, html, value);
		return;
	}
	if(line <= 0)
	{
		html->html.top_line = 0;
		_XmHTMLDebug(1, ("XmHTML.c: ScrollToLine, "
			"calling _XmHTMLMoveToPos\n"));
		_XmHTMLMoveToPos(html->html.vsb, html, 0);
		return;
	}

	for(tmp = html->html.formatted; tmp != NULL && line > tmp->line;
		tmp = tmp->next);

	/* get vertical position */
	if(tmp)
	{
		int i, value;	/* position to scroll to */

		/* we might have gone one object to far. Check and adjust */
		tmp = (line != tmp->line ? (tmp->prev ? tmp->prev : tmp) : tmp);

		value = tmp->y - tmp->height;
		html->html.top_line = tmp->line;

		/* 
		* Not exactly the requested line. Now check the line numbers of
		* the text inside this object. We need to subtract the height of this
		* object if we want to have it displayed properly.
		*/
		if(tmp->line != line)
		{
			if(tmp->n_words)
			{
				for(i = 0; i < tmp->n_words && line < tmp->words[i].line; 
					i++);
				/* if found, we need to take y position of the previous word */
				if(i != tmp->n_words && i != 0)
				{
					html->html.top_line = tmp->words[i-1].line;
					value = tmp->words[i-1].y - tmp->words[i-1].height;
				}
			}
		}
		_XmHTMLDebug(1, ("XmHTML.c: ScrollToLine, "
			"requested line: %i, lineno found: %i, y_pos: %i\n",
			line, tmp->line, value));

		/* fix 01/30/97-04, kdh */
		AdjustVerticalScrollValue(html->html.vsb, value);

		_XmHTMLMoveToPos(html->html.vsb, html, value);
	}
	else
	{
		_XmHTMLDebug(1, ("XmHTML.c: ScrollToLine, "
			"failed to detect requested line number!\n"));
	}
}

/*****
* Public Interfaces
*****/
/*****
* Name: 		XmHTMLAnchorGetId
* Return Type: 	int
* Description: 	returns the internal id of an anchor
* In: 
*	w:			XmHTMLWidget
*	anchor:		anchor to locate
* Returns:
*	the id upon success, -1 if not found.
*****/
int
XmHTMLAnchorGetId(TWidget w, String anchor)
{
	XmHTMLObjectTableElement anchor_data = NULL;

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLAnchorGetId");
		return(-1);
	}

	if((anchor_data = _XmHTMLGetAnchorByName(XmHTML (w), anchor)) != NULL)
		return(anchor_data->id);
	else /* not found */
		return(-1);
}

/*****
* Name: 		XmHTMLScrollToAnchorById
* Return Type: 	void
* Description: 	moves the text with the current anchor on top.
* In: 
*	w:			XmHTMLWidget
*	anchor_id:	internal anchor id to scroll to.
* Returns:
*	nothing.
*****/
void
XmHTMLAnchorScrollToId(TWidget w, int anchor_id)
{
	XmHTMLWidget html;
	XmHTMLObjectTableElement anchor_data = NULL;	

	/* sanity check */
	if(!w || !XmIsHTML(w) || anchor_id < 0)
	{
		_XmHTMLWarning(__WFUNC__(w, "XmHTMLAnchorScrollToId"),
			"%s passed to XmHTMLAnchorScrollToId.",
			w ? (anchor_id < 0 ? "Invalid id" : "Invalid parent") :
			"NULL parent");
		return;
	}

	html = XmHTML (w);

	/* only scroll when we have a vertical scrollbar */
	/* fix 10/22/97-01, kdh */
	if((anchor_data = _XmHTMLGetAnchorByValue(html, anchor_id)) != NULL &&
		html->html.needs_vsb)
	{
		int value;

		_XmHTMLDebug(1, ("XmHTML.c: XmHTMLAnchorScrollToId, "
			"calling _XmHTMLMoveToPos\n"));

		value = anchor_data->y - anchor_data->height;

		/* fix 01/30/97-04, kdh */
		AdjustVerticalScrollValue(html->html.vsb, value);

		_XmHTMLMoveToPos(html->html.vsb, html, value);
	}
}

/*****
* Name: 		XmHTMLAnchorScrollToName
* Return Type: 	void
* Description: 	moves the text with the current anchor on top.
* In: 
*	w:			XmHTMLWidget
*	anchor:		anchor to scroll to.
* Returns:
*	nothing.
*****/
void
XmHTMLAnchorScrollToName(TWidget w, String anchor)
{
	XmHTMLWidget html;
	XmHTMLObjectTableElement anchor_data = NULL;

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLAnchorScrollToName");
		return;
	}

	html = XmHTML (w);

	/* only scroll when we have a vertical scrollbar */
	/* fix 10/22/97-01, kdh */
	if((anchor_data = _XmHTMLGetAnchorByName(html, anchor)) != NULL &&
		html->html.needs_vsb)
	{
		int value;

		_XmHTMLDebug(1, ("XmHTML.c: XmHTMLAnchorScrollToName, "
			"calling _XmHTMLMoveToPos\n"));

		value = anchor_data->y - anchor_data->height;

		/* fix 01/30/97-04, kdh */
		AdjustVerticalScrollValue(html->html.vsb, value);

		_XmHTMLMoveToPos(html->html.vsb, html, value);
	}
	return;
}

/*****
* Name: 		XmHTMLTextScrollToLine
* Return Type: 	void
* Description: 	scrolls the TWidget to the given line number.
* In: 
*	w:			TWidget to scroll
*	line:		line number to scroll to.
* Returns:
*	nothing.
*****/
void
XmHTMLTextScrollToLine(TWidget w, int line)
{
	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLAnchorScrollToLine");
		return;
	}

	if(line == (XmHTML (w))->html.top_line)
		return;

	/* scroll to the requested line */
	ScrollToLine(XmHTML(w), line);
}

/*****
* Name: 		XmHTMLTextGetSource
* Return Type: 	String
* Description: 	returns a copy of the original, unmodified document.
* In: 
*	w:			XmHTMLWidget in question
* Returns:
*	a *pointer* to the original text, or NULL when w isn't a subclass of XmHTML
*	or there wasn't a current document.
*****/
String
XmHTMLTextGetSource(TWidget w)
{
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLTextGetSource");
		return(NULL);
	}

	return((XmHTML (w))->html.source);
}

/*****
* Name: 		XmHTMLTextGetString
* Return Type: 	String
* Description: 	composes a text buffer consisting of the parser output.
*				This return buffer is not necessarely equal to the original
*				document as the document verification and repair routines
*				are capable of modifying the original rather heavily.
* In: 
*	w:			XmHTMLWidget in question
* Returns:
*	An allocated buffer containing the composed text upon success, NULL on
*	failure.
* Note:
*	The return value from this function must be freed by the caller.
*	Typical use of this function is to set this buffer into the TWidget when
*	the parser failed to verify the document as it might well be that a next
*	parser pass on the original document does produce a HTML3.2 conforming
*	and verified document.
*****/
String
XmHTMLTextGetString(TWidget w)
{
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLTextGetString");
		return(NULL);
	}

	return(_XmHTMLTextGetString((XmHTML(w))->html.elements));
}

/*****
* Name: 		XmHTMLGetVersion
* Return Type: 	int
* Description: 	returns the version number of XmHTML
* In: 
*	nothing
* Returns:
*	version number of this library.
*****/
int 
XmHTMLGetVersion(void)
{
	return(XmHTMLVersion);
}

/*****
* Name: 		XmHTMLGetTitle
* Return Type: 	String
* Description: 	returns the value of the <title></title> element
* In: 
*	w:			XmHTMLWidget in question
* Returns:
*	value of the title upon success, NULL on failure.
*****/
String 
XmHTMLGetTitle(TWidget w)
{
	XmHTMLWidget html;
	XmHTMLObject *tmp;
	static String ret_val;
	String start, end;

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLGetTitle");
		return(NULL);
	}

	html = XmHTML(w);

	for(tmp = html->html.elements;
		tmp != NULL && tmp->id != HT_TITLE && tmp->id != HT_BODY;
		tmp = tmp->next);

	/* sanity check */
	if(!tmp || !tmp->next || tmp->id == HT_BODY)
		return(NULL);

	/* ok, we have reached the title element, pick up the text */
	tmp = tmp->next;

	/* another sanity check */
	if(!tmp->element)
		return(NULL);

	/* skip leading... */
	for(start = tmp->element; *start != '\0' && isspace(*start); start++);

	/* ...and trailing whitespace */
	for(end = &start[strlen(start)-1]; *end != '\0' && isspace(*end);
		end--);

	/* always backs up one to many */
	end++;

	/* sanity */
	if(*start == '\0' || (end - start) <= 0)
		return(NULL);

  	/* duplicate the title */
	ret_val = my_strndup(start, end - start);

	/* expand escape sequences */
	_XmHTMLExpandEscapes(ret_val, html->html.bad_html_warnings);

	/* and return to caller */
	return(ret_val);
}

/*****
* Name: 		XmHTMLImageFreeAllImages
* Return Type: 	void
* Description: 	releases all allocated images and associated structures
* In: 
*	html:		XmHTMLWidget for which to free all images
* Returns:
*	nothing
*****/
void 
XmHTMLImageFreeAllImages(TWidget w)
{
	XmHTMLImage *image, *image_list;
	Display *dpy;
	XmHTMLWidget html;

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLImageFreeAllImages");
		return;
	}

	html = XmHTML (w);
 	image_list = html->html.images;
 	dpy = Toolkit_Display(w);

	while(image_list != NULL)
	{
		image = image_list->next;
		_XmHTMLFreeImage(html, image_list);
		image_list = NULL;
		image_list = image;
	}
	html->html.images = NULL;

	/* alpha channel stuff */
	if(html->html.alpha_buffer)
	{
		if(html->html.alpha_buffer->ncolors)
			free(html->html.alpha_buffer->bg_cmap);
		free(html->html.alpha_buffer);
	}
	html->html.alpha_buffer = (AlphaPtr)NULL;

	/* only release XCC when we aren't using a fixed palette */
	if(html->html.map_to_palette == XmDISABLED)
	{
		XCCFree(html->html.xcc);
		html->html.xcc = (XCC)NULL;
	}
}

/*****
* Name: 		XmHTMLImageAddImageMap
* Return Type: 	void
* Description: 	add the given imagemap to a HTML TWidget
* In: 
*	w:			TWidget
*	image_map:	raw html data containing the imagemap to parse.
* Returns:
*	nothing
*****/
void
XmHTMLImageAddImageMap(TWidget w, String image_map)
{
	XmHTMLWidget html;
	XmHTMLObject *parsed_map, *temp;
	XmHTMLImageMap *imageMap = NULL;

	/* sanity check */
	if(!w || !XmIsHTML(w) || image_map == NULL)
	{
		_XmHTMLWarning(__WFUNC__(w, "XmHTMLImageAddImageMap"),
			"%s passed to XmHTMLImageAddImageMap.",
			w ? (image_map ? "Invalid parent" : "NULL imagemap") :
			"NULL parent");
		return;
	}

	html = XmHTML (w);

	/* parse the imagemap */
	if((parsed_map = _XmHTMLparseHTML(html, NULL, image_map, NULL)) == NULL)
		return;

	for(temp = parsed_map; temp != NULL; temp = temp->next)
	{
		switch(temp->id)
		{
			case HT_MAP:
				if(temp->is_end)
				{
					_XmHTMLStoreImagemap(html, imageMap);
					imageMap = NULL;
				}
				else
				{
					String chPtr;

					chPtr = _XmHTMLTagGetValue(temp->attributes, "name");
					if(chPtr != NULL)
					{
						imageMap = _XmHTMLCreateImagemap(chPtr);
						free(chPtr);
					}
					else
						_XmHTMLWarning(__WFUNC__(w, "XmHTMLAddImagemap"),
							"unnamed map, ignored (line %i in input).",
							temp->line);
				}
				break;

			case HT_AREA:
				if(imageMap)
					_XmHTMLAddAreaToMap(html, imageMap, temp);
				else
					_XmHTMLWarning(__WFUNC__(w, "XmHTMLAddImagemap"),
						"<AREA> element outside <MAP>, ignored "
						"(line %i in input).", temp->line);
				break;
			default:
				break;
		}
	}
	/* free the parsed imagemap data */
	(void)_XmHTMLparseHTML(html, parsed_map, NULL, NULL);
}

/*****
* Name: 		XmHTMLRedisplay
* Return Type: 	void
* Description: 	forces a layout recomputation of the currently loaded document
*				and triggers a redisplay.
* In: 
*	w:			TWidget for which to redo layout computation.
* Returns:
*	nothing
* Note:
*	This function is mostly useful in combination with the image updating
*	and/or replacing routines.
*****/
void
XmHTMLRedisplay(TWidget w)
{
	XmHTMLWidget html;

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLRedisplay");
		return;
	}

	html = XmHTML (w);

	/* recompute screen layout */
	Layout(html);

	if(html->html.gc != NULL)
		XmHTML_Frontend_Redisplay (html);
}

/*****
* Name: 		XmHTMLImageUpdate
* Return Type: 	XmImageStatus
* Description: 	updates an image
* In: 
*	w:			XmHTMLWidget
*	image:		image info representing the image to be updated.
*				This must be an XmImageInfo structure *known* to XmHTML.
* Returns:
*	XmIMAGE_ALMOST if updating this image requires a recomputation of the
*	document layout, XmIMAGE_OK if not and some other value upon error.
*****/
XmImageStatus
XmHTMLImageUpdate(TWidget w, XmImageInfo *image)
{
	XmHTMLWidget html;
	XmHTMLObjectTableElement temp;
	static String func = "XmHTMLImageUpdate";
	Boolean is_body_image;
	XmImageStatus status;

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, func);
		return(XmIMAGE_ERROR);
	}

	if(image == NULL)
	{
		_XmHTMLWarning(__WFUNC__(w, func), "%s called with a NULL image "
			"argument.", func);
		return(XmIMAGE_BAD);
	}

	html = XmHTML (w);

	/* do we already have the body image? */
	is_body_image = html->html.body_image != NULL;

	/* return error code if failed */
	if((status = _XmHTMLReplaceOrUpdateImage(html, image, NULL, &temp))
		!= XmIMAGE_OK)
		return(status);

	if(temp != NULL)
	{
		int xs, ys;
		xs = temp->x - html->html.scroll_x;
		ys = temp->y - html->html.scroll_y;
		/* We may paint the image, but we only do it when it's visible */
		if(!(xs + temp->width < 0 || xs > html->html.work_width || 
			ys + temp->height < 0 || ys > html->html.work_height))
		{
			_XmHTMLDebug(1, ("XmHTML.c: XmHTMLImageUpdate, painting image "
				"%s\n", image->url));

			/* clear the current image, don't generate an exposure */
			Toolkit_Clear_Area (XtDisplay(html->html.work_area), 
					    Toolkit_Widget_Window(html->html.work_area),
					    xs, ys, temp->width, temp->height);
			/* put up the new image */
			_XmHTMLPaint(html, temp, temp->next);
			Toolkit_Flush (Toolkit_Display((TWidget)html), True);
		}
	}
	else
	{
		/* if we've updated the body image, plug it in */
		if(!is_body_image && html->html.body_image != NULL)
		{
			Toolkit_Widget_Force_Repaint (html);
		}
	}
	return(XmIMAGE_OK);
}

/*****
* Name: 		XmHTMLImageReplace
* Return Type: 	XmImageStatus
* Description: 	replaces the XmImageInfo structure with a new one
* In: 
*	w:			XmHTMLWidget
*	image:		XmImageInfo structure to be replaced, must be known by XmHTML
*	new_image:	new XmImageInfo structure 
* Returns:
*	XmIMAGE_ALMOST if replacing this image requires a recomputation of the
*	document layout, XmIMAGE_OK if not and some other value upon error.
*****/
XmImageStatus
XmHTMLImageReplace(TWidget w, XmImageInfo *image, XmImageInfo *new_image)
{
	XmHTMLWidget html;
	XmHTMLObjectTableElement temp;
	XmImageStatus status;
	Boolean is_body_image;
	static String func = "XmHTMLImageReplace";

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, func);
		return(XmIMAGE_ERROR);
	}

	/* sanity */
	if(image == NULL || new_image == NULL)
	{
		_XmHTMLWarning(__WFUNC__(w, func), "%s called with a NULL %s "
			"argument", func, (image == NULL ? "image" : "new_image"));
		return(XmIMAGE_BAD);
	}
	html = XmHTML (w);

	/* do we already have the body image? */
	is_body_image = html->html.body_image != NULL;

	if((status = _XmHTMLReplaceOrUpdateImage(html, image, new_image, &temp))
		!= XmIMAGE_OK)
		return(status);

	if(temp != NULL)
	{
		int xs, ys;
		xs = temp->x - html->html.scroll_x;
		ys = temp->y - html->html.scroll_y;
		/* We may paint the image, but we only do it when it's visible */
		if(!(xs + temp->width < 0 || xs > html->html.work_width || 
			ys + temp->height < 0 || ys > html->html.work_height))
		{
			_XmHTMLDebug(1, ("XmHTML.c: XmHTMLImageReplace, painting image "
				"%s\n", image->url));
			/* clear the current image, don't generate an exposure */
			Toolkit_Clear_Area(XtDisplay(html->html.work_area), 
				Toolkit_Widget_Window(html->html.work_area), xs, ys,
				temp->width, temp->height);
			/* put up the new image */
			_XmHTMLPaint(html, temp, temp->next);
			Toolkit_Flush(Toolkit_Display ((TWidget)html), True);
		}
	}
	else
	{
		/* if we've replaced the body image, plug it in */
		if(!is_body_image && html->html.body_image != NULL)
		{
			Toolkit_Widget_Force_Repaint (html);
		}
	}
	return(XmIMAGE_OK); 
}

/*****
* Name: 		XmHTMLImageFreeImageInfo
* Return Type: 	void
* Description: 	free the given XmImageInfo structure
* In: 
*	info:		image to free
* Returns:
*	nothing
*****/
void
XmHTMLImageFreeImageInfo(TWidget w, XmImageInfo *info)
{
	static String func = "XmHTMLImageFreeImageInfo";

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, func);
		return;
	}

	/* sanity check */
	if(info == NULL)
	{
		_XmHTMLWarning(__WFUNC__(NULL, func), "NULL XmImageInfo "
			"structure passed to %s.", func);
		return;
	}

	_XmHTMLFreeImageInfo(XmHTML (w), info, True);
}

/*****
* Name: 		XmHTMLGetImageType
* Return Type: 	int
* Description: 	determines the type of a given image
* In: 
*	file:		name of image file to check
* Returns:
*	the image type if supported, IMAGE_UNKNOWN otherwise.
*****/
unsigned char
XmHTMLImageGetType(String file, unsigned char *buf, int size)
{
	ImageBuffer data, *dp = NULL;
	Byte ret_val = IMAGE_UNKNOWN;

	if(!file)
		return(IMAGE_ERROR);

	if(size == 0 || buf == NULL)
	{
		if((dp = _XmHTMLImageFileToBuffer(file)) == NULL)
			return(IMAGE_ERROR);
	}
	else
	{
		if(buf != NULL && size != 0)
		{
			data.file = file;
			data.buffer = (Byte*)buf;
			data.size = (size_t)size;
			data.next = 0;
			data.may_free = False;
			dp = &data;
		}
		else
			return(IMAGE_ERROR);
	}

	ret_val = _XmHTMLGetImageType(dp);

	FreeImageBuffer(dp);

	return(ret_val);
}

Boolean
XmHTMLImageJPEGSupported(void)
{
#ifdef HAVE_JPEG
	return(True);
#else
	return(False);
#endif
}

Boolean
XmHTMLImagePNGSupported(void)
{
#ifdef HAVE_PNG
	return(True);
#else
	return(False);
#endif
}

Boolean
XmHTMLImageGZFSupported(void)
{
#if defined(HAVE_PNG) || defined(HAVE_ZLIB)
	return(True);
#else
	return(False);
#endif
}

/*****
* Name: 		XmHTMLFrameGetChild
* Return Type: 	TWidget
* Description: 	returns the TWidget id of a frame child given it's name.
* In: 
*	w:			XmHTMLWidget
*	name:		name of frame to locate.
* Returns:
*	If found, the TWidget id of the requested frame, NULL otherwise. 
*****/
TWidget
XmHTMLFrameGetChild(TWidget w, String name)
{
	XmHTMLWidget html;
	int i;

	/* sanity check */
	if(!w || !XmIsHTML(w) || name == NULL)
	{
		_XmHTMLWarning(__WFUNC__(w, "XmHTMLFrameGetChild"),
			"%s passed to XmHTMLFrameGetChild.",
			w ? (name == NULL ? "NULL frame name" : "Invalid parent") :
			"NULL parent");
		return(NULL);
	}

	html = XmHTML (w);

	for(i = 0; i < html->html.nframes; i++)
	{
		if(!(strcmp(html->html.frames[i]->name, name)))
			return(html->html.frames[i]->frame);
	}
	return(NULL);
}

/*****
* Name: 		XmHTMLTextSetString
* Return Type: 	void
* Description: 	sets the given text into the given HTML TWidget
* In: 
*	w:			XmHTMLWidget in question
*	value:		text to set
* Returns:
*	clears any previous text and sets the new text.
*****/
void
XmHTMLTextSetString(TWidget w, String text)
{
	XmHTMLWidget html;

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLTextSetString");
		return;
	}

	_XmHTMLDebug(1, ("XmHTML.c: XmHTMLSetTextString, start\n"));

	html = XmHTML (w);

	/* almost impossible */
	if(html->html.value == text)
		return;

	/* check if the new value is different from the current source */
	if(text && html->html.source && !(strcmp(html->html.source, text)))
		return;

	/* First kill any outstanding PLC's */
	_XmHTMLKillPLCCycler(html);

	/* now destroy any forms */
	_XmHTMLFreeForm(html, html->html.form_data);
	html->html.form_data = (XmHTMLFormData*)NULL;

	/* clear the current display area. Prevents color flashing etc. */
	if(html->html.gc != NULL)
	{
		Toolkit_Clear_Area (XtDisplay(html->html.work_area), 
				    Toolkit_Widget_Window(html->html.work_area), 0, 0, 
				    Toolkit_Widget_Dim (html).width,
				    Toolkit_Widget_Dim (html).height);
	}

	/* clear current source */
	if(html->html.source)
	{
		free(html->html.source);
		html->html.source = NULL;
		html->html.value  = NULL;
	}

	/* set new source text */
	if(text)
	{
		html->html.source = strdup(text);
		html->html.value = html->html.source;
	}

	/* destroy any existing frames */
	if(html->html.nframes)
		_XmHTMLDestroyFrames(html, html->html.nframes);

	/* free all non-persistent resources and images */
	FreeExpendableResources(html, True);

	/* reset some important vars */
	ResetWidget(html, True);

	/* Parse the raw HTML text */
	html->html.elements = _XmHTMLparseHTML(html, html->html.elements, 
		html->html.source, html);

	/* Trigger link callback */
	if(html->html.link_callback)
		_XmHTMLLinkCallback(html);

	/* reset topline */
	html->html.top_line = 0;

	/* check for frames */
	html->html.nframes = _XmHTMLCheckForFrames(html, html->html.elements);

#ifdef WITH_MOTIF
	/* set appropriate background color */
	XtVaSetValues(html->html.work_area, 
		XmNbackground, html->html.body_bg, NULL);
#else
	fprintf (stderr, "SHOULD SET THE BACKGROUND COLOR\n");
#endif
	/* get new values for top, bottom & highlight */
	_XmHTMLRecomputeColors(html);

	/* create frames */
	if(!_XmHTMLCreateFrames(NULL, html))
	{
		html->html.frames = NULL;
		html->html.nframes = 0;
		/* keep current frame setting */
	}

	/* do initial markup */
	html->html.formatted = _XmHTMLformatObjects(html->html.formatted,
			html->html.anchor_data, html);

	/* check for delayed external imagemaps */
	_XmHTMLCheckImagemaps(html);

	/* compute new screen layout */
	Layout(html);

	/* and clear the display, causing an Expose event */
	if(html->html.gc != NULL)
		Toolkit_Widget_Repaint (html);

	/* and start up the PLC cycler */
	html->html.plc_suspended = False;
#ifdef WITH_MOTIF
	_XmHTMLPLCCycler((XtPointer)html, None);
#else
	_XmHTMLPLCCycler((XtPointer)html);
#endif
}

/*****
* Name: 		XmHTMLXYToInfo
* Return Type: 	XmHTMLInfoStructure*
* Description: 	Retrieves the contents of an image and/or anchor at the
*				given cursor position.
* In: 
*	w:			XmHTMLWidget id;
*	x:			x-location of pointer, relative to left side of the workArea
*	y:			y-location of pointer, relative to top side of the workArea
* Returns:
*	A filled XmHTMLInfoStructure when the pointer was pressed on an image
*	and/or anchor. NULL if not.
* Note:
*	The return value, nor one of its members may be freed by the caller.
*****/
XmHTMLInfoPtr
XmHTMLXYToInfo(TWidget w, int x, int y)
{
	static XmHTMLInfoStructure cbs;
	static XmHTMLImage *image;
	static XmHTMLAnchorCallbackStruct anchor_cbs;
	static XmImageInfo info;
	long line = -1;
	XmHTMLAnchor *anchor;
	XmHTMLWord *anchor_word;
	XmHTMLWidget html;

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLXYToInfo");
		return(NULL);
	}

	html = XmHTML (w);

	/* default fields */
	cbs.x      = x;
	cbs.y      = y;
	cbs.is_map = XmMAP_NONE;
	cbs.image  = NULL;
	cbs.anchor = NULL;
	line = -1;

	/* pick up a possible anchor or imagemap location */
	anchor = NULL;
			
	if((anchor_word = GetAnchor(html, x, y)) == NULL)
		anchor = GetImageAnchor(html, x, y);

	/* no regular anchor, see if it's an imagemap */
	if(anchor == NULL && anchor_word)
		anchor = anchor_word->owner->anchor;

	/*
	* Final check: if this anchor is a form component it can't be followed 
	* as this is an internal-only anchor.
	*/
	if(anchor && anchor->url_type == ANCHOR_FORM_IMAGE)
		anchor = NULL;

	/* check if we have an anchor */
	if(anchor != NULL)
	{
		/* set to zero */
		(void)memset(&anchor_cbs, 0, sizeof(XmHTMLAnchorCallbackStruct));

		/* initialize callback fields */
		anchor_cbs.reason   = XmCR_ACTIVATE;
		anchor_cbs.event    = NULL;
		anchor_cbs.url_type = anchor->url_type;
		anchor_cbs.line     = anchor->line;
		anchor_cbs.href     = anchor->href;
		anchor_cbs.target   = anchor->target;
		anchor_cbs.rel      = anchor->rel;
		anchor_cbs.rev      = anchor->rev;
		anchor_cbs.title    = anchor->title;
		anchor_cbs.doit     = False;
		anchor_cbs.visited  = anchor->visited;
		cbs.anchor = &anchor_cbs;
		line       = anchor->line;
	}

	/* check if we have an image */
	if((image = OnImage(html, x, y)) != NULL)
	{
		/* set map type */
		cbs.is_map = (image->map_type != XmMAP_NONE);

		if(image->html_image != NULL)
		{
			/* no image info if this image is being loaded progressively */
			if(!ImageInfoProgressive(image->html_image))
			{
				/* use real url but link all other members */
				info.url          = image->url;
				info.data         = image->html_image->data;
				info.clip         = image->html_image->clip;
				info.width        = image->html_image->width;
				info.height       = image->html_image->height;
				info.reds         = image->html_image->reds;
				info.greens       = image->html_image->greens;
				info.blues        = image->html_image->blues;
				info.bg           = image->html_image->bg;
				info.ncolors      = image->html_image->ncolors;
				info.options      = image->html_image->options;
				info.type         = image->html_image->type;
				info.depth        = image->html_image->depth;
				info.colorspace   = image->html_image->colorspace;
				info.transparency = image->html_image->transparency;
				info.swidth       = image->html_image->swidth;
				info.sheight      = image->html_image->sheight;
				info.scolors      = image->html_image->scolors;
				info.alpha        = image->html_image->alpha;
				info.fg_gamma     = image->html_image->fg_gamma;
				info.x            = image->html_image->x;
				info.y            = image->html_image->y;
				info.loop_count   = image->html_image->loop_count;
				info.dispose      = image->html_image->dispose;
				info.timeout      = image->html_image->timeout;
				info.nframes      = image->html_image->nframes;
				info.frame        = image->html_image->frame;
				info.user_data    = image->html_image->user_data;
				/* set it */
				cbs.image = &info;
			}
		}
		else
		{
			/* XmImageInfo has been freed, construct one */
			/* set to zero */
			memset(&info, 0, sizeof(XmImageInfo));
			/* fill in the fields we know */
			info.url     = image->url;
			info.type    = IMAGE_UNKNOWN;
			info.width   = image->swidth;
			info.height  = image->sheight;
			info.swidth  = image->width;
			info.sheight = image->height;
			info.ncolors = image->npixels;
			info.nframes = image->nframes;
			/* set it */
			cbs.image     = &info;
		}
		if(line == -1)
			line = (image->owner ? image->owner->line : -1);
	}
	/* no line number yet, get one */
	if(line == -1)
		cbs.line = VerticalPosToLine(html, y + html->html.scroll_y);
	else
		cbs.line = line;
	return(&cbs);
}

/*****
* Name:			XmHTMLTextGetFormatted
* Return Type: 	String
* Description: 	returns a formatted copy of the current document.
* In: 
*	w:			XmHTMLWidget id;
*	papertype:	type of paper to use (any of the XmHTMLTEXT_PAPERSIZE enums);
*	papersize:	size of paper for custom stuff, or default overrides;
*	type:		type of output wanted, plain, formatted or PS;
*	PSoptions:	options to use when creating postscript output.
* Returns:
*	a string which needs to be freed by the caller.
*****/
String
XmHTMLTextGetFormatted(TWidget w, unsigned char papertype,
	XmHTMLPaperSize *paperdef, unsigned char type, unsigned char PSoptions)
{
	XmHTMLWidget html;
#if 0
	XmHTMLPaperSize *pdef, pbase;
#endif

	/* sanity check */
	if(!w || !XmIsHTML(w))
	{
		_XmHTMLBadParent(w, "XmHTMLTextGetFormatted");
		return(NULL);
	}

	/*****
	* Check args: we only allow a papersize of XmNONE for plain and formatted
	* output. PS requires a papersize.
	*****/
	if(paperdef == NULL && type == XmHTMLTEXT_POSTSCRIPT)
	{
		_XmHTMLWarning(__WFUNC__(w, "XmHTMLTextGetFormatted"),
			"Formatted text output: postscript requires a papertype.");
		return(NULL);
	}
	/* custom papersize requires a paper definition. */
	if(papertype == XmHTMLTEXT_PAPERSIZE_CUSTOM && paperdef == NULL)
	{
		_XmHTMLWarning(__WFUNC__(w, "XmHTMLTextGetFormatted"),
			"Formatted text output: custom papersize misses a "
			"papersize definition.");
		return(NULL);
	}

	/* TWidget ptr */
	html = XmHTML (w);

#if 0
	/*****
	* get appropriate papersize definitions if not given.
	*****/
	if(papertype != XmHTMLTEXT_PAPERSIZE_CUSTOM && paperdef == NULL)
	{
		/* formatting routines use point size */
		if(papertype == XmHTMLTEXT_PAPERSIZE_A4)
		{
			pbase.unit_type     = XmHTML_POINT;
			pbase.paper_type    = XmHTMLTEXT_PAPERSIZE_A4;
			pbase.width         = 845;	/* 297mm */
			pbase.height        = 597;	/* 210mm */
			pbase.left_margin   = 57;	/* 20mm  */
			pbase.right_margin  = 57;
			pbase.top_margin    = 57;
			pbase.bottom_margin = 57;
		}
		else 	/* XmHTMLTEXT_PAPERSIZE_LETTER */
		{
			pbase.unit_type     = XmHTML_POINT;
			pbase.paper_type    = XmHTMLTEXT_PAPERSIZE_LETTER;
			pbase.width         = 795;	/* 11in  */
			pbase.height        = 614;	/* 8.5in */
			pbase.left_margin   = 65;	/* 0.9in */
			pbase.right_margin  = 65;
			pbase.top_margin    = 65;
			pbase.bottom_margin = 51;	/* 0.7in */
		}
		/* convert to correct output type */
		pdef = _XmHTMLTextCheckAndConvertPaperDef(html, &paperdef, type);
	}
	else	/* check validity of paper definition and convert to correct type */
		if((pdef = _XmHTMLTextCheckAndConvertPaperDef(html, paperdef,
			type)) == NULL)
			return(NULL);

	if(type == XmHTMLTEXT_PLAIN) 
		return(_XmHTMLTextGetPlain(html, pdef, html->html.formatted, NULL, 0));
	else if(type == XmHTMLTEXT_FORMATTED)
		return(_XmHTMLTextGetFormatted(html, pdef, html->html.formatted, NULL,
			0));
	else if(type == XmHTMLTEXT_POSTSCRIPT) 
		return(_XmHTMLTextGetPS(html, pdef, html->html.formatted, NULL,
			PSoptions));
	else
		_XmHTMLWarning(__WFUNC__(w, "XmHTMLTextGetFormatted"),
			"Formatted text output: Invalid type selected.");
#endif
	return(NULL);
}
