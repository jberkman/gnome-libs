#include <config.h>

#include <gnome.h>


GtkWidget *create_newwin(gboolean normal, gchar *appname, gchar *title);
#ifdef HAVE_DEVGTK
void create_canvas (void);
#endif
