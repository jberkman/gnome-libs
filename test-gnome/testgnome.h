#include <config.h>

#include <gnome.h>


GtkWidget *create_newwin(gboolean normal, gchar *appname, gchar *title);
#ifdef GTK_HAVE_ACCEL_GROUP
void create_canvas (void);
#endif
