#ifndef lint
static char rcsId[]="$Header$";
#endif
/*****
* StringUtil.c:  badly named file with misc. routines that don't fit anywhere
*                else.
*
* This file Version	$Revision$
*
* Creation date:		Wed May 29 22:35:32 GMT+0100 1996
* Last modification: 	$Date$
* By:					$Author$
* Current State:		$State$
*
* Author:				newt
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*
* Hashing routines Copyright (c) 1997 Alfredo K. Kojima
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
* Source History:
* ForUtil-0.52
* newt
*****/
/*****
* ChangeLog 
* $Log$
* Revision 1.1  1997/11/28 03:38:54  gnomecvs
* Work in progress port of XmHTML;  No, it does not compile, don't even try -mig
*
* Revision 1.8  1997/10/23 00:24:40  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.7  1997/08/30 00:28:24  newt
* HashTable routines added. Changed my_ prefix to __rsd_.
*
* Revision 1.6  1997/08/01 12:53:38  newt
* Modified debugging memory allocation routines to show file and line info.
*
* Revision 1.5  1997/05/28 01:31:26  newt
* Added debug versions of all memory allocation routines using assertions.
*
* Revision 1.4  1997/04/29 14:20:15  newt
* Prefixed all functions with my_ to prevent name conflicts with libwww3.
* Added my_strndup
*
* Revision 1.3  1997/03/02 23:04:39  newt
* changed unsigned char to Byte
*
* Revision 1.2  1997/02/11 02:04:09  newt
* Added strcasecmp/strncasecmp
*
* Revision 1.1  1997/01/09 06:54:51  newt
* expanded copyright marker
*
* Revision 2.0  1996/09/19 02:45:27  newt
* Updated for source revision 2.0
*
* Revision 1.1  1996/06/27 03:53:51  newt
* Initial Revision. 
* Originally comes from ForUtil-0.52, but has been adapted for Newt.
*
*****/ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>	/* toupper, tolower */
#include <sys/types.h>

#include <XmHTML/toolkit.h>
#include "XmHTMLfuncs.h"

/*** 
* Character translation table for converting from upper to lower case 
* Since this is a table lookup, it might perform better than the libc
* tolower routine on a number of systems.
***/

Byte __my_translation_table[256]={
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
	24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,
	45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,97,98,
	99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
	116,117,118,119,120,121,122,91,92,93,94,95,96,97,98,99,100,101,102,
	103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,
	120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,
	137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,
	154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,
	171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,
	188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,
	205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,
	222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,
	239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};

static void table_idestroy(HashTable *table);

static HashEntry * delete_fromilist(HashTable *table, HashEntry *entry,
	unsigned long key);

static unsigned long hash_ikey(unsigned long key);

static void rebuild_itable(HashTable *table);

/*****
* Name: 		my_upcase
* Return Type: 	void
* Description: 	makes a string all uppercase
* In: 
*	string:		string to translate to uppercase
* Returns:
*	nothing, string is changed upon return.
*****/
void
my_upcase(char *string)
{
	register char *outPtr = string;
	for(outPtr = string; *outPtr != '\0'; 
		*(outPtr++) = toupper(*(string++)));
}

/*****
* Name: 		my_locase
* Return Type: 	void
* Description: 	make a string all lower case
* In: 
*	string:		string to translate to lowercase
* Returns:
*	nothing, string is changed upon return.
*****/
void
my_locase(char *string)
{
	register char *outPtr = string;
	for(outPtr = string; *outPtr != '\0'; 
		*(outPtr++) = _FastLower(*(string++)));
}

/*****
* Name: 		my_strcasestr
* Return Type: 	char *
* Description: 	returns the starting address of s2 in s1, ignoring case
* In: 
*	s1:			string to examine
*	s2:			string to find
* Returns:
*	a ptr to the position in s1 where s2 is found, or NULL if s2 is not found.
*****/
char *
my_strcasestr(const char *s1, const char *s2)
{
	register int i;
	register const char *p1, *p2, *s = s1;

	for (p2 = s2, i = 0; *s; p2 = s2, i++, s++)
	{
		for (p1 = s; *p1 && *p2 && _FastLower(*p1) == _FastLower(*p2); 
				p1++, p2++)
			;
		if(!*p2)
			break;
	}
	if (!*p2)
		return((char*)s1 + i);
	return 0;
}

