/*****
* XmHTMLfuncs.h : widely used functions and overall configuration settings.
*
* This file Version	$Revision$
*
* Creation date:		Tue Dec  3 15:00:14 GMT+0100 1996
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
* Revision 1.2  1997/12/19 00:03:49  unammx
* gtk/xmhtml updates
*
* Revision 1.1  1997/11/28 03:38:56  gnomecvs
* Work in progress port of XmHTML;  No, it does not compile, don't even try -mig
*
* Revision 1.16  1997/10/23 00:24:48  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.15  1997/08/30 00:43:02  newt
* HashTable stuff. Changed proto's for almost every routine in here.
*
* Revision 1.14  1997/08/01 12:56:02  newt
* Progressive image loading changes. Changes to debug memory alloc protos.
*
* Revision 1.13  1997/05/28 01:43:34  newt
* Added protos and defines for debug memory allocation functions.
*
* Revision 1.12  1997/04/29 14:23:32  newt
* Moved all XmHTML private functions to XmHTMLP.h
*
* Revision 1.11  1997/04/03 05:32:52  newt
* ImageInfoShared macro. _XmHTMLLoadBodyImage proto
*
* Revision 1.10  1997/03/28 07:06:42  newt
* Frame interface prototypes from frames.c
*
* Revision 1.9  1997/03/20 08:07:25  newt
* added external html_tokens definition, _XmHTMLReplaceOrUpdateImage
*
* Revision 1.8  1997/03/11 19:52:05  newt
* ImageBuffer; XmHTMLImage and XmImageInfo macros; new protos for animated Gifs
*
* Revision 1.7  1997/03/04 18:47:01  newt
* _XmHTMLDrawImagemapSelection proto added
*
* Revision 1.6  1997/03/04 00:57:29  newt
* Delayed Image Loading: _XmHTMLReplaceImage and _XmHTMLUpdateImage
*
* Revision 1.5  1997/03/02 23:14:13  newt
* malloc defines; function proto's for all private image/imagemap routines
*
* Revision 1.4  1997/02/11 02:02:57  newt
* Changes for NEED_STRCASECMP
*
* Revision 1.3  1997/01/09 06:55:59  newt
* expanded copyright marker
*
* Revision 1.2  1997/01/09 06:48:43  newt
* updated function definitions
*
* Revision 1.1  1996/12/19 02:17:18  newt
* Initial Revision
*
*****/ 

#ifndef _XmHTMLfuncs_h_
#define _XmHTMLfuncs_h_

#include <errno.h>
#ifdef WITH_MOTIF
#include <X11/IntrinsicP.h>		/* for Widget definition & fast macros */
#endif

#ifndef BYTE_ALREADY_TYPEDEFED
#define BYTE_ALREADY_TYPEDEFED
typedef unsigned char Byte;
#endif /* BYTE_ALREADY_TYPEDEFED */

/*****
* The top section of this file contains a number of default values that
* can only be set at compile-time.
* These values modify the default behaviour of the XmHTML widget, so be
* carefull when modifying these values.
*****/

/*** 
* button press & release must occur within half a second of each other to 
* trigger an anchor activation
***/
#define MAX_RELEASE_TIME	500		

/* Default margin offsets */
#define DEFAULT_MARGIN		20

/* initial horizontal & vertical increment when a scrollbar is moved */
#define HORIZONTAL_INCREMENT	12		/* average char width */
#define VERTICAL_INCREMENT		18		/* average line height */

/*****
* maximum no of colors we allow. You can safely decrease this number, but
* *never* increase it.
*****/
#define MAX_IMAGE_COLORS	256

/*****
* Default gamma correction value for your display. This is only used for
* images that support gamma correction (JPEG and PNG).
* 2.2 is a good assumption for almost every X display.
* For a Silicon Graphics displays, change this to 1.8
* For Macintosh displays (MkLinux), change this to 1.4 (so I've been told)
* If you change this value, it *must* be a floating point value.
*****/
#define XmHTML_DEFAULT_GAMMA	2.2

/*****
* Maximum size of the PLC get_data() buffer. This is the maximum amount
* of data that will be requested to a function installed on the
* XmNprogressiveReadProc. Although this define can have any value, using
* a very small value will make progressive loading very slow, while using
* a large value will make the response of XmHTML slow while any PLC's are
* active.
* The first call to the get_data() routine will request PLC_MAX_BUFFER_SIZE
* bytes, while the size requested by any following calls will depend on the
* type of image being loaded and the amount of data left in the current input
* buffer.
*****/
#define PLC_MAX_BUFFER_SIZE		2048

