#ifndef lint
static char rcsId[]="$Header$";
#endif
/*****
* format.c : XmHTML formatting routines: translates parsed HTML to 	info 
*			required for displaying a HTML page.
*
* This file Version	$Revision$
*
* Creation date:		Tue Nov 26 17:03:09 GMT+0100 1996
* Last modification: 	$Date$
* By:					$Author$
* Current State:		$State$
*
* Author:				newt
* (C)Copyright 1995-1996 Ripley Software Development
* All Rights Reserved
*
* This file is part of the XmHTML TWidget Library.
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
* Revision 1.2  1997/12/11 21:20:21  unammx
* Step 2: more gtk/xmhtml code, still non-working - mig
*
* Revision 1.1  1997/11/28 03:38:56  gnomecvs
* Work in progress port of XmHTML;  No, it does not compile, don't even try -mig
*
* Revision 1.17  1997/10/23 00:24:56  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.16  1997/08/31 17:34:24  newt
* renamed _rec structures to Rec.
*
* Revision 1.15  1997/08/30 00:55:24  newt
* Completed <form></form> support.
* Bugfix in _XmHTMLInitializeFontSizeLists, the default font is now properly
* changed.
* Made the font loading routines again robuster.
* ParseBodyTags now always attempts to load a body image.
* Made XmHTMLGetURLType a bit stricter.
*
* Revision 1.14  1997/08/01 13:00:21  newt
* Bugfixes in font switching (<b>...<font><i>...</i></font>...</b>) now
* properly handler. Enhanced form support.
*
* Revision 1.13  1997/05/28 01:46:35  newt
* Added support for the XmNbodyImage resource: it's now used but only if no
* bgcolor resource has been set.
*
* Revision 1.12  1997/04/29 14:26:18  newt
* HTML forms changes
*
* Revision 1.11  1997/04/03 05:34:25  newt
* _XmHTMLLoadBodyImage added.
* Placed a large number of warnings between a #ifdef PEDANTIC/#endif
*
* Revision 1.10  1997/03/28 07:12:43  newt
* Fixed buffer overrun in TexToPre. 
* Fixed font resolution: x and y resolution are now always equal. 
* XmHTML now ignores the ending body tag.
*
* Revision 1.9  1997/03/20 08:10:04  newt
* Split font cache in a true cache and a font stack.
* Added stack checks when document has been formatted.
*
* Revision 1.8  1997/03/11 19:52:17  newt
* added ImageToWord
*
* Revision 1.7  1997/03/04 00:59:26  newt
* ?
*
* Revision 1.6  1997/03/02 23:17:46  newt
* Way too many changes. Most important: font loading/switching scheme; anchor 
* treatment; image/imagemap treatment
*
* Revision 1.5  1997/02/11 02:08:44  newt
* Way to many. Anchor treatment has been changed completely. 
* Bugfixes in anchor parsing. Potential buffer overruns eliminated.
*
* Revision 1.4  1997/02/04 02:56:49  newt
* Bugfix in LoadQueryFont. 
* Added code to deal with the basefont element. 
* Changed the font element handling.
*
* Revision 1.3  1997/01/09 06:55:39  newt
* expanded copyright marker
*
* Revision 1.2  1997/01/09 06:44:42  newt
* lots of changes: linebreaking and changes related to changed XmHTMLWord
*
* Revision 1.1  1996/12/19 02:17:10  newt
* Initial Revision
*
*****/ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>	/* isspace, tolower */

/* Local includes */
#include "XmHTMLP.h"
#include "XmHTMLfuncs.h"

/*** External Function Prototype Declarations ***/
#ifdef DEBUG
extern void dumpFontCacheStats(void);
#endif

extern void _XmHTMLaddFontMapping(XmHTMLWidget html, String name,
	String family, int ptsz, Byte style, XmHTMLfont *font);

/*** Public Variable Declarations ***/

/*** Private Datatype Declarations ****/
#define MAX_NESTED_LISTS	26	/* maximum no of nested lists */
#define IDENT_SPACES		3	/* length of each indent, in no of spaces */

/* required for anchor testing */
typedef int (*anchorProc)(TWidget,String);

typedef struct{
	String name;
	Marker type;	/* Marker is an enumeration type defined in XmHTMLP.h */
}listMarkers;

/* for nesting of ordered and unordered lists */
typedef struct{
	Boolean isindex;	/* propagete index numbers? */
	int level;			/* item number */
	htmlEnum type;		/* ol or ul, used for custom markers */
	Marker marker;		/* marker to use */
}listStack;

/* for nesting of colors */
typedef struct colorRec{
	unsigned long color;
	struct colorRec *next;
}colorStack;

typedef struct fontRec{
	int size;
	XmHTMLfont *font;	/* ptr to cached font */
	struct fontRec *next;
}fontStack;

/* for nesting of alignments */
typedef struct alignRec{
	Alignment align;
	struct alignRec *next;
}alignStack;

/*** Private Function Prototype Declarations ****/

/****
* Formatting routines 
*****/
/* Created a new element for the formatted element table */
static XmHTMLObjectTableElement NewTableElement(XmHTMLObject *data);

/* Insert and element into the formatted element table */
static void InsertTableElement(XmHTMLWidget html, 
	XmHTMLObjectTableElement element, Boolean is_anchor);

/* Release the formatted element table */
static void FreeObjectTable(XmHTMLObjectTable *list);

/* Initialize the formatted element table */
static void InitObjectTable(XmHTMLObjectTable *list, XmHTMLAnchor *anchors);

/* load a font (or get from font cache) and place it on the font stack. */
static XmHTMLfont *NextFont(XmHTMLWidget html, htmlEnum font_id, int size);

/* load a font (or get from font cache) with a given size and face */
static XmHTMLfont *NextFontWithFace(XmHTMLWidget html, int size, String face);

/* push/pop a font on font stack */
static void PushFont(XmHTMLfont *font, int size);
static XmHTMLfont *PopFont(int *size);

/* copy given text into an internal buffer */
static String CopyText(XmHTMLWidget html, String text, Boolean formatted,
	Byte *text_data, Boolean expand_escapes);

/* collapse all consecutive whitespace into a single space */
static void CollapseWhiteSpace(String text);

/* Split raw text into an array of words */
static XmHTMLWord* TextToWords(String text, int *num_words, Dimension *height, 
	XmHTMLfont *font, Byte line_data, Byte text_data, 
	XmHTMLObjectTableElement owner);

/* Split an image into an array of words ;-) */
static XmHTMLWord *ImageToWord(XmHTMLWidget html, String attributes, 
	int *num_words, Dimension *height, XmHTMLObjectTableElement owner,
	Boolean formatted);

static XmHTMLWord *allocFormWord(XmHTMLForm *form, Dimension *width,
	Dimension *height, XmHTMLObjectTableElement owner, Boolean formatted);

static XmHTMLWord *InputToWord(XmHTMLWidget html, String attributes, 
	int *num_words, Dimension *width, Dimension *height,
	XmHTMLObjectTableElement owner, Boolean formatted);

static XmHTMLWord *SelectToWord(XmHTMLWidget html, XmHTMLObject *start,
	int *num_words, Dimension *width, Dimension *height,
	XmHTMLObjectTableElement owner, Boolean formatted);

static XmHTMLWord *TextAreaToWord(XmHTMLWidget html, XmHTMLObject *start,
	int *num_words, Dimension *width, Dimension *height,
	XmHTMLObjectTableElement owner, Boolean formatted);

/* Split raw text into a chunk of preformatted lines */
static XmHTMLWord *TextToPre(String text, int *num_words, XmHTMLfont *font, 
	Byte line_data, XmHTMLObjectTableElement owner);
 
/* Insert a horizontal tab */
static XmHTMLWord* SetTab(int size, Dimension *height, XmHTMLfont *font, 
	XmHTMLObjectTableElement owner);  

/* Parse body tags and update the TWidget */
static void ParseBodyTags(XmHTMLWidget html, XmHTMLObject *data);

/* Check whether a linefeed is required or not */
static int CheckLineFeed(int this, Boolean force);

/* push/pop a color on the color stack */
static void PushColor(Pixel color);
static Pixel PopColor(void);

/* push/pop an alignment on/from the alignment stack */
static void PushAlignment(Alignment align);
static Alignment PopAlignment(void);

/* split the given anchor spec into a href, target and other stuff */
static void parseHref(String text, XmHTMLAnchor *anchor); 

/*** Private Variable Declarations ***/
/* Element data bits */
#define ELE_ANCHOR				(1<<1)
#define ELE_ANCHOR_TARGET		(1<<2)
#define ELE_ANCHOR_VISITED		(1<<3)
#define ELE_ANCHOR_INTERN		(1<<4)
#define ELE_UNDERLINE			(1<<5)
#define ELE_UNDERLINE_TEXT		(1<<6)
#define ELE_STRIKEOUT			(1<<7)
#define ELE_STRIKEOUT_TEXT		(1<<8)

/* Private formatted element table data */
static struct{
	unsigned long num_elements;
	unsigned long num_anchors;
	XmHTMLObjectTableElement head;
	XmHTMLObjectTableElement current;
	XmHTMLAnchor *anchor_head;
	XmHTMLAnchor *anchor_current;
}list_data;

/* color and alignment stacks */
static colorStack color_base, *color_stack;
static alignStack align_base, *align_stack;
static fontStack font_base, *font_stack;

/* Marker information for HTML lists, ordered list. */
#define OL_ARRAYSIZE	5
static listMarkers ol_markers[OL_ARRAYSIZE] = {
	{"1", XmMARKER_ARABIC},
	{"a", XmMARKER_ALPHA_LOWER},
	{"A", XmMARKER_ALPHA_UPPER},
	{"i", XmMARKER_ROMAN_LOWER},
	{"I", XmMARKER_ROMAN_UPPER},
};

/* Unordered list. */
#define UL_ARRAYSIZE	3
static listMarkers ul_markers[UL_ARRAYSIZE] = {
	{"disc", XmMARKER_DISC},
	{"square", XmMARKER_SQUARE},
	{"circle", XmMARKER_CIRCLE},
};

#ifdef DEBUG
static int allocated;
#endif

/*****
* Name: 		NewTableElement
* Return Type: 	XmHTMLObjectTableElement
* Description: 	creates a ObjectTableElement and fills it.
* In: 
*	data:		raw data for this element.
* Returns:
*	the newly created element.
*****/
static XmHTMLObjectTableElement
NewTableElement(XmHTMLObject *data)
{
	static XmHTMLObjectTableElement element = NULL;

	element = (XmHTMLObjectTableElement)malloc(sizeof(XmHTMLObjectTable));

	/* initialise to zero */
	(void)memset(element, 0, sizeof(XmHTMLObjectTable));
	/* fill in appropriate fields */
	element->object = data;

#ifdef DEBUG
	allocated++;
#endif
	return(element);
}

/*****
* Name: 		InsertTableElement
* Return Type: 	void
* Description: 	inserts a given formatted element in the list of elements.
* In: 
*	w:			XmHTMLWidget to which this element belongs.
*	element:	element to add
*	is_anchor:	true if this element is an anchor.
* Returns:
*	nothing.
*****/
static void
InsertTableElement(XmHTMLWidget html, XmHTMLObjectTableElement element, 
	Boolean is_anchor) 
{
	/* attach prev and next ptrs to the appropriate places */
	element->prev = list_data.current;
	list_data.current->next = element;
	list_data.current = element;
	/* increment element counter */
	list_data.num_elements++;
#ifdef DEBUG
	if(is_anchor)
		list_data.num_anchors++;
#endif
}

/*****
* Name: 		parseHref
* Return Type: 	void
* Description: 	returns the url specification found in the given anchor.
* In: 
*	text:		full anchor spec.
*	href:		url found in given anchor. Filled upon return.
*	target:		any target attribute found. Filled upon return.
*	extra:		any additional attributes found. Filled upon return.
* Returns:
*	nothing.
*****/
static void
parseHref(String text, XmHTMLAnchor *anchor) 
{
	if(text == NULL ||
		(anchor->href = _XmHTMLTagGetValue(text, "href")) == NULL)
	{
		/* allocate empty href field so later strcmps won't explode */
		anchor->href = (char *)malloc(1);
		anchor->href[0] = '\0'; /* fix 02/03/97-05, kdh */
		/*
		* Could be a named anchor with a target spec. Rather impossible but
		* allow for it anyway (I can imagine this to be true for a
		* split-screen display).
		*/
		if(text == NULL)
			return;
	}

	/* Check if there is a target specification */
	anchor->target= _XmHTMLTagGetValue(text, "target");

	/* Also check for rel, rev and title */
	anchor->rel = _XmHTMLTagGetValue(text, "rel");
	anchor->rev = _XmHTMLTagGetValue(text, "rev");
	anchor->title  = _XmHTMLTagGetValue(text, "title");
}

/*****
* Name: 		FreeObjectTable
* Return Type: 	void
* Description: 	releases all memory occupied by the formatted list of elements.
* In: 
*	list:		previous list to be released.
* Returns:
*	nothing.
* Note:
*	Images are freed in XmHTML.c, which calls XmHTMLFreeAllImages to do the
*	job.
*****/
static void 
FreeObjectTable(XmHTMLObjectTable *list)
{
	XmHTMLObjectTableElement temp;

#ifdef DEBUG
	int i = 0, j = 0;
#endif

	/* free all parsed objects */
	while(list != NULL)
	{
		temp = list->next;
		if(list->text)	/* space occupied by text to display */
			free(list->text);

		/* free list of words. Can't be done above, <pre> doesn't have this! */
		if(list->n_words)
		{
			/* 
			* only the first word contains a valid ptr, all others point to
			* some char in this buffer, so freeing them *will* cause a
			* segmentation fault eventually.
			*/
			free(list->words[0].word);
			/* Free raw word data */
			free(list->words);
		}
		free(list);
		list = temp;
#ifdef DEBUG
		i++;
#endif
	}
	_XmHTMLDebug(2, ("format.c: FreeObjectTable End, freed %i elements and "
		"%i anchors.\n", i, j));
}