#ifdef DEBUG
/*****
* Name: 		__rsd_strdup
* Return Type: 	char*
* Description: 	debugging version of strdup
* In: 
*	s1:			string to be duplicated
* Returns:
*	duplicated string.
*****/
char*
__rsd_strdup(const char *s1, char *file, int line)
{
	static char *ret_val;

	/* dump if failed */
	my_assert(s1 != NULL);

	ret_val = malloc(strlen(s1)+1);
	strcpy(ret_val, s1);
	return(ret_val);
}
#endif

/*****
* Name: 		my_strndup
* Return Type: 	char*
* Description: 	duplicates up to len chars of string s1
* In: 
*	s1:			source string;
*	len:		max no of chars to copy;
* Returns:
*	a ptr to the duplicated string, padded with NULL if len is larger then
*	s1. Return value is always NULL terminated.
*****/
char *
my_strndup(const char *s1, size_t len)
{
	register int i;
	register char *p2;
	register const char *p1 = s1;
	static char *s2;

	/* no negative lengths */
	if(len < 0 || s1 == NULL || *s1 == '\0')
		return(NULL);

	/* size of text + a terminating \0 */
	s2 = (char*)malloc(len+1);

	for(p2 = s2, i = 0; *p1 && i < len; *(p2++) = *(p1++), i++);

	/* NULL padding */
	while(i++ < len)
		*(p2++) = '\0';

	*p2 = '\0';	/* NULL terminate */

	return(s2);
}

/*****
* UnixWare doesn't have these functions in its standard C library 
* contributed by Thanh Ma (tma@encore.com), fix 02/03/97-03, tma
*****/

#ifdef NEED_STRCASECMP
/*****
* Name: 		strncasecmp
* Return Type: 	int
* Description: 	case insensitive string compare upto n characters of string
*				s1.
* In: 
*	s1:			source string
*	s2:			string to compare with
*	n:			no of characters to compare.
* Returns:
*	0 when they match, character difference otherwise.
*****/
int
my_strncasecmp (const char *s1, const char *s2, size_t n)
{
	register int c1, c2, l=0;

	while (*s1 && *s2 && l < n)
	{
		c1 = _FastLower(*s1);
		c2 = _FastLower(*s2);
		if (c1 != c2)
			return(c1 - c2);
		s1++;
		s2++;
		l++;
	}
	return((int)(0));
}

/*****
* Name: 		strcasecmp
* Return Type: 	int
* Description: 	case insensitive string compare 
* In: 
*	s1:			source string
*	s2:			string to compare with
* Returns:
*	0 when they match, character difference otherwise.
*****/
int
my_strcasecmp (const char *s1, const char *s2)
{
	register int c1, c2;

	while (*s1 && *s2)
	{
		c1 = _FastLower(*s1);
		c2 = _FastLower(*s2);
		if (c1 != c2)
			return(c1 - c2);
		s1++;
		s2++;
	}                                                                           
	return((int)(*s1 - *s2));
}
#endif /* NEED_STRCASECMP */

/*****
* hashing functions.
*****/

#define INIT_TABLE_SIZE	512		/* aligned color hash */

/*****
* Private routines
******/

/*****
* Name: 		table_idestroy
* Return Type: 	void
* Description: 	frees the table of a given hashtable. Only used when table
*				is being rebuild.
* In: 
*	table:		table to be destroyed;
* Returns:
*	nothing.
*****/
static void
table_idestroy(HashTable *table)
{
	HashEntry *entry, *next;
	int i;

	for (i=0; i<table->size; i++)
	{
		entry=table->table[i];
		while (entry)
		{
			next = entry->next;
			entry = next;
		}
	}
	free(table->table);
}

