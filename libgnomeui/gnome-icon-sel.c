/* gnome-icon-sel.c: Copyright (C) 1998 Free Software Foundation
 * Written by: Havoc Pennington, based on John Ellis's code.
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

#include "gnome-icon-sel.h"

#include <config.h>

#include <gdk_imlib.h>
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "gnome-uidefs.h"
#include "gnome-icon-list.h"

#include <unistd.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#define ICON_SIZE 48

static void gnome_icon_selection_class_init (GnomeIconSelectionClass *klass);
static void gnome_icon_selection_init       (GnomeIconSelection      *messagebox);

static void gnome_icon_selection_destroy (GtkObject *gis);

static GtkVBoxClass *parent_class;

static int sort_file_list( gconstpointer a, gconstpointer b);

guint
gnome_icon_selection_get_type ()
{
  static guint gis_type = 0;

  if (!gis_type)
    {
      GtkTypeInfo gis_info =
      {
	"GnomeIconSelection",
	sizeof (GnomeIconSelection),
	sizeof (GnomeIconSelectionClass),
	(GtkClassInitFunc) gnome_icon_selection_class_init,
	(GtkObjectInitFunc) gnome_icon_selection_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      gis_type = gtk_type_unique (gtk_vbox_get_type (), &gis_info);
    }

  return gis_type;
}

static void
gnome_icon_selection_class_init (GnomeIconSelectionClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) klass;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = gnome_icon_selection_destroy;
}

static void
gnome_icon_selection_init (GnomeIconSelection *gis)
{
	GtkWidget *box;
	GtkWidget *frame;
	GtkWidget *sb;
	gis->box = gtk_vbox_new(FALSE, 0);

	gtk_container_add(GTK_CONTAINER(gis), gis->box);

	gtk_widget_show(gis->box);

	box = gtk_hbox_new(FALSE, 5);

	gtk_box_pack_end(GTK_BOX(gis->box), box, TRUE, TRUE, 0);
	gtk_widget_show(box);
	
	sb = gtk_vscrollbar_new(NULL);
	gtk_box_pack_end(GTK_BOX(box),sb,FALSE,FALSE,0);
	gtk_widget_show(sb);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
	gtk_widget_show (frame);

	gis->gil = gnome_icon_list_new(ICON_SIZE+30,
				       gtk_range_get_adjustment(GTK_RANGE(sb)),
				       FALSE);
	gtk_widget_set_usize(gis->gil,350,300);
	gnome_icon_list_set_selection_mode(GNOME_ICON_LIST(gis->gil),
					   GTK_SELECTION_SINGLE);
	gtk_container_add (GTK_CONTAINER (frame), gis->gil);
	gtk_widget_show(gis->gil);

	gis->file_list = NULL;
}


/**
 * gnome_icon_selection_new:
 *
 * Description: Creates a new icon selection widget, it uses GnomeIconList
 * for the listing of icons
 *
 * Returns: Returns the new object
 **/
GtkWidget* gnome_icon_selection_new (void)
{
  GnomeIconSelection * gis;
  
  gis = gtk_type_new(gnome_icon_selection_get_type());

  return GTK_WIDGET (gis);
}

static void gnome_icon_selection_destroy (GtkObject *o)
{
	GnomeIconSelection *gis;
	g_return_if_fail(o != NULL);
	g_return_if_fail(GNOME_IS_ICON_SELECTION(o));
	
	gis = GNOME_ICON_SELECTION(o);

	/*clear our data if we have some*/
	if(gis->file_list) {
		g_list_foreach(gis->file_list,(GFunc)g_free,NULL);
		g_list_free(gis->file_list);
		gis->file_list = NULL;
	}

	if (GTK_OBJECT_CLASS(parent_class)->destroy)
		(* (GTK_OBJECT_CLASS(parent_class)->destroy))(o);
}

/**
 * gnome_icon_selection_add_defaults:
 * @gis: GnomeIconSelection to work with
 *
 * Description: Adds the default pixmap directory into the selection
 * widget. It doesn't show the icons in the selection until you
 * do #gnome_icon_selection_show_icons.
 *
 * Returns:
 **/
