/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
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

#ifndef GNOME_GCONF_H
#define GNOME_GCONF_H

#include <gconf/gconf-value.h>
#include <gconf/gconf-client.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkradiobutton.h>
#include <libgnome/gnomelib-init2.h>
#include "gnome-color-picker.h"
#include "gnome-entry.h"
#include "gnome-file-entry.h"
#include "gnome-icon-entry.h"
#include "gnome-pixmap-entry.h"
/* GTK Widgets */
GConfValue *gnome_gconf_gtk_entry_get          (GtkEntry         *entry,
						GConfValueType    type);
void        gnome_gconf_gtk_entry_set          (GtkEntry         *entry,
						GConfValue       *value);
GConfValue *gnome_gconf_spin_button_get        (GtkSpinButton    *spin_button,
						GConfValueType    type);
void        gnome_gconf_spin_button_set        (GtkSpinButton   *spin_button,
						GConfValue      *value);
GConfValue *gnome_gconf_gtk_radio_button_get   (GtkRadioButton   *radio,
						GConfValueType    type);
void        gnome_gconf_gtk_radio_button_set   (GtkRadioButton   *radio,
						GConfValue       *value);
GConfValue *gnome_gconf_gtk_range_get          (GtkRange         *range,
						GConfValueType    type);
void        gnome_gconf_gtk_range_set          (GtkRange         *range,
						GConfValue       *value);
GConfValue *gnome_gconf_gtk_toggle_button_get  (GtkToggleButton  *toggle,
						GConfValueType    type);
void        gnome_gconf_gtk_toggle_button_set  (GtkToggleButton  *toggle,
						GConfValue       *value);


/* GNOME Widgets */
GConfValue *gnome_gconf_gnome_color_picker_get (GnomeColorPicker *picker,
						GConfValueType    type);
void        gnome_gconf_gnome_color_picker_set (GnomeColorPicker *picker,
						GConfValue       *value);
GConfValue *gnome_gconf_gnome_entry_get        (GnomeEntry       *entry,
						GConfValueType    type);
void        gnome_gconf_gnome_entry_set        (GnomeEntry       *entry,
						GConfValue       *value);
GConfValue *gnome_gconf_gnome_file_entry_get   (GnomeFileEntry   *file_entry,
						GConfValueType    type);
void        gnome_gconf_gnome_file_entry_set   (GnomeFileEntry   *file_entry,
						GConfValue       *value);
GConfValue *gnome_gconf_gnome_icon_entry_get   (GnomeIconEntry   *icon_entry,
						GConfValueType    type);
void        gnome_gconf_gnome_icon_entry_set   (GnomeIconEntry   *icon_entry,
						GConfValue       *value);
GConfValue *gnome_gconf_gnome_pixmap_entry_get (GnomePixmapEntry *pixmap_entry,
						GConfValueType    type);
void        gnome_gconf_gnome_pixmap_entry_set (GnomePixmapEntry *pixmap_entry,
						GConfValue       *value);


/* GNOME GConf module; basically what this does is
   create a global GConfClient for a GNOME application; it's used
   by libgnomeui, and applications can either use it or create
   their own. However note that signals will be emitted for
   libgnomeui settings and errors! Also the module inits
   GConf
*/

GConfClient *gnome_gconf_client_get (void);

extern GnomeModuleInfo gnome_gconf_module_info;
#define GNOME_GCONF_INIT GNOME_PARAM_MODULE,&gnome_gconf_module_info

#endif


