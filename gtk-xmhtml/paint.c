#ifndef lint
static char rcsId[]="$Header$";
#endif
/*****
* paint.c : XmHTML rendering routines
*
* This file Version	$Revision$
*
* Creation date:		Fri Dec  6 19:50:20 GMT+0100 1996
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
* Revision 1.5  1997/12/26 21:03:34  sopwith
* A few miscellaneous XmHTML bug fixes, including a note to miguel so he can fix frames ;-)
*
* Revision 1.4  1997/12/25 01:38:29  unammx
* Small bug fixes
*
* Revision 1.3  1997/12/25 01:34:13  unammx
* Good news for the day:
*
*    I have upgraded our XmHTML sources to XmHTML 1.1.1.
*
*    This basically means that we got table support :-)
*
* Still left to do:
*
*    - Set/Get gtk interface for all of the toys in the widget.
*    - Frame support is broken, dunno why.
*    - Form support (ie adding widgets to it)
*
* Miguel.
*
* Revision 1.2  1997/12/11 21:20:23  unammx
* Step 2: more gtk/xmhtml code, still non-working - mig
*
* Revision 1.1  1997/11/28 03:38:58  gnomecvs
* Work in progress port of XmHTML;  No, it does not compile, don't even try -mig
*
* Revision 1.17  1997/10/23 00:25:06  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.16  1997/08/31 17:37:20  newt
* log edit
*
* Revision 1.15  1997/08/30 01:17:29  newt
* Text layout for <PRE></PRE> is now properly handled.
* Added anchor highlighting support.
* Generalized anchor button rendering.
* Added support for colored <HR>.
*
* Revision 1.14  1997/08/01 13:05:28  newt
* Lots of bugfixes: baseline adjustment of images, forms components or large
* fonts, animated gif disposal handling. Reduced data storage.
*
* Revision 1.13  1997/05/28 01:52:43  newt
* Bugfix 05/26/97-01. Form layout changes.
*
* Revision 1.12  1997/04/29 14:30:19  newt
* Bugfix 04/26/97-01. Fixed SetText to use dimensions of baseline word when
* updating the object data.
*
* Revision 1.11  1997/04/03 05:40:21  newt
* Anchor and word rendering of mixed fonts finally works. Fixed anchor
* transparency for documents with a body images. Fixed image anchor scrolling
* problem.
*
* Revision 1.10  1997/03/28 07:18:37  newt
* Alignment bugfix: now only done with a positive offset.
* Bugfix in frame disposal: index was one frame too early.
* Bugfix in image anchor rendering: now done before checking exposure region.
*
* Revision 1.9  1997/03/20 08:12:38  newt
* SetText: images now use their vertical alignment specs for baseline
* adjustment.
*
* Revision 1.8  1997/03/11 19:57:04  newt
* SetText now does both text and image layout. 
* DrawImage now does animated gifs and transparent images
*
* Revision 1.7  1997/03/04 18:48:52  newt
* Animation stuff. Fixed a spacing bug in SetText
*
* Revision 1.6  1997/03/04 01:00:55  newt
* ?
*
* Revision 1.5  1997/03/02 23:20:31  newt
* Added setting and rendering of images. image-related changes to SetText. 
* SetText now also ``glues'' words together properly
*
* Revision 1.4  1997/02/11 02:09:28  newt
* Fair amount of changes: text layout has been completely changed
*
* Revision 1.3  1997/01/09 06:55:47  newt
* expanded copyright marker
*
* Revision 1.2  1997/01/09 06:46:39  newt
* lots of changes: painting now divided in a layout en rendering part.
*
* Revision 1.1  1996/12/19 02:17:12  newt
* Initial Revision
*
*****/ 
#include <stdlib.h>
#include <stdio.h>

#include "XmHTMLP.h"
#include "XmHTMLfuncs.h"

/*** External Function Prototype Declarations ***/

/*** Public Variable Declarations ***/

/*** Private Datatype Declarations ****/

