#include "gnorba.h"
#include <dirent.h>

#define SERVER_LISTING_PATH "/CORBA/Servers"

typedef struct {
  gpointer impl_ptr;
  char *id;
} ActiveServerInfo;

extern CORBA_ORB gnome_orbit_orb; /* In orbitgtk.c */
static GSList *our_active_servers = NULL;

static GoadServerType goad_server_typename_to_type(const char *typename);
static CORBA_Object goad_server_activate_shlib(GoadServer *sinfo,
					       GoadActivationFlags flags,
					       CORBA_Environment *ev);
static CORBA_Object goad_server_activate_exe(GoadServer *sinfo,
					     GoadActivationFlags flags,
					     CORBA_Environment *ev);
static void goad_servers_unregister_atexit(void);

/**** goad_server_list_get
      Description: Returns an array listing all the servers available
                   for activation.
 */
GoadServer *
goad_server_list_get(void)
{
  GArray *servinfo;
  GoadServer newval, *retval;
  gpointer iter;
  char *typename;
  GString *tmpstr;

  servinfo = g_array_new(TRUE, FALSE, sizeof(GoadServer));
  tmpstr = g_string_new(NULL);

  /*
   * XXX TODO: In the future we should probably check both system-wide
   * and per-user listings
   */
  iter = gnome_config_init_iterator_sections(SERVER_LISTING_PATH);
  while((iter = gnome_config_iterator_next(iter, NULL, &newval.id))) {

    g_string_sprintf(tmpstr, SERVER_LISTING_PATH "/%s/type",
		     newval.id);
    typename = gnome_config_get_string(tmpstr->str);
    newval.type = goad_server_typename_to_type(typename);
    g_free(typename);
    if(!newval.type) {
      g_warning("Server %s has invalid activation method.", newval.id);
      g_free(newval.id);
      continue;
    }

    g_string_sprintf(tmpstr, SERVER_LISTING_PATH "/%s/repo_id",
		     newval.id);
    newval.repo_id = gnome_config_get_string(tmpstr->str);
    
    g_string_sprintf(tmpstr, SERVER_LISTING_PATH "/%s/description",
		     newval.description);
    newval.description = gnome_config_get_string(tmpstr->str);

    g_string_sprintf(tmpstr, SERVER_LISTING_PATH "/%s/location_info",
		     newval.description);
    newval.location_info = gnome_config_get_string(tmpstr->str);
    
    g_array_append_val(servinfo, newval);
  }

  retval = (GoadServer *)servinfo->data;
  g_array_free(servinfo, FALSE);
  g_string_free(tmpstr, TRUE);

  return retval;
}
  
static GoadServerType
goad_server_typename_to_type(const char *typename)
{
  if(!strcmp(typename, "shlib"))
    return GOAD_SERVER_SHLIB;
  else if(!strcmp(typename, "exe"))
    return GOAD_SERVER_EXE;
  else if(!strcmp(typename, "relay"))
    return GOAD_SERVER_RELAY;
  else
    return 0; /* Invalid */
}

/**** goad_server_list_free
      Inputs: 'server_list' - an array of GoadServer structures.
      Description: Frees up all the memory associated with 'server_list'
                   (which should have been received from goad_server_list_get())
      Side effects: Invalidates the memory pointed to by 'server_list'.
 */
void
goad_server_list_free(GoadServer *server_list)
{
  int i;

  for(i = 0; server_list[i].repo_id; i++) {
    g_free(server_list[i].repo_id);
    g_free(server_list[i].id);
    g_free(server_list[i].description);
    g_free(server_list[i].location_info);
  }

  g_free(server_list);
}

/* Picks the first one on the list that meets criteria */
CORBA_Object
goad_server_activate_with_repo_id(GoadServer *server_list,
				  const char *repo_id,
				  GoadActivationFlags flags)
{
  g_assert(!"NYI");

  return CORBA_OBJECT_NIL;
}

/**** goad_server_activate
      Inputs: 'sinfo' - information on the server to be "activated"
              'flags' - information on how the application wants
	                the server to be activated.
      Description: Activates a CORBA server specified by 'sinfo', using
                   the 'flags' hints on how to activate that server.
 */
