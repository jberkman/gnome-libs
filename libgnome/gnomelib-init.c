#include <config.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib.h>
#include "gnome-defs.h"
#include "gnome-util.h"
#include "gnome-i18n.h"

char *gnome_user_home_dir = 0;
char *gnome_user_dir = 0;
char *gnome_app_id = 0;

void
gnomelib_init (char *app_id)
{
	gnome_user_home_dir = getenv ("HOME");
	/* never freed - gnome_config currently uses this, and it's better
	   to figure it out once than to repeatedly get it */
	gnome_user_dir = g_concat_dir_and_file (gnome_user_home_dir, ".gnome");
	mkdir (gnome_user_dir, 0755);
	gnome_app_id = app_id;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
}

/* Register any command-line arguments we might have.  */
void
gnomelib_register_arguments (void)
{
}
