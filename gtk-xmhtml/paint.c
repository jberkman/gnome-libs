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
#define MAX_OUTLINE_ITERATIONS	1500

/*****
* Object bounding box. Used for recursive layout computations in tables
* and text flowing around images.
*****/
typedef struct{
	Cardinal x;		/* upper left x position of box */
	Cardinal y;		/* upper left y position of box */
	int left;		/* left margin of box, absolute position */
	int right;		/* right margin of box, absolute position */
	int width;		/* height of box, relative position */
	int height;		/* width of box, relative position */
}PositionBox;

/*** Private Function Prototype Declarations ****/
/*****
* Painting routines
*****/
static void DrawText(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawAnchor(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawImageAnchor(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawRule(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawBullet(XmHTMLWidget html, XmHTMLObjectTableElement data);
static void DrawFrame(XmHTMLWidget html, XmHTMLImage *image, int xs, int ys);

/*****
* Layout driving routines
*****/
static void SetText(XmHTMLWidget html, int *x, int *y,
	XmHTMLObjectTableElement start, XmHTMLObjectTableElement end,
	Boolean in_pre);
static void SetRule(XmHTMLWidget html, int *x, int *y, 
	XmHTMLObjectTableElement data);
static void SetTable(XmHTMLWidget html, int *x, int *y, 
	XmHTMLObjectTableElement data);
static void SetApplet(XmHTMLWidget html, int *x, int *y, 
	XmHTMLObjectTableElement data);
static void SetBlock(XmHTMLWidget html, int *x, int *y, 
	XmHTMLObjectTableElement data);
static void SetBullet(XmHTMLWidget html, int *x, int *y, 
	XmHTMLObjectTableElement data);

/*****
* Layout computation routines
*****/
static void ComputeTextLayout(XmHTMLWidget html, PositionBox *box,
	XmHTMLWord **words, int nstart, int *nwords, Boolean last_line);
static void ComputeTextLayoutPre(XmHTMLWidget html, PositionBox *box,
	XmHTMLWord **words, int nstart, int *nwords, Boolean last_line);

/*****
* Misc. drawing routines called by other drawing routines.
*****/
static void DrawLineBreak(XmHTMLWidget html, int *x, int *y, 
	XmHTMLObjectTableElement data);
static void DrawAnchorButton(XmHTMLWidget html, int x, int y, Dimension width,
	Dimension height, TGC top, TGC bottom);

/*****
* Various helper routines
*****/
static XmHTMLWord **getWords(XmHTMLObjectTableElement start,
	XmHTMLObjectTableElement end, int *nwords);

static XmHTMLWord **getWordsRtoL(XmHTMLObjectTableElement start,
	XmHTMLObjectTableElement end, int *nwords);

static void MakeTextOutline(XmHTMLWidget html, XmHTMLWord *words[],
	int word_start, int word_end, Dimension sw, int len, int line_len,
	int skip_id);

static void CheckAlignment(XmHTMLWidget html, XmHTMLWord *words[],
	int word_start, int word_end, int sw, int line_len, Boolean last_line,
	int skip_id);

static void AdjustBaseline(XmHTMLWord *base_obj, XmHTMLWord **words,
	int start, int end, int *lineheight, Boolean last_line);

static void AdjustBaselinePre(XmHTMLWord *base_obj, XmHTMLWord **words,
	int start, int end, int *lineheight, Boolean last_line);

#ifdef WITH_MOTIF
static void
TimerCB(XtPointer data, XtIntervalId *id);
#else
int
TimerCB(gpointer data);
#endif

/* 
* characters that must be flushed against a word. Can't use ispunct since
* that are all printable chars that are not a number or a letter.
*/
#define IS_PUNCT(c) (c =='.' || c == ',' || c == ':' || c == ';' || \
	c == '!' || c == '?')

/*** Private Variable Declarations ***/
static int line, last_text_line;
static int max_width;
static XmHTMLWord *baseline_obj;
static Boolean had_break;		/* indicates a paragraph had a break */
static XmHTMLWord** (*get_word_func)(XmHTMLObjectTableElement,
	XmHTMLObjectTableElement, int *);

#ifdef DEBUG
static int lines_done;
static int total_iterations;
#endif

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

	_XmHTMLDebug(5, ("paint.c: _XmHTMLPaint Start, paint_start: x = %i, "
		"y = %i\n", start->x, start->y));

	temp = start;

	while(temp != end)
	{
		_XmHTMLDebug(5, ("paint.c: _XmHTMLPaint, painting object %s\n",
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
	_XmHTMLDebug(5, ("paint.c: _XmHTMLPaint End\n"));
	return;
}

#define STORE_ANCHOR(DATA) { \
	if(DATA->text_data & TEXT_ANCHOR) \
	{ \
		/* save anchor data */ \
		for(i = 0 ; i < DATA->n_words; i++) \
		{ \
			/* sanity check */ \
			if(curr_anchor == html->html.anchor_words) \
			{ \
				_XmHTMLWarning(__WFUNC__(html, "_XmHTMLpaint"), \
					"I'm about to crash: exceeding anchor word count!"); \
				curr_anchor--; \
			} \
			/* copy worddata and adjust y position */ \
			html->html.anchors[curr_anchor] = DATA->words[i]; \
			if(DATA->words[i].type == OBJ_IMG) \
				html->html.anchors[curr_anchor].y =  DATA->words[i].y; \
			else \
				html->html.anchors[curr_anchor].y =  \
					DATA->words[i].y - DATA->words[i].font->xfont->ascent; \
			curr_anchor++; \
		} \
	} \
	if(DATA->text_data & TEXT_ANCHOR_INTERN) \
	{ \
		/* save named anchor location */ \
		html->html.named_anchors[named_anchor] = *DATA; \
		named_anchor++; \
	} \
}

/*****
* Name:			_XmHTMLComputeLayout
* Return Type:	void
* Description:	displays every formatted object on to the screen.
* In:
*	w:			Widget to display 
* Returns:
*	nothing.
*****/
void
_XmHTMLComputeLayout(XmHTMLWidget html)
{
	XmHTMLObjectTableElement temp, end;
	int x = 0, y = 0;
	int i, curr_anchor = 0, named_anchor = 0;

	_XmHTMLDebug(5, ("paint.c: _XmHTMLpaint Start\n"));

	html->html.paint_start = temp = html->html.formatted;
	html->html.paint_x = 0;
	html->html.paint_width = html->html.work_width + html->html.margin_width;

	line = last_text_line = 0;
	baseline_obj = (XmHTMLWord*)NULL;
	max_width = 0;
	had_break = False;
	x = html->html.margin_width;
	y = html->html.margin_height + html->html.default_font->xfont->ascent;

	/* select appropriate word collector */
	if(html->html.string_direction == TSTRING_DIRECTION_R_TO_L)
		get_word_func = getWordsRtoL;
	else
		get_word_func = getWords;

#ifdef DEBUG
	lines_done = 0;
	total_iterations = 0;
	_XmHTMLDebug(5, ("paint.c: _XmHTMLpaint:\n"
		"\tCore offset: %ix%i\n"
		"\tmargins: width = %i, height = %i\n"
		"\twidget offset: %ix%i\n",
		html->core.x, html->core.y, html->html.margin_width,
		html->html.margin_height, x, y));
#endif

	/* sanity check */
	if(temp == NULL)
		return;		/* fix 01/28/97-06, kdh */

	_XmHTMLFullDebug(5, ("paint.c: _XmHTMLpaint, x = %d, y = %d \n", x, y));

	while(temp != NULL)
	{
		switch(temp->object_type)
		{
			/*
			* To get a proper text layout, we need to do the layout for
			* whole blocks of text at a time.
			*/
			case OBJ_TEXT:
				for(end = temp; end != NULL && end->object_type == OBJ_TEXT; 
					end = end->next);

				/* go and do text layout */
				SetText(html, &x, &y, temp, end, False);

				/* make temp point at the real end of this block */
				for(end = temp; end->next != NULL && 
					end->next->object_type == OBJ_TEXT; 
					end = end->next);

				for(; temp != NULL && temp->object_type == OBJ_TEXT; 
					temp = temp->next)
				{
					STORE_ANCHOR(temp);
				}
				/* back up one element */
				temp = end;
				break;

			case OBJ_PRE_TEXT:
				for(end = temp; end != NULL && end->object_type == OBJ_PRE_TEXT;
					end = end->next);

				/* go and do text layout */
				SetText(html, &x, &y, temp, end, True);

				/* make temp point at the real end of this block */
				for(end = temp; end->next != NULL && 
					end->next->object_type == OBJ_PRE_TEXT; 
					end = end->next);

				for(; temp != NULL && temp->object_type == OBJ_PRE_TEXT; 
					temp = temp->next)
				{
					STORE_ANCHOR(temp);
				}
				/* back up one element */
				temp = end;
				break;
			case OBJ_BULLET:
				SetBullet(html, &x, &y, temp);
				break;
			case OBJ_HRULE:
				SetRule(html, &x, &y, temp);
				break;
			case OBJ_TABLE:
				SetTable(html, &x, &y, temp);
				DrawLineBreak(html, &x, &y, temp);
				break;
			case OBJ_APPLET:
				SetApplet(html, &x, &y, temp);
				DrawLineBreak(html, &x, &y, temp);
				break;
			case OBJ_BLOCK:
				SetBlock(html, &x, &y, temp);
				DrawLineBreak(html, &x, &y, temp);
				break;
			case OBJ_NONE:
				SetBlock(html, &x, &y, temp);
				/* empty named anchors can cause this */
				if(temp->text_data & TEXT_ANCHOR_INTERN)
				{
					/* save named anchor location */
					html->html.named_anchors[named_anchor] = *temp;
					named_anchor++;
				}
				break;
			default:
				_XmHTMLWarning(__WFUNC__(html, "_XmHTMLpaint"), 
					"Unknown object type!");
		}
		/*
		* Store end command for re-painting.
		*/
		if((y - temp->height > html->html.work_height) || 
			(y > html->html.work_height))
			html->html.paint_end = temp;
		if(x > max_width)
			max_width = x;
		temp = temp->next;
	}
	/***** 
	* Now adjust width of the anchors.
	* If the current anchor word and the next are on the same line, and these
	* words belong to the same anchor, the width of the current anchor word 
	* is adjusted so it will seem to be continue across the whole line when
	* the mouse pointer is moved over an anchor.
	* We can adjust the width field directly because the html.anchors field is
	* only used for anchor lookup, not for rendering (see the note at
	* AdjustWordWidth below).
	*****/
	for(i = 0 ; i < html->html.anchor_words; i++)
			html->html.anchors[i].x = html->html.anchors[i].self->x;
	for(i = 0 ; i < html->html.anchor_words; i++)
	{
		if((html->html.anchors[i].owner == html->html.anchors[i+1].owner) &&
			(html->html.anchors[i].line == html->html.anchors[i+1].line))
		{
			html->html.anchors[i].width = 
				html->html.anchors[i+1].x - html->html.anchors[i].x + 2;
		}
	}
	/* 
	* store total height for this document. We add the marginheight and
	* font descent to get the text nicely centered.
	*/
	html->html.formatted_height = 
		y + html->html.margin_height + html->html.default_font->xfont->descent; 

	/* Preferred width for this document, includes horizontal margin once. */
	html->html.formatted_width = max_width;

	/* store new maximum line number */
	html->html.nlines = line;

	/* 
	* Never adjust top_line, scroll_x or scroll_y. This will make the
	* widget jump to the line in question and start drawing at the scroll_x 
	* and scroll_y positions.
	*/

	_XmHTMLFullDebug(5, ("paint.c: _XmHTMLpaint, x_max = %d, y_max = %d.\n",
		html->html.formatted_width, html->html.formatted_height));
	_XmHTMLFullDebug(5, ("paint.c: _XmHTMLpaint, stored %i/%i anchor words\n",
		curr_anchor, html->html.anchor_words));

	/* now process any images with an alpha channel (if any) */
	if(html->html.delayed_creation)
		_XmHTMLImageCheckDelayedCreation(html);

#ifdef DEBUG
	/* prevent divide by zero */
	if(lines_done)
	{
		_XmHTMLDebug(5, ("outlining stats\n"));
		_XmHTMLDebug(5, ("\tlines done: %i\n", lines_done));
		_XmHTMLDebug(5, ("\ttotal iterations: %i\n", total_iterations));
		_XmHTMLDebug(5, ("\taverage iterations per line: %f\n", 
			(float)(total_iterations/(float)lines_done)));
	}
#endif

	_XmHTMLDebug(5, ("paint.c: _XmHTMLpaint End\n"));
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
* Name: 		MakeTextOutline
* Return Type: 	void
* Description: 	adjusts interword spacing to produce fully outlined text.
*				Outlining is done on basis of the longest words.
* In: 
*	start:		starting text element
*	end:		ending text element
*	w_start:	index in starting text element
*	w_end:		index in ending text element
*	sw:			width of a space in the current font
*	len:		current line length for this text
*	line_len:	maximum length of a line.
* Returns:
*	nothing, but *items contains updated delta fields to reflect the 
*	required interword spacing.
* Note:
*	Words that start with a punctuation character are never adjusted,
*	they only get shoved to the right.
*	This routine could be much more efficient if the text to be outlined
*	would be sorted.
*****/
static void
MakeTextOutline(XmHTMLWidget html, XmHTMLWord *words[], int word_start, 
	int word_end, Dimension sw, int len, int line_len, int skip_id)
{
	int word_len, longest_word = 0, nspace = 0, i, j, num_iter = 0;

	/* See how many spaces we have to add */
	nspace = (int)((line_len - len)/(sw == 0 ? (sw = 3) : sw));

	/* 
	* last line of a block or no spaces to add. Don't adjust it.
	* nspace can be negative if there are words that are longer than
	* the available linewidth
	*/
	if(nspace < 1) 
		return;

	/* we need at least two words if we want this to work */
	if((word_end - word_start) < 2)
		return;

	/* no hassling for a line with two words, fix 07/03/97-02, kdh */
	if((word_end - word_start) == 2)
	{
		/* just flush the second word to the right margin */
		words[word_start+1]->x += nspace*sw;
		return;
	}

	/* pick up the longest word */
	for(i = word_start; i < word_end; i++)
	{
		if(i == skip_id)
			continue;
		if(words[i]->len > longest_word)
			longest_word = words[i]->len;
	}

	word_len = longest_word;

	/* adjust interword spacing until we run out of spaces to add */
	while(nspace && num_iter < MAX_OUTLINE_ITERATIONS)
	{
		/* walk all words in search of the longest one */
		for(i = word_start ; i < word_end && nspace; i++, num_iter++)
		{
			if(i == skip_id)
				continue;
			/* Found! */
			if(words[i]->len == word_len && 
					!IS_PUNCT(*(words[i]->word)) &&
					!(words[i]->spacing & TEXT_SPACE_NONE))
			{
				/* see if we are allowed to shift this word */
				if(!(words[i]->spacing & TEXT_SPACE_TRAIL) ||
					!(words[i]->spacing & TEXT_SPACE_LEAD))
					continue;

				/*****
				* Add a leading space if we may, but always shift all 
				* following words to the right.
				*
				* fix 07/03/97-01, kdh
				******/
				if(words[i]->spacing & TEXT_SPACE_LEAD && i != word_start)
				{
					for(j = i; j < word_end; j++)
					{
						if(j == skip_id)
							continue;
						words[j]->x += sw;
					}
					nspace--;
				}
				if(nspace)
				{
					for(j = i + 1; j < word_end; j++)
					{
						if(j == skip_id)
							continue;
						words[j]->x += sw;
					}

					/* we have only added a space if this is true */
					if(j != i+1)
						nspace--;
				}
			}
		}
		num_iter++;
		/* move on to next set of words eligible for space adjustement. */
		word_len = (word_len == 0 ? longest_word : word_len - 1);
	}
	if(num_iter == MAX_OUTLINE_ITERATIONS)
	{
		_XmHTMLWarning(__WFUNC__(NULL, "MakeTextOutline"),
			"Text justification: bailing out after %i iterations\n"
			"    (line %i of input)", MAX_OUTLINE_ITERATIONS, 
			words[word_start]->owner->object->line);
	}
#ifdef DEBUG
	lines_done++;
	total_iterations += num_iter;
#endif
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
			_XmHTMLDebug(5, ("paint.c: DrawText, skipping %s, outside "
				"vertical range.\n", tmp->word));
			continue;
		}

		if(html->html.paint_x > tmp->x + tmp->width ||
			html->html.paint_width < tmp->x)
		{
			_XmHTMLDebug(5, ("paint.c: DrawText, skipping %s, outside "
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

	_XmHTMLDebug(5, ("paint.c: DrawAnchor, anchor contains %i words\n",
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
		_XmHTMLDebug(5, ("paint.c: DrawAnchor, allocating a word array for %i "
			"words\n", nwords));

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
		_XmHTMLDebug(5, ("paint.c: DrawAnchor, not allocating a word array, "
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

		_XmHTMLFullDebug(5, ("paint.c: painting anchor, x = %i, y = %i\n",
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
					TFontStruct *my_font = words [j].font->xfont;
					
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

/*****
* Name: 		CheckAlignment
* Return Type: 	void
* Description: 	adjusts x-position of every word to reflect requested
*				alignment.
* In: 
*	w:			XmHTML widget
*	start:		starting text element
*	end:		ending text element
*	word_start:	starting word index in the start element.
*	sw:			current space width.
*	last_line:	indicates this is the last line in a text block;
* Returns:
*	nothing, but every word in start and end (and any object(s) in between
*	them) that belongs to the same line is updated to reflect the alignment.
*	This routine just returns if the current alignment matches the default
*	alignment.
*****/
static void
CheckAlignment(XmHTMLWidget html, XmHTMLWord *words[], int word_start,
	int word_end, int sw, int line_len, Boolean last_line, int skip_id)
{
	int i, width, offset;

	/* sanity */
	if(word_end < 1)
		return;

	/* total line width occupied by these words */
	width = words[word_end-1]->x + words[word_end-1]->width - 
			words[word_start]->x;

	_XmHTMLFullDebug(5, ("paint.c: CheckAlignment, start word: %s, index %i, "
		"end word: %s, index %i, width = %i\n", 
		words[word_start]->word, word_start, 
		words[word_end-1]->word, word_end-1, width));

	/* get amount of spacing to play with */
	width += words[word_start]->owner->ident;

	switch(words[word_start]->owner->halign)
	{
		case XmHALIGN_RIGHT:
			offset = line_len - width;
			break;
		case XmHALIGN_CENTER:
			offset = (line_len - width)/2;
			break;
		case XmHALIGN_LEFT:		/* layout computation is always left-sided */
			offset = 0;
			break;
		case XmHALIGN_OUTLINE:
			/* sw == -1 when used for <pre> text */
			if(html->html.enable_outlining && !last_line && sw != -1)
			{
				MakeTextOutline(html, words, word_start, word_end, sw, width,
					line_len, (word_start < skip_id ? skip_id : -1));
				offset = 0;
				break;
			}
			/* fall thru */
		case XmHALIGN_NONE:
		default:
			/* use specified alignment */
			switch(html->html.alignment)
			{
				case TALIGNMENT_END:
					offset = line_len - width;
					break;
				case TALIGNMENT_CENTER:
					offset = (line_len - width)/2;
					break;
				case TALIGNMENT_BEGINNING:
				default:
					offset = 0;
					break;
			}
			break;
	}
	/*****
	* only adjust with a positive offset. A negative offset indicates
	* that the current width is larger than the available width.
	* Will ignore alignment setting for pre text that is wider than the
	* available window width.
	*****/
	if(offset <= 0)
		return;
	for(i = word_start; i < word_end; i++)
		words[i]->x += offset;
}

/*****
* Name: 		getWords
* Return Type: 	XmHTMLWord**
* Description: 	creates an array containing all OBJ_TEXT elements between
*				start and end.
* In: 
*	start:		element at which to start collecting words;
*	end:		element at which to end collecting words;
*	nwords:		no of words collected. Updated upon return;
* Returns:
*	an array of XmHTMLWord.
* Note:
*	This routine is used by the text layout routines to keep layout computation
*	managable.
*****/
static XmHTMLWord**
getWords(XmHTMLObjectTableElement start, XmHTMLObjectTableElement end,
	int *nwords)
{
	static XmHTMLWord **words;
	XmHTMLObjectTableElement tmp;
	int i, k, cnt = 0;

	for(tmp = start; tmp != end ; tmp = tmp->next)
		cnt += tmp->n_words;

	words = (XmHTMLWord**)calloc(cnt, sizeof(XmHTMLWord*));

	for(tmp = start, k = 0; tmp != end; tmp = tmp->next)
	{
		for(i = 0 ; i < tmp->n_words; i++)
		{
			/* store word ptr & reset position to zero */
			words[k] = &(tmp->words[i]);
			words[k]->x = 0;
			words[k]->y = 0;
			words[k++]->line = 0;
		}
	}

	*nwords = cnt;
	return(words);
}

/*****
* Name: 		getWordsRtoL
* Return Type: 	XmHTMLWord**
* Description: 	see getWords
* In: 
*	see getWords
* Returns:
*	see getWords
* Note:
*	This routines reverses the objects to properly accomodate right-to-left
*	layout.
*	This is a seperate routine for performance reasons.
*****/
static XmHTMLWord**
getWordsRtoL(XmHTMLObjectTableElement start, XmHTMLObjectTableElement end,
	int *nwords)
{
	static XmHTMLWord **words;
	XmHTMLObjectTableElement tmp;
	int i, k, cnt = 0;

	for(tmp = start; tmp != end ; tmp = tmp->next)
		cnt += tmp->n_words;

	words = (XmHTMLWord**)calloc(cnt, sizeof(XmHTMLWord*));

	/* sanity */
	if(end == NULL)
		for(end = start; end->next != NULL; end = end->next);
	for(tmp = end->prev, k = 0; tmp != start->prev; tmp = tmp->prev)
	{
		for(i = 0; i < tmp->n_words; i++)
		{
			/* store word ptr & reset position to zero */
			words[k] = &(tmp->words[i]);
			words[k]->x = 0;
			words[k]->y = 0;
			words[k++]->line = 0;
		}
	}
	*nwords = cnt;
	return(words);
}

/*****
* Name:			AdjustBaseline
* Return Type: 	void
* Description: 	adjusts the baseline for each word between start and end.
* In: 
*	base_obj:	object which controls the baseline offset;
*	**words:	array of all words being laid out;
*	start:		starting word index;
*	end:		ending word index;
*	lineheight:	new lineheight (= spacing between to consecutive lines of text)
*	last_line:	True when called for the last line in paragraph. Used for
*				computing the proper vertical offset between the end of this
*				paragraph and the object following it.
* Returns:
*	nothing, but all words between start and end have their baseline adjusted.
*****/
static void
AdjustBaseline(XmHTMLWord *base_obj, XmHTMLWord **words, int start, int end, 
	int *lineheight, Boolean last_line)
{
	int i, y_offset = 0;

	if(base_obj->type == OBJ_IMG)
	{
		switch(base_obj->image->align)
		{
			case XmVALIGN_MIDDLE:
				y_offset = (*lineheight - base_obj->font->xfont->ascent)/2.;
				/* adjust return value from SetText */
				/* fix 07/03/97-04, kdh */
				if(last_line && base_obj != words[end-1])
					*lineheight = y_offset;
				break;
			case XmVALIGN_BASELINE:
			case XmVALIGN_BOTTOM:
				y_offset = *lineheight - base_obj->font->xfont->ascent;
				*lineheight += base_obj->font->xfont->ascent/2.;
				break;
			case XmVALIGN_TOP:
			default:
				break;
		}
	}
	else if(base_obj->type == OBJ_FORM)
	{
		/* fix 07/04/97-01, kdh */
		/* form elements are always aligned in the middle */
		y_offset = (*lineheight - base_obj->font->xfont->ascent)/2.;

		/* But they move the baseline down */
		*lineheight += base_obj->font->xfont->ascent/2.;
	}
	else if(!last_line) /* sanity */
		*lineheight = words[end]->height;

	/*****
	* Now adjust the baseline for every word on this line.
	* Split into a y_offset and non y_offset part for performance reasons.
	*****/
	if(y_offset)
	{
		for(i = start; i < end; i++)
		{
			/* only move text objects */
			if(words[i]->type == OBJ_TEXT)
				words[i]->y += y_offset;
			words[i]->base = base_obj;
		}
	}
	else
	{
		for(i = start; i < end; i++)
			words[i]->base = base_obj;
	}
}

#define UPDATE_WORD(W) { \
	/* images and forms need to have the font ascent substracted to get a */ \
	/* proper vertical alignment. */ \
	(W)->line = line; \
	(W)->x = x_pos + e_space; \
	if((W)->type != OBJ_TEXT) \
	{ \
		(W)->y = y_pos + (W)->owner->y_offset - (W)->font->xfont->ascent; \
		have_object = True; \
	} \
	/* regular text, no additional adjustment required */ \
	else (W)->y = y_pos + (W)->owner->y_offset; \
	x_pos = (W)->x + (W)->width + (W)->owner->x_offset; \
}

/*****
* Name: 		SetText
* Return Type: 	void
* Description: 	main text layout driver;
* In: 
*	html:		XmHTMLWidget id;
*	*x:			initial x position, updated to new x position upon return;
*	*y:			initial y position, updated to new y position upon return;
*	start:		starting object id;
*	end:		ending object id;
* Returns:
*	nothing
*****/
static void
SetText(XmHTMLWidget html, int *x, int *y, XmHTMLObjectTableElement start,
	XmHTMLObjectTableElement end, Boolean in_pre)
{
	XmHTMLWord **words;
	int nwords, word_start, i;
	PositionBox box;
	XmHTMLObjectTableElement current = NULL;

	/***** 
	* to make it ourselves _much_ easier, put all the words starting from
	* start and up to end in a single block of words.
	*****/
	words = get_word_func(start, end, &nwords);

	/* sanity */
	if(nwords == 0)
		return;

	/*****
	* Set up the initial PositionBox to be used for text layout.
	*****/
	box.x      = *x;
	box.y      = *y;
	box.left   = html->html.margin_width + start->ident;
	box.right  = html->html.work_width;
	box.height = -1;
	box.width  = box.right - html->html.margin_width;

	/* do text layout */
	if(in_pre)
		ComputeTextLayoutPre(html, &box, words, 0, &nwords, True);
	else
		ComputeTextLayout(html, &box, words, 0, &nwords, True);

	/* Now update all ObjectTable elements for these words */
	current = NULL;
	for(i = 0; i < nwords; i++)
	{
		if(current != words[i]->owner)
		{
			word_start = i;
			current    = words[i]->owner;
			current->x = words[i]->x;
			current->width = words[i]->width;
			current->line  = words[i]->line;
			/*****
			* To get correct screen updates, the vertical position and height
			* of this object are that of the baseline object.
			*****/
			current->y = words[i]->base->y;
			current->height = words[i]->base->height;

			/* get index of last word on the first line of this object. */
			for(; i < word_start + current->n_words-1 &&
				words[i]->line == words[i+1]->line; i++);
			/*****
			* Total line width is given by end position of last word on this
			* line minus the starting position of the first word on this line.
			* (ensures we take interword spacing into account)
			*****/
			current->width = words[i]->x + words[i]->width - current->x;

			/*****
			* Lineheight of this object is given by vertical position of last
			* word minus vertical position of first word in this block. Only
			* valid when this object spans multiple lines.
			*****/
			if(i != word_start + current->n_words-1)
			{
				current->height = words[word_start + current->n_words - 1]->y -
					words[word_start]->y;
			}
			else if(in_pre && words[i]->base->spacing)
			{
				/* vertical line spacing in preformatted text */ 
				current->height = ((int)words[i]->base->spacing) *
						words[i]->base->font->height;
			}
			/* and set i to last word of this object */
			i = word_start + current->n_words-1;
			_XmHTMLDebug(5, ("paint.c: SetText, object data: x = %d, y = %d, "
				"width = %d, height = %d, line = %i\n", current->x,
				current->y, current->width, current->height, current->line));
		}
	}

	/* and update return values */
	*x = box.x;
	*y = box.y;

	/* free words */
	free(words);
}

/*****
* Name: 		ComputeTextLayout
* Return Type: 	void
* Description: 	orders the given textdata into single lines, breaking and
*				moving up to the next line if necessary.
* In: 
*	w:			widget for which to do this;
*	box:		bounding box to be used for computing text layout;
*	words:		array of words to be laid out;
*	nstart:		starting idx;
*	nwords:		ending idx, can be updated upon return;
*	last_line:	indicates that this routine is called for the the last line in
*				a paragraph.
* Returns:
*	nothing
* Note:
*	This function does the layout of complete paragraphs at once.
*	A paragraph is given by all text elements between start and end.
*
*	This is a rather complex routine. Things it does are the following:
*	- considers images, HTML form members and text as the same objects;
*	- adjusts baseline according to the highest object on a line;
*	- adjusts space width if font changes;
*	- performs horizontal alignment;
*	- performs text outlining if required;
*	- glues words together if required (interword spacing);
*****/
static void
ComputeTextLayout(XmHTMLWidget html, PositionBox *box, XmHTMLWord **words,
	int nstart, int *nwords, Boolean last_line)
{
	XmHTMLfont *basefont, *font;
	XmHTMLWord *base_obj;
	Cardinal x_pos, y_pos;
	int i, sw, e_space = 0, word_start, word_width;
	int lineheight = 0, p_height = 0;
	Boolean have_object = False, first_line = True, done = False;
	Boolean in_line = True;
	int skip_id = -1, left, right, width, height;

	/* initial offsets */
	x_pos  = box->x;
	y_pos  = box->y;
	left   = box->left;
	right  = box->right;
	width  = box->width;
	height = box->height;

	basefont = font = words[nstart]->font;
	/* interword spacing */
	e_space = sw = font->isp;

	/*****
	* Proper baseline continuation of lines consisting of words with different
	* properties (font, fontstyle, images, form members or anchors) require us
	* to check if we are still on the same line. If we are, we use the baseline
	* object of that line. If we are on a new line, we take the first word of
	* this line as the baseline object.
	*****/
	if(!baseline_obj)
		base_obj = words[nstart];
	else
		base_obj = (last_text_line == line ? baseline_obj : words[nstart]);

	/* lineheight always comes from the current baseline object */
	lineheight = base_obj->height;

	word_start = nstart;

	/*****
	* Text layout:
	* we keep walking words until we are about to exceed the available
	* linewidth. When we are composing a line in this way, we keep track
	* of the highest word (which will define the maximum lineheight).
	* If a linefeed needs to be inserted, the lineheight is added to
	* every word for a line. We then move to the next line (updating the
	* vertical offset as we do) and the whole process repeats itself.
	*****/
	for(i = nstart; i < *nwords && !done; i++)	
	{
		/*****
		* We must flow text around a left-aligned image. First finish the
		* the current line, then adjust the left margin and available
		* linewidth and the height we should use.
		* We can only honor this attribute if the width of this image is
		* less than the available width.
		* Multiple left/right aligned images aren't supported (yet).
		*****/
		if(words[i]->type == OBJ_IMG &&
			(words[i]->image->align == XmHALIGN_LEFT ||
			words[i]->image->align == XmHALIGN_RIGHT))
		{
			if(skip_id == -1 && words[i]->width < width)
			{
				skip_id = i;
				/* we are already busy with a line, finish it first */
				if(in_line)
					continue;
				/* start of a line, just proceed */
			}
		}
		in_line = True;	/* we are busy with a line of text */

		/* get new space width if font changes */
		if(font != words[i]->font)
		{
			font = words[i]->font;
			sw = font->isp;		/* new interword spacing */

			/*****
			* If this font is larger than the current font it will become
			* the baseline font for non-text objects.
			*****/
			if(font->lineheight > basefont->lineheight)
				basefont = font;
		}

		/*****
		* Sigh, need to check if we may break words before we do the
		* check on current line width: if the current word doesn't have
		* a trailing space, walk all words which don't have a leading
		* and trailing space as well and end if we encounter the first word
		* which *does* have a trailing space. We then use the total width
		* of this word to check against available line width.
		*****/
		if(!(words[i]->spacing & TEXT_SPACE_TRAIL) && 
			i+1 < *nwords && !(words[i+1]->spacing & TEXT_SPACE_LEAD))
		{
			int j = i+1;
			word_width = words[i]->width;
			while(j < *nwords)
			{
				if(!(words[j]->spacing & TEXT_SPACE_LEAD))
					word_width += words[j]->width;

				/* see if this word has a trail space and the next a leading */
				if(!(words[j]->spacing & TEXT_SPACE_TRAIL) && 
					j+1 < *nwords && !(words[j+1]->spacing & TEXT_SPACE_LEAD))
					j++;
				else
					break;
			}
		}
		else
			word_width = words[i]->width;

		/* Check if we are about to exceed the viewing width */
		if(i && x_pos + word_width + e_space >= right)
		{
			/*****
			* set font of non-text objects to the largest font of the
			* text objects (required for proper anchor drawing)
			*****/
			if(base_obj->type != OBJ_TEXT)
				base_obj->font = basefont;
			/* adjust baseline for all words on the current line */
			AdjustBaseline(base_obj, words, word_start, i, &lineheight, False);

			/* Adjust for alignment */
			CheckAlignment(html, words, word_start, i, sw, width, False,
				skip_id);

			/* increment absolute height */
			y_pos += lineheight;

			/* update maximum box width */
			if(x_pos > max_width)
				max_width = x_pos;

			x_pos = left;
			line++;
			word_start  = i;		/* next word starts on a new line */
			lineheight  = words[i]->height;
			base_obj    = words[i];
			have_object = False;	/* object has been done */
			first_line  = False;	/* no longer the first line */
			in_line     = False;	/* done with current line */

			_XmHTMLFullDebug(5, ("paint.c: ComputeTextLayout, linefeed, "
				"x = %d, y = %d.\n", x_pos, y_pos));

			/* line is finished, set all margins for proper text flowing */
			if(skip_id != -1)
			{
				/* start of text flowing */
				if(box->height == -1)
				{
					/* save all info for this word */
					words[skip_id]->line = line;
					have_object = True;
					words[skip_id]->y = y_pos +
						words[skip_id]->owner->y_offset -
						words[skip_id]->font->xfont->ascent;

					/* this word sets the baseline for itself */
					words[skip_id]->base = words[skip_id];

					/* set appropriate margins */
					if(words[skip_id]->image->align == XmHALIGN_RIGHT)
					{
						/* flush to the right margin */
						words[skip_id]->x = right - words[skip_id]->width;
						right = words[skip_id]->x;
					}
					else
					{
						/*****
						* Flush to the left margin, it's the first word on
						* this line, so no leading space is required.
						*****/
						words[skip_id]->x = x_pos;
						x_pos = words[skip_id]->x + words[skip_id]->width;
						left = x_pos + e_space;
					}
					p_height = 0;
					box->height = words[skip_id]->height;
					width = box->width - words[skip_id]->width - sw - e_space;
				}
				else /* increment height of this paragraph */
					p_height += lineheight;

				/*****
				* If this is True, we are at the bottom of the image
				* Restore margins and continue.
				*****/
				if(p_height >= box->height)
				{
					skip_id = -1;
					box->height = -1;
					left  = box->left;
					right = box->right;
					width = box->width;
					x_pos = box->x;
				}
			}
		}

		/* save maximum lineheight */
		if(lineheight < words[i]->height)
		{
			/*****
			* Shift all words already placed on this line down. Don't do it
			* for the first line in a paragraph and if this word is actually
			* an image as this is already taken into account (paragraph
			* spacing)
			******/
			if(!first_line && words[i]->type != OBJ_IMG)
			{
				/* fix 07/03/97-03, kdh */
				int k = word_start;

				/* new vertical position of all words in the current line */
				y_pos += (words[i]->height - lineheight);

				/* shift 'em down. No need to check skip_id */
				for(; k < i; k++)
					words[k]->y = y_pos;
			}
			/* save new lineheight */
			lineheight = words[i]->height;
			base_obj   = words[i];
		}

		/*****
		* Interword Spacing.
		* 	1. word starts at beginning of a line, don't space it at all.
		*	   (box->left includes indentation as well)
		* 	2. previous word does not have a trailing spacing:
		* 		a. current word does have leading space, space it.
		*		b. current word does not have a leading space, don't space it.
		* 	3. previous word does have a trailing space:
		*		a. always space current word.
		* 	4. previous word does not have any spacing:
		*		a. current word has leading space, space it.
		*		b. current word does not have a leading space, don't space it.
		* Note: if the previous word does not have a trailing space and the
		*	current word does not have a leading space, these words are
		*	``glued'' together.
		*****/
		e_space = 0;
		if(i != 0 && x_pos != left)
		{
			if(!(words[i-1]->spacing & TEXT_SPACE_TRAIL))
			{
				if(words[i]->spacing & TEXT_SPACE_LEAD)
					e_space = sw;
			}
			else if(words[i-1]->spacing & TEXT_SPACE_TRAIL)
				e_space = sw;
			else if(words[i]->spacing & TEXT_SPACE_LEAD)
				e_space = sw;

			/* additional end-of-line spacing? */
			if(e_space && words[i]->word[words[i]->len-1] == '.')
				e_space += font->eol_sp;
		}
		/*****
		* save linenumber, x and y positions for this word or for
		* multiple words needing to be ``glued'' together.
		*****/
		if(!(words[i]->spacing & TEXT_SPACE_TRAIL) && 
			i+1 < *nwords && !(words[i+1]->spacing & TEXT_SPACE_LEAD))
		{
			/* first word must take spacing into account */
			UPDATE_WORD(words[i]);
			/* all other words are glued, so no spacing! */
			e_space = 0;
			i++;
			while(i < *nwords)
			{
				/* don't take left/right flushed image into account */
				if(i == skip_id)
					continue;
				/* connected word, save line, x and y pos. */
				if(!(words[i]->spacing & TEXT_SPACE_LEAD))
					UPDATE_WORD(words[i])

				/* this word has a trailing and the next a leading space? */
				if(!(words[i]->spacing & TEXT_SPACE_TRAIL) && 
					i+1 < *nwords && !(words[i+1]->spacing & TEXT_SPACE_LEAD))
					i++;
				else
					break;
			}
		}
		else /* save line, x and y pos for this word. */
			UPDATE_WORD(words[i])
	}
	/*****
	* If we've got an image left, update it. We only have an image left if
	* it's position hasn't been updated in the above loop, it will be
	* positioned otherwise, but we ran out of text before we reached the
	* box's height. So we need to update y_pos to move the baseline properly
	* down. The box itself isn't restored as we have to check the alignment
	* for this last line as well.
	*****/
	if(skip_id != -1)
	{
		if(words[skip_id]->x == 0 && words[skip_id]->y == 0)
		{
			UPDATE_WORD(words[skip_id]);
		}
		else	/* update y_pos */
			y_pos += box->height - p_height;
	}

	/*****
	* How do we know we are at the end of this block of text objects??
	* If the calling routine set last_line to True, we know we are done
	* and we can consider the layout computation done.
	* If last_line is False, we can be sure that other text is coming so
	* we must continue layout computation on the next call to this routine.
	* If we haven't finished computing the layout for all words, we were
	* flowing text around an object (currently only images), and we need
	* to adjust the number of words done *and* be able to restart computation
	* on the next call to this routine.
	*****/
	if(i == *nwords)
	{
		if(last_line)
			done = True;
		else
			done = False;
	}
	else if(done)
	{
		*nwords = i;
		done = False;
	}

	/* also adjust baseline for the last line */
	if(base_obj->type != OBJ_TEXT)
		base_obj->font = basefont;

	AdjustBaseline(base_obj, words, word_start, i, &lineheight, done);

	/* also adjust alignment for the last line */
	CheckAlignment(html, words, word_start, *nwords, sw, box->width, done,
		skip_id);

	/* non-text objects (images & form members) move the baseline downward */
	if(have_object)
	{
		box->y = y_pos + lineheight;
		had_break = True;
	}
	else
		box->y = y_pos;
	box->x = x_pos;

	/* final check, required for very long words/images */
	if(x_pos > max_width)
		max_width = x_pos;

	/* last text line and baseline object for this piece of text */
	last_text_line = line;
	baseline_obj   = base_obj;

	/*****
	* If we haven't done a full line, we must increase linenumbering
	* anyway as we've inserted a linebreak.
	*****/
	if(first_line)
		line++;
}

/*****
* Name:			AdjustBaselinePre
* Return Type: 	void
* Description: 	see AdjustBaseline
* In: 
*
* Returns:
*	nothing.
*****/
static void
AdjustBaselinePre(XmHTMLWord *base_obj, XmHTMLWord **words, int start, int end, 
	int *lineheight, Boolean last_line)
{
	int i, y_offset = 0;

	if(base_obj->type == OBJ_IMG)
	{
		switch(base_obj->image->align)
		{
			case XmVALIGN_MIDDLE:
				y_offset = (*lineheight - base_obj->font->xfont->ascent)/2.;
				/* adjust return value from SetText */
				/* fix 07/03/97-04, kdh */
				if(last_line && base_obj != words[end-1])
					*lineheight = y_offset;
				break;
			case XmVALIGN_BASELINE:
			case XmVALIGN_BOTTOM:
				y_offset = *lineheight - base_obj->font->xfont->ascent;
				*lineheight += 0.5*base_obj->font->xfont->ascent;
				break;
			case XmVALIGN_TOP:
			default:
				break;
		}
	}
	else if(base_obj->type == OBJ_FORM)
	{
		/* fix 07/04/97-01, kdh */
		/* form elements are always aligned in the middle */
		y_offset = 0.5*(*lineheight - base_obj->font->xfont->ascent);

		/* But they move the baseline down */
		*lineheight += 0.5*base_obj->font->xfont->ascent;
	}
	else  /* sanity */
		return;

	/* Now adjust the baseline offset for every word on this line. */
	if(y_offset)
	{
		for(i = start; i < end; i++)
		{
			/* only move text objects */
			if(words[i]->type == OBJ_TEXT)
				words[i]->y += y_offset;
		}
	}
}

/*****
* Name:			ComputeTextLayoutPre
* Return Type: 	void
* Description: 	main text layout engine for preformatted text.
* In: 
*	html:		XmHTMLWidget id;
*	box:		bounding box to be used for computing text layout;
*	words:		array of words to be laid out;
*	nstart:		starting idx;
*	nwords:		ending idx, can be updated upon return;
*	last_line:	indicates that this routine is called for the the last line in
*				a paragraph.
* Returns:
*	nothing, but all words between start and end now have a screen position
*	and a line number.
*****/
static void
ComputeTextLayoutPre(XmHTMLWidget html, PositionBox *box, XmHTMLWord **words,
	int nstart, int *nwords, Boolean last_line)
{
	XmHTMLfont *basefont, *font;
	XmHTMLWord *base_obj;
	Cardinal x_pos, y_pos;
	int i, word_start;
	int lineheight = 0, y_offset, p_height = 0;
	Boolean have_object = False, first_line = True, done = False;

	/* initial offsets */
	x_pos = box->x;
	y_pos = box->y;

	/*****
	* Baseline stuff. Always initialize to the first word of this para.
	*****/
	base_obj   = words[0];
	basefont   = font = words[0]->font;
	word_start = 0;
 	y_offset   = basefont->height;

	if(base_obj->spacing != 0)
		lineheight = y_offset;
	else
		lineheight = base_obj->height;

	/*****
	* Text layout:
	* we keep walking words until we are about to insert a newline.
	* Newlines are marked by words width a non-zero spacing field.
	* When we are composing a line in this way, we keep track
	* of the highest word (which will define the maximum lineheight).
	* If a linefeed needs to be inserted, the lineheight is added to
	* every word for a line. We then move to the next line (updating the
	* vertical offset as we do) and the whole process repeats itself.
	*****/
	for(i = nstart; i < *nwords && !done; i++)	
	{
		/* compute new line spacing if font changes */
		if(font != words[i]->font)
		{
			font = words[i]->font;
			/*
			* If this font is larger than the current font it will become
			* the baseline font for non-text objects.
			* Must use maxbounds fontheight for fixed width fonts.
			*/
			if(font->lineheight > basefont->lineheight)
			{
				basefont = font;
 				y_offset = basefont->lineheight;
			}
		}

		/* check maximum lineheight */
		if(lineheight < words[i]->height)
		{
			/* this becomes the new baseline object */
			base_obj = words[i];
			/*****
			* Shift all words already placed on this line down. Don't do
			* it for the first line in a paragraph and if this word is
			* actually an image as this is already taken into account
			* (paragraph spacing)
			******/
			if(!first_line && words[i]->type != OBJ_IMG)
			{
				/* fix 07/03/97-03, kdh */
				int k = word_start;	/* idx of first word on this line */

				/* new vertical position of all words in the current line */
				y_pos += (words[i]->height - lineheight);

				/* shift 'em down */
				for(; k < i; k++)
				{
					words[k]->y = y_pos;
					words[k]->base = base_obj;
				}
			}
			/* Store new lineheight */
			if(words[i]->spacing != 0)
				lineheight = ((int)words[i]->spacing) * y_offset;
			else
				lineheight = words[i]->height;
		}
		/*****
		* save line, x and y pos for this word.
		* We don't do any interword spacing for <PRE> objects, they
		* already have it. Images and forms need to have the font ascent
		* substracted to get a proper vertical alignment.
		*****/
		words[i]->line = line;	/* fix 04/26/97-01, kdh */
		words[i]->x    = x_pos;
		words[i]->base = base_obj;
		if(words[i]->type != OBJ_TEXT)
		{
			words[i]->y = y_pos + words[i]->owner->y_offset -
							words[i]->font->xfont->ascent;
			have_object = True;
		}
		/* regular text, no additional adjustment required */
		else
			words[i]->y = y_pos + words[i]->owner->y_offset;

		x_pos += words[i]->width;

		/* we must insert a newline */
		if(words[i]->spacing != 0)
		{
			/*****
			* Adjust font of non-text objects to the largest font of the
			* text objects (required for proper anchor drawing)
			*****/
			if(base_obj->type != OBJ_TEXT)
				base_obj->font = basefont;

			/*****
			* Adjust baseline for all words on the current line if the
			* current baseline object is an image or a form component.
			* For all plain text objects we only need the lineheight.
			*****/
			if(base_obj->type == OBJ_IMG || base_obj->type == OBJ_FORM)
				AdjustBaselinePre(base_obj, words, word_start, i, &lineheight,
					False);
			else
				lineheight = (((int)words[i]->spacing) * y_offset);

			y_pos += lineheight;

			/* increment height of this paragraph */
			p_height += lineheight;

			/* Adjust for alignment */
			CheckAlignment(html, words, word_start, i, -1, box->width, False,
				-1);

			if(x_pos > max_width)
				max_width = x_pos;

			x_pos = box->left;
			line++;
			word_start  = i+1;		/* next word starts on a new line */
			base_obj    = words[i];
			basefont    = base_obj->font;
 			y_offset    = basefont->lineheight;
			lineheight  = y_offset;		/* default lineheight */
			have_object = False;
			first_line  = False;

			_XmHTMLFullDebug(5, ("paint.c: ComputeTextLayoutPre, linefeed, "
				"x = %d, y = %d.\n", x_pos, y_pos));

			if(box->height != -1 && p_height >= box->height)
				done = True;
		}
	}
	/* sanity, can be true for short <pre></pre> paragraphs. */
	if(word_start == *nwords)
		word_start--;

	if(i == *nwords)
	{
		if(last_line)
			done = True;
		else
			done = False;
	}
	else if(done)
	{
		*nwords = i;
		done = False;
	}

	/* also adjust baseline for the last line */
	if(base_obj->type == OBJ_IMG || base_obj->type == OBJ_FORM)
		AdjustBaselinePre(base_obj, words, word_start, i, &lineheight, done);

	/* also adjust alignment for the last line */
	CheckAlignment(html, words, word_start, *nwords, -1, box->width, done, -1);

	/* non-text objects (images & form members) move the baseline downward */
	if(have_object)
	{
		box->y = y_pos + lineheight;
		had_break = True;
	}
	else
		box->y = y_pos;
	box->x = x_pos;

	/* final check, required for very wide words/images */
	if(x_pos > max_width)
		max_width = x_pos;

	/* make sure we have a linefeed */
	if(first_line)
		line++;

	/* all done */
}

static void
SetTable(XmHTMLWidget html, int *x, int *y, XmHTMLObjectTableElement data)
{
	data->x = *x;
	data->y = *y;
	data->height = html->html.default_font->height;
	data->line   = line;
}

static void
SetApplet(XmHTMLWidget html, int *x, int *y, XmHTMLObjectTableElement data)
{
	data->x = *x;
	data->y = *y;
	data->height = html->html.default_font->height;
	data->line   = line;
}

static void
SetBlock(XmHTMLWidget html, int *x, int *y, XmHTMLObjectTableElement data)
{
	XmHTMLfont *font;

	/* use seperate font var, FONTHEIGHT contains multiple evals */
	font = (data->font ? data->font : html->html.default_font);
	data->x = *x;
	data->y = *y;
	data->height = font->lineheight;	/* fix 01/25/97-01; kdh */
	data->line   = line;
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

		_XmHTMLFullDebug(5, ("paint.c: painting image anchor, x = %i, y = %i\n",
			data->x, data->y));

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
		_XmHTMLDebug(5, ("paint.c: DrawImage, removing animation %s " \
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

	_XmHTMLDebug(5, ("paint.c: DrawFrame, animation %s, frame %i, "
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
			_XmHTMLDebug(5, ("paint.c: DrawFrame, %s, disposing frame %i "
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

				_XmHTMLDebug(5, ("paint.c: DrawFrame: background disposal "
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

				_XmHTMLDebug(5, ("paint.c: DrawFrame: background disposal "
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
			XCopyArea(dpy, image->pixmap, prev_state, tmpGC, fx, fy, width,
				height, 0, 0);

			/* and save it */
			image->frames[idx].prev_state = prev_state;

			/* free and destroy */
			XFreeGC(dpy, tmpGC);
			/* 		FEDERICO: MIRA ESTO. */
#endif
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
	_XmHTMLDebug(5, ("paint.c: DrawFrame end\n"));
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

	_XmHTMLDebug(5, ("paint.c: DrawImage start, x = %i, y = %i\n",
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
			_XmHTMLDebug(5, ("paint.c: DrawImage end, animation %s not in "
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
				_XmHTMLDebug(5, ("paint.c: DrawImage end, out of vertical "
					"range.\n"));
				return;
			}
			if(html->html.paint_x > data->words->x + image->width ||
				html->html.paint_width < data->words->x)
			{
				_XmHTMLDebug(5, ("paint.c: DrawImage end, out of horizontal "
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
	
	_XmHTMLDebug(5, ("paint.c: DrawImage end\n"));
}

static void
SetRule(XmHTMLWidget html, int *x, int *y, XmHTMLObjectTableElement data)
{
	int width = html->html.work_width - html->html.margin_width;
	int h;

	/* horizontal offset */
	*x = html->html.margin_width;

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
				*x = html->html.margin_width + html->html.work_width - width;
				break;
			case XmHALIGN_CENTER:
				*x = html->html.margin_width + 
					(html->html.work_width - width - html->html.margin_width)/2;
			default:	/* shutup compiler */
				break;
		}
	}
	/* Save position and width */
	data->x = *x;
	data->y = *y;
	data->line  = line;
	data->width = width;

	/* Adjust x and y */
	*x = html->html.margin_width;
	/*
	* This might seem funny, but it is a hack to correct the linefeeding
	* mechanism in format.c. Horizontal rules are a real pain in the you
	* know what to deal with.
	* If we do not increase y, text will appear above and on (or even in)
	* a rule.
	*/
	if(data->linefeed)
		h = 2*data->linefeed + data->height;
	else
		h = (int)(2*(html->html.default_font->lineheight)) + data->height;

	/* linefeed */
	line += 2;
	*y += h;
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
		int x = data->x, y = data->y;
		SetRule(html, &x, &y, data);
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
SetBullet(XmHTMLWidget html, int *x, int *y, XmHTMLObjectTableElement data)
{
	Dimension radius;
	char number[42];	/* large enough buffer for a zillion numbers */
	XmHTMLfont *font = html->html.default_font;
	String prefix;

	/* linefeed if not at left margin */
	if(*x != html->html.margin_width)
		line++;
	*y += data->linefeed; 
	*x = html->html.margin_width + data->ident;

	/***** 
	* x-offset for any marker and radius for a bullet or length of a 
	* side for a square marker.
	*****/
	radius = (Dimension)(0.5*(Toolkit_XFont (font->xfont)->max_bounds.lbearing + 
		Toolkit_XFont (font->xfont)->max_bounds.rbearing)); 

	data->x = *x;
	data->y = *y;
	data->height = html->html.default_font->height;
	data->line   = line;

	if(data->marker == XmMARKER_DISC || data->marker == XmMARKER_SQUARE ||
		data->marker == XmMARKER_CIRCLE)
	{
		/* y-offset for this marker */
		data->height = (Dimension)(0.5*font->lineheight + 0.25*radius);
		data->width = radius;
	}
	else
	{
		/*****
		* If we have a word, this is an ordered list for which the index
		* should be propageted.
		*****/
		if(data->words)
			prefix = data->words[0].word;
		else
			prefix = "";
		switch(data->marker)
		{
			case XmMARKER_ALPHA_LOWER: 
				sprintf(number, "%s%s.", prefix,
					ToAsciiLower(data->list_level));
				break;
			case XmMARKER_ALPHA_UPPER: 
				sprintf(number, "%s%s.", prefix,
					ToAsciiUpper(data->list_level));
				break;
			case XmMARKER_ROMAN_LOWER:
				sprintf(number, "%s%s.", prefix,
					ToRomanLower(data->list_level));
				break;
			case XmMARKER_ROMAN_UPPER:
				sprintf(number, "%s%s.", prefix,
					ToRomanUpper(data->list_level));
				break;
			case XmMARKER_ARABIC:
			default:
				sprintf(number, "%s%i.", prefix, data->list_level);
				break;
		}
		data->text  = strdup(number);
		data->len   = strlen(number);
		data->width = radius + Toolkit_Text_Width(font->xfont, data->text, data->len);
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

static void
DrawLineBreak(XmHTMLWidget html, int *x, int *y,
	XmHTMLObjectTableElement data)
{
	int linefeed = data->linefeed;

	/* save current position */
	data->y = *y;
	data->x = *x;

	/* add a linebreak if requested */
	if(linefeed)
	{
		/* if we already had a linefeed, we can substract one */
		if(had_break && baseline_obj)
		{
			linefeed -= baseline_obj->font->lineheight;
			had_break = False;
		}
		/* no negative linefeeds!! */
		if(linefeed > 0)
		{
			line++;
			*y += data->linefeed;
		}
	}
	*x = html->html.margin_width + data->ident; 
	data->line = line;
	data->height = *y - data->y;	/* height of this linefeed */
	return;
}