/*** Private Function Prototype Declarations ****/
static void DrawText(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawAnchor(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawImageAnchor(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawRule(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawBullet(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawFrame(XmHTMLWidget html, XmHTMLImage *image, int xs, int ys);
static void DrawAnchorButton(XmHTMLWidget html, int x, int y, Dimension width,
	Dimension height, TGC top, TGC bottom);

#ifdef WITH_MOTIF
static void
TimerCB(XtPointer data, XtIntervalId *id);
#else
int
TimerCB(gpointer data);
#endif

/*** Private Variable Declarations ***/

/*****
* Name: 		_XmHTMLPaint
* Return Type: 	void
* Description: 	re-paints the given amount of data.
* In: 
*	w:			widget to paint text to
*	start:		start item
*	end:		end item
* Returns:
*	nothing.
*****/
void
_XmHTMLPaint(XmHTMLWidget html, XmHTMLObjectTable *start, 
	XmHTMLObjectTable *end)
{
	XmHTMLObjectTableElement temp;

	_XmHTMLDebug(16, ("paint.c: _XmHTMLPaint Start, paint_start: x = %i, "
		"y = %i\n", start->x, start->y));

	temp = start;

	while(temp != end)
	{
		_XmHTMLDebug(16, ("paint.c: _XmHTMLPaint, painting object %s\n",
			temp->object->element));
		switch(temp->object_type)
		{
			case OBJ_TEXT:
			case OBJ_PRE_TEXT:
				/*
				* First check if this is an image. DrawImage will render
				* an image as an anchor if required.
				*/
				if(temp->text_data & TEXT_IMAGE)
					_XmHTMLDrawImage(html, temp, 0, False);
				else
				{
					/* form scrolling gets handled by formScroll in XmHTML.c */
					if(temp->text_data & TEXT_FORM)
						break;
					else
					{
						if(temp->text_data & TEXT_ANCHOR)
							DrawAnchor(html, temp);
						else
							DrawText(html, temp);
					}
				}
				break;
			case OBJ_BULLET:
				DrawBullet(html, temp);
				break;
			case OBJ_HRULE:
				DrawRule(html, temp);
				break;
			case OBJ_IMG:
				_XmHTMLWarning(__WFUNC__(html, "_XmHTMLPaint"), 
					"Refresh: Invalid image object.");
				break;
			case OBJ_TABLE:
#if 0
			{
				/* horrible kludge */
				int paint_y = html->html.paint_y;
				int paint_x = html->html.paint_x;
				int paint_height = html->html.paint_height;
				int paint_width  = html->html.paint_width;
							
				_XmHTMLPaint(html, temp->table->start->next, temp->table->end);

				html->html.paint_y = paint_y;
				html->html.paint_x = paint_x;
				html->html.paint_height = paint_height;
				html->html.paint_width  = paint_width;

				/* move to last object in this table */
				if((temp = temp->table->end) == NULL);
				{
					_XmHTMLDebug(16, ("paint.c: _XmHTMLPaint End\n"));
					return;
				}
			}
			break;
#endif
			case OBJ_APPLET:
			case OBJ_BLOCK:
			case OBJ_NONE:
				break;
			default:
				_XmHTMLWarning(__WFUNC__(html, "_XmHTMLPaint"), 
					"Unknown object type!");
		}
		temp = temp->next;
	}
	_XmHTMLDebug(16, ("paint.c: _XmHTMLPaint End\n"));
	return;
}

/*****
* Name: 		_XmHTMLRestartAnimations
* Return Type: 	void
* Description: 	restarts all animations. Called by SetValues when the
*				value of the XmNfreezeAnimations resource switches from 
*				True to False.
* In: 
*	html:		XmHTMLWidget id
* Returns:
*	nothing.
*****/
void
_XmHTMLRestartAnimations(XmHTMLWidget html)
{
	XmHTMLImage *tmp;

	for(tmp = html->html.images; tmp != NULL; tmp = tmp->next)
	{
		if(ImageIsAnim(tmp))
		{
			tmp->options |= IMG_FRAMEREFRESH;
			_XmHTMLDrawImage(html, tmp->owner, 0, False);
		}
	}
}

/*****
* Same note as for anchor width adjustment applies here to, except that we
* _must_ do this locally instead of adjusting the width field of a word
* directly. The width field is used by the line breaking algorithm in SetText,
* so modifying it will make the words wider every time the widget is resized.
* Luckily this is only required for underlined/striked words.
*****/
#define AdjustWordWidth { \
	width = words[i].width; \
	if(i < nwords-1 && words[i].line == words[i+1].line) { \
			width = words[i+1].x - words[i].x; \
	} \
}

/*****
* Name:			DrawText
* Return Type: 	void
* Description: 	main text rendering engine.
* In: 
*	html:		XmHTMLWidget id;
*	data:		element to be painted;
* Returns:
*	nothing.
* Note:
*	Used for both regular and preformatted text.
*****/
static void
DrawText(XmHTMLWidget html, XmHTMLObjectTableElement data)
{
	int width, ys, xs, nwords = data->n_words;
	XmHTMLWord *words = data->words;
	Display *dpy = Toolkit_Display(html->html.work_area);
	TWindow win = Toolkit_Widget_Window (html->html.work_area);
	TGC gc = html->html.gc;
	register int i;
	register XmHTMLWord *tmp;
	TFontStruct *my_font;

	if(!nwords)
		return;

	/* check for background color */
	if(data->bg != html->html.body_bg)
	{
		xs = data->x - html->html.scroll_x;
		ys = data->y - html->html.scroll_y;
		Toolkit_Set_Foreground(dpy, gc, data->bg);
		Toolkit_Fill_Rectangle(dpy, win, gc, xs, ys - words[0].font->xfont->ascent,
			data->width, data->height);
	}

	/* only need to set this once */
	Toolkit_Set_Font (dpy, gc, words [0].font->xfont);
	my_font = words [0].font->xfont;
	Toolkit_Set_Foreground(dpy, gc, data->fg);

	for(i = 0 ; i < nwords; i++)
	{
		tmp = &words[i];

		/* 
		* When any of the two cases below is true, the text at the current
		* position is outside the exposed area. Not doing this check would 
		* cause a visible flicker of the screen when scrolling: the entire 
		* line would be repainted, even the invisible text.
		*/
		if(html->html.paint_y > tmp->y + tmp->height ||
			html->html.paint_height < tmp->y)
		{
			_XmHTMLDebug(16, ("paint.c: DrawText, skipping %s, outside "
				"vertical range.\n", tmp->word));
			continue;
		}

		if(html->html.paint_x > tmp->x + tmp->width ||
			html->html.paint_width < tmp->x)
		{
			_XmHTMLDebug(16, ("paint.c: DrawText, skipping %s, outside "
				"horizontal range.\n", tmp->word));
			continue;
		}

		xs = tmp->x - html->html.scroll_x;
		ys = tmp->y - html->html.scroll_y;

		Toolkit_Draw_String (dpy, win, gc, xs, ys, tmp->word, tmp->len, my_font);

		if(tmp->line_data & LINE_UNDER)
		{
			int dy;
			/* 
			* vertical position for underline, barely connects with the 
			* underside of the ``deepest'' character.
			*/
			dy = ys + tmp->base->font->ul_offset;
			AdjustWordWidth;

			Toolkit_Set_Line_Attributes(dpy, gc, tmp->base->font->ul_thickness, 
				(tmp->line_data & LINE_SOLID ? TLineSolid : TLineDoubleDash), 
				TCapButt, TJoinBevel);
			Toolkit_Draw_Line(dpy, win, gc, xs, dy, xs + width, dy);
			if(tmp->line_data & LINE_DOUBLE)
				Toolkit_Draw_Line(dpy, win, gc, xs, dy+2, xs + width, dy+2);
		}
		if(tmp->line_data & LINE_STRIKE)
		{
			int dy;
			/* strikeout line is somewhere near the middle of a line */
			dy = ys - tmp->base->font->st_offset;
			AdjustWordWidth;

			Toolkit_Set_Line_Attributes(dpy, gc, tmp->base->font->st_thickness,
						    TLineSolid, TCapButt, TJoinBevel);
			Toolkit_Draw_Line(dpy, win, gc, xs, dy, xs + width, dy);
		}
	}
}

/*****
* Name:			DrawAnchor
* Return Type: 	void
* Description: 	main text anchor renderer.
* In: 
*	html:		XmHTMLWidget id;
*	data:		anchor to be painted;
* Returns:
*	nothing.
* Note:
*	This routine handles all textual anchor stuff. It paints anchors according
*	to the selected anchor style and (optionally) performs anchor highlighting.
*	Image anchors are rendered by DrawImageAnchor.
*****/
static void
DrawAnchor(XmHTMLWidget html, XmHTMLObjectTableElement data)
{
	int x, xs, y, ys, width, start, nwords = 0;
	Display *dpy = Toolkit_Display(html->html.work_area);
	TWindow win = Toolkit_Widget_Window(html->html.work_area);
	TGC gc = html->html.gc;
	XmHTMLfont *font;
	register int i, j;
	XmHTMLWord **all_words = NULL, *words = NULL, *tmp;
	XmHTMLObjectTableElement a_start, a_end, temp;

	/* pick up the real start of this anchor */
	for(a_start = data; a_start != NULL && a_start->anchor == data->anchor;
		a_start = a_start->prev);

	/* sanity, should never happen */
	if(a_start == NULL)
	{
		_XmHTMLWarning(__WFUNC__(html, "DrawAnchor"), "Internal Error: "
			"could not locate anchor starting point!");
		return;
	}
	/* previous loop always walks back one to many */
	a_start = a_start->next;

	/* pick up the real end of this anchor and count the words */
	for(a_end = a_start, nwords = 0;
		a_end != NULL && a_end->anchor == a_start->anchor; a_end = a_end->next)
	{
		/* ignore image words, they get handled by DrawImageAnchor. */
		if(!(a_end->text_data & TEXT_IMAGE))
			nwords += a_end->n_words;
	}

	_XmHTMLDebug(16, ("paint.c: DrawAnchor, anchor contains %i words\n",
		nwords));

	/* sanity check */
	if(!nwords)
		return;		/* fix 01/30/97-01, kdh */

	/* 
	* put all anchor words into an array if this anchor spans multiple
	* objects (as can be the case with font changes within an anchor)
	* If this isn't the case, just use the words of the current data
	* object.
	*/
	if(a_start->next != a_end)
	{
		_XmHTMLDebug(16, ("paint.c: DrawAnchor, allocating a word array for "
			"%i words\n", nwords));

		all_words = (XmHTMLWord**)calloc(nwords, sizeof(XmHTMLWord*));

		i = 0;
		for(temp = a_start; temp != a_end; temp = temp->next)
		{
			/* ignore image words, they get handled by DrawImageAnchor. */
			if(!(temp->text_data & TEXT_IMAGE))
			{
				for(j = 0 ; j < temp->n_words; j++)
					all_words[i++] = &(temp->words[j]);
			}
		}
		words = NULL;
	}
	else
	{
		_XmHTMLDebug(16, ("paint.c: DrawAnchor, not allocating a word array, "
			"all words belong to the same object.\n"));
		words = data->words;
	}

	/*
	* this is used for drawing the bounding rectangle of an anchor.
	* When an anchor is encountered, width is used to compute the total
	* width of a rectangle surrounding all anchor words on the same line.
	* The bounding rectangle drawn extends a little bit to the left and
	* right of the anchor.
	*/
	width = (words ? words[0].width : all_words[0]->width);
	/* extend to the left */
	x = (words ? words[0].x : all_words[0]->x) - html->html.scroll_x - 2;
	y = (words ? words[0].y : all_words[0]->y) - html->html.scroll_y;
	i = start = 0;

	do
	{
		tmp = (words ? &words[i] : all_words[i]);

		/* anchors are always painted */
		xs = tmp->x - html->html.scroll_x;
		ys = tmp->y - html->html.scroll_y;

		/* baseline font */
		font = tmp->base->font;

		_XmHTMLFullDebug(16, ("paint.c: painting anchor, x = %i, y = %i\n",
			tmp->x, tmp->y));

		/* compute total width of all words on this line */
		if(ys == y)
			width = xs + tmp->width - x;	/* + 2; */
		/* extend to the right if this word has a trailing space */
		width += (tmp->spacing & TEXT_SPACE_TRAIL ? 2 : 0);

		if(ys != y || i == nwords-1)
		{
			/*****
			* Anchor painting. We first make the distinction between documents
			* with and without a body image.
			* Then we check if we need to paint the anchors as pushbuttons
			* and then we check if we are to perform anchor highlighting.
			*****/
			if(html->html.body_image == NULL)
			{
				/*****
				* No body image present.
				* Check if we are to paint the anchors as pushbuttons.
				*****/
				if(html->html.anchor_buttons)
				{
					if(html->html.highlight_on_enter)
					{
						/* can only happen if XmNhighlightOnEnter is set */
						if(data->anchor_state == ANCHOR_INSELECT)
						{
							/* paint button highlighting */
							Toolkit_Fill_Rectangle (dpy, win,
								Toolkit_StyleGC_Highlight(html), x,
								y - font->xfont->ascent, width,
								font->lineheight);
							/* and draw as unselected */
							DrawAnchorButton(html, x, y - font->xfont->ascent,
								width, font->lineheight,
								Toolkit_StyleGC_TopShadow(html),
								Toolkit_StyleGC_BottomShadow(html));
						}
						else if(data->anchor_state == ANCHOR_SELECTED)
						{
							/* paint button highlighting */
							Toolkit_Fill_Rectangle(dpy, win,
								Toolkit_StyleGC_Highlight(html), x,
								y - font->xfont->ascent, width,
								font->lineheight);

							/* and draw as selected */
							DrawAnchorButton(html, x, y - font->xfont->ascent,
								width, font->lineheight,
								Toolkit_StyleGC_BottomShadow(html),
								Toolkit_StyleGC_TopShadow(html));
						}
						else	/* button is unselected */
						{
							/* restore correct background */
							Toolkit_Set_Foreground(dpy, gc, html->html.body_bg);
							Toolkit_Fill_Rectangle(dpy, win, gc, x,
								y - font->xfont->ascent, width,
								font->lineheight);
							/* and draw as unselected */
							DrawAnchorButton(html, x, y - font->xfont->ascent,
								width, font->lineheight,
								Toolkit_StyleGC_TopShadow(html),
								Toolkit_StyleGC_BottomShadow(html));
						}
					}
					else 	/* either selected or unselected */
					{
						/* draw button as selected */
						if(data->anchor_state == ANCHOR_SELECTED)
							DrawAnchorButton(html, x, y - font->xfont->ascent,
								width, font->lineheight,
								Toolkit_StyleGC_BottomShadow(html),
								Toolkit_StyleGC_TopShadow(html));
						else	/* draw button as unselected */
							DrawAnchorButton(html, x, y - font->xfont->ascent,
								width, font->lineheight,
								Toolkit_StyleGC_TopShadow(html),
								Toolkit_StyleGC_BottomShadow(html));
					}
				}
				else	
				{
					/*****
					* No anchor buttons. Determine which color to use to paint
					* the bounding box around the anchor.
					* Note: without a body image, anchor highlighting is
					* achieved by painting the bounding box in the highlight
					* color.
					*****/
					if(data->anchor_state == ANCHOR_INSELECT)
						Toolkit_Set_Foreground(dpy, gc, Toolkit_StyleColor_Highlight(html));
					else if(data->anchor_state == ANCHOR_SELECTED)
						Toolkit_Set_Foreground(dpy, gc, html->html.anchor_activated_bg);
					else
						Toolkit_Set_Foreground(dpy, gc, html->html.body_bg);

					Toolkit_Fill_Rectangle(dpy, win, gc, x, y - font->xfont->ascent,
						width, font->lineheight);
				}
				/* set appropriate foreground color */
				Toolkit_Set_Foreground(dpy, gc,
					data->anchor_state == ANCHOR_SELECTED ? 
					html->html.anchor_activated_fg : data->fg);
			}
			else
			{
				/*****
				* We have a body image. Painting the buttons as above would
				* obliterate the part of the image under the anchor, so
				* if we are to perform button highlighting, we paint the
				* anchor's *text* in the highlight color.
				*****/
				if(html->html.anchor_buttons)
				{
					if(data->anchor_state == ANCHOR_SELECTED)
					{
						/* draw as selected */
						DrawAnchorButton(html, x, y - font->xfont->ascent,
							width, font->lineheight,
							Toolkit_StyleGC_BottomShadow(html),
							Toolkit_StyleGC_TopShadow(html));
					}
					else	/* button is unselected or being selected */
					{
						/*****
						* button is unselected or being selected. In both
						* cases draw it as unselected.
						*****/
						DrawAnchorButton(html, x, y - font->xfont->ascent,
							width, font->lineheight,
							Toolkit_StyleGC_TopShadow(html),
							Toolkit_StyleGC_BottomShadow(html));
					}
				}
				/*****
				* no special stuff for anchor bounding box if we aren't
				* painting the anchors as buttons.
				*****/
				/* set appropriate foreground color */
				if(data->anchor_state == ANCHOR_INSELECT)
					Toolkit_Set_Foreground(dpy, gc, Toolkit_StyleColor_Highlight(html));
				else if(data->anchor_state == ANCHOR_SELECTED)
					Toolkit_Set_Foreground(dpy, gc, html->html.anchor_activated_fg);
				else
					Toolkit_Set_Foreground(dpy, gc, data->fg);
			}
			/* 
			* paint all text. Need to do it here since the XFillRect call
			* would obliterate any text painted before it.
			*/

			if(words)
			{
				for(j = start; j < i+1; j++)
				{
					TFontStruct *my_font = words [j].font->xfont;
					
					Toolkit_Set_Font (dpy, gc, my_font);
					Toolkit_Draw_String(dpy, win, gc,
						words[j].x - html->html.scroll_x,
						words[j].y - html->html.scroll_y,
						words[j].word, words[j].len, my_font);
				}
			}
			else
			{
				for(j = start; j < i+1; j++)
				{
					TFontStruct *my_font = all_words [j]->font->xfont;
					
					Toolkit_Set_Font (dpy, gc, all_words[j]->font->xfont);
					Toolkit_Draw_String(dpy, win, gc,
						all_words[j]->x - html->html.scroll_x,
						all_words[j]->y - html->html.scroll_y,
						all_words[j]->word, all_words[j]->len, my_font);
				}
			}

			/* Anchor buttons are never underlined, it looks ugly */
			if(!html->html.anchor_buttons && 
				(tmp->line_data & LINE_SOLID || tmp->line_data & LINE_DASHED))
			{
				int dy = y + Toolkit_XFont (font->xfont)->max_bounds.descent-2;
				Toolkit_Set_Line_Attributes(dpy, gc, 1, 
					(tmp->line_data & LINE_SOLID ? TLineSolid : TLineDoubleDash), 
					TCapButt, TJoinBevel);
				Toolkit_Draw_Line(dpy, win, gc, x+2, dy, x + width, dy);
				/* draw another line if requested */
				if(tmp->line_data & LINE_DOUBLE)
					Toolkit_Draw_Line(dpy, win, gc, x+2, dy+2, x + width, dy+2);
			}
			if(tmp->line_data & LINE_STRIKE)
			{
				int dy = y - (int)(0.5*(Toolkit_XFont (font->xfont)->max_bounds.ascent))+3;
				Toolkit_Set_Line_Attributes(dpy, gc, 1, TLineSolid, TCapButt, TJoinBevel);
				Toolkit_Draw_Line(dpy, win, gc, x+2, dy, x + width - 2, dy);
			}
			/* stupid hack to get the last word of a broken anchor right */
			if(ys != y && i == nwords-1)
				i--;
			/* next word starts on another line */
			width = tmp->width;
			start = i;
			x = xs-2;
			y = ys;
		}
		i++;
	}
	while(i != nwords);

	if(words == NULL)
	{
		_XmHTMLDebug(5, ("paint.c: DrawAnchor, freeing allocated word "
			"array.\n"));
		free(all_words);	/* fix 05/26/97-01, kdh */
		/*****
		* adjust current object data as we have now updated a number of
		* objects in one go. We must use prev as _XmHTMLPaint will advance
		* to a_end itself.
		*****/
		data = (a_end ? a_end->prev : data);
	}
}

static void
DrawImageAnchor(XmHTMLWidget html, XmHTMLObjectTableElement data)
{
	int x, y, width, height;
	Display *dpy = Toolkit_Display(html->html.work_area);
	TWindow win = Toolkit_Widget_Window(html->html.work_area);
	TGC gc = html->html.gc;

	/*
	* this is used for drawing the bounding rectangle of an anchor.
	* When an anchor is encountered, width is used to compute the total
	* width of a rectangle surrounding all anchor words on the same line.
	* The bounding rectangle drawn extends a little bit to the left and
	* right of the anchor.
	*/
	width  = data->words->width + 2;
	height = data->words->height + 2;

	/* extend to the left */
	x = data->words->x - html->html.scroll_x - 1;
	/* and to the top */
	y = data->words->y - html->html.scroll_y - 1;

	if(data->words->image->border)
	{
		/* add border offsets as well */
		x -= data->words->image->border;
		y -= data->words->image->border;

		_XmHTMLFullDebug(16, ("paint.c: painting image anchor, x = %i, "
			"y = %i\n", data->x, data->y));

		if(html->html.anchor_buttons)
		{
			TGC top, bottom;
			if(html->html.highlight_on_enter)
			{
				/* can only happen if XmNhighlightOnEnter is set */
				if(data->anchor_state == ANCHOR_INSELECT)
				{
					/* paint button highlighting */
					if(html->html.body_image == NULL && 
						data->words->image->clip != None)
						Toolkit_Fill_Rectangle(dpy, win, Toolkit_StyleGC_Highlight(html),
							x, y , width, height);
					/* and draw as unselected */
					top    = Toolkit_StyleGC_TopShadow(html);
					bottom = Toolkit_StyleGC_BottomShadow(html);
				}
				else if(data->anchor_state == ANCHOR_SELECTED)
				{
					/* paint button highlighting */
					if(html->html.body_image == NULL && 
						data->words->image->clip != None)
						Toolkit_Fill_Rectangle(dpy, win, Toolkit_StyleGC_Highlight(html), x,
							y, width, height);

					/* and draw as selected */
					top    = Toolkit_StyleGC_BottomShadow(html);
					bottom = Toolkit_StyleGC_TopShadow(html);
				}
				else	/* button is unselected */
				{
					/* restore correct background */
					if(html->html.body_image == NULL && 
						data->words->image->clip != None)
					{
						Toolkit_Set_Foreground(dpy, gc, html->html.body_bg);
						Toolkit_Fill_Rectangle(dpy, win, gc, x, y, width, height);
						Toolkit_Set_Foreground(dpy, gc, html->html.body_fg);
					}
					/* and draw as unselected */
					top    = Toolkit_StyleGC_TopShadow(html);
					bottom = Toolkit_StyleGC_BottomShadow(html);
				}
			}
			else 	/* either selected or unselected */
			{
				if(data->anchor_state == ANCHOR_SELECTED)
				{
					top = Toolkit_StyleGC_BottomShadow(html);
					bottom = Toolkit_StyleGC_TopShadow(html);
				}
				else
				{
					bottom = Toolkit_StyleGC_BottomShadow(html);
					top = Toolkit_StyleGC_TopShadow(html);
				}
			}
			DrawAnchorButton(html, x, y, width, height, top, bottom);
		}
		else
		{
			/* set line attribs */
			Toolkit_Set_Line_Attributes(dpy, gc, data->words->image->border,
				TLineSolid, TCapButt, TJoinRound);

			/* draw background */
			/* fix 04/03/97-01, kdh */
			if(html->html.body_image == NULL)
			{
				if(data->anchor_state == ANCHOR_INSELECT)
					Toolkit_Set_Foreground(dpy, gc, Toolkit_StyleColor_Highlight(html));
				else if(data->anchor_state == ANCHOR_SELECTED)
					Toolkit_Set_Foreground(dpy, gc, html->html.anchor_activated_bg);
				else
					Toolkit_Set_Foreground(dpy, gc, html->html.body_bg);
				Toolkit_Fill_Rectangle(dpy, win, gc, x, y, width, height);
			}

			/* draw lines */
			Toolkit_Set_Foreground(dpy, gc, data->anchor_state == ANCHOR_SELECTED ?
				html->html.anchor_activated_fg : html->html.anchor_fg);
			Toolkit_Draw_Rectangle(dpy, win, gc, x, y, width, height);
		}
	}
	/* paint the alt text if images are disabled */
	if(!html->html.images_enabled)
	{
		TFontStruct *my_font;
		
		my_font = data->words->font->xfont;
		Toolkit_Set_Font(dpy, gc, my_font);
		Toolkit_Set_Foreground(dpy, gc, data->anchor_state == ANCHOR_SELECTED ?
			html->html.anchor_activated_fg : html->html.anchor_fg);

		/* put text inside bounding rectangle */
		x += data->words->image->width + 4;
		y += data->words->image->height/2 + 4;
		Toolkit_Draw_String (dpy, win, gc, x, y, data->words->word, data->words->len, my_font);
	}
}

/*
* To prevent racing conditions, we must first remove an
* existing timeout proc before we add a new one.
*/
#ifdef WITH_MOTIF
#define REMOVE_TIMEOUTPROC(IMG) { \
	if(IMG->proc_id) \
	{ \
		_XmHTMLDebug(16, ("paint.c: DrawImage, removing animation %s " \
			"timeout\n", IMG->url)); \
		XtRemoveTimeOut(IMG->proc_id); \
		IMG->proc_id = None; \
	} \
}
#else
#define REMOVE_TIMEOUTPROC(IMG) { \
	if (IMG->proc_id == None) { gtk_timeout_remove (IMG->proc_id); IMG->proc_id = None; } }
#endif

#ifdef WITH_MOTIF
static void
TimerCB(XtPointer data, XtIntervalId *id)
#else
int
TimerCB(gpointer data)
#endif
{
	XmHTMLImage *image = (XmHTMLImage*)data;

	/* freeze animation at current frame */
	if(image->html->html.freeze_animations)
	{
		REMOVE_TIMEOUTPROC(image);
#ifdef WITH_MOTIF
		return;
#else
		return TIdleKeep;
#endif
	}
	image->options |= IMG_FRAMEREFRESH;
	_XmHTMLDrawImage(image->html, image->owner, 0, True);
#ifndef WITH_MOTIF
	return TIdleRemove;
#endif
}

#ifdef WITH_MOTIF
#define RESET_GC(MYGC) { \
	values.clip_mask = None; \
	values.clip_x_origin = 0; \
	values.clip_y_origin = 0; \
	valuemask = GCClipMask | GCClipXOrigin | GCClipYOrigin; \
	XChangeGC(dpy, MYGC, valuemask, &values); \
}
#else
#define RESET_GC(MYGC) { \
	gdk_gc_set_clip_origin (MYGC, 0, 0); \
        gdk_gc_set_clip_mask (MYGC, NULL); }
