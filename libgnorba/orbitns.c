#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gnorba.h"

extern CORBA_ORB gnome_orbit_orb;

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
  CORBA_Object gnome_context       = CORBA_OBJECT_NIL;
  CORBA_Object server_context      = CORBA_OBJECT_NIL;
  CORBA_Object retval = NULL;
  char *ior;
  GdkAtom propname, proptype;
  CORBA_Environment ev;
  CosNaming_NameComponent nc;
  CosNaming_Name          context_name;
  
  g_return_val_if_fail(gnome_orbit_orb, CORBA_OBJECT_NIL);

  CORBA_exception_init(&ev);

  if(CORBA_Object_is_nil(name_service, &ev)) {
    int iopipes[2];
    char iorbuf[2048];

    ior = get_name_server_ior_from_root_window ();
    
    if (ior) {
	name_service = CORBA_ORB_string_to_object(gnome_orbit_orb, ior, &ev);
	g_free (ior);
    }

    if (!CORBA_Object_is_nil(name_service, &ev)) {
      CosNaming_NameComponent nc = {"GNOME", "subcontext"};
      CosNaming_Name          nom = {0, 1, &nc, CORBA_FALSE};
      CORBA_Object            gnome_context;
      
      gnome_context = CosNaming_NamingContext_resolve(name_service, &nom, &ev);
      
      if (gnome_context != CORBA_OBJECT_NIL && ev._major == CORBA_NO_EXCEPTION)
	/* Everything is well, the orbit-name server found is working */
	goto out;
      
      if (ev._major == CORBA_SYSTEM_EXCEPTION && !CORBA_exception_id(&ev)) {
	/*
	 * The property is set, but the name server is not running.
	 * Maybe we should use the same trick here, as we are using in the name server code
	 * to detect dead servers
	*/
      } else if (ev._major != CORBA_NO_EXCEPTION) {
	/*
	 * An error occured. Inform the user, and start a fresh name server.
	 */
	switch (ev._major) {
	case CORBA_USER_EXCEPTION:
	  g_warning(_("Unexpected USER exception '%s', during orbit-name-server detection\n"),
		    CORBA_exception_id(&ev));
	  break;
	case CORBA_SYSTEM_EXCEPTION:
	  g_warning(_("Unexpected SYSTEM exception '%s', during orbit-name-server detection\n"),
		    CORBA_exception_id(&ev));
	default:
	}
      }
      
    }
    /*
     * Since we're pretty sure no name server is running, we start it ourself
     * and tell the (GNOME session) world about it
     */
    /* fork & get the ior from orbit-name-service stdout */
    pipe(iopipes);

    if(fork()) {
      FILE *iorfh;

      /* Parent */
      close(iopipes[1]);

      iorfh = fdopen(iopipes[0], "r");

      while(fgets(iorbuf, sizeof(iorbuf), iorfh) && strncmp(iorbuf, "IOR:", 4))
	/* Just read lines until we get what we're looking for */ ;

      if(strncmp(iorbuf, "IOR:", 4)) {
	name_service = CORBA_OBJECT_NIL;
	goto out;
      }

      fclose(iorfh);
      /* strip newline if it's there */
      if (iorbuf[strlen(iorbuf)-1] == '\n')
	iorbuf[strlen(iorbuf)-1] = '\0';

      name_service = CORBA_ORB_string_to_object(gnome_orbit_orb, iorbuf, &ev);
      if (name_service != CORBA_OBJECT_NIL) {
	  /*
	    Create the default context "/GNOME/servers"
	  */
	context_name._maximum = 0;
	context_name._length  = 1;
	context_name._buffer = &nc;
	context_name._release = CORBA_FALSE;
	nc.id = "GNOME";
	nc.kind = "subcontext";
	gnome_context = CosNaming_NamingContext_bind_new_context(name_service, &context_name, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
	  g_warning(_("Creating '/GNOME' context %s %d"), __FILE__, __LINE__);
	  switch( ev._major ) {
	  case CORBA_SYSTEM_EXCEPTION:
	    g_warning("sysex: %s.\n", CORBA_exception_id(&ev));
	    break;
	  case CORBA_USER_EXCEPTION:
	    g_warning("usrex: %s.\n", CORBA_exception_id( &ev ) );
	  default:
	    break;
	  }
	}
	ev._major = CORBA_NO_EXCEPTION;
	if (CORBA_Object_is_nil(gnome_context, &ev)) {
	  g_warning(_("gnome_name_server_get: '/GNOME' context is nil\n"));
	  return CORBA_OBJECT_NIL;
	}

	nc.id="Servers";
	nc.kind = "subcontext";
	server_context = CosNaming_NamingContext_bind_new_context(gnome_context, &context_name, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
	  g_warning(_("Creating '/GNOME/servers' context %s %d"), __FILE__, __LINE__);
	  switch( ev._major ) {
	  case CORBA_SYSTEM_EXCEPTION:
	    g_warning("	sysex: %s.\n", CORBA_exception_id(&ev));
	    break;
	  case CORBA_USER_EXCEPTION:
	    g_warning("usr	ex: %s.\n", CORBA_exception_id( &ev ) );
	  default:
	    break;
	  }
	}
	if (CORBA_Object_is_nil(server_context, &ev)) {
	  g_warning(_("gnome_name_server_get: '/GNOME/servers context is nil\n"));
	  return CORBA_OBJECT_NIL;
	}
      } else {
	return CORBA_OBJECT_NIL;
      }
    } else if(fork()) {
      _exit(0); /* de-zombifier process, just exit */
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
      execlp("orbit-name-server", "orbit-name-server", NULL);
      _exit(1);
    }
  }
  
 out:
  if(!CORBA_Object_is_nil(name_service, &ev))
    retval = CORBA_Object_duplicate(name_service, &ev);
  else
    g_warning(_("Could not get name service!"));

  CORBA_exception_free(&ev);
  return retval;
}