/*****
* Name: 		FreeAnchors
* Return Type: 	void
* Description: 	frees the memory occupied by the anchor data
* In: 
*	anchors:	list of anchors to be freed
* Returns:
*	nothing.
*****/
static void
FreeAnchors(XmHTMLAnchor *anchors)
{
	XmHTMLAnchor *tmp;
	int i = 0;

	while(anchors)
	{
		tmp = anchors->next;
		/* href field is always allocated */
		free(anchors->href);
		if(anchors->target)
			free(anchors->target);
		if(anchors->rel)
			free(anchors->rel);
		if(anchors->rev)
			free(anchors->rev);
		if(anchors->title)
			free(anchors->title);
		if(anchors->name)		/* fix 07/09/97-01, kdh */
			free(anchors->name);
		free(anchors);
		anchors = NULL;
		anchors = tmp;
		i++;
	}
	_XmHTMLDebug(2, ("format.c: FreeAnchors, freed %i XmHTMLAnchor objects\n", 
		i));
}

/*****
* Name: 		InitObjectTable
* Return Type: 	void
* Description: 	initializes the list of formatted elements.
* In: 
*	list:		previous list to be released.
* Returns:
*	nothing
* Note:
*	The list head is a dummy element and is never used. It is done to gain
*	some performance (a test on an empty head is not required now in the
*	InsertTableElement routine).
*****/
static void
InitObjectTable(XmHTMLObjectTable *list, XmHTMLAnchor *anchors)
{
	if(list != NULL)
	{
		FreeObjectTable(list);
		list = NULL;
	}

	if(anchors != NULL)
	{
		FreeAnchors(anchors);
		anchors = NULL;
	}
	if(list_data.head)
		free(list_data.head);
	list_data.head = NewTableElement(NULL);
	list_data.current = list_data.head;
	list_data.anchor_head = (XmHTMLAnchor*)NULL;
	list_data.anchor_current = (XmHTMLAnchor*)NULL;
	list_data.num_elements = 1;
	list_data.num_anchors  = 0;
}

/*****
* Name: 		NextFont
* Return Type: 	XmHTMLfont*
* Description: 	loads a new font, with the style determined by the current 
*				font: if current font is bold, and new is italic then a 
*				bold-italic font will be returned.
* In: 
*	w:			TWidget for which to load a font
*	font_id:	id describing type of font to load.
*	size:		size of font to load. Only used for HT_FONT.
* Returns:
*	the loaded font.
*****/
static XmHTMLfont*
NextFont(XmHTMLWidget html, htmlEnum font_id, int size)
{
	XmHTMLfont *new_font = NULL, *curr_font;
	String family;
	int ptsz;
	Byte new_style = (Byte)0, font_style;
	Boolean ok = True;	/* enable font warnings */

	/* pick up style of the current font */
	curr_font = font_stack->font;

	/* curr_font *must* always have a value as it references a cached font */
	my_assert(curr_font != NULL);

	font_style = curr_font->style;

	_XmHTMLDebug(2,("NextFont: current font is %s %s %s.\n",
		(font_style & FONT_FIXED  ? "fixed"  : "scalable"),
		(font_style & FONT_BOLD   ? "bold"   : "medium"),
		(font_style & FONT_ITALIC ? "italic" : "regular"))); 

	/* See if we need to proceed with bold font */
	if(font_style & FONT_BOLD)
		new_style = FONT_BOLD;
	else
		new_style &= ~FONT_BOLD;

	/* See if we need to proceed with italic font */
	if(font_style & FONT_ITALIC)
		new_style |= FONT_ITALIC;
	else
		new_style &= ~FONT_ITALIC;

	/* See if we need to proceed with a fixed font */
	if(font_style & FONT_FIXED)
	{
		new_style |= FONT_FIXED;
		family = html->html.font_family_fixed;
		ptsz = xmhtml_fn_fixed_sizes[0];
	}
	else
	{
		new_style &= ~FONT_FIXED;
		family = curr_font->font_family;
		ptsz = xmhtml_fn_sizes[0];
	}

	switch(font_id)
	{
		case HT_CITE:
		case HT_I:
		case HT_EM:
		case HT_DFN:
		case HT_ADDRESS:
			new_font = _XmHTMLloadQueryFont((TWidget)html, family, NULL, 
				xmhtml_basefont_sizes[size-1], new_style|FONT_ITALIC, &ok);  
			break;
		case HT_STRONG:
		case HT_B:
		case HT_CAPTION:
			new_font = _XmHTMLloadQueryFont((TWidget)html, family, NULL, 
				xmhtml_basefont_sizes[size-1], new_style | FONT_BOLD, &ok);
			break;

		/*****
		* Fixed fonts always use the font specified by the value of the
		* fontFamilyFixed resource.
		*****/
		case HT_SAMP:
		case HT_TT:
		case HT_VAR:
		case HT_CODE:
		case HT_KBD:
 		case HT_PRE:	/* fix 01/20/97-03, kdh */
			new_font = _XmHTMLloadQueryFont((TWidget)html,
				html->html.font_family_fixed, NULL, xmhtml_fn_fixed_sizes[0],
				new_style |FONT_FIXED, &ok);
			break;

		/* The <FONT> element is useable in *every* state */
		case HT_FONT:
			new_font = _XmHTMLloadQueryFont((TWidget)html, family, NULL, size,
				new_style, &ok);
			break;

		/*****
		* Since HTML Headings may not occur inside a <font></font> declaration,
		* they *must* use the specified document font, and not derive their
		* true font from the current font.
		*****/
		case HT_H1:
			new_font = _XmHTMLloadQueryFont((TWidget)html,
				html->html.font_family, NULL, xmhtml_fn_sizes[2],
				FONT_SCALABLE|FONT_BOLD, &ok);
			break;
		case HT_H2:
			new_font = _XmHTMLloadQueryFont((TWidget)html,
				html->html.font_family, NULL, xmhtml_fn_sizes[3],
				FONT_SCALABLE|FONT_BOLD, &ok);
			break;
		case HT_H3:
			new_font = _XmHTMLloadQueryFont((TWidget)html,
				html->html.font_family, NULL, xmhtml_fn_sizes[4],
				FONT_SCALABLE|FONT_BOLD, &ok);
			break;
		case HT_H4:
			new_font = _XmHTMLloadQueryFont((TWidget)html,
				html->html.font_family, NULL, xmhtml_fn_sizes[5],
				FONT_SCALABLE|FONT_BOLD, &ok);
			break;
		case HT_H5:
			new_font = _XmHTMLloadQueryFont((TWidget)html,
				html->html.font_family, NULL, xmhtml_fn_sizes[6],
				FONT_SCALABLE|FONT_BOLD, &ok);
			break;
		case HT_H6:
			new_font = _XmHTMLloadQueryFont((TWidget)html,
				html->html.font_family, NULL, xmhtml_fn_sizes[7],
				FONT_SCALABLE|FONT_BOLD, &ok);
			break;

		/* should never be reached */
		default:
#ifdef PEDANTIC
			_XmHTMLWarning(__WFUNC__(html, "NextFont"), 
				"Unknown font switch. Using default font.");
#endif /* PEDANTIC */
			/* this will always succeed */
			ok = False;
			new_font = _XmHTMLloadQueryFont((TWidget)html, family, NULL, ptsz, 
				FONT_SCALABLE|FONT_REGULAR|FONT_MEDIUM, &ok);
			break;
	}
	return(new_font);
}

/*****
* Name: 		NextFontWithFace
* Return Type: 	XmHTMLfont*
* Description: 	load a new font with given pixelsize and face. 
*				Style is determined by the current font: if current font
*				is bold, and new is italic then a bold-italic font will be 
*				returned.
* In: 
*	w:			TWidget for which to load a font
*	size:		size of font to load. Only used for HT_FONT.
*	face:		a comma separated list of font faces to use, contents are 
*				destroyed when this function returns.
* Returns:
*	A new font with a face found in the list of faces given upon success
*	or the default font on failure.
*****/
static XmHTMLfont*
NextFontWithFace(XmHTMLWidget html, int size, String face)
{
	XmHTMLfont *new_font = NULL, *curr_font;
	String chPtr, family, all_faces, first_face = NULL;
	Byte new_style = (Byte)0, font_style;
	int try;

	/* pick up style of the current font */
	curr_font = font_stack->font;

	my_assert(curr_font != NULL);

	font_style = curr_font->style;

	/* See if we need to proceed with bold font */
	if(font_style & FONT_BOLD)
		new_style = FONT_BOLD;
	else
		new_style &= ~FONT_BOLD;

	/* See if we need to proceed with italic font */
	if(font_style & FONT_ITALIC)
		new_style |= FONT_ITALIC;
	else
		new_style &= ~FONT_ITALIC;

	/***** 
	* See if we need to proceed with a fixed font, only used to determine
	* initial font family.
	*****/
	if(font_style & FONT_FIXED)
	{
		new_style |= FONT_FIXED;
		family = html->html.font_family_fixed;
	}
	else
	{
		new_style &= ~FONT_FIXED;
		family = html->html.font_family;
	}

	/* we must have a ``,'' or strtok will fail */
	if((strstr(face, ",")) == NULL)
	{
		all_faces = (String)malloc(strlen(face) + 2);
		strcpy(all_faces, face);
		strcat(all_faces, ",\0");
	}
	else
		all_faces = strdup(face);

	/* walk all possible spaces */
	try = 0;
	for(chPtr = strtok(all_faces, ","); chPtr != NULL;
		chPtr = strtok(NULL, ","))
	{
		Boolean ok = False;

		try++;

		/* skip any leading spaces */
		while(isspace(*chPtr))
			chPtr++;

		_XmHTMLDebug(2, ("format.c: NextFontWithFace, trying with face %s\n",
			chPtr));

		/***** 
		* Disable font not found warning message, we are trying to find
		* a font of which we don't know if it exists.
		*****/
		ok = False;
		new_font = _XmHTMLloadQueryFont((TWidget)html, family, chPtr, size,
			new_style, &ok);
		if(new_font && ok)
		{
			_XmHTMLDebug(2, ("format.c: NextFontWithFace, font loaded.\n"));
			break;
		}
		if(try == 1)
			first_face = strdup(chPtr);
	}
	free(all_faces);
	/*****
	* hmm, the first font in this face specification didn't yield a valid
	* font. To speed up things considerably, we add a font mapping for the
	* first face in the list of given spaces. There's no sense in doing this
	* when there is only one face specified as this will always get us the
	* default font. We only add a mapping if the name of the returned font
	* contains at least one of the allowed faces. Not doing this check would
	* ignore face specs which do have a face we know. We also want the font
	* styles to match as well.
	* BTW: this is a tremendous speedup!!!
	*****/
	if(first_face)
	{
		/*****
		* Only add a mapping if the returned name contains one of the allowed
		* faces. No need to check for the presence of a comma: we only take
		* lists that have multiple face specifications.
		*****/
		if(try > 1)
		{
			/*****
			* Walk all possible faces. Nukes the face array but that's not
			* bad as we are the only ones using it.
			*****/
			for(chPtr = strtok(face, ","); chPtr != NULL;
				chPtr = strtok(NULL, ","))
			{
				/* skip any leading spaces */
				while(isspace(*chPtr))
					chPtr++;
				/* caseless 'cause fontnames ignore case */
				if(my_strcasestr(new_font->font_name, chPtr) &&
					new_font->style == new_style)
				{
					_XmHTMLaddFontMapping(html, family, first_face, size,
						new_style, new_font);
					break;
				}
			}
		}
		free(first_face);
	}
	return(new_font);
}

/*****
* Name: 		CollapseWhiteSpace
* Return Type: 	void
* Description: 	collapses whitespace in the given text
* In: 
*	text:		text for which multiple whitespace has to be collapsed.
* Returns:
*	nothing, but text is updated when this function returns.
*****/
static void
CollapseWhiteSpace(String text)
{
	register char *outPtr = text;

	/* 
	* We only collapse valid text and text that contains more than whitespace
	* only. This should never be true since CopyText will filter these
	* things out. It's just here for sanity.
	*/
	if(*text == '\0' || !strlen(text))
		return;

	_XmHTMLDebug(2, ("format.c: CollapseWhiteSpace, text in is:\n%s\n", text));

	/*
	* Now collapse each occurance of multiple whitespaces.
	* This may produce different results on different systems since
	* isspace() might not produce the same on each and every platform.
	*/
	while(True)
	{
		switch(*text)
		{
			case '\f':
			case '\n':
			case '\r':
			case '\t':
			case '\v':
				*text = ' ';	/* replace by a single space */
				/* fall through */
			case ' ':
				/* skip past first space */
				*(outPtr++) = *(text++);	
				/* collapse every space following */
				while(*text != '\0' && isspace(*text))
					*text++ = '\0';
				break;
			default:
				*(outPtr++) = *(text++);
				break;
		}
		if(*text == 0)
		{
			*outPtr = '\0';
			return;
		}
	}
}

