#ifndef lint
static char rcsId[]="$Header$";
#endif
/*****
* parse.c : XmHTML HTML parser
*
* This file Version	$Revision$
*
* Creation date:		Wed Nov 13 00:33:27 GMT+0100 1996
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
* Revision 1.18  1997/10/23 00:25:08  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.17  1997/08/31 17:37:26  newt
* removed HT_TEXTFLOW
*
* Revision 1.16  1997/08/30 01:22:19  newt
* _XmHTMLWarning proto changes.
* Fixed parser to remove out of order style and script elements.
* Fixed quote detection, unbalanced quotes inside tags are now properly
* recognized.
* Fixed the main parser so SetValues can be called from within the
* XmNdocumentCallback.
* Removed all progressive parsing stuff, it was unused and didn't work
* properly either.
*
* Revision 1.15  1997/08/01 13:07:07  newt
* Reduced data storage. Minor bugfixes in HTML rules. Added state stack
* backtracking and updated comments (again...).
*
* Revision 1.14  1997/05/28 01:53:43  newt
* Bugfixes in comment parsing. Modified the parser to properly deal with the
* contents of the <SCRIPT> and <STYLE> head attributes.
*
* Revision 1.13  1997/04/29 14:30:48  newt
* Removed ParserCallback stuff.
*
* Revision 1.12  1997/04/03 05:40:54  newt
* #ifdef PEDANTIC/#endif changes
*
* Revision 1.11  1997/03/28 07:23:01  newt
* More changes in document verification/repair.
* Implemented parserCallback stuff.
* Frameset support added.
* XmNmimeType changes: split _XmHTMLparseHTML into parseHTML, parsePLAIN
* and parseIMAGE.
*
* Revision 1.10  1997/03/20 08:13:16  newt
* Major changes: almost a full rewrite and integrated document verification
* and repair.
*
* Revision 1.9  1997/03/11 19:58:06  newt
* added a third argument to _XmHTMLTagGetNumber: default return value. 
* Added _XmHTMLGetImageAlignment
*
* Revision 1.8  1997/03/04 18:49:04  newt
* ?
*
* Revision 1.7  1997/03/04 01:01:53  newt
* CheckTermination: changed <p> handling
*
* Revision 1.6  1997/03/02 23:22:48  newt
* Sneaky bugfix in expandEscapes (Dick Porter, dick@cymru.net); Sanity check 
* in storeTextElement changed to check if len <= 0 instead of <= 1
*
* Revision 1.5  1997/02/11 02:10:13  newt
* Added support for SGML shorttags. Re-ordered all switch statements
*
* Revision 1.4  1997/02/04 02:53:18  newt
* state checking now checks for overlapping and missing closing elements. 
* Added the basefont element.
*
* Revision 1.3  1997/01/09 06:55:50  newt
* expanded copyright marker
*
* Revision 1.2  1997/01/09 06:47:00  newt
* a few bugfixes in XmHTMLTagCheck and XmHTMLTagGetValue
*
* Revision 1.1  1996/12/19 02:17:13  newt
* Initial Revision
*
*****/ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef VERIFY
# include "verify.h"
#else
# ifdef DMALLOC
#  include <dmalloc.h>
# endif /* DMALLOC */

# include "XmHTMLP.h"
# include "escapes.h"
# include "XmHTMLfuncs.h"
#endif /* VERIFY */

/*** External Function Prototype Declarations ***/

/*** Public Variable Declarations ***/
/***** 
* HTML Elements names.
* This list is alphabetically sorted to speed up the searching process.
* DO NOT MODIFY
*****/
String html_tokens[] =
{"!doctype", "a", "address", "applet", "area", "b", "base", "basefont", "big", 
"blockquote", "body", "br", "caption", "center", "cite", "code", "dd", "dfn", 
"dir", "div", "dl", "dt", "em", "font", "form", "frame", "frameset", "h1",
"h2", "h3", "h4", "h5", "h6", "head", "hr", "html", "i", "img", "input",
"isindex", "kbd", "li", "link", "map", "menu", "meta", "noframes", "ol",
"option", "p", "param", "pre", "samp", "script", "select", "small", "strike",
"strong", "style", "sub", "sup", "tab", "table", "td", "textarea", "th",
"title", "tr", "tt", "u", "ul", "var", "plain text"};

/*** Private Datatype Declarations ****/
#define POOL_SIZE		512		/* alloc this many objects at once */
static struct{
	int num_elements;
	int num_text;
	XmHTMLObject *head;
	XmHTMLObject *current;
	XmHTMLObject *obj_pool;		/* pool of objects */
	XmHTMLObject *curr_obj;		/* current position in pool */
	int pool_total;				/* total size of pool */
	int pool_avail;				/* available objects left */
}list_data;

/*** Private Function Prototype Declarations ****/
/* convert a string to an internal id */
static htmlEnum tokenToId(String token, Boolean warn);

/* convert a character escape sequence */
static char tokenToEscape(char **escape, Boolean warn);

/* check if the given element has a terminating counterpart */
static Boolean getTerminatorState(htmlEnum id);

/* store a text element */
static void storeTextElement(char *start, char *end);

/* store a text element and reverse it's contents */
#ifndef VERIFY
static void storeTextElementRtoL(char *start, char *end);
#endif

/* store a real element */
static String storeElement(char *start, char *end);

/* store a real element. Skips verification */
#ifndef VERIFY
static String storeElementUnconditional(char *start, char *end);
#endif

/* create a new element */
static XmHTMLObject *newElement(htmlEnum id, char *element, char *attributes,
	Boolean is_end, Boolean terminated);

/* insert a new element */
static void insertElement(String element, htmlEnum new_id, Boolean is_end);

/* terminate the given element. Backtracks both stack and element list */
static void terminateElement(String element, htmlEnum current);

/* copy an element */
static void copyElement(XmHTMLObject *src, Boolean is_end);

/* verify (and possibly correct) the presence of element id */
static int verifyElement(htmlEnum id, Boolean is_end);

/* verify whether the parser successfully verified/repaired a document */
static XmHTMLObject *verifyVerification(XmHTMLObject *objects);

/* check whether the given element may appear inside the <BODY> tag */
static Boolean isBodyElement(htmlEnum id);

/* check whether presence of current in state is allowed */
static int checkOccurance(htmlEnum current, htmlEnum state);

/* check whether current is allowed to appear in state */
static Boolean checkContent(htmlEnum current, htmlEnum state);

/* push a parser state on the stack */
static void pushState(htmlEnum id, int line);

/* pop a parser state from the stack */
static htmlEnum popState(int line);

/* see if id is present in the stack */
static Boolean onStack(htmlEnum id);

/* parserCallback driver */
static void parserWarning(htmlEnum id, htmlEnum current, parserError error);

#define parserCallback(ID,CURR,ERR) \
	do { if(bad_html_warnings) parserWarning(ID, CURR, ERR); break; } while(0)

/* main HTML parser driver */
static XmHTMLObject *parserDriver(XmHTMLWidget html, XmHTMLObject *old_list,
	String input);

/* HTML parser */
static void parseHTML(XmHTMLWidget html, char *text);

/* Fast HTML parser for HTML3.2 conformant documents */
#ifndef VERIFY
static void parsePerfectHTML(XmHTMLWidget html, char *text);

/* plain text parser */
static void parsePLAIN(XmHTMLWidget html, char *text);

/* image parser */
static void parseIMAGE(XmHTMLWidget html, char *image_file);
#endif

#if defined(DEBUG) || defined(VERIFY)
static void writeParsedOutputToFile(XmHTMLObject *objects, String prefix);
#endif

/* elements for which a closing counterpart is optional */
#define OPTIONAL_CLOSURE(id) ((id) == HT_DD || (id) == HT_DT || \
	(id) == HT_LI || (id) == HT_P || (id) == HT_OPTION || (id) == HT_TD || \
	(id) == HT_TH || (id) == HT_TR)

/* physical/logical markup elements */
#define IS_MARKUP(id) ((id) == HT_TT || (id) == HT_I || (id) == HT_B || \
	(id) == HT_U || (id) == HT_STRIKE || (id) == HT_BIG || (id) == HT_SMALL || \
	(id) == HT_SUB || (id) == HT_SUP || (id) == HT_EM || (id) == HT_STRONG || \
	(id) == HT_DFN || (id) == HT_CODE || (id) == HT_SAMP || (id) == HT_KBD || \
	(id) == HT_VAR || (id) == HT_CITE || (id) == HT_FONT)

#define IS_MISC(id) ((id) == HT_P || (id) == HT_H1 || (id) == HT_H2 || \
	(id) == HT_H3 || (id) == HT_H4 || (id) == HT_H5 || (id) == HT_H6 || \
	(id) == HT_PRE || (id) == HT_ADDRESS || (id) == HT_APPLET || \
	(id) == HT_CAPTION || (id) == HT_A || (id) == HT_DT)

/* text containers */
#define IS_CONTAINER(id) ((id) == HT_BODY || (id) == HT_DIV || \
	(id) == HT_CENTER || (id) == HT_BLOCKQUOTE || (id) == HT_FORM || \
	(id) == HT_TH || (id) == HT_TD || (id) == HT_DD || (id) == HT_LI || \
	(id) == HT_NOFRAMES)

/* all elements that may be nested */
#define NESTED_ELEMENT(id) (IS_MARKUP(id) || (id) == HT_APPLET || \
	(id) == HT_BLOCKQUOTE || (id) == HT_DIV || (id) == HT_CENTER || \
	(id) == HT_FRAMESET)


/*** Private Variable Declarations ***/
static int num_lines;
static Dimension max_line_len;	/* maximum line length in entire document */
static stateStack state_base, *state_stack;
static TWidget widget;				/* for the warning messages */
static XmHTMLWidget html_widget;	/* for parserCallback */
static int current_start_pos;		/* for parserCallback */
static int current_end_pos;			/* for parserCallback */
static int err_count;				/* for parserCallback */
static String text_input;			/* for parserCallback */
static Boolean strict_checking;		/* HTML 3.2 looseness flag */
static Boolean have_body;			/* indicates presence of <body> tag */
static Boolean bad_html_warnings;	/* warn about bad html constructs */
static Boolean bad_html;			/* bad HTML document flag */
static Boolean html32;				/* HTML32 conforming document flag */

#if defined(DEBUG) || defined(VERIFY)
static int num_lookups, num_searches, p_inserted, p_removed, id_depth;
static Boolean notext;
#endif

#define RESET_STATESTACK { \
	state_stack = &state_base; \
	state_stack->id = HT_DOCTYPE; \
	state_stack->next = NULL; \
}

/*****
* Name: 		tokenToId
* Return Type: 	int
* Description: 	converts the html token passed to an internal id.
* In: 
*	token:		token for which to fetch an internal id.
*	warn:		if true, spits out a warning for unknown tokens.
* Returns:
*	The internal id upon success, -1 upon failure
*
* Note: this routine uses a binary search into an array of all possible
*	HTML 3.2 tokens. It is very important that _BOTH_ the array 
*	html_tokens _AND_ the enumeration htmlEnum are *NEVER* changed. 
*	Both arrays are alphabetically sorted. Modifying any of these two 
*	arrays will	have VERY SERIOUS CONSEQUENCES, te return value of this
*	function matches a corresponding htmlEnum value.
*	As the table currently contains about 70 elements, a match will always
*	be found in at most 7 iterations (2^7 = 128)
*****/
static htmlEnum
tokenToId(String token, Boolean warn)
{
	register int mid, lo = 0, hi = HT_ZTEXT-1;
	int cmp;

#ifdef DEBUG
	num_lookups++;
#endif

	while(lo <= hi)
	{
#ifdef DEBUG
		num_searches++;
#endif
		mid = (lo + hi)/2;
		if((cmp = strcmp(token, html_tokens[mid])) == 0)
			return(mid);

		else
			if(cmp < 0)				/* in lower end of array */
				hi = mid - 1;
			else					/* in higher end of array */
				lo = mid + 1;
	}

	/* 
	* Not found, invalid token passed 
	* We don't want always have warnings. When XmNhandleShortTags is set to 
	* True, this routine is used to check whether we a / is right behind a 
	* token or not.
	*/
	if(warn)
		_XmHTMLWarning(__WFUNC__(widget, "tokenToId"),
			"Unknown element %s at line %i of input", token, num_lines);
	return(-1);
}

/*****
* Name: 		tokenToEscape
* Return Type: 	char
* Description: 	converts the HTML & escapes sequences to the appropriate char.
* In: 
*	**escape:	escape sequence to convert. This argument is updated upon
*				return.
*	warn:		warning issue flag;
* Returns:
*	the character representation of the given escape sequence
*
* Note: this routine uses a sorted table defined in the header file escapes.h
*	and uses a binary search to locate the appropriate character for the given
*	escape sequence.
*	This table contains the hashed escapes as well as the named escapes.
*	The number of elements is NUM_ESCAPES (currently 197), so a match is always
*	found in less than 8 iterations (2^8=256).
*	If an escape sequence is not matched and it is a hash escape, the value
*	is assumed to be below 160 and converted to a char using the ASCII 
*	representation of the given number. For other, non-matched characters, 0
*	is returned and the return pointer is updated to point right after the
*	ampersand sign.
*****/
static char 
tokenToEscape(char **escape, Boolean warn)
{
	register int mid, lo = 0, hi = NUM_ESCAPES -1;
	int cmp, skip = 1;
	char tmp[8];

	/*
	* first check if this is indeed an escape sequence.
	* It's much more cost-effective to do this test here instead of in
	* the calling routine.
	*/
	if(*(*escape+1) != '#' && !(isalpha(*(*escape+1))))
	{
		if(warn)
		{
			/* bad escape, spit out a warning and continue */
			strncpy(tmp, *escape, 7);
			tmp[7] = '\0';
			_XmHTMLWarning(__WFUNC__(NULL, "tokenToEscape"),
				"Invalid escape sequence: %s...", tmp);
		}
		/* skip and return */
		*escape += 1;
		return('&');
	}
	/*
	* run this loop twice: one time with a ; assumed present and one
	* time with ; present.
	*/
	for(skip = 0; skip != 2; skip++)
	{
		lo = 0;
		hi = NUM_ESCAPES - 1;
		while(lo <= hi)
		{
			mid = (lo + hi)/2;
			if((cmp = strncmp(*escape+1, escapes[mid].escape, 
				escapes[mid].len - skip)) == 0)
			{
				/* update ptr to point right after the escape sequence */
				*escape += escapes[mid].len + (1 - skip);
				return(escapes[mid].token);
			}
			else
				if(cmp < 0)				/* in lower end of array */
					hi = mid - 1;
				else					/* in higher end of array */
					lo = mid + 1;
		}
	}

	/*
	* If we get here, the escape sequence wasn't matched: big chance
	* it uses a &# escape below 160. To deal with this, we pick up the numeric
	* code and convert to a plain ASCII value which is returned to the
	* caller
	*/
	if( *(*escape+1) == '#')
	{
		char *chPtr, ret_char;
		int len = 0;

		*escape += 2;	/* skip past the &# sequence */
		chPtr = *escape;
		while(isdigit(*chPtr))
		{
			chPtr++;
			len++;
		}
		if(*chPtr == ';')
		{
			*chPtr = '\0';	/* null out the ; */
			len++;
		}
		ret_char = (char)atoi(*escape);	/* get corresponding char */
		/* move past the escape sequence */
		if(*(*escape + len) == ';')
			*escape += len + 1;
		else
			*escape += len;
		return(ret_char);
	}

	/* bad escape, spit out a warning and continue */
	if(warn)
	{
		strncpy(tmp, *escape, 7);
		tmp[7] = '\0';
		_XmHTMLWarning(__WFUNC__(NULL, "tokenToEscape"),
			"Invalid escape sequence %s...", tmp);
	}
	*escape += 1;
	return('&');
}

