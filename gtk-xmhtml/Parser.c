#ifndef lint
static char rcsId[]="$Header$";
#endif
/*****
* Parser.c : htmlParserObjectClass routines
*
* This file Version	$Revision$
*
* Creation date:		Sun Apr 13 00:58:49 GMT+0100 1997
* Last modification: 	$Date$
* By:					$Author$
* Current State:		$State$
*
* Author:				newt
*
* Copyright (C) 1994-1997 by Ripley Software Development 
* All Rights Reserved
*
* This file is part of the XmHTML TWidget Library
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
* Revision 1.1  1997/11/28 03:38:54  gnomecvs
* Work in progress port of XmHTML;  No, it does not compile, don't even try -mig
*
* Revision 1.6  1997/10/23 00:24:36  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.5  1997/08/31 17:31:10  newt
* Removed HT_TEXTFLOW
*
* Revision 1.4  1997/08/30 00:26:56  newt
* my_strdup -> strdup and _XmHTMLWarning changes.
*
* Revision 1.3  1997/08/01 12:53:09  newt
* Minor bugfixes in HTML rules + state stack backtracking added.
*
* Revision 1.2  1997/05/28 01:30:58  newt
* Fixes in HTML comment parsing. Modified the parser to properly pick up the
* contents of the SCRIPT and STYLE head elements.
*
* Revision 1.1  1997/04/29 14:19:21  newt
* Initial Revision
*
*****/ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>				/* parser termination */

/* our private header file */
#include "ParserP.h"
#include "XmHTMLfuncs.h"

/*** External Function Prototype Declarations ***/

/*** Public Variable Declarations ***/

/*** Private Datatype Declarations ****/

/*** Private Function Prototype Declarations ****/
/** Usefull macros **/
#define ATTR(ID)		parser->parser.ID

#ifdef WITH_MOTIF
#    define PARSER		XmHTMLParserObject parser
#else
#    define PARSER              GtkXmHTMLParser *parser
#endif

/* push id on state stack */
static void pushState(PARSER, htmlEnum id);

/* pop id from state stack */
static htmlEnum popState(PARSER);

/* is id on the stack? */
static Boolean onStack(PARSER, htmlEnum id);

/* clear and reset the stack */
static void clearStack(PARSER);

/* convert token to and internal id using an XmHTMLAliasTable */
static htmlEnum tokenToAlias(PARSER, String token);

/* convert token to an internal id */
static htmlEnum tokenToId(PARSER, String token, Boolean warn);

/* see if id is a terminated HTML token or not */
static Boolean getTerminatorState(htmlEnum id);

/* see if id may appear inside <BODY> */
static Boolean isBodyElement(htmlEnum id);

/* verify occurance of current in parser state state */
static int checkOccurance(PARSER, htmlEnum current, htmlEnum state);

/* verify if current may appear as content of parser state state */
static Boolean checkContent(htmlEnum current, htmlEnum state);

/* create a new object */
static XmHTMLObject *newElement(PARSER, htmlEnum id, char *element,
	char *attributes, Boolean is_end, Boolean terminated);

/* create and insert a new element object */
static void insertElement(PARSER, String element, htmlEnum new_id,
	Boolean is_end);

/* create and store a text object */
static void storeTextElement(PARSER, char *start, char *end);

/* create and store an element object */
static String storeElement(PARSER, char *start, char *end);

/* verify presence of id, interactive checking */
static int verifyElement(PARSER, htmlEnum id, Boolean is_end);

/* verify presence of id, automatic checking */
static int verifyDefault(PARSER, htmlEnum id, Boolean is_end);

/* verify the verification and repair of the current document */
static XmHTMLObject *verifyVerification(PARSER);

/* create a valid parser tree for viewing image image_file */
static int parseIMAGE(PARSER, char *image_file);

/* parse (also progressive) using PARSER */
static int parseHTML(PARSER);

/* remove comments from given text. */
static String cutComment(PARSER, String start);

/* main parser driver routine */
static Boolean parserDriver(PARSER, String source);

/* perform parser wrapup */
static Boolean parserEndSource(PARSER);

/* create a text block for the given id */
static void makeTextBlockFromId(XmHTMLTextBlock block, htmlEnum id,
		Boolean is_end);

/* make a text block from current position in the current source text */
static void makeTextBlockFromInput(PARSER, XmHTMLTextBlock block);

/* XmNmodifyVerifyCallback driver */
static void modifyCallback(PARSER, Byte action, htmlEnum id, Boolean is_end);

/* XmNdocumentCallback driver */
static Boolean documentCallback(PARSER, Boolean verified);

/* XmNparserCallback driver */
static int parserCallback(PARSER, htmlEnum id, htmlEnum current,
		htmlEnum new_id, parserError error, Boolean is_end);

/* alphabetically sort the given alias table */
static void sortAliasTable(XmHTMLAliasTable table, int nalias);

/* add an alias to the given alias table */
static XmHTMLAliasTable addAliasToTable(PARSER, XmHTMLAliasTable table,
		int *num, String element, htmlEnum alias);

/* remove an alias from the given alias table */
static XmHTMLAliasTable removeAliasFromTable(PARSER, XmHTMLAliasTable table,
		int *num, String element, htmlEnum alias, Boolean *error);

/* copy the given alias table to a new alias table */
static XmHTMLAliasTable copyAliasTable(XmHTMLAliasTable source, int nalias,
		int *copied);

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
	(id) == HT_CAPTION || (id) == HT_A)

/* text containers */
#define IS_CONTAINER(id) ((id) == HT_BODY || (id) == HT_DIV || \
	(id) == HT_CENTER || (id) == HT_BLOCKQUOTE || (id) == HT_TD || \
	(id) == HT_FORM || (id) == HT_TH || (id) == HT_DT || (id) == HT_DD || \
	(id) == HT_LI || (id) == HT_NOFRAMES)

/* all elements that may be nested */
#define NESTED_ELEMENT(id) (IS_MARKUP(id) || (id) == HT_APPLET || \
	(id) == HT_BLOCKQUOTE || (id) == HT_DIV || (id) == HT_CENTER || \
	(id) == HT_FRAMESET)

#define DEFAULT_CONTENT(id) ((id) == HT_A || (id) == HT_APPLET || \
	(id) == HT_B || (id) == HT_BIG || (id) == HT_BR || (id) == HT_CITE || \
	(id) == HT_CODE || (id) == HT_DFN || (id) == HT_EM || (id) == HT_FONT || \
	(id) == HT_I || (id) == HT_IMG || (id) == HT_INPUT || (id) == HT_KBD || \
	(id) == HT_MAP || (id) == HT_NOFRAMES || (id) == HT_SAMP || \
	(id) == HT_SCRIPT || (id) == HT_SELECT || (id) == HT_SMALL || \
	(id) == HT_STRIKE || (id) == HT_STRONG || (id) == HT_SUB || \
	(id) == HT_SUP || (id) == HT_TEXTAREA || (id) == HT_TT || (id) == HT_U || \
	(id) == HT_VAR || (id) == HT_ZTEXT)

/*** Private Variable Declarations ***/
static Boolean parser_terminated;			/* for parser termination */
static jmp_buf parser_jmp;					/* for parser termination */

/* parseHTML return codes */
#define PARSER_END 			0	/* no new source added since last call */
#define PARSER_CONTINUE 	1	/* parser parsed the text successfully */
#define PARSER_ERROR 		2	/* parser encountered an error */

/* no translations */

/* no actions */

#ifdef WITH_MOTIF
#   include "Parser-motif.c"
#else
#   include "gtk-xmhtml-parser.c"
#endif

/*****
* Name: 		Destroy
* Return Type: 	void
* Description: 	XmHTMLParserObjectClass destroy method.
* In: 
*	w:			parser to destroy
* Returns:
*	nothing.
*****/
static void
Destroy(TWidget w)
{
	XmHTMLParserObject parser = (XmHTMLParserObject)w;

	/* free current source */
	if(ATTR(source))
		free(ATTR(source));

	/* clear current parser tree */
	if(ATTR(head))
		_XmHTMLParserFreeObjects(ATTR(head));

	/* clear open state stack */
	if(ATTR(stack))
		clearStack(parser);

	/* destroy alias table */
	if(ATTR(nalias))
		XmHTMLParserDestroyAliasTable(ATTR(alias_table), ATTR(nalias));

}

/*****************************************************************************
* Chapter 2
* Private Functions
*****************************************************************************/

/***
* Section 2.1 Parser state stack routines
*
* This set of routines maintains the parser's internal stack of HTML block
* elements. This stack defines the current parsers state, which is important
* for checking both occurance and contents of newly encountered HTML elements.
***/

/*****
* Name: 		pushState
* Return Type: 	void
* Description: 	pushes the given id on the state stack
* In: 
*	PARSER:		current parser
*	id:			element id to push
* Returns:
*	nothing.
*****/
static void
pushState(PARSER, htmlEnum id)
{
	stateStack *tmp;

	tmp = (stateStack*)malloc(sizeof(stateStack));
	tmp->id = id;
	tmp->next = ATTR(stack);
	ATTR(stack) = tmp;
	ATTR(depth) += 1;
}

/*****
* Name: 		popState
* Return Type: 	htmlEnum
* Description: 	pops an element of the state stack
* In: 
*	PARSER:		current parser
* Returns:
*	id of element popped.
*****/
static htmlEnum
popState(PARSER)
{
	htmlEnum id;
	stateStack *tmp;

	if(ATTR(stack)->next != NULL)
	{
		tmp = ATTR(stack);
		ATTR(stack) = ATTR(stack)->next;
		id = tmp->id;
		free((char*)tmp);
		ATTR(depth) -= 1;
	}
	else
		id = ATTR(stack)->id;
	return(id);
}

/*****
* Name: 		onStack
* Return Type: 	Boolean
* Description: 	checks whether the given id is somewhere on the current
*				state stack.
* In: 
*	PARSER:		current parser
*	id:			element id to check.
* Returns:
*	True when present, False if not.
*****/
static Boolean
onStack(PARSER, htmlEnum id)
{
	stateStack *tmp = ATTR(stack);

	while(tmp->next != NULL && tmp->id != id)
		tmp = tmp->next;
	return(tmp->id == id);
}

/*****
* Name: 		clearStack
* Return Type: 	void
* Description: 	clears and resets the state stack of a parser
* In: 
*	parser:		XmHTMLParserObject id
* Returns:
*	nothing
*****/
static void
clearStack(PARSER)
{
	while(ATTR(stack)->next != NULL)
		(void)popState(parser);
	ATTR(depth)      = 0;
	ATTR(base).next  = NULL;
	ATTR(base).id    = HT_DOCTYPE;
	ATTR(stack)      = &(ATTR(base));
}

/***
* Section 2.2 Token Resolvers.
*
* This set of routines convert HTML element identifiers to their corresponding
* internal id. All these routines use a binary search thru a list of
* alphabetically sorted strings.
***/

/*****
* Name: 		tokenToAlias
* Return Type: 	htmlEnum
* Description: 	searches the current parser alias table to convert an
*				unknown html token to a known internal id.
* In: 
*	PARSER:		current parser
*	token:		unknown token
* Returns:
*	aliased id if found, -1 if not found.
*****/
static htmlEnum
tokenToAlias(PARSER, String token)
{
	register int mid, lo = 0, hi = ATTR(nalias);
	XmHTMLAliasTable alias_table = ATTR(alias_table);
	int cmp;

	/* sanity */
	if(hi == 0)
		return(-1);

	while(lo <= hi)
	{
		mid = (lo + hi)/2;
		if((cmp = strcmp(token, alias_table[mid].element)) == 0)
			return(alias_table[mid].alias);
		else
			if(cmp < 0)				/* in lower end of array */
				hi = mid - 1;
			else					/* in higher end of array */
				lo = mid + 1;
	}
	return(-1);
}

