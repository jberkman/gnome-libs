#ifndef lint
static char rcsId[]="$Header$";
#endif
/*****
* numbers.c : HTML numbered lists converters: number->ascii, number->roman
*
* This file Version	$Revision$
*
* Creation date:		Fri Dec 13 17:16:38 GMT+0100 1996
* Last modification: 	$Date$
* By:					$Author$
* Current State:		$State$
*
* Author:				newt
*
* Copyright (C) 1994-1997 by Ripley Software Development 
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
* Revision 1.1  1997/12/17 04:40:29  unammx
* Your daily XmHTML code is here.  It almost links.  Only the
* images.c file is left to port.  Once this is ported we are all
* set to start debugging this baby.
*
* btw, Dickscrape is a Motif based web browser that is entirely
* based on this widget, I just tested it today, very impressive.
*
* Miguel.
*
* Revision 1.6  1997/10/23 00:25:05  newt
* XmHTML Beta 1.1.0 release
*
* Revision 1.5  1997/08/01 13:03:15  newt
* upcase -> my_upcase
*
* Revision 1.4  1997/04/29 14:28:31  newt
* Removed dmalloc.h stuff
*
* Revision 1.3  1997/02/11 02:09:07  newt
* Potential buffer overruns fixed
*
* Revision 1.2  1997/01/09 06:46:10  newt
* replaced ToAscii and ToRoman with more efficient to_ascii and to_roman
*
* Revision 1.1  1996/12/19 02:17:11  newt
* Initial Revision
*
*****/ 
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "XmHTMLP.h"
#include "XmHTMLfuncs.h"

/*** External Function Prototype Declarations ***/

/*** Public Variable Declarations ***/

/*** Private Datatype Declarations ****/

/*** Private Function Prototype Declarations ****/
static String to_ascii(int val);
static String to_roman(int val);

/*** Private Variable Declarations ***/
static char *Ones[] = 
		{"i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix"};
static char *Tens[] = 
		{"x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc"};
static char *Hundreds[] = 
		{"c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm"};


/*****
* Name: 		to_ascii
* Return Type: 	String
* Description: 	converts a numerical value to an abc representation.
* In: 
*	val:		number to convert
* Returns:
*	converted number.
*****/
static String
to_ascii(int val)
{
	int remainder, i = 0, j = 0, value = val;
	char number[10];
	static char out[10];	/* return buffer */

	do
	{
		remainder = (value % 26);
		number[i++] = (remainder ? remainder + 96 : 'z');
	}
	while((value = (remainder ? (int)(value/26) : (int)((value-1)/26))) 
		&& i < 10); /* no buffer overruns */

	for(j = 0; i > 0 && j < 10; i--, j++)
		out[j] = number[i-1];

	out[j] = '\0';	/* NULL terminate */

	return(out);
}

/*****
* Name: 		to_roman
* Return Type: 	String
* Description: 	converts the given number to a lowercase roman numeral.
* In: 
*	val:		number to convert
* Returns:
*	converted number
* Note:
*	This routine is based on a similar one found in the Arena browser.
*****/
static String
to_roman(int val)
{
	int value, one, ten, hundred, thousand;
	static char buf[20], *p, *q;

	value = val;
	/* 
	* XmHTML probably crashes **long** before a number with value 10^20 is
	* reached.
	*/
	sprintf(buf, "%i", val);
	
	thousand = value/1000;
	value = value % 1000;
	hundred = value/100;
	value = value % 100;
	ten = value/10;
	one = value % 10;

	p = buf;
	while(thousand-- > 0)
		*p++ = 'm';

	if(hundred)
	{
		q = Hundreds[hundred-1];
		while ((*p++ = *q++));
		--p;
	}
	if(ten)
	{
		q = Tens[ten-1];
		while ((*p++ = *q++));
		--p;
	}
	if(one)
	{
		q = Ones[one-1];
		while ((*p++ = *q++));
		--p;
	}
	*p = '\0';
	
	return(buf);
}

/*****
* Name: 		ToAsciiLower
* Return Type: 	String
* Description: 	returns the abc representation of the given number
* In: 
*	val:		number to convert
* Returns:
*	converted number
*****/
String
ToAsciiLower(int val)
{
	return((to_ascii(val)));
}

/*****
* Name: 		ToAsciiUpper
* Return Type: 	String
* Description: 	returns the ABC representation of the given number
* In: 
*	val:		number to convert
* Returns:
*	converted number
*****/
String
ToAsciiUpper(int val)
{
	static String buf;
	buf = to_ascii(val);
	my_upcase(buf);
	return(buf);
}

/*****
* Name: 		ToRomanLower
* Return Type: 	String
* Description: 	converts numbers between 1-3999 to roman numerals, lowercase.
* In: 
*	value:		value to convert
* Returns:
*	lowercase roman numeral
*****/
String
ToRomanLower(int val)
{
	return(to_roman(val));
}

/*****
* Name: 		ToRomanUpper
* Return Type: 	String
* Description: 	converts numbers between 1-3999 to roman numerals, uppercase.
* In: 
*	value:		value to convert
* Returns:
*	uppercase roman numeral
*****/
String
ToRomanUpper(int val)
{
	static String buf;
	buf = to_roman(val);
	my_upcase(buf);
	return(buf);
}

Cardinal
SgmlIdToNumber(String sgml_id)
{
	int i;
	String chPtr, id;
	char number[11];

	memset(number, '\0', 11);

	chPtr = &number[0];
	for(i = 0, id = sgml_id; i < strlen(sgml_id) && i < 11; i++, id++)
	{
		if(isalpha(*id) || *id == '.' || *id == '-')
			sprintf(chPtr, "%i", (int)*id);
		else if(isdigit(*id))
			sprintf(chPtr, "%c", *id);
		else
			break;
		if(strlen(number) > 11)
			break;
		chPtr = number + strlen(number);
	}
	return(strtoul(number, (char**)NULL, 10));
}