/*****
* Name: 		pushState
* Return Type: 	void
* Description: 	pushes the given id on the state stack
* In: 
*	id:			element id to push
*	line:		current document line number.
* Returns:
*	nothing.
*****/
static void
pushState(htmlEnum id, int line)
{
	stateStack *tmp;

	tmp = (stateStack*)malloc(sizeof(stateStack));
	tmp->id = id;
	tmp->next = state_stack;
	state_stack = tmp;
#ifdef DEBUG
	id_depth++;
	{
		int i;
		_XmHTMLDebug(4, ("%i: ", id_depth));
		if(id_depth < 10)
			for(i = 0; i < id_depth; i++)
			{
				_XmHTMLDebug(4, ("\t"));
			}
		else
		{
			_XmHTMLDebug(4, ("\t\t\t\t\t...\t\t\t\t"));
		}
		_XmHTMLDebug(4, ("parse.c: pushed %s (line %i)\n", html_tokens[id],
			line));
	}
#endif
}

/*****
* Name: 		popState
* Return Type: 	htmlEnum
* Description: 	pops an element of the state stack
* In: 
*	line:		current document line number.
* Returns:
*	id of element popped.
*****/
static htmlEnum
popState(int line)
{
	htmlEnum id;
	stateStack *tmp;

	if(state_stack->next != NULL)
	{
		tmp = state_stack;
		state_stack = state_stack->next;
		id = tmp->id;
		free((char*)tmp);
	}
	else
		id = state_stack->id;
#ifdef DEBUG
	id_depth--;
	{
		int i;
		_XmHTMLDebug(4, ("%i: ", id_depth+1));
		if(id_depth < 9)
			for(i = 0; i < id_depth+1; i++)
			{
				_XmHTMLDebug(4, ("\t"));
			}
		else
		{
			_XmHTMLDebug(4, ("\t\t\t\t\t...\t\t\t\t"));
		}
		_XmHTMLDebug(4, ("parse.c: popped %s (line %i)\n", html_tokens[id],
			line));
	}
#endif
	return(id);
}

/*****
* Name: 		onStack
* Return Type: 	Boolean
* Description: 	checks whether the given id is somewhere on the current
*				state stack.
* In: 
*	id:			element id to check.
* Returns:
*	True when present, False if not.
*****/
static Boolean
onStack(htmlEnum id)
{
	stateStack *tmp = state_stack;

	while(tmp->next != NULL && tmp->id != id)
		tmp = tmp->next;
	return(tmp->id == id);
}

/*****
* Name: 		getTerminatorState
* Return Type: 	Boolean
* Description: 	checks if the given element has a terminating counterpart
* In: 
*	id:			element to check
* Returns:
*	True when the given element is terminated, false if not.
*****/
static Boolean
getTerminatorState(htmlEnum id)
{
	switch(id)
	{
		/* Elements that are never terminated */
		case HT_AREA:
		case HT_BASE:
		case HT_BASEFONT:
		case HT_BR:
		case HT_DOCTYPE:
		case HT_FRAME:
		case HT_HR:
		case HT_IMG:
		case HT_INPUT:
		case HT_ISINDEX:
		case HT_LINK:
		case HT_META:
		case HT_STYLE:
		case HT_TAB:
		case HT_ZTEXT:
			return(False);

		/* all other elements are terminated */
		default:
			return(True);
	}
	return(False);	/* not reached */
}

/*****
* Name: 		isBodyElement
* Return Type: 	Boolean
* Description: 	checks whether the given id is allowed to appear inside the
*				<BODY> tag.
* In: 
*	id:			id to check.
* Returns:
*	True when allowed, False if not.
*****/
static Boolean
isBodyElement(htmlEnum id)
{
	switch(id)
	{
		/* all but these belong inside a <body></body> tag */
		case HT_DOCTYPE:
		case HT_BASE:
		case HT_HTML:
		case HT_HEAD:
		case HT_LINK:
		case HT_META:
		case HT_STYLE:
		case HT_TITLE:
		case HT_ZTEXT:
		case HT_FRAMESET:
		case HT_FRAME:
		case HT_SCRIPT:
			return(False);
		default:
			return(True);
	}
	return(False);	/* not reached */
}

/*****
* Name: 		insertElement
* Return Type: 	void
* Description: 	creates and inserts a new element in the parser tree
* In: 
*	element:	element name
*	new_id:		id of element to insert
*	is_end:		False when the new element should open, True when it should
*				close.
* Returns:
*	nothing.
*****/
static void
insertElement(String element, htmlEnum new_id, Boolean is_end)
{
	XmHTMLObject *extra;
	String tmp;

	/* need to do this, _XmHTMLFreeObjects will free this */
	tmp = strdup(element);

	/* allocate a element */
	extra = newElement(new_id, tmp, NULL, is_end, True);
#ifdef VERIFY
	extra->auto_insert = True;
#endif

	/* insert this element in the list */
	list_data.num_elements++;
	extra->prev = list_data.current;
	list_data.current->next = extra;
	list_data.current = extra;

	_XmHTMLDebug(4, ("parse.c: insertElement, added a missing %s %s at "
		"line %i.\n", is_end ? "closing" : "opening", element, num_lines));

#if defined(DEBUG) || defined(VERIFY)
	p_inserted++;
#endif
}

/*****
* Name: 		terminateElement
* Return Type: 	void
* Description: 	backtracks in the list of elements to terminate the given
*				element. Used for terminating an unbalanced element.
* In: 
*	element:	element name
*	new_id:		id of element to be terminated
* Returns:
*	nothing.
*****/
static void
terminateElement(String element, htmlEnum current)
{
	XmHTMLObject *extra, *obj, *swap;
	String tmp;
	stateStack *state = state_stack;
	int i, level = 0;

	/* need to do this, _XmHTMLFreeObjects will free this */
	tmp = strdup(element);

	/* allocate a element */
	extra = newElement(current, tmp, NULL, True, True);
#ifdef VERIFY
	extra->auto_insert = True;
#endif

	/* Check how deep this element is in the stateStack */
	while(state->next != NULL && state->id != current)
	{
		state = state->next;
		level++;
	}

	/*
	* Is first element on stack, simply terminate it by inserting at current
	* list position.
	*/
	if(level == 0)
	{
		list_data.num_elements++;
		extra->prev = list_data.current;
		list_data.current->next = extra;
		list_data.current = extra;

		/* restore stack */
		(void)popState(num_lines);
	}
	else
	{
		/*
		* Now walk the list of objects until we have backtracked level+1 
		* elements (level is the level in the stack at which the offending
		* id is found).
		*/
		for(i = 0, obj = list_data.current; obj != NULL && i != level+1;
			obj = obj->prev)
		{
			/*
			* As the stack only contains terminated elements, we may only look
			* for terminated elements which are opening (still following?)
			*/
			if(obj->terminated && !obj->is_end)
				i++;
		}
		/* rather impossible but allow for it anyway by simply terminating it */
		if(obj == NULL)
		{
			list_data.num_elements++;
			extra->prev = list_data.current;
			list_data.current->next = extra;
			list_data.current = extra;

			/* restore stack */
			(void)popState(num_lines);
		}
		else
		{
			htmlEnum *stack;

			/* insert this element in the list */
			list_data.num_elements++;
			swap = obj->next;
			obj->next   = extra;
			extra->prev = obj;
			extra->next = swap;
			swap->prev  = extra;

			/* Save stack up to offending id */
			stack = (htmlEnum*)malloc(level*sizeof(htmlEnum));
			i = 0;
			while(state_stack->id != current)
				stack[i++] = popState(num_lines);

			/* pop offending id */
			(void)popState(num_lines);

			/* and restore stack again */
			do
			{
				pushState(stack[--i], num_lines);
			}while(i);
			free(stack);
		}
	}

	_XmHTMLDebug(4, ("parse.c: terminateElement, terminated element %s "
		"at line %i, %i levels deep.\n", element, num_lines, level));

#if defined(DEBUG) || defined(VERIFY)
	p_inserted++;
#endif
}

static void
initMasterObjectList(void)
{
	if(list_data.head)
		free(list_data.head);

	list_data.num_elements = 0;
	list_data.num_text = 0;
	list_data.head = NULL;
	list_data.current = NULL;

#if 0
	/* initialize the object pool */
	list_data.obj_pool = (XmHTMLObject*)calloc(POOL_SIZE, sizeof(XmHTMLObject));
	list_data.curr_obj = list_data.obj_pool;
	list_data.pool_total = list_data.pool_avail = POOL_SIZE;
#endif
}

#if 0
static void
finalizeMasterObjectList(void)
{
	/* resize pool list */
	list_data.obj_pool = (XmHTMLObject*)realloc(list_data.obj_pool,
		(list_data.pool_total - list_data.pool_avail)*sizeof(XmHTMLObject));

	_XmHTMLDebug(4, ("parse.c: pool statistics\n"));
	_XmHTMLDebug(4, ("\ttotal pool size: %i\n", list_data.pool_total));
	_XmHTMLDebug(4, ("\tstill available: %i\n", list_data.pool_avail));
	_XmHTMLDebug(4, ("\tno of pool resizes: %i\n",
			(int)(list_data.pool_total/POOL_SIZE)));
}
#endif

/*****
* Name: 		newElement
* Return Type: 	XmHTMLObject
* Description: 	allocates a new XmHTMLObject structure
* In: 
*	id:			id for this element
*	element:	char description for this element
*	attributes:	attributes for this element
*	is_end:		bool indicating whether this element is a closing one
*	terminated:	True when this is element has a terminating counterpart
* Returns:
*	a newly allocated XmHTMLObject. Exits the program if the allocation fails.
*****/
static XmHTMLObject*
newElement(htmlEnum id, char *element, char *attributes, Boolean is_end,
	Boolean terminated)
{
	static XmHTMLObject *entry = NULL;

#if 0
	if(!list_data.pool_avail)
	{
		/* enlarge current pool */
		list_data.obj_pool = (XmHTMLObject*)realloc(list_data.obj_pool,
			(list_data.pool_total + POOL_SIZE)*sizeof(XmHTMLObject));
		/* current available object */
		list_data.curr_obj = list_data.obj_pool + list_data.pool_total;
		/* new poolsize */
		list_data.pool_total += POOL_SIZE;
		/* this many entries available */
		list_data.pool_avail = POOL_SIZE;
	}
	/* get an object from the pool */
	entry = list_data.curr_obj++;
	list_data.pool_avail--;
#endif

	entry = (XmHTMLObject*)malloc(sizeof(XmHTMLObject));

	entry->id         = id;
	entry->element    = element;
	entry->attributes = attributes;
	entry->is_end     = is_end;
	entry->terminated = terminated;
	entry->line = num_lines;
	entry->next = (XmHTMLObject*)NULL;
	entry->prev = (XmHTMLObject*)NULL;

#ifdef VERIFY
	entry->ignore      = False;
	entry->auto_insert = False;
#endif

	return(entry);
}

/*****
* Name: 		storeTextElement
* Return Type: 	void
* Description: 	allocates and stores a plain text element
* In: 
*	start:		plain text starting point 
*	end:		plain text ending point
* Returns:
*	nothing
*****/
static void
storeTextElement(char *start, char *end)
{
	static XmHTMLObject *element = NULL;
	static char *content = NULL;
	/* length of this block */
	int len = end - start;	

	/* sanity */
	if(*start == '\0' || len <= 0)
		return;

	content = my_strndup(start, len);

	/*
	* expansion of character escape sequences is done in format.c,
	* routine CopyText. The reason for this is that we must be able
	* to construct a valid HTML text for a number of the XmHTMLText
	* routines.
	*/

	element = newElement(HT_ZTEXT, content, NULL, False, False);

	/* store this element in the list */
	list_data.num_text++;
	element->prev = list_data.current;
	list_data.current->next = element;
	list_data.current = element;
}

/*****
* Name: 		storeTextElementRtoL
* Return Type: 	void
* Description: 	allocates and stores a plain text element. Inverts
*				contents of this text element as well.
* In: 
*	start:		plain text starting point 
*	end:		plain text ending point
* Returns:
*	nothing
*****/
#ifndef VERIFY
static void
storeTextElementRtoL(char *start, char *end)
{
	static XmHTMLObject *element = NULL;
	static char *content = NULL;
	register char *inPtr, *outPtr;

	/* length of this block */
	int len = end - start;	

	/* sanity */
	if(*start == '\0' || len <= 0)
		return;

	content = (char*)malloc(len+1);	/* +1 for terminating \0 */

	/* copy text, reversing contents as we do */
	inPtr = start;
	outPtr = &content[len-1];
	while(1)
	{
		switch(*inPtr)
		{
			case '&':
				/* we don't touch escape sequences */
				{
					register char *ptr;

					/* set start position */
					ptr = inPtr;

					/* get end */
					while(ptr < end && *ptr != ';')
						ptr++;

					/* might not be a valid escape sequence */
					if(ptr == end)
						break;

					/* insertion position */
					outPtr -= (ptr - inPtr);

					/* copy literally */
					memcpy(outPtr, inPtr, (ptr+1) - inPtr);

					/* new start position */
					inPtr = ptr;
				}
				break;
			/*****
			* All bi-directional characters need to be reversed if we want
			* them to keep their intended behaviour.
			*****/
			case '`':
				*outPtr = '\'';
				break;
			case '\'':
				*outPtr = '`';
				break;
			case '<':
				*outPtr = '>';
				break;
			case '>':
				*outPtr = '<';
				break;
			case '\\':
				*outPtr = '/';
				break;
			case '/':
				*outPtr = '\\';
				break;
			case '(':
				*outPtr = ')';
				break;
			case ')':
				*outPtr = '(';
				break;
			case '[':
				*outPtr = ']';
				break;
			case ']':
				*outPtr = '[';
				break;
			case '{':
				*outPtr = '}';
				break;
			case '}':
				*outPtr = '{';
				break;
			default:
				*outPtr = *inPtr;
				break;
		}
		inPtr++;
		outPtr--;
		if(inPtr == end)
			break;
	}
	content[len] = '\0';	/* NULL terminate */

	/*
	* expansion of character escape sequences is done in format.c,
	* routine CopyText. The reason for this is that we must be able
	* to construct a valid HTML text for a number of the XmHTMLText
	* routines.
	*/

	element = newElement(HT_ZTEXT, content, NULL, False, False);

	/* store this element in the list */
	list_data.num_text++;
	element->prev = list_data.current;
	list_data.current->next = element;
	list_data.current = element;
}
#endif

/*****
* Name: 		copyElement
* Return Type: 	void
* Description: 	copies and inserts the given object
* In: 
*	src:		object to copy
*	is_end:		terminator state
* Returns:
*	nothing
*****/
static void
copyElement(XmHTMLObject *src, Boolean is_end)
{
	static XmHTMLObject *copy;
	int len;

	/* sanity */
	if(src == NULL)
		return;

	copy = (XmHTMLObject*)malloc(sizeof(XmHTMLObject));

	copy->id   = src->id;
	copy->is_end = is_end;
	copy->terminated = src->terminated;
	copy->line = num_lines;
	copy->next = (XmHTMLObject*)NULL;
	copy->attributes = NULL;
#ifdef VERIFY
	copy->ignore = src->ignore;
	copy->auto_insert = src->auto_insert;
#endif

	/* allocate element data */
	len = strlen(src->element)+(src->attributes ? strlen(src->attributes) : 1);
	copy->element = (char*)malloc((len+2)*sizeof(char));

	/* copy element data */
	len = strlen(src->element);
	strcpy(copy->element, src->element);
	copy->element[len] = '\0';

	/* copy possible attributes */
	if(src->attributes)
	{
		strcpy(&copy->element[len+1], src->attributes);
		copy->attributes = &copy->element[len+1];
	}

	list_data.num_elements++;
	/* attach prev and next ptrs to the appropriate places */
	copy->prev = list_data.current;
	list_data.current->next = copy;
	list_data.current = copy;
}

