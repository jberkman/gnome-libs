#include <gtk/gtk.h>
#include "gtk-xmhtml.h"

main (int argc, char *argv [])
{
	GtkWidget *window, *html;
	char *p = malloc (10);
			  
	gtk_init (&argc, &argv);
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_show (window);
	html = gtk_xmhtml_new (200, 200, "<html>\n"
			       "<head><title>Hola</title></head>\n"
			       "<b>Bold</b><p>Nuevo parrafo<p><a href=\"xxx\">test</a>.<p>"
			       "<ul><li>Uno<li>Dos<li>Tres</ul>"
			       "</html>");
	gtk_container_add (GTK_CONTAINER (window), html);
	gtk_widget_show (html);
	gtk_main ();

}

