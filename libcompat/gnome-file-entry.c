/* GnomeFileEntry widget - Combo box with "Browse" button for files
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 */
#include <config.h>
#include <unistd.h> /*getcwd*/
#include <sys/param.h> /*realpath*/
#include <gtk/gtkbutton.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-mime.h"
#include "gnome-file-entry.h"


static void gnome_file_entry_class_init (GnomeFileEntryClass *class);
static void gnome_file_entry_init       (GnomeFileEntry      *fentry);
static void gnome_file_entry_finalize   (GtkObject           *object);
static void gnome_file_entry_drag_data_received (GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 GtkSelectionData *data,
						 guint             info,
						 guint32           time);
static void browse_clicked(GnomeFileEntry *fentry);
static GtkHBoxClass *parent_class;

guint
gnome_file_entry_get_type (void)
{
	static guint file_entry_type = 0;

	if (!file_entry_type){
		GtkTypeInfo file_entry_info = {
			"GnomeFileEntry",
			sizeof (GnomeFileEntry),
			sizeof (GnomeFileEntryClass),
			(GtkClassInitFunc) gnome_file_entry_class_init,
			(GtkObjectInitFunc) gnome_file_entry_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		file_entry_type = gtk_type_unique (gtk_hbox_get_type (), &file_entry_info);
	}

	return file_entry_type;
}

enum {
	BROWSE_CLICKED_SIGNAL,
	LAST_SIGNAL
};

static int gnome_file_entry_signals[LAST_SIGNAL] = {0};

static void
gnome_file_entry_class_init (GnomeFileEntryClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;
	parent_class = gtk_type_class (gtk_hbox_get_type ());
	
	gnome_file_entry_signals[BROWSE_CLICKED_SIGNAL] =
		gtk_signal_new("browse_clicked",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(GnomeFileEntryClass,
			       			 browse_clicked),
			       gtk_signal_default_marshaller,
			       GTK_TYPE_NONE,
			       0);
	gtk_object_class_add_signals(object_class,gnome_file_entry_signals,
				     LAST_SIGNAL);

	object_class->finalize = gnome_file_entry_finalize;

	class->browse_clicked = browse_clicked;
	
}

static void
browse_dialog_ok (GtkWidget *widget, gpointer data)
{
	GtkFileSelection *fs;
	GnomeFileEntry *fentry;
	GtkWidget *entry;

	fs = GTK_FILE_SELECTION (data);
	fentry = GNOME_FILE_ENTRY (gtk_object_get_user_data (GTK_OBJECT (fs)));
	entry = gnome_file_entry_gtk_entry (fentry);

	gtk_entry_set_text (GTK_ENTRY (entry),
			    gtk_file_selection_get_filename (fs));
	gtk_widget_destroy (GTK_WIDGET (fs));
}

static void
browse_dialog_kill (GtkWidget *widget, gpointer data)
{
	GnomeFileEntry *fentry;
	fentry = GNOME_FILE_ENTRY (data);
	fentry->fsw = NULL;
}

static void
browse_clicked(GnomeFileEntry *fentry)
{	
	GtkWidget *fsw;
	GtkFileSelection *fs;
	char *p;

	/*if it already exists make sure it's shown and raised*/
	if(fentry->fsw) {
		gtk_widget_show(fentry->fsw);
		if(fentry->fsw->window)
			gdk_window_raise(fentry->fsw->window);
		fs = GTK_FILE_SELECTION(fentry->fsw);
		gtk_widget_set_sensitive(fs->file_list,
					 !fentry->directory_entry);
		p = gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)));
		if(p && *p!='/' && fentry->default_path) {
			p = g_concat_dir_and_file (fentry->default_path, p);
			gtk_file_selection_set_filename (fs, p);
			g_free(p);
		} else
			gtk_file_selection_set_filename (fs, p);
		return;
	}


	fsw = gtk_file_selection_new (fentry->browse_dialog_title
				      ? fentry->browse_dialog_title
				      : _("Select file"));
	gtk_object_set_user_data (GTK_OBJECT (fsw), fentry);

	fs = GTK_FILE_SELECTION (fsw);
	gtk_widget_set_sensitive(fs->file_list,
				 !fentry->directory_entry);

	p = gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)));
	if(p && *p!='/' && fentry->default_path) {
		p = g_concat_dir_and_file (fentry->default_path, p);
		gtk_file_selection_set_filename (fs, p);
		g_free(p);
	} else
		gtk_file_selection_set_filename (fs, p);

	gtk_signal_connect (GTK_OBJECT (fs->ok_button), "clicked",
			    (GtkSignalFunc) browse_dialog_ok,
			    fs);
	gtk_signal_connect_object (GTK_OBJECT (fs->cancel_button), "clicked",
				   GTK_SIGNAL_FUNC(gtk_widget_destroy),
				   GTK_OBJECT(fsw));
	gtk_signal_connect (GTK_OBJECT (fsw), "destroy",
			    GTK_SIGNAL_FUNC(browse_dialog_kill),
			    fentry);

	gtk_widget_show (fsw);
	
	if(fentry->is_modal)
		gtk_grab_add(fsw);
	fentry->fsw = fsw;
}

