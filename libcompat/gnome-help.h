#include <glib.h>

typedef struct {
    gchar *name;
    gchar *path;
} GnomeHelpMenuEntry;

/* gnome_help_file_path() - Return a fully resolved file name for path. */
/*                          User need to g_free path when done.         */
gchar *gnome_help_file_path(gchar *app, gchar *path);

/* gnome_help_goto() - Cause a help viewer to display help entry. */
void gnome_help_display(void *ignore, GnomeHelpMenuEntry *ref);

/* gnome_help_goto() - Cause a help viewer to display file. */
void gnome_help_goto(void *ignore, gchar *file);