#endif

#define GET_TILE_OFFSETS(TSX,TSY) { \
	int tile_width, tile_height, x_dist, y_dist; \
	int ntiles_x, ntiles_y; \
	int x_offset, y_offset, xd, yd; \
	/* adjust for logical screen offsets */ \
	xd = xs + fx; /* x-pos relative to upper-left screen corner */ \
	yd = ys + fy; /* y-pos relative to upper-left screen corner */ \
	tile_width  = html->html.body_image->width; \
	tile_height = html->html.body_image->height; \
	x_dist = html->html.scroll_x + xd; /* total distance covered so far */ \
	y_dist = html->html.scroll_y + yd; /* total distance covered so far */ \
	ntiles_x = (int)(x_dist/tile_width); /* no of horizontal tiles */ \
	ntiles_y = (int)(y_dist/tile_height); /* no of vertical tiles */ \
	x_offset = x_dist - (ntiles_x * tile_width); \
	y_offset = y_dist - (ntiles_y * tile_height); \
	TSX = xd - x_offset; \
	TSY = yd - y_offset; \
}

/*****
* Name: 		DrawFrame
* Return Type: 	void
* Description: 	animation driver, does frame disposal and renders a new
*				frame
* In: 
*	w:			XmHTMLWidget id
*	image:		image data
*	xs:			absolute screen x-coordinate
*	ys:			absolute screen y-coordinate
* Returns:
*	nothing
* Note:
*	Complex animations
*	------------------
*	Instead of drawing into the window directly, we draw into an internal
*	pixmap and blit this pixmap to the screen when all required processing
*	has been done, which is a lot faster than drawing on the screen directly.
*	Another advantage of this approach is that we always have a current state
*	available which can be used when an animation is scrolled on and off
*	screen (frame dimensions differ from logical screen dimensions or a
*	disposal method other than XmIMAGE_DISPOSE_NONE is to be used).
*
*	Easy animations
*   ---------------
*	Each frame is blitted to screen directly, only processing done is using a
*	possible clipmask (frame dimensions equal to logical screen dimensions and
*	a disposal method of XmIMAGE_DISPOSE_NONE).
*****/
static void
DrawFrame(XmHTMLWidget html, XmHTMLImage *image, int xs, int ys)
{
	int idx, width = 0, height = 0, fx, fy;
	unsigned long valuemask;
	XGCValues values;
	Display *dpy = Toolkit_Display(html->html.work_area);
	TWindow win = Toolkit_Widget_Window(html->html.work_area);
	TGC gc = html->html.gc;

	_XmHTMLDebug(16, ("paint.c: DrawFrame, animation %s, frame %i, "
		"(x = %i, y = %i)\n", image->url, image->current_frame, xs, ys));

	/* first reset the gc */
	RESET_GC(gc);

	/*
	* First check if we are running this animation internally. If we aren't
	* we have a simple animation of which each frame has the same size and
	* no disposal method has been specified. This type of animations are blit
	* to screen directly.
	*/
	if(!ImageHasState(image))
	{
		/* index of current frame */
		idx = image->current_frame;
		width  = image->frames[idx].w;
		height = image->frames[idx].h;

		/* can happen when a frame falls outside the logical screen area */
		if(image->frames[idx].pixmap != None)
		{
			/* plug in the clipmask */
			if(image->frames[idx].clip)
			{
#ifdef WITH_MOTIF
				values.clip_mask = image->frames[idx].clip;
				values.clip_x_origin = xs;
				values.clip_y_origin = ys;
				valuemask = GCClipMask | GCClipXOrigin | GCClipYOrigin;
				XChangeGC(dpy, gc, valuemask, &values);
#else
				gdk_gc_set_clip_origin (gc, xs, ys);
				gdk_gc_set_clip_mask (gc, image->frames [idx].clip);
#endif
			}
			/* blit frame to screen */
			Toolkit_Copy_Area (dpy, image->frames[idx].pixmap, win, gc, 0, 0, width,
					   height, xs, ys);
		}
		/*
		* Jump to frame updating when we are not triggered
		* by an exposure, otherwise just return.
		*/
		if(ImageFrameRefresh(image))
			goto nextframe;
		return;
	}
	/*
	* If DrawFrame was triggered by an exposure, just blit current animation
	* state to screen and return immediatly.
	*/
	if(!ImageFrameRefresh(image))
	{
		Toolkit_Copy_Area (dpy, image->pixmap, win, gc, 0, 0, image->width, image->height, xs, ys);
		return;
	}

	/*****
	* If we get here we are running the animation internally. First check the
	* disposal method and update the current state accordingly *before*
	* putting the next frame on the display. 
	* Pixmap can be None if a frame falls outside the logical screen area.
	* idx is the index of the previous frame (the frame that is currently
	* being displayed).
	*****/
	idx = image->current_frame ? image->current_frame - 1 : image->nframes - 1;

	if(image->frames[idx].pixmap != None)
	{
		fx     = image->frames[idx].x;
		fy     = image->frames[idx].y;
		width  = image->frames[idx].w;
		height = image->frames[idx].h;

		if(image->frames[idx].dispose == XmIMAGE_DISPOSE_BY_BACKGROUND)
		{
			_XmHTMLDebug(16, ("paint.c: DrawFrame, %s, disposing frame %i "
				"by background, x = %i, y = %i, %ix%i\n", image->url, idx,
				xs + fx, ys + fy, width, height));

			/* we have a body image; get proper background tile offsets. */
			if(html->html.body_image)
			{
				/* we have a body image, compute correct tile offsets */
				int tsx, tsy;

				GET_TILE_OFFSETS(tsx,tsy);

#ifdef WITH_MOTIF
				values.fill_style = FillTiled;
				values.tile = html->html.body_image->pixmap;
				values.ts_x_origin = tsx - xs;
				values.ts_y_origin = tsy - ys;

				_XmHTMLDebug(16, ("paint.c: DrawFrame: background disposal "
					"uses a tile with origin at (%i,%i)\n", tsx, tsy));

				/* fix 06/18/97-02, kdh */
				/* set gc values */
				valuemask = GCTile | GCTileStipXOrigin | GCTileStipYOrigin |
					GCFillStyle ;
				XChangeGC(dpy, html->html.bg_gc, valuemask, &values);
#else
				gdk_gc_set_fill (html->html.bg_gc, GDK_TILED);
				gdk_gc_set_tile (html->html.bg_gc, html->html.body_image->pixmap);
				gdk_gc_set_ts_origin (html->html.bg_gc, tsx-xs, tsx-ys);
#endif
				/* paint it. */
				Toolkit_Fill_Rectangle(dpy, image->pixmap, html->html.bg_gc, fx, fy, width, height);
			}
			/*
			* No body image, do a fillrect in background color. clipmasks
			* are ignored since we are already restoring to background!
			*/
			else
			{
				Toolkit_Set_Foreground(dpy, gc, html->html.body_bg);
				Toolkit_Fill_Rectangle(dpy, image->pixmap, gc, fx, fy, width, height);
				Toolkit_Set_Foreground(dpy, gc, html->html.body_fg);
			}
		}
		/*
		* no disposal method but we have a clipmask. Need to plug in
		* the background image or color. As we are completely overlaying
		* the current image with the new image, we can safely erase
		* the entire contents of the current state with the wanted
		* background, after which we use the clipmask to copy the requested
		* parts of the image on screen.
		*
		* Please note that this is *only* done for the very first frame
		* of such an animation. All other animations, whether they have a
		* clipmask or not, are put on top of this frame. Doing it for
		* other frames as well would lead to unwanted results as the
		* underlying portions of the animation would be replaced with the
		* current background, and thereby violating the none disposal
		* method logic.
		*/
		else if(image->frames[idx].dispose == XmIMAGE_DISPOSE_NONE &&
			idx == 0 && image->frames[idx].clip != None)
		{
			/* we have a body image, compute correct tile offsets */
			if(html->html.body_image)
			{
				int tsx, tsy;

				GET_TILE_OFFSETS(tsx,tsy);

				_XmHTMLDebug(16, ("paint.c: DrawFrame: background disposal "
					"uses a tile with origin at (%i,%i)\n", tsx, tsy));

#ifdef WITH_MOTIF
				/* update gc values */
				values.fill_style = FillTiled;
				values.tile = html->html.body_image->pixmap;
				values.ts_x_origin = tsx - xs;
				values.ts_y_origin = tsy - ys;
				valuemask = GCTile | GCTileStipXOrigin | GCTileStipYOrigin |
					GCFillStyle ;
				XChangeGC(dpy, html->html.bg_gc, valuemask, &values);
#else
				gdk_gc_set_fill (html->html.bg_gc, GDK_TILED);
				gdk_gc_set_tile (html->html.bg_gc, html->html.body_image->pixmap);
				gdk_gc_set_ts_origin (html->html.bg_gc, tsx-xs, tsy-ys);
#endif
				/* do a fillrect to render the background image */ 
				Toolkit_Fill_Rectangle(dpy, image->pixmap, html->html.bg_gc, fx, fy,
					width, height);
				/*
				* no need to reset the background gc, its only used for
				* overall background rendering.
				*/
			}
			else
			{
				/* do a plain fillrect in current background color */
				Toolkit_Set_Foreground(dpy, gc, html->html.body_bg);
				Toolkit_Fill_Rectangle(dpy, image->pixmap, gc, fx, fy, width, height);
				Toolkit_Set_Foreground(dpy, gc, html->html.body_fg);
			}
#ifdef WITH_MOTIF
			/* now plug in the clipmask */
			values.clip_mask = image->frames[idx].clip;
			values.clip_x_origin = fx;
			values.clip_y_origin = fy;
			valuemask = GCClipMask | GCClipXOrigin | GCClipYOrigin;
			XChangeGC(dpy, gc, valuemask, &values);
#else
			gdk_gc_set_clip_mask (gc, image->frames [idx].clip);
			gdk_gc_set_clip_origin (gc, fx, fy);
#endif
			/* paint it. Use full image dimensions */
			Toolkit_Copy_Area(dpy, image->frames[idx].pixmap, image->pixmap, gc,
					  0, 0, width, height, fx, fy);
		}
		/* dispose by previous (the only one to have a prev_state) */
		else if(image->frames[idx].prev_state != None)
		{
			/* plug in the clipmask */
			if(image->frames[idx].clip)
			{
#ifdef WITH_MOTIF
				/* set gc values */
				values.clip_mask = image->frames[idx].clip;
				values.clip_x_origin = fx;
				values.clip_y_origin = fy;
				valuemask = GCClipMask | GCClipXOrigin | GCClipYOrigin;
				XChangeGC(dpy, gc, valuemask, &values);
#else
				gdk_gc_set_clip_mask (gc, image->frames [idx].clip);
				gdk_gc_set_clip_origin (gc, fx, fy);
#endif
			}
			/* put previous screen state on current state */
			Toolkit_Copy_Area(dpy, image->frames[idx].prev_state, image->pixmap, gc,
					  0, 0, width, height, fx, fy);
		}
	}
	/* reset gc */
	RESET_GC(gc);

	/* index of current frame */
	idx = image->current_frame;

	/* can happen when a frame falls outside the logical screen area */
	if(image->frames[idx].pixmap != None)
	{
		fx      = image->frames[idx].x;
		fy      = image->frames[idx].y;
		width   = image->frames[idx].w;
		height  = image->frames[idx].h;

		/*
		* get current screen state if we are to dispose of this frame by the 
		* previous state. The previous state is given by the current pixmap,
		* so we just create a new pixmap and copy the current one into it.
		* This is about the fastest method I can think of.
		*/
		if(image->frames[idx].dispose == XmIMAGE_DISPOSE_BY_PREVIOUS &&
			image->frames[idx].prev_state == None)
		{
			TPixmap prev_state;
			TGC tmpGC;

			/* create pixmap that is to receive the image */
			prev_state = Toolkit_Create_Pixmap(dpy, win, width, height, 
							   XCCGetDepth(html->html.xcc));

#ifdef WITH_MOTIF
			/* copy it */
			tmpGC = XCreateGC(dpy, prev_state, 0, 0);
			XSetFunction(dpy, tmpGC, GXcopy);
#else
			tmpGC = gdk_gc_new(prev_state);
			gdk_gc_set_function(tmpGC, GDK_COPY);
#endif
			Toolkit_Copy_Area(dpy, image->pixmap, prev_state, tmpGC, fx, fy, width, height, 0, 0);

			/* and save it */
			image->frames[idx].prev_state = prev_state;

			/* free and destroy */
			Toolkit_GC_Free(dpy, tmpGC);
		}
		if(image->frames[idx].clip)
		{
#ifdef WITH_MOTIF
			values.clip_mask = image->frames[idx].clip;
			values.clip_x_origin = fx;
			values.clip_y_origin = fy;
			valuemask = GCClipMask | GCClipXOrigin | GCClipYOrigin;
			XChangeGC(dpy, gc, valuemask, &values);
#else
			gdk_gc_set_clip_origin (gc, fx, fy);
			gdk_gc_set_clip_mask (gc, image->frames [idx].clip);
#endif
		}
		Toolkit_Copy_Area(dpy, image->frames[idx].pixmap, image->pixmap, gc, 0, 0,
				  width, height, fx, fy);

		/* reset gc */
		RESET_GC(gc);

		/* blit current state to screen */
		Toolkit_Copy_Area(dpy, image->pixmap, win, gc, 0, 0, image->width,
			image->height, xs, ys);
	}
nextframe:
	image->current_frame++;

	/* will get set again by TimerCB */
	image->options &= ~(IMG_FRAMEREFRESH);

	if(image->current_frame == image->nframes)
	{
		image->current_frame = 0;
		/*
		* Sigh, when an animation is running forever (loop_count == 0) and
		* some sucker is keeping XmHTML up forever, chances are that we *can*
		* exceed INT_MAX. Since some systems don't wrap their integers properly
		* when their value exceeds INT_MAX, we can't keep increasing the
		* current loop count forever since this *can* lead to a crash (which
		* is a potential security hole). To prevent this from happening, we
		* *only* increase current_loop when run this animation a limited
		* number of times.
		*/
		if(image->loop_count)
		{
			image->current_loop++;
			/*
			* If the current loop count matches the total loop count, depromote
			* the animation to a regular image so the next time the timer
			* callback is activated we will enter normal image processing. 
			*/
			if(image->current_loop == image->loop_count)
				image->options &= ~(IMG_ISANIM);
		}
	}
	/*
	* To prevent racing conditions, we must first remove an existing 
	* timeout proc before adding a new one.
	*/
	REMOVE_TIMEOUTPROC(image);

#ifdef WITH_MOTIF
	image->proc_id = XtAppAddTimeOut(image->context, 
		image->frames[idx].timeout, TimerCB, image);
#else
	gtk_timeout_add (image->frames [idx].timeout, TimerCB, image);
#endif
	_XmHTMLDebug(16, ("paint.c: DrawFrame end\n"));
}