static void
browse_clicked_signal(GtkWidget *widget, gpointer data)
{
	gtk_signal_emit(GTK_OBJECT(data),
			gnome_file_entry_signals[BROWSE_CLICKED_SIGNAL]);
}

static void
gnome_file_entry_drag_data_received (GtkWidget        *widget,
				     GdkDragContext   *context,
				     gint              x,
				     gint              y,
				     GtkSelectionData *selection_data,
				     guint             info,
				     guint32           time)
{
	GList *files;

	/*here we extract the filenames from the URI-list we recieved*/
	files = gnome_uri_list_extract_filenames(selection_data->data);
	/*if there's isn't a file*/
	if(!files) {
		gtk_drag_finish(context,FALSE,FALSE,time);
		return;
	}

	gtk_entry_set_text (GTK_ENTRY(widget), files->data);

	/*free the list of files we got*/
	gnome_uri_list_free_strings (files);
}

#define ELEMENTS(x) (sizeof (x) / sizeof (x[0]))

static void
gnome_file_entry_init (GnomeFileEntry *fentry)
{
	GtkWidget *button, *the_gtk_entry;
	static GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };

	fentry->browse_dialog_title = NULL;
	fentry->default_path = NULL;
	fentry->is_modal = FALSE;
	fentry->directory_entry = FALSE;

	gtk_box_set_spacing (GTK_BOX (fentry), 4);

	fentry->gentry = gnome_entry_new (NULL);
	the_gtk_entry = gnome_file_entry_gtk_entry (fentry);

	gtk_drag_dest_set (GTK_WIDGET (the_gtk_entry),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, ELEMENTS(drop_types), GDK_ACTION_COPY);

	gtk_signal_connect (GTK_OBJECT (the_gtk_entry), "drag_data_received",
			    GTK_SIGNAL_FUNC (gnome_file_entry_drag_data_received),
			    NULL);

	gtk_box_pack_start (GTK_BOX (fentry), fentry->gentry, TRUE, TRUE, 0);
	gtk_widget_show (fentry->gentry);

	button = gtk_button_new_with_label (_("Browse..."));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    (GtkSignalFunc) browse_clicked_signal,
			    fentry);
	gtk_box_pack_start (GTK_BOX (fentry), button, FALSE, FALSE, 0);
	gtk_widget_show (button);
}

static void
gnome_file_entry_finalize (GtkObject *object)
{
	GnomeFileEntry *fentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (object));

	fentry = GNOME_FILE_ENTRY (object);

	if (fentry->browse_dialog_title)
		g_free (fentry->browse_dialog_title);
	if (fentry->default_path)
		g_free (fentry->default_path);
	if (fentry->fsw)
		gtk_widget_destroy(fentry->fsw);

	(* GTK_OBJECT_CLASS (parent_class)->finalize) (object);
}

/**
 * gnome_file_entry_new:
 * @history_id: the id given to #gnome_entry_new
 * @browse_dialog_title: title of the browse dialog
 *
 * Description: Creates a new file entry widget
 *
 * Returns: Returns the new object
 **/
GtkWidget *
gnome_file_entry_new (char *history_id, char *browse_dialog_title)
{
	GnomeFileEntry *fentry;

	fentry = gtk_type_new (gnome_file_entry_get_type ());

	gnome_entry_set_history_id (GNOME_ENTRY (fentry->gentry), history_id);
	gnome_entry_load_history (GNOME_ENTRY(fentry->gentry));
	gnome_file_entry_set_title (fentry, browse_dialog_title);

	return GTK_WIDGET (fentry);
}

/**
 * gnome_file_entry_gnome_entry:
 * @fentry: the GnomeFileEntry to work with
 *
 * Description: Get the GnomeEntry widget that's part of the entry
 *
 * Returns: Returns GnomeEntry widget
 **/
GtkWidget *
gnome_file_entry_gnome_entry (GnomeFileEntry *fentry)
{
	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);

	return fentry->gentry;
}

