/* gnome-dentry-edit.h: Copyright (C) 1998 Free Software Foundation
 *
 * Written by: Havoc Pennington, based on code by John Ellis.
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
 */

/******************** NOTE: this is an object, not a widget.
 ********************       You must supply a GtkNotebook.
 The reason for this is that you might want this in a property box, 
 or in your own notebook. Look at the test program at the bottom 
 of gnome-dentry-edit.c for a usage example.
 */

#ifndef GNOME_DENTRY_EDIT_H
#define GNOME_DENTRY_EDIT_H

#include <gtk/gtk.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-dentry.h"

BEGIN_GNOME_DECLS

typedef struct _GnomeDEntryEdit GnomeDEntryEdit;
typedef struct _GnomeDEntryEditClass GnomeDEntryEditClass;

#define GNOME_DENTRY_EDIT(obj)          GTK_CHECK_CAST (obj, gnome_dentry_edit_get_type (), GnomeDEntryEdit)
#define GNOME_DENTRY_EDIT_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_dentry_edit_get_type (), GnomeDEntryEditClass)
#define GNOME_IS_DENTRY_EDIT(obj)       GTK_CHECK_TYPE (obj, gnome_dentry_edit_get_type ())

struct _GnomeDEntryEdit {
  GtkObject object;
  
  /* Remaining fields are private - if you need them, 
     please add an accessor function. */

  GtkWidget *name_entry;
  GtkWidget *comment_entry;
  GtkWidget *exec_entry;
  GtkWidget *tryexec_entry;
  GtkWidget *doc_entry;

  GtkWidget *type_combo;

  GtkWidget *terminal_button;  

  GtkWidget *desktop_icon;  /* a GnomePixmap or GtkLabel */
  GtkWidget *icon_button;   /* a GtkButton holding desktop_icon */
  GtkWidget *icon_label;    /* Label with icon filename */
  gchar     *icon;          /* The full icon pathname */

  GtkWidget *icon_dialog;
};

struct _GnomeDEntryEditClass {
  GtkObjectClass parent_class;

  /* Any information changed */
  void (* changed)         (GnomeDEntryEdit * gee);
  /* These two more specific signals are provided since they 
     will likely require a display update */
  /* The icon in particular has changed. */
  void (* icon_changed)    (GnomeDEntryEdit * gee);
  /* The name of the item has changed. */
  void (* name_changed)    (GnomeDEntryEdit * gee);
};

guint       gnome_dentry_edit_get_type  (void);

/* Create a new edit in this notebook - appends two pages to the 
   notebook. */
GtkObject * gnome_dentry_edit_new       (GtkNotebook * notebook);

void        gnome_dentry_edit_clear     (GnomeDEntryEdit * dee);

/* The GnomeDEntryEdit does not store a dentry, and it does not keep
   track of the location field of GnomeDesktopEntry which will always
   be NULL. */

/* Make the display reflect dentry at path */
void        gnome_dentry_edit_load_file  (GnomeDEntryEdit * dee,
					  const gchar * path);

/* Copy the contents of this dentry into the display */
void        gnome_dentry_edit_set_dentry (GnomeDEntryEdit * dee,
					  GnomeDesktopEntry * dentry);

/* Generate a dentry based on the contents of the display */
GnomeDesktopEntry * gnome_dentry_get_dentry(GnomeDEntryEdit * dee);

/* Accessor functions - do NOT free returned string. */
gchar *     gnome_dentry_edit_get_icon   (GnomeDEntryEdit * dee);
gchar *     gnome_dentry_edit_get_name   (GnomeDEntryEdit * dee);

END_GNOME_DECLS
   
#endif /* GNOME_DENTRY_EDIT_H */