/*****
* Name: 		TextToWords
* Return Type: 	XmHTMLWord*
* Description: 	splits the given text into an array of words.
* In: 
*	text:		text to split
*	num_words:	number of words in the given text. Filled upon return;
*	font:		font to use for this text.
* Returns:
*	an array of words. When allocation fails, this routine exits.
*****/
static XmHTMLWord* 
TextToWords(String text, int *num_words, Dimension *height, XmHTMLfont *font, 
	Byte line_data, Byte text_data, XmHTMLObjectTableElement owner)
{
	int n_words, len, i;
	char *start;
	static XmHTMLWord *words;
	static char *raw;
	register int j;
	register char *chPtr;

	/* sanity check */
	if(text == NULL)
	{
		*height = *num_words = 0;
		return(NULL);
	}

	_XmHTMLFullDebug(2, ("format.c: TextToWords, text in is:\n%s\n", text));

	/* compute how many words we have */
	n_words = 0;
	for(chPtr = text; *chPtr != '\0'; chPtr++)
		if(*chPtr == ' ')
			n_words++;
	/* also pick up the last word */
	n_words++;

	/* copy text */
	raw = strdup(text);

	/* allocate memory for all words */
	words = (XmHTMLWord*)malloc((n_words)*sizeof(XmHTMLWord));

	/* Split the text in words and fill in the appropriate fields */
	*height = font->height;
	chPtr = start = raw;

	for(i = 0, j = 0, len = 0; ; chPtr++, len++, j++)
	{
		/* also pick up the last word! */
		if(*chPtr == ' ' || *chPtr == '\0')
		{
			if(*chPtr)
			{
				chPtr++;			/* nuke the space */
				raw[j++] = '\0';
			}
			words[i].base      = NULL;
			words[i].self      = &words[i];
			words[i].x         = words[i].y = 0;
			words[i].word      = start;
			words[i].len       = len;
			words[i].height    = *height;
			words[i].width     = Toolkit_Text_Width(font->xfont, words[i].word, len);
			words[i].owner     = owner;
			words[i].font      = font;
			words[i].spacing   = TEXT_SPACE_LEAD | TEXT_SPACE_TRAIL;
			words[i].type      = OBJ_TEXT;
			words[i].line_data = line_data;
			words[i].line      = 0;

			_XmHTMLFullDebug(2, ("format.c: TextToWords, word is %s, len is "
				"%i, width is %i, height is %i\n", words[i].word, words[i].len,
				words[i].width, words[i].height));

			start = chPtr;
			i++;
			len = 0;
		}
		if(*chPtr == '\0')
			break;
	}
	/* 
	* when there is more than one word in this block, the first word
	* _always_ has a trailing space.
	* Likewise, the last word always has a leading space.
	*/
	if(n_words > 1)
	{
		/* unset nospace bit */
		Byte spacing = text_data & ~TEXT_SPACE_NONE;
		words[0].spacing = spacing | TEXT_SPACE_TRAIL;
		words[n_words-1].spacing = spacing | TEXT_SPACE_LEAD;
	}
	else
		words[0].spacing = text_data;

	_XmHTMLFullDebug(2, ("format.c: TextToWords counted %i words\n", n_words));

	*num_words = i; /* n_words */;
	return(words);	
}

/*****
* Name: 		ImageToWord
* Return Type: 	XmHTMLWord*
* Description: 	converts an image to a word
* In: 
*	w:			XmHTMLWidget id
*	attributes:	raw <img> specification
*	height:		object height, updated upon return
*	owner:		owning object
*	formatted:	True when this image is part of a block of <pre></pre> text.
* Returns:
*	a word representing the image
*****/
static XmHTMLWord*
ImageToWord(XmHTMLWidget html, String attributes, int *num_words, 
	Dimension *height, XmHTMLObjectTableElement owner, Boolean formatted)
{
	static XmHTMLWord *word;
	static XmHTMLImage *image;
	Dimension width = 0;

	*num_words = 0;

	/* sanity check */
	if(attributes == NULL || 
		(image = _XmHTMLNewImage(html, attributes, &width, height)) == NULL)
	{
		*height = 0;
		return(NULL);
	}

	_XmHTMLFullDebug(2, ("format.c: ImageToWord, image in is: %s\n",
		image->url));

	word = (XmHTMLWord*)malloc(sizeof(XmHTMLWord));

	/* required for image anchoring/replace/update */
	image->owner = owner;

	word->base   = NULL;
	word->self   = word;
	word->x      = word->y = 0;
	word->word   = strdup(image->alt);	/* we always have this */
	word->len    = strlen(image->alt);
	word->width  = width + 2*image->hspace + 2*image->border;
	word->height = *height + 2*image->vspace + 2*image->border;
	word->owner  = owner;
	word->font   = font_base.font;		/* always use the default font */
	/*****
	* if image support is disabled, add width of the alt text to the
	* image width (either from default image or specified in the doc).
	* This is required for proper exposure handling when images are disabled.
	*****/
	if(!html->html.images_enabled)
		word->width += Toolkit_Text_Width(word->font->xfont, word->word, word->len);

	/*****
	* No spacing if part of a chunk of <pre></pre> text
	* Fix 07/24/97, kdh
	*****/
	word->spacing = formatted ? 0 : TEXT_SPACE_LEAD | TEXT_SPACE_TRAIL;
	word->type = OBJ_IMG;
	word->line_data = NO_LINE;	/* no underlining for images */
	word->line  = 0;
	word->image = image;

	_XmHTMLFullDebug(2, ("format.c: TextToWords, word is %s, len is %i, "
		"width is %i, height is %i\n", word->word, word->len,
		word->width, word->height));

	*num_words = 1;
	return(word);
}

/*****
* Name:			allocFormWord
* Return Type: 	XmHTMLWord*
* Description: 	allocates a default XmHTMLWord for use within a HTML form.
* In: 
*	form:		form entry for which this word should be allocated;
*	*width:		object's width, updated upon return;
*	*height:	object's height, updated upon return;
*	owner:		owning object.
*	formatted:	true when allocating a form component present in <pre></pre>
* Returns:
*	a newly allocated word.
*****/
static XmHTMLWord*
allocFormWord(XmHTMLForm *form, Dimension *width, Dimension *height,
	XmHTMLObjectTableElement owner, Boolean formatted)
{
	static XmHTMLWord *word;

	/* allocate new entry */
	word = (XmHTMLWord*)malloc(sizeof(XmHTMLWord));
	memset(word, 0, sizeof(XmHTMLWord));

	/* fill in appropriate fields */
	word->self    = word;
	word->word    = strdup(form->name); 	/* we always have this */
	word->len     = strlen(form->name);
	word->height  = *height = form->height;
	word->width   = *width  = form->width;
	word->owner   = owner;
	word->font    = font_base.font; 		/* always use default font */
	word->spacing = formatted ? 0 : TEXT_SPACE_LEAD | TEXT_SPACE_TRAIL;
	word->type    = OBJ_FORM;
	word->form    = form;

	return(word);
}

/*****
* Name: 		InputToWord
* Return Type: 	XmHTMLWord*
* Description: 	converts a HTML form <input> element to a word
* In: 
*	w:			XmHTMLWidget id
*	attributes:	raw form element specification
*	width:		object width, updated upon return
*	height:		object height, updated upon return
*	owner:		owning object
*	formatted:	true when this form component is placed in a <pre></pre> tag.
* Returns:
*	a word representing the image
*****/
static XmHTMLWord*
InputToWord(XmHTMLWidget html, String attributes, int *num_words, 
	Dimension *width, Dimension *height, XmHTMLObjectTableElement owner,
	Boolean formatted)
{
	static XmHTMLForm *form_entry;
	XmHTMLWord *word;

	*num_words = 0;

	/* sanity check */
	if(attributes == NULL ||
		(form_entry = _XmHTMLFormAddInput(html, attributes)) == NULL)
		return(NULL);

	/* save owner, we need it in the paint routines */
	form_entry->data = owner;

	/* image buttons are treated as anchored images */
	if(form_entry->type == FORM_IMAGE)
	{
		word = ImageToWord(html, attributes, num_words, height, owner,
				formatted);
		/* remove alt text */
		free(word->word);
		/* use form member name instead */
		word->word = strdup(form_entry->name);
		word->len  = strlen(form_entry->name);
		word->form = form_entry;

		_XmHTMLFullDebug(2, ("format.c: InputToWord, word is %s, len is %i, "
			"width is %i, height is %i (type = image)\n", word->word,
			word->len, word->width, word->height));

		return(word);
	}

	/* allocate new word for this form member */
	word = allocFormWord(form_entry, width, height, owner, formatted);

	_XmHTMLFullDebug(2, ("format.c: InputToWord, word is %s, len is %i, "
		"width is %i, height is %i\n", word->word, word->len,
		word->width, word->height));

	*num_words = 1;
	return(word);
}

/*****
* Name:			SelectToWord
* Return Type: 	XmHTMLWord*
* Description:	converts a HTML form <select></select> to a HTMLWord.
*				Also processes any <option></option> items within this select.
* In: 
*	html:		XmHTMLWidget id;
*	start:		object at which <select> starts;
*	*num_words:	no of words allocated. Updated upon return;
*	*width:		width of returned object. Updated upon return;
*	*height:	height of returned object. Updated upon return;
*	owner:		owning element.
*	formatted:	true when this form component is placed in a <pre></pre> tag.
* Returns:
*	a newly allocated word upon success. NULL on failure.
*****/
static XmHTMLWord*
SelectToWord(XmHTMLWidget html, XmHTMLObject *start, int *num_words, 
	Dimension *width, Dimension *height, XmHTMLObjectTableElement owner,
	Boolean formatted)
{
	static XmHTMLForm *form_entry;
	XmHTMLWord *word;
	XmHTMLObject *tmp = start;

	*num_words = 0;

	/* sanity check */
	if(start->attributes == NULL ||
		(form_entry = _XmHTMLFormAddSelect(html, start->attributes)) == NULL)
		return(NULL);

	/* save owner */
	form_entry->data = owner;

	/* move to next element */
	tmp = tmp->next;

	/* add all option tags */
	for(; tmp != NULL && tmp->id != HT_SELECT; tmp = tmp->next)
	{
		if(tmp->id == HT_OPTION && !tmp->is_end)
		{
			XmHTMLObject *sel_start = tmp;
			Byte foo;
			String text = NULL;

			/*
			* The next object should be plain text, if not it's an
			* error and we should ignore it
			*/
			tmp = tmp->next;
			if(tmp->id != HT_ZTEXT)
			{
				if(html->html.bad_html_warnings)
				{
					/* empty option tag, ignore it */
					if(tmp->id == HT_OPTION)
						_XmHTMLWarning(__WFUNC__(html, "SelectToWord"),
							"Empty <OPTION> tag, ignored (line %i in input).",
							tmp->line);
					else
						_XmHTMLWarning(__WFUNC__(html, "SelectToWord"),
							"<%s> not allowed inside <OPTION> tag, ignored "
							"(line %i in input).", html_tokens[tmp->id],
							tmp->line);
				}
				continue;
			}
			/* get text */
			if((text = CopyText(html, tmp->element, False, &foo, True)) == NULL)
				continue;

			CollapseWhiteSpace(text);
			if(strlen(text))
			{
				_XmHTMLFormSelectAddOption(html, form_entry,
					sel_start->attributes, text);
				/* no longer needed */
				free(text);
			}
		}
	}
	/* close this selection and get width and height */
	_XmHTMLFormSelectClose(html, form_entry);

	/* allocate new word for this form member */
	word = allocFormWord(form_entry, width, height, owner, formatted);

	_XmHTMLFullDebug(2, ("format.c: SelectToWord, word is %s, len is %i, "
		"width is %i, height is %i\n", word->word, word->len,
		word->width, word->height));

	*num_words = 1;
	return(word);
}

/*****
* Name:			TextAreaToWord
* Return Type: 	XmHTMLWord*
* Description:	converts a HTML form <textarea> to a HTMLWord.
* In: 
*	html:		XmHTMLWidget id;
*	start:		object at which <textarea> starts;
*	*num_words:	no of words allocated. Updated upon return;
*	*width:		width of returned object. Updated upon return;
*	*height:	height of returned object. Updated upon return;
*	owner:		owning element.
*	formatted:	true when this form component is placed in a <pre></pre> tag.
* Returns:
*	a newly allocated word upon success. NULL on failure.
*****/
static XmHTMLWord*
TextAreaToWord(XmHTMLWidget html, XmHTMLObject *start, int *num_words,
	Dimension *width, Dimension *height, XmHTMLObjectTableElement owner,
	Boolean formatted)
{
	static XmHTMLForm *form_entry;
	XmHTMLWord *word;
	String text = NULL;
	Byte foo;

	*num_words = 0;
	*height = *width = 0;

	/* sanity check */
	if(start->attributes == NULL)
		return(NULL);

	/* get text between opening and closing <textarea>, if any */
	if(start->next->id == HT_ZTEXT)
		text = CopyText(html, start->next->element, True, &foo, False);

	/* create new form entry. text will serve as the default content */
	if((form_entry = _XmHTMLFormAddTextArea(html, start->attributes,
		text)) == NULL)
	{
		if(text)
			free(text);
		return(NULL);
	}
	form_entry->data = owner;

	/* allocate new word for this form member */
	word = allocFormWord(form_entry, width, height, owner, formatted);

	_XmHTMLFullDebug(2, ("format.c: TextAreaToWord, word is %s, len is %i, "
		"width is %i, height is %i\n", word->word, word->len,
		word->width, word->height));

	*num_words = 1;
	return(word);
}