/*****
* Name: 		tokenToId
* Return Type: 	int
* Description: 	converts the html token passed to an internal id.
* In: 
*	PARSER:		current parser
*	token:		token for which to fetch an internal id.
*	warn:		if true, spits out a warning for unknown tokens.
* Returns:
*	The internal id upon success, -1 upon failure
*
* Note: this routine uses a binary search into an array of all possible
*	HTML 3.2 tokens. It is very important that _BOTH_ the array 
*	html_tokens _AND_ the enumeration htmlEnum are *NEVER* changed. 
*	Both arrays are alphabetically sorted. Modifying any of these two 
*	arrays will	have VERY SERIOUS CONSEQUENCES, the return value of this
*	function matches a corresponding htmlEnum value.
*	As the table currently contains about 70 elements, a match will always
*	be found in at most 7 iterations (2^7 = 128)
*****/
static htmlEnum
tokenToId(PARSER, String token, Boolean warn)
{
	register int mid, lo = 0, hi = HT_ZTEXT-1;
	int cmp;

	while(lo <= hi)
	{
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
	* We don't want always have warnings. When scanning for SGML shorttags,
	* this routine is used to check whether we a / is right behind a token or
	* not.
	*/
	if(warn)
	{
		int ret_val = -1;
		if(ATTR(nalias))
			ret_val = tokenToAlias(parser, token);

		if(ret_val == -1)
		{
			ret_val = parserCallback(parser, HT_ZTEXT, HT_ZTEXT, HT_ZTEXT,
				HTML_UNKNOWN_ELEMENT, False);
			if(ret_val == HTML_REMOVE)
				return(-1);
			else
				/*
				* unknown element was aliased to a known one. Recheck this
				* element. Parser aliases are only supported for the standalone
				* parsers, so we assume the aliased id is a valid one. If it
				* isn't too bad then, its the programmers responsibility to set
				* up a correct parser alias table.
				*/
				return(tokenToAlias(parser, token));
		}
		return(ret_val);
	}
	return(-1);
}

/***
* Section 2.3 Element property routines.
*
* These are probably the most important routines used by the parser. They
* define the ending state of an element, verify if a new element is allowed
* to appear inside the current parser state and whether an element may contain
* a given element. Depending on the state of the parser, the latter two
* routines also suggest which element should be inserted to allow the offending
* element (this basically constitutes the parser's ability to verify and
* repair HTML documents).
***/

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

		/* all other elements are always terminated */
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
		/* all elements but these belong in body */
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
			return(False);
		default:
			return(True);
	}
	return(True);	/* not reached */
}

/*****
* Name:			checkOccurance
* Return Type:	Boolean
* Description:	checks whether the appearence of the current token is 
*				allowed in the current parser state.
* In: 
*	PARSER:		current parser
*	current:	HTML token to check
*	state:		parser state
* Returns:
*	When current is not allowed, the id of the element that should be
*	preceeding this one. If no suitable preceeding element can be deduced,
*	it returns -1. When the element is allowed, HT_ZTEXT is returned.
*****/
static int
checkOccurance(PARSER, htmlEnum current, htmlEnum state)
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
			if(state == HT_HTML || state == HT_FRAMESET)
				return((int)HT_ZTEXT);
			else
				return((int)HT_HTML); /* obvious */
			break;

		case HT_NOFRAMES:
			if(state == HT_BODY)
				return((int)HT_ZTEXT);
			else
				return((int)HT_BODY); /* not really obvious */
			break;

		case HT_FRAME:
			if(state == HT_FRAMESET)
				return((int)HT_ZTEXT);
			else
				return((int)HT_FRAMESET); /* obvious */
			break;

		case HT_BASE:
		case HT_ISINDEX:
		case HT_LINK:
		case HT_META:
		case HT_SCRIPT:
		case HT_STYLE:
		case HT_TITLE:
			if(state == HT_HEAD)
				return((int)HT_ZTEXT); /* only allowed in the <HEAD> section */
			else
				return((int)HT_HEAD); /* obvious */
			break;

		case HT_A:
			if(state == HT_A)
				return(-1); /* no nested anchors */
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
		case HT_IMG:
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
		case HT_PRE:
		case HT_OL:
		case HT_UL:
			if(IS_CONTAINER(state))
				return((int)HT_ZTEXT);
			return(-1); /* too bad, obliterate it */

		case HT_LI:
			if(state == HT_UL || state == HT_OL || state == HT_DIR || 
				state == HT_MENU)
				return((int)HT_ZTEXT);
			/*
			* Guess a return value: walk the current parser state and
			* see if a list is already present. If it's not, return HT_UL,
			* else return -1.
			*/
			for(curr = ATTR(stack); curr->next != NULL; curr = curr->next)
			{
				if(curr->id == HT_UL || curr->id == HT_OL ||
					curr->id == HT_DIR || curr->id == HT_MENU)
					return(-1);
			}
			return((int)HT_UL); /* start a new list */

		/*
		* <dl> receives special treatment: people often use this element
		* out of context to get an indented outline by using the following
		* sequence: <dl><dt>..</dt><dl>... and so on. This is an invalid
		* construct as the <dl> element may only be nested if it appears
		* inside a <dd> element...
		*/
		case HT_DL:
			if(IS_CONTAINER(state))
				return((int)HT_ZTEXT);
			return(onStack(parser, HT_DD) ? -1 : (int)HT_DD);

		case HT_DT:
		case HT_DD:
			if(state == HT_DL)
				return((int)HT_ZTEXT);
			return(onStack(parser, HT_DL) ? -1 : (int)HT_DL);

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
			/* see if we have a row */
			return(onStack(parser, HT_TR) ? -1 : (int)HT_TR);

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
			if(current == HT_TITLE || current == HT_ISINDEX || 
				current == HT_BASE || current == HT_SCRIPT ||
				current == HT_STYLE || current == HT_META || current == HT_LINK)
				return(True);
			break;

		case HT_NOFRAMES:
		case HT_BODY:
			if(current == HT_A || current == HT_ADDRESS ||
				current == HT_APPLET || current == HT_B || current == HT_BIG ||
				current == HT_BLOCKQUOTE || current == HT_BR ||
				current == HT_CENTER || current == HT_CITE ||
				current == HT_CODE || current == HT_DFN || current == HT_DIR ||
				current == HT_DIV || current == HT_DL || current == HT_EM ||
				current == HT_FONT || current == HT_FORM ||
				current == HT_NOFRAMES || current == HT_H1 ||
				current == HT_H2 || current == HT_H3 || current == HT_H4 ||
				current == HT_H5 || current == HT_H6 || current == HT_HR ||
				current == HT_I || current == HT_IMG || current == HT_INPUT ||
				current == HT_KBD || current == HT_MAP || current == HT_MENU ||
				current == HT_OL || current == HT_P || current == HT_PRE ||
				current == HT_SAMP || current == HT_SCRIPT ||
				current == HT_SELECT || current == HT_SMALL ||
				current == HT_STRIKE || current == HT_STRONG ||
				current == HT_SUB || current == HT_SUP || current == HT_TABLE ||
				current == HT_TEXTAREA || current == HT_TT || current == HT_U ||
				current == HT_UL || current == HT_VAR || current == HT_ZTEXT)
				return(True);
			break;

		case HT_ADDRESS:
			if(current == HT_P || DEFAULT_CONTENT(current))
				return(True);
			break;
		case HT_APPLET:
			if(current == HT_PARAM || DEFAULT_CONTENT(current)) 
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
			if(DEFAULT_CONTENT(current))
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

		/* these elements may only contain plain text */
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
			if(current == HT_H1 || current == HT_H2 || current == HT_H3 ||
				current == HT_H4 || current == HT_H5 || current == HT_H6 ||
				current == HT_ADDRESS)
				return(True);
			/* fall thru */
		case HT_LI:
		case HT_DD:
			if(current == HT_BLOCKQUOTE || current == HT_CENTER ||
				current == HT_DIR || current == HT_DIV || current == HT_DL ||
				current == HT_FORM || current == HT_HR || current == HT_MENU ||
				current == HT_OL || current == HT_P || current == HT_PRE ||
				current == HT_TABLE || current == HT_UL ||
				DEFAULT_CONTENT(current))
				return(True);
			break;

		/* special case, only a limited subset of the default content allowed */
		case HT_PRE:
			if(current == HT_A || current == HT_APPLET || current == HT_B ||
				current == HT_BR || current == HT_CITE || current == HT_CODE ||
				current == HT_DFN || current == HT_EM || current == HT_I ||
				current == HT_INPUT || current == HT_KBD || current == HT_MAP ||
				current == HT_NOFRAMES || current == HT_SAMP ||
				current == HT_SCRIPT || current == HT_SELECT ||
				current == HT_STRIKE || current == HT_STRONG ||
				current == HT_TEXTAREA || current == HT_TT || current == HT_U ||
				current == HT_VAR || current == HT_ZTEXT)
				return(True);
			break;

		/* unterminated tags don't contain a thing */
		case HT_BASE:
		case HT_BASEFONT:
		case HT_BR:
		case HT_HR:
		case HT_IMG:
		case HT_INPUT:
		case HT_ISINDEX:
		case HT_LINK:
		case HT_META:
		case HT_PARAM:
			return(True);

		case HT_ZTEXT:
			return(True); /* always allowed */

		/* no default so w'll get a warning when we miss anything */
	}
	return(False);
}

/***
* Section 2.4 Element allocation routines
*
* These routines create new parser tree objects and insert them in the
* tree.
***/

/*****
* Name: 		newElement
* Return Type: 	XmHTMLObject
* Description: 	allocates a new XmHTMLObject structure
* In: 
*	PARSER:		current parser
*	id:			id for this element
*	element:	char description for this element
*	attributes:	attributes for this element
*	is_end:		bool indicating whether this element is a closing one
*	terminated:	True when this is element has a terminating counterpart
* Returns:
*	a newly allocated XmHTMLObject. Exits the program if the allocation fails.
*****/
static XmHTMLObject*
newElement(PARSER, htmlEnum id, char *element, char *attributes, Boolean is_end,
	Boolean terminated)
{
	static XmHTMLObject *entry = NULL;

	entry = (XmHTMLObject*)malloc(sizeof(XmHTMLObject));

	entry->id         = id;
	entry->element    = element;
	entry->attributes = attributes;
	entry->is_end     = is_end;
	entry->terminated = terminated;
	entry->line       = ATTR(num_lines);
	entry->next       = (XmHTMLObject*)NULL;
	entry->prev       = (XmHTMLObject*)NULL;
	return(entry);
}

/*****
* Name: 		insertElement
* Return Type: 	void
* Description: 	creates and inserts a new element in the parser tree
* In: 
*	PARSER:		current parser
*	element:	element name
*	new_id:		id of element to insert
*	is_end:		False when the new element should open, True when it should
*				close.
* Returns:
*	nothing.
*****/
static void
insertElement(PARSER, String element, htmlEnum new_id, Boolean is_end)
{
	XmHTMLObject *extra;
	String tmp;

	/* need to do this, _XmHTMLParserFreeObjects will free this */
	tmp = strdup(element);

	/* allocate a element */
	extra = newElement(parser, new_id, tmp, NULL, is_end, True);

	/* insert this element in the list */
	ATTR(nelements) += 1;
	extra->prev = ATTR(current);
	ATTR(current)->next = extra;
	ATTR(current) = extra;

	modifyCallback(parser, HTML_INSERT, new_id, is_end);
}

/*****
* Name: 		storeTextElement
* Return Type: 	void
* Description: 	allocates and stores a plain text element
* In: 
*	PARSER:		current parser
*	start:		plain text starting point 
*	end:		plain text ending point
* Returns:
*	nothing
*****/
static void
storeTextElement(PARSER, char *start, char *end)
{
	static XmHTMLObject *element = NULL;
	static char *content = NULL;
	/* length of this block */
	int len = end - start;	

	/* sanity */
	if(*start == '\0' || len <= 0)
		return;

	content = my_strndup(start, len);

	element = newElement(parser, HT_ZTEXT, content, NULL, False, False);

	/* store this element in the list */
	ATTR(ntext) += 1;
	element->prev = ATTR(current);
	ATTR(current)->next = element;
	ATTR(current) = element;
}

/*****
* Name: 		copyElement
* Return Type: 	void
* Description: 	copies and inserts the given object
* In: 
*	PARSER:		current parser
*	src:		object to copy
*	is_end:		terminator state
* Returns:
*	nothing
*****/
static void
copyElement(PARSER, XmHTMLObject *src, Boolean is_end)
{
	static XmHTMLObject *copy;
	int len;

	/* sanity */
	if(src == NULL)
		return;

	copy = (XmHTMLObject*)malloc(sizeof(XmHTMLObject));

	copy->id         = src->id;
	copy->is_end     = is_end;
	copy->terminated = src->terminated;
	copy->line       = ATTR(num_lines);
	copy->next       = (XmHTMLObject*)NULL;
	copy->attributes = NULL;

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

	ATTR(nelements)++;
	/* attach prev and next ptrs to the appropriate places */
	copy->prev = ATTR(current);
	ATTR(current)->next = copy;
	ATTR(current) = copy;
}

