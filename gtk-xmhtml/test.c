#include <stdio.h>
#include <gtk/gtk.h>
#include "gtk-xmhtml.h"

main (int argc, char *argv [])
{
	GtkWidget *window, *html, *scr;
	char *p = malloc (10);
	GString *file_contents;
	char aline[1024];
	FILE *afile = NULL;
			  
	gtk_init (&argc, &argv);
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_show (window);
	file_contents = g_string_new(NULL);
	afile = fopen(argv[1], "r");
	if(afile != NULL) {
	  while(fgets(aline, sizeof(aline), afile))
		file_contents = g_string_append(file_contents, aline);
	  fclose(afile);
	}
	if(argc < 2 || afile == NULL)
		file_contents = g_string_append(file_contents, "<html>\n"
			       "<head><title>Hola</title></head>\n"
			       "<b>Bold</b><p>Nuevo parrafo<p><a href=\"xxx\">test</a>.<p>"
			       "<ul><li>Uno<li>Dos<li>Tres</ul>"
			       "</html>");
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		GTK_SIGNAL_FUNC(gtk_true), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
		GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	html = gtk_xmhtml_new (200, 200, file_contents->str);
	scr = gtk_scrolled_window_new(GTK_XMHTML(html)->hsba, GTK_XMHTML(html)->vsba);
	gtk_container_add (GTK_CONTAINER (scr), html);
	gtk_widget_show (html);
	gtk_container_add (GTK_CONTAINER (window), scr);
	gtk_widget_show (scr);
	gtk_main ();

}