/*****
* Name: 		parserWarning
* Return Type: 	int
* Description: 	gives out warning messages depending on the type of error.
* In: 
*	id:			offending id
*	current:	current parser state
*	error:		type of error
* Returns:
*	nothing.
*****/
static void
parserWarning(htmlEnum id, htmlEnum current, parserError error)
{
	static char msg[1024], tmp[128];
	int len;

#if defined(DEBUG) && !defined(VERIFY)
	/* overrides any bad_html_warnings resource */
	if(html_widget->html.debug_disable_warnings)
		return;
#endif

	/* get max length for the tmp array */
	len = current_end_pos - current_start_pos;
	if(len > 127)
		len = 127;

	/*
	* make appropriate error message, set bad_html flag and update error
	* count when error indicates a markup error or HTML violation.
	*/
	switch(error)
	{
		case HTML_UNKNOWN_ELEMENT:
			strncpy(tmp, &text_input[current_start_pos], len);
			tmp[len] = '\0';
			sprintf(msg, "%s: unknown HTML identifier.", tmp);
			break;
		case HTML_OPEN_ELEMENT:
			sprintf(msg, "Unbalanced terminator: got %s while %s is "
				"required.", html_tokens[id], html_tokens[current]);
			html32 = False;
			err_count++;
			break;
		case HTML_BAD:
			sprintf(msg, "Terrible HTML! element %s completely out "
				"of balance", html_tokens[id]);
			html32 = False;
			err_count++;
			break;
		case HTML_OPEN_BLOCK:
			sprintf(msg, "A new block level element (%s) was encountered "
				"while %s is still open.", html_tokens[id],
				html_tokens[current]);
			html32 = False;
			err_count++;
			break;
		case HTML_CLOSE_BLOCK:
			sprintf(msg, "A closing block level element (%s) was encountered "
				"while it was\n    never opened.", html_tokens[id]);
			html32 = False;
			err_count++;
			break;
		case HTML_NESTED:
			sprintf(msg, "Improperly nested element: %s may not be nested",
				html_tokens[id]);
			html32 = False;
			err_count++;
			break;
		case HTML_VIOLATION:
			sprintf(msg, "%s may not occur inside %s",
				html_tokens[id], html_tokens[current]);
			html32 = False;
			err_count++;
			break;
		case HTML_NOTIFY:
			return;
		case HTML_INTERNAL:
			sprintf(msg, "Internal parser error!");
			err_count++;
			break;
		/* no default */
	}

	_XmHTMLWarning(__WFUNC__(widget, "verifyElement"), "%s\n    "
		"(line %i in input)", msg, num_lines);
}

/*****
* Name: 		verifyElement
* Return Type: 	int
* Description: 	element verifier, the real funny part.
* In: 
*	id:			element to verify
*	is_end:		element terminating state
* Returns:
*	-1 when element should be removed, 0 when the element is not terminated
*	and 1 when it is.
*	This routine tries to do a huge amount of damage control by a number
*	of checks (and is a real mess).
* Note:
*	This routine is becoming increasingly complex, especially with possible
*	iteration over all current parser states to find a proper insertion element
*	when the new element is out of place, the checks on contents of the current
*	element and appearance of the new element and the difference between
*	opening and closing elements.
*	This routine is far too complex to explain, I can hardly even grasp it
*	myself. If you really want to know what is happening here, read thru it
*	and keep in mind that checkOccurance and checkContent behave *very*
*	differently from each other.
*****/
static int
verifyElement(htmlEnum id, Boolean is_end)
{
	/* current parser state */
	htmlEnum current = state_stack->id;
	int iter = 0, new_id;

	/* ending elements are automatically terminated */
	if(is_end || getTerminatorState(id))
	{
		if(!is_end)
		{
			/*
			* First check: if the new element matches the current state,
			* we first need to terminate the previous element (remember the
			* new element is a starting one). Don't do this for nested
			* elements since that has the potential of seriously messing
			* things up.
			*/
			if(id == current && !(NESTED_ELEMENT(id)))
			{
				/* invalid nesting if this is not an optional closure */
				if(!(OPTIONAL_CLOSURE(current)))
					parserCallback(id, current, HTML_NESTED);

				/* default is to terminate current state */
				insertElement(html_tokens[current], current, True);
				/* new element matches current, so it stays on the stack */
				return(1);
			}
			/*
			* Second check: see if the new element is allowed to occur
			* inside the current element.
			*/
			if((new_id = checkOccurance(id, current)) != HT_ZTEXT && 
				new_id != -1)
			{
				parserCallback(id, current, HTML_VIOLATION);
				insertElement(html_tokens[new_id], (htmlEnum)new_id,
					new_id == current);
				/*
				* If the new element terminates it's opening counterpart,
				* pop it from the stack. Otherwise it adds a new parser state.
				*/
				if(new_id == current)
					(void)popState(num_lines);
				else
					pushState((htmlEnum)new_id, num_lines);
				/* new element is now allowed, push it */
				pushState(id, num_lines);
				return(1);
			}
			/*
			* not allowed, see if the content matches. If not, terminate
			* the previous element and proceed as if nothing ever happened.
			*/
recheck:
			/* damage control */
			if(iter > 4 || (state_stack->next == NULL && iter))
			{
				/* stack restoration */
				if(state_stack->id == HT_DOCTYPE)
					pushState(HT_HTML, num_lines);
				if(state_stack->id == HT_HTML)
					pushState(HT_BODY, num_lines);

				/* HTML_BAD, default is to remove it */
				parserCallback(id, current, HTML_BAD);
				return(-1);
			}
			iter++;
			/*
			* Third check: see if the new element is allowed as content
			* of the current element. This check will iterate until it
			* finds a matching parser state or until the parser runs out
			* of states.
			*/
			if(!checkContent(id, current))
			{
				/*
				* HTML_OPEN_BLOCK, default is to insert current
				* spit out a warning if it's really missing
				 */
				if(!(OPTIONAL_CLOSURE(current)))
					parserCallback(id, current, HTML_OPEN_BLOCK);

				/* terminate current element before adding the new one*/
				if(id == current ||
					(current != HT_DOCTYPE && current != HT_HTML &&
					 current != HT_BODY))
					insertElement(html_tokens[current], current, True);
				(void)popState(num_lines);
				current = state_stack->id;
				goto recheck;
			}
			else if(!is_end && !(checkContent(id, current)))
			{
				parserCallback(id, current, HTML_VIOLATION);
				return(-1);
			}
			/* element allowed, push it */
			pushState(id, num_lines);
			return(1);
		}
		else 
		{
			/* First check: see if this element has a terminating counterpart */
			if(!getTerminatorState(id))
			{
				/*
				* We do not known terminated elements that can't be
				* terminated (*very* stupid HTML).
				*/
				parserWarning(current, current, HTML_UNKNOWN_ELEMENT);
				return(-1);	/* obliterate it */
			}
			/*
			* Second check: see if the counterpart of this terminating element
			* is on the stack. If it isn't, we probably terminated it
			* ourselves to keep the document properly balanced and we don't
			* want to insert this terminator as it probably will change the
			* document substantially (a perfect example is
			* <p><form></p>..</form>, which without this check would be changed
			* to <p></p><form></form><p></p>... instead of
			* <p></p><form>...</form>.
			*/
			if(!onStack(id))
			{
				parserWarning(id, current, HTML_CLOSE_BLOCK);
				return(-1);
			}

			/* element ends, check balance. */
reterminate:
			/* damage control */
			if(iter > 4 || (state_stack->next == NULL && iter))
			{
				/* stack restoration */
				if(state_stack->id == HT_DOCTYPE)
					pushState(HT_HTML, num_lines);
				if(state_stack->id == HT_HTML)
					pushState(HT_BODY, num_lines);

				/* HTML_BAD, default is to remove it */
				parserCallback(id, current, HTML_BAD);
				return(-1);
			}
			iter++;
			if(id != current)
			{
				/*
				* This check and the next are real ugly: this one checks
				* whether the current parser state is still valid if the
				* new terminator is inserted, while the next one checks whether
				* the new terminator may appear in the current parser state.
				* This is becoming increasingly complex :-(
				*/
				if((checkOccurance(id, current)) != HT_ZTEXT)
				{
					/* remove if it's not an optional closing element */
					if(!(OPTIONAL_CLOSURE(current)))
					{
						/*
						* if id is present on stack we have an unbalanced
						* terminator
						*/
						if(onStack(id))
							goto unbalanced;

						parserCallback(id, current, HTML_CLOSE_BLOCK);
						/* HTML_CLOSE_BLOCK, default is to remove it */
						return(-1);
					}
					/* terminate current before adding the new one */
					if(id == current ||
						(current != HT_DOCTYPE && current != HT_HTML &&
						 current != HT_BODY))
						insertElement(html_tokens[current], current, True);
					current = popState(num_lines);
					if(current != id)
					{
						current = state_stack->id;
						goto reterminate;
					}
				}
				else if((new_id = checkOccurance(current, id)) != -1)
				{
					if(new_id == HT_ZTEXT)
						new_id = current;
					/* remove if it's not an optional closing element */
					if(!(OPTIONAL_CLOSURE(current)))
					{
						/*
						* if id is present on stack we have an unbalanced
						* terminator
						*/
						if(onStack(id))
							goto unbalanced;

						/* HTML_CLOSE_BLOCK, default is to remove it */
						parserCallback(id, current, HTML_CLOSE_BLOCK);
						return(-1);
					}
					/* terminate current before adding the new one */
					if(id == current ||
						(current != HT_DOCTYPE && current != HT_HTML &&
						 current != HT_BODY))
						insertElement(html_tokens[new_id], new_id, True);
					current = popState(num_lines);
					if(current != id)
					{
						current = state_stack->id;
						goto reterminate;
					}
				}
				else
				{
unbalanced:
					/* switch if it's not an optional closing element */
					if(!(OPTIONAL_CLOSURE(current)))
					{
						parserCallback(id, current, HTML_OPEN_ELEMENT);
						/* HTML_OPEN_ELEMENT, default is to switch */
						terminateElement(html_tokens[id], id);
					}
					/*
					* Current state is an optional closure and the new
					* element causes it to be inserted. Make it so and
					* redo the entire process. This will emit all optional
					* closures that prevent the new element from becoming
					* legal and will balance the stack.
					* Sigh. The horror of HTML...
					*/
					/* fix 08/04/97-01, kdh */
					else
					{
						if(id == current ||
							(current != HT_DOCTYPE && current != HT_HTML &&
							 current != HT_BODY))
							insertElement(html_tokens[current], current, True);
						current = popState(num_lines);
						if(current != id)
						{
							current = state_stack->id;
							goto reterminate;
						}
					}
				}
			}
			/* resync */
			if(id == state_stack->id)
				(void)popState(num_lines);
		}
		return(1);	/* element allowed */
	}
	else
	{
		/* see if the new element is allowed as content of current element. */
		if((new_id = checkOccurance(id, current)) != HT_ZTEXT)
		{
			/* maybe terminate the current parser state? */
			if(new_id == -1)
				new_id = current;

			parserCallback(id, current, HTML_VIOLATION);

			/* HTML_VIOLATION, default is to insert since new_id is valid */
			insertElement(html_tokens[new_id], (htmlEnum)new_id, 
				new_id == current);
			if(new_id == current)
				(void)popState(num_lines);
			else
				pushState((htmlEnum)new_id, num_lines);
		}
		return(0);
	}
	return(0);	/* not reached */
}

/*****
* Name: 		verifyVerification
* Return Type: 	XmHTMLObject*
* Description: 	checks whether the document verification/repair routines did
*				a proper job.
* In: 
*	html:		XmHTMLWidget id;
*	objects:	parser output.
* Returns:
*	NULL when all parser states are balanced, offending object otherwise.
*****/
static XmHTMLObject *
verifyVerification(XmHTMLObject *objects)
{
	XmHTMLObject *tmp = objects;
	htmlEnum current;

#ifdef VERIFY
	struct timeval ts, te;
	int secs, usecs;
	fprintf(stderr, "-------------------\n");
	fprintf(stderr, "Verifying parser output: parser ");

	/* verify the validitity of the parser output */
	gettimeofday(&ts, NULL);
#endif

	/* walk to the first terminated item in the list */
#ifdef VERIFY
	while(tmp != NULL && !tmp->terminated && !tmp->ignore)
#else
	while(tmp != NULL && !tmp->terminated)
#endif
		tmp = tmp->next;

	/* reset state stack */
	state_stack = &state_base;
	state_stack->id = current = tmp->id;
	state_stack->next = NULL;
#ifdef DEBUG
	id_depth = 0;
#endif

	tmp = tmp->next;

	for(; tmp != NULL; tmp = tmp->next)
	{
#ifdef VERIFY
		if(tmp->terminated && !tmp->ignore)
#else
		if(tmp->terminated)
#endif
		{
			if(tmp->is_end)
			{
				if(current != tmp->id)
					break;
				current = popState(tmp->line);
			}
			else
			{
				pushState(current, tmp->line);
				current = tmp->id;
			}
		}
	}
#ifdef VERIFY
	gettimeofday(&te, NULL);
	secs = (int)(te.tv_sec - ts.tv_sec);
	usecs = (int)(te.tv_usec - ts.tv_usec);
	if(usecs < 0) usecs *= -1;
	fprintf(stderr, "%s\n", tmp == NULL ? "did OK" : "Failed");
	if(tmp)
		fprintf(stderr, "offending %s element %s found at line %i in input\n",
			tmp->is_end ? "closing":"opening", html_tokens[tmp->id], tmp->line);
	fprintf(stderr, "verifyVerification done in %i.%i seconds\n", secs, usecs);
#endif

	return(tmp);
}