/*****
* Name: 		storeElement
* Return Type: 	String
* Description: 	allocates and stores a HTML element
* In: 
*	PARSER:		current parser
*	start:		element starting point 
*	end:		element ending point
* Returns:
*	Updated char position if we had to skip <SCRIPT> or <STYLE> data.
*****/
static String
storeElement(PARSER, char *start, char *end)
{
	register char *chPtr, *elePtr;
	char *startPtr, *endPtr;
	Boolean is_end = False;
	static XmHTMLObject *element;
	static char *content;
	htmlEnum id;
	int terminated;

	if(end == NULL || *end == '\0')
		return(end);

	/* absolute ending position for this element */
	ATTR(cend) = ATTR(cstart) + (end - start);

	/*
	* If this is true, we have an empty tag or an empty closing tag. 
	* action to take depends on what type of empty tag it is.
	*/
	if(start[0] == '>' || (start[0] == '/' && start[1] == '>'))
	{
		/* 
		* if start[0] == '>', we have an empty tag, otherwise we have an empty
		* closing tag. In the first case, we walk backwards until we reach the 
		* very first tag. 
		* An empty tag simply means: copy the previous tag, nomatter what 
		* content it may have. In the second case, we need to pick up the 
		* last recorded opening tag and copy it.
		*/
		if(start[0] == '>')
		{
			/*
			* Walk backwards until we reach the first non-text element.
			* Elements with an optional terminator which are not terminated
			* are updated as well.
			*/
			for(element = ATTR(current) ; element != NULL;
				element = element->prev)
			{
				if(OPTIONAL_CLOSURE(element->id) && !element->is_end &&
					element->id == ATTR(stack)->id)
				{
					insertElement(parser, element->element, element->id, True);
					(void)popState(parser);
					break;
				}
				else if(element->id != HT_ZTEXT)
					break;
			}
			copyElement(parser, element, False);
			if(element->terminated)
				pushState(parser, element->id);
		}
		else
		{
			for(element = ATTR(current); element != NULL; 
				element = element->prev)
			{
				if(element->terminated)
				{
					if(OPTIONAL_CLOSURE(element->id))
					{
						/* add possible terminators for these elements */
						if(!element->is_end && element->id == ATTR(stack)->id)
						{
							insertElement(parser, element->element,
								element->id, True);
							(void)popState(parser);
						}
					}
					else
						break;
				}
			}

			copyElement(parser, element, True);
			(void)popState(parser);
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

		/* 
		* First skip past spaces and a possible opening /. The endPtr test
		* is mandatory or else we would walk the entire text over and over
		* again every time this routine is called.
		*/
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

		/* 
		* Move past the text to get any element attributes. The ! will let us 
		* pick up the !DOCTYPE definition.
		* Don't put the chPtr++ inside the tolower, chances are that it will be
		* evaluated multiple times if tolower is a macro.
		* From: Danny Backx <u27113@kb.be>
		*/
		if(*chPtr == '!')
			chPtr++;
		while(*chPtr && !(isspace(*chPtr)))		/* fix 01/17/97-01; kdh */
		{
			*chPtr = tolower(*chPtr);
			chPtr++;
		}

		/* 
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
		*/
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
		if((id = tokenToId(parser, elePtr, True)) != -1)
		{
			/*
			* Check if this element belongs to body. This test is as best
			* as it can get (we do not scan raw text for non-whitespace chars)
			* but will omit any text appearing *before* the <body> tag is
			* inserted.
			*/
			if(!ATTR(have_body))
			{
				if(id == HT_BODY)
					ATTR(have_body) = True;
				else if(isBodyElement(id))
				{
					insertElement(parser, "body", HT_BODY, False);
					pushState(parser, HT_BODY);
					ATTR(have_body) = True;
				}
			}

			/*
			* Go and verify presence of the new element. Use interactive
			* when a parserCallback is installed, use default element
			* verification otherwise.
			*/
			if(ATTR(automatic))
				terminated = verifyDefault(parser, id, is_end);
			else
				terminated = verifyElement(parser, id, is_end);

			if(terminated == -1)
			{
				ATTR(html32) = False;	/* not HTML32 conforming */
				free(content);
				return(end);
			}

			/* insert the current element */
			element = newElement(parser, id, elePtr, chPtr, is_end,
				(Boolean)terminated);

			/* attach prev and next ptrs to the appropriate places */
			ATTR(nelements)++;
			element->prev = ATTR(current);
			ATTR(current)->next = element;
			ATTR(current) = element;

			/*****
			* The SCRIPT element is a real pain in the ass to deal with
			* properly: the script itself is text with whatever in it,
			* and it's terminated by a closing SCRIPT element. It would be
			* *very* easy if the scripts content is enclosed within a 
			* comment, but since this is *NOT* required by the HTML 3.2 spec,
			* we need to check it in here...
			*****/
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
							if(*(end+1) == '/')
							{
								if(!(strncasecmp(end+1, "/script", 7)))
									done = 1;
								else if(!(strncasecmp(end+1, "/style", 6)))
									done = 2;
							}
							break;
						case '\n':
							ATTR(num_lines++);
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
					storeTextElement(parser, start, end-1);

					/* this was a script */
					if(done == 1)
						insertElement(parser, "script", HT_SCRIPT, True);
					else
						insertElement(parser, "style", HT_STYLE, True);
					/* pop state from stack */
					(void)popState(parser);

					/* and move to the closing > */
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
		else
		{
			modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
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

/***
* Section 2.5 Element verification routines.
*
* These routines contain the Parser's document verification and repair
* capabilities. There are two of them: one which performs calls to an
* installed XmNparserCallback and one which doesn't. This is done for
* performance reasons: a single check on presence of such a callback in the
* calling routine (newElement) is much more efficient than making a lot
* of calls to the parser callback and returning.
***/ 

/*****
* Name: 		verifyElement
* Return Type: 	int
* Description: 	element verifier, the real funny part.
* In: 
*	PARSER:		current parser
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
*	when the new element is out of place, possible user-interaction thru the
*	return value from parserCallback, the checks on contents of the current
*	element and appearance of the new element and the difference between
*	opening and closing elements.
*	This routine is far too complex to explain, I can hardly even grasp it
*	myself. If you really want to know what is happening here, read thru it
*	and keep in mind that checkOccurance and checkContent behave *very*
*	differently from each other.
*****/
static int
verifyElement(PARSER, htmlEnum id, Boolean is_end)
{
	/* current parser state */
	htmlEnum current = ATTR(stack)->id;
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
				/* tell caller we are about to insert something */
				switch(parserCallback(parser, id, current, current,
					HTML_NESTED, is_end))
				{
					case HTML_REMOVE:
						modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
						return(-1);
					case HTML_KEEP: 
						/* element allowed, push it */
						pushState(parser, id);
						return(1);
					case HTML_IGNORE:
					default:
						insertElement(parser, html_tokens[current], current,
							True);
						/* new element matches current: stays on the stack */
						return(1);
				}
			}
			/*
			* Second check: see if the new element is allowed to occur
			* inside the current element.
			*/
			if((new_id = checkOccurance(parser, id, current)) != HT_ZTEXT && 
				new_id != -1)
			{
				Boolean new_ending = ((htmlEnum)new_id == current);
				/*
				* We have received the id of the element that should be
				* preceeding this element. Call parserCallback when the
				* new id isn't one of the optional closing elements.
				*/
				if(!(OPTIONAL_CLOSURE((htmlEnum)new_id)))
				{
					/* tell caller we inserted something */
					switch(parserCallback(parser, id , current,
						(htmlEnum)new_id, HTML_VIOLATION, new_ending))
					{
						case HTML_REMOVE:
							modifyCallback(parser, HTML_REMOVE, HT_ZTEXT,
								is_end);
							return(-1);
						case HTML_KEEP:
							/* element allowed, push it */
							pushState(parser, id);
							return(1);
						default:
							break;
					}
				}
				insertElement(parser, html_tokens[new_id], (htmlEnum)new_id,
					new_ending);
				/*
				* If the new element terminates it's opening counterpart,
				* pop it from the stack. Otherwise it adds a new parser state.
				*/
				if(new_ending)
					(void)popState(parser);
				else
					pushState(parser, (htmlEnum)new_id);
				/* new element is now allowed, push it */
				pushState(parser, id);
				return(1);
			}
			/*
			* not allowed, see if the content matches. If not, terminate
			* the previous element and proceed as if nothing ever happened.
			*/
recheck:
			/* damage control */
			if(iter > 4 || (ATTR(stack)->next == NULL && iter))
			{
				/* stack restoration */
				if(ATTR(stack)->id == HT_DOCTYPE)
					pushState(parser, HT_HTML);
				if(ATTR(stack)->id == HT_HTML)
					pushState(parser, HT_BODY);

				/* remove, ignore */
				if((parserCallback(parser, id, current, current, HTML_BAD,
					is_end)) == HTML_IGNORE);
				{
					/* element allowed, push it */
					pushState(parser, id);
					return(1);
				}
				/* not allowed, remove it */
				modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
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
				/* spit out a warning if it's really missing */
				if(!(OPTIONAL_CLOSURE(current)))
				{
					/* insert, remove, keep, terminate */
					switch(parserCallback(parser, id, current, current,
						HTML_OPEN_BLOCK, is_end))
					{
						case HTML_INSERT:
							/* default processing */
							break;
						case HTML_REMOVE:
							/* not allowed, remove it */
							modifyCallback(parser, HTML_REMOVE, HT_ZTEXT,
								is_end);
							return(-1);
						case HTML_KEEP:
							/* element allowed, push it */
							pushState(parser, id);
							return(1);
					}
				}
				else /* tell caller we inserted something */
					(void)parserCallback(parser, current, current, current,
						HTML_NOTIFY, is_end);
				/* terminate current element before adding the new one*/
				if(id == current ||
					(current != HT_DOCTYPE && current != HT_HTML &&
					 current != HT_BODY))
					insertElement(parser, html_tokens[current], current, True);
				(void)popState(parser);
				current = ATTR(stack)->id;
				goto recheck;
			}
			else
			/*
			* Fourth check: does this do anything?
			* Update: it does something, but don't ask me what.
			*/
			if(!is_end && !(checkContent(id, current)))
			{
				/* remove, keep, terminate */
				if((parserCallback(parser, id, current, HT_ZTEXT,
					HTML_VIOLATION, is_end)) == HTML_REMOVE)
				{
					modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
					return(-1);
				}
			}
			/* element allowed, push it */
			pushState(parser, id);
			return(1);
		}
		else 
		{
			/* element ends, check balance. */
reterminate:
			/* damage control */
			if(iter > 4 || (ATTR(stack)->next == NULL && iter))
			{
				/* stack restoration */
				if(ATTR(stack)->id == HT_DOCTYPE)
					pushState(parser, HT_HTML);
				if(ATTR(stack)->id == HT_HTML)
					pushState(parser, HT_BODY);

				/* remove, ignore */
				if((parserCallback(parser, id, current, current, HTML_BAD,
					is_end)) == HTML_IGNORE);
				{
					/* element allowed, return */
					return(1);
				}
				/* not allowed, remove it */
				modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
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
				if((checkOccurance(parser, id, current)) != HT_ZTEXT)
				{
					/* spit out a warning if it's really missing */
					if(!(OPTIONAL_CLOSURE(current)))
					{
						/*
						* if id is present on stack we have an unbalanced
						* terminator
						*/
						if(onStack(parser, id))
							goto unbalanced;
						/* insert, remove, keep, terminate */
						switch(parserCallback(parser, id, current, current, 
							HTML_CLOSE_BLOCK, is_end))
						{
							case HTML_REMOVE:
								/* default processing */
								modifyCallback(parser, HTML_REMOVE, HT_ZTEXT,
									is_end);
								return(-1);
								break;
							case HTML_INSERT:
								/* terminate current element */
								break;
							case HTML_KEEP:
								/* element allowed */
								return(1);
						}
					}
					else /* tell caller we inserted something */
						(void)parserCallback(parser, current, current, current, 
							HTML_NOTIFY, is_end);
					/* terminate current before adding the new one */
					if(id == current ||
						(current != HT_DOCTYPE && current != HT_HTML &&
						 current != HT_BODY))
						insertElement(parser, html_tokens[current], current,
							True);
					current = popState(parser);
					if(current != id)
					{
						current = ATTR(stack)->id;
						goto reterminate;
					}
				}
				else if((new_id = checkOccurance(parser, current, id)) != -1)
				{
					if((htmlEnum)new_id == HT_ZTEXT)
						new_id = (int)current;
					/* spit out a warning if it's really missing */
					if(!(OPTIONAL_CLOSURE(current)))
					{
						/*
						* if id is present on stack we have an unbalanced
						* terminator
						*/
						if(onStack(parser, id))
							goto unbalanced;
						/* insert, remove, keep, terminate */
						switch(parserCallback(parser, id, current,
							(htmlEnum)new_id, HTML_CLOSE_BLOCK, is_end))
						{
							case HTML_REMOVE:
								/* default processing */
								modifyCallback(parser, HTML_REMOVE, HT_ZTEXT,
									is_end);
								return(-1);
								break;
							case HTML_INSERT:
								/* terminate current element */
								break;
							case HTML_KEEP:
								/* element allowed */
								return(1);
						}
					}
					else /* tell caller we inserted something */
						(void)parserCallback(parser, current, current,
							(htmlEnum)new_id, HTML_NOTIFY, is_end);
					/* terminate current before adding the new one */
					if(id == current ||
						(current != HT_DOCTYPE && current != HT_HTML &&
						 current != HT_BODY))
						insertElement(parser, html_tokens[new_id],
							(htmlEnum)new_id, True);
					current = popState(parser);
					if(current != id)
					{
						current = ATTR(stack)->id;
						goto reterminate;
					}
				}
				else
				{
unbalanced:
					/* spit out a warning if it's really missing */
					if(!(OPTIONAL_CLOSURE(current)))
					{
						/* remove, switch, terminate */
						if((parserCallback(parser, id, current, current,
							HTML_OPEN_ELEMENT, is_end)) == HTML_REMOVE)
						{
							modifyCallback(parser, HTML_REMOVE, HT_ZTEXT,
								is_end);
							return(-1);
						}
						/* switch element: insert current and continue. */
						insertElement(parser, html_tokens[current], current,
							True);
						/* restore stack */
						(void)popState(parser);
					}
					else
					{
						modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
						return(-1);
					}
				}
			}
			/* resync */
			if(id == ATTR(stack)->id)
				(void)popState(parser);
		}
		return(1);
	}
	else
	{
		/*
		* see if the new element is allowed as content of the
		* current element.
		*/
		if((new_id = checkOccurance(parser, id, current)) != (int)HT_ZTEXT)
		{
			Boolean new_ending;

			/* maybe terminate the current parser state? */
			if(new_id == -1)
			{
				new_id = (int)current;
				new_ending = True;
			}
			else
				new_ending = (new_id == (int)current);

			/* remove if requested */
			switch(parserCallback(parser, id, current, (htmlEnum)new_id,
				HTML_VIOLATION, new_ending))
			{
				case HTML_REMOVE:
					/* default */
					modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, False);
					return(-1);
				case HTML_INSERT:
					/* insert new element and adjust stack accordingly */
					insertElement(parser, html_tokens[new_id],
						(htmlEnum)new_id, new_ending);
					if(new_ending)
						(void)popState(parser);
					else
						pushState(parser, (htmlEnum)new_id);
					/* fall thru */
				case HTML_KEEP:
					default:
						break;
			}
		}
		return(0);
	}
	return(0);	/* not reached */
}

/*****
* Name: 		verifyDefault
* Return Type: 	int
* Description: 	see verifyElement. Only this routine does the default
*				behaviour and does not call the parserCallback, which is
*				a lot less overhead.
* In: 
*	PARSER:		current parser
*	id:			id to be verified
*	is_end:		element end flag.
* Returns:
*	-1 when element should be removed, 0 when the element is not terminated
*	and 1 when it is.
*****/
static int
verifyDefault(PARSER, htmlEnum id, Boolean is_end)
{
	/* current parser state */
	htmlEnum current = ATTR(stack)->id;
	int iter = 0, new_id;

	/* ending elements are automatically terminated */
	if(is_end || getTerminatorState(id))
	{
		if(!is_end)
		{
			/* 1: check element balance */
			if(id == current && !(NESTED_ELEMENT(id)))
			{
				/* default is to insert it */
				insertElement(parser, html_tokens[current], current, True);
				/* new element matches current, so it stays on the stack */
				return(1);
			}
			/* 2: see if new element may occur inside current element. */
			if((new_id = checkOccurance(parser, id, current)) != HT_ZTEXT && 
				new_id != -1)
			{
				/* HTML_VIOLATION, default is to insert new_id */
				insertElement(parser, html_tokens[new_id], (htmlEnum)new_id,
					new_id == current);
				if(new_id == current)
					(void)popState(parser);
				else
					pushState(parser, (htmlEnum)new_id);
				/* new element is now allowed, push it */
				pushState(parser, id);
				return(1);
			}
			/* 3: see if content matches */
recheck:
			/* damage control */
			if(iter > 4 || (ATTR(stack)->next == NULL && iter))
			{
				/* stack restoration */
				if(ATTR(stack)->id == HT_DOCTYPE)
					pushState(parser, HT_HTML);
				if(ATTR(stack)->id == HT_HTML)
					pushState(parser, HT_BODY);

				/* HTML_BAD, default is to remove it */
				modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
				return(-1);
			}
			iter++;
			/* 4: see if the new is allowed as content of current element. */
			if(!checkContent(id, current))
			{
				/* HTML_OPEN_BLOCK, default is to insert current */

				/* terminate current element before adding the new one*/
				if(id == current ||
					(current != HT_DOCTYPE && current != HT_HTML &&
					 current != HT_BODY))
					insertElement(parser, html_tokens[current], current, True);
				(void)popState(parser);
				current = ATTR(stack)->id;
				goto recheck;
			}
			else if(!is_end && !(checkContent(id, current)))
			{
				/* HTML_VIOLATION, no substitution, default is to remove it */
				modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
				return(-1);
			}
			/* element allowed, push it */
			pushState(parser, id);
			return(1);
		}
		else 
		{
			/* element ends, check balance. */
reterminate:
			/* damage control */
			if(iter > 4 || (ATTR(stack)->next == NULL && iter))
			{
				/* stack restoration */
				if(ATTR(stack)->id == HT_DOCTYPE)
					pushState(parser, HT_HTML);
				if(ATTR(stack)->id == HT_HTML)
					pushState(parser, HT_BODY);

				/* HTML_BAD, default is to remove it */
				modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
				return(-1);
			}
			iter++;
			if(id != current)
			{
				if((checkOccurance(parser, id, current)) != HT_ZTEXT)
				{
					/* remove if it's not an optional closing element */
					if(!(OPTIONAL_CLOSURE(current)))
					{
						/* if id is on stack: unbalanced terminator */
						if(onStack(parser, id))
							goto unbalanced;
						/* HTML_CLOSE_BLOCK, default is to remove it */
						modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
						return(-1);
					}
					/* terminate current before adding the new one */
					if(id == current ||
						(current != HT_DOCTYPE && current != HT_HTML &&
						 current != HT_BODY))
						insertElement(parser, html_tokens[current], current,
							True);
					current = popState(parser);
					if(current != id)
					{
						current = ATTR(stack)->id;
						goto reterminate;
					}
				}
				else if((new_id = checkOccurance(parser, current, id)) != -1)
				{
					if(new_id == HT_ZTEXT)
						new_id = current;
					/* remove if it's not an optional closing element */
					if(!(OPTIONAL_CLOSURE(current)))
					{
						/* if id on stack: unbalanced terminator */
						if(onStack(parser, id))
							goto unbalanced;
						/* HTML_CLOSE_BLOCK, default is to remove it */
						modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
						return(-1);
					}
					/* terminate current before adding the new one */
					if(id == current ||
						(current != HT_DOCTYPE && current != HT_HTML &&
						 current != HT_BODY))
						insertElement(parser, html_tokens[new_id], new_id,
							True);
					current = popState(parser);
					if(current != id)
					{
						current = ATTR(stack)->id;
						goto reterminate;
					}
				}
				else
				{
unbalanced:
					/* switch if it's not an optional closing element */
					if(!(OPTIONAL_CLOSURE(current)))
					{
						/* HTML_OPEN_ELEMENT, default is to switch */
						insertElement(parser, html_tokens[current], current,
							True);
						/* restore stack */
						(void)popState(parser);
					}
					else
					{
						modifyCallback(parser, HTML_REMOVE, HT_ZTEXT, is_end);
						return(-1);
					}
				}
			}
			/* resync */
			if(id == ATTR(stack)->id)
				(void)popState(parser);
		}
		return(1);
	}
	else
	{
		/* see if the new element is allowed as content of current element. */
		if((new_id = checkOccurance(parser, id, current)) != HT_ZTEXT)
		{
			/* maybe terminate the current parser state? */
			if(new_id == -1)
				new_id = current;

			/* HTML_VIOLATION, default is to insert since new_id is valid */
			insertElement(parser, html_tokens[new_id], (htmlEnum)new_id, 
				new_id == current);
			if(new_id == current)
				(void)popState(parser);
			else
				pushState(parser, (htmlEnum)new_id);
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
*	PARSER:		current parser
* Returns:
*	NULL when all parser states are balanced, offending object otherwise.
*****/
static XmHTMLObject *
verifyVerification(PARSER)
{
	XmHTMLObject *tmp = ATTR(head);
	htmlEnum current;

	/* walk to the first terminated item in the list */
	while(tmp != NULL && !tmp->terminated)
		tmp = tmp->next;

	/* a bunch of text, is practically impossible, but you'll never know... */
	if(tmp == NULL)
		return(NULL);

	/* reset state stack */
	ATTR(stack) = &ATTR(base);
	ATTR(stack)->id = current = tmp->id;
	ATTR(stack)->next = NULL;

	tmp = tmp->next;

	for(; tmp != NULL; tmp = tmp->next)
	{
		if(tmp->terminated)
		{
			if(tmp->is_end)
			{
				if(current != tmp->id)
					break;
				current = popState(parser);
			}
			else
			{
				pushState(parser, current);
				current = tmp->id;
			}
		}
	}
	clearStack(parser);
	return(tmp);
}

/***
* Section 2.6 Main HTML parsing routines
*
* This set of routines performs the actual parsing
***/

/*****
* Name: 		cutComment
* Return Type: 	String
* Description: 	removes HTML comments from the input stream
* In: 
*	start:		comment opening position
* Returns:
*	comment ending position
* Note:
*	See parse.c for the full comment.
*****/
static String
cutComment(PARSER, String start)
{
	int dashes = 0, nchars = 0, start_line = ATTR(num_lines);
	Boolean end_comment = False, start_dashes = False;
	String chPtr = start;

	/* move past opening exclamation character */
	chPtr++;
	while(!end_comment && *chPtr != '\0')
	{
		switch(*chPtr)
		{
			case '\n':
				ATTR(num_lines)++;
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
				/* only when next is also a dash we count them */
				if(*(chPtr+1) == '-')
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
					dashes++;
					if(!(dashes % 4))
						end_comment = True;
					else
					{
						char *sub = chPtr;
						Boolean end_sub = False;
						int sub_lines = ATTR(num_lines), sub_nchars = nchars;
						/*
						* Scan ahead until we run into another --> sequence or
						* element opening. If we don't, rewind and terminate
						* the comment.
						*/
						do
						{
							chPtr++;
							switch(*chPtr)
							{
								case '\n':
									ATTR(num_lines)++;
									nchars++;
									break;	/* fix 01/14/97-01; kdh */
								case '-':
									if(*(chPtr+1) == '-')
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
										dashes++;
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
							ATTR(num_lines) = sub_lines;
							nchars = sub_nchars;
						}
					}
				}
				break;
		}
		/* can run out of chars at any time */
		if(*chPtr == '\0')
			break;
		chPtr++;
		nchars++;
	}
	/* spit out a warning if the dash count is no multiple of four */
#if 0
	if(dashes %4 && bad_html_warnings)
		_XmHTMLWarning(__WFUNC__(html, "parseHTML"),
			"Bad HTML comment on line %i of input:\n    number of dashes is "
			"no multiple of four (counted %i dashes)", start_line, dashes);
#endif
	return(chPtr);
}

/*****
* A very usefull macro for handling shorttags.
* SGML shorttag handling. We use a buffer of a fixed length for storing an
* encountered token. We can do this safely ;-) since SGML shorttag's may
* *never* contain any attributes whatsoever. The fixed buffer does include
* some room for leading whitespace though.
* Note: all multi-line comments in this macro is placed as multiple
* single-line comments. Some cpp's choke on it otherwise..
*****/
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
	/* cut leading spaces and count possible newlines */ \
	while(*ptr && (isspace(*ptr))) \
	{ if(*ptr == '\n') ATTR(num_lines)++; ptr++; } \
	/* make lowercase */ \
	my_locase(ptr); \
	/* no warning message when tokenToId fails */ \
	if((id = tokenToId(parser, token, False)) != -1) \
	{ \
		/* store this element */ \
		(void)storeElement(parser, start, chPtr); \
		/* move past the / */ \
		chPtr++; \
		text_start = chPtr; \
		text_len = 0; \
		/* walk up to the next / which terminates this block */ \
		for(; *chPtr != '\0' && *chPtr != '/'; chPtr++, cnt++, text_len++) \
			if(*chPtr == '\n') ATTR(num_lines)++; \
		/* store the text */ \
		if(text_len && text_start != NULL) \
			storeTextElement(parser, text_start, chPtr); \
		text_start = chPtr + 1; /* starts after this slash */ \
		text_len = 0; \
		/* store the end element. Use the empty closing element notation so */ \
		/* storeElement knows what to do. Reset element ptrs after that. */ \
		(void)storeElement(parser, "/>", chPtr); \
		start = NULL;		/* entry has been terminated */ \
		done = True; \
	} \
}

/*****
* Name: 		parseImage
* Return Type: 	void
* Description: 	creates a parser tree for the given image so XmHTML can
*				display it. This is the only mimeType that doesn't perform
*				any progressive stuff.
* In: 
*	PARSER:		current parser
*	image_file:	name of image file to be loaded.
* Returns:
*	nothing.
*****/
static int
parseIMAGE(PARSER, char *image_file)
{
	String input = NULL;
	static char *def_img = "img src=\"%s\">";

	/* document header */
	insertElement(parser, html_tokens[HT_HTML], HT_HTML, False);
	insertElement(parser, html_tokens[HT_BODY], HT_BODY, False);

	/* one single element: def_img and image filename */
	input = (char*)malloc((13 + strlen(image_file))* sizeof(char));
	sprintf(input, def_img, image_file);
	(void)storeElement(parser, input, input + strlen(input)-1);
	free(input);
	
	/* document closure */
	insertElement(parser, html_tokens[HT_BODY], HT_BODY, True);
	insertElement(parser, html_tokens[HT_HTML], HT_HTML, True);

	/* end-of-input */
	return(PARSER_END);
}

/*****
* Name: 		parseHTML
* Return Type: 	Boolean
* Description: 	main HTML parser. Used for HTML and plain text in both
*				normal and progressive mode. The driver will make the proper
*				arrangments such as stack resetting, clearing any existing
*				parser tree and switching the parser to the correct mode.
* In: 
*	PARSER:		current parser
*	text:		accumulated text.
* Returns:
*	PARSER_END when no new source has been added since the last call;
*	PARSER_CONTINUE when the parser parsed the text successfully;
*	PARSER_ERROR when an error was encountered.
*****/
static int
parseHTML(PARSER)
{
	/* always initialize, source updating relies on it */
	register char *chPtr;
	char *start = NULL, *text_start = NULL, *element_start = NULL;
	int cnt = 0;
	Cardinal text_len = 0, line_len = 0;
	Boolean done = False;

	/* When the current parser and has not been reset, it's an error. */
	if(!ATTR(active) && !ATTR(reset))
	{
		_XmHTMLWarning(__WFUNC__(parser, "parseHTML"),
			"Internal Error: parser is inactive but has not been reset.");
		return(PARSER_ERROR);
	}

	/* this parser is active */
	ATTR(active) = True;
	ATTR(reset)  = False;

	/* and not terminated */
	ATTR(terminated) = False;

	/* Restore various flags and counters */
	cnt              = ATTR(cnt);
	line_len         = ATTR(line_len);

	/* parser starting point */
	chPtr            = &(ATTR(source)[ATTR(index)]);

	/* establish the setjmp return context for proper parser termination */
	if(setjmp(parser_jmp))
		parser_terminated = True;

	/* start scanning */
	while(*chPtr && !parser_terminated)
	{
		switch(*chPtr)
		{
			case '<':		/* start of element */
				/* absolute starting position for this element */
				element_start = chPtr;

				/* See if we have any text pending */
				if(text_len && text_start != NULL)
				{
					storeTextElement(parser, text_start, chPtr);
					text_start = NULL;
					text_len = 0;
				}
				/* move past element starter */
				start = chPtr+1; /* element text starts here */
				done = False;
				/* starting position for this element */
				ATTR(cstart) = (Cardinal)(start - ATTR(source));
				/*
				* scan until the end of this tag. Comments are removed
				* properly, but are *not* allowed inside tags (which is a
				* violation IMHO).
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
								chPtr = cutComment(parser, chPtr);
								/* back up one element */
								chPtr--;
								start = chPtr;
								done = True;
							}
							break;
						case '>':
							/* go and store the element */
							chPtr = storeElement(parser, start, chPtr);
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
							ATTR(num_lines)++;
							/* fall thru */
						default:
							break;
					}
				}
				if(done)
				{
					text_start = chPtr+1; /* plain text starts here */
					/* element has been added, invalidate starting position */
					element_start = NULL;
				}
				text_len = 0;
				start = NULL;
				break;
			case '\n':
				ATTR(num_lines)++;
				if(cnt > line_len)
					line_len = (Cardinal)cnt;
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

	/* If the parser was terminated for some reason, we die here */
	if(parser_terminated)
	{
		ATTR(terminated) = True;
		/* reset global termination flag */
		parser_terminated  = False;
		return(PARSER_ERROR);
	}

	/* and save parser state again */
	ATTR(cnt)      = cnt;
	ATTR(line_len) = line_len;

	/*
	* We were in the middle of an element when the source went out of
	* characters, so we can't possibly have text, but instead need to 
	* back up to the starting position of this element.
	*/
	if(element_start != NULL)
		ATTR(index)      = (Cardinal)(element_start - ATTR(source));
	else
	{
		/*
		* Any remaining text is flushed. XmHTML's formatter knows how to
		* glue it back together.
		*/
		if(text_len && text_start != NULL)
			storeTextElement(parser, text_start, chPtr);

		/* last known position */
		ATTR(index) = ATTR(source_len);
	}
	return(PARSER_CONTINUE);
}

/*****
* Name: 		parserDriver
* Return Type: 	Boolean
* Description: 	main parser driving routine. Performs required initialization
*				and checks and brances off to the correct sub-parser routines.
* In: 
*	PARSER:		current parser
*	source:		text to be parsed
* Returns:
*	True when sub-parser returned successfully, False when another pass
*	is requested/required.
*****/
static Boolean
parserDriver(PARSER, String source)
{
	int return_code = PARSER_END;
	Boolean ret_val = False;

    /* parse text */
	if(!(strcmp(ATTR(mime_type), "text/html")))
	{
		return_code = parseHTML(parser);
		ATTR(is_html) = True;
	}
	else if(!(strcmp(ATTR(mime_type), "text/plain")))
	{
		/****
		* Put in a document header when we haven't got one yet
		* parserEndDriver will add a document closure when parsing has
		* finished.
		****/
		if(!ATTR(have_body))
		{
			insertElement(parser, html_tokens[HT_HTML], HT_HTML, False);
			insertElement(parser, html_tokens[HT_BODY], HT_BODY, False);
			insertElement(parser, html_tokens[HT_PRE], HT_PRE, False);
		}
		return_code = parseHTML(parser);
		/* plain text file */
		ATTR(is_html) = False;
	}
	else if(!(strncmp(ATTR(mime_type), "image/", 6)))
	{
		parseIMAGE(parser, source);
		return_code = PARSER_END;

		/* always balanced but never valid HTML 3.2 */
		ATTR(is_html)    = True;
		ATTR(unbalanced) = False;
		ATTR(html32)     = False;
		ret_val = True;
	}
	else /* unknown mime type */
	{
		_XmHTMLWarning(__WFUNC__(parser, "parserDriver"),
			"%s: unknown mime type specification.");
		return(True);
	}
	/* Parsing terminated. Now check the return values */
	if(return_code == PARSER_ERROR)
	{
		/* clear any elements still on the stack and return */
		clearStack(parser);
		return(True);
	}

	/****
	* PARSER_END is only returned by the parser when it's in progressive mode
	* and no new text has been added since the last pass.
	* When the parser is *not* in progressive mode, PARSER_CONTINUE means
	* that the source has been parsed.
	* In both cases we need to perform document wrapup.
	****/
	if(return_code == PARSER_END || 
		(return_code == PARSER_CONTINUE && !ATTR(progressive)))
	{
		/****
		* Do document wrapup. parserEndSource returns False when another pass
		* was requested, True when wrapup was succesfully.
		****/
		ret_val = parserEndSource(parser);
	}
	else
	{
		/****
		* return code is PARSER_CONTINUE, which means we are in progressive
		* mode. In this case, document parsing should continue when new
		* text arrives, so we just return True.
		****/
		ret_val = True;
	}
	return(ret_val);
}

/*****
* Name: 		parserEndSource
* Return Type: 	Boolean
* Description: 	performs parser wrapup: checks stack consistency and flushes
*				it, adds document closure when mimetype is text/plain and
*				calls any installed XmNdocumentCallback callback resources
* In: 
*	PARSER:		current parser
* Returns:
*	False when another pass is requested/required, True when wrapup was
*	succesfully.
*****/
static Boolean
parserEndSource(PARSER)
{
	XmHTMLObject *tmp;
	XmHTMLObject *checked = NULL;

	/* premature end */
	if(ATTR(terminated))
	{
		/* give out a warning */
		_XmHTMLWarning(__WFUNC__(parser, "parserEndSource"),
			"HTML parser terminated before document was completed.");
		/* clear any elements still on the stack */
		clearStack(parser);
	}

	/* add document closure if this was a plain text file */
	if(!ATTR(is_html))
	{
		insertElement(parser, html_tokens[HT_PRE], HT_PRE, True);
		insertElement(parser, html_tokens[HT_BODY], HT_BODY, True);
		insertElement(parser, html_tokens[HT_HTML], HT_HTML, True);
	}

	/****
	* check state stack and flush out remaining objects. This is only
	* reached for text/html mimeType documents.
	****/
	if(ATTR(stack)->next != NULL)
	{
		htmlEnum state;

		/* this is an unbalanced document */
		ATTR(unbalanced) = True;
		/* and thus non-conforming */
		ATTR(html32)     = False;

		/* set insertion position for the elements still on the stack */
		ATTR(cstart) = ATTR(source_len);
		ATTR(cend)   = ATTR(cstart)+1;

		while(ATTR(stack)->next != NULL)
		{
			state = popState(parser);
			insertElement(parser, html_tokens[state], state, True);
		}
		/* check if this document verifies */
		checked = verifyVerification(parser);
	}

	/* adjust head and set object */
	tmp = ATTR(head);
	ATTR(head) = tmp->next;
	/* sanity */
	if(ATTR(head) != NULL)
	{
		ATTR(head)->prev = NULL;
		ATTR(current) = ATTR(head);
		ATTR(objects) = ATTR(head);
		ATTR(nobjects) = ATTR(nelements) + ATTR(ntext);
	}
	free(tmp);

	/* Check if we need to keep the current source text. If not, delete it */
	if(!ATTR(retain_source))
	{
		/* sanity check */
		if(ATTR(source))
			free(ATTR(source));
		ATTR(source)     = NULL;
		ATTR(source_len) = 0;
	}

	/****
	* Call the documentCallback. Returns False when another pass on the
	* current text is required, True when not.
	****/
	return(documentCallback(parser, checked == NULL));
}

/*****************************************************************************
* Chapter 3
* Callback drivers
*****************************************************************************/

/*****
* Name: 		makeTextBlockFromId
* Return Type: 	void
* Description: 	constructs the contents of a XmHTMLTextBlock from an 
*				htmlEnum id. 
* In: 
*	block:		XmHTMLTextBlock to be filled
*	id:			id to use
*	is_end:		element ending flag
* Returns:
*	nothing, but block is updated upon return.
*****/
static void
makeTextBlockFromId(XmHTMLTextBlock block, htmlEnum id, Boolean is_end)
{
	int len;
	static String ptr;
	String chPtr, elePtr;

	elePtr = html_tokens[id];

	/* <> + / if is_end is true, element and a closing \0 */
	len = 2 + (int)is_end + strlen(elePtr);
	ptr = (String)malloc(len+1);
	
	chPtr = ptr;

	*(chPtr++) = '<';
	if(is_end)
		*(chPtr++) = '/';
	while(*elePtr)
		*(chPtr++) = *(elePtr++);

	*(chPtr++) = '>';	/* close it */
	*chPtr = '\0';	/* NULL terminate */

	block->ptr = ptr;
	block->len = len;
}

/*****
* Name: 		makeTextBlockFromInput
* Return Type: 	void
* Description: 	constructs the contents of a XmHTMLTextBlock from the current
*				text positions.
* In: 
*	PARSER:		current parser
*	block:		block to be filled
* Returns:
*	nothing, but block is updated upon return.
*****/
static void 
makeTextBlockFromInput(PARSER, XmHTMLTextBlock block)
{
	/*
	* current_start_pos always points right after an opening < and
	* current_end_pos always right before a closing >
	*/
	block->len = ATTR(cend) - ATTR(cstart) + 2;
	block->ptr = my_strndup(&ATTR(source)[ATTR(cstart)- 1], block->len);
}

/*****
* Name: 		modifyCallback
* Return Type: 	void
* Description: 	Parser's XmNmodifyVerifyCallback handler
* In: 
*	PARSER:		current parser
*	action:		HTML_INSERT or HTML_REMOVE
*	id:			id to be inserted or removed. If id == HT_ZTEXT, plain text
*				is to be removed
*	is_end:		element ending flag
* Returns:
*	nothing.
*****/
static void
modifyCallback(PARSER, Byte action, htmlEnum id, Boolean is_end)
{
	static XmHTMLVerifyCallbackStruct cbs;
	static XmHTMLTextBlockRec text;

	if(!ATTR(modify_verify_callback))
		return;

	if(id == HT_ZTEXT)
		makeTextBlockFromInput(parser, &text);
	else
		makeTextBlockFromId(&text, id, is_end);

	cbs.reason    = XmCR_HTML_MODIFYING_TEXT_VALUE;
	cbs.event     = NULL;
	cbs.doit      = True;
	cbs.action    = action;
	cbs.line      = ATTR(num_lines);
	cbs.text      = &text;
	/* keep in sync with what we've inserted/removed so far */
	cbs.start_pos = ATTR(cstart) + ATTR(inserted) - 1;

	/*
	* current_start_pos always points right after an opening < and
	* current_end_pos always right before a closing >
	*/
	if(action == HTML_REMOVE)
	{
		/* end_pos - start_pos == length of text to cut */
		cbs.end_pos = cbs.start_pos + text.len; 

		/* source decrements by this amount */
		ATTR(inserted) -= text.len;
	}
	else
	{
		/* text insertion, start_pos and end_pos are always equal */
		cbs.end_pos = cbs.start_pos;

		/* source increments by this amount */
		ATTR(inserted) += text.len;
	}

	XtCallCallbackList((TWidget)parser, ATTR(modify_verify_callback), &cbs);

	/* free element copy */
	if(text.len)
		free(text.ptr);
}

/*****
* Name: 		documentCallback
* Return Type: 	Boolean
* Description: 	XmNdocumentCallback driver
* In: 
*	PARSER:		current parser.
*	verified:	true when the flushed stack is balanced, False when not.
* Returns:
* 	False when another pass on the current text is requested/required,
*	True when not.
*****/
static Boolean
documentCallback(PARSER, Boolean verified)
{
	XmHTMLDocumentCallbackStruct cbs;

	/* Parser has completed a pass on the document so increase loop_count */
	ATTR(loop_count)++;

	if(ATTR(document_callback))
	{
		cbs.reason     = XmCR_HTML_DOCUMENT;
		cbs.event      = (XEvent*)NULL;
		cbs.html32     = ATTR(html32);
		cbs.verified   = verified;
		cbs.terminated = ATTR(terminated);
		cbs.pass_level = ATTR(loop_count);
		cbs.balanced   = !ATTR(unbalanced);
		cbs.redo       = ATTR(unbalanced);

		XtCallCallbackList((TWidget)parser, ATTR(document_callback), &cbs);

		return(!cbs.redo);
	}
	else
	{
		/****
		* No document callback. Always return true when we did more than one
		* pass. When in this was the first pass, return False only when this
		* document isn't verified
		****/
		if(ATTR(loop_count) != 1)
			return(True);
		return(verified);
	}
	return(True);	/* not reached */
}

/*****
* Name: 		parserCallback
* Return Type: 	int
* Description: 	XmNparserCallback driver
* In: 
*	PARSER:		current parser
*	id:			offending id
*	current:	current parser state
*	new_id:		proposed element to fix the error
*	error:		type of error
*	is_end:		True when id is closing, False if opening
* Returns:
*	Depending on the type of error, an action identifying what the parser
*	should do with either the offending id or the current parser state.
*****/
static int
parserCallback(PARSER, htmlEnum id, htmlEnum current, htmlEnum new_id,
	parserError error, Boolean is_end)
{
	static XmHTMLTextBlockRec repair, offender, curr;
	XmHTMLParserCallbackStruct cbs;

	repair.ptr = offender.ptr = curr.ptr = NULL;
	repair.len = offender.len = curr.len = 0;

	switch(error)
	{
		case HTML_UNKNOWN_ELEMENT:	/* remove, alias, terminate */
		case HTML_CLOSE_BLOCK:		/* remove, insert, keep, terminate */
		case HTML_BAD:				/* remove, ignore, terminate */
			cbs.action = HTML_REMOVE;
			break;
		case HTML_OPEN_ELEMENT:		/* switch, remove, terminate */
			cbs.action = HTML_SWITCH;
			break;
		case HTML_OPEN_BLOCK:		/* insert, remove, keep, terminate */
			cbs.action = HTML_INSERT;
			break;
		case HTML_VIOLATION:		/* remove, insert, keep, terminate */
			if(new_id != HT_ZTEXT)
				cbs.action = HTML_INSERT;
			else
				cbs.action = ATTR(strict) ? HTML_REMOVE : HTML_KEEP;
			break;
		case HTML_NESTED:			/* keep, remove, ignore, terminate */
			cbs.action = HTML_KEEP;
			break;
		case HTML_NOTIFY:			/* insert, terminate */
			cbs.action = HTML_INSERT;
			break;
		case HTML_INTERNAL:			/* terminate, ignore */
			cbs.action = HTML_TERMINATE;
			break;
		/* no default */
	}

	/*
	* make appropriate text blocks, set bad_html flag and update error
	* count when error indicates a markup error or HTML violation.
	*/
	switch(error)
	{
		case HTML_UNKNOWN_ELEMENT:
			makeTextBlockFromInput(parser, &offender);
			ATTR(html32) = False;
			ATTR(err_count)++;
			break;
		case HTML_OPEN_ELEMENT:
			makeTextBlockFromInput(parser, &offender);
			makeTextBlockFromId(&repair, new_id, is_end);
			makeTextBlockFromId(&curr, current, is_end);
			ATTR(html32) = False;
			ATTR(err_count)++;
			break;
		case HTML_BAD:
			makeTextBlockFromInput(parser, &offender);
			ATTR(html32) = False;
			ATTR(err_count)++;
			break;
		case HTML_OPEN_BLOCK:
			makeTextBlockFromInput(parser, &offender);
			makeTextBlockFromId(&repair, new_id, True);
			makeTextBlockFromId(&curr, current, True);
			ATTR(html32) = False;
			ATTR(err_count)++;
			break;
		case HTML_CLOSE_BLOCK:
			makeTextBlockFromInput(parser, &offender);
			makeTextBlockFromId(&repair, new_id, is_end);
			ATTR(html32) = False;
			ATTR(err_count)++;
			break;
		case HTML_NESTED:
			makeTextBlockFromInput(parser, &offender);
			makeTextBlockFromId(&curr, current, is_end);
			ATTR(html32) = False;
			ATTR(err_count)++;
			break;
		case HTML_VIOLATION:
			makeTextBlockFromInput(parser, &offender);
			if(new_id != HT_ZTEXT)
				makeTextBlockFromId(&repair, new_id, is_end);
			makeTextBlockFromId(&curr, current, is_end);
			ATTR(html32) = False;
			ATTR(err_count)++;
			break;
		case HTML_NOTIFY:
			makeTextBlockFromId(&repair, new_id, is_end);
			break;
		case HTML_INTERNAL:
			ATTR(err_count)++;
			break;
		/* no default */
	}

	if(ATTR(parser_callback))
	{
		cbs.reason    = XmCR_HTML_PARSER;
		cbs.event     = NULL;
		cbs.errnum    = ATTR(err_count);
		cbs.line      = ATTR(num_lines);
		cbs.start_pos = ATTR(cstart) + ATTR(inserted) - 1;
		cbs.end_pos   = ATTR(cend) + ATTR(inserted);
		cbs.error     = error;
		cbs.repair    = &repair;
		cbs.offender  = &offender;
		cbs.current   = &curr;

		XtCallCallbackList((TWidget)parser, ATTR(parser_callback), &cbs);

		/* free XmHTMLTextBlockRec strings */
		if(repair.len)
			free(repair.ptr);
		if(offender.len)
			free(offender.ptr);
		if(curr.len)
			free(curr.ptr);

		/* check return action against error */
		switch(error)
		{
			case HTML_UNKNOWN_ELEMENT:	/* remove, alias, terminate */
				if(cbs.action != HTML_REMOVE &&
					cbs.action != HTML_ALIAS && 
					cbs.action != HTML_TERMINATE)
					cbs.action = HTML_REMOVE;
				break;
			case HTML_OPEN_ELEMENT:		/* switch, remove, terminate */
				if(cbs.action != HTML_REMOVE &&
					cbs.action != HTML_SWITCH &&
					cbs.action != HTML_TERMINATE)
					cbs.action = HTML_SWITCH;
				break;
			case HTML_BAD:				/* remove, ignore, terminate */
				if(cbs.action != HTML_REMOVE &&
					cbs.action != HTML_IGNORE &&
					cbs.action != HTML_TERMINATE)
					cbs.action = HTML_REMOVE;
				break;
			case HTML_OPEN_BLOCK:		/* insert, remove, keep, terminate */
				if(cbs.action != HTML_REMOVE &&
					cbs.action != HTML_INSERT &&
					cbs.action != HTML_KEEP &&
					cbs.action != HTML_TERMINATE)
					cbs.action = HTML_INSERT;
				break;
			case HTML_CLOSE_BLOCK:		/* remove, insert, keep, terminate */
				if(cbs.action != HTML_REMOVE &&
					cbs.action != HTML_INSERT &&
					cbs.action != HTML_KEEP &&
					cbs.action != HTML_TERMINATE)
					cbs.action = HTML_REMOVE;
				break;
			case HTML_NESTED:			/* remove, keep, ignore, terminate */
				if(cbs.action != HTML_REMOVE &&
					cbs.action != HTML_KEEP &&
					cbs.action != HTML_IGNORE &&
					cbs.action != HTML_TERMINATE)
					cbs.action = HTML_KEEP;
				break;
			case HTML_VIOLATION:		/* remove, keep, insert, terminate */
				if(cbs.action != HTML_REMOVE &&
					cbs.action != HTML_KEEP &&
					cbs.action != HTML_INSERT &&
					cbs.action != HTML_TERMINATE)
					if(new_id != HT_ZTEXT)
						cbs.action = HTML_INSERT;
					else
						cbs.action = ATTR(strict) ? HTML_REMOVE : HTML_KEEP;
				break;
			case HTML_NOTIFY:			/* insert, terminate */
				if(cbs.action != HTML_INSERT &&
					cbs.action != HTML_TERMINATE)
					cbs.action = HTML_INSERT;
				break;
			case HTML_INTERNAL:			/* terminate, ignore */
				if(cbs.action != HTML_IGNORE &&
					cbs.action != HTML_TERMINATE)
					cbs.action = HTML_TERMINATE;
				break;
			/* no default */
		}
	}
	/* parser termination, jump */
	if(cbs.action == HTML_TERMINATE)
		longjmp(parser_jmp, 1);

	return(cbs.action);
}

/*****************************************************************************
* Chapter 4
* HTML alias routines
*****************************************************************************/

/*****
* Name: 		sortAliasTable
* Return Type: 	void
* Description: 	sorts the given alias table in alphabetical order
* In: 
*	table:		table to be sorted
*	nalias:		no of items in table.
* Returns:
*	nothing, but the alias table of the given parser is sorted.
* Note
*	Since the alias table of a parser will be relatively small, this routine
*	just uses a simple bubble sort to sort the table.
*****/
static void
sortAliasTable(XmHTMLAliasTable table, int nalias)
{
	int pass, i;
	String hold_element;
	htmlEnum hold_id;

	/* sanity */
	if(table == NULL || nalias < 2)
		return;

	/* no of passes to perform */
	for(pass = 1; pass < nalias; pass++)
	{
		/* a single pass */
		for(i = 0; i < nalias - 1; i++)
		{
			/* compare */
			if((strcmp(table[i].element, table[i+1].element)) > 0)
			{
				/* swap contents */
				hold_element       = table[i].element;
				hold_id            = table[i].alias;
				table[i].element   = table[i+1].element;
				table[i].alias     = table[i+1].alias;
				table[i+1].element = hold_element;
				table[i+1].alias   = hold_id;
			}
		}
	}
}

/*****
* Name: 		addAliasToTable
* Return Type: 	XmHTMLAliasTable
* Description: 	adds an alias to the given alias table.
* In: 
*	PARSER:		current parser
*	table:		current alias table
*	num:		no of aliases in table, updated upon return.
*	unknown:	unknown element for which an alias is being added
*	alias:		alias to use for this unknown element.
* Returns:
*	When alias was successfully added, a ptr to a new table. Previous table
*	is destroyed.
*	Null if not successfully added and previous table is left untouched.
*****/
static XmHTMLAliasTable
addAliasToTable(PARSER, XmHTMLAliasTable table, int *num, String element,
	htmlEnum alias)
{
	static XmHTMLAliasTable new_table;
	static String unknown;
	String start, end;
	int i, nalias;

	/*
	* We need to filter out the element itself, not any braces, whitespace
	* or whatever.
	*/
	start = element;
	/* get to real element start */
	while(*start)
	{
		if(isalnum(*start))
			break;
		start++;
	}
	if(!*start)
	{
		_XmHTMLWarning(__WFUNC__(parser, "addAliasToTable"),
			"Could not insert alias for unknown element %s: failed to detect"
			" element start.", element);
		return(NULL);
	}

	/* get to the end of this element */
	end = start;
	while(*end && isalnum(*end))
		end++;

	/* copy it */
	unknown = my_strndup(start, end - start);

	if(unknown == NULL || !*unknown)
	{
		_XmHTMLWarning(__WFUNC__(parser, "addAliasToTable"),
			"Could not insert alias for unknown element %s: failed to detect"
			" element end.", element);
		return(NULL);
	}
	/* make lowercase */
	my_locase(unknown);

	/* see if this alias already exists */
	if((i = tokenToAlias(parser, unknown)) != -1)
	{
		_XmHTMLWarning(__WFUNC__(parser, "addAliasToTable"),
			"Alias for unknown element %s not installed: element already "
			"aliased to %s.", unknown, html_tokens[i]);
		free(unknown);
		return(NULL);
	}
	nalias = *num + 1;

	/* create new alias table */
	new_table = (XmHTMLAliasTable)calloc(nalias, sizeof(XmHTMLAlias));

	/* copy previous table */
	for(i = 0; i < *num; i++)
	{
		new_table[i].element = strdup(table[i].element);
		new_table[i].alias = table[i].alias;
	}
	/* add new element */
	new_table[i].element = unknown;
	new_table[i].alias   = alias;

	/* free current alias table */
	XmHTMLParserDestroyAliasTable(table, *num);

	/* new alias table size */
	*num = nalias;

	/* sort it */
	sortAliasTable(new_table, *num);

	return(new_table);
}

/*****
* Name: 		removeAliasFromTable
* Return Type: 	XmHTMLAliasTable
* Description: 	removes the given alias from the given alias table.
* In: 
*	PARSER:		current parser 
*	table:		current alias table
*	num:		no of aliases in table, updated upon return.
*	unknown:	unknown element for which an alias is to be removed
*	alias:		alias used by this unknown element.
*	error:		error flag, set when some error occurs.
* Returns:
*	Updated table when alias was successfully added, previous table is
*	destroyed. Upon error, the error flag is set and the previous table is
*	left untouched.
* Note:
*	an alias is only removed if the given element/alias pair matches the
*	installed element/alias pair.
*****/
static XmHTMLAliasTable
removeAliasFromTable(PARSER, XmHTMLAliasTable table, int *num, String element,
	htmlEnum alias, Boolean *error)
{
	static XmHTMLAliasTable new_table;
	static String unknown;
	String start, end;
	int i, nalias;

	*error = False;

	/* various sanity checks */
	if(*num == 0 || table == NULL)
		return(table);

	if(element == NULL || !*element)
	{
		*error = True;
		return(table);
	}

	/*
	* We need to filter out the element itself, not any braces, whitespace
	* or whatever.
	*/
	start = element;
	/* get to real element start */
	while(*start)
	{
		if(isalnum(*start))
			break;
		start++;
	}
	if(!*start)
	{
		_XmHTMLWarning(__WFUNC__(parser, "removeAliasFromTable"),
			"Could not remove alias for element %s: failed to detect "
			"element start.", element);
		*error = True;
		return(table);
	}

	/* get to the end of this element */
	end = start;
	while(*end && isalnum(*end))
		end++;

	/* copy it */
	unknown = my_strndup(start, end - start);

	if(unknown == NULL || !*unknown)
	{
		_XmHTMLWarning(__WFUNC__(parser, "removeAliasFromTable"),
			"Could not remove alias unknown element %s: failed to detect "
			"element end.", element);
		*error = True;
		return(table);
	}
	/* make lowercase */
	my_locase(unknown);

	/* see if this alias already exists */
	if((i = tokenToAlias(parser, unknown)) != alias)
	{
		_XmHTMLWarning(__WFUNC__(parser, "removeAliasFromTable"),
			"Alias for element %s not removed: installed alias %s does not "
			"match given alias %s.", unknown, html_tokens[i],
			html_tokens[alias]);
		free(unknown);
		*error = True;
		return(table);
	}
	nalias = *num - 1;

	/* table is empty */
	if(nalias == 0)
	{
		XmHTMLParserDestroyAliasTable(table, *num);
		*num = 0;
		*error = False;
		return(NULL);
	}

	/* create new alias table */
	new_table = (XmHTMLAliasTable)calloc(nalias, sizeof(XmHTMLAlias));

	/* copy previous table, and leave the element to be removed out */
	for(i = 0; i < *num; i++)
	{
		/*
		* Both alias and element name must match, an alias can be used
		* for more than one element
		*/
		if(table[i].alias != alias && strcmp(table[i].element, unknown))
		{
			new_table[i].element = strdup(table[i].element);
			new_table[i].alias = table[i].alias;
		}
	}
	/* no longer needed */
	free(unknown);

	/* free current alias table */
	XmHTMLParserDestroyAliasTable(table, *num);

	/* new alias table size */
	*num = nalias;

	/* sort it */
	sortAliasTable(new_table, *num);

	return(new_table);
}

/*****
* Name: 		copyAliasTable
* Return Type: 	XmHTMLAliasTable
* Description: 	copies the given alias table to a new table
* In: 
*	source:		table to be copied
*	nalias:		no of aliases in this table
*	*copied:	no of items in table copied, updated upon return
* Returns:
*	a copy of the given table. *copied will match nalias if no error occured,
*	otherwise it will contain the no of items actually copied.
*****/
static XmHTMLAliasTable
copyAliasTable(XmHTMLAliasTable source, int nalias, int *copied)
{
	static XmHTMLAliasTable table;
	int i;

	if(nalias == 0 || source == NULL)
	{
		*copied = 0;
		return(NULL);
	}

	/* copy it */
	table = (XmHTMLAliasTable)calloc(nalias, sizeof(XmHTMLAlias));

	/* fill it in */
	for(i = 0; i < nalias; i++)
	{
		if(source[i].element == NULL || !*(source[i].element))
		{
			*copied = i;
			/* sort it */
			sortAliasTable(table, i);
			return(table);
		}
		/* duplicate element */
		table[i].element = strdup(source[i].element);
		table[i].alias   = source[i].alias;
		/* make lowercase */
		my_locase(source[i].element);
	}
	/* sort it */
	sortAliasTable(table, nalias);

	*copied = nalias;
	return(table);
}

/*****************************************************************************
* Chapter 5
* Private Interfaces
*****************************************************************************/

/*****
* Name: 		_XmHTMLParserGetString
* Return Type: 	String
* Description: 	creates a HTML source document from the given parser tree.
* In: 
*	objects:	parser tree.
* Returns:
*	created document in a buffer. This buffer must be freed by the caller.
*****/
String
_XmHTMLParserGetString(XmHTMLObject *objects)
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
* Name: 		_XmHTMLParserFreeObjects
* Return Type: 	void
* Description: 	releases all memory occupied by the parsed list of objects.
* In: 
*	objects:	object list to be destroyed
* Returns:
*	nothing.
*****/
void
_XmHTMLParserFreeObjects(XmHTMLObject *objects)
{
	XmHTMLObject *temp;

	/* free all parsed objects */
	while(objects)
	{
		temp = objects->next;
		/* sanity check. Should not be needed anyway. */
		if(objects->element)
			free(objects->element);
		free(objects);
		objects = temp;
	}
	objects = NULL;
}

/*****************************************************************************
* Chapter 6
* Public Interfaces
*****************************************************************************/

/***
* Section 6.1 Parser creation and configuring routines.
***/

/*****
* Name:			XmHTMLParserSetAutoMode
* Return Type: 	void
* Description: 	switches XmNparserCallback honoring on or off.
* In: 
*	w:			XmHTMLParser object
*	set:		value to set, True enables, False disables.
* Returns:
*	nothing
*****/
void
XmHTMLParserSetAutoMode(TWidget w, Boolean set)
{
	XmHTMLParserObject parser;

	if(w && !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserSetAutoMode");
		return;
	}

	parser = XmHTMLParser (w);
	if(parser->parser.parser_callback)
		parser->parser.automatic = set;
}

/*****
* Name:			XmHTMLParserSetProgressiveMode
* Return Type: 	void
* Description: 	enables/disables progressive parsing mode
* In: 
*	w:			XmHTMLParser object
*	set:		value to set, True enables, False disables.
* Returns:
*	nothing
*****/
void
XmHTMLParserSetProgressiveMode(TWidget w, Boolean set)
{
	XmHTMLParserObject parser;

	if(w && !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserSetProgressiveMode");
		return;
	}

	parser = XmHTMLParser (w);
	parser->parser.progressive = set;
}

/***
* Section 6.2 Text Parsing routines
*
* This set of routines is to be used for parsing a document.
***/

/*****
* Name: 		XmHTMLParserSetString
* Return Type: 	Boolean
* Description: 	sets a new source into a HTML parser and starts parsing.
* In: 
*	w:			XmHTMLParserObject id
*	source:		new source to be parsed.
* Returns:
*	True when the new source was parsed succesfully, False if another pass
*	is required.
*****/
Boolean
XmHTMLParserSetString(TWidget w, String source)
{
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserSetString");
		return(True);
	}

	parser = XmHTMLParser (w);

	/***
	* first clear any old stuff 
	***/

	/* current source */
	if(ATTR(source))
	{
		free(ATTR(source));
		ATTR(source)     = NULL;
		ATTR(source_len) = 0;
	}

	/* clear current parser tree */
	if(ATTR(head))
		_XmHTMLParserFreeObjects(ATTR(head));

	/* open state stack */
	if(ATTR(stack)->next != NULL)
		clearStack(parser);

	/***
	* Done clearing, now reset this parser
	*
	* We don't reset the following fields as there are other routines that
	* handle this:
	* parser->parser.alias_table
	* parser->parser.num_alias
	* parser->parser.progressive
	* parser->parser.auto
	***/

	/* list of objects is always initialized to contain a head text element */
	ATTR(objects)     = (XmHTMLObject*)NULL;
	ATTR(head)        = newElement(parser, HT_ZTEXT, NULL, NULL, False, False);
	ATTR(current)     = parser->parser.head;
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
	ATTR(base).id     = HT_DOCTYPE;
	ATTR(base).next   = (stateStack*)NULL;
	ATTR(stack)       = &(parser->parser.base);
	ATTR(depth)       = 0;

	ATTR(unbalanced)  = False;
	ATTR(html32)      = True;
	ATTR(have_body)   = False;
	ATTR(reset)       = True;
	ATTR(active)      = False;
	ATTR(terminated)  = False;

	/* start parsing when we have a source */
	if(source)
	{
		/* this parser is active */
		ATTR(active) = True;
		ATTR(reset)  = False;

		/* and not terminated */
		ATTR(terminated) = False;

		/* duplicate */
		ATTR(source)     = strdup(source);
		ATTR(source_len) = strlen(source);

		/****
		* Return True when successfully ended, False when another pass
		* is requested.
		****/
		return(parserDriver(parser, ATTR(source)));
	}
	else	/* clear current source */
	{
		ATTR(source)      = (String)NULL;
		ATTR(source_len)  = 0;
	}
	return(True);
}

/*****
* Name: 		XmHTMLParserUpdateSource
* Return Type: 	Boolean
* Description: 	Progressive parser interface.
* In: 
*	w:			HTMLParserObject id
*	source:		new source
* Returns:
*	True when parsed successfully, False on end of input or error.
*****/
Boolean
XmHTMLParserUpdateSource(TWidget w, String source)
{
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserUpdateSource");
		return(False);
	}

	parser = XmHTMLParser (w);

	/*
	* Progressive parsing terminates when there is no text to scan,
	* the new text doesn't increment anymore or previous source part
	* doesn't match the current source part, in which case the input is
	* corrupted.
	* A final check is parser termination, user might have used the
	* XmHTMLParserTerminate routine to interrupt parsing.
	* Checks are only done when we have an existing source.
	*/
	if(ATTR(source) != NULL && 
		(source == NULL || ATTR(terminated) ||
		(ATTR(source_len) && strlen(source) <= ATTR(source_len)) ||
		(ATTR(source_len) && strncmp(ATTR(source), source, ATTR(source_len)))))
	{
		/*
		* This is a very serious error, text has disappeared or been
		* modified by some external influence. We can't do anything with
		* that so we spit out a warning and set the terminated flag.
		*/
		if(strncmp(ATTR(source), source, ATTR(source_len)) ||
			strlen(source) < ATTR(source_len))
		{
			_XmHTMLWarning(__WFUNC__(parser, "XmHTMLParserUpdateSource"),
				"Cannot continue parsing: input buffer corrupted!");
			ATTR(terminated) = True;

			/* call parser terminator */
		}
		return(False);
	}

	/* this parser is active */
	ATTR(active) = True;
	ATTR(reset)  = False;

	/* and not terminated */
	ATTR(terminated) = False;

	/* free previous source, it's become invalid */
	if(ATTR(source))
		free(ATTR(source));

	/* duplicate new source */
	ATTR(source)     = strdup(source);
	ATTR(source_len) = strlen(source);

	/****
	* Call the driver. Returns True when successfully scanned the new text,
	* False when another pass is requested.
	****/
	return(parserDriver(parser, source));
}

