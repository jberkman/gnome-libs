#include <config.h>

#include <gnome.h>


GtkWidget *create_newwin(gboolean normal, gchar *appname, gchar *title);

#ifdef GTK_HAVE_FEATURES_1_1_0
void create_canvas (void);
GtkWidget *create_canvas_primitives (void);
GtkWidget *create_canvas_arrowhead (void);
GtkWidget *create_canvas_fifteen (void);
#endif