/*****
* Name: 		storeElement
* Return Type: 	String
* Description: 	allocates and stores a HTML element
* In: 
*	start:		element starting point 
*	end:		element ending point
* Returns:
*	Updated char position if we had to skip <SCRIPT> or <STYLE> data.
*****/
static String
storeElement(char *start, char *end)
{
	register char *chPtr, *elePtr;
	char *startPtr, *endPtr;
	Boolean is_end = False;
	static XmHTMLObject *element;
	static char *content;
	htmlEnum id;
	int terminated;
#ifdef VERIFY
	Boolean ignore;
#endif

	if(end == NULL || *end == '\0')
		return(end);

	/* absolute ending position for this element */
	current_end_pos = current_start_pos + (end - start);

	/*****
	* If this is true, we have an empty tag or an empty closing tag. 
	* action to take depends on what type of empty tag it is.
	*****/
	if(start[0] == '>' || (start[0] == '/' && start[1] == '>'))
	{
		/***** 
		* if start[0] == '>', we have an empty tag, otherwise we have an empty
		* closing tag. In the first case, we walk backwards until we reach the 
		* very first tag. 
		* An empty tag simply means: copy the previous tag, nomatter what 
		* content it may have. In the second case, we need to pick up the 
		* last recorded opening tag and copy it.
		*****/
		if(start[0] == '>')
		{
			/*****
			* Walk backwards until we reach the first non-text element.
			* Elements with an optional terminator which are not terminated
			* are updated as well.
			*****/
			for(element = list_data.current ; element != NULL;
				element = element->prev)
			{
				if(OPTIONAL_CLOSURE(element->id) && !element->is_end &&
					element->id == state_stack->id)
				{
					insertElement(element->element, element->id, True);
					(void)popState(num_lines);
					break;
				}
				else if(element->id != HT_ZTEXT)
					break;
			}
			_XmHTMLDebug(4, ("parse.c: storeElement, empty tag on line %i, "
				"inserting %s\n", num_lines, element->element));
			copyElement(element, False);
			if(element->terminated)
				pushState(element->id, num_lines);
		}
		else
		{
			for(element = list_data.current; element != NULL; 
				element = element->prev)
			{
				if(element->terminated)
				{
					if(OPTIONAL_CLOSURE(element->id))
					{
						/* add possible terminators for these elements */
						if(!element->is_end && element->id == state_stack->id)
						{
							insertElement(element->element, element->id, True);
							(void)popState(num_lines);
						}
					}
					else
						break;
				}
			}

			_XmHTMLDebug(4, ("parse.c: storeElement, empty closing tag on "
				"line %i, inserting %s\n", num_lines, element->element));
			copyElement(element, True);
			(void)popState(num_lines);
		}
		return(end);
	}

	startPtr = start;		
	/* Check if we have any unclosed tags */
	if((endPtr = strstr(start, "<")) == NULL)
		endPtr = end;
	/* check if we stay within bounds */
	else if(endPtr - end > 0)
		endPtr = end;

	while(1)
	{
		is_end = False;

		/***** 
		* First skip past spaces and a possible opening /. The endPtr test
		* is mandatory or else we would walk the entire text over and over
		* again every time this routine is called.
		*****/
		for(elePtr = startPtr; *elePtr != '\0' && elePtr != endPtr; elePtr++)
		{
			if(*elePtr == '/')
			{
				is_end = True;
				elePtr++;
				break;
			}
			if(!(isspace(*elePtr)))
				break;
		}
		/* usefull sanity */
		if(endPtr - elePtr < 1)
			break;

		/* allocate and copy element */
		content = my_strndup(elePtr, endPtr - elePtr);

		chPtr = elePtr = content;

		/***** 
		* Move past the text to get any element attributes. The ! will let us 
		* pick up the !DOCTYPE definition.
		* Don't put the chPtr++ inside the tolower, chances are that it will be
		* evaluated multiple times if tolower is a macro.
		* From: Danny Backx <u27113@kb.be>
		*****/
		if(*chPtr == '!')
			chPtr++;
		while(*chPtr && !(isspace(*chPtr)))		/* fix 01/17/97-01; kdh */
		{
			*chPtr = tolower(*chPtr);
			chPtr++;
		}

		/***** 
		* attributes are only allowed in opening elements 
		* This is a neat hack: to reduce allocations, we do *not* copy the
		* element name into it's own buffer. Instead we use the one allocated
		* above, and place a \0 in the space right after the HTML element.
		* If this element has attributes, we set the attribute pointer (=chPtr)
		* to point right after this \0. 
		* This also has the advantage that no reallocation or string copying 
		* is required.
		* Freeing the memory allocated can be done in one call on the element 
		* field of the XmHTMLObject struct.
		*****/
		if(!is_end)
		{
			if(*chPtr && *(chPtr+1))
			{
				content[chPtr-elePtr] = '\0';
				chPtr = content + strlen(content) + 1;
			}
			else	/* no trailing attributes for this element */
				if(*chPtr)		/* fix 01/17/97-01; kdh */
					content[chPtr-elePtr] = '\0';
				else
					chPtr = NULL;
		}
		else	/* closing element, can't have any attributes */
			chPtr = NULL;

		/* Ignore elements we do not know */
		if((id = tokenToId(elePtr, bad_html_warnings)) != -1)
		{
			/*****
			* Check if this element belongs to body. This test is as best
			* as it can get (we do not scan raw text for non-whitespace chars)
			* but will omit any text appearing *before* the <body> tag is
			* inserted.
			*****/
			if(!have_body)
			{
				if(id == HT_BODY)
					have_body = True;
				else if(isBodyElement(id))
				{
					insertElement("body", HT_BODY, False);
					pushState(HT_BODY, num_lines);
					have_body = True;
				}
			}

			/* Go and verify presence of the new element. */
			terminated = verifyElement(id, is_end);

#ifdef VERIFY
			if(terminated == -1)
			{
				html32 = False;	/* not HTML32 conforming */
				p_removed++;
				ignore = True;
				terminated = getTerminatorState(id);
			}
			else
				ignore = False;
#else
			if(terminated == -1)
			{
				html32 = False;	/* not HTML32 conforming */
#if defined(DEBUG)
				p_removed++;
#endif
				free(content);
				/* remove contents of badly placed SCRIPT & STYLE elements */
				if((id == HT_SCRIPT || id == HT_STYLE) && !is_end)
					goto removeData;
				return(end);
			}
#endif /* VERIFY */

			/* insert the current element */
			element = newElement(id, elePtr, chPtr, is_end,
				(Boolean)terminated);
#ifdef VERIFY
			element->ignore = ignore;
#endif
			/* attach prev and next ptrs to the appropriate places */
			list_data.num_elements++;
			element->prev = list_data.current;
			list_data.current->next = element;
			list_data.current = element;

			/*****
			* The SCRIPT & STYLE elements are a real pain in the ass to deal
			* with properly: they are text with whatever in it,
			* and it's terminated by a closing element. It would be
			* *very* easy if the tags content are enclosed within a 
			* comment, but since this is *NOT* required by the HTML 3.2 spec,
			* we need to check it in here...
			*****/
#ifndef VERIFY
removeData:
#endif
			if((id == HT_SCRIPT || id == HT_STYLE) && !is_end)
			{
				int done = 0;
				char *start = end;
				register int text_len = 0;

				/* move past closing > */
				start++;
				while(*end != '\0' && done == 0)
				{
					switch(*end)
					{
						case '<':
							/* catch comments */
							if(*(end+1) == '/')
							{
								if(!(strncasecmp(end+1, "/script", 7)))
									done = 1;
								else if(!(strncasecmp(end+1, "/style", 6)))
									done = 2;
							}
							break;
						case '\n':
							num_lines++;
							/* fall through */
						default:
							text_len++;
							break;
					}
					if(*end == '\0')
						break;
					end++;
				}
				if(done)
				{
					/*****
					* Only store contents if this tag was in place. This
					* check is required as this piece of code is also used
					* to remove the tags content when the tag is out of it's
					* proper place (not inside the <head> section).
					* We must always do this when we are compiled as standalone
					* parser.
					*****/
#ifndef VERIFY
					if(terminated != -1)
#endif
					{
						/* store script contents */
						storeTextElement(start, end-1);

						/* this was a script */
						if(done == 1)
							insertElement("script", HT_SCRIPT, True);
						else
							insertElement("style", HT_STYLE, True);
						/* pop state from stack */
						(void)popState(num_lines);
					}

					/* move to the closing > */
					while(*end != '\0' && *end != '>')
						end++;
				}
				else
					/* restore original end position */
					end = start-1;
				
#if defined(DEBUG) || defined(VERIFY)
				/* closing tag is also removed */
				if(terminated == -1)
					p_removed++;
#endif
				/* no check for unclosed tags here, just return */
				return(end);
			}
		}
		else
		{
			/* ignore */
			free(content);	/* fix 01/28/97-01, kdh */
		}

		/* check if we have any unclosed tags remaining. */
		if(endPtr - end < 0)
		{
			endPtr++;
			startPtr = endPtr;		
			/* Check if we have any unclosed tags */
			if((endPtr = strstr(startPtr, "<")) == NULL)
				endPtr = end;

			/* check if we stay within bounds */
			else if(endPtr - end > 0)
					endPtr = end;
		}
		else
			break;
	}
	return(end);
}

#ifndef VERIFY
/*****
* Name: 		storeElementUnconditional
* Return Type: 	String
* Description: 	allocates and stores a HTML element *without* verifying it.
* In: 
*	start:		element starting point 
*	end:		element ending point
* Returns:
*	Updated char position if we had to skip <SCRIPT> or <STYLE> data.
*****/
static String
storeElementUnconditional(char *start, char *end)
{
	register char *chPtr, *elePtr;
	char *startPtr, *endPtr;
	Boolean is_end = False;
	static XmHTMLObject *element;
	static char *content;
	htmlEnum id;
	Boolean terminated;

	if(end == NULL || *end == '\0')
		return(end);

	/* absolute ending position for this element */
	current_end_pos = current_start_pos + (end - start);

	/* no null end tags here */

	startPtr = start;		
	/* Check if we have any unclosed tags */
	if((endPtr = strstr(start, "<")) == NULL)
		endPtr = end;
	/* check if we stay within bounds */
	else if(endPtr - end > 0)
		endPtr = end;

	is_end = False;

	/***** 
	* First skip past spaces and a possible opening /. The endPtr test
	* is mandatory or else we would walk the entire text over and over
	* again every time this routine is called.
	*****/
	for(elePtr = startPtr; *elePtr != '\0' && elePtr != endPtr; elePtr++)
	{
		if(*elePtr == '/')
		{
			is_end = True;
			elePtr++;
			break;
		}
		if(!(isspace(*elePtr)))
			break;
	}
	/* usefull sanity */
	if(endPtr - elePtr < 1)
		return(end);

	/* allocate and copy element */
	content = my_strndup(elePtr, endPtr - elePtr);

	chPtr = elePtr = content;

	/* Move past the text to get any element attributes. */
	if(*chPtr == '!')
		chPtr++;
	while(*chPtr && !(isspace(*chPtr)))	
	{
		*chPtr = tolower(*chPtr);
		chPtr++;
	}

	/* attributes are only allowed in opening elements */
	if(!is_end)
	{
		if(*chPtr && *(chPtr+1))
		{
			content[chPtr-elePtr] = '\0';
			chPtr = content + strlen(content) + 1;
		}
		else	/* no trailing attributes for this element */
			if(*chPtr)
				content[chPtr-elePtr] = '\0';
			else
				chPtr = NULL;
	}
	else	/* closing element, can't have any attributes */
		chPtr = NULL;

	/* Ignore elements we do not know */
	if((id = tokenToId(elePtr, bad_html_warnings)) != -1)
	{
		/* see if this element has a closing counterpart */
		terminated = getTerminatorState(id);

		/* insert the current element */
		element = newElement(id, elePtr, chPtr, is_end, terminated); 

#ifdef VERIFY
		element->ignore = False;
#endif
		/* attach prev and next ptrs to the appropriate places */
		list_data.num_elements++;
		element->prev = list_data.current;
		list_data.current->next = element;
		list_data.current = element;

		/* get contents of the SCRIPT & STYLE elements */
		if((id == HT_SCRIPT || id == HT_STYLE) && !is_end)
		{
			int done = 0;
			char *start = end;
			register int text_len = 0;

			/* move past closing > */
			start++;
			while(*end != '\0' && done == 0)
			{
				switch(*end)
				{
					case '<':
						/* catch comments */
						if(*(end+1) == '/')
						{
							if(!(strncasecmp(end+1, "/script", 7)))
								done = 1;
							else if(!(strncasecmp(end+1, "/style", 6)))
								done = 2;
						}
						break;
					case '\n':
						num_lines++;
						/* fall through */
					default:
						text_len++;
						break;
				}
				if(*end == '\0')
					break;
				end++;
			}
			if(done)
			{
				/* store script contents */
				storeTextElement(start, end-1);

				/* this was a script */
				if(done == 1)
					insertElement("script", HT_SCRIPT, True);
				else
					insertElement("style", HT_STYLE, True);

				/* move to the closing > */
				while(*end != '\0' && *end != '>')
					end++;
			}
			else
				/* restore original end position */
				end = start-1;
				
			/* no check for unclosed tags here, just return */
			return(end);
		}
	}
	else /* ignore */
		free(content);

	return(end);
}
#endif

#if defined(DEBUG) || defined(VERIFY)
static void
writeParsedOutputToFile(XmHTMLObject *objects, String prefix)
{
	XmHTMLObject *tmp;
	char name[1024];
	FILE *file;
	int i, tablevel = 0;
	static int count = 0;

	/* 
	* No buffer overrun check here. If this sigsegv's, its your own fault.
	* Don't use names longer than 1024 bytes then.
	*/
	sprintf(name, "%s.%i", prefix, count);
	count++;

	if((file = fopen(name, "w")) == NULL)
	{
		fprintf(stderr, "parse.c: can't open file %s for writing\n", name);
		return;
	}

	for(tmp = objects; tmp != NULL; tmp = tmp->next)
	{
		if(tmp->id != HT_ZTEXT)
		{
			fprintf(file, "%i:", tmp->line);
			if(tmp->is_end)
			{
#ifdef VERIFY
				if(!tmp->ignore)
#endif
					tablevel--;
				if(tablevel < 0) tablevel++;
				for(i = 0; i != tablevel; i++)
					fputs("\t", file);
				fprintf(file, "</%s", html_tokens[tmp->id]); 
			}
			else
			{
				for(i = 0; i != tablevel; i++)
					fputs("\t", file);
				fprintf(file, "<%s", html_tokens[tmp->id]); 
#ifdef VERIFY
				if(tmp->terminated && !tmp->ignore)
#else
				if(tmp->terminated)
#endif
					tablevel++;
			}

			if(tmp->attributes)
				fprintf(file, " %s", tmp->attributes);

			fputs(">", file);

#ifdef VERIFY
			if(tmp->auto_insert == 1)
				fputs(" [AUTO-INSERTED]", file);
			else if(tmp->auto_insert == 2)
				fputs(" [CHECK-AUTO-INSERTED]", file);
			if(tmp->ignore)
				fputs(" [IGNORED]", file);
#endif
			fputs("\n", file);
		}
		else if(!notext)
			fprintf(file, "%i: %s\n", tmp->line, tmp->element); 
	}
	fputs("\n", file);
	fclose(file);
	fprintf(stderr, "parse.c: parser output written to %s\n", name);
}
#endif