/*****
* Name: 		XmHTMLParserTerminateSource
* Return Type: 	void
* Description: 	terminates parsing
* In: 
*	w:			HTMLParserObject id
* Returns:
*	nothing
*****/
void
XmHTMLParserTerminateSource(TWidget w)
{
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserTerminateSource");
		return;
	}

	parser = XmHTMLParser(w);
	ATTR(terminated) = True;

	(void)parserEndSource(parser);
}

/*****
* Name: 		XmHTMLParserEndSource
* Return Type: 	Boolean
* Description:	Perform parser wrapup and call any installed
*				XmNdocumentCallbacks
* In: 
*	w:			XmHTMLParserObject id
* Returns:
*	False when document should be re-parsed, True when document was ended
*	successfully.
*****/
Boolean
XmHTMLParserEndSource(TWidget w)
{
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserEndSource");
		return(False);
	}

	parser = XmHTMLParser(w);

	/* terminate current source */
	return(parserEndSource(parser));
}

/***
* Section 6.3 Parser Output routines
*
* This set of routines provides access to any generated document or parser
* tree
***/

/*****
* Name: 		XmHTMLParserGetSource
* Return Type: 	String
* Description: 	returns a copy of the current source
* In: 
*	w:			XmHTMLParserObject id
* Returns:
*	a copy of the current parser source or NULL when no source is present.
*	Return value must be freed by the caller.
*****/
String
XmHTMLParserGetSource(TWidget w)
{
	XmHTMLParserObject parser;
	static String source;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserGetSource");
		return(NULL);
	}

	parser = XmHTMLParser(w);

	if(ATTR(source) == NULL)
		return(NULL);

	source = strdup(ATTR(source));
	return(source);
}