/*****
* Name:			indexToWord
* Return Type: 	XmHTMLWord
* Description: 	creates a prefix for numbered lists with the ISINDEX
*				attribute set.
* In: 
*	html:		XmHTMLWidget id;
*	list_stack:	stack of all lists;
*	current...:	current list id;
*	owner:		owning element.
*	formatted:	true when this form component is placed in a <pre></pre> tag.
* Returns:
*	a newly allocated word.	
* Note:
*	This routine creates the prefix based on the type and depth of the
*	current list. All types can be intermixed, so this routine is capable
*	of returning something like 1.A.IV.c.iii for a list nested five levels,
*	the first with type `1', second with type `A', third with type `I',
*	fourth with type `a' and fifth with type `i'.
*****/
static XmHTMLWord* 
indexToWord(XmHTMLWidget html, listStack list_stack[], int current_list,
	XmHTMLObjectTableElement owner, Boolean formatted)
{
	static XmHTMLWord *word;
	int i;
	char index[42], number[42];		/* enough for a zillion numbers & depths */

	word = (XmHTMLWord*)malloc(sizeof(XmHTMLWord));

	(void)memset(&index, '\0', 42);
	for(i = 0; i < current_list; i++)
	{
		if(list_stack[i].type == HT_OL)
		{
			switch(list_stack[i].marker)
			{
				case XmMARKER_ALPHA_LOWER: 
					sprintf(number, "%s.", ToAsciiLower(list_stack[i].level));
					break;
				case XmMARKER_ALPHA_UPPER: 
					sprintf(number, "%s.", ToAsciiUpper(list_stack[i].level));
					break;
				case XmMARKER_ROMAN_LOWER:
					sprintf(number, "%s.", ToRomanLower(list_stack[i].level));
					break;
				case XmMARKER_ROMAN_UPPER:
					sprintf(number, "%s.", ToRomanUpper(list_stack[i].level));
					break;
				case XmMARKER_ARABIC:
				default:
					sprintf(number, "%i.", list_stack[i].level);
					break;
			}
			/* no buffer overflow */
			if(strlen(index) + strlen(number) > 42)
				break;
			strcat(index, number);
		}
	}

	word->word = strdup(index);
	word->len  = strlen(index);

	word->base      = NULL;					/* unused */
	word->self      = word;					/* unused */
	word->x         = word->y = 0;			/* unused */
	word->height    = 0;					/* unused */
	word->width     = 0;					/* unused */
	word->owner     = owner;				/* unused */
	word->font      = font_base.font;		/* unused */
	word->spacing   = formatted ? 0 : TEXT_SPACE_NONE;
	word->type      = OBJ_TEXT;				/* unused */
	word->line_data = NO_LINE;				/* unused */
	word->line      = 0;					/* unused */
	word->image     = NULL;					/* unused */
	word->form      = NULL;					/* unused */

	return(word);
}

/*****
* Name: 		TextToPre
* Return Type: 	XmHTMLWord*
* Description: 	splits the given text into an array of preformatted lines
* In: 
*	text:		text to split
*	num_words:	number of words in the given text. Filled upon return;
*	font:		font to use for this text.
* Returns:
*	an array of words. When allocation fails, this routine exits.
* Note:
*	the static var nchars is used to propagate the tab index to another
*	chunk of preformatted text if the current text is a block of preformatted
*	text with whatever formatting. It is only reset if an explicit newline
*	is encountered.
*****/
static XmHTMLWord* 
TextToPre(String text, int *num_words, XmHTMLfont *font, Byte line_data, 
	XmHTMLObjectTableElement owner)
{
	int nwords, len, i, j, ntabs, max_width, in_word, size, nfeeds;
	static char *raw;
	static XmHTMLWord *words;
	static int nchars = 0;
	register char *chPtr, *start, *end;
#ifdef DEBUG
	int used;
#endif

	/* sanity check */
	if(text == NULL)
	{
		*num_words = 0;
		return(NULL);
	}
 
	_XmHTMLFullDebug(2, ("format.c: TextToPre, text in is:\n%s\n", text));

	chPtr = text;
	raw = NULL;
		
	/***** 
	* compute how many words we have. A preformatted word is started
	* with a printing char and is terminated by either a newline or a
	* sequence of whitespaces. Multiple newlines are collapsed into a 
	* single word where the height of the word indicates the number of
	* newlines to insert.
	* The in_word logic comes from GNU wc.
	*****/
	in_word = nwords = ntabs = 1;	/* fix 01/30/97-02, kdh */
	while(True)
	{
		switch(*chPtr)
		{
			/* tabs and single spaces are collapsed */
			case '\t':	/* horizontal tab */
			case ' ':
				if(in_word)
				{
					while(*chPtr != '\0' && (*chPtr == ' ' || *chPtr == '\t'))
					{
						if(*chPtr == '\t')
							ntabs++;	/* need to know how many to expand */
						chPtr++;
					}
					nwords++;
					in_word = False;
				}
				else
				{
					/* fix 03/23/97-01, kdh */
					if(*chPtr == '\t')
						ntabs++;	/* need to know how many to expand */
					chPtr++;
				}
				break;
			/* newlines reset the tab index and are collapsed */
			case '\n':
				while(*chPtr != '\0' && *chPtr == '\n')
					chPtr++;
				nwords++;	/* current word is terminated */
				nchars = 1;
				break;
			default:
				chPtr++;
				in_word = True;
				break;
		}
		if(*chPtr == '\0')
			break;
	}

	/* sanity check */
	if(nwords == 0)
	{
		*num_words = 0;
		return(NULL);
	}

	/* add an extra word and tab for safety */
	nwords++;	/* preformatted text with other formatting *needs* this */
	ntabs++;

	/* compute amount of memory to allocate */
	size = ((ntabs*8)+strlen(text)+1)*sizeof(char);

	raw = (char*)malloc(size);

	_XmHTMLDebug(2, ("format.c: TextToPre, allocated %i bytes\n", size));

	/* allocate memory for all words */
	words = (XmHTMLWord*)calloc(nwords, sizeof(XmHTMLWord));

	chPtr = text;
	end = raw;
#ifdef DEBUG
	used = 0;
#endif
	/* first filter out all whitespace and other non-printing characters */
	while(True)
	{
		switch(*chPtr)
		{
			case '\f':	/* formfeed, ignore */
			case '\r':	/* carriage return, ignore */
			case '\v':	/* vertical tab, ignore */
				chPtr++;
#ifdef DEBUG
				used++;
#endif
				break;	
			case '\t':	/* horizontal tab */
				/* no of ``floating spaces'' to emulate a tab */
				len = ((nchars / 8) + 1) * 8;
				for(j = 0; j < (len - nchars); j++)
				{
					*end++ = ' ';		/* insert a tab */
#ifdef DEBUG
					used++;
#endif
				}
				nchars = len;
#ifdef DEBUG
				used++;
#endif
				chPtr++;
				break;
			/* newlines reset the tab index */
			case '\n':
				nchars = 0;	/* reset tab spacing index */
				/* fall thru */
			default:
				nchars++;
				*end++ = *chPtr++;
#ifdef DEBUG
				used++;
#endif
				break;
		}
		if(*chPtr == '\0')	/* terminate loop */
		{
			*end = '\0';
			break;
		}
	}
	_XmHTMLDebug(2, ("format.c: TextToPre, %i bytes actually used\n", used));

	/* Now go and allocate all words */
	start = end = raw;
	max_width = i = len = 0;
	nfeeds = 0;

	while(True)
	{
		/* also pick up the last word! */
		if(*end == ' ' || *end == '\n' || *end == '\0')
		{
			if(*end)
			{
				/* skip past all spaces */
				while(*end != '\0' && *end != '\n' && *end == ' ')
				{
					end++;
					len++;
				}

				/***** 
				* if this word is ended by a newline, remove the newline.
				* X doesn't know how to interpret them.
				* We also want to recognize multiple newlines, so we must
				* skip past them.
				*****/
				if(*end == '\n')
				{
					while(*end != '\0' && *end == '\n')
					{
						nfeeds++;
						*end++ = '\0';
					}
					/*****
					* Since the no of newlines to add is stored in a
					* Byte, we need to limit the no of newlines to the
					* max. value a Byte can have: 255 (= 2^8)
					*****/
					if(nfeeds > 255)
						nfeeds = 255;
				}
			}

			words[i].type      = OBJ_TEXT;
			words[i].base      = NULL;
			words[i].self      = &words[i];
			words[i].x         = words[i].y = 0;
			words[i].word      = start;
			words[i].height    = font->height;
			words[i].owner     = owner;
			words[i].spacing   = (Byte)nfeeds;	/* no of newlines */
			words[i].font      = font;
			words[i].line_data = line_data;
			words[i].line      = 0;
			words[i].len       = len;
			words[i].width     = Toolkit_Text_Width(font->xfont, words[i].word, len);
			start = end;
			i++;
			len = 0;
			nfeeds = 0;
		}
		if(*end == '\0')	/* terminate loop */
			break;
		end++;	/* move to the next char */
		len++;
	}

	_XmHTMLDebug(2, ("format.c: TexToPre, allocated %i words, %i actually "
		"used\n", nwords, i));

	/* total no of words */
	*num_words = i;
	return(words);
}
  
/*****
* Name: 		SetTab
* Return Type: 	XmHTMLWord*
* Description: 	returns a XmHTMLWord with spaces required for a tab.
* In: 
* Returns:
*	the tab.
*****/
static XmHTMLWord* 
SetTab(int size, Dimension *height, XmHTMLfont *font, 
	XmHTMLObjectTableElement owner)
{
	static XmHTMLWord *tab;
	static char *raw;

	/* The tab itself */
	raw = (char*)malloc((size+1)*sizeof(char));

	/* fill with spaces */
	(void)memset(raw, ' ', size);
	raw[size] = '\0'; /* NULL terminate */

	tab = (XmHTMLWord*)malloc(sizeof(XmHTMLWord));

	/* Set all text fields for this tab */
	tab->base      = NULL;
	tab->self      = tab;
	tab->x         = tab->y = 0;
	tab->word      = raw;
	tab->len       = size;
	tab->height    = *height = font->height;
	tab->width     = Toolkit_Text_Width(font->xfont, raw, size);
	tab->owner     = owner;
	tab->spacing   = TEXT_SPACE_NONE;	/* a tab is already spacing */
	tab->font      = font;
	tab->type      = OBJ_TEXT;
	tab->line_data = NO_LINE;

	return(tab);	
}

/*****
* Name: 		CopyText
* Return Type: 	String
* Description: 	copies the given text to a newly malloc'd buffer.
* In: 
*	html:		XmHTMLWidget id;
*	text:		text to clean out.
*	formatted:	true when this text occurs inside <pre></pre>
*	text_data:	text option bits, spacing and such
*	expand_escapes:	
*				True -> expand escape sequences in text. Only viable when
*				copying pre-formatted text (plain text documents are handled
*				internally as consisting completely of preformatted text for
*				which the escapes may not be expanded).
* Returns:
*	cleaned up text. Terminates if malloc fails.
*****/
static String 
CopyText(XmHTMLWidget html, String text, Boolean formatted, Byte *text_data,
	Boolean expand_escapes)
{
	static String ret_val;
	char *start = text;
	int len;
	static Boolean have_space = False;

	/* sanity check */
	if(*text == '\0' || !strlen(text))
		return(NULL);

	/* preformatted text, just copy and return */
	if(formatted)
	{
		*text_data = TEXT_SPACE_NONE;
		ret_val    = strdup(text);
		/* expand all escape sequences in this text */
		if(expand_escapes)
			_XmHTMLExpandEscapes(ret_val, html->html.bad_html_warnings);
		have_space = False;
		return(ret_val);
	}

	_XmHTMLFullDebug(2, ("format.c: CopyText, text in is:\n%s\n", text));

	/* initial length of full text */
	len = strlen(text);

	*text_data = 0;

	/* see if we have any leading/trailing spaces */
	if(isspace(*text) || have_space)
		*text_data = TEXT_SPACE_LEAD;

	if(isspace(text[len-1]))
		*text_data |= TEXT_SPACE_TRAIL;

	/*****
	* Remove leading/trailing spaces
	* very special case: spaces between different text formatting
	* elements must be retained
	*****/
	/* remove all leading space */
	while(*start != '\0' && isspace(*start))
		start++;
	/* remove all trailing space */
	len = strlen(start);
	while(len > 0 && isspace(start[len-1]))
		len--; 

	/*****
	* Spaces *can* appear between different text formatting elements. 
	* We want to retain this spacing since the above whitespace checking
	* only yields the current element, and does not take the previous text
	* element into account.
	* So when the current element doesn't have any leading or trailing spaces,
	* we use the spacing from the previous full whitespace element.
	* Obviously we must reset this data if we have text to process.
	*
	* Very special case: this text only contains whitespace and its therefore
	* most likely just spacing between formatting elements.
	* If the next text elements are in the same paragraph as this single
	* whitespace, we need to add a leading space if that text doesn't have
	* leading spaces. That's done above. If we have plain text, we reset
	* the prev_spacing or it will mess up the layout later on.
	*****/
	if(!len)
	{
		have_space = True;
		return(NULL);
	}
	have_space = False;

	/*
	* We are a little bit to generous here: consecutive multiple whitespace 
	* will be collapsed into a single space, so we may over-allocate. 
	* Hey, better to overdo this than to have one byte to short ;-)
	*/
	ret_val = (String)malloc((len+1)*sizeof(char));
	strncpy(ret_val, start, len);	/* copy it */
	ret_val[len] = '\0';			/* NULL terminate */

	/* expand all escape sequences in this text */
	if(expand_escapes)
		_XmHTMLExpandEscapes(ret_val, html->html.bad_html_warnings);

	return(ret_val);
}