/*****
* Name: 		cutComment
* Return Type: 	String
* Description: 	removes HTML comments from the input stream
* In: 
*	start:		comment opening position
* Returns:
*	comment ending position
* Note:
*	HTML comments are one of the most difficult things to deal with due to
*	the unlucky definition: a comment starts with a <!, followed by zero or
*	more comments, followed by >. A comment starts and ends with "--", and does
*	not contain any occurance of "--". This effectively means that dashes
*	*must* occur in a multiple of four. And this is were the problems lies:
*	_many_ people don't realize this and thus open their comments with <!-- and
*	end it with --> and put everything they like in it, including any number
*	of dashes and --> sequences. To deal with all of this as much as we can,
*	we scan the text until we reach a --> sequence with a balanced number of
*	dashes. If we run into a --> and we don't have a balanced number of dashes,
*	we look ahead in the buffer. The *original* comment is then terminated (by
*	rewinding to the original comment ending) if we encounter an element
*	opening (can be anything *except* <-) or if we run out of characters. If
*	we encounter another --> sequence, the comment ends here.
*	This is a severe performance penalty since a considerable number of
*	characters *can* be scanned in order to find an element start or the next
*	comment ending. Wouldn't life be *much* easier if we lived in a perfect
*	world!
*****/
static String
cutComment(XmHTMLWidget html, String start)
{
	int dashes = 0, nchars = 0, start_line = num_lines;
	Boolean end_comment = False, start_dashes = False;
	String chPtr = start;

	/* move past opening exclamation character */
	chPtr++;
	while(!end_comment && *chPtr != '\0')
	{
		switch(*chPtr)
		{
			case '\n':
				num_lines++;
				nchars++;
				break;	/* fix 01/14/97-01; kdh */
			case '-':
				/* comment dashes occur twice in succession */
				/* fix 01/14/97-02; kdh */
				/* fix 04/30/97-1; sl */
				if(*(chPtr+1) == '-' && !start_dashes)
				{
					chPtr++;
					nchars++;
					dashes++;
					start_dashes = True;
				}
				if(*(chPtr+1) == '-' || *(chPtr-1) == '-')
					dashes++;
				break;
			case '>':
				/*
				* Problem: > in a comment is a valid character, so the comment
				* should only be terminated when we have a multiple of four
				* dashes. If we haven't, we need to look ahead.
				*/
				if(*(chPtr-1) == '-')
				{
					if(!(dashes % 4))
						end_comment = True;
					else
					{
						char *sub = chPtr;
						Boolean end_sub = False;
						int sub_lines = num_lines, sub_nchars = nchars;
						/*
						* Scan ahead until we run into another --> sequence or
						* element opening. If we don't, rewind and terminate the
						* comment.
						*/
						do
						{
							chPtr++;
							switch(*chPtr)
							{
								case '\n':
									num_lines++;
									nchars++;
									break;	/* fix 01/14/97-01; kdh */
								case '-':
									if(*(chPtr+1) == '-' || *(chPtr-1) == '-')
										dashes++;
									break;
								case '<':
									/* comment ended at original position */
									if(*(chPtr+1) != '-')
									{
										chPtr = sub;
										end_sub = True;
									}
									break;
								case '>':
									/* comment ends here */
									if((strncmp(chPtr-2, "--", 2) == 0) &&
										start_dashes)
									{
										end_sub = True;
										end_comment = True;
										break;
									}
									/* another nested > */
									break;
								case '\0':
									/* comment ended at original position */
									chPtr = sub;
									end_sub = True;
									break;
							}
						}
						while(*chPtr != '\0' && !end_sub);
						if(chPtr == sub)
						{
							/* comment was ended at original position, rewind */
							end_comment = True;
							num_lines = sub_lines;
							nchars = sub_nchars;
						}
					}
				}
				else
					/* special case: the empty comment */
					if(*(chPtr-1) == '!' && !(dashes % 4))
						end_comment = True;
				break;
		}
		chPtr++;
		nchars++;
	}
	_XmHTMLDebug(4, ("parse.c: parseHTML, removed comment spanning "
		"%i chars between line %i and %i\n", nchars, start_line, num_lines));
	/* spit out a warning if the dash count is no multiple of four */
	if(dashes %4 && bad_html_warnings)
		_XmHTMLWarning(__WFUNC__(html, "parseHTML"),
			"Bad HTML comment on line %i of input:\n    number of dashes is "
			"no multiple of four (counted %i dashes)", start_line, dashes);
	return(chPtr);
}

#define NULL_END_TAG { \
	/* longest token is 10 chars (blockquote), include whitespace room */ \
	char token[16], *ptr; \
	htmlEnum id; \
	/* check if text between opening < and this first / is a valid html tag.*/ \
	/* Opening NULL-end tags must always be attached to the tag itself.*/ \
	if(chPtr - start > 15 || isspace(*(chPtr-1))) \
		break; \
	/* copy text */ \
	strncpy(token, start, chPtr - start); \
	token[chPtr - start] = '\0'; \
	ptr = token; \
	_XmHTMLDebug(4, ("parse.c: possible null-end token in: %s\n", token)); \
	/* cut leading spaces */ \
	while(*ptr && (isspace(*ptr))) \
	{ if(*ptr == '\n') num_lines++; ptr++; } \
	/* make lowercase */ \
	my_locase(ptr); \
	_XmHTMLDebug(4, ("parse.c: checking null-end token %s\n", token)); \
	/* no warning message when tokenToId fails */ \
	if((id = tokenToId(token, False)) != -1) \
	{ \
		/* store this element */ \
		(void)storeElement(start, chPtr); \
		_XmHTMLDebug(4,("parse.c: stored valid null-end token %s\n",token)); \
		/* move past the / */ \
		chPtr++; \
		text_start = chPtr; \
		text_len = 0; \
		/* walk up to the next / which terminates this block */ \
		for(; *chPtr != '\0' && *chPtr != '/'; chPtr++, cnt++, text_len++) \
			if(*chPtr == '\n') num_lines++; \
		/* store the text */ \
		if(text_len && text_start != NULL) \
			store_text_func(text_start, chPtr); \
		text_start = chPtr + 1; /* starts after this slash */ \
		text_len = 0; \
		/* store the end element. Use the empty closing element notation so */ \
		/* storeElement knows what to do. Reset element ptrs after that. */ \
		(void)storeElement("/>", chPtr); \
		start = NULL;		/* entry has been terminated */ \
		done = True; \
	} \
	else { _XmHTMLDebug(4, ("parse.c: %s: not a token.\n", token)); } \
}

/*****
* Name: 		parseHTML
* Return Type: 	void
* Description: 	html parser; creates a doubly linked list of all elements
*				(both HTML and plain text).
* In: 
*	html:		XmHTMLWidget id
*	old_list:	previous list to be freed.
*	input:		HTML text to parse
* Returns:
*	nothing.
*****/
static void
parseHTML(XmHTMLWidget html, char *text)
{
	register char *chPtr;
	char *start, *text_start;
	int cnt = 0, text_len = 0;
	Dimension line_len = 0;
	Boolean done;

	/* text insertion function to be used */
	void (*store_text_func)(char*, char*);

	/* we assume all documents are valid ;-) */
	bad_html = False;
	/* and that every document is HTML 3.2 conforming */
	html32 = True;

	/* are we instructed to being strict on HTML 3.2 conformance ? */
#ifndef VERIFY
	strict_checking = html->html.strict_checking;
	if(html->html.string_direction == TSTRING_DIRECTION_R_TO_L)
		store_text_func = storeTextElementRtoL;
	else
		store_text_func = storeTextElement;
#else
	store_text_func = storeTextElement;
#endif

	/* start scanning */
	start = text_start = NULL;
	text_input = chPtr = text;
	text_len = 0;
	num_lines = 1;	/* every editor starts its linecount at 1 */
	err_count = current_start_pos = current_end_pos = 0;

	while(*chPtr)
	{
		switch(*chPtr)
		{
			case '<':		/* start of element */
				/* See if we have any text pending */
				if(text_len && text_start != NULL)
				{
					store_text_func(text_start, chPtr);
					text_start = NULL;
					text_len = 0;
				}
				/* move past element starter */
				start = chPtr+1; /* element text starts here */
				done = False;
				/* absolute starting position for this element */
				current_start_pos = start - text;
				/*
				* scan until the end of this tag. Comments are removed
				* properly, but are *not* allowed inside tags.
				*/
				while(*chPtr != '\0' && !done)
				{
					chPtr++;
					switch(*chPtr)
					{
						case '!':
							/* either a comment or the !doctype */
							if((*(chPtr+1) == '>' || *(chPtr+1) == '-'))
							{
								chPtr = cutComment(html, chPtr);
								/* back up one element */
								chPtr--;
								start = chPtr;
								done = True;
							}
							break;
							/*
							* those goddamn quotes. They should be balanced,
							* so we look ahead and see if we can find a
							* closing > after the closing quote. Anything
							* can appear within these quotes so we break
							* out of it once we find either < or /> inside
							* the quote or > after the closing quote.
							*/
						case '\"':
							{
								/* first look ahead for a corresponding " */
								char *tmpE, *tmpS = chPtr;

								chPtr++;	/* move past it */
								for(; *chPtr && *chPtr != '\"' && *chPtr != '>';
									num_lines += (*chPtr++ == '\n' ? 1 : 0));

								/* phieeuw, we found one */
								if(!*chPtr || *chPtr == '\"')
									break;

								/*
								* Fuck me, it's unbalanced, check if we
								* run into one before we see a <.
								* save position first.
								*/
								tmpE = chPtr;
								for(; *chPtr && *chPtr != '\"' && *chPtr != '<';
									num_lines += (*chPtr++ == '\n' ? 1 : 0));

								/* phieeuw, we found one */
								if(!*chPtr || *chPtr == '\"')
									break;

								/*
								* If we get here it means the element
								* didn't have a closing quote. Spit out
								* a warning and restore saved position.
								*/
								if(bad_html_warnings)
								{
									int len = chPtr - tmpS;

									/* no overruns */
									char *msg = my_strndup(tmpS,
										(len < 128 ? len : 128));

									_XmHTMLWarning(__WFUNC__(html,
										"parseHTML"), "%s: badly placed or "
										"missing quote\n    (line %i in "
										"input)", msg, num_lines);
									free(msg);
								}
								chPtr = tmpE;
								/* fall thru */
							}

						case '>':
							/* go and store the element */
							chPtr = storeElement(start, chPtr);
							done = True;
							break;
						case '/':
							/*
							* only handle shorttags when requested. 
							* We have a short tag if this / is preceeded by
							* a valid character.
							*/
							if(isalnum(*(chPtr-1)))
							{
								NULL_END_TAG;
							}
							break;
						case '\n':
							num_lines++;
							break;
						default:
							break;
					}
				}
				if(done)
					text_start = chPtr+1; /* plain text starts here */
				text_len = 0;
				start = NULL;
				break;
			case '\n':
				num_lines++;
				if(cnt > line_len)
					line_len = cnt;
				cnt = -1;	/* fall through */
			default:
				cnt++;
				text_len++;
				break;
		}
		/* Need this test, we can run out of characters at *any* time. */
		if(*chPtr == '\0')
			break;
		chPtr++;
	}

	/* see if everything is balanced */
	if(state_stack->next != NULL)
	{
		htmlEnum state;
		int i = 0;

		/* this is a bad html document */
		bad_html = True;
		/* and thus not HTML 3.2 conforming */
		html32 = False;

		/*
		* in very bad HTML documents, text might appear after the last
		* closing tag. For completeness, we need to flush that text also.
		* Please note that this can only happen when the stack is
		* unbalanced, and that's the reason it's in here and not outside
		* this stack test.
		*/
		if(text_len && text_start != NULL)
		{
			store_text_func(text_start, chPtr);
		}
		/* bad hack to make sure everything gets appended at the end */
		current_start_pos = strlen(text);
		current_end_pos = current_start_pos + 1;
		/* make all elements balanced */
		while(state_stack->next != NULL)
		{
			i++;
			state = popState(num_lines);
			insertElement(html_tokens[state], state, True);
		}
#ifdef VERIFY
		printf("-------------------\n");
		printf("%s: verification failed.\n", html);
		printf("HTML 3.2 conformant: No.\n");
		printf("%i element(s) remain on stack.\n", i);
		if(p_inserted)
			printf("AutoCorrect inserted %i missing elements.\n", p_inserted);
		if(p_removed)
			printf("AutoCorrect ignored %i elements.\n", p_removed);
		fflush(stdout);
#endif
	}

#ifdef VERIFY
	else
	{
		printf("-------------------\n");
		printf("%s: verified.\n", html);
		printf("HTML 3.2 conformant: %s.\n", html32 ? "Yes" : "No");
		if(p_inserted)
			printf("AutoCorrect inserted %i missing elements.\n", p_inserted);
		if(p_removed)
			printf("AutoCorrect ignored %i elements.\n", p_removed);
		fflush(stdout);
	}
#endif

	/* 
	* allow lines to have 80 chars at maximum. It's only used when
	* the XmNresizeWidth resource is true.
	*/
	max_line_len = (line_len > 80 ? 80 : line_len);

	_XmHTMLDebug(4, ("parse.c: parseHTML, allocated %i HTML elements "
		"and %i text elements (%i total).\n", list_data.num_elements, 
		list_data.num_text, list_data.num_elements + list_data.num_text));

	_XmHTMLDebug(4, ("parse.c: parseHTML, removed %i unbalanced elements\n",
		p_removed));
	_XmHTMLDebug(4, ("parse.c: parseHTML, inserted %i missing elements\n",
		p_inserted));

	_XmHTMLDebug(4, ("tokenToId statistics\nno of lookups: %i\n"
		"Average search actions: %f\n", num_lookups,
		(float)num_searches/(float)num_lookups));

#if defined(DEBUG) && !defined(VERIFY)
	if(html->html.debug_prefix)
		writeParsedOutputToFile(list_data.head->next, html->html.debug_prefix);
#endif
}

#define CUT_COMMENT { \
	int dashes = 0; \
	Boolean end_comment = False; \
	chPtr++; \
	while(!end_comment && *chPtr != '\0') \
	{ \
		switch(*chPtr) \
		{ \
			case '\n': \
				num_lines++; \
				break; \
			case '-': \
				/* comment dashes occur twice in succession */ \
				if(*(chPtr+1) == '-') \
				{ \
					chPtr++; \
					dashes+=2; \
				} \
				break; \
			case '>': \
				if(*(chPtr-1) == '-' && !(dashes % 4)) \
					end_comment = True; \
				break; \
		} \
		chPtr++; \
	} \
}

/*****
* Name: 		parsePerfectHTML
* Return Type: 	void
* Description: 	html parser; creates a doubly linked list of all elements
*				(both HTML and plain text).
* In: 
*	html:		XmHTMLWidget id
*	old_list:	previous list to be freed.
*	input:		HTML text to parse
* Returns:
*	nothing.
* Note:
*	This parser assumes the text it is parsing is ABSOLUTELY PERFECT HTML3.2.
*	No parserstack is used or created. It's only called when the mime type of
*	a document is text/html-perfect. No HTML shorttags & comments must be
*	correct. Elements of which the terminator is optional *MUST* have a
*	terminator present or bad things will happen.
*****/
#ifndef VERIFY
static void
parsePerfectHTML(XmHTMLWidget html, char *text)
{
	register char *chPtr;
	char *start, *text_start;
	int cnt = 0, text_len = 0;
	Dimension line_len = 0;
	Boolean done;

	/* text insertion function to be used */
	void (*store_text_func)(char*, char*);

	/* we assume all documents are valid ;-) */
	bad_html = False;
	/* and that every document is HTML 3.2 conforming */
	html32 = True;

#ifndef VERIFY
	strict_checking = True;
	if(html->html.string_direction == TSTRING_DIRECTION_R_TO_L)
		store_text_func = storeTextElementRtoL;
	else
		store_text_func = storeTextElement;
#else
	store_text_func = storeTextElement;
#endif

	_XmHTMLDebug(4, ("parse.c: parsePerfectHTML, start\n"));

	/* start scanning */
	start = text_start = NULL;
	text_input = chPtr = text;
	text_len = 0;
	num_lines = 1;	/* every editor starts its linecount at 1 */
	err_count = current_start_pos = current_end_pos = 0;

	while(*chPtr)
	{
		switch(*chPtr)
		{
			case '<':		/* start of element */
				/* See if we have any text pending */
				if(text_len && text_start != NULL)
				{
					store_text_func(text_start, chPtr);
					text_start = NULL;
					text_len = 0;
				}
				/* move past element starter */
				start = chPtr+1; /* element text starts here */
				done = False;
				/* absolute starting position for this element */
				current_start_pos = start - text;

				/* scan until end of this tag */
				while(*chPtr != '\0' && !done)
				{
					chPtr++;
					switch(*chPtr)
					{
						case '!':
							/* either a comment or the !doctype */
							if((*(chPtr+1) == '>' || *(chPtr+1) == '-'))
							{
								CUT_COMMENT;
								/* back up one element */
								chPtr--;
								start = chPtr;
								done = True;
							}
							break;
						case '>':
							/* go and store the element */
							chPtr = storeElementUnconditional(start, chPtr);
							done = True;
							break;
						case '\n':
							num_lines++;
							break;
						default:
							break;
					}
				}
				if(done)
					text_start = chPtr+1; /* plain text starts here */
				text_len = 0;
				start = NULL;
				break;
			case '\n':
				num_lines++;
				if(cnt > line_len)
					line_len = cnt;
				cnt = -1;	/* fall through */
			default:
				cnt++;
				text_len++;
				break;
		}
		/* Need this test, we can run out of characters at *any* time. */
		if(*chPtr == '\0')
			break;
		chPtr++;
	}

#ifdef VERIFY
	printf("-------------------\n");
	printf("%s: verified.\n", html);
	printf("HTML 3.2 conformant: Yes.\n");
	fflush(stdout);
#endif

	/* 
	* allow lines to have 80 chars at maximum. It's only used when
	* the XmNresizeWidth resource is true.
	*/
	max_line_len = (line_len > 80 ? 80 : line_len);

	_XmHTMLDebug(4, ("parse.c: parsePerfectHTML, allocated %i HTML elements "
		"and %i text elements (%i total).\n", list_data.num_elements, 
		list_data.num_text, list_data.num_elements + list_data.num_text));

	_XmHTMLDebug(4, ("tokenToId statistics\nno of lookups: %i\n"
		"Average search actions: %f\n", num_lookups,
		(float)num_searches/(float)num_lookups));
}