/*****
* Name: 		XmHTMLParserGetString
* Return Type: 	String
* Description: 	returns the current parser contents
* In: 
*	w:			XmHTMLParserObject id
* Returns:
*	parser contents. This return value must be freed by the caller.
*****/
String
XmHTMLParserGetString(TWidget w)
{
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserGetString");
		return(NULL);
	}

	parser = XmHTMLParser(w);

	return(_XmHTMLParserGetString(ATTR(objects)));
}

/*****
* Name: 		XmHTMLParserGetTree
* Return Type: 	XmHTMLObject*
* Description: 	returns the parser tree from the given parser.
* In: 
*	parser:		XmHTMLParserObject id
* Returns:
*	requested parser tree. May *never* be freed or touched by caller.
*****/
XmHTMLObject *
XmHTMLParserGetTree(TWidget w)
{
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserGetTree");
		return(NULL);
	}

	parser = XmHTMLParser(w);

	return(ATTR(objects));
}

/***
* Section 6.4 Parser HTML alias table routines
*
* This set of routines creates, installs, uninstalls a XmHTMLAlias or
* XmHTMLAliasTable.
***/

/*****
* Name: 		XmHTMLParserSetAliasTable
* Return Type: 	int
* Description: 	adds an alias table to the current parser
* In: 
*	w:			XmHTMLParserObject id
*	table:		alias table to add
*	nalias:		no of aliases in the alias_table
* Returns:
*	no of aliases added or 0 on failure.
* Note:
*	this routine makes a copy of the given alias table. When it encounters
*	an empty element in the given alias table, it returns the no of aliases
*	added so far.
*****/
int
XmHTMLParserSetAliasTable(TWidget w, XmHTMLAliasTable table, int nalias)
{
	int copied = 0;
	static XmHTMLAliasTable aliases;
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserSetAliasTable");
		return(0);
	}

	parser = XmHTMLParser(w);

	/* copy it */
	aliases = copyAliasTable(table, nalias, &copied);

	if(copied != nalias)
	{
		_XmHTMLWarning(__WFUNC__(w, "XmHTMLParserSetAliasTable"),
			"Could only install %i aliases of %i requested, alias %i is empty.",
				copied, nalias, copied+1);
	}
	/* release any previously installed alias table */
	XmHTMLParserDestroyAliasTable(ATTR(alias_table), ATTR(nalias));

	/* set new alias table */
	ATTR(alias_table) = aliases;
	ATTR(nalias)      = copied;

	/* return no of aliases installed */
	return(copied);
}