/*****
* Name: 		_XmHTMLDrawImage
* Return Type: 	void
* Description: 	image refresher.
* In: 
*	w:			XmHTMLWidget id
*	data:		Object data.
*	y_offset:	vertical offset for screen copying;
*	from_timerCB: true when called from the timeout proc.
* Returns:
*	nothing
* Note:
*	this is a funny routine: it does plain images as well as
*	animations. Animations with a loop count of zero will loop
*	forever. Other animations will loop their counts and when that
*	has been reached they are depromoted to regular images.
*	The only way to restore animations with a loop count to animations
*	again is to reload them (XmHTMLImageUpdate).
*****/
void
_XmHTMLDrawImage(XmHTMLWidget html, XmHTMLObjectTableElement data, int y_offset,
	Boolean from_timerCB)
{
	int xs, ys;
	XmHTMLImage *image;
	unsigned long valuemask;
	XGCValues values;
	Display *dpy;
	TWindow win;
	TGC gc;

	/* sanity check */
	if((image = data->words->image) == NULL)
		return;

	dpy = Toolkit_Display(html->html.work_area);
	win = Toolkit_Widget_Window(html->html.work_area);
	gc = (ImageIsProgressive(image) ? html->html.plc_gc : html->html.gc);

	/* compute correct image offsets */
	xs = data->words->x - html->html.scroll_x;
	ys = data->words->y - html->html.scroll_y;

	_XmHTMLDebug(16, ("paint.c: DrawImage start, x = %i, y = %i\n",
		data->x, data->y));

	/*
	* animation frames should be repainted if the animation is somewhere
	* in the visible area.
	*/
	if(ImageFrameRefresh(image))
	{
		if(xs + image->width < 0 || xs > html->html.work_width ||
			ys + image->height < 0 || ys > html->html.work_height)
		{
			_XmHTMLDebug(16, ("paint.c: DrawImage end, animation %s not in "
				"visible area.\n", image->url));
			REMOVE_TIMEOUTPROC(image);
			return;
		}
	}
	/*
	* Only do this when we are repainting this image as a result of an
	* exposure.
	*/
	if(!from_timerCB)
	{
		/* anchors are always repainted if they are visible */
		/* fix 03/25/97-01, kdh */
		if(data->text_data & TEXT_ANCHOR)
		{
			if(xs + image->width > 0 && xs < html->html.work_width &&
				ys + image->height > 0 && ys < html->html.work_height)
				DrawImageAnchor(html, data);
		}
		else
		{
			/* 
			* When any of the two cases below is true, the image is not in the 
			* exposed screen area. Not doing this check would cause a visible 
			* flicker of the screen when scrolling: the entire image would be 
			* repainted even if it is not visible.
			*/
			if(html->html.paint_y > data->words->y + image->height ||
				html->html.paint_height < data->words->y)
			{
				_XmHTMLDebug(16, ("paint.c: DrawImage end, out of vertical "
					"range.\n"));
				return;
			}
			if(html->html.paint_x > data->words->x + image->width ||
				html->html.paint_width < data->words->x)
			{
				_XmHTMLDebug(16, ("paint.c: DrawImage end, out of horizontal "
					"range.\n"));
				return;
			}
		}
	}

	/*
	* If this is an animation, paint next frame or restore current
	* state when we are scrolling this animation on and off screen.
	*/
	if(ImageIsAnim(image))
		DrawFrame(html, image, xs, ys);
	else if(image->pixmap != None)
	{
		/* put in clipmask */
		if(image->clip)
		{
#ifdef WITH_MOTIF
			/* set gc values */
			values.clip_mask = image->clip;
			values.clip_x_origin = xs;
			values.clip_y_origin = ys;
			valuemask = GCClipMask | GCClipXOrigin | GCClipYOrigin;
			XChangeGC(dpy, gc, valuemask, &values);
#else
			gdk_gc_set_clip_mask (gc, image->clip);
			gdk_gc_set_clip_origin (gc, xs, ys);
#endif
		}
		/* copy to screen */
		Toolkit_Copy_Area(dpy, image->pixmap, win, gc, 0, y_offset, image->width,
				  image->height, xs, ys + y_offset);
	}
	/* reset gc */
	RESET_GC(gc);

	/*****
	* Paint the alt text if images are disabled or when this image is
	* delayed.
	*****/
	if((!html->html.images_enabled ||
		(image->html_image && ImageInfoDelayed(image->html_image))) &&
		!(data->text_data & TEXT_ANCHOR))
	{
		TFontStruct *my_font = data->words->font->xfont;

		Toolkit_Set_Font(dpy, gc, my_font);
		Toolkit_Set_Foreground(dpy, gc, html->html.body_fg);

		/* put text inside bounding rectangle */
		xs += image->width + 4;
		ys += image->height/2 + 4;
		Toolkit_Draw_String(dpy, win, gc, xs, ys, data->words->word, data->words->len, my_font);
	}

	/* check if we have to draw the imagemap bounding boxes */
	if(image->map_type == XmMAP_CLIENT && html->html.imagemap_draw)
		_XmHTMLDrawImagemapSelection(html, image);
	
	_XmHTMLDebug(16, ("paint.c: DrawImage end\n"));
}