/*****
* Name: 		parsePLAIN
* Return Type: 	void
* Description: 	creates a parser tree for plain text.
* In: 
*	html:		XmHTMLWidget id;
*	text:		raw text to be displayed.
* Returns:
*	nothing
* Note:
* 	This routine adds html and body tags and the full text in a <pre></pre>.
*	We don't parse a single thing since it can screw up the autocorrection
*	routines when this raw text contains html commands.
*****/
static void
parsePLAIN(XmHTMLWidget html, char *text)
{
	register char *chPtr;
	int i, line_len;

	line_len = i = 0;
	num_lines = 1;	/* every editor starts its linecount at 1 */
	current_start_pos = current_end_pos = 0;

	insertElement(html_tokens[HT_HTML], HT_HTML, False);
	insertElement(html_tokens[HT_BODY], HT_BODY, False);
	insertElement(html_tokens[HT_PRE], HT_PRE, False);

	/* store the raw text */
	chPtr = text + strlen(text);

	if(html->html.string_direction == TSTRING_DIRECTION_R_TO_L)
		storeTextElementRtoL(text, chPtr);
	else
		storeTextElement(text, chPtr);

	/* count how many lines we have and get the longest line as well */
	for(chPtr = text; *chPtr != '\0'; chPtr++)
	{
		switch(*chPtr)
		{
			case '\n':
				num_lines++;
				if(i > line_len)
					line_len = i;
				i = 0;
				break;
			default:
				i++;
		}
	}

	/* add closing elements */
	insertElement(html_tokens[HT_PRE], HT_PRE, True);
	insertElement(html_tokens[HT_BODY], HT_BODY, True);
	insertElement(html_tokens[HT_HTML], HT_HTML, True);

	/* maximum line length */
	max_line_len = (line_len > 80 ? 80 : line_len);
}

static char *content_image = "<html><body><img src=\"%s\"></body></html>";

/*****
* Name: 		parseImage
* Return Type: 	void
* Description: 	creates a parser tree for the given image so XmHTML can
*				display it.
* In: 
*	html:		XmHTMLWidget id;
*	image_file:	name of image file to be loaded.
* Returns:
*	nothing.
*****/
static void
parseIMAGE(XmHTMLWidget html, char *image_file)
{
	char *input;

	input = (char*)malloc((strlen(content_image) + strlen(image_file)+1)*
		sizeof(char));
	sprintf(input, content_image, image_file);
	parseHTML(html, input);
	free(input);
}
#endif

/*****
* Name: 		parserDriver
* Return Type: 	XmHTMLObject*
* Description: 	main HTML parser driver.
* In: 
*	html:		XmHTMLWidget id;
*	old_list:	objects to be freed;
*	input:		text to parse.
* Returns:
*	a newly generated parser tree.
*****/
static XmHTMLObject *
parserDriver(XmHTMLWidget html, XmHTMLObject *old_list, String input)
{
	char *text = NULL;
	
	/* free any previous list */
	if(old_list != NULL)
	{
		_XmHTMLFreeObjects(old_list);
		old_list = NULL;
	}

	/* initialize the master list */
	initMasterObjectList();

	max_line_len = 0;
	num_lines = 1; /* text editors start line numbers at 1*/

	/* should we issue warning messages about bad HTML documents? */
#ifndef VERIFY
	bad_html_warnings = html->html.bad_html_warnings;
#else
	bad_html_warnings = (nowarn == False);
#endif

	/* reset debug counters */
#if defined(DEBUG) || defined(VERIFY)
	num_lookups = num_searches = p_inserted = p_removed = id_depth = 0;
#endif

	/* initialize the stateStack */
	RESET_STATESTACK;

	have_body = False;

	/* 
	* Initialize list data. More efficient than every conditional test
	* when an element is to be stored in the list.
	*/
	list_data.head = newElement(HT_ZTEXT, NULL, NULL, False, False);
	list_data.current = list_data.head;
	list_data.num_elements = 1;

	/* First copy the input text to a private buffer, it will get modified. */
	text = my_strndup(input, strlen(input));

	/* parse text */
#ifndef VERIFY
	if(!(strcasecmp(html->html.mime_type, "text/html")))
	{
		html->html.mime_id = XmPLC_DOCUMENT;
		parseHTML(html, text);
	}
	else if(!(strcasecmp(html->html.mime_type, "text/html-perfect")))
	{
		html->html.mime_id = XmPLC_DOCUMENT;
		parsePerfectHTML(html, text);
	}
	else if(!(strcasecmp(html->html.mime_type, "text/plain")))
	{
		html->html.mime_id = XmNONE;
		parsePLAIN(html, text);
	}
	else if(!(strncasecmp(html->html.mime_type, "image/", 6)))
	{
		html->html.mime_id = XmPLC_IMAGE;
		parseIMAGE(html, text);
	}
#else
	parseHTML(html, text);
#endif

	free(text);

	/* first element is a dummy one, so remove it */
	list_data.current = list_data.head->next;

	/* sanity */
	if(list_data.current)
		list_data.current->prev = NULL;
	free(list_data.head);
	list_data.head = NULL;
	return(list_data.current);
}

/*****
* Name:			checkOccurance
* Return Type:	Boolean
* Description:	checks whether the appearence of the current token is 
*				allowed in the current parser state.
* In: 
*	current:	HTML token to check
*	state:		parser state
* Returns:
*	When current is not allowed, the id of the element that should be
*	preceeding this one. If no suitable preceeding element can be deduced,
*	it returns -1. When the element is allowed, HT_ZTEXT is returned.
*****/
static int
checkOccurance(htmlEnum current, htmlEnum state)
{
	stateStack *curr;

	switch(current)
	{
		case HT_DOCTYPE:
			return((int)HT_ZTEXT); /* always allowed */

		case HT_HTML:
			if(state == HT_DOCTYPE)
				return((int)HT_ZTEXT);
			return(-1);

		case HT_BODY:
			if(state == HT_HTML || state == HT_FRAMESET)
				return((int)HT_ZTEXT);
			else
			{
				/* try and guess an appropriate return value */
				if(state == HT_HEAD)
					return((int)HT_HEAD);
				else
					return((int)HT_HTML);
			}
			return(-1);	/* not reached */

		case HT_HEAD:
		case HT_FRAMESET:
			/* frames may be nested */
			if(state == HT_HTML || state == HT_FRAMESET)
				return((int)HT_ZTEXT);
			else
				return((int)HT_HTML); /* obvious */
			break;

		case HT_NOFRAMES:
			if(state == HT_HTML)
				return((int)HT_ZTEXT);
			else
				return((int)HT_HTML); /* obvious */
			break;

		case HT_FRAME:
			if(state == HT_FRAMESET)
				return((int)HT_ZTEXT);
			else
				return((int)HT_FRAMESET); /* obvious */
			break;

		case HT_BASE:
		case HT_ISINDEX:
		case HT_META:
		case HT_LINK:
		case HT_SCRIPT:
		case HT_STYLE:
		case HT_TITLE:
			if(state == HT_HEAD)
				return((int)HT_ZTEXT); /* only allowed in the <HEAD> section */
			else
				return((int)HT_HEAD); /* obvious */
			break;

		case HT_IMG:
			if(state == HT_PRE)
			{
				/* strictly speaking, images are not allowed inside <pre> */
				if(!strict_checking)
					parserCallback(current, state, HTML_VIOLATION);
				else
					return(-1);
			}
			if(IS_CONTAINER(state) || IS_MARKUP(state) || IS_MISC(state))
				return((int)HT_ZTEXT);
			else
				return(-1); /* too bad, obliterate it */

		case HT_A:
			if(state == HT_A)
				return(-1); /* no nested anchors */
			/* fall thru, all these elements may occur in the given context */
		case HT_APPLET:
		case HT_B:
		case HT_BASEFONT:
		case HT_BIG:
		case HT_BR:
		case HT_CITE:
		case HT_CODE:
		case HT_DFN:
		case HT_EM:
		case HT_FONT:
		case HT_I:
		case HT_INPUT:
		case HT_KBD:
		case HT_MAP:
		case HT_SMALL:
		case HT_SAMP:
		case HT_SELECT:
		case HT_STRIKE:
		case HT_STRONG:
		case HT_SUB:
		case HT_SUP:
		case HT_TAB:
		case HT_TEXTAREA:
		case HT_TT:
		case HT_U:
		case HT_VAR:
			if(IS_CONTAINER(state) || IS_MARKUP(state) || IS_MISC(state))
				return((int)HT_ZTEXT);
			else
				return(-1); /* too bad, obliterate it */

		case HT_ZTEXT:
				return(HT_ZTEXT);  /* always allowed */

		case HT_AREA:		/* only allowed when inside a <MAP> */
			if(state == HT_MAP)
				return((int)HT_ZTEXT);
			else
				return((int)HT_MAP); /* obvious */
			break;

		case HT_P: 
			if(state == HT_ADDRESS || IS_CONTAINER(state))
				return((int)HT_ZTEXT);
			/* guess a proper return value */
			switch(state)
			{
				case HT_OL:
				case HT_UL:
				case HT_DIR:
				case HT_MENU:
					return((int)HT_LI);
				case HT_TABLE:
					return((int)HT_TR);
				case HT_DL:
				default:
					return(-1); /* too bad, obliterate it */
			}
			return(-1);	/* not reached */

		case HT_FORM:
			if(state == HT_FORM)
				return(-1); /* no nested forms */
			/* fall thru */
		case HT_ADDRESS:
		case HT_BLOCKQUOTE:
		case HT_CENTER:
		case HT_DIV:
		case HT_H1:
		case HT_H2:
		case HT_H3:
		case HT_H4:
		case HT_H5:
		case HT_H6:
		case HT_HR:
		case HT_TABLE:
		case HT_DIR:
		case HT_MENU:
		case HT_DL:
		case HT_PRE:
		case HT_OL:
		case HT_UL:
			if(IS_CONTAINER(state))
				return((int)HT_ZTEXT);
			/* correct for most common errors */
			switch(state)
			{
				case HT_OL:
				case HT_UL:
				case HT_DIR:
				case HT_MENU:
					return((int)HT_LI);
				case HT_TABLE:
					return((int)HT_TR);
				case HT_DL:
				default:
					/*
					* Almost everyone ignores the fact that horizontal
					* rules may *only* occur in container elements and
					* nowhere else. We can safely loosen this when we are
					* told not to be strict as it is a single element.
					*/
					if(current == HT_HR && !strict_checking)
					{
						parserCallback(current, state, HTML_VIOLATION);
						return(HT_ZTEXT);
					}
					return(-1); /* too bad, obliterate it */
			}
			return(-1);	/* not reached */

		case HT_LI:
			if(state == HT_UL || state == HT_OL || state == HT_DIR || 
				state == HT_MENU)
				return((int)HT_ZTEXT);
			/*
			* Guess a return value: walk the current parser state and
			* see if a list is already present. If it's not, return HT_UL,
			* else return -1.
			*/
			for(curr = state_stack; curr->next != NULL; curr = curr->next)
			{
				if(curr->id == HT_UL || curr->id == HT_OL ||
					curr->id == HT_DIR || curr->id == HT_MENU)
					return(-1);
			}
			return((int)HT_UL); /* start a new list */

		case HT_DT:
		case HT_DD:
			if(state == HT_DL)
				return((int)HT_ZTEXT);
			return(onStack(HT_DL) ? -1 : (int)HT_DL);

		case HT_OPTION:		/* Only inside the SELECT element */
			if(state == HT_SELECT)
				return((int)HT_ZTEXT);
			else
				return((int)HT_SELECT); /* obvious */
			break;

		case HT_CAPTION: /* Only allowed in TABLE */
		case HT_TR:
			if(state == HT_TABLE)
				return((int)HT_ZTEXT);
			/* no smart guessing here, it completely fucks things up */
			return(-1);

		case HT_TD:
		case HT_TH:
			/* Only allowed when in a table row */
			if(state == HT_TR)
				return((int)HT_ZTEXT);
			/* nested cells are not allowed, so insert another row */
			if(state == current)
				return(HT_TR);
			/* final check: insert a row when one is not present on the stack */
			return(onStack(HT_TR) ? -1 : (int)HT_TR);

		case HT_PARAM: /* Only allowed in applets */
			if(state == HT_APPLET)
				return((int)HT_ZTEXT);
			else
				return((int)HT_APPLET); /* obvious */
			break;
		/* no default so w'll get a warning when we miss anything */
	}
	return(-1);
}