/*****
* Name: 		ParseBodyTags
* Return Type: 	void
* Description: 	checks the <BODY> element for additional tags
* In: 
*	w:			HTML TWidget to check
*	data:		body element data.
* Returns:
*	nothing, but the HTML TWidget is updated to reflect the body stuff.
* Note:
*	hugely inefficient due to the various _XmHTMLGetAnchorValue calls.
*****/
static void 
ParseBodyTags(XmHTMLWidget html, XmHTMLObject *data)
{
	char *chPtr;
	Boolean bg_color_set = False;	/* flag for bodyImage substitution */

	/* check all body color tags */
	if(html->html.body_colors_enabled)
	{
		Boolean doit = True;

		if((chPtr = _XmHTMLTagGetValue(data->attributes, "text")))
		{
			if(html->html.strict_checking)
				doit = _XmHTMLConfirmColor32(chPtr);

			if(doit)
				html->html.body_fg = _XmHTMLGetPixelByName(html, chPtr, 
					html->html.body_fg_save);
			free(chPtr);

#ifdef WITH_MOTIF
			html->manager.foreground = html->html.body_fg;
#else
			/* FEDERICO */
			GTK_WIDGET(html)->style->fg[GTK_STATE_NORMAL].pixel = html->html.body_fg;
			/* XXX: do we have to set it for all the states? */
			/* XXX: why does it not set the gc foreground as well? */
#endif
		}

		if(doit && (chPtr = _XmHTMLTagGetValue(data->attributes, "bgcolor")))
		{
			bg_color_set = True;

			if(html->html.strict_checking)
				doit = _XmHTMLConfirmColor32(chPtr);

			/* only change if we had success */
			if(doit)
			{
				html->html.body_bg = _XmHTMLGetPixelByName(html, chPtr, 
					html->html.body_bg_save);

				/* also set as background for the entire TWidget */
#ifdef WITH_MOTIF
				html->core.background_pixel = html->html.body_bg;
				XtVaSetValues(html->html.work_area,
					XmNbackground, html->html.body_bg, NULL);
#else
				/* FEDERICO */
				GTK_WIDGET(html)->style->bg[GTK_STATE_NORMAL].pixel = html->html.body_bg;
				/* FIXME: set the background resource equivalent */
#endif
				/* get new values for top, bottom & highlight */
				_XmHTMLRecomputeColors(html);
			}

			free(chPtr);
		}

		if(doit && (chPtr = _XmHTMLTagGetValue(data->attributes, "link")))
		{
			if(html->html.strict_checking)
				doit = _XmHTMLConfirmColor32(chPtr);

			if(doit)
				html->html.anchor_fg = _XmHTMLGetPixelByName(html, chPtr, 
					html->html.anchor_fg_save);
			free(chPtr);
		}

		if(doit && (chPtr = _XmHTMLTagGetValue(data->attributes, "vlink")))
		{
			if(html->html.strict_checking)
				doit = _XmHTMLConfirmColor32(chPtr);
			if(doit)
				html->html.anchor_visited_fg = _XmHTMLGetPixelByName(html,
					chPtr, html->html.anchor_visited_fg_save);
			free(chPtr);
		}

		if(doit && (chPtr = _XmHTMLTagGetValue(data->attributes, "alink")))
		{
			if(html->html.strict_checking)
				doit = _XmHTMLConfirmColor32(chPtr);
			if(doit)
				html->html.anchor_activated_fg = _XmHTMLGetPixelByName(html,
					chPtr, html->html.anchor_activated_fg_save);
			free(chPtr);
		}
		/*****
		* an invalid color spec, ignore them all together and revert to
		* saved settings
		*****/
		if(doit == False)
		{
			/* first check if we changed the background color */
#ifdef WITH_MOTIF
			if(html->core.background_pixel != html->html.body_bg_save)
			{
				html->html.body_fg          = html->html.body_fg_save;
				html->html.body_bg          = html->html.body_bg_save;
				html->manager.foreground    = html->html.body_fg;
				html->core.background_pixel = html->html.body_bg;
				XtVaSetValues(html->html.work_area,
					XmNbackground, html->html.body_bg, NULL);

				/* restore values for top, bottom & highlight */
				_XmHTMLRecomputeColors(html);
			}
#else
			/* FEDERICO */

			/* XXX: I don't know whether this is correct at all */

			if (GTK_WIDGET(html)->style->bg[GTK_STATE_NORMAL].pixel != html->html.body_bg_save)
			{
				html->html.body_fg = html->html.body_fg_save;
				html->html.body_bg = html->html.body_bg_save;
				GTK_WIDGET(html)->style.fg[GTK_STATE_NORMAL].pixel = html->html.body_fg;
				GTK_WIDGET(html)->style.bg[GTK_STATE_NORMAL].pixel = html->html.body_bg;
				/* XXX: set the background resource equivalent */
			}
					
#endif
			html->html.body_fg            = html->html.body_fg_save;
			html->html.body_bg            = html->html.body_bg_save;
			html->html.anchor_fg          = html->html.anchor_fg_save;
			html->html.anchor_visited_fg  = html->html.anchor_visited_fg_save;
			html->html.anchor_activated_fg=html->html.anchor_activated_fg_save;
#ifdef WITH_MOTIF
			html->manager.foreground      = html->html.body_fg;
#else
			/* FEDERICO */
			GTK_WIDGET(html)->style->fg[GTK_STAT_NORMAL].pixel = html->html.body_fg;
#endif
			bg_color_set = False;
		}
	}

	/* Check background image spec. First invalidate any existing body image */
	if(html->html.body_image)
		html->html.body_image->options |= IMG_ORPHANED;
	html->html.body_image = (XmHTMLImage*)NULL;
	html->html.body_image_url = NULL;

	/*
	* *ALWAYS* load the body image if we want the SetValues method to
	* behave itself.
	*/
	if((chPtr = _XmHTMLTagGetValue(data->attributes, "background")))
	{
		_XmHTMLLoadBodyImage(html, chPtr);
		/* store document's body image location */
		if(html->html.body_image)
			html->html.body_image_url = html->html.body_image->url;
		free(chPtr);
	}
	/*
	* Use default body image if present *and* if no background color
	* has been set.
	*/
	else if(!bg_color_set && html->html.def_body_image_url)
		_XmHTMLLoadBodyImage(html, html->html.def_body_image_url);

	/*
	* Now nullify it if we aren't to show the background image.
	* makes sense huh?
	*/
	if(!html->html.images_enabled || !html->html.body_images_enabled)
	{
		if(html->html.body_image)
			html->html.body_image->options |= IMG_ORPHANED;
		html->html.body_image = NULL;
	}
	/*****
	* When a body image is present it is very likely that a highlight
	* color based upon the current background actually makes an anchor
	* invisible when highlighting is selected. Therefore we base the
	* highlight color on the activated anchor background when we have a body
	* image, and on the document background when no body image is present.
	*****/
	if(html->html.body_image)
		_XmHTMLRecomputeHighlightColor(html, html->html.anchor_activated_fg);
	else
		_XmHTMLRecomputeHighlightColor(html, html->html.body_bg);
}

/*****
* Name: 		CheckLineFeed
* Return Type: 	int
* Description: 	checks wether the requested newline is honored.
* In: 
*	this:		newline to add.
*	force:		add the requested newline anyway
* Returns:
*	computed vertical pixel offset.
* Note:
*	any of CLEAR_NONE, CLEAR_SOFT or CLEAR_HARD
*****/
static int
CheckLineFeed(int this, Boolean force)
{
	static int prev_state = CLEAR_NONE;
	int ret_val = this;

	if(force)
	{
		prev_state = this;
		return(ret_val);
	}

	/* multiple soft and hard returns are never honored */
	switch(this)
	{
		case CLEAR_HARD:
			if(prev_state == CLEAR_SOFT)
			{
				ret_val = CLEAR_SOFT;
				prev_state = CLEAR_HARD;
				break;
			}
			if(prev_state == CLEAR_HARD)
			{
				ret_val = CLEAR_NONE;
				prev_state = CLEAR_HARD;
				break;
			}
			prev_state = ret_val = this;
			break;
		case CLEAR_SOFT:
			if(prev_state == CLEAR_SOFT)
			{
				ret_val = CLEAR_NONE;
				prev_state = CLEAR_SOFT;
				break;
			}
			if(prev_state == CLEAR_HARD)
			{
				ret_val = CLEAR_NONE;
				prev_state = CLEAR_HARD;
				break;
			}
			ret_val = prev_state = this;
			break;
		case CLEAR_NONE:
			ret_val = prev_state = this;
			break;
	}
	return(ret_val);
}

static void
PushAlignment(Alignment align)
{
	alignStack *tmp;

	tmp = (alignStack*)malloc(sizeof(alignStack));
	tmp->align = align;
	tmp->next = align_stack;
	align_stack = tmp;
}

static Alignment
PopAlignment(void)
{
	Alignment align;
	alignStack *tmp;

	if(align_stack->next != NULL)
	{
		tmp = align_stack;
		align_stack = align_stack->next;
		align = tmp->align;
		free(tmp);
	}
	else
	{
		_XmHTMLDebug(2, ("XmHTML: negative alignment stack!\n"));
		align = align_stack->align;
	}
	return(align);
}

static void
PushColor(Pixel color)
{
	colorStack *tmp;

	tmp = (colorStack*)malloc(sizeof(colorStack));
	tmp->color = color;
	tmp->next = color_stack;
	color_stack = tmp;
}

static Pixel
PopColor(void)
{
	Pixel color;
	colorStack *tmp;

	if(color_stack->next != NULL)
	{
		tmp = color_stack;
		color_stack = color_stack->next;
		color = tmp->color;
		free(tmp);
	}
	else
	{
		_XmHTMLDebug(2, ("XmHTML: negative color stack!\n"));
		color = color_stack->color;
	}
	return(color);
}

static void
PushFont(XmHTMLfont *font, int size)
{
	fontStack *tmp;

	tmp = (fontStack*)malloc(sizeof(fontStack));
	tmp->font = font;
	tmp->size = size;
	tmp->next = font_stack;
	font_stack = tmp;
#ifdef DEBUG
	_XmHTMLDebug(2, ("format.c: PushFont, pushed font %s\n", font->font_name));
#endif
}

static XmHTMLfont* 
PopFont(int *size)
{
	XmHTMLfont *font;
	fontStack *tmp;

	if(font_stack->next != NULL)
	{
		tmp = font_stack;
		font_stack = font_stack->next;
		font = tmp->font;
		*size = tmp->size;
		free(tmp);
	}
	else
	{
		_XmHTMLDebug(2, ("XmHTML: negative font stack!\n"));
		font = font_stack->font;
		*size = font_stack->size;
	}
#ifdef DEBUG
	_XmHTMLDebug(2, ("format.c: PopFont, popped font %s\n", font->font_name));
#endif
	return(font);
}

#define PUSH_COLOR(TWidget) { \
	char *chPtr; \
	/* check for color spec */ \
	PushColor(fg); \
	chPtr = _XmHTMLTagGetValue(temp->attributes, "color"); \
	if(chPtr != NULL) \
	{ \
		Boolean doit = True; \
		if(html->html.strict_checking) \
			doit = _XmHTMLConfirmColor32(chPtr); \
		if(doit) fg = _XmHTMLGetPixelByName(html, chPtr, fg); \
		free(chPtr); \
	} \
}

#define CHECK_LINE { \
	if(element_data & ELE_ANCHOR) { \
		if(element_data & ELE_ANCHOR_TARGET) \
			line_data = html->html.anchor_target_line; \
		else if(element_data & ELE_ANCHOR_VISITED) \
			line_data = html->html.anchor_visited_line; \
		else \
			line_data = html->html.anchor_line; \
	} \
	/* ignore <u> for anchors */ \
	else { \
		if(element_data & ELE_UNDERLINE) \
			line_data  = LINE_SOLID | LINE_UNDER; \
	} \
	/* check strikeout flag */ \
	if(element_data & ELE_STRIKEOUT) \
		line_data |= LINE_STRIKE; \
}

/********
****** Private XmHTML Functions
********/

/*****
* Name: 		XmHTMLGetURLType
* Return Type: 	URLType
* Description: 	tries to figure out what type of url the given href is
* In: 
*	href:		url specification
* Returns:
*	type of url when we know it, ANCHOR_UNKNOWN otherwise.
* Note:
*	this routine is quite forgiving on typos of any url spec: only the
*	first character is checked, the remainder doesn't matter.
*****/
URLType
XmHTMLGetURLType(String href)
{
	char *chPtr;

	if(href == NULL || *href == '\0')
		return(ANCHOR_UNKNOWN);

	_XmHTMLDebug(2, ("format.c: XmHTMLGetURLType; checking url type of %s\n",
		href));

	/* first pick up any leading url spec */
	if((chPtr = strstr(href, ":")) != NULL)
	{
		/* check for URL types we know of. Do in most logical order(?) */
		if(!strncasecmp(href, "http", 4))
			return(ANCHOR_HTTP);
		if(!strncasecmp(href, "mailto", 6))
			return(ANCHOR_MAILTO);
		if(!strncasecmp(href, "ftp", 3))
			return(ANCHOR_FTP);
		if(!strncasecmp(href, "file", 4))
			return(ANCHOR_FILE_REMOTE);
		if(!strncasecmp(href, "news", 4))
			return(ANCHOR_NEWS);
		if(!strncasecmp(href, "telnet", 6))
			return(ANCHOR_TELNET);
		if(!strncasecmp(href, "gopher", 6))
			return(ANCHOR_GOPHER);
		if(!strncasecmp(href, "wais", 4))
			return(ANCHOR_WAIS);
		if(!strncasecmp(href, "exec", 4) ||
			!strncasecmp(href, "xexec", 5))
			return(ANCHOR_EXEC);
		_XmHTMLDebug(2, ("format.c: XmHTMLGetURLType; unknown type\n"));
		return(ANCHOR_UNKNOWN);
	}
	return(href[0] == '#' ? ANCHOR_JUMP : ANCHOR_FILE_LOCAL);
}

