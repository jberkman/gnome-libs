#include <config.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

/* debugging hack */
#if defined(__GLIBC__) && defined(linux)
#include <syslog.h>
#include <malloc.h>
#endif
#include <glib.h>
#include "libgnomeP.h"
#include <errno.h>
#include <signal.h>

char *gnome_user_home_dir = 0;
char *gnome_user_dir = 0;
char *gnome_user_private_dir = 0;
char *gnome_app_id = 0, *gnome_app_version = 0;
char gnome_do_not_create_directories = 0;

static gboolean disable_sound = FALSE;
static gboolean enable_sound = FALSE;
static char *esound_host = NULL;

static void
create_user_gnome_directories (void)
{
	if (gnome_do_not_create_directories)
		return;
	
	if(
	   mkdir(gnome_user_dir, 0700) /* This is per-user info
					   - no need for others to see it */
	   && errno != EEXIST)
	  g_error("Could not create per-user Gnome directory <%s> - aborting\n",
		  gnome_user_dir);

	if(
	   mkdir(gnome_user_private_dir, 0700) /* This is private per-user info
						  mode 700 will be enforced!
						  maybe even other security
						  meassures will be taken */
	   && errno != EEXIST)
		g_error("Could not create private per-user Gnome directory <%s> - aborting\n",
			gnome_user_private_dir);

	/* change mode to 0700 on the private directory */
	if (chmod (gnome_user_private_dir, 0700) != 0)
		g_error (
			"Could not set mode 0700 on private per-user Gnome directory <%s> - aborting\n",
			gnome_user_private_dir);

}

#ifdef DEBUG
static void
dump_memusage(int signo)
{
#if defined(__GLIBC__) && defined(linux)
	struct mallinfo mi;
	mi = mallinfo();
	
	syslog(LOG_DEBUG, "uordblks = %d ordblks = %d hblkhd = %d",
	       mi.uordblks, mi.ordblks, mi.hblkhd);
#endif
}
#endif

static void
gnomelib_option_cb(poptContext ctx, enum poptCallbackReason reason,
		   const struct poptOption *opt, const char *arg,
		   void *data)
{
	gboolean real_enable_sound;
	
	switch(reason) {
	case POPT_CALLBACK_REASON_POST:
		real_enable_sound = disable_sound ?FALSE :enable_sound ? TRUE : gnome_config_get_bool ("/sound/system/settings/start_esd=true");
		
		if (real_enable_sound){
			if (esound_host)
				gnome_sound_init (esound_host);
			else
				gnome_sound_init (NULL);
		}
		
		gnome_triggers_init ();
		
#if defined(DEBUG) && defined(__GLIBC__) && defined(linux)
		{
			struct sigaction sa;
			openlog(program_invocation_name, LOG_PID|LOG_PERROR, LOG_USER);
			memset(&sa, 0, sizeof(sa));
			sa.sa_handler = dump_memusage;
			sigaction(SIGXFSZ, &sa, NULL);
		}
#endif
		break;
	default:
	}
}

static const struct poptOption gnomelib_options[] = {
	{ NULL, '\0', POPT_ARG_CALLBACK|POPT_CBFLAG_POST, gnomelib_option_cb, 0, NULL, NULL},
	{ "disable-sound", '\0', POPT_ARG_NONE,
	  &disable_sound, 0, N_("Disable sound server usage"), NULL},
	{ "enable-sound", '\0', POPT_ARG_NONE,
	  &enable_sound, 0, N_("Enable sound server usage"), NULL},
	{ "espeaker", '\0', POPT_ARG_STRING,
	  &esound_host, 0, N_("Host:port on which the sound server to use is running"),
	  N_("HOSTNAME:PORT")},
	POPT_AUTOHELP
	{ NULL, '\0', 0, NULL, 0 }
};

static void
gnomelib_register_options(void)
{
	gnomelib_register_popt_table(gnomelib_options, N_("GNOME Options"));
}

void
gnomelib_init (const char *app_id,
	       const char *app_version)
{
	gnome_app_id = (char *)app_id;
	gnome_app_version = (char *)app_version;

	gnome_user_home_dir = getenv ("HOME");

	if (!gnome_user_home_dir){
		char *user;
		struct passwd *pw;
		
		user = getenv ("USER");
		if (user)
			pw = getpwnam (user);
		else 
			pw = getpwuid (getuid ());

		if (pw)
			gnome_user_home_dir = g_strdup (pw->pw_dir);

		endpwent ();
	}

	if (gnome_user_home_dir == 0)
		gnome_user_home_dir = "/";
			
	/*
	 * never freed - gnome_config currently uses this, and it's better
	 * to figure it out once than to repeatedly get it
	 */
	gnome_user_dir = g_concat_dir_and_file (gnome_user_home_dir, ".gnome");
	gnome_user_private_dir = g_concat_dir_and_file (gnome_user_home_dir,
							".gnome_private");
	create_user_gnome_directories ();

	gnomelib_register_options();

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	gnome_i18n_init ();
}