/*****
* Name:			checkContent
* Return Type:	Boolean
* Description:	checks whether the appearence of the current token is valid in
*				the current state.
* In: 
*	current:	token to check
*	state:		current state of the parser
* Returns:
*	True if the current token is in a valid state. False otherwise.
*****/
static Boolean
checkContent(htmlEnum current, htmlEnum state)
{
	/* plain text is always allowed */
	if(current == HT_ZTEXT)
		return(True);

	switch(state)
	{
		case HT_DOCTYPE:
			return(True);

		case HT_HTML:
			if(current == HT_HTML || current == HT_BODY ||
				current == HT_HEAD || current == HT_FRAMESET)
				return(True);
			break;

		case HT_FRAMESET:
			if(current == HT_FRAME || current == HT_FRAMESET)
				return(True);
			break;

		case HT_HEAD:
			if(current == HT_TITLE ||
				current == HT_ISINDEX || 
				current == HT_BASE ||
				current == HT_SCRIPT ||
				current == HT_STYLE ||
				current == HT_META ||
				current == HT_LINK)
				return(True);
			break;

		case HT_NOFRAMES:
		case HT_BODY:
			if(current == HT_A ||
				current == HT_ADDRESS ||
				current == HT_APPLET ||
				current == HT_B ||
				current == HT_BIG ||
				current == HT_BLOCKQUOTE ||
				current == HT_BR ||
				current == HT_CENTER ||
				current == HT_CITE ||
				current == HT_CODE ||
				current == HT_DFN ||
				current == HT_DIR ||
				current == HT_DIV ||
				current == HT_DL ||
				current == HT_EM ||
				current == HT_FONT ||
				current == HT_FORM ||
				current == HT_NOFRAMES ||
				current == HT_H1 ||
				current == HT_H2 ||
				current == HT_H3 ||
				current == HT_H4 ||
				current == HT_H5 ||
				current == HT_H6 ||
				current == HT_HR ||
				current == HT_I ||
				current == HT_IMG ||
				current == HT_INPUT ||
				current == HT_KBD ||
				current == HT_MAP ||
				current == HT_MENU ||
				current == HT_OL ||
				current == HT_P ||
				current == HT_PRE ||
				current == HT_SAMP ||
				current == HT_SELECT ||
				current == HT_SMALL ||
				current == HT_STRIKE ||
				current == HT_STRONG ||
				current == HT_SUB ||
				current == HT_SUP ||
				current == HT_TABLE ||
				current == HT_TEXTAREA ||
				current == HT_TT ||
				current == HT_U ||
				current == HT_UL ||
				current == HT_VAR ||
				current == HT_ZTEXT)
				return(True);
			break;

		case HT_ADDRESS:
			if(current == HT_P)
				return(True);
			/* fall thru, these elements are also allowed */
		case HT_A:
		case HT_B:
		case HT_BIG:
		case HT_CAPTION:
		case HT_CITE:
		case HT_CODE:
		case HT_DFN:
		case HT_DT:
		case HT_EM:
		case HT_FONT:
		case HT_H1:
		case HT_H2:
		case HT_H3:
		case HT_H4:
		case HT_H5:
		case HT_H6:
		case HT_I:
		case HT_KBD:
		case HT_P:
		case HT_SAMP:
		case HT_SMALL:
		case HT_STRIKE:
		case HT_STRONG:
		case HT_SUB:
		case HT_SUP:
		case HT_TAB:
		case HT_TT:
		case HT_U:
		case HT_VAR:
			if(current == HT_A ||
				current == HT_APPLET ||
				current == HT_B ||
				current == HT_BIG ||
				current == HT_BR ||
				current == HT_CITE ||
				current == HT_CODE ||
				current == HT_DFN ||
				current == HT_EM ||
				current == HT_FONT ||
				current == HT_I ||
				current == HT_IMG ||
				current == HT_INPUT ||
				current == HT_KBD ||
				current == HT_MAP ||
				current == HT_NOFRAMES ||
				current == HT_SAMP ||
				current == HT_SCRIPT ||
				current == HT_SELECT ||
				current == HT_SMALL ||
				current == HT_STRIKE ||
				current == HT_STRONG ||
				current == HT_SUB ||
				current == HT_SUP ||
				current == HT_TEXTAREA ||
				current == HT_TT ||
				current == HT_U ||
				current == HT_VAR ||
				current == HT_ZTEXT)
				return(True);
			break;

		case HT_APPLET:
			if(current == HT_A ||
				current == HT_APPLET ||
				current == HT_B ||
				current == HT_BIG ||
				current == HT_BR ||
				current == HT_CITE ||
				current == HT_CODE ||
				current == HT_DFN ||
				current == HT_EM ||
				current == HT_FONT ||
				current == HT_I ||
				current == HT_IMG ||
				current == HT_INPUT ||
				current == HT_KBD ||
				current == HT_MAP ||
				current == HT_NOFRAMES ||
				current == HT_PARAM ||
				current == HT_SAMP ||
				current == HT_SCRIPT ||
				current == HT_SELECT ||
				current == HT_SMALL ||
				current == HT_STRIKE ||
				current == HT_STRONG ||
				current == HT_SUB ||
				current == HT_SUP ||
				current == HT_TEXTAREA ||
				current == HT_TT ||
				current == HT_U ||
				current == HT_VAR ||
				current == HT_ZTEXT)
				return(True);
			break;

		case HT_MAP:
			if(current == HT_AREA)
				return(True);
			break;

		case HT_AREA:		/* only allowed when inside a <MAP> */
			if(state == HT_MAP)
				return(True);
			break;

		/* unterminated tags that may not contain anything */
		case HT_BASE:
		case HT_BR:
		case HT_HR:
		case HT_IMG:
		case HT_INPUT:
		case HT_ISINDEX:
		case HT_LINK:
		case HT_META:
		case HT_PARAM:
			return(True);

		case HT_OPTION:
		case HT_SCRIPT:
		case HT_STYLE:
		case HT_TEXTAREA:
		case HT_TITLE:
			if(current == HT_ZTEXT)
				return(True);
			break;

		case HT_FRAME:
			if(current == HT_FRAMESET)
				return(True);
			break;
		case HT_SELECT:
			if(current == HT_OPTION)
				return(True);
			break;

		case HT_TABLE:
			if(current == HT_CAPTION ||
				current == HT_TR)
				return(True);
			break;

		case HT_TR:
			if(current == HT_TH ||
				current == HT_TD)
				return(True);
			break;

		case HT_DIR:
		case HT_MENU:
		case HT_OL:
		case HT_UL:
			if(current == HT_LI)
				return(True);
			break;

		case HT_DL:
			if(current == HT_DT ||
				current == HT_DD)
				return(True);
			break;

		case HT_FORM:
			/* nested forms are not allowed */
			if(current == HT_FORM)
				return(False);
			/* fall thru */
		case HT_BLOCKQUOTE:
		case HT_CENTER:
		case HT_DIV:
		case HT_TD:
		case HT_TH:
			if(current == HT_H1 ||
				current == HT_H2 ||
				current == HT_H3 ||
				current == HT_H4 ||
				current == HT_H5 ||
				current == HT_H6 ||
				current == HT_ADDRESS)
				return(True);
			/* fall thru */
		case HT_LI:
		case HT_DD:
			if(current == HT_A ||
				current == HT_APPLET ||
				current == HT_B ||
				current == HT_BIG ||
				current == HT_BLOCKQUOTE ||
				current == HT_BR ||
				current == HT_CENTER ||
				current == HT_CITE ||
				current == HT_CODE ||
				current == HT_DFN ||
				current == HT_DIR ||
				current == HT_DIV ||
				current == HT_DL ||
				current == HT_EM ||
				current == HT_FONT ||
				current == HT_FORM ||
				current == HT_HR ||
				current == HT_I ||
				current == HT_IMG ||
				current == HT_INPUT ||
				current == HT_KBD ||
				current == HT_MAP ||
				current == HT_MENU ||
				current == HT_NOFRAMES ||
				current == HT_OL ||
				current == HT_P ||
				current == HT_PRE ||
				current == HT_SAMP ||
				current == HT_SCRIPT ||
				current == HT_SELECT ||
				current == HT_SMALL ||
				current == HT_STRIKE ||
				current == HT_STRONG ||
				current == HT_SUB ||
				current == HT_SUP ||
				current == HT_TABLE ||
				current == HT_TEXTAREA ||
				current == HT_TT ||
				current == HT_U ||
				current == HT_UL ||
				current == HT_VAR ||
				current == HT_ZTEXT)
				return(True);
			break;

		case HT_PRE:
			if(current == HT_A ||
				current == HT_APPLET ||
				current == HT_B ||
				current == HT_BR ||
				current == HT_CITE ||
				current == HT_CODE ||
				current == HT_DFN ||
				current == HT_EM ||
				current == HT_I ||
				current == HT_INPUT ||
				current == HT_KBD ||
				current == HT_MAP ||
				current == HT_NOFRAMES ||
				current == HT_SAMP ||
				current == HT_SCRIPT ||
				current == HT_SELECT ||
				current == HT_STRIKE ||
				current == HT_STRONG ||
				current == HT_TEXTAREA ||
				current == HT_TT ||
				current == HT_U ||
				current == HT_VAR ||
				current == HT_FONT ||
				current == HT_ZTEXT)
				return(True);
			break;

		case HT_ZTEXT:
			return(True); /* always allowed */

		/* elements of which we don't know any state information */
		case HT_BASEFONT:
			return(True);

		/* no default so w'll get a warning when we miss anything */
	}
	return(False);
}

/********
****** Public Functions
********/

/*****
* Name: 		_XmHTMLparseHTML
* Return Type: 	XmHTMLObject*
* Description: 	html parser driver
* In: 
*	html:		XmHTMLWidget id
*	old_list:	previous list to be freed.
*	input:		HTML text to parse
*	dest:		destination widget id, if any
* Returns:
*	nothing when standalone, parsed list of objects otherwise.
*****/
XmHTMLObject*
_XmHTMLparseHTML(XmHTMLWidget html, XmHTMLObject *old_list, char *input,
	XmHTMLWidget dest)
{
	static XmHTMLObject *output;
	XmHTMLObject *prev_output, *checked;
	String text;
	int loop_count = 0;
	Boolean redo = False;

	widget = (TWidget)html;
	html_widget = html;

	/* free any previous list */
	if(old_list != NULL)
	{
		_XmHTMLFreeObjects(old_list);
		old_list = NULL;
	}

	/* sanity check */
	if(input == NULL || *input == '\0')
		return(NULL);

	prev_output = old_list;
	text = input;

#ifndef VERIFY
	/*
	* block setValues while we are parsing. This is required as a large
	* number of resources can initialize a recomputation of everything.
	* This *can* be rather funest since we are currently parsing the text
	* that is to be formatted!
	* (the least effects would be a double call to the formatter and
	*  a visible flickering of the screen).
	*/
	if(dest) dest->html.in_layout = True;
#endif

	do
	{
		_XmHTMLDebug(4, ("parse.c, _XmHTMLparseHTML, doing pass %i\n",
			loop_count)); 

		checked = NULL;
		redo = False;

		output = parserDriver(html, prev_output, text);

		/* sanity */
		if(output == NULL)
		{
			/* need to free parser output also */
			if(loop_count)
				free(text);
#ifndef VERIFY
			/* release SetValues lock */
			if(dest) dest->html.in_layout = True;
#endif
			return(NULL);
		}

		/*
		* If the state stack was unbalanced, check if the verification/repair
		* routines produced a balanced parser tree.
		*/
		if(bad_html)
			checked = verifyVerification(output);

		/*
		* If we have a document callback, call it now. If we don't have one
		* and verifyVerification failed, we iterate again on the
		* current document.
		* The verify stuff mimics the default DocumentCallback behaviour,
		* that is, advise another pass if the parser tree is unbalanced.
		* The reason for this is that, although a document may have yielded
		* an unbalanced state stack, the parser tree *is* properly balanced
		* and hence suitable for usage.
		*
		* Additional note, 08/15/97: we only have to call an installed
		* documentCallback if dest is non-NULL, in which case we have
		* been called to parse new text. If dest *is* NULL we have been
		* called to parse something else (for now only externally installed
		* imageMaps can cause this).
		*/
#ifndef VERIFY
		if(html->html.document_callback && dest)
		{
			if(loop_count)
				free(text);
			text = NULL;

			dest->html.elements = output;

			redo = _XmHTMLDocumentCallback(html, html32, !bad_html,
				checked == NULL, False, loop_count);

			/*
			* We have been requested to do another pass on the current
			* document.
			*/
			if(redo)
			{
				/* save ptr to current parser output */
				prev_output = output;

				/* pull new source out of the parser */
				text = _XmHTMLTextGetString(output);
			}
		}
		/* no document callback, do one iteration using parser tree output */
		else
		{
			/* no document callback, so we can never have a new source */
			if(loop_count)
				free(text);
			text = NULL;
			redo = False;

			/* only one additional loop if parser tree wasn't balanced */
			if((loop_count < 2 && checked != NULL))
			{
				/* save ptr to current parser output */
				prev_output = output;
				redo = True;
				text = _XmHTMLTextGetString(output);
			}
		}
#else
		if((loop_count < 4 && checked != NULL))
		{
			redo = True;
			if(loop_count)
				free(text);
			text = _XmHTMLTextGetString(output);
			prev_output = output;
		}
#endif
		loop_count++;
	}
	while(redo);

	/*
	* Free parser output if loop_count is larger than one and we did not
	* receive a new source. text check required so we don't crash if no new
	* source has been provided.
	*/
	if(loop_count > 1 && text != NULL)	/* fix 06/18/97-01, dp */
		free(text);

#ifdef VERIFY
	if(loop_count == 5 && (bad_html || checked != NULL))
		fprintf(stderr, "Terrible HTML document: AutoCorrect failed after 5 "
			"passes!\n");
#else
	/* release SetValues lock & transfer mimetype */
	if(dest)
	{
		dest->html.in_layout = True;
		dest->html.mime_id = html->html.mime_id;
	}
#endif

	return(output);
}

/*****
* Name: 		_XmHTMLTextGetString
* Return Type: 	String
* Description: 	creates a HTML source document from the given parser tree.
* In: 
*	objects:	parser tree.
* Returns:
*	created document in a buffer. This buffer must be freed by the caller.
*****/
String
_XmHTMLTextGetString(XmHTMLObject *objects)
{
	XmHTMLObject *tmp;
	static String buffer;
	String chPtr;
	int i, size = 0;
	int *sizes;	/* for storing element lengths */

	if(objects == NULL)
		return(NULL);

	sizes = (int*)malloc((int)HT_ZTEXT*sizeof(int));

	for(i = 0; i < (int)HT_ZTEXT; i++)
		sizes[i] = strlen(html_tokens[(htmlEnum)i]);

	/* first pass, compute length of buffer to allocate */
	for(tmp = objects; tmp != NULL; tmp = tmp->next)
	{
#ifdef VERIFY
		if(tmp->ignore)
			continue;
#endif
		if(tmp->id != HT_ZTEXT)
		{
			if(tmp->is_end)
				size += 1;	/* a / */

			/* a pair of <> + element length */
			size += 2 + sizes[(int)tmp->id];

			/* a space and the attributes */
			if(tmp->attributes)
				size += 1 + strlen(tmp->attributes);
		}
		else 
			size += strlen(tmp->element);
	}
	size +=1;	/* terminating character */

	buffer = (String)malloc(size * sizeof(char));
	chPtr = buffer;

	/* second pass, compose the text */
	for(tmp = objects; tmp != NULL; tmp = tmp->next)
	{
#ifdef VERIFY
		if(tmp->ignore)
			continue;
#endif
		if(tmp->id != HT_ZTEXT)
		{
			*chPtr++ = '<';
			if(tmp->is_end)
				*chPtr++ = '/';
			strcpy(chPtr, html_tokens[tmp->id]);
			chPtr += sizes[(int)tmp->id];

			/* a space and the attributes */
			if(tmp->attributes)
			{
				*chPtr++ = ' ';
				strcpy(chPtr, tmp->attributes);
				chPtr += strlen(tmp->attributes);
			}
			*chPtr++ = '>';
		}
		else 
		{
			strcpy(chPtr, tmp->element);
			chPtr += strlen(tmp->element);
		}
	}
	*chPtr = '\0';	/* NULL terminate */
	free(sizes);	/* no longer needed */
	return(buffer);
}

/*****
* Name: 		_XmHTMLExpandEscapes
* Return Type: 	void
* Description: 	replaces character escapes sequences with the appropriate char.
* In: 
*	string:		text to scan for escape sequences
* Returns:
*	nothing
*****/
void
_XmHTMLExpandEscapes(char *string, Boolean warn)
{
	register char *chPtr = string;
	char escape;	/* value of escape character */

	/* scan the entire text in search of escape codes (yuck) */
	while(*string)	/* fix 02/26/97-02, dp */
	{
		switch(*string)
		{
			case '&':
				if((escape = tokenToEscape(&string, warn)) != 0)
					*chPtr++ = escape;
				break;
			default:
				{
					/* keep current start position up to date as well */
					current_start_pos++;
					*(chPtr++) = *(string++);
				}
		}
		if(*string == 0)
		{
			*chPtr = '\0';	/* NULL terminate */
			return;
		}
	}
}

/*****
* Name: 		_XmHTMLFreeObjects
* Return Type: 	void
* Description: 	releases all memory occupied by the parsed list of objects.
* In: 
*	objects:	object list to be destroyed
* Returns:
*	nothing.
*****/
void
_XmHTMLFreeObjects(XmHTMLObject *objects)
{
	XmHTMLObject *temp;
#ifdef DEBUG
	int i = 0;
#endif

	/* free all parsed objects */
	while(objects)
	{
		temp = objects->next;
		/* sanity check. Should not be needed anyway. */
		if(objects->element)
			free(objects->element);
		free(objects);
		objects = temp;
#ifdef DEBUG
		i++;
#endif
	}
	objects = NULL;

	_XmHTMLDebug(4, ("parse.c: _XmHTMLFreeObjects, freed %i elements\n", i));
}