/*****
* The default timeout value for the Progressive Loader Context. This
* timeout is the default value for the XmNprogressiveInitialDelay and
* specifies the polling interval between subsequent PLC calls.
* 
* Specified in milliseconds (1 second = 1000 milliseconds)
* XmHTML dynamically adjusts the timeout value as necessary and recomputes
* it after each PLC call.
* PLC_MIN_DELAY is the minimum value XmHTML can reduce the timeout to while
* PLC_MAX_DELAY is the maximum value XmHTML can increase the timeout to.
*****/
#define PLC_DEFAULT_DELAY		250
#define PLC_MIN_DELAY			5
#define PLC_MAX_DELAY			1000

/***************** End of User configurable section *****************/ 

/*****
* magic number for the XmHTMLImage structure. XmHTML uses this field to verify
* the return value from a user-installed primary image cache.
*****/
#define XmHTML_IMAGE_MAGIC		0xce

/* lint kludge */
#ifdef lint
#undef True
#undef False
#define True ((Boolean)1)
#define False ((Boolean)0)
#endif /* lint */

extern Byte bitmap_bits[];

/* usefull macros */
#define Abs(x)		((x) < 0 ? -(x) : (x))
#define Max(x,y)	(((x) > (y)) ? (x) : (y))
#define Min(x,y)	(((x) < (y)) ? (x) : (y))
#define FONTHEIGHT(f) ((f)->max_bounds.ascent + (f)->max_bounds.descent)
#define FnHeight(f) ((f)->ascent + (f)->descent)

#ifdef WITH_MOTIF
#    define FreePixmap(DPY,PIX) if((PIX)!= None) XFreePixmap((DPY),(PIX))
#else
#    define FreePixmap(DPY,PIX) if((PIX)!= TNone) gdk_pixmap_unref((PIX))
#endif

/* check whether the body image is fully loaded */
#define BodyImageLoaded(IMAGE) \
	((IMAGE) ? (!ImageInfoDelayed((IMAGE)) && \
		!ImageInfoProgressive((IMAGE))) : True)

/* RANGE forces a to be in the range b..c (inclusive) */
#define RANGE(a,b,c) { if (a < b) a = b;  if (a > c) a = c; }

/****
* debug.c
* Must include this before anything else to prevent inconsistencies.
****/
#include "debug.h"

/* Normal builds use Xt memory functions */
#if !defined(DMALLOC) && !defined(DEBUG)
#    ifdef WITH_MOTIF
#        define malloc(SZ)		XtMalloc((SZ))
#        define calloc(N,SZ)		XtCalloc((N),(SZ))
#        define realloc(PTR,SZ)		XtRealloc((char*)(PTR),(SZ))
#        define free(PTR)		XtFree((char*)(PTR))
#        define strdup(STR)		XtNewString((STR))
#    else
#        define malloc(SZ)              g_malloc(SZ)
#        define calloc(N,SZ)            g_malloc0((SZ)*(N))
#        define realloc(PTR,SZ)         g_realloc(PTR,SZ)
#        define free(PTR)               g_free(PTR)
#        define strdup(STR)             g_strdup(STR)
#    endif
#elif !defined(DMALLOC)

/* debug builds use asserted functions unless DMALLOC is defined */
extern char *__rsd_malloc(size_t size, char *file, int line);
extern char *__rsd_calloc(size_t nmemb, size_t size, char *file, int line);
extern char *__rsd_realloc(void *ptr, size_t size, char *file, int line);
extern char *__rsd_strdup(const char *s1, char *file, int line);
extern void  __rsd_free(void *ptr, char *file, int line);

/* every source file has a static variable called src_file */
#define malloc(SZ)			__rsd_malloc((SZ), __FILE__, __LINE__)
#define calloc(N,SZ)		__rsd_calloc((N),(SZ), __FILE__, __LINE__)
#define realloc(PTR,SZ)		__rsd_realloc((PTR),(SZ), __FILE__, __LINE__)
#define free(PTR)			__rsd_free((PTR), __FILE__, __LINE__)
#define strdup(STR)			__rsd_strdup((STR), __FILE__, __LINE__)

#else /* DMALLOC */

/* let dmalloc.h define it all */
#include <dmalloc.h>

#endif /* DEBUG && DMALLOC */

/* global debug messages disabling */
#ifdef DEBUG
extern Boolean debug_disable_warnings;

/* macro to display an error message and dump the core */
#define my_assert(TST) if((TST) != True) do { \
	fprintf(stderr, "Assertion failed: %s\n    (file %s, line %i)\n", \
		#TST, __FILE__, __LINE__); \
	abort(); \
}while(0)

#else

#define my_assert(TST)	/* empty */

#endif

/****
* error.c
* We've got two separate versions of XmHTML's error & warning functions.
* The debug versions include full location information while the normal
* build versions only contain the warning/error message. This allows us
* to reduce the data size of the normal build.
****/
#ifdef DEBUG

#define __WFUNC__(WIDGET_ID, FUNC)	(TWidget)WIDGET_ID, __FILE__, \
	 __LINE__, FUNC