/*****
* Name: 		_XmHTMLNewAnchor
* Return Type:	XmHTMLAnchor *
* Description: 	allocates and fills an anchor object
* In: 
*	html:		owning TWidget
*	object:		raw anchor data
* Returns:
*	the allocated object
*****/
XmHTMLAnchor*
_XmHTMLNewAnchor(XmHTMLWidget html, XmHTMLObject *object)
{
	static XmHTMLAnchor *anchor;

	/* stupid sanity check */
	if(object->attributes == NULL)
		return(NULL);
	
	anchor = (XmHTMLAnchor*)malloc(sizeof(XmHTMLAnchor));

	/* set all fields to zero */
	(void)memset(anchor, 0, sizeof(XmHTMLAnchor));

	/* anchors can be both named and href'd at the same time */
	anchor->name = _XmHTMLTagGetValue(object->attributes, "name");

	/* get the url specs */
	parseHref(object->attributes, anchor); 

	/* get the url type */
	anchor->url_type = XmHTMLGetURLType(anchor->href);

	/* promote to named if necessary */
	if(anchor->url_type == ANCHOR_UNKNOWN && anchor->name)
		anchor->url_type = ANCHOR_NAMED;

#ifdef PEDANTIC
	if(anchor->url_type == ANCHOR_UNKNOWN)
	{
		_XmHTMLWarning(__WFUNC__(html, "_XmHTMLNewAnchor"), "Could "
			"not determine URL type for anchor %s (line %i of input)\n",
			object->attributes, object->line);
	}
#endif /* PEDANTIC */

	/* 
	* If we have a proc available for anchor testing, call it and 
	* set the visited field.
	*/
	if(html->html.anchor_visited_proc)
	{
		anchorProc proc = (anchorProc)html->html.anchor_visited_proc;
		if((*proc)((TWidget)html, anchor->href)) 
			anchor->visited = True;
	}
	/* insert in the anchor list */
	if(list_data.anchor_head)
	{
		/*****
		* We can't do anything about duplicate anchors. Removing them
		* would mess up the named anchor lookup table.
		*****/
		list_data.anchor_current->next = anchor;
		list_data.anchor_current = anchor;
	}
	else
	{
		list_data.anchor_head = list_data.anchor_current = anchor;
	}
	return(anchor);
}

/*****
* Name:			_XmHTMLformatObjects
* Return Type:	XmHTMLObjectTable*
* Description:	creates a list of formatted HTML objects.
* In:
*	old_table:	a formatted object list to be freed.
*	w:			TWidget containing parsed html data
* Returns:
*	list of formatted html objects
*****/
XmHTMLObjectTable*
_XmHTMLformatObjects(XmHTMLObjectTable *old_table, XmHTMLAnchor *old_anchor,
	XmHTMLWidget html)
{
	/* text level variables */
	String text;
	int linefeed, n_words, anchor_words, named_anchors;
	int x_offset = 0, y_offset = 0;
	Byte text_data, line_data;		/* text and line data bits */
	unsigned long element_data = 0;
	XmHTMLWord *words;
	XmHTMLAnchor *anchor_data, *form_anchor_data;
#ifdef DEBUG
	int num_ignore = 0;
	static String func = "_XmHTMLformatObjects";
#endif

	/* list variables */
	int ul_level, ol_level, ident_level, current_list;
	listStack list_stack[MAX_NESTED_LISTS];

	/* remaining object variables */
	Pixel fg, bg;
	Dimension width, height;
	Alignment halign, valign; 
	ObjectType object_type;
	XmHTMLfont *font;
	static XmHTMLObjectTableElement element;
	XmHTMLObjectTableElement previous_element = NULL;
	int basefont;

	/* imagemap and area stuff */
	XmHTMLImageMap *imageMap = NULL;

	/* local flags */
	Boolean ignore = False, in_pre = False;

	/* misc. variables */
	XmHTMLObject *temp;
	int i, new_anchors = 0;
	Boolean anchor_data_used = False;

#ifdef DEBUG
	allocated = 0;
#endif

	/* Free any previous lists and initialize it */
	InitObjectTable(old_table, old_anchor);

	/* need to reset list ptrs as well, fix 09/17/97-01, kdh */
	html->html.anchor_data = (XmHTMLAnchor*)NULL;

	/* 
	* Nothing to do, just return. Should only happen if we get called
	* from Destroy().
	*/
	if(html->html.elements == NULL)
	{
		/* free top of list */
		if(list_data.head)
			free(list_data.head);
		list_data.head = NULL;
		return(NULL);
	}

	/* Move to the body element */
	for(temp = html->html.elements; temp != NULL && temp->id != HT_BODY; 
		temp = temp->next);

	/*
	* No <body> element found. This is an error since the parser will
	* *always* add a <body> element if none is present in the source
	* document.
	*/
	if(temp == NULL)
	{
		_XmHTMLWarning(__WFUNC__(html, func), "Nothing to display: no "
			"<BODY> tag found (HTML parser failure)!");
		return(NULL);
	}

	/* initialize font stack */
	font_stack = &font_base;
	font_stack->font = font = _XmHTMLSelectFontCache(html, False);
	font_stack->size = basefont = 4;
	font_stack->next = NULL;

	/* Reset anchor count */
	anchor_words = 0;
	named_anchors = 0;
	anchor_data = form_anchor_data = NULL;

	/* initialize list variables */
	ul_level = 0;
	ol_level = 0;
	ident_level = 0;
	current_list = 0;

	/* reset stacks */
	for(i = 0; i < MAX_NESTED_LISTS; i++)
	{
		list_stack[i].isindex = False;
		list_stack[i].marker  = XmMARKER_NONE;
		list_stack[i].level   = 0;
		list_stack[i].type    = HT_ZTEXT;
	}

	/* Initialize linefeeding mechanism */
	linefeed = CheckLineFeed(CLEAR_SOFT, True);

	/* Initialize alignment */
	align_stack = &align_base;
	align_stack->align = halign = html->html.default_halign;
	align_stack->next = NULL;
	valign = XmVALIGN_NONE;
	object_type = OBJ_NONE;

	/* check for background stuff */
	ParseBodyTags(html, temp);

	/* foreground color to use */
	fg = html->html.body_fg;
	/* background color to use */
	bg = html->html.body_bg;

	/* Initialize color stack */
	color_stack = &color_base;
	color_stack->color = fg;
	color_stack->next = NULL;

	/* move to the next element */
	temp = temp->next;

	/*
	* Only elements between <BODY></BODY> elements are really interesting.
	* BUT: if the HTML verification/reparation routines in parse.c should
	* fail, we might have a premature </body> element, so we don't check on
	* it but walk thru every item found.
	*/
	while(temp != NULL)
	{
		/* create new element */
		element = NewTableElement(temp);

		/* Initialize all fields changed in here */
		text = NULL;
		ignore = False;
		object_type = OBJ_NONE;
		n_words = 0;
		width = height = 0;
		words = NULL;
		linefeed = CLEAR_NONE;
		line_data = NO_LINE;
		text_data = TEXT_SPACE_NONE;

		_XmHTMLDebug(2, ("format.c, _XmHTMLformatObjects, object data:\n"
			"\telement id: %s\n\tattributes: %s\n\tis_end: %s\n",
			html_tokens[temp->id], 
			temp->attributes ? temp->attributes : "<none>",
			temp->is_end ? "Yes" : "No"));

		switch(temp->id)
		{
			/* plain text */
			case HT_ZTEXT:
				/*
				* text_data gets completely reset in CopyText.
				* We do not want escape expansion if we are loading a plain
				* text document.
				*/
				if((text = CopyText(html, temp->element, in_pre, &text_data,
					html->html.mime_id != XmNONE)) == NULL)
				{
					/* 
					* named anchors can be empty, so keep them but mark the
					* element as empty.
					*/
					if(!(element_data & ELE_ANCHOR_INTERN))
						ignore = True; /* ignore empty text fields */
					else
						object_type = OBJ_NONE;
					break;
				}
				if(!in_pre)
				{
					object_type = OBJ_TEXT;
					CollapseWhiteSpace(text);
					/*
					* If this turns out to be an empty block, ignore it,
					* but only if it's not an empty named anchor.
					*/
					if(strlen(text) == 0)
					{
						if(!(element_data & ELE_ANCHOR_INTERN))
							ignore = True;
						else
							object_type = OBJ_NONE;
						free(text);
						break;
					}
					/* check line data */
					CHECK_LINE;
					/* convert text to a number of words */
					words = TextToWords(text, &n_words, &height, font, 
						line_data, text_data, element);
				}
				else
				{
					object_type = OBJ_PRE_TEXT;
					/* check line data */
					CHECK_LINE;
					/* convert text to a number of words, keep formatting. */
					words = TextToPre(text, &n_words, font, line_data, element);
				}
				/* No returns for plain text, reset */
				linefeed = CheckLineFeed(CLEAR_NONE, False);
				break;

			/* images */
			case HT_IMG:
				if((words = ImageToWord(html, temp->attributes, &n_words,
					&height, element, in_pre)) == NULL)
				{
					ignore = True;
					break;
				}
				text_data |= TEXT_IMAGE;
				object_type = in_pre ? OBJ_PRE_TEXT : OBJ_TEXT;
				/* No explicit returns for images, reset */
				linefeed = CheckLineFeed(CLEAR_NONE, False);
				break;

			/* anchors */
			case HT_A:
				if(temp->is_end)
				{
					/*
					* this is a very sneaky hack: since empty named anchors
					* are allowed, we must store it somehow. And this is how
					* we do it: set object_type to OBJ_NONE, ignore to False
					* and back up one element.
					*/
					if(!anchor_data_used && (element_data & ELE_ANCHOR_INTERN))
					{
						_XmHTMLDebug(2, ("format.c: _XmHTMLformatObjects, "
							"adding bogus named anchor %s\n", 
							anchor_data->name));
						object_type = OBJ_NONE;
						anchor_data_used = True;
						temp = temp->prev;
						ignore = False;
						break;
					}
					/* unset anchor bitfields */
					element_data &= ( ~ELE_ANCHOR & ~ELE_ANCHOR_TARGET & 
							~ELE_ANCHOR_VISITED & ~ELE_ANCHOR_INTERN);
					fg = PopColor();
					bg = html->html.body_bg;
					anchor_data = NULL;
					ignore = True;	/* only need anchor data */
					_XmHTMLDebug(2,("format.c: _XmHTMLformatObjects: anchor "
						"end\n"));
				}
				else
				{
					/* allocate a new anchor */
					anchor_data = _XmHTMLNewAnchor(html, temp);

					/* sanity check */
					if(!anchor_data)
					{
						ignore = True;
						break;
					}
					/* save current color */
					PushColor(fg);

					new_anchors++;
					anchor_data_used = False;

					/* set proper element bits */

					/* maybe it's a named one */
					if(anchor_data->name)
						element_data |= ELE_ANCHOR_INTERN;

					/*
					* maybe it's also a plain anchor. If so, see what 
					* foreground color we have to use to render this
					* anchor.
					*/
					if(anchor_data->href[0] != '\0')
					{
						element_data |= ELE_ANCHOR;
						fg = html->html.anchor_fg;

						/* maybe it's been visited */
						if(anchor_data->visited)
						{
							element_data |= ELE_ANCHOR_VISITED;
							fg = html->html.anchor_visited_fg;
						}
						/* maybe it's a target */
						else if(anchor_data->target)
						{
							element_data |= ELE_ANCHOR_TARGET;
							fg = html->html.anchor_target_fg;
						}
					}
					_XmHTMLDebug(2,("format.c: _XmHTMLformatObjects: anchor "
						"start\n"));
					ignore = True;	/* only need anchor data */
				}
				break;

			/* font changes */
			case HT_CITE:		/* italic */
			case HT_I:			/* italic */
			case HT_EM:			/* italic */
			case HT_DFN:		/* italic */
			case HT_STRONG:		/* bold */
			case HT_B:			/* bold */
			case HT_SAMP:		/* fixed width */
			case HT_TT:			/* fixed width */
			case HT_VAR:		/* fixed width */
			case HT_CODE:		/* fixed width */
			case HT_KBD:		/* fixed width */
				if(temp->is_end)
				{
					if(html->html.allow_color_switching)
						fg = PopColor();
					font = PopFont(&basefont);
				}
				else
				{
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);
					PushFont(font, basefont);
					font = NextFont(html, temp->id, basefont);
				}
				ignore = True; /* only need font data */
				break;

			case HT_U:
				if(temp->is_end) /* unset underline bitfields */
					element_data &= (~ELE_UNDERLINE & ~ELE_UNDERLINE_TEXT);
				else
					element_data |= ELE_UNDERLINE;
				ignore = True; /* only need underline data */
				break;

			case HT_STRIKE:
				if(temp->is_end) /* unset strikeout bitfields */
					element_data &= (~ELE_STRIKEOUT & ~ELE_STRIKEOUT_TEXT);
				else
					element_data |= ELE_STRIKEOUT;
				ignore = True; /* only need strikeout data */
				break;

			case HT_BASEFONT:
				{
					basefont = _XmHTMLTagGetNumber(temp->attributes, "size", 0);
					/* take absolute value */
					basefont = Abs(basefont);
					if(basefont < 1 || basefont > 7)
					{
						if(html->html.bad_html_warnings)
							_XmHTMLWarning(__WFUNC__(html, func),
								"Invalid basefont size %i at line %i of "
								"input.", basefont, temp->line);
						basefont = 4;
					}
				}
				ignore = True;	/* only need font data */
				break;

			/*****
			* <font> is a bit performance hit. We always need to push & pop
			* the font *even* if only the font color has been changed as we
			* can't keep track of what has actually been changed.
			*****/
			case HT_FONT:
				if(temp->is_end)
				{
					if(html->html.allow_font_switching)
						font = PopFont(&basefont);
					if(html->html.allow_color_switching)
						fg = PopColor();
				}
				else
				{
					char *chPtr;
					int size = xmhtml_basefont_sizes[basefont - 1];

					if(html->html.allow_color_switching)
						PUSH_COLOR(html);

					if(html->html.allow_font_switching)
						PushFont(font, basefont);
					else
						break;

					/* can't use TagGetNumber: fontchange can be relative */
					chPtr = _XmHTMLTagGetValue(temp->attributes, "size");
					if(chPtr != NULL)
					{
						int f_inc = atoi(chPtr);

						/* check wether size is relative or not */
						if(chPtr[0] == '-' || chPtr[0] == '+')
						{
							f_inc = basefont + f_inc; /* + 1; */
						}
						/* sanity check */
						if(f_inc < 1 || f_inc > 7)
						{
							if(f_inc < 1)
								f_inc = 1;
							else
								f_inc = 7;
						}

						basefont = f_inc;
						/* minus one: zero based array */
						size = xmhtml_basefont_sizes[f_inc-1];
						free(chPtr); /* fix 01/28/98-02, kdh */
						chPtr = NULL;
					}
					/*****
					* Font face changes only allowed when not in preformatted
					* text.
					* Only check when not being pedantic.
					*****/
#ifndef PEDANTIC
					if(!in_pre)
#endif
						chPtr = _XmHTMLTagGetValue(temp->attributes, "face");

					if(chPtr != NULL)
					{
#ifdef PEDANTIC
						if(in_pre)
						{
							_XmHTMLWarning(__WFUNC__(html, func),
								"<FONT FACE=\"%s\"> not allowed inside <PRE>,"
								" ignored.\n    (line %i in input)", chPtr,
								temp->line);
							/*****
							* Ignore face but must allow for size change.
							* (Font stack will get unbalanced otherwise!)
							*****/
							font = NextFont(html, HT_FONT, size);
						}
						else
#endif
							font = NextFontWithFace(html, size, chPtr);

						free(chPtr);
					}
					else
						font = NextFont(html, HT_FONT, size);
				}
				ignore = True; /* only need font data */
				break;

			case HT_BIG:
				if(temp->is_end)
					font = PopFont(&basefont);
				else /* multiple big elements are not honoured */
				{
					PushFont(font, basefont);
					font = NextFont(html, HT_FONT, xmhtml_basefont_sizes[4]);
				}
				ignore = True; /* only need font data */
				break;

			case HT_SMALL:
				if(temp->is_end)
					font = PopFont(&basefont);
				else /* multiple small elements are not honoured */
				{
					PushFont(font, basefont);
					font = NextFont(html, HT_FONT, xmhtml_basefont_sizes[2]);
				}
				ignore = True; /* only need font data */
				break;

			case HT_SUB:
			case HT_SUP:
				if(temp->is_end)
				{
					/* restore vertical offset */
					y_offset = 0;
					x_offset = 0;
					font = PopFont(&basefont);
				}
				else /* multiple small elements are not honoured */
				{
					PushFont(font, basefont);
					font = NextFont(html, HT_FONT, xmhtml_basefont_sizes[2]);
					y_offset = (temp->id == HT_SUB ? 
						font->sub_yoffset : font->sup_yoffset);
					x_offset = (temp->id == HT_SUB ? 
						font->sub_xoffset : font->sup_xoffset);
				}
				ignore = True; /* only need font data */
				break;

			case HT_H1:
			case HT_H2:
			case HT_H3:
			case HT_H4:
			case HT_H5:
			case HT_H6:
				if(temp->is_end)
				{
					if(html->html.allow_color_switching)
						fg = PopColor();
					halign = PopAlignment();
					font = PopFont(&basefont);
				}
				else
				{
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);

					PushAlignment(halign);
					halign = _XmHTMLGetHorizontalAlignment(temp->attributes,
						halign);
					PushFont(font, basefont);
					font = NextFont(html, temp->id, basefont);
					/*
					* Need to update basefont size as well so font face changes
					* *inside* these elements use the correct font size as
					* well.
					* The sizes used by the headers are in reverse order.
					*/
					basefont = (int)(HT_H6 - temp->id) + 1;
				}
				linefeed = CheckLineFeed(CLEAR_HARD, False);
				object_type = OBJ_BLOCK;
				break;

			/* lists. The COMPACT tag is ignored */
			case HT_UL:
			case HT_DIR:
			case HT_MENU:
				if(temp->is_end)
				{

					ul_level--;
					ident_level--;
					if(ident_level < 0)
					{
						if(html->html.bad_html_warnings)
							_XmHTMLWarning(__WFUNC__(html, func),
								"Negative indentation at line %i of input. "
								"Check your document.", temp->line);
						ident_level = 0;
					}
					current_list = (ident_level ? ident_level - 1 : 0);
				}
				else
				{
					int mark_id;
					/* avoid overflow of mark id array */
					mark_id = ul_level % UL_ARRAYSIZE;

					/* set default marker & list start */
					list_stack[ident_level].marker = ul_markers[mark_id].type;
					list_stack[ident_level].level = 0;
					list_stack[ident_level].type = temp->id;

					if(ident_level == MAX_NESTED_LISTS)
					{
  						_XmHTMLWarning(__WFUNC__(html, func), "Exceeding"
							" maximum nested list depth for nested lists (%i) "
							"at line %i of input.", MAX_NESTED_LISTS,
							temp->line);
						ident_level = MAX_NESTED_LISTS-1;
					}
					current_list = ident_level;

					if(temp->id == HT_UL)
					{
						char *chPtr;
						/* check if user specified a custom marker */
						chPtr = _XmHTMLTagGetValue(temp->attributes, "type");
						if(chPtr != NULL)
						{
							/*
							* Walk thru the list of possible markers. If a 
							* match is found, store it so we can switch back
							* to the correct marker once this list terminates.
							*/
							for(i = 0 ; i < UL_ARRAYSIZE; i++)
							{
								if(!(strcasecmp(ul_markers[i].name, chPtr)))
								{
									list_stack[ident_level].marker = 
										ul_markers[i].type;
									break;
								}
							}
							free(chPtr);
						}
					}
					ul_level++;
					ident_level++;
				}
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_BLOCK;
				break;

			case HT_OL:
				if(temp->is_end)
				{
					/* must be reset properly, only possible for <ol> lists. */
					list_stack[current_list].isindex = False;

					ol_level--;
					ident_level--;
					if(ident_level < 0)
					{
						if(html->html.bad_html_warnings)
							_XmHTMLWarning(__WFUNC__(html, func),
								"Negative indentation at line %i of input. "
								"Check your document.", temp->line);
						ident_level = 0;
					}
					current_list = (ident_level ? ident_level - 1 : 0);
				}
				else
				{
					int mark_id;
					char *chPtr;

					/* avoid overflow of mark id array */
					mark_id = ol_level % OL_ARRAYSIZE;

					/* set default marker & list start */
					list_stack[ident_level].marker = ol_markers[mark_id].type;
					list_stack[ident_level].level = 0;
					list_stack[ident_level].type = temp->id;

					if(ident_level == MAX_NESTED_LISTS)
					{
  						_XmHTMLWarning(__WFUNC__(html, func), "Exceeding"
							" maximum nested list depth for nested lists (%i) "
							"at line %i of input.", MAX_NESTED_LISTS,
							temp->line);
						ident_level = MAX_NESTED_LISTS-1;
					}
					current_list = ident_level;

					/* check if user specified a custom marker */
					chPtr = _XmHTMLTagGetValue(temp->attributes, "type");
					if(chPtr != NULL)
					{
						/*
						* Walk thru the list of possible markers. If a 
						* match is found, store it so we can switch back
						* to the correct marker once this list terminates.
						*/
						for(i = 0 ; i < OL_ARRAYSIZE; i++)
						{
							if(!(strcmp(ol_markers[i].name, chPtr)))
							{
								list_stack[ident_level].marker = 
									ol_markers[i].type;
								break;
							}
						}
						free(chPtr);
					}

					/* see if a start tag exists */
					if(_XmHTMLTagCheck(temp->attributes, "start"))
					{
						/* pick up a start spec */
						list_stack[ident_level].level = 
							_XmHTMLTagGetNumber(temp->attributes, "start", 0);
						list_stack[ident_level].level--;
					}

					/* see if we have to propage the current index number */
					list_stack[ident_level].isindex =
						_XmHTMLTagCheck(temp->attributes, "isindex");
					ol_level++;
					ident_level++;
				}
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_BLOCK;
				break;

			case HT_DL:
				if(temp->is_end)
					ident_level--;
				else
					ident_level++;
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_BLOCK;
				break;

			case HT_LI:
				if(temp->is_end)	/* optional termination */
					object_type = OBJ_BLOCK;
				else
				{
					char *chPtr;

					/* increase list counter */
					list_stack[current_list].level++;

					/* check if user specified a custom marker */
					chPtr = _XmHTMLTagGetValue(temp->attributes, "type");
					if(chPtr != NULL)
					{
						/*
						* depending on current list type, check and set
						* the marker.
						*/
						if(list_stack[current_list].type == HT_OL)
						{
							for(i = 0 ; i < OL_ARRAYSIZE; i++)
							{
								if(!(strcmp(ol_markers[i].name, chPtr)))
								{
									list_stack[current_list].marker = 
										ol_markers[i].type;
									break;
								}
							}
						}
						else if(list_stack[current_list].type == HT_UL)
						{
							for(i = 0 ; i < UL_ARRAYSIZE; i++)
							{
								if(!(strcmp(ul_markers[i].name, chPtr)))
								{
									list_stack[current_list].marker = 
										ul_markers[i].type;
									break;
								}
							}
						}
						free(chPtr);
					}
					/* check if user specified a custom number for ol lists */
					if(list_stack[current_list].type == HT_OL && 
						_XmHTMLTagCheck(temp->attributes, "value"))
					{
						list_stack[current_list].level = 
							_XmHTMLTagGetNumber(temp->attributes, "value", 0);
					}
					/*
					* If the current list is an index, create a prefix for
					* the current item
					*/
					if(list_stack[current_list].isindex)
					{
						words = indexToWord(html, list_stack, current_list,
									element, in_pre);
						n_words = 1;
					}
					object_type = OBJ_BULLET;
				}
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				break;

			case HT_DT:
			case HT_DD:
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_BLOCK;
				break;

			/* block commands */
			case HT_ADDRESS:
				if(temp->is_end)
				{
					if(html->html.allow_color_switching)
						fg = PopColor();
					font = PopFont(&basefont);
				}
				else
				{
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);
					PushFont(font, basefont);
					font = NextFont(html, temp->id, basefont);
				}
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_BLOCK;
				break;

			case HT_BR:
