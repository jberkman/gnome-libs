#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gnorba.h"

extern CORBA_ORB _gnorba_gnome_orbit_orb;

/*
 * get_name_server_ior_from_root_window:
 *
 * This gets the IOR for the name server in a reliable way.
 * The root window has a property set "CORBA_NAME_SERVICE"
 * which has a Window ID.  We then fetch that Window's
 * CORBA_NAME_SERVICE property and if it points to itself,
 * we know we had a valid and running name service
 */
static char *
get_name_server_ior_from_root_window (void)
{
	GdkAtom name_server_atom, name_server_ior_atom;
	GdkAtom string_atom, window_atom;
	Atom type;
	char *ior = NULL, *result;
	Window *proxy_data;
	Window proxy;
	int format;
	unsigned long nitems, after;
	guint32 old_warnings;
	
	old_warnings = gdk_error_warnings;
	gdk_error_warnings =0;
	gdk_error_code = 0;

	type = None;
	proxy = None;
	
	name_server_atom = gdk_atom_intern("GNOME_NAME_SERVER", FALSE);
	name_server_ior_atom = gdk_atom_intern("GNOME_NAME_SERVER_IOR", FALSE);
	string_atom = gdk_atom_intern("STRING", FALSE);
	window_atom = gdk_atom_intern("WINDOW", FALSE);

	proxy_data = NULL;
	XGetWindowProperty (
		GDK_DISPLAY (), GDK_ROOT_WINDOW(),
		name_server_atom, 0, 1, False, AnyPropertyType,
		&type, &format, &nitems, &after, (guchar **) &proxy_data);

	if (type == None)
		goto error;

	if (type != None){
		if ((format == 32) && (nitems = 1))
			proxy = *proxy_data;

		XFree (proxy_data);
	}

	proxy_data = NULL;
	XGetWindowProperty (
		GDK_DISPLAY (), proxy,
		name_server_atom, 0, 1, False, AnyPropertyType,
		&type, &format, &nitems, &after, (guchar **) &proxy_data);

	if (!gdk_error_code && type != None){
		if ((format == 32) && (nitems == 1))
			if (*proxy_data != proxy){
				XFree (proxy_data);
				goto error;
			}
	} else
		proxy = GDK_NONE;

	XFree (proxy_data);
	
	if (proxy == GDK_NONE)
		goto error;

	XGetWindowProperty (GDK_DISPLAY(), proxy,
			    name_server_ior_atom, 0, 9999, False, string_atom,
			    &type, &format, &nitems, &after, (guchar **)&ior);

	if (type != string_atom){
		if (type != None)
			XFree (ior);
		goto error;
	}

	result = g_strdup (ior);
	XFree(ior);
	
	gdk_flush ();
	gdk_error_code = 0;
	gdk_error_warnings = old_warnings;
	gdk_flush ();

	return result;

error:
	gdk_flush ();
	gdk_error_code = 0;
	gdk_error_warnings = old_warnings;
	gdk_flush ();

	return NULL;
}

static CORBA_Object
name_server_by_forking (CORBA_Environment *ev)
{
	CORBA_Object name_service = CORBA_OBJECT_NIL;
	int iopipes[2];
	char iorbuf[2048];
	pid_t pid;
	
	/*
	 * Since we're pretty sure no name server is running,
	 * we start it ourself and tell the (GNOME session)
	 * world about it
	 */
	
	/* fork & get the ior from orbit-name-service stdout */
	pipe(iopipes);

	pid = fork ();
	if (pid == -1)
		return name_service;
	
	if (pid) {
		FILE *iorfh;
		char *ret;
		int status;

		/* Parent */
		waitpid(pid, &status, 0); /* de-zombify */

		close(iopipes[1]);
		
		iorfh = fdopen(iopipes[0], "r");
		
		while ((ret = fgets(iorbuf, sizeof(iorbuf), iorfh)) != NULL) {
			if (strncmp(iorbuf, "IOR:", 4) == 0) {
				break;
			}
		}

		if (!ret)
			return name_service;
		
		fclose(iorfh);
		
		/* strip newline if it's there */
		if (iorbuf[strlen(iorbuf)-1] == '\n')
			iorbuf[strlen(iorbuf)-1] = '\0';
		
		name_service = CORBA_ORB_string_to_object((CORBA_ORB)_gnorba_gnome_orbit_orb, iorbuf, ev);

	} else if (fork ()) {
		/* de-zombifier process, just exit */
		_exit(0);
	} else {
		/* Child of a child. We run the naming service */
		struct sigaction sa;
		struct rlimit rl;
		int    i;
		
		getrlimit(RLIMIT_NOFILE, &rl);
		i = rl.rlim_cur;
		
		sa.sa_handler = SIG_IGN;
		sigaction(SIGPIPE, &sa, 0);
		close(0);
		close(iopipes[0]);
		dup2(iopipes[1], 1);
		dup2(iopipes[1], 2);
		/* close all file descriptors */
		while (i > 2) {
			close(i);
			i--;
		}
		
		setsid();
		
		execlp("gnome-name-service", "gnome-name-service", NULL);
		_exit(1);
	}

	return name_service;
}

/**
 * gnome_name_service_get:
 *
 * This routine returns an object reference to the name server
 * for this user/session pair.    This launches the name server if
 * it is the first application running it.
 *
 * Returns: an object reference to the name service.
 *
 */
CORBA_Object
gnome_name_service_get(void)
{
	static CORBA_Object name_service = CORBA_OBJECT_NIL;
	CORBA_Object retval = CORBA_OBJECT_NIL;
	CORBA_Environment ev;
	char *ior;
	int attempts;
	
	g_return_val_if_fail(_gnorba_gnome_orbit_orb, CORBA_OBJECT_NIL);
	
	CORBA_exception_init(&ev);

	/*
	 * Actually, only 2 attempts would be enough
	 *
	 * We need this as 2 gnome-name-servers might be started
	 * at the same time, so the name_server_by_forking might
	 * fail.
	 */
	for (attempts = 0; attempts < 3; attempts++){
		ior = get_name_server_ior_from_root_window ();
		if (ior) {
			name_service = CORBA_ORB_string_to_object(_gnorba_gnome_orbit_orb, ior, &ev);
			g_free (ior);

			if (!CORBA_Object_is_nil (name_service, &ev)){
				retval = CORBA_Object_duplicate (name_service, &ev);
				CORBA_exception_free (&ev);
				return retval;
			}
		}

		name_service = name_server_by_forking (&ev);
		if (!CORBA_Object_is_nil (name_service, &ev)){
			retval = CORBA_Object_duplicate(name_service, &ev);
			CORBA_exception_free(&ev);
			return retval;
		}
	}
	
	g_warning(_("Could not get name service!"));
	
	return name_service;
}

