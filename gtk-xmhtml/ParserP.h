/*****
* ParserP.h : HTMLParser Object private header file
*
* This file Version	$Revision$
*
* Creation date:		Sun Apr 13 00:58:46 GMT+0100 1997
* Last modification: 	$Date$
* By:					$Author$
* Current State:		$State$
*
* Author:				newt
*
* Copyright (C) 1994-1997 by Ripley Software Development 
* All Rights Reserved
*
* This file is part of the XmHTML Widget Library
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
* $Source$
*****/
/*****
* ChangeLog 
* $Log$
* Revision 1.1  1997/11/28 03:38:54  gnomecvs
* Work in progress port of XmHTML;  No, it does not compile, don't even try -mig
*
* Revision 1.2  1997/10/23 00:24:39  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.1  1997/04/29 14:19:29  newt
* Initial Revision
*
*****/ 

#ifndef _ParserP_h_
#define _ParserP_h_

#ifdef WITH_MOTIF
#    include <X11/IntrinsicP.h>
#    include <X11/ObjectP.h>
#else
#    include <XmHTML/gtk.h>
#endif

#include <XmHTML/Parser.h>

_XFUNCPROTOBEGIN

#ifndef BYTE_ALREADY_TYPEDEFED
#define BYTE_ALREADY_TYPEDEFED
typedef unsigned char Byte;
#endif /* BYTE_ALREADY_TYPEDEFED */

#ifdef WITH_MOTIF
/*****
* Class pointer and extension record definition
*****/
typedef struct {
	XtPointer extension;					/* Pointer to extension record */
}XmHTMLParserClassPart;

typedef struct _XmHTMLParserClassRec
{
	ObjectClassPart object_class;			/* parent class */
	XmHTMLParserClassPart html_parser_class;
}XmHTMLParserClassRec;
#endif /* WITH_MOTIF */

/*****
* Parser state stack object
*****/
typedef struct _stateStack{
	htmlEnum id;							/* current state id */
	struct _stateStack *next;				/* ptr to next record */
}stateStack;

/*****
* HTMLParserPart definition
*****/
typedef struct _XmHTMLParserPart {
	/****
	* Public members
	****/
	String				mime_type;			/* mime type of this text */

	/* parser configuration */
	Boolean				strict;				/* strict HTML 3.2 checking */
	Boolean				progressive;		/* parser mode */
	Boolean				retain_source;		/* retain source after parsing */

	/* callback resources */
	TCallbackList		document_callback;
	TCallbackList		parser_callback;
	TCallbackList		modify_verify_callback;

	/* user data field */
	TPointer			user_data;			/* not used by the parser */

	String				unknown_string;		/* HTML_UNKNOWN_ELEMENT */
	String				open_string;		/* HTML_OPEN_ELEMENT */
	String				bad_html_string;	/* HTML_BAD */
	String				open_block_string;	/* HTML_OPEN_BLOCK */
	String				close_block_string;	/* HTML_CLOSE_BLOCK */
	String				violation_string;	/* HTML_VIOLATION */
	String				notify_string;		/* HTML_NOTIFY */
	String				fatal_string;		/* HTML_INTERNAL */

	/****
	* Private members
	****/
	/* parser aliasing table */
	XmHTMLAliasTable	alias_table;		/* unknown element aliasing table */
	Cardinal			nalias;				/* no of elements in alias table */

	String				source;				/* raw HTML text */
	int					is_html;			/* only false for text/plain */

	XmHTMLObject		*objects;			/* final parser tree */
	XmHTMLObject		*head;				/* head of parser tree */
	XmHTMLObject		*current;			/* last known processed element */
	Cardinal			nobjects;			/* object count */
	Cardinal			nelements;			/* HTML object count */
	Cardinal			ntext;				/* plain text object count */

	Cardinal			source_len;			/* current source length */
	Cardinal			loop_count;			/* total no of loops made */

	Cardinal			index;				/* current text index */
	int					inserted;			/* no of auto-inserted chars */

	Cardinal			line_len;			/* maximum line length */
	int					cnt;				/* current line length */
	Cardinal			num_lines;			/* current line no in source */

	Cardinal			cstart;				/* current element start position*/
	Cardinal			cend;				/* current element end position */
	Cardinal			err_count;			/* bad HTML error count */

	stateStack			base;				/* stack base point */
	stateStack			*stack;				/* actual stack */
	int					depth;				/* current stack depth */

	Boolean				automatic;			/* honor parser callback */
	Boolean				unbalanced;			/* true when stack is unbalanced */
	Boolean				html32;				/* html conformancy flag */
	Boolean				have_body;			/* seen the body tag */
	Boolean				reset;				/* parser has been reset */
	Boolean				active;				/* parser is active */
	Boolean				terminated;			/* premature end */
}XmHTMLParserPart;

#ifdef WITH_MOTIF
typedef struct _XmHTMLParserRec {
	ObjectPart object;
	XmHTMLParserPart parser;
}XmHTMLParserRec;

externalref XmHTMLParserClassRec xmHTMLParserClassRec;
#endif

/*****
* Private functions
*****/

/* free the given object list */
extern void _XmHTMLParserFreeObjects(XmHTMLObject *objects);

/* compose a HTML source document from the given list of objects */
extern String _XmHTMLParserGetString(XmHTMLObject *objects);

#ifndef WITH_MOTIF
#   include "gtk-xmhtml-parser.h"
#endif

_XFUNCPROTOEND

/* Don't add anything after this endif! */
#endif /* _ParserP_h_ */