#if 0
				{
					String chPtr;
					
					if((chPtr = _XmHTMLTagGetValue(temp->attributes,
						"clear")) != NULL && locase(chPtr[0]) != 'n')
					{
						/* all clear attribs but none reset the linefeeder */
						(void)CheckLineFeed(CLEAR_HARD, True);
						linefeed = CLEAR_ALL;

						if(locase(chPtr[0]) == 'l')	/* clear = left */
							halign = XmHALIGN_LEFT;
						else if(locase(chPtr[0]) == 'r')	/* clear = right */
							halign = XmHALIGN_RIGHT;
						/* no default */

						free(chPtr);
					}
					else /* fix 01/20/97-02, kdh */
						linefeed = CheckLineFeed(CLEAR_SOFT, False);
				}
#endif
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_BLOCK;
				break;

			case HT_TAB:
				{
					char *chPtr;

					object_type = OBJ_TEXT;

					element->len = 8; /* default tabsize */

					/* see if we have a width spec */
					chPtr = _XmHTMLTagGetValue(temp->attributes, "size");
					if(chPtr != NULL)
					{
						element->len = atoi(chPtr);
						free(chPtr);
					}
					n_words = 1;
					words = SetTab(element->len, &height, font, element);
				}
				break;
				
			case HT_PRE:
				if(temp->is_end)
				{
					if(html->html.allow_color_switching)
						fg = PopColor();
					font = PopFont(&basefont);
					in_pre = False;
				}
				else
				{
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);
					PushFont(font, basefont);
					font = NextFont(html, temp->id, basefont);
					in_pre = True;
				}
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_BLOCK;
				break;

			case HT_BLOCKQUOTE:
				if(temp->is_end)
				{
					ident_level--;
					if(html->html.allow_color_switching)
						fg = PopColor();
				}
				else
				{
					ident_level++;
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);
				}
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_BLOCK;
				break;

			case HT_P:
			case HT_DIV:
				if(temp->is_end)
				{
					halign = PopAlignment();
					/*
					* Paragraph ending also adds linespacing (natural flow
					* of text between paragraphs).
					*/
					linefeed = CheckLineFeed(
						(temp->id == HT_P ? CLEAR_HARD : CLEAR_SOFT), False);
					/* do we have a color attrib? */
					if(html->html.allow_color_switching)
						fg = PopColor();
				}
				else
				{
					PushAlignment(halign);
					halign = _XmHTMLGetHorizontalAlignment(temp->attributes,
						halign);
					linefeed = CheckLineFeed(
						(temp->id == HT_P ? CLEAR_HARD : CLEAR_SOFT), False);
					/* do we have a color attrib? */
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);
				}
				object_type = OBJ_BLOCK;
				break;

			case HT_CENTER:
				if(temp->is_end)
				{
					halign = PopAlignment();
					/* do we have a color attrib? */
					if(html->html.allow_color_switching)
						fg = PopColor();
				}
				else
				{
					PushAlignment(halign);
					halign = XmHALIGN_CENTER;
					/* do we have a color attrib? */
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);
				}
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_BLOCK;
				break;

			case HT_HR:
				{
					char *chPtr;

					/* 
					* horizontal rules don't have an ending counterpart,
					* so the alignment is never pushed. If we should do that, 
					* we would get an unbalanced stack.
					*/
					element->halign =
						_XmHTMLGetHorizontalAlignment(temp->attributes,
							halign);
					/* see if we have a width spec */
					chPtr = _XmHTMLTagGetValue(temp->attributes, "width");
					if(chPtr)
					{
						/* when len is negative, a percentage has been used */
						if((strpbrk(chPtr, "%")) != NULL)
							element->len = -1*atoi(chPtr);
						else
							element->len = atoi(chPtr);
						free(chPtr);
					}
					/* check height */
					height = _XmHTMLTagGetNumber(temp->attributes, "size", 0);
					/* sanity check */
					if(height <= 0 )
						height = 2;
					/* y_offset is used as a flag for the NOSHADE attr. */
					element->y_offset = (int)_XmHTMLTagCheck(temp->attributes, 
						"noshade");

					/* do we have a color attrib? */
					if(html->html.allow_color_switching &&
						!html->html.strict_checking)
						PUSH_COLOR(html);
				}
				/* horizontal rules always have a soft return */
				linefeed = CheckLineFeed(CLEAR_SOFT, False);
				object_type = OBJ_HRULE;
				break;

			/* forms */
			case HT_FORM:
				if(temp->is_end)
					_XmHTMLEndForm(html);
				else
					_XmHTMLStartForm(html, temp->attributes);

				/* only need form data */
				ignore = True;
				break;

			case HT_SELECT:
				/* this form component can only contain option tags */
				if((words = SelectToWord(html, temp, &n_words, &width, &height,
					element, in_pre)) == NULL)
				{
					ignore = True;
					break;
				}
				/* walk to the end of this select */
				temp = temp->next;
				for(; temp != NULL && temp->id != HT_SELECT;
					temp = temp->next);

				text_data |= TEXT_FORM;
				object_type = in_pre ? OBJ_PRE_TEXT : OBJ_TEXT;
				break;

			/*****
			* It's an error if we get this, SelectToWord deals with these
			* tags.
			*****/
			case HT_OPTION:
				if(!temp->is_end)
				{
					if(html->html.bad_html_warnings)
						_XmHTMLWarning(__WFUNC__(html, func), 
							"Bad <OPTION> tag: outside a <SELECT> tag, "
							"ignoring (line %i in input).", temp->line);
				}
				ignore = True;
				break;

			case HT_TEXTAREA:
				if((words = TextAreaToWord(html, temp, &n_words, &width,
					&height, element, in_pre)) == NULL)
				{
					ignore = True;
					break;
				}
				/*
				* Walk to the end of this textarea. If there was any text
				* provided, we've already picked it up.
				*/
				temp = temp->next;
				for(; temp != NULL && temp->id != HT_TEXTAREA;
					temp = temp->next);

				text_data |= TEXT_FORM;
				object_type = in_pre ? OBJ_PRE_TEXT : OBJ_TEXT;
				break;

			case HT_INPUT:
				if((words = InputToWord(html, temp->attributes, &n_words,
					&width, &height, element, in_pre)) == NULL)
				{
					ignore = True;
					break;
				}
				/* type=image is promoted to a true image */
				if(words->form->type == FORM_IMAGE)
				{
					text_data |= TEXT_IMAGE;

					/* allocate a new anchor */
					if((form_anchor_data = _XmHTMLNewAnchor(html, temp))
						== NULL)
						break;

					/* promote to internal form anchor */
					form_anchor_data->url_type = ANCHOR_FORM_IMAGE;

					new_anchors++;

					/* set proper element bits, we assume it's a plain one */
					element_data |= ELE_ANCHOR;
				}
				else
				{
					text_data |= TEXT_FORM;
				}
				object_type = in_pre ? OBJ_PRE_TEXT : OBJ_TEXT;
				break;

			/* applets */
			case HT_APPLET:
				if(temp->is_end)
				{
					/*
					* INSERT CODE
					* to end this applet
					*/
				}
				else
				{
					if(html->html.bad_html_warnings)
						_XmHTMLWarning(__WFUNC__(html, func), 
							"<APPLET> element not supported yet.");
					/*
					* INSERT CODE
					* to start this applet
					*/
				}
				object_type = OBJ_APPLET;
				ignore = True;
				break;

			case HT_PARAM:		/* applet params */
				if(html->html.bad_html_warnings)
					_XmHTMLWarning(__WFUNC__(html, func), 
						"<PARAM> element not supported yet.");
				object_type = OBJ_APPLET;
				ignore = True;
				break;

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
					else if(html->html.bad_html_warnings)
						_XmHTMLWarning(__WFUNC__(html, func), "unnamed "
							"map, ignored (line %i in input).", temp->line);
				}
				ignore = True;	/* only need imagemap name */
				break;

			case HT_AREA:
				if(imageMap)
					_XmHTMLAddAreaToMap(html, imageMap, temp);
				else if(html->html.bad_html_warnings)
					_XmHTMLWarning(__WFUNC__(html, func), "<AREA> "
						"element outside <MAP>, ignored (line %i in input).",
						temp->line);
				ignore = True;	/* only need area data */
				break;

			/* tables */
			case HT_TABLE:
				if(temp->is_end)
				{
					linefeed = CheckLineFeed(CLEAR_SOFT, True);
					halign = PopAlignment();
				}
				else
				{
					linefeed = CheckLineFeed(CLEAR_SOFT, False);
					PushAlignment(halign);

					/* 
					* table alignment has limited scope within the table 
					* itself. 
					*/
					halign = _XmHTMLGetHorizontalAlignment(temp->attributes,
						html->html.default_halign);
					if(html->html.bad_html_warnings)
						_XmHTMLWarning(__WFUNC__(html, func), 
							"<TABLE> element not supported yet.");
				}
				ignore = True;
				object_type = OBJ_TABLE;
				break;

			case HT_CAPTION:		/* table caption */
				if(temp->is_end)
				{
					font = PopFont(&basefont);
					halign = PopAlignment();
				}
				else
				{
					linefeed = CheckLineFeed(CLEAR_SOFT, False);
					PushAlignment(halign);
					halign = _XmHTMLGetHorizontalAlignment(temp->attributes,
						halign);
					PushFont(font, basefont);
					font = NextFont(html, temp->id, basefont);
				}
				linefeed = CheckLineFeed(CLEAR_HARD, False);
				object_type = OBJ_TABLE;
				break;

			case HT_TR:		/* table row */
				if(temp->is_end)	/* optional termination */
				{
					if(html->html.allow_color_switching)
						fg = PopColor();
#if 0
					halign = PopAlignment();
#endif
					valign = XmVALIGN_NONE;
				}
				else
				{
#if 0
					PushAlignment(halign);
					halign = _XmHTMLGetHorizontalAlignment(temp->attributes,
						halign);
#endif
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);
					valign = _XmHTMLGetVerticalAlignment(temp->attributes);
					linefeed = CheckLineFeed(CLEAR_SOFT, False);
				}
				object_type = OBJ_TABLE;
				break;

			case HT_TH:		/* table header */
				if(temp->is_end)	/* optional termination */
				{
					font = PopFont(&basefont);
					if(html->html.allow_color_switching)
						fg = PopColor();
					valign = XmVALIGN_NONE;
#if 0
					halign = PopAlignment();
#endif
				}
				else
				{
					/* header cell uses a bold font */
					PushFont(font, basefont);
					font = NextFont(html, HT_B, basefont);
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);
#if 0
					PushAlignment(halign);
					halign = _XmHTMLGetHorizontalAlignment(temp->attributes,
						halign);
