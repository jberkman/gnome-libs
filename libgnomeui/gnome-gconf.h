/* GNOME GUI Library - gnome-gconf.h
 * Copyright (C) 2000  Red Hat Inc.,
 *
 * Author: Jonathan Blandford  <jrb@redhat.com>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#ifndef _GNOME_GCONF_H
#define _GNOME_GCONF_H

#include <gconf/gconf-value.h>
#include <gtk/gtkentry.h>

GConfValue *gnome_gconf_entry_get (GtkEntry       *entry,
				   GConfValueType  type);
void        gnome_gconf_entry_set (GtkEntry       *entry,
				   GConfValue     *value);


#endif
