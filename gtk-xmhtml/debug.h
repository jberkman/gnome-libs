/*****
* debug.h : ntUtil debug defines
*
* This file Version	$Revision$
*
* Creation date:		Fri Oct 18 04:52:25 GMT+0100 1996
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
* $Source$
*****/
/*****
* ChangeLog 
* $Log$
* Revision 1.1  1997/11/28 03:38:56  gnomecvs
* Work in progress port of XmHTML;  No, it does not compile, don't even try -mig
*
* Revision 1.5  1997/08/30 00:47:36  newt
* debug to file changes.
*
* Revision 1.4  1997/08/01 12:58:54  newt
* Updated function protos.
*
* Revision 1.3  1997/03/02 23:16:20  newt
* Modified header files.
*
* Revision 1.2  1997/01/09 06:56:02  newt
* expanded copyright marker
*
* Revision 1.1  1996/12/19 02:17:19  newt
* Initial Revision
*
*****/ 

#ifndef _debug_h_
#define _debug_h_

/* obliterate when NO_DEBUG has been defined */
#if defined(DEBUG) && defined(NO_DEBUG)
# ifdef DEBUG
#  undef DEBUG
# endif
#endif

#ifndef NO_DEBUG

#define MAX_DEBUG_LEVELS	32

/* array of selected debug levels */
extern int xmhtml_debug_levels_defined[];
/* full debug selection flag */
extern int xmhtml_debug_full;
/* handle to output file */
extern FILE *__rsd_debug_file;

extern int  __rsd_selectDebugLevels(char *levels);
extern void __rsd_setDebugLevels(int *argc, char **argv);
extern void __rsd_initDebug(int initial);
extern void __rsd_fprintf(String fmt, ...);

#endif /* NO_DEBUG */

#ifdef DEBUG
/* 
* Select possible debug levels 
* levels _can_ start with a ``-d'', so you can call this routine
* from a routine that processes the command line options.
* Calling this routine with a NULL param deselects all debug levels
* Calling this routine with the string "all" selects all debug levels.
*/
#define _XmHTMLSelectDebugLevels(LEVELS)	__rsd_selectDebugLevels(LEVELS)
#define _XmHTMLSetDebugLevels(ARGC, ARGV)	__rsd_setDebugLevels(ARGC, ARGV)
#define _XmHTMLInitDebug(LEVEL)				__rsd_initDebug(LEVEL)
#define _XmHTMLSetFullDebug(STATE)			xmhtml_debug_full = STATE

/* display a debug message */
#define _XmHTMLDebug(LEVEL,MSG) do {\
	if(xmhtml_debug_levels_defined[LEVEL]) \
		{ __rsd_fprintf MSG;} \
	 }while(0)

/*
* flush a message to the output file. Mainly intended for XmHTML's warning
* & error routines.
*/
#define _XmHTMLDebugMirrorToFile(MSG) do {\
	if(__rsd_debug_file != stdout) __rsd_fprintf MSG; \
}while(0)

/* display a debug message */
#define _XmHTMLFullDebug(LEVEL,MSG) do {\
	if(xmhtml_debug_levels_defined[LEVEL] && xmhtml_debug_full == True) \
		{ __rsd_fprintf MSG;} \
 }while(0)

#else	/* !DEBUG */

#define _XmHTMLDebug(LEVEL,MSG)		 		/* empty */
#define _XmHTMLFullDebug(LEVEL,MSG)	 		/* empty */
#define _XmHTMLDebugMirrorToFile(MSG)		/* empty */
#define _XmHTMLSelectDebugLevels(LEVELS)	/* empty */
#define _XmHTMLSetDebugLevels(ARGC, ARGV)	/* empty */
#define _XmHTMLSetFullDebug(STATE)			/* empty */
#define _XmHTMLInitDebug(LEVEL)				/* empty */

#endif	/* DEBUG */

/* Don't add anything after this endif! */
#endif /* _debug_h_ */
