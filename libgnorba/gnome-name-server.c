/*
 * gnome-name-server
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org)
 *   Elliot Lee (sopwith@cuc.edu)
 *
 * This is just a version of the name service that registers its IOR
 * with the X server.  That way GNOME applications running against that
 * display will be able to find the naming service. 
 *
 * If the name server got killed for any reason, it is importnat to have
 * a way to reliably find this situation, we use the following trick to
 * achieve this:
 *
 * The GNOME_NAME_SERVICE property on the X root window is set to point
 * to an invisible window (the window id).  On this window, we store also
 * a GNOME_NAME_SERVICE property that points to itself.  Additionally,
 * a GNOME_NAME_SERVICE_IOR property is kept there with the IOR to this
 * name server.
 *
 * In the event of the name-server being killed, we have a reliable way
 * of finding this out.
 *
 * Code and ideas are based on Owen Taylor's and Federico Mena's proxy window
 * for the Midnight Commander.
 */
#include <config.h>
#include <gnome.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <gtk/gtkinvisible.h>
#include <ORBitservices/CosNaming.h>
#include <ORBitservices/CosNaming_impl.h>
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include "gnorba.h"

/* The widget that points to the proxy window that keeps the IOR alive */
GtkWidget *proxy_window;

static gboolean
setup_atomically_name_server_ior (CORBA_char *ior)
{
	GdkAtom name_server_atom, name_server_ior_atom, window_atom, string_atom;
	guint32 proxy_xid;
	Atom type;
	int format;
	Window *proxy_data;
	Window proxy;
	guint32 old_warnings;
	unsigned long nitems, after;
	
	proxy_window = gtk_invisible_new ();
	gtk_widget_show (proxy_window);
		
	name_server_atom = gdk_atom_intern ("GNOME_NAME_SERVER", FALSE);
	name_server_ior_atom = gdk_atom_intern ("GNOME_NAME_SERVER_IOR", FALSE);
	window_atom = gdk_atom_intern ("WINDOW", FALSE);
	string_atom = gdk_atom_intern ("STRING", FALSE),
	
	XGrabServer (GDK_DISPLAY ());
	proxy_xid = GDK_WINDOW_XWINDOW (proxy_window->window);
	type = None;
	proxy = None;

	old_warnings = gdk_error_warnings;
	gdk_error_code = 0;
	gdk_error_warnings = 0;

	XGetWindowProperty (
		GDK_DISPLAY (), GDK_ROOT_WINDOW(), name_server_atom, 0, 1, False, AnyPropertyType,
		&type, &format, &nitems, &after, (guchar **) &proxy_data);

	if (type != None){
		if ((format == 32) && (nitems == 1))
			proxy = *proxy_data;

		XFree (proxy_data);
	}

	/*
	 * If the property was set, check if the window it poitns to exists and has
	 * a "GNOME_NAME_SERVER" property pointing to itself
	 */
	if (proxy){
		XGetWindowProperty (GDK_DISPLAY (), proxy,
				    name_server_atom, 0,
				    1, False, AnyPropertyType,
				    &type, &format, &nitems, &after,
				    (guchar **) &proxy_data);

		if (!gdk_error_code && type != None){
			if ((format == 32) && (nitems == 1))
				if (*proxy_data != proxy)
					proxy = GDK_NONE;
			XFree (proxy_data);
		} else
			proxy = GDK_NONE;
	}

	/*
	 * If the window was invalid, set the property to point to us
	 */
	if (!proxy){
		syslog (LOG_INFO, "Stale reference to the GNOME name server");
		XChangeProperty (GDK_DISPLAY (), GDK_ROOT_WINDOW(), name_server_atom,
				 window_atom,
				 32, PropModeReplace, (guchar *) &proxy_xid, 1);
	}

	if (!proxy){
		XChangeProperty (
			GDK_DISPLAY (), proxy_xid,
			name_server_atom, window_atom,
			32, PropModeReplace, (guchar *) &proxy_xid, 1);

		XChangeProperty (
			GDK_DISPLAY (), proxy_xid,
			name_server_ior_atom, string_atom,
			8, PropModeReplace, (guchar *) ior, strlen (ior)+1);
	}
	XUngrabServer (GDK_DISPLAY ());
	gdk_flush ();
	gdk_error_code = 0;
	gdk_error_warnings = old_warnings;
	gdk_flush ();

	if (proxy)
		return FALSE;
	else
		return TRUE;
}

