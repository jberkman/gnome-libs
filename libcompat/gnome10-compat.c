/* gnome10-compat.c - GNOME 1.0 compatibility functions.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "gnome-defs.h"
#include "gnome10-compat.h"
#include "gnomelib-init2.h"
#include "gnomelib-init.h"

void
gnomelib_init (const char *app_id,
	       const char *app_version)
{
  char *fakeargv[] = { "Unknown", NULL };

  gnome_program_init (app_id, app_version, 1, fakeargv, LIBGNOME_INIT, NULL);
}