/*****
* Name: 		XmHTMLParserAddAlias
* Return Type: 	Boolean
* Description: 	adds an alias to the alias table of a parser
* In: 
*	w:			XmHTMLParserObject id
*	unknown:	unknown element for which an alias is being added
*	alias:		alias to use for this unknown element.
* Returns:
*	True when alias table was successfully added. False if not.
*****/
Boolean
XmHTMLParserAddAlias(TWidget w, String element, htmlEnum alias)
{
	static XmHTMLAliasTable new_table;
	int nalias;
	XmHTMLAliasTable old_table;
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserAddAlias");
		return(False);
	}

	parser = XmHTMLParser(w);

	/* sanity check */
	if(element == NULL || !*element || alias < 0 || alias > HT_ZTEXT)
		return(False);

	nalias = ATTR(nalias);
	old_table = ATTR(alias_table);

	new_table = addAliasToTable(parser, old_table, &nalias, element, alias);

	if(new_table == NULL)
		return(False);

	/* set new alias table */
	ATTR(alias_table) = new_table;
	ATTR(nalias)      = nalias;

	return(True);
}

/*****
* Name: 		XmHTMLParserRemoveAlias
* Return Type: 	nothing
* Description: 	removes an alias from the alias table of a parser
* In: 
*	w:			XmHTMLParserObject id.
*	unknown:	unknown element for which the alias is to be removed
*	alias:		alias used for this unknown element.
* Returns:
*	nothing, but the alias table for this parser is updated.
*****/
Boolean
XmHTMLParserRemoveAlias(TWidget w, String element, htmlEnum alias)
{
	static XmHTMLAliasTable new_table;
	XmHTMLAliasTable old_table;
	int nalias;
	Boolean error = False;
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserRemoveAlias");
		return(False);
	}

	parser = XmHTMLParser(w);

	/* sanity check */
	if(element == NULL || !*element || alias < 0 || alias > HT_ZTEXT ||
		ATTR(nalias) == 0)
		return(False);

	nalias    = ATTR(nalias);
	old_table = ATTR(alias_table);

	new_table = removeAliasFromTable(parser, old_table, &nalias, element,
		alias, &error);

	if(error)
		return(False);

	/* set new alias table */
	ATTR(alias_table) = new_table;
	ATTR(nalias)      = nalias;

	return(True);
}