void  gnome_icon_selection_add_defaults   (GnomeIconSelection * gis)
{
  gchar *pixmap_dir;

  g_return_if_fail(gis != NULL);

  pixmap_dir = gnome_unconditional_datadir_file("pixmaps");
  
  gnome_icon_selection_add_directory(gis, pixmap_dir);

  g_free(pixmap_dir);
}

static void 
append_an_icon(GnomeIconSelection * gis, const gchar * path)
{
	GdkImlibImage *iml;
	GdkImlibImage *im;
	int pos;
	int w,h;
	
	iml = gdk_imlib_load_image((char *)path);
	/*if I can't load it, ignore it*/
	if(!iml)
		return;
	
	w = iml->rgb_width;
	h = iml->rgb_height;
	if(w>h) {
		if(w>ICON_SIZE) {
			h = h*((double)ICON_SIZE/w);
			w = ICON_SIZE;
		}
	} else {
		if(h>ICON_SIZE) {
			w = w*((double)ICON_SIZE/h);
			h = ICON_SIZE;
		}
	}
	w = w>0?w:1;
	h = h>0?h:1;
	
	im = gdk_imlib_clone_scaled_image(iml,w,h);
	gdk_imlib_destroy_image(iml);
	if(!im)
		return;
	
	pos = gnome_icon_list_append_imlib(GNOME_ICON_LIST(gis->gil),im,
					   g_basename(path));
	gnome_icon_list_set_icon_data_full(GNOME_ICON_LIST(gis->gil), pos, 
					   g_strdup(path),
					   (GtkDestroyNotify) g_free );
/* 	gdk_imlib_destroy_image(im); */ /* FIXME: this needs ref/unref capabilities in imlib */
}

static int sort_file_list( gconstpointer a, gconstpointer b)
{
	return strcmp( (gchar *)a, (gchar *)b );
}

/**
 * gnome_icon_selection_add_directory:
 * @gis: GnomeIconSelection to work with
 * @dir: directory with pixmaps
 *
 * Description: Adds the icons from the directory @dir to the
 * selection widget. It doesn't show the icons in the selection
 * until you do #gnome_icon_selection_show_icons.
 *
 * Returns:
 **/
void  gnome_icon_selection_add_directory  (GnomeIconSelection * gis,
					   const gchar * dir)
{
  struct stat statbuf;
  struct dirent * de;
  DIR * dp;

  g_return_if_fail(gis != NULL);
  g_return_if_fail(dir != NULL);

  if ( stat(dir, &statbuf) == -1 ) {
    g_warning("GnomeIconSelection: Couldn't stat directory");
    return;
  }

  if ( ! S_ISDIR(statbuf.st_mode) ) {
    g_warning("GnomeIconSelection: not a directory");
    return;
  }

  dp = opendir(dir);

  if ( dp == NULL ) {
    g_warning("GnomeIconSelection: couldn't open directory");
    return;
  }

  while ( (de = readdir(dp)) != NULL ) {
#ifdef GNOME_ENABLE_DEBUG
    g_print("File: %s\n", de->d_name);
#endif
    if ( *(de->d_name) == '.' ) continue; /* skip dotfiles */

    if ( g_is_image_filename(de->d_name) ) {
      gchar * full_path = g_concat_dir_and_file(dir, de->d_name);
#ifdef GNOME_ENABLE_DEBUG
    g_print("Full path: %s\n", full_path);
#endif
      if ( stat(full_path, &statbuf) != -1 ) {
	if ( S_ISREG(statbuf.st_mode) ) {
	  /* Image filename, exists, regular file, go for it. */
          gis->file_list = g_list_insert_sorted(gis->file_list,
            g_strdup (full_path), sort_file_list);
	}
      }
      g_free(full_path);
    }
  }

  closedir(dp);
}

/**
 * gnome_icon_selection_show_icons:
 * @gis: GnomeIconSelection to work with
 *
 * Description: Shows the icons inside the widget that
 * were added with #gnome_icon_selection_add_defaults and
 * #gnome_icon_selection_add_directory. Before this function
 * is called the icons aren't actually added to the listing 
 * and can't be picked by the user.
 *
 * Returns:
 **/