#endif
					valign = _XmHTMLGetVerticalAlignment(temp->attributes);
					linefeed = CheckLineFeed(CLEAR_SOFT, False);
				}
				object_type = OBJ_TABLE;
				break;

			case HT_TD:			/* table cell */
				if(temp->is_end)	/* optional termination */
				{
#if 0
					PopAlignment();
					valign = XmVALIGN_NONE;
#endif
					if(html->html.allow_color_switching)
						fg = PopColor();
				}
				else
				{
#if 0
					PushAlignment(halign);
					halign = _XmHTMLGetHorizontalAlignment(temp->attributes,
						halign);
#endif
					if(html->html.allow_color_switching)
						PUSH_COLOR(html);
					valign = _XmHTMLGetVerticalAlignment(temp->attributes);
					linefeed = CheckLineFeed(CLEAR_SOFT, True);
				}
				object_type = OBJ_TABLE;
				break;

			/*****
			* According to HTML3.2, the following elements may not occur
			* inside the body content, but a *lot* of HTML documents are
			* in direct violation with this and the parser isn't always
			* successfully in removing them. So we need to handle these
			* elements as well and skip all data between the opening and
			* closing element.
			*****/
			case HT_STYLE:
			case HT_SCRIPT:
				{
					htmlEnum end_id = temp->id;
					/* move past element */
					temp = temp->next;
					/* skip it entirely */
					for(; temp != NULL; temp = temp->next)
						if(temp->id == end_id && temp->is_end)
							break;
					ignore = True;
				}
				break;

			default:
				_XmHTMLDebug(2, ("format.c: _XmHTMLformatObjects; "
					"Unused element %s.\n", temp->element));
				ignore = True;
		}
		if(!ignore)
		{
			/* adjust anchor count */
			if(element_data & ELE_ANCHOR)
			{
				text_data |= TEXT_ANCHOR;
				anchor_words += n_words; 
				anchor_data_used = True;
			}
			/* mark object as internal anchor */
			if(element_data & ELE_ANCHOR_INTERN)
			{
				text_data |= TEXT_ANCHOR_INTERN;
				named_anchors++;
				anchor_data_used = True;
			}
			element->text = text;
			element->text_data = text_data;
			element->words = words;
			element->n_words = n_words;
			element->width = width;
			element->height = height;
			element->fg = fg;
			element->bg = bg;
			element->font = font;
			element->marker = list_stack[current_list].marker;
			element->list_level = list_stack[current_list].level;
			/* 
			* <dt> elements have an identation one less than the current.
			* all identation must use the default font (consistency).
			*/
			if(temp->id == HT_DT && ident_level)
				element->ident = (ident_level-1) * IDENT_SPACES * 
					Toolkit_XFont(html->html.default_font->xfont)->max_bounds.width;
			else
				element->ident = ident_level * IDENT_SPACES * 
					Toolkit_XFont(html->html.default_font->xfont)->max_bounds.width;

			element->linefeed = (int)((1+linefeed)*font->lineheight);

			/* stupid hack so HT_HR won't mess up alignment and color stack */
			if(temp->id != HT_HR)
			{
				element->halign = halign;
				element->y_offset = y_offset;
				element->x_offset = x_offset;
			}
			else if(html->html.allow_color_switching &&
						!html->html.strict_checking)
				fg = PopColor();

			element->object_type = object_type;

			/*****
			* If we have a form component of type <input type="image">, we
			* have promoted it to an anchor. Set this anchor data as the
			* anchor for this element and, as it is used only once, reset
			* it to NULL. In all other case we have a plain anchor.
			*
			* Note: as form components are allowed inside anchors, this is
			* the only place in which we can possibly have nested anchors.
			* This is a problem we will have to live with...
			*****/
			if(form_anchor_data)
			{
				element->anchor = form_anchor_data;
				form_anchor_data = NULL;
				element_data &= ~ELE_ANCHOR;
			}
			else
				element->anchor = anchor_data;

			/* add an anchor id if this data belongs to a named anchor */
			if(element_data & ELE_ANCHOR_INTERN) 
				element->id = named_anchors;

			InsertTableElement(html, element, element_data & ELE_ANCHOR);
			previous_element = element;
		}
		/* Clean up if this new element should be ignored. */
		else 
		{
			free(element);	/* fix 01/28/97-05, kdh */
#ifdef DEBUG
			num_ignore++;
#endif
		}

		/* move to next element */
		temp = temp->next;
	}
	/*
	* Some sucker forget to terminate a list and parser failed to repair it.
	* Spit out a warning.
	*/
	if(html->html.bad_html_warnings && ident_level != 0)
		_XmHTMLWarning(__WFUNC__(html, func), "non-zero indentation at "
			"end of input. Check your document.");

	/* clear colorstack */
	if(color_stack->next)
	{
		int i=0;
		while(color_stack->next)
		{
			(void)PopColor();
			i++;
		}
		_XmHTMLDebug(2, ("unbalanced color stack (%i colors remain).\n", i));
	}

	/* clear alignment stack */
	if(align_stack->next)
	{
		int i = 0;
		while(align_stack->next)
		{
			(void)PopAlignment();
			i++;
		}
		_XmHTMLDebug(2, ("unbalanced alignment stack (%i alignments "
			"remain).\n", i));
	}

	/* clear font stack */
	if(font_stack->next)
	{
		int i = 0;
		while(font_stack->next)
		{
			(void)PopFont(&basefont);
			i++;
		}
		_XmHTMLDebug(2, ("unbalanced font stack (%i fonts remain).\n", i));
	}

	/* 
	* allocate memory for all anchor words in this document, gets filled in
	* paint.c
	*/
	if(anchor_words)
	{
		html->html.anchors= (XmHTMLWord*)calloc(anchor_words+1,
			sizeof(XmHTMLWord));

		_XmHTMLDebug(2,("_XmHTMLFormatObjects: anchors contain %i words\n",
			anchor_words));
		html->html.anchor_words = anchor_words;
	}
	html->html.num_anchors = list_data.num_anchors;

	/* allocated memory for all named anchors. Gets filled in paint.c */
	if(named_anchors)
	{
		html->html.named_anchors = 
			(XmHTMLObjectTable*)calloc(named_anchors+1, 
				sizeof(XmHTMLObjectTable));

		html->html.num_named_anchors = named_anchors;
	}

	_XmHTMLDebug(2,("_XmHTMLFormatObjects: formatted %li elements of which %li"
		" anchors.\n", list_data.num_elements, list_data.num_anchors));
	_XmHTMLDebug(2, ("_XmHTMLformatObjects, allocated %i elements of which "
		"%i have been ignored.\n", allocated, num_ignore));

	_XmHTMLDebug(2, ("_XmHTMLformatObjects, allocated %i XmHTMLAnchor "
		"objects\n", new_anchors));

	/* store the anchor list */
	html->html.anchor_data = list_data.anchor_head;

	/* Since the table head is a dummy element, we return the next one */
	list_data.current = list_data.head->next;

	/* this is *required* */
	if(list_data.current)
		list_data.current->prev = NULL;

	/* free top of the list */
	free(list_data.head);
	list_data.head = NULL;

#ifdef DEBUG
	dumpFontCacheStats();
#endif

	return(list_data.current);
}