/*****
* Name: 		XmHTMLParserDestroyAliasTable
* Return Type: 	void
* Description: 	releases the given alias table.
* In: 
*	table:		alias table to be freed.
*	nalias:		no of elements in the given table.
* Returns:
*	nothing.
*****/
void
XmHTMLParserDestroyAliasTable(XmHTMLAliasTable table, int nalias)
{
	int i;

	/* sanity */
	if(table == NULL || nalias == 0)
		return;

	/* free all elements */
	for(i = 0; i < nalias; i++)
		free(table[i].element);

	/* free parser table */
	free(table);

	table = NULL;
}

/*****
* Name: 		XmHTMLParserFreeAliasTable
* Return Type: 	void
* Description: 	frees up the alias table for the given parser.
* In: 
*	w:			XmHTMLParserObject id
* Returns:
*	nothing.
*****/
void
XmHTMLParserFreeAliasTable(TWidget w)
{
	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, "XmHTMLParserFreeAliasTable");
		return;
	}

	parser = XmHTMLParser(w);

	/* sanity */
	if(ATTR(nalias) == 0)
		return;

	/* free parser table */
	XmHTMLParserDestroyAliasTable(ATTR(alias_table), ATTR(nalias));

	/* reset alias table fields */
	ATTR(alias_table) = NULL;
	ATTR(nalias)      = 0;
}

