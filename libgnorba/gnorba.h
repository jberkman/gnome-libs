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
CORBA_ORB gnome_CORBA_init(char *app_id,
			   struct argp *app_parser,
			   int *argc, char **argv,
			   unsigned int flags,
			   int *arg_index,
			   CORBA_Environment *ev);
CORBA_Object gnome_get_name_service(void);

/**** gnome-plugins module ****/
typedef struct {
  const char *repo_id, *id, *kind, *description;
  CORBA_Object (*activate)(PortableServer_POA poa,
			   gpointer *impl_ptr,
			   CORBA_Environment *ev);
  void       (*deactivate)(PortableServer_POA poa,
			   gpointer impl_ptr,
			   CORBA_Environment *ev);
} GnomePluginObject;

typedef struct {
  const GnomePluginObject *plugin_object_list;
  const char *description;
} GnomePlugin;

/* Returns an array of plugin ID's */
char **gnome_plugin_get_available_plugins(void);
/* Loads the plugin and returns the GnomePlugin structure for it, */
const GnomePlugin *gnome_plugin_use(const char *plugin_id);

/**** goad module ****/
typedef enum { GOAD_SERVER_SHLIB,
	       GOAD_SERVER_EXE,
	       GOAD_SERVER_RELAY
} GoadServerType;

typedef struct {
  GoadServerType type;
  const char *repo_id, *id, *description;
  const char *location_info; /* executable/shlib path, relayer IOR, whatever */
} GoadServer;

typedef enum {
  /* these two are mutually exclusive */
  GOAD_ACTIVATE_SHLIB = 1 << 0, /* prefer shlib activation */
  GOAD_ACTIVATE_REMOTE = 1 << 1, /* prefer remote activation */

  /* these two are mutually exclusive */
  GOAD_ACTIVATE_EXISTING_ONLY = 1 << 2 /* Only do lookup in name
					  service for currently running
					  version. */
  GOAD_ACTIVATE_NEW_ONLY = 1 << 3 /* No lookup in name service. */
} GoadActivationFlags;

const GoadServer *goad_servers_list(void);
CORBA_Object goad_server_activate(GoadServer *sinfo, GoadActivationFlags flags);
/* Picks the first one on the list that meets criteria */
CORBA_Object goad_server_activate_with_repo_id(const char *repo_id,
					       GoadActivationFlags flags);

#endif