/*****
* Name: 		_XmHTMLTagCheck
* Return Type: 	Boolean
* Description: 	checks whether the given tag exists in the attributes of a 
*				HTML element
* In: 
*	attributes:	attributes from an HTML element
*	tag:		tag to look for.
* Returns:
*	True if tag is found, False otherwise.
*****/
Boolean 
_XmHTMLTagCheck(char *attributes, char *tag)
{
	char *chPtr, *start;

	/* sanity check */
	if(attributes == NULL)
		return(False);

	if((chPtr = my_strcasestr(attributes, tag)) != NULL)
	{
		/* see if this is a valid tag: it must be preceeded with whitespace. */
		while(*(chPtr-1) && !isspace(*(chPtr-1)))
		{
			start = chPtr+strlen(tag); /* start right after this element */
			if((chPtr = my_strcasestr(start, tag)) == NULL)
				return(False);
		}
		if(chPtr)
			return(True);
		else
			return(False);
	}
	return(False);
}

/*****
* Name: 		_XmHTMLTagGetValue
* Return Type: 	char *
* Description: 	looks for the specified tag in the given list of attributes.
* In: 
*	attributes:	attributes from an HTML element
*	tag:		tag to look for.
* Returns:
*	if tag exists, the value of this tag, NULL otherwise. 
*	return value is always malloc'd; caller must free it.
*****/
String
_XmHTMLTagGetValue(char *attributes, char *tag)
{
	static char *buf;
	char *chPtr, *start, *end;

	if(attributes == NULL || tag == NULL)	/* sanity check */
		return(NULL);

	_XmHTMLDebug(4, ("parse.c: _XmHTMLTagGetValue, attributes: %s, tag %s\n", 
		attributes, tag));

	if((chPtr = my_strcasestr(attributes, tag)) != NULL)
	{
		/* 
		* check if the ptr obtained is correct, eg, no whitespace before it. 
		* If this is not the case, get the next match.
		* Need to do this since a single my_strcasestr on, for example, align 
		* will match both align _and_ valign.
		*/
		while(*(chPtr-1) && !isspace(*(chPtr-1)))
		{
			start = chPtr+strlen(tag); /* start right after this element */
			if((chPtr = my_strcasestr(start, tag)) == NULL)
				return(NULL);
		}
		if(chPtr == NULL)
			return(NULL);
		
		start = chPtr+strlen(tag); /* start right after this element */
		/* remove leading spaces */
		while(isspace(*start))
			start++;

		/* if no '=', return NULL */
		if(*start != '=')
		{
			_XmHTMLDebug(4, ("parse.c: _XmHTMLTagGetValue, tag has no "
				"= sign.\n"));
			return(NULL);
		}

		start++;	/* move past the '=' char */

		/* remove more spaces */
		while(*start != '\0' && isspace(*start))
			start++;

		/* sanity check */
		if(*start == '\0')
		{
#ifdef PEDANTIC
			_XmHTMLWarning(__WFUNC__(NULL, "_XmHTMLTagGetValue"), 
				"tag %s has no value.", tag);
#endif /* PEDANTIC */
			return(NULL);
		}

		/* unquoted tag values are treated differently */
		if(*start != '\"')
		{
			for(end = start; !(isspace(*end)) && *end != '\0' ; end++);
		}
		else
		{
			start++;
			for(end = start; *end != '\"' && *end != '\0' ; end++);
		}
		/* empty string */
		if(end == start) 
			return(NULL);

		buf = my_strndup(start, end - start);

		_XmHTMLDebug(4, ("parse.c: _XmHTMLTagGetValue, returning %s\n", buf));

		return(buf);
	}
	return(NULL);
}

/*****
* Name: 		_XmHTMLTagGetNumber
* Return Type: 	int
* Description: 	retrieves the numerical value of the given tag.
* In: 
*	attributes:	attributes from an HTML element
*	tag:		tag to look for.
*	def:		default value if tag is not found.
* Returns:
*	if tag exists, the value of this tag, def otherwise
*****/
int
_XmHTMLTagGetNumber(char *attributes, char *tag, int def)
{
	char *chPtr;
	int ret_val = def;

	if((chPtr = _XmHTMLTagGetValue(attributes, tag)) != NULL)
	{
		ret_val = atoi(chPtr);
		_XmHTMLDebug(4, ("parse.c: _XmHTMLTagGetNumber, value for tag %s "
			"is %i\n",
			tag, ret_val));
		free(chPtr);
	}
	return(ret_val);
}

/*****
* Name: 		_XmHTMLTagCheckValue
* Return Type: 	Boolean
* Description: 	checks whether the specified tag in the given list of attributes
*				has a certain value.
* In: 
*	attributes:	attributes from an HTML element
*	tag:		tag to look for.
*	check:		value to check.
* Returns:
*	returns True if tag exists and has the correct value, False otherwise.
*****/
Boolean 
_XmHTMLTagCheckValue(char *attributes, char *tag, char *check)
{
	char *buf;

	_XmHTMLDebug(4, ("parse.c: _XmHTMLTagCheckValue: tag %s, check %s\n", 
		tag, check));

	/* no sanity check, TagGetValue returns NULL if attributes is empty */

	if((buf = _XmHTMLTagGetValue(attributes, tag)) == NULL || 
		strcasecmp(buf, check))
	{
		if(buf != NULL)
			free(buf);
		return(False);
	}
	free(buf);		/* fix 12-21-96-01, kdh */
	return(True);
}

/*****
* Name: 		_XmHTMLGetImageAlignment
* Return Type: 	Alignment
* Description: 	returns any specified image alignment
* In: 
*	attributes:	<IMG> attributes
* Returns:
*	specified image alignment. If none found, XmVALIGN_BOTTOM
*****/
Alignment
_XmHTMLGetImageAlignment(char *attributes)
{
	char *buf;
	Alignment ret_val = XmVALIGN_BOTTOM;

	/* First check if this tag does exist */
	if((buf = _XmHTMLTagGetValue(attributes, "align")) == NULL)
		return(ret_val);

	/* transform to lowercase */
	my_locase(buf);

	if(!(strcmp(buf, "left")))
		ret_val = XmHALIGN_LEFT;
	else if(!(strcmp(buf, "right")))
		ret_val = XmHALIGN_RIGHT;
	else if(!(strcmp(buf, "top")))
		ret_val = XmVALIGN_TOP;
	else if(!(strcmp(buf, "middle")))
		ret_val = XmVALIGN_MIDDLE;
	else if(!(strcmp(buf, "bottom")))
		ret_val = XmVALIGN_BOTTOM;
	else if(!(strcmp(buf, "baseline")))
		ret_val = XmVALIGN_BASELINE;

	free(buf);	/* fix 01/12/97-01; kdh */
	return(ret_val);
}

/*****
* Name: 		_XmHTMLGetHorizontalAlignment
* Return Type: 	Alignment
* Description:	Retrieve the value of the ALIGN attribute
* In: 
*	attributes:	attributes to check for the ALIGN tag
*	def_align:	default alignment.
* Returns:
*	selected ALIGN enumeration type or def_align if no match is found.
*****/
Alignment 
_XmHTMLGetHorizontalAlignment(char *attributes, Alignment def_align)
{
	char *buf;
	Alignment ret_val = def_align;

	/* First check if this tag does exist */
	if((buf = _XmHTMLTagGetValue(attributes, "align")) == NULL)
		return(ret_val);

	/* transform to lowercase */
	my_locase(buf);

	if(!(strcmp(buf, "center")))
		ret_val = XmHALIGN_CENTER;
	else if(!(strcmp(buf, "right")))
		ret_val = XmHALIGN_RIGHT;
	else if(!(strcmp(buf, "outline")))
		ret_val = XmHALIGN_OUTLINE;
	else if(!(strcmp(buf, "left")))
		ret_val = XmHALIGN_LEFT;

	free(buf);	/* fix 01/12/97-01; kdh */
	return(ret_val);
}

/*****
* Name: 		_XmHTMLGetVerticalAlignment
* Return Type: 	Alignment
* Description:	Retrieve the value of the VALIGN attribute
* In: 
*	attributes:	attributes to check for the VALIGN tag
* Returns:
*	selected VALIGN enumeration type or XmVALIGN_TOP when no valign tag 
*	is found among the element's attributes.
*****/
Alignment 
_XmHTMLGetVerticalAlignment(char *attributes)
{
	char *buf;
	Alignment ret_val = XmVALIGN_BOTTOM;

	/* First check if this tag does exist */
	if((buf = _XmHTMLTagGetValue(attributes, "valign")) == NULL)
		return(ret_val);

	if(!(strcmp(buf, "top")))
		ret_val = XmVALIGN_TOP;
	else if(!(strcmp(buf, "middle")))
		ret_val = XmVALIGN_MIDDLE;
	else if(!(strcmp(buf, "bottom")))
		ret_val = XmVALIGN_BOTTOM;
	else if(!(strcmp(buf, "baseline")))
		ret_val = XmVALIGN_BASELINE;

	free(buf);		/* fix 01/12/97-02; kdh */
	return(ret_val);
}

/*****
* Name: 		_XmHTMLGetMaxLineLength
* Return Type: 	Dimension
* Description: 	returns and estimated guess on how wide the formatted document
*				will be.
* In: 
*	html:		XmHTMLWidget id
* Returns:
*	guess what?
*****/
Dimension
_XmHTMLGetMaxLineLength(XmHTMLWidget html)
{
#ifdef VERIFY
	return(7*max_line_len);
#else
	Dimension max = 0, ret_val;

	/*
	* If no max_line_len, assume a line with 80 chars as maximum.
	*/
	if(max_line_len == 0)
		max_line_len = 80;

	/* assume an average width of 7 pixels per character */
	ret_val = 7*max_line_len;

	/* we allow widths up to 75% of the screen width */
	max = (Dimension)(0.75*Toolkit_Screen_Width (html));

	ret_val = (ret_val > max ? max : ret_val); 

	_XmHTMLDebug(4, ("parse.c: _XmHTMLGetMaxLineLength, returning %d\n",
		ret_val));

	return(ret_val);
#endif
}

/***************************************************************************** 
 *                                                                           * 
 *                           Standalone parser.                              * 
 *                                                                           * 
 *****************************************************************************/

#ifdef VERIFY

static void
writeHTMLOutputToFile(XmHTMLObject *objects, String prefix)
{
	XmHTMLObject *tmp;
	char name[1024];
	FILE *file;
	static int count;

	/* 
	* No buffer overrun check here. If this sigsegv's, its your own fault.
	* Don't use names longer than 1024 bytes then.
	*/
	sprintf(name, "%s.%i.html", prefix, count);
	count++;

	if((file = fopen(name, "w")) == NULL)
	{
		perror(name);
		return;
	}

	for(tmp = objects; tmp != NULL; tmp = tmp->next)
	{
		if(tmp->id != HT_ZTEXT)
		{
			if(!tmp->ignore)
			{
				if(tmp->is_end)
					fprintf(file, "</%s", html_tokens[tmp->id]); 
				else
					fprintf(file, "<%s", html_tokens[tmp->id]); 

				if(tmp->attributes)
					fprintf(file, " %s", tmp->attributes);
				fputs(">", file);
				/* add a newline to auto inserted elements */
				if(tmp->auto_insert == 1 || notext)
					fputs("\n", file);
			}
		}
		else if(!notext)
			fprintf(file, "%s", tmp->element); 
	}
	fputs("\n", file);
	fclose(file);
	fprintf(stderr, "parse.c: HTML parser output written to %s\n", name);
}

int
main(int argc, char **argv)
{
	FILE *fp;
	int i, len, secs, usecs;
	char *buffer, *use_file = NULL;
	XmHTMLObject *parsed_elements;
	struct timeval ts, te;
	Boolean write_files = True;

	if(argc == 1)
	{
		fprintf(stderr, "Usage: %s <html-doc> [options]\n", argv[0]);
		fprintf(stderr, "\nOptions:\n");
		fprintf(stderr, "\t-debug   : show debug information (mostly parser "
			"states).\n");
		fprintf(stderr, "\t-notext  : only output HTML tags in output file.\n");
		fprintf(stderr, "\t-nowarn  : disable bad HTML warnings. Only shows "
			"parser statistics.\n");
		fprintf(stderr, "\t-nowrite : don't generate any output files.\n");
		fprintf(stderr, "\t-nostrict: loosen HTML 3.2 checking.\n");
		fprintf(stderr, "\nOutput file prefix is parsed_output.\n");
		exit(EXIT_FAILURE);
	}

	/* default settings */
	notext          = False;
	nowarn          = False;
	write_files     = True;
	strict_checking = True;
	debug           = False;

	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			switch(argv[i][1])
			{
				case 'n':
					if(!strcmp(argv[i], "-notext"))
						notext = True;
					else if(!strcmp(argv[i], "-nowarn"))
						nowarn = True;
					else if(!strcmp(argv[i], "-nowrite"))
						write_files = False;
					else if(!strcmp(argv[i], "-nostrict"))
						strict_checking = False;
					break;
				case 'd':
					if(!strcmp(argv[i], "-debug"))
						debug = True;
					break;
				default:
					break;
			}
		}
		else
			use_file = argv[i];
	}
	if(use_file == NULL)
	{
		fprintf(stderr, "HTMLparse: no file given\n");
		exit(EXIT_FAILURE);
	}
	if((fp = fopen(use_file, "r")) == NULL)
	{
		perror(use_file);
		exit(EXIT_FAILURE);
	}
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if((buffer = (char*)malloc(len)) == NULL)
	{
		fprintf(stderr, "Fatal: malloc failed for %i bytes.\n", len);
		fclose(fp);
		exit(EXIT_FAILURE);
	}
	/* read it */
	fread(buffer, len, 1, fp);
	/* close it */
	fclose(fp);

	/* start time */
	gettimeofday(&ts, NULL);

	/* Parse it */
	parsed_elements = _XmHTMLparseHTML(use_file, NULL, buffer, NULL);

	/* end time */
	gettimeofday(&te, NULL);
	secs = (int)(te.tv_sec - ts.tv_sec);
	usecs = (int)(te.tv_usec - ts.tv_usec);
	if(usecs < 0) usecs *= -1;

	/* print some parser statistics */
	fprintf(stderr, "-------------------\n");
	fprintf(stderr, "HTMLparse statistics\n");
	fprintf(stderr, "\tinput file     : %s\n", use_file);
	fprintf(stderr, "\tBytes parsed   : %i\n", len);
	fprintf(stderr, "\tTotal elements : %i\n",
		list_data.num_elements + list_data.num_text);
	fprintf(stderr, "\tHTML elements  : %i\n", list_data.num_elements);
	fprintf(stderr, "\ttext elements  : %i\n", list_data.num_text);
	fprintf(stderr, "\tErrors found   : %i\n", err_count);
	fprintf(stderr, "\tProcessing time: %i.%i seconds\n", secs, usecs);

	/* free buffer */
	free(buffer);

	/* write output to disk */
	if(write_files)
	{
		fprintf(stderr, "-------------------\n");
		/* has been reset to NULL */
		list_data.head = list_data.current;

		/* parser output */
		writeParsedOutputToFile(parsed_elements, "parsed_output");

		/* html output for live testing with other browsers */
		writeHTMLOutputToFile(parsed_elements, "parsed_output");
	}

	/* Free the list */
	gettimeofday(&ts, NULL);

	_XmHTMLFreeObjects(parsed_elements);

	gettimeofday(&te, NULL);
	secs = (int)(te.tv_sec - ts.tv_sec);
	usecs = (int)(te.tv_usec - ts.tv_usec);
	if(usecs < 0) usecs *= -1;
	fprintf(stderr, "-------------------\n");
	fprintf(stderr, "_XmHTMLFreeObjects done in %i.%i seconds\n", secs, usecs);
	fprintf(stderr, "-------------------\n");

	return(EXIT_SUCCESS);
}
#endif /* VERIFY */
