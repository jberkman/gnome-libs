/* gnome-hello-basic.c -- Example for the "Starting with GNOME" section
   of the Gnome Developers' Tutorial (that's is included in the
   Gnome Developers' Documentation in devel-progs/)
   */
/* Includes: Basic stuff
             Drag & Drop */

/* Copyright (C) 1998 Mark Galassi, Horacio J. Peña, Elliot Lee */

/* including gnome.h gives you all you need to use the gtk toolkit as
   well as the GNOME libraries; it also handles internationalization
   via GNU gettext. Including config.h before gnome.h is very important
   (else gnome-i18n can't find ENABLE_NLS), of course i'm assuming
   that we're in the gnome tree. */
#include <config.h>
#include <gnome.h>

void hello_cb (GtkWidget *widget, void *data);
void quit_cb (GtkWidget *widget, void *data);
void drag_cb (GtkWidget *widget, GdkEventDragRequest *event);
void drop_highlight_cb(GtkWidget *widget, GdkEvent *event);
void drop_cb(GtkWidget *widget, GdkEventDropDataAvailable *event);

void prepare_app();

GtkWidget *app;

int
main(int argc, char *argv[])
{
  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME */
  gnome_init (&argc, &argv);

  /* prepare_app() makes all the gtk calls necessary to set up a
     minimal Gnome application; It's based on the hello world example
     from the Gtk+ tutorial */
  prepare_app ();

  gtk_main ();

  return 0;
}

void
prepare_app()
{
  GtkWidget *button, *button2, *vbox;
  char *drag_types[] = {
    "text/html",
    "text/plain"
  };
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
  button = gtk_button_new_with_label ("Drag me");
  button2 = gtk_button_new_with_label ("to this button");
  gtk_container_add(GTK_CONTAINER(vbox), button);
  gtk_container_add(GTK_CONTAINER(vbox), button2);
  gtk_widget_realize(button);
  gtk_widget_realize(button2);

  /* You currently have to do all the realization before setting
     up D&D, so once we've done this all is well and we do
     the D&D-specific stuff */

  gtk_signal_connect (GTK_OBJECT (button), "drag_request_event",
		      GTK_SIGNAL_FUNC(drag_cb), NULL);

  gtk_widget_dnd_drag_set(button, TRUE, drag_types, 2);
  gtk_container_border_width (GTK_CONTAINER (button), 60);

  gtk_signal_connect (GTK_OBJECT (button2), "drop_enter_event",
		      GTK_SIGNAL_FUNC(drop_highlight_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (button2), "drop_leave_event",
		      GTK_SIGNAL_FUNC(drop_highlight_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (button2), "drop_data_available_event",
		      GTK_SIGNAL_FUNC(drop_cb), NULL);
  gtk_widget_dnd_drop_set(button2, TRUE, drop_types, 1, FALSE);
  gtk_container_border_width (GTK_CONTAINER (button2), 60);

  /* We now show the widgets, the order doesn't matter, but i suggests 
     showing the main window last so the whole window will popup at
     once rather than seeing the window pop up, and then the button form
     inside of it. Although with such simple example, you'd never notice. */
  gtk_widget_show (button);
  gtk_widget_show (button2);
  gtk_widget_show (app);
}

/* Callbacks functions */

void
quit_cb (GtkWidget *widget, void *data)
{
  gtk_main_quit ();
  return;
}

void
drag_cb (GtkWidget *widget, GdkEventDragRequest *event)
{
  char *html_data = "<HTML><HEAD><TITLE>Hi there</TITLE></HEAD><BODY>I am the HTML drag data.</BODY></HTML>";
  char *ascii_data = "Hi there - I am the ASCII drag data.";

  g_print("drag_cb\n");
  if(!strcmp(event->data_type, "text/html"))
    {
      gtk_widget_dnd_data_set(widget, event, html_data,
			      strlen(html_data) + 1);
    }
  else if(!strcmp(event->data_type, "text/plain"))
    {
      gtk_widget_dnd_data_set(widget, event, ascii_data,
			      strlen(ascii_data) + 1);
    }
}

void
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

void
drop_cb(GtkWidget *widget, GdkEventDropDataAvailable *event)
{
  g_print("drop_cb\n");
  if(GTK_IS_BUTTON(widget)
     && GTK_IS_LABEL(GTK_BUTTON(widget)->child))
    {
      if(!strcmp(event->data_type, "text/plain"))
	{
	  gtk_label_set(GTK_LABEL(GTK_BUTTON(widget)->child), event->data);
	}
      /* We could also handle drops of images here by creating
	 a pixmap inside the button - the sky is the limit */
    }
}
