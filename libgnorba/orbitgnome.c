#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gnorba.h"

#include "libgnome/gnomelib-init2.h"

extern struct poptOptions goad_popt_options[];
extern GnomeModuleInfo gnorba_module_info, gtk_module_info;

static void gnorbaui_pre_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info);
static void gnorbaui_post_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info);
extern void _gnome_gnorba_cookie_setup(Display *disp, Window rootwin);

static GnomeModuleRequirement gnorbaui_requirements[] = {
  {VERSION, &gnorba_module_info},
  {NULL, &gtk_module_info},
  {NULL, NULL}
};

GnomeModuleInfo gnorbaui_module_info = {
  "gnorbaui", VERSION, "Gnorba/GUI",
  gnorbaui_requirements,
  gnorbaui_pre_args_parse,
  gnorbaui_post_args_parse,
  goad_popt_options
};

static void
gnorbaui_pre_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
  /* Set default values */
  gnome_program_attributes_set(app,
			  GNORBA_PARAM_SERVER_FUNC, FALSE,
			  GNORBA_PARAM_USE_COOKIES, TRUE,
			  GNORBA_PARAM_HIGH_PRIORITY, FALSE,
			  GNORBA_PARAM_USE_X11, TRUE,
			  NULL);
}

static void
gnorbaui_post_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
  gboolean server_func = FALSE;

  gnome_program_attributes_get(app, 
			  GNORBA_PARAM_SERVER_FUNC, &server_func,
			  NULL);
}

/**
 * gnome_CORBA_init:
 * @app_id: Application id.
 * @app_version: Application version.
 * @argc: pointer to argc (for example, as received by main)
 * @argv: pointer to argv (for example, as received by main)
 * @gnorba_flags: GNORBA initialization flags.
 * @ev: CORBA Environment to return CORBA errors.
 *
 * Wrapper around the GNORBA CORBA support routines for GNOME applications.
 * This routine takes care of calling gnome_init after registering our
 * command line arguments.  After gnome_init is invoked, the GNORBA CORBA
 * setup is bootstrapped by calling gnorba_CORBA_init for you.
 *
 * Returns the CORBA_ORB for this application.
 */
CORBA_ORB
gnome_CORBA_init (const char *app_id,
		  const char *app_version,
		  int *argc, char **argv,
		  GnorbaInitFlags gnorba_flags,
		  CORBA_Environment *ev)
{
	CORBA_ORB retval;

	if(gnorba_flags & GNORBA_INIT_SERVER_FUNC)
		goad_register_arguments();

	gnome_init(app_id, app_version, *argc, argv);
	retval = gnorba_CORBA_init(argc, argv,
				   gnorba_flags|GNORBA_INIT_DISABLE_COOKIES,
				   ev);

	if(!(gnorba_flags & GNORBA_INIT_DISABLE_COOKIES))
	  _gnome_gnorba_cookie_setup(GDK_DISPLAY(), GDK_ROOT_WINDOW());

	return retval;
}

/**
 * gnome_CORBA_init_with_popt_table:
 * @app_id: Application id.
 * @app_version: Application version.
 * @argc: pointer to argc (for example, as received by main)
 * @argv: pointer to argc (for example, as received by main)
 * @options: an array of poptOptions.
 * @popt_flags: flags passes to popt.
 * @return_ctx: if non-NULL a popt context is returned here.
 * @gnorba_flags: GNORBA initialization flags.
 * @ev:CORBA Environment to return CORBA errors.
 *
 * Wrapper around the GNORBA CORBA support routines for GNOME applications.
 * This routine takes care of calling gnome_init after registering our
 * command line arguments.  After gnome_init is invoked, the GNORBA CORBA
 * setup is bootstrapped by calling gnorba_CORBA_init for you.
 *
 * This differs from gnome_CORBA_init only in that it provides a way
 * to pass a table of popt options for argument parsing.
 *
 * If you pass return_ctx, you need to release it with the proper popt
 * routine where you are done.
 *
 * Returns the CORBA_ORB for this application.
 *
 */
CORBA_ORB
gnome_CORBA_init_with_popt_table (const char *app_id,
				  const char *app_version,
				  int *argc, char **argv,
				  const struct poptOption *options,
				  int popt_flags,
				  poptContext *return_ctx,
				  GnorbaInitFlags gnorba_flags,
				  CORBA_Environment *ev)
{
	CORBA_ORB retval;

	if(gnorba_flags & GNORBA_INIT_SERVER_FUNC)
		goad_register_arguments();

	gnome_init_with_popt_table(app_id, app_version, *argc, argv, options,
				   popt_flags, return_ctx);

	retval = gnorba_CORBA_init(argc, argv,
				   gnorba_flags|GNORBA_INIT_DISABLE_COOKIES,
				   ev);

	if(!(gnorba_flags & GNORBA_INIT_DISABLE_COOKIES))
	  _gnome_gnorba_cookie_setup(GDK_DISPLAY(), GDK_ROOT_WINDOW());

	return retval;
}