CORBA_Object
goad_server_activate(GoadServer *sinfo,
		     GoadActivationFlags flags)
{
  CORBA_Object retval = CORBA_OBJECT_NIL;
  CORBA_Environment ev;
  CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  CosNaming_Name nom = {0, 3, nc, CORBA_FALSE};

  g_return_val_if_fail(sinfo, CORBA_OBJECT_NIL);
  /* make sure they passed in a sane 'flags' */
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_SHLIB)
		   && (flags & GOAD_ACTIVATE_REMOTE)), CORBA_OBJECT_NIL);
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_EXISTING_ONLY)
		   && (flags & GOAD_ACTIVATE_NEW_ONLY)), CORBA_OBJECT_NIL);

  CORBA_exception_init(&ev);

  /* First, do name service lookup (if not specifically told not to) */
  if(!(flags & GOAD_ACTIVATE_NEW_ONLY)) {
    CORBA_Object name_service;

    name_service = gnome_name_service_get();
    g_assert(name_service != CORBA_OBJECT_NIL);

    nc[2].id = sinfo->id;
    nc[2].kind = "object";

    retval = CosNaming_NamingContext_resolve(name_service, &nom, &ev);
    CORBA_Object_release(name_service, &ev);

    if(ev._major != CORBA_NO_EXCEPTION) {
      retval = CORBA_OBJECT_NIL;
      goto out;
    }

    if(!CORBA_Object_is_nil(retval, &ev))
      goto out;
  }

  switch(sinfo->type) {
  case GOAD_SERVER_SHLIB:
    retval = goad_server_activate_shlib(sinfo, flags, &ev);
    break;
  case GOAD_SERVER_EXE:
    retval = goad_server_activate_exe(sinfo, flags, &ev);
    break;
  case GOAD_SERVER_RELAY:
    g_warning("Relay interface not yet defined (write an RFC :). Relay objects NYI");
    break;
  }

  /* This goes _before_ "out:" - don't want to reregister
     the object with the name service if we got it from there ;-) */
  if(!CORBA_Object_is_nil(retval, &ev)
     && !(flags & GOAD_ACTIVATE_NO_NS_REGISTER)) {
    /* Register this object with the name service */

    CORBA_Object name_service;

    name_service = gnome_name_service_get();

    nc[2].id = sinfo->id;
    nc[2].kind = "object";
    CosNaming_NamingContext_bind(name_service, &nom, retval, &ev);

    CORBA_Object_release(name_service, &ev);
  }

 out:
  CORBA_exception_free(&ev);

  return retval;
}

/**** goad_server_activate_shlib
      Inputs: 'sinfo' - information on the plugin to be loaded.
              'flags' - information about how the plugin should be loaded, etc.
	      'ev' - exception information (passed in to save us
	      creating another one)
      Pre-conditions: Assumes sinfo->type == GOAD_SERVER_SHLIB

      Side effects: May add information on the newly created server to
      'our_active_servers' list, so we can unregister the server from the
      name service when we exit.

      Description: Loads the plugin specified in 'sinfo'. Looks for
                   an object of id 'sinfo->id' in it, and activates it
		   if found.
 */
static CORBA_Object
goad_server_activate_shlib(GoadServer *sinfo,
			   GoadActivationFlags flags,
			   CORBA_Environment *ev)
{
  const GnomePlugin *plugin;
  int i;
  PortableServer_POA poa;
  CORBA_Object retval;
  ActiveServerInfo *local_server_info;
  gpointer impl_ptr;

  plugin = gnome_plugin_use(sinfo->location_info);
  g_return_val_if_fail(plugin, CORBA_OBJECT_NIL);

  for(i = 0; plugin->plugin_object_list[i].repo_id; i++) {
    if(!strcmp(sinfo->id, plugin->plugin_object_list[i].id)
       && !strcmp(sinfo->repo_id, plugin->plugin_object_list[i].repo_id))
       break;
  }
  g_return_val_if_fail(plugin->plugin_object_list[i].repo_id, CORBA_OBJECT_NIL);

  poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(gnome_orbit_orb,
								 "RootPOA", ev);

  retval = plugin->plugin_object_list[i].activate(poa, &impl_ptr, ev);
  g_return_val_if_fail(retval && ev->_major != CORBA_NO_EXCEPTION,
		       CORBA_OBJECT_NIL);

  if(!(flags & GOAD_ACTIVATE_NO_NS_REGISTER)) {
    local_server_info = g_new(ActiveServerInfo, 1);
    local_server_info->impl_ptr = impl_ptr;
    local_server_info->id = g_strdup(sinfo->id);

    if(!our_active_servers)
      g_atexit(goad_servers_unregister_atexit);

    our_active_servers = g_slist_prepend(our_active_servers, local_server_info);
  }

  return retval;
}


/**** goad_servers_unregister_atexit
      Description: For each shlib server that we had started,
                   try to unregister it from the name service.
 */
static void
goad_server_unregister_atexit(ActiveServerInfo *ai, CORBA_Environment *ev)
{
  CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  CosNaming_Name nom = {0, 3, nc, CORBA_FALSE};
  CORBA_Object name_service;
  PortableServer_ObjectId *oid;
  PortableServer_POA poa;

  CORBA_exception_free(ev); /* Clear previous exceptions */

  name_service = gnome_name_service_get();

  poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(gnome_orbit_orb,
								 "RootPOA", ev);
  oid = PortableServer_POA_servant_to_id(poa, ai->impl_ptr, ev);

  /* Check if the object is still active. If so, then deactivate it. */
  if(oid && !ev->_major) {
    nc[2].id = ai->id;
    nc[2].kind = "object";
    CosNaming_NamingContext_unbind(name_service, &nom, ev);

    CORBA_free(oid);
  }

  CORBA_Object_release(name_service, ev);
}
static void
goad_servers_unregister_atexit(void)
{
  CORBA_Environment ev;
  CORBA_exception_init(&ev);
  g_slist_foreach(our_active_servers, (GFunc)goad_server_unregister_atexit, &ev);
  CORBA_exception_free(&ev);
}

static CORBA_Object
goad_server_activate_exe(GoadServer *sinfo,
			 GoadActivationFlags flags,
			 CORBA_Environment *ev)
{
  g_assert(!"NYI");

  return CORBA_OBJECT_NIL;
}
