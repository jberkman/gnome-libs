/* gnome-hello-basic.c -- Example for the "Starting with GNOME" section
   of the Gnome Developers' Tutorial (that's is included in the
   Gnome Developers' Documentation in devel-progs/)
   */
/* Includes: Basic stuff
             Drag & Drop */

/* Copyright (C) 1998 Mark Galassi, Horacio J. Pe�a, Elliot Lee */

#include <config.h>
#include <gnome.h>

static void hello_cb (GtkWidget *widget, void *data);
static void quit_cb (GtkWidget *widget, void *data);
static void drag_cb (GtkWidget *widget, GdkEventDragRequest *event);
static void drop_highlight_cb(GtkWidget *widget, GdkEvent *event);
static void drop_cb(GtkWidget *widget, GdkEventDropDataAvailable *event);

static void prepare_app(void);

GtkWidget *app;

int
main(int argc, char *argv[])
{
  argp_program_version = VERSION;

  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME.  It also parses
     the command-line arguments.  */
  gnome_init ("gnome-hello-5-dnd", NULL, argc, argv,
	      0, NULL);

  prepare_app ();

  gtk_main ();

  return 0;
}

static void
prepare_app(void)
{
  GtkWidget *source, *drop_target, *vbox;

  /* This is the list of formats the drag source exports */
  char *drag_types[] = {
    "text/html",
    "text/plain"
  };

  /* This is the list of formats the drop target suppots */
  char *drop_types[] = {
    "text/plain"
  };

  /* Make the main window and binds the delete event so you can close
     the program from your WM */
  app = gnome_app_new ("hello", "Hello World Gnomified");
  gtk_widget_realize (app);
  gtk_signal_connect (GTK_OBJECT (app), "delete_event",
                      GTK_SIGNAL_FUNC (quit_cb),
                      NULL);

  vbox = gtk_hbox_new(5, FALSE);
  gnome_app_set_contents ( GNOME_APP (app), vbox);
  gtk_widget_realize(vbox);
  
  source = gtk_button_new_with_label ("Drag me");
  drop_target = gtk_button_new_with_label ("to this button");
  gtk_container_add(GTK_CONTAINER(vbox), source);
  gtk_container_add(GTK_CONTAINER(vbox), drop_target);

  /* Widgets get realized, as the X-window associated with them
   * should exist prior to be able to work with DND
   */
  gtk_widget_realize(source);
  gtk_widget_realize(drop_target);

  /* You currently have to do all the realization before setting
     up D&D, so once we've done this all is well and we do
     the D&D-specific stuff */

  gtk_signal_connect (GTK_OBJECT (source), "drag_request_event",
		      GTK_SIGNAL_FUNC(drag_cb), NULL);

  gtk_widget_dnd_drag_set(source, TRUE, drag_types, 2);
  gtk_container_border_width (GTK_CONTAINER (source), 60);

  gtk_signal_connect (GTK_OBJECT (drop_target), "drop_enter_event",
		      GTK_SIGNAL_FUNC(drop_highlight_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (drop_target), "drop_leave_event",
		      GTK_SIGNAL_FUNC(drop_highlight_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (drop_target), "drop_data_available_event",
		      GTK_SIGNAL_FUNC(drop_cb), NULL);
  gtk_widget_dnd_drop_set(drop_target, TRUE, drop_types, 1, FALSE);
  gtk_container_border_width (GTK_CONTAINER (drop_target), 60);

  /* We now show the widgets, the order doesn't matter, but i suggests 
     showing the main window last so the whole window will popup at
     once rather than seeing the window pop up, and then the button form
     inside of it. Although with such simple example, you'd never notice. */
  gtk_widget_show (source);
  gtk_widget_show (drop_target);
  gtk_widget_show (app);
}

/* Callbacks functions */

static void
quit_cb (GtkWidget *widget, void *data)
{
  gtk_main_quit ();
  return;
}

static void
drag_cb (GtkWidget *widget, GdkEventDragRequest *event)
{
  char *html_data = "<HTML><HEAD><TITLE>Hi there</TITLE></HEAD><BODY>I am the HTML drag data.</BODY></HTML>";
  char *ascii_data = "Hi there - I am the ASCII drag data.";
  char *url_data = "file1\0file2\0file3";

  g_print("drag_cb\n");
  if(!strcmp(event->data_type, "text/html"))
    {
      gtk_widget_dnd_data_set(widget, (GdkEvent*)event, html_data,
			      strlen(html_data) + 1);
    }
  else if(!strcmp(event->data_type, "text/plain"))
    {
      gtk_widget_dnd_data_set(widget, (GdkEvent*)event, ascii_data,
			      strlen(ascii_data) + 1);
    } else if (!strcmp (event->data_type, "url:ALL"))
    {
      gtk_widget_dnd_data_set (widget, (GdkEvent*)event, url_data, 
			      strlen(url_data) + 1);
    }
}

static void
drop_highlight_cb(GtkWidget *widget, GdkEvent *event)
{
  g_print("drop_highlight_cb\n");
  if(event->type == GDK_DROP_ENTER)
    {
      gtk_widget_set_state(widget, GTK_STATE_PRELIGHT);
    }
  else if(event->type == GDK_DROP_LEAVE)
    {
      gtk_widget_set_state(widget, GTK_STATE_NORMAL);
    }
}

static void
drop_cb(GtkWidget *widget, GdkEventDropDataAvailable *event)
{
  g_print("drop_cb\n");
  if(GTK_IS_BUTTON(widget)
     && GTK_IS_LABEL(GTK_BUTTON(widget)->child))
    {
      if(!strcmp(event->data_type, "text/plain"))
	{
	  printf ("Numbytes: %ld\n", event->data_numbytes);
	  gtk_label_set(GTK_LABEL(GTK_BUTTON(widget)->child), event->data);
	}
      /* We could also handle drops of images here by creating
	 a pixmap inside the button - the sky is the limit */
    }
}
