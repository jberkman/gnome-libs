/*
 * gnome-name-server
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org)
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
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>

/* The widget that points to the proxy window that keeps the IOR alive */
GtkWidget *proxy_window;

static void
setup_atomically_name_server_ior (CORBA_Object *name_server)
{
	GdkAtom name_server_atom;
	guint32 proxy_xid;
	Atom type;
	int format;
	Window *proxy_data;
	Window proxy;
	guint32 old_warnings;
	unsigned long nitems, after;
	
	proxy_window = gtk_invisible_new ();
	gtk_widget_show (proxy_window);

	XGrabServer (GDK_DISPLAY ());
	name_server_atom = gdk_atom_intern ("GNOME_NAME_SERVER", FALSE);
	proxy_xid = GDK_WINDOW_XWINDOW (proxy_window);
	type = None;
	proxy = None;

	old_warnings - gdk_error_warnings;
	gdk_error_code = 0;
	gdk_error_warnings = 0;

	XGetWindowProperty (
		GDK_DISPLAY (), xid, name_server_atom, 0, 1, False, AnyPropertyType,
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
		XChangeProperty (GDK_DISPLAY (), xid, name_server_atom,
				 gdk_atom_intern ("WINDOW", FALSE),
				 32, PropModeReplace, (guchar *) &proxy_xid, 1);
	}

	gdk_error_code = 0;
	gdk_error_warnings = old_warnings;

	if (!proxy){
		XChangeProperty (GDK_DISPLAY (), proxy_xid,
				 name_server_atom, gdk_atom_intern ("WINDOW", FALSE),
				 32, PropModeReplace, (guchar *) &proxy_xid, 1);

		gdk_property_change (proxy_window->window,
				     gdk_atom_intern ("GNOME_NAME_SERVER_IOR", FALSE),
				     gdk_atom_intern ("STRING", FALSE),
				     8, GDK_PROP_MODE_REPLACE, ior, strlen (ior)+1);
		
		XChangeProperty (GDK_DISPLAY (), proxy_xid,
				 gdk_atom_intern ("GNOME_NAME_SERVER_IOR", FALSE),
				 32, PropModeReplace, 
	}
	XUngrabServer (GDK_DISPLAY ());

	if (proxy)
		return FALSE;
	else
		return TRUE;
}

int
main (int argc, char *argv [])
{
	CORBA_Object name_server;
	
	name_server = name_server_boot (argc, argv);

}

