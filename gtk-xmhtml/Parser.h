/*****
* Parser.h : xmHTMLParserObjectClass public header file
*
* This file Version	$Revision$
*
* Creation date:		Sun Apr 13 00:58:43 GMT+0100 1997
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
* Revision 1.2  1997/10/23 00:24:37  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.1  1997/04/29 14:19:25  newt
* Initial Revision
*
*****/ 

#ifndef _Parser_h_
#define _Parser_h_

#include <XmHTML/toolkit.h>
#include <XmHTML/HTML.h>

_XFUNCPROTOBEGIN

#ifdef WITH_MOTIF
#   include "Parser-motif.h"
#endif

/********    Public Function Declarations    ********/

/* Quick method for enabling/disabling XmNparserCallback honoring */
extern void XmHTMLParserSetAutoMode(TWidget w, Boolean set);

/* Quick method for enabling/disabling XmNparserIsProgressive resource */
extern void XmHTMLParserSetProgressiveMode(TWidget w, Boolean set);

/*****
* Set and parse a new HTML source. Resets and clears any old source.
* When source is NULL, any existing source and parser tree is cleared.
*****/
extern Boolean XmHTMLParserSetString(TWidget w, String source);

/*****
* Update the current HTML source when in progressive mode and start
* parsing the new part. Returns True when successfully set and parsed,
*
* Returns False if parser isn't in progressive mode, the new source matches
* the old source exactly, the new source is smaller than the current source,
* the lower part of the old source doesn't match the current source or when
* the new source is NULL.
* When returning False, this function also calls any installed 
* XmNdocumentCallback callback resources.
*****/
extern Boolean XmHTMLParserUpdateSource(TWidget w, String source);

/* Perform parser wrapup and call any installed XmNdocumentCallbacks */
extern Boolean XmHTMLParserEndSource(TWidget w);

/* expand all character escape sequences in the given text */
extern void XmHTMLParserExpandEscapes(String source);

/*****
* Terminate the current parsing process. Calls any installed
* XmNdocumentCallback callback resources with the Terminated field set to True.
*****/
extern void XmHTMLParserTerminateSource(TWidget w);

/* Get the original source text */
extern String XmHTMLParserGetSource(TWidget w); 

/* Create a HTML source from the parser tree */
extern String XmHTMLParserGetString(TWidget w);

/* Return the generated parser tree. May never be freed. */
extern XmHTMLObject *XmHTMLParserGetTree(TWidget w);

/*****
* Unknown HTML alias table routines 
* All routines that return an alias table return a copy of the requested
* alias table which must be freed by the caller.
* All routines that install an alias or alias table make a copy of the table
* or alias to be installed.
*****/
/* sets an alias table in the given parser. */
extern int XmHTMLParserSetAliasTable(TWidget w, XmHTMLAliasTable alias_table,
	int nalias);

/* returns the current alias table of the given parser. */
extern XmHTMLAliasTable XmHTMLParserGetAliasTable(TWidget w, int *nalias);

/* add a single alias to the alias table of a parser */
extern Boolean XmHTMLParserAddAlias(TWidget w, String element, htmlEnum alias);

/* remove an alias from the alias table of a parser. */
extern Boolean XmHTMLParserRemoveAlias(TWidget w, String element,
	htmlEnum alias);

/* free the alias table of a parser */
extern void XmHTMLParserFreeAliasTable(TWidget w);

/* Return a copy the global alias table used by all parsers */
extern XmHTMLAliasTable XmHTMLGetGlobalAliasTable(int *nalias);

/* free the given alias table */
extern void XmHTMLParserDestroyAliasTable(XmHTMLAliasTable table, int nalias);

_XFUNCPROTOEND

/* Don't add anything after this endif! */
#endif /* _Parser_h_ */