/*****
* Name: 		delete_fromilist
* Return Type: 	HashEntry
* Description: 	deletes a given entry from the given hashtable.
* In: 
*	table:		table from which an entry should be deleted.
*	entry:		entry to be deleted;
*	key:		entry identifier
* Returns:
*	entry following the deleted entry. This can be non-null if a hashvalue
*	contains multiple keys.
*****/
static HashEntry *
delete_fromilist(HashTable *table, HashEntry *entry, unsigned long key)
{
	HashEntry *next;

	if(entry==NULL)
		return NULL;
	if(entry->key==key)
	{
		if(table->last == entry)
			table->last = entry->pptr;
		if(entry->nptr)
			entry->nptr->pptr = entry->pptr;
		if(entry->pptr)
			entry->pptr->nptr = entry->nptr;
		next = entry->next;
		free(entry);
		return next;
	}
	entry->next = delete_fromilist(table, entry->next, key);
	return entry;
}

/*****
* Name: 		hash_ikey
* Return Type: 	unsigned long
* Description: 	computes a hash value based on a given key.
* In: 
*	key:		key for which to compute a hashing value.
* Returns:
*	computed hash value.
*****/
static unsigned long
hash_ikey(unsigned long key)
{
	unsigned long h = 0, g;
	int i;
	char skey[sizeof(unsigned long)];
	memcpy(skey, &key, sizeof(unsigned long));
	for (i=0; i<sizeof(unsigned long); i++)
	{
		h = (h<<4) + skey[i];
		if ((g = h & 0x7fffffff))
			h ^=g >> 24;
		h &= ~g;
	}
	return h;
}

/*****
* Name: 		rebuild_itable
* Return Type: 	void
* Description: 	enlarges & rebuilds the given hashtable. Used when the
*				size of the current hashtable is becoming to small to store
*				new info efficiently.
* In: 
*	table:		table to rebuild
* Returns:
*	nothing.
*****/
static void
rebuild_itable(HashTable *table)
{
	HashTable newtable;
	HashEntry *entry;
	int i;

	newtable.last = NULL;
	newtable.elements = 0;
	newtable.size = table->size*2;
	newtable.table = (HashEntry**)malloc(newtable.size * sizeof(HashEntry*));
	memset(newtable.table, 0, newtable.size * sizeof(HashEntry*));
	for (i=0; i<table->size; i++)
	{
		entry = table->table[i];
		while (entry)
		{
			_XmHTMLHashPut(&newtable, entry->key, entry->data);
			entry=entry->next;
		}
	}
	table_idestroy(table);
	table->elements = newtable.elements;
	table->size = newtable.size;
	table->table = newtable.table;
}

/*****
* Public routines.
*****/

/*****
* Name: 		_XmHTMLHashInit
* Return Type: 	HashTable
* Description: 	Initializes a hashtable with a initial size = INIT_TABLE_SIZE
*				The table must already be allocated.
* In: 
*	table:		hashtable to be initialized;
* Returns:
*	initialized table.
*****/
HashTable *
_XmHTMLHashInit(HashTable *table)
{
	table->elements = 0;
	table->size = INIT_TABLE_SIZE;
	table->table = (HashEntry**)malloc(INIT_TABLE_SIZE*sizeof(HashEntry*));
	table->last = NULL;
	memset(table->table, 0, INIT_TABLE_SIZE*sizeof(HashEntry*));

#ifdef DEBUG
	table->requests = table->hits = table->misses = 0;
	table->puts = table->collisions = 0;
#endif

	return table;
}

/*****
* Name: 		_XmHTMLHashPut
* Return Type: 	void
* Description: 	puts a new entry in the hash table
* In: 
*	key:		handle to data to be stored;
*	data:		data to be stored;
* Returns:
*	nothing.
*****/
void
_XmHTMLHashPut(HashTable *table, unsigned long key, unsigned long data)
{
	unsigned long hkey;
	HashEntry *nentry;
    
#ifdef DEBUG
	table->puts++;
#endif
	nentry = (HashEntry*)malloc(sizeof(HashEntry));
#ifdef DEBUG
	nentry->ncoll = 0;
#endif

	nentry->key = key;
	nentry->data = data;
	hkey = hash_ikey(key) % table->size;
	/* Aaie, collided */
	if (table->table[hkey]!=NULL)
	{
#ifdef DEBUG
		nentry->ncoll++;
#endif
		nentry->next = table->table[hkey];
		table->table[hkey] = nentry;
#ifdef DEBUG
		table->collisions++;
#endif
    }
	else
	{
		nentry->next = NULL;
		table->table[hkey] = nentry;
    }
	table->elements++;
    
	nentry->nptr = NULL;
	nentry->pptr = table->last;
	if(table->last)
		table->last->nptr = nentry;
	table->last = nentry;
    
	if(table->elements>(table->size*3)/2)
	{
		_XmHTMLDebug(9, ("StringUtil.c: _XmHTMLHashPut, rebuilding table, "
			"%i elements stored using %i available slots.\n", table->elements,
			table->size));
		rebuild_itable(table);
    }
}

