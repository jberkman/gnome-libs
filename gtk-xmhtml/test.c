#include <stdio.h>
#include <gtk/gtk.h>
#include "gtk-xmhtml.h"

char *urls [] = {
	"unknown", "named (...)", "jump (#...)",
	"file_local (file.html)", "file_remote (file://foo.bar/file)",
	"ftp", "http", "gopher", "wais", "news", "telnet", "mailto",
	"exec:foo_bar", "internal"
};

void
click (GtkWidget *widget, gpointer data)
{
	XmHTMLAnchorCallbackStruct *cbs = (XmHTMLAnchorCallbackStruct *) data;
	
	printf ("click!\n");
	printf ("URLtype: %s\n", urls [cbs->url_type]);
	printf ("line:    %d\n", cbs->line);
	printf ("href:    %s\n", cbs->href);
	printf ("target:  %s\n", cbs->target);
	printf ("rel:     %s\n", cbs->rel);
	printf ("rev:     %s\n", cbs->rev);
	printf ("title:   %s\n", cbs->title);
	printf ("doit:    %d\n", cbs->doit);
	printf ("visited: %s\n", cbs->visited);
}

int
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
	if(argc == 2) {
		afile = fopen(argv[1], "r");
		if(afile != NULL) {
		  while(fgets(aline, sizeof(aline), afile))
			file_contents = g_string_append(file_contents, aline);
		  fclose(afile);
		}
	}
	if(strlen(file_contents->str) <= 0)
		file_contents = g_string_append(file_contents, "<html>\n"
			       "<head><title>Hola</title></head>\n"
			       "<b>Bold</b><p>Nuevo parrafo<p><a href=\"xxx\">test</a>.<p>"
			       "<ul><li>Uno<li>Dos<li>Tres</ul>"
			       "</html>");
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		GTK_SIGNAL_FUNC(gtk_true), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "destroy",
		GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
	html = gtk_xmhtml_new (file_contents->str);
	gtk_widget_show (html);

	gtk_signal_connect (GTK_OBJECT(html), "activate", (GtkSignalFunc) click, html);
	
	gtk_container_add (GTK_CONTAINER (window), html);
	gtk_main ();
	return 0;
}