/*****
* Name: 		DrawRule
* Return Type: 	void
* Description: 	paints a horizontal rule.
* In: 
*	html:		XmHTMLWidget id;
*	data:		element data;
* Returns:
*	nothing.
* Note:
*	Rules that had their noshade attribute set are identiefied by having
*	a non-zero y_offset field in the data. We support a color attribute
*	in this case as well, so colored rules are possible. They are also
*	possible if a hr is in the proper context: one of the extensions
*	supported by XmHTML is a color attribute on the DIV tag.
*****/
static void
DrawRule(XmHTMLWidget html, XmHTMLObjectTableElement data)
{
	int dy;
	Display *dpy = Toolkit_Display(html->html.work_area);
	TWindow win = Toolkit_Widget_Window(html->html.work_area);
	TGC gc;
	int xs, ys;

	/* 
	* recompute rule layout if we are auto-sizing.
	* Needs to be done since the formatted_width is only known once
	* the entire document has been laid out.
	*/
	if(html->html.resize_width)
	{
		int x;
		int width = html->html.work_width - html->html.margin_width;

		/* horizontal offset */
		x = html->html.margin_width + data->ident;

		/* See if we have an width specification */
		if(data->len != 0)
		{
			if(data->len < 0)	/* % spec */
				width *= (float)(-1*data->len/100.);
			else	/* pixel spec, cut if wider than available */
				width = (data->len > width ? width : data->len);
			/* alignment is only honored if there is a width spec */
			switch(data->halign)
			{
				case XmHALIGN_RIGHT:
					x = html->html.margin_width + html->html.work_width - width;
					break;
				case XmHALIGN_CENTER:
					x = html->html.margin_width + 
					(html->html.work_width - width - html->html.margin_width)/2;
				default:	/* shutup compiler */
					break;
			}
		}
		/* Save updated position and width */
		data->x = x;
		data->width = width;
	}

	xs = data->x - html->html.scroll_x;

	/* vertical offset */
	dy = (int)(0.75*(html->html.default_font->height));
	ys = data->y - html->html.scroll_y;

	if(data->height)
	{
		if(data->y_offset)	/* noshade */
		{
			gc = html->html.gc;
			Toolkit_Set_Line_Attributes(dpy, gc, 1, TLineSolid, TCapButt, TJoinBevel);
			Toolkit_Set_Foreground(dpy, gc, data->fg);
			Toolkit_Fill_Rectangle(dpy, win, gc, xs, ys + dy, data->width, data->height); 
		}
		else
		{
			/* top & left border */
			gc = Toolkit_StyleGC_BottomShadow(html);
			/* top & left border */
			Toolkit_Fill_Rectangle(dpy, win, gc, xs, ys + dy, data->width, 1);
			Toolkit_Fill_Rectangle(dpy, win, gc, xs, ys + dy, 1, data->height-1);

			/* bottom & right border */
			gc = Toolkit_StyleGC_TopShadow(html);
			Toolkit_Fill_Rectangle(dpy, win, gc, xs+1, ys + dy + data->height-1, 
				data->width-1, 1);
			Toolkit_Fill_Rectangle(dpy, win, gc, 
				xs + data->width - 1, ys + dy + 1, 1, data->height-2);
		}
	}
	else
	{
		if(data->y_offset) /* noshade */
		{
			gc = html->html.gc;
			Toolkit_Set_Line_Attributes(dpy, gc, 1, TLineSolid, TCapButt, TJoinBevel);
			Toolkit_Set_Foreground(dpy, gc, data->fg);
			Toolkit_Draw_Line(dpy, win, gc, xs, ys + dy, xs + data->width, ys + dy);
			Toolkit_Draw_Line(dpy, win, gc, xs, ys + dy + 1, xs + data->width, ys + dy + 1);
		}
		else
		{
			/* topline */
			gc = Toolkit_StyleGC_BottomShadow(html);
			Toolkit_Draw_Line (dpy, win, gc, xs, ys + dy, xs + data->width, ys + dy);

			/* bottomline */
			gc = Toolkit_StyleGC_TopShadow(html);
			Toolkit_Draw_Line (dpy, win, gc, xs, ys + dy + 1, xs + data->width, ys + dy + 1);
		}
	}
}