/*****
* Name: 		XmHTMLParserGetAliasTable
* Return Type: 	XmHTMLAliasTable
* Description: 	returns the current parser's alias table
* In: 
*	w:			XmHTMLParserObject for which to get the alias table
*	nalias:		no of aliases in alias table, updated upon return
* Returns:
*	the current alias table of the given parser. Return value must be
*	freed using XmHTMLDestroyAliasTable().
*****/
XmHTMLAliasTable
XmHTMLParserGetAliasTable(TWidget w, int *nalias)
{
	static XmHTMLAliasTable table;
	int copied;
	static String func = "XmHTMLParserGetAliasTable";

	XmHTMLParserObject parser;

	if(w == NULL || !XmIsHTMLParser(w))
	{
		_XmHTMLBadParent(w, func);
		return(NULL);
	}

	parser = XmHTMLParser(w);

	if(ATTR(nalias) == 0)
	{
		*nalias = 0;
		return(NULL);
	}

	/* make a copy of the current table */
	table = copyAliasTable(ATTR(alias_table), ATTR(nalias), &copied);

	if(copied != ATTR(nalias))
	{
		_XmHTMLWarning(__WFUNC__(w, func),
			"Corrupted HTML alias table, releasing it.");
		/* destroy copy */
		XmHTMLParserDestroyAliasTable(table, copied);

		/* and give up on the current alias table as well */
		XmHTMLParserFreeAliasTable(w);
		return(NULL);
	}

	*nalias = copied;

	return(table);
}

/*****
* Name: 		XmHTMLGetGlobalAliasTable
* Return Type: 	XmHTMLAliasTable
* Description: 	returns a copy of the global alias table used by all parsers.
* In: 
*	nalias:		no of elements in the global alias table. Updated upon return.
* Returns:
*	a copy of the global alias table.
*****/
XmHTMLAliasTable
XmHTMLGetGlobalAliasTable(int *nalias)
{
	static XmHTMLAliasTable global_table;
	int i;

	/* global html token table is zero based */
	global_table = (XmHTMLAliasTable)calloc(HT_ZTEXT+1, sizeof(XmHTMLAlias));

	/* fill it with all known elements */
	for(i = 0; i < HT_ZTEXT+1; i++)
	{
		global_table[i].element = strdup(html_tokens[i]);
		global_table[i].alias = (htmlEnum)i;
	}
	*nalias = (int)HT_ZTEXT+1;
	return(global_table);
}