/*****
* Name: 		_XmHTMLHashGet
* Return Type: 	Boolean
* Description: 	retrieves a hash entry.
* In: 
*	key:		id of entry to retrieve;
*	*data:		object in which to store data reference;
* Returns:
*	True when entry was found, False if not.
*****/
Boolean
_XmHTMLHashGet(HashTable *table, unsigned long key, unsigned long *data)
{
	unsigned long hkey;
	HashEntry *entry;
#ifdef DEBUG
	table->requests++;
#endif
    
	hkey = hash_ikey(key) % table->size;
	entry = table->table[hkey];
	while (entry!=NULL)
	{
		if(entry->key==key)
		{
			*data=entry->data;
#ifdef DEBUG
			table->hits++;
#endif
			return(True);
		}
		entry = entry->next;
	}
#ifdef DEBUG
	table->misses++;
#endif
	return(False);
}

/*****
* Name: 		_XmHTMLHashDelete
* Return Type: 	void
* Description: 	deletes the hash entry for the given key.
* In: 
*	table:		hashtable from which to delete an entry;
*	key:		id of entry to be deleted.
* Returns:
*	nothing.
*****/
void
_XmHTMLHashDelete(HashTable *table, unsigned long key)
{
    unsigned long hkey;
    
    hkey = hash_ikey(key) % table->size;
    table->table[hkey] = delete_fromilist(table, table->table[hkey], key);
    table->elements--;
}

/*****
* Name: 		_XmHTMLHashDestroy
* Return Type: 	void
* Description: 	completely destroys the given hashtable contents. Table
*				and contents are not destroyed.
* In: 
*	table:		table to be destroyed;
* Returns:
*	nothing.
*****/
void
_XmHTMLHashDestroy(HashTable *table)
{
	int i;

	_XmHTMLDebug(9, ("Hash statistics:\n"
		"%i elements hashed, %i collided\n"
		"%i requests, %i hits and %i misses.\n",
		table->puts, table->collisions, table->requests, table->hits,
		table->misses));

	for (i=0; i<table->size; i++)
	{
		/* delete all entries */
		if(table->table[i]!=NULL)
			while((table->table[i] = delete_fromilist(table, table->table[i],
				table->table[i]->key)) != NULL);
	}
	/* delete table */
	free(table->table);

	/* sanity */
	table->table = NULL;

#ifdef DEBUG
	table->requests = table->hits = table->misses = 0;
	table->puts = table->collisions = 0;
#endif
}

/* debugging memory functions */
#ifdef DEBUG

/* need to undefine them or w'll get in an endless loop */
#undef malloc
#undef calloc
#undef realloc
#undef free

char*
__rsd_malloc(size_t size, char *file, int line)
{
	static char *ret_val;

	ret_val = (char*) malloc (size);

	/* dump if failed */
	my_assert(ret_val != NULL);

	return(ret_val);
}

char*
__rsd_calloc(size_t nmemb, size_t size, char *file, int line)
{
	static char *ret_val;

	ret_val = (char*) calloc (nmemb, size);

	/* dump if failed */
	my_assert(ret_val != NULL);

	return(ret_val);
}

char*
__rsd_realloc(void *ptr, size_t size, char *file, int line)
{
	static char *ret_val;

	if(size == 0)
	{
		my_assert(ptr != NULL);
		free (ptr);
		return(NULL);
	}
	ret_val = (char*) realloc (ptr, size);

	/* dump if failed */
	my_assert(ret_val != NULL);

	return(ret_val);
}

void
__rsd_free(void *ptr, char *file, int line)
{
	my_assert(ptr != NULL);
	free (ptr);
}

#endif /* DEBUG && !DMALLOC */