static void
DrawBullet(XmHTMLWidget html, XmHTMLObjectTableElement data)
{
	Display *dpy = Toolkit_Display(html->html.work_area);
	TWindow win = Toolkit_Widget_Window(html->html.work_area);
	TGC gc = html->html.gc;
	int ys, xs;

	/* reset colors, an anchor might have been drawn before */
	Toolkit_Set_Foreground(dpy, gc, data->fg);
	Toolkit_Set_Line_Attributes(dpy, gc, 1, TLineSolid, TCapButt, TJoinBevel);

	xs = data->x - html->html.scroll_x;
	ys = data->y - html->html.scroll_y;

	switch(data->marker)
	{
		case XmMARKER_DISC:
			Toolkit_Fill_Arc(dpy, win, gc,
					 xs - 2*data->width, ys - data->height, 
					 data->width, data->width, 0, 23040);
			break;
		case XmMARKER_SQUARE:
			Toolkit_Draw_Rectangle(dpy, win, gc,
				xs - 2*data->width, ys - data->height, 
				data->width, data->width);
			break;
		case XmMARKER_CIRCLE:
			Toolkit_Draw_Arc(dpy, win, gc,
				xs - 2*data->width, ys - data->height, 
				data->width, data->width, 0, 23040);
			break;
		default: {
			TFontStruct *my_font;

			my_font = html->html.default_font->xfont;
			Toolkit_Set_Font(dpy, gc, my_font);
			Toolkit_Draw_String(dpy, win, gc,
				xs - data->width, ys, data->text, data->len, my_font);
			break;
			}
	}
}

static void
DrawAnchorButton(XmHTMLWidget html, int x, int y, Dimension width,
	Dimension height, TGC top_shadow_GC, TGC bottom_shadow_GC)
{
	Display *dpy = Toolkit_Display(html->html.work_area);
	TWindow win = Toolkit_Widget_Window(html->html.work_area);

	/* top & left border */
	Toolkit_Draw_Line(dpy, win, top_shadow_GC, x, y, x + width - 1, y);
	Toolkit_Draw_Line(dpy, win, top_shadow_GC, x, y + 1, x, y + height - 1);

	/* bottom & right border */
	Toolkit_Draw_Line(dpy, win, bottom_shadow_GC, x, y + height, x + width,
		y + height);
	Toolkit_Draw_Line(dpy, win, bottom_shadow_GC, x + width,  y, x + width,
		y + height - 1);
}
