/* gnome-hello-basic.c -- Example for the "Starting with GNOME" section
   of the Gnome Developers' Tutorial (that's is included in the
   Gnome Developers' Documentation in devel-progs/)
   */
/* Includes: Basic stuff
             Drag & Drop */

/* Copyright (C) 1998 Mark Galassi, Horacio J. Peña, Elliot Lee */

#include <config.h>
#include <gnome.h>

static void quit_cb (GtkWidget *widget, void *data);
static void drag_data_get_cb (GtkWidget        *widget,
			      GdkDragContext   *context,
			      GtkSelectionData *data,
			      guint             info,
			      guint             time);
static void drop_cb          (GtkWidget *widget,
			      GdkDragContext   *context,
			      gint              x,
			      gint              y,
			      GtkSelectionData *selection_data,
			      guint             info,
			      guint             time);

static void prepare_app(void);

GtkWidget *app;

/* An enumeration to identify types of dragged data */
enum {
	TARGET_HTML,
	TARGET_PLAIN,
	TARGET_URL
};

int
main(int argc, char *argv[])
{
  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME.  It also parses
     the command-line arguments.  */
  gnome_init ("gnome-hello-5-dnd", VERSION, argc, argv);

  prepare_app ();

  gtk_main ();

  return 0;
}

static void
prepare_app(void)
{
  GtkWidget *source, *drop_target, *vbox;

  /* This is the list of formats the drag source exports */
  static GtkTargetEntry drag_types[] = {
	  { "text/html",  0, TARGET_HTML },
	  { "text/plain", 0, TARGET_PLAIN }
  };

  /* This is the list of formats the drop destination supports */
  static GtkTargetEntry drop_types[] = {
	  { "text/plain", 0, TARGET_PLAIN }
  };

  /* Make the main window and binds the delete event so you can close
     the program from your WM */
  app = gnome_app_new ("hello", "Hello World Gnomified");
  gtk_signal_connect (GTK_OBJECT (app), "delete_event",
                      GTK_SIGNAL_FUNC (quit_cb),
                      NULL);

  vbox = gtk_hbox_new(5, FALSE);
  gnome_app_set_contents ( GNOME_APP (app), vbox);
  
  source = gtk_button_new_with_label ("Drag me");
  drop_target = gtk_button_new_with_label ("to this button");
  gtk_container_add(GTK_CONTAINER(vbox), source);
  gtk_container_add(GTK_CONTAINER(vbox), drop_target);

  gtk_signal_connect (GTK_OBJECT (source), "drag_data_get",
		      GTK_SIGNAL_FUNC(drag_data_get_cb), NULL);

  gtk_drag_source_set (source, 
		       GDK_BUTTON1_MASK,
		       drag_types, sizeof(drag_types) / sizeof(drag_types[0]),
		       GDK_ACTION_COPY);

  gtk_container_border_width (GTK_CONTAINER (source), 60);

  gtk_signal_connect (GTK_OBJECT (drop_target), "drag_data_received",
		      GTK_SIGNAL_FUNC(drop_cb), NULL);

  gtk_drag_dest_set (drop_target,
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_DROP,
		     drop_types, sizeof(drop_types) / sizeof(drop_types[0]),
		     GDK_ACTION_COPY);

  gtk_container_border_width (GTK_CONTAINER (drop_target), 60);

  gtk_widget_show_all (app);
}

/* Callbacks functions */

static void
quit_cb (GtkWidget *widget, void *data)
{
  gtk_main_quit ();
  return;
}

static void
drag_data_get_cb (GtkWidget        *widget,
		  GdkDragContext   *context,
		  GtkSelectionData *selection_data,
		  guint             info,
		  guint             time)
{
  char *html_data = "<HTML><HEAD><TITLE>Hi there</TITLE></HEAD><BODY>I am the HTML drag data.</BODY></HTML>";
  char *ascii_data = "Hi there - I am the ASCII drag data.";
  char *url_data = "file1\0file2\0file3";

  g_print("drag_data_get_cb\n");

  switch (info) {
  case TARGET_HTML:
	  gtk_selection_data_set (selection_data,
				  selection_data->target, 8,
				  html_data, strlen(html_data));
	  break;
  case TARGET_PLAIN:
	  gtk_selection_data_set (selection_data,
				  selection_data->target, 8,
				  ascii_data, strlen(ascii_data));
	  break;
  case TARGET_URL:
	  gtk_selection_data_set (selection_data,
				  selection_data->target, 8,
				  url_data, strlen(url_data));
	  break;
  }
}

static void
drop_cb (GtkWidget *widget,
	 GdkDragContext   *context,
	 gint              x,
	 gint              y,
	 GtkSelectionData *selection_data,
	 guint             info,
	 guint             time)
{
  g_print("drop_cb\n");
  if (GTK_IS_BUTTON(widget) &&
      GTK_IS_LABEL(GTK_BUTTON(widget)->child)) {
	  g_print ("Numbytes: %d\n", selection_data->length);
	  gtk_label_set(GTK_LABEL(GTK_BUTTON(widget)->child),
			selection_data->data);
	  /* We could also handle drops of images here by creating
	     a pixmap inside the button - the sky is the limit */
  }
}
