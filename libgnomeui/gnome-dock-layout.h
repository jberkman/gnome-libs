/* gnome-dock-layout.c

   Copyright (C) 1998 Free Software Foundation

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
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#ifndef _GNOME_DOCK_LAYOUT_H
#define _GNOME_DOCK_LAYOUT_H

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GNOME_DOCK_LAYOUT(obj) \
  GTK_CHECK_CAST (obj, gnome_dock_layout_get_type (), GnomeDockLayout)
#define GNOME_DOCK_LAYOUT_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gnome_dock_layout_get_type (), GnomeDockLayoutClass)
#define GNOME_IS_DOCK_LAYOUT(obj) \
  GTK_CHECK_TYPE (obj, gnome_dock_layout_get_type ())

typedef struct _GnomeDockLayoutItem GnomeDockLayoutItem;
typedef struct _GnomeDockLayoutClass GnomeDockLayoutClass;
typedef struct _GnomeDockLayout GnomeDockLayout;

#include "gnome-dock.h"
#include "gnome-dock-item.h"

struct _GnomeDockLayoutItem
{
  GnomeDockItem *item;

  GnomeDockPositionType position;

  gint band_num;
  gint band_position;
  gint offset;
};

struct _GnomeDockLayout
{
  GtkObject object;

  GList *items;                 /* GnomeDockLayoutItem */
};

struct _GnomeDockLayoutClass
{
  GtkObjectClass parent_class;
};



GtkObject           *gnome_dock_layout_new      (void);
guint                gnome_dock_layout_get_type (void);
   
gboolean             gnome_dock_layout_add      (GnomeDockLayout *layout,
                                                 GnomeDockItem *item,
                                                 GnomeDockPositionType position,
                                                 gint band_num,
                                                 gint band_position,
                                                 gint offset);
   
GnomeDockLayoutItem *gnome_dock_layout_get_item (GnomeDockLayout *layout,
                                                 GnomeDockItem *item);
GnomeDockLayoutItem *gnome_dock_layout_get_item_by_name
                                                (GnomeDockLayout *layout,
                                                 const gchar *name);

gboolean             gnome_dock_layout_remove_item
                                                (GnomeDockLayout *layout,
                                                 GnomeDockItem *item);
gboolean             gnome_dock_layout_remove_item_by_name
                                                (GnomeDockLayout *layout,
                                                 const gchar *name);

gchar               *gnome_dock_layout_create_string
                                                (GnomeDockLayout *layout);
gboolean             gnome_dock_layout_parse_string
                                                (GnomeDockLayout *layout,
                                                 const gchar *string);

gboolean             gnome_dock_layout_add_to_dock
                                                (GnomeDockLayout *layout,
                                                 GnomeDock *dock);

END_GNOME_DECLS

#endif