/* Display a warning message and continue */
extern void __XmHTMLWarning(TWidget w, String module, int line, String routine, 
	String fmt, ...);

/* Display an error message and exit */
extern void __XmHTMLError(TWidget w, String module, int line, String routine, 
		String fmt, ...);

/* Display a NULL/invalid parent warning message and continue */
extern void __XmHTMLBadParent(TWidget w, String src_file, int line, String func);

#define _XmHTMLBadParent(W,FUNC)	__XmHTMLBadParent(W,__FILE__,__LINE__,FUNC)

#else

#define __WFUNC__(WIDGET_ID, FUNC)	(TWidget)WIDGET_ID

/* Display a warning message and continue */
extern void __XmHTMLWarning(TWidget w, String fmt, ...);

/* Display an error message and exit */
extern void __XmHTMLError(TWidget w, String fmt, ...);

/* Display a NULL/invalid parent warning message and continue */
extern void __XmHTMLBadParent(TWidget w, String func);

#define _XmHTMLBadParent(W,FUNC)	__XmHTMLBadParent(W,FUNC)

#endif /* DEBUG */

#define _XmHTMLWarning __XmHTMLWarning
#define _XmHTMLError   __XmHTMLError

/* Display an error message due to allocation problems and exit */
extern void _XmHTMLAllocError(TWidget w, char *module, char *routine, 
	char *func, int size);

/****
* StringUtil.c
****/
extern void my_upcase(char *string);
extern void my_locase(char *string);
extern char *my_strcasestr(const char *s1, const char *s2);
extern char *my_strndup(const char *s1, size_t len);
extern Byte __my_translation_table[];
#define _FastLower(x) (__my_translation_table[(unsigned int)x])

#ifdef NEED_STRERROR
extern char *sys_errlist[];
extern int errno;
#define strerror(ERRNUM) sys_errlist[ERRNUM]
#endif

#ifdef NEED_STRCASECMP
# include <sys/types.h>
extern int my_strcasecmp(const char *s1, const char *s2);
extern int my_strncasecmp(const char *s1, const char *s2, size_t n);
#define strcasecmp(S1,S2) my_strcasecmp(S1,S2)
#define strncasecmp(S1,S2,N) my_strncasecmp(S1,S2,N)
#endif

typedef struct _HashEntry{
	struct _HashEntry *nptr;
	struct _HashEntry *pptr;    /* linked list */
	unsigned long key;
	unsigned long data;
#ifdef DEBUG
	int ncoll;					/* no of collisions for this entry */
#endif
	struct _HashEntry *next;		/* next on the linked-list for collisions */
}HashEntry;

/* A generic hash table structure */
typedef struct _HashTable{
	int elements;				/* elements stored in the table */
	int size;					/* size of the table */
	HashEntry **table;
	HashEntry *last;			/* last on the linked list */
#ifdef DEBUG
	int requests, hits, misses, puts, collisions;
#endif
}HashTable;

/* initialize the given hashtable. */
extern HashTable *_XmHTMLHashInit(HashTable *table);

/* put a new entry in the hashtable */
extern void _XmHTMLHashPut(HashTable *table, unsigned long key,
	unsigned long data);

/* get an entry from the hashtable */
extern Boolean _XmHTMLHashGet(HashTable *table, unsigned long key,
	unsigned long *data);

/* delete an entry from the hashtable */
extern void _XmHTMLHashDelete(HashTable *table,
	unsigned long key);

/* completely wipe the given hashtable */
extern void _XmHTMLHashDestroy(HashTable *table);

/*****
* timings only available when compiled with GCC and when requested. 
* Defining _WANT_TIMINGS yourself doesn't have *any* effect, its defined in
* source files where I want to known how much time a routine requires to
* perform it's task (crude profiling).
*****/
#if defined(DEBUG) && defined(_WANT_TIMINGS) && defined(__GNUC__)
#include <sys/time.h>	/* timeval def */
#include <unistd.h>		/* gettimeofday() */

static struct timeval tstart, tend;
#define SetTimer gettimeofday(&tstart,NULL)
#define ShowTimer(LEVEL, FUNC) do { \
	int secs, usecs; \
	gettimeofday(&tend, NULL); \
	secs = (int)(tend.tv_sec - tstart.tv_sec); \
	usecs = (int)(tend.tv_usec - tstart.tv_usec); \
	if(usecs < 0) usecs *= -1; \
	_XmHTMLDebug(LEVEL,("%s: done in %i.%i seconds\n",FUNC,secs,usecs)); \
}while(0)

#else
#define SetTimer				/* empty */
#define ShowTimer(LEVEL,FUNC)	/* empty */
#endif

/* Don't add anything after this endif! */
#endif /* _XmHTMLfuncs_h_ */
