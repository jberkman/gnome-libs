#ifndef GNORBA_H
#define GNORBA_H 1

#include <orb/orbit.h>
#include <libgnorba/name-service.h>
#include <gnome.h>

/**** orbitgtk module ****/
/* 
 * Almost the same as gnome_init, except it initializes ORBit for use
 * with gtk+ too 
 */
CORBA_ORB gnome_CORBA_init          (char *app_id,
				     char *app_version,
				     int *argc, char **argv,
				     CORBA_Environment *ev);
CORBA_ORB gnome_CORBA_init_with_popt_table(char *app_id,
					   char *app_version,
					   int *argc, char **argv,
					   const struct poptOption *options,
					   int popt_flags,
					   poptContext *return_ctx,
					   CORBA_Environment *ev);

/*
  Gets the naming server from the X Propery on the root window. If
  this property does not exist, or the name server which has been
  registered has died, a new name server is started and a naming
  context object is returned.
*/
CORBA_Object gnome_name_service_get       (void);

/* register an object with the name server. name_server is the object
   returned by a call to gnome_name_service_get, and server is your
   CORBA server.

   Return -1 on error,
          -2 if another server with the same NAME and
	     KIND is already active and running,
	   0 otherwise;
   You might check ev for more error information.
*/
int          gnome_register_corba_server  (CORBA_Object name_server, CORBA_Object server,
					   gchar* name, gchar* kind, CORBA_Environment* ev);
/*
  Deregister name from the name server.
*/
int          gnome_unregister_corba_server  (CORBA_Object name_server,
					     gchar* name, gchar* kind, CORBA_Environment* ev);



/**** gnome-plugins module ****/
typedef struct {
	const char   *repo_id;
	const char   *server_id;
	const char   *kind;
	const char   *description;
	CORBA_Object (*activate)   (PortableServer_POA poa,
				    gpointer *impl_ptr,
				    CORBA_Environment *ev);
	void         (*deactivate) (PortableServer_POA poa,
				    gpointer impl_ptr,
				    CORBA_Environment *ev);
} GnomePluginObject;

typedef struct {
	const GnomePluginObject *plugin_object_list;
	const char *description;
} GnomePlugin;

/* Returns an array of plugin ID's */
char **gnome_plugin_get_available_plugins (void);

/* Loads the plugin and returns the GnomePlugin structure for it, */
const GnomePlugin *gnome_plugin_use (const char *plugin_id);

/**** goad module ****/
typedef enum {
	GOAD_SERVER_SHLIB = 1,
	GOAD_SERVER_EXE = 2,
	GOAD_SERVER_RELAY = 3
} GoadServerType;

typedef struct {
	GoadServerType type;
	char     *repo_id;
	char     *server_id;
	char     *description;
	
        /*
	 * Executable/shlib path, relayer IOR, whatever.
	 * This field may disappear at any time. You have been warned ;-)
	 */
	char     *location_info;
} GoadServer;

typedef enum {
	/* these two are mutually exclusive */
	GOAD_ACTIVATE_SHLIB = 1 << 0, 	/* prefer shlib activation */
	GOAD_ACTIVATE_REMOTE = 1 << 1, 	/* prefer remote activation */

	/* these two are mutually exclusive */
	GOAD_ACTIVATE_EXISTING_ONLY = 1 << 2, /* Only do lookup in name
					       * service for currently running
					       * version.
					       */
	GOAD_ACTIVATE_NEW_ONLY = 1 << 3,      /* No lookup in name service. */

	/*
	  Since name server registration now happens in the server,
	  this is not necessary and the server doesn't know about it 
	  either.
	*/
#if 0	
	GOAD_ACTIVATE_NO_NS_REGISTER = 1 << 4 /* DON'T register this new
					       * server with the name service
					       */
#endif
} GoadActivationFlags;

/*
 * goad_servers_list:
 *
 * Return value:
 *   An array of GoadServers. The repo_id in the last array element is NULL
 */
GoadServer *      goad_server_list_get              (void);
void              goad_server_list_free             (GoadServer *server_list);

/*
 * Passing GOAD_ACTIVATE_{REMOTE,SHLIB} flags to this routine doesn't make sense,
 * since the activation info is already specified in 'sinfo'.
 */
CORBA_Object      goad_server_activate              (GoadServer *sinfo,
						     GoadActivationFlags flags);

/*
 * Picks the first one on the list that meets criteria.
 * You can pass in a NULL 'server_list' if you wish, and
 * this routine will call goad_server_list_get() internally.
 */
CORBA_Object      goad_server_activate_with_repo_id (GoadServer *server_list,
						     const char *repo_id,
						     GoadActivationFlags flags);

/*
 * Activates a specific server by its GOAD ID.
 */
CORBA_Object
goad_server_activate_with_id(GoadServer *server_list,
			     const char *server_id,
			     GoadActivationFlags flags);

#endif