/**
 * gnome_file_entry_gtk_entry:
 * @fentry: the GnomeFileEntry to work with
 *
 * Description: Get the GtkEntry widget that's part of the entry
 *
 * Returns: Returns GtkEntry widget
 **/
GtkWidget *
gnome_file_entry_gtk_entry (GnomeFileEntry *fentry)
{
	g_return_val_if_fail (fentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry), NULL);

	return gnome_entry_gtk_entry (GNOME_ENTRY (fentry->gentry));
}

/**
 * gnome_file_entry_set_title:
 * @fentry: the GnomeFileEntry to work with
 * @browse_dialog_title: the title
 *
 * Description: Set the title of the browse dialog to @browse_dialog_title,
 * this will go into effect the next time the browse button is pressed
 *
 * Returns:
 **/
void
gnome_file_entry_set_title (GnomeFileEntry *fentry, char *browse_dialog_title)
{
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));

	if (fentry->browse_dialog_title)
		g_free (fentry->browse_dialog_title);

	fentry->browse_dialog_title = g_strdup (browse_dialog_title); /* handles NULL correctly */
}

/**
 * gnome_file_entry_set_default_path:
 * @fentry: the GnomeFileEntry to work with
 * @path: path string
 *
 * Description: Set the default path of browse dialog to @path. The
 * default path is only used if the entry is empty or if the contents
 * of the entry is not a full absolute path, in that case the default
 * path is prepended to it before the dialog is started
 *
 * Returns:
 **/
void
gnome_file_entry_set_default_path(GnomeFileEntry *fentry, char *path)
{
	char rpath[MAXPATHLEN+1];
	char *p;
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));
	
	if(path) {
		if(realpath(path,rpath))
			p = g_strdup(rpath);
		else
			p = NULL;
	} else
		p = NULL;

	if(fentry->default_path)
		g_free(fentry->default_path);
	
	/*handles NULL as well*/
	fentry->default_path = p;
}

/**
 * gnome_file_entry_get_full_path:
 * @fentry: the GnomeFileEntry to work with
 * @file_must_exist: boolean
 *
 * Description: Gets the full absolute path of the file from the entry,
 * if @file_must_exist is true, then the path is only returned if the path
 * actually exists. In case the entry is a directory entry (see
 * #gnome_file_entry_set_directory), then if the path exists and is a
 * directory then it's returned, if not, it is assumed it was a file so
 * we try to strip it, and try again. This only happens if @file_must_exist
 * is true, if it's false, nothing is tested, it's just returned.
 *
 * Returns: a newly allocated string with the path or NULL if something went
 * wrong
 **/
char *
gnome_file_entry_get_full_path(GnomeFileEntry *fentry, int file_must_exist)
{
	char *p;
	char *t;
	g_return_val_if_fail (fentry != NULL,NULL);
	g_return_val_if_fail (GNOME_IS_FILE_ENTRY (fentry),NULL);

	t = gtk_entry_get_text (GTK_ENTRY (gnome_file_entry_gtk_entry (fentry)));
	if(!t || !*t)
		return NULL;
	if(*t=='/')
		p = g_strdup(t);
	else if(fentry->default_path)
			p = g_concat_dir_and_file (fentry->default_path, t);
	else {
		char *cwd = getcwd(NULL,0);
		p = g_concat_dir_and_file (cwd, t);
		free(cwd);
	}
	if (file_must_exist) {
		if (fentry->directory_entry) {
			char *d;
			if (g_file_test (p,G_FILE_TEST_ISDIR))
				return p;
			d = g_dirname (p);
			g_free (p);
			if (g_file_test (d,G_FILE_TEST_ISDIR))
				return d;
			p = d;
		} else if (g_file_exists (p))
			return p;
	} else 
		return p;

	g_free (p);
	return NULL;
}

/**
 * gnome_file_entry_set_modal:
 * @fentry: the GnomeFileEntry to work with
 * @is_modal: boolean
 *
 * Description: Sets the modality of the browse dialog
 *
 * Returns:
 **/
void
gnome_file_entry_set_modal(GnomeFileEntry *fentry, int is_modal)
{
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));
	
	fentry->is_modal = is_modal;
}

/**
 * gnome_file_entry_set_directory:
 * @fentry: the GnomeFileEntry to work with
 * @directory_entry: boolean
 *
 * Description: Sets wheather this is a directory only entry, if
 * @directory_entry is true, then #gnome_file_entry_get_full_path will
 * check for the file being a directory, and the browse dialog will have
 * the file list disabled
 *
 * Returns:
 **/
void
gnome_file_entry_set_directory(GnomeFileEntry *fentry, int directory_entry)
{
	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));
	
	fentry->directory_entry = directory_entry;
}