static void
signal_handler (int signo)
{
  syslog (LOG_ERR, "Receveived signal %d\nshutting down.", signo);

  switch(signo) {
    case SIGSEGV:
	abort();

    default:
	exit(1);
  }
}


static gboolean
setup_name_server (CORBA_Object name_service, CORBA_Environment *ev)
{
	CORBA_Object gnome_context  = CORBA_OBJECT_NIL;
	CORBA_Object server_context = CORBA_OBJECT_NIL;
	CosNaming_NameComponent nc[2] = {{"GNOME","subcontext"},
				 {"Servers", "subcontext"}};
	CosNaming_Name context_name = {2, 2, &nc, FALSE};

	context_name._length = 1;
	/*
	  Create the default context "/GNOME/Servers"
	*/
	gnome_context = CosNaming_NamingContext_bind_new_context(name_service, &context_name, ev);
	if (ev->_major != CORBA_NO_EXCEPTION) {
		g_warning(_("Creating '/GNOME' context %s %d"), __FILE__, __LINE__);
		switch( ev->_major ) {
		case CORBA_SYSTEM_EXCEPTION:
			g_warning("sysex: %s.\n", CORBA_exception_id(ev));
			break;
		case CORBA_USER_EXCEPTION:
			g_warning("usrex: %s.\n", CORBA_exception_id( ev ) );
		default:
			break;
		}
		return FALSE;
	}
	CORBA_Object_release(gnome_context, ev);

	context_name._length = 2;
	server_context = CosNaming_NamingContext_bind_new_context(name_service, &context_name, ev);
	if (ev->_major != CORBA_NO_EXCEPTION) {
		g_warning(_("Creating '/GNOME/Servers' context %s %d"), __FILE__, __LINE__);
		switch( ev->_major ) {
		case CORBA_SYSTEM_EXCEPTION:
			g_warning("	sysex: %s.\n", CORBA_exception_id(ev));
			break;
		case CORBA_USER_EXCEPTION:
			g_warning("usr	ex: %s.\n", CORBA_exception_id( ev ) );
		default:
			break;
		}
		return FALSE;
	}

	CORBA_Object_release(server_context, ev);

	return TRUE;
}

int
main (int argc, char *argv [])
{
	CORBA_Object name_server;
	CORBA_ORB orb;
	CORBA_Environment ev;
	PortableServer_POA root_poa;
	PortableServer_POAManager pm;
	struct sigaction act;
	sigset_t empty_mask;
	CORBA_char *ior;
	gboolean v;
	
	/* Logs */
	openlog ("gnome-name-server", LOG_NDELAY | LOG_PID, LOG_DAEMON);
	syslog (LOG_INFO, "starting");

	/* Session setup */
	sigemptyset (&empty_mask);
	act.sa_handler = signal_handler;
	act.sa_mask    = empty_mask;
	act.sa_flags   = 0;
	sigaction (SIGINT,  &act, 0);
	sigaction (SIGHUP,  &act, 0);
	sigaction (SIGSEGV, &act, 0);
	sigaction (SIGABRT, &act, 0);

	act.sa_handler = SIG_IGN;
	sigaction (SIGINT, &act, 0);
	
	sigemptyset (&empty_mask);

	CORBA_exception_init (&ev);
	orb = gnome_CORBA_init ("gnome-name-service", "1.0", &argc, argv, GNORBA_INIT_DISABLE_COOKIES, &ev);

	root_poa = (PortableServer_POA)
		CORBA_ORB_resolve_initial_references (orb, "RootPOA", &ev);

	name_server = impl_CosNaming_NamingContext__create (root_poa, &ev);
	if (!setup_name_server (name_server, &ev)){
		syslog (LOG_INFO, "Could not setup the name server\n");
		CORBA_Object_release (name_server, &ev);
		return 1;
	}

	ior = CORBA_ORB_object_to_string (orb, name_server, &ev);

	v = setup_atomically_name_server_ior (ior);

	if (!v){
		syslog (LOG_INFO, "name server was running on display, exiting");
		CORBA_Object_release (name_server, &ev);
		return 1;
	} 

	printf ("%s\n", ior);
	fflush (stdout);
	CORBA_free (ior);
	
	pm = PortableServer_POA__get_the_POAManager (root_poa, &ev);
	PortableServer_POAManager_activate (pm, &ev);

	
	gtk_main ();
	syslog (LOG_INFO, "exiting");
	return 0;
}