void  gnome_icon_selection_show_icons  (GnomeIconSelection * gis)
{
  GList * list;
  GtkWidget *label;
  GtkWidget *progressbar;
  int file_count, i;

  g_return_if_fail(gis != NULL);
  if(!gis->file_list) return;

  list = gis->file_list;
  
  label = gtk_label_new(_("Loading Icons..."));
  gtk_box_pack_start(GTK_BOX(gis->box),label,FALSE,FALSE,0);
  gtk_widget_show(label);

  progressbar = gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(gis->box),progressbar,FALSE,FALSE,0);
  gtk_widget_show(progressbar);

  file_count = g_list_length(list);
  i = 0;
  
  gnome_icon_list_freeze(GNOME_ICON_LIST(gis->gil));

  while (list) {
    append_an_icon(gis, list->data);
    g_free(list->data);
    list = list->next;
    /*only do this for every 5th to save some time*/
    if(i%5==0) {
	    gtk_progress_bar_update (GTK_PROGRESS_BAR (progressbar), (float)i / file_count);
	    while ( gtk_events_pending() ) {
		    gtk_main_iteration();
	    }
    }
    i++;
  }

  g_list_free(gis->file_list);
  gis->file_list = NULL;

  gnome_icon_list_thaw(GNOME_ICON_LIST(gis->gil));

  gtk_widget_destroy(progressbar);
  gtk_widget_destroy(label);
}


/**
 * gnome_icon_selection_clear:
 * @gis: GnomeIconSelection to work with
 * @not_shown: boolean
 *
 * Description: Clear the currently shown icons, the ones
 * that weren't shown yet are not cleared unless the not_shown
 * parameter is given, in which case even those are cleared.
 *
 * Returns:
 **/
void  gnome_icon_selection_clear          (GnomeIconSelection * gis,
					   gboolean not_shown)
{
	g_return_if_fail(gis != NULL);

	/*clear our data if we have some and not_shown is set*/
	if(not_shown && gis->file_list) {
		g_list_foreach(gis->file_list,(GFunc)g_free,NULL);
		g_list_free(gis->file_list);
		gis->file_list = NULL;
	}

	gnome_icon_list_clear(GNOME_ICON_LIST(gis->gil));
}

/**
 * gnome_icon_selection_get_icon:
 * @gis: GnomeIconSelection to work with
 * @full_path: boolean
 *
 * Description: Gets the currently selected icon name, if
 * full_path is true, it returns the full path to the icon,
 * if none is selected it returns NULL
 *
 * Returns: internal string, it must not be changed or freed
 * or NULL
 **/
const gchar * 
gnome_icon_selection_get_icon     (GnomeIconSelection * gis,
				   gboolean full_path)
{
  GList * sel;

  g_return_val_if_fail(gis != NULL, NULL);

  sel = GNOME_ICON_LIST(gis->gil)->selection;
  if ( sel ) {
    gchar * p;
    gint pos = GPOINTER_TO_INT(sel->data);
    p = gnome_icon_list_get_icon_data(GNOME_ICON_LIST(gis->gil), pos);
    if (full_path) return p;
    else return g_filename_pointer(p);
  }
  else return NULL;
}

/**
 * gnome_icon_selection_select_icon:
 * @gis: GnomeIconSelection to work with
 * @filename: icon filename
 *
 * Description: Selects the icon @filename. This icon must have
 * already been added and shown * (see @gnome_icon_selection_show_icons)
 *
 * Returns:
 **/
void  gnome_icon_selection_select_icon    (GnomeIconSelection * gis,
					   const gchar * filename)
{
  gint pos;
  gint icons;
  
  g_return_if_fail(gis != NULL);
  g_return_if_fail(filename != NULL);

  icons = GNOME_ICON_LIST(gis->gil)->icons;
  pos = 0;

  while ( pos < icons ) {
    gchar * file = 
      gnome_icon_list_get_icon_data(GNOME_ICON_LIST(gis->gil),pos);
    if ( strcmp(g_filename_pointer(file),filename) == 0 ) {
      gnome_icon_list_select_icon(GNOME_ICON_LIST(gis->gil), pos);
      return;
    }

    ++pos;
  }
}
