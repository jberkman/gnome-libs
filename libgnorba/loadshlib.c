#include <stdio.h>
#include <gnome.h>
#include "libgnorba/gnorba.h"

typedef struct {
	gpointer impl_ptr;
	char     *server_id;
} ActiveServerInfo;

static gchar*           shlib = 0;
static gchar*           id = NULL;
static gchar*           rid = NULL;
static ActiveServerInfo local_server_info;
static CORBA_ORB        orb;


static const struct poptOption options[] = {
  {"id", 'i', POPT_ARG_STRING, &id, 0, N_("ID under which GOAD knows this server"), N_("GOAD_ID")},
  {"rid", 'r', POPT_ARG_STRING, &rid, 0, N_("Repository ID under which GOAD knows this server"), N_("REPO_ID")},
  {NULL, '\0', 0, NULL, 0}
};

static
void Exception( CORBA_Environment* ev )
{
  switch( ev->_major )
    {
    case CORBA_SYSTEM_EXCEPTION:
      g_warning("CORBA system exception %s.\n", CORBA_exception_id(ev));
      exit ( 1 );
    case CORBA_USER_EXCEPTION:
      g_warning("CORBA user exception: %s.\n", CORBA_exception_id( ev ) );
      exit ( 1 );
    default:
      break;
    }
}


static void
goad_server_unregister_atexit()
{
  CORBA_Environment ev;
  CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  CosNaming_Name          nom = {0, 3, nc, CORBA_FALSE};

  CORBA_Object            name_service;
  PortableServer_ObjectId *oid;
  PortableServer_POA      poa;

  CORBA_exception_init(&ev);
  name_service = gnome_name_service_get();
  poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(orb, "RootPOA", &ev);

  
  oid = PortableServer_POA_servant_to_id(poa, local_server_info.impl_ptr, &ev);
  
  /* Check if the object is still active. If so, then deactivate it. */
  if(oid && !ev._major) {
    nc[2].id = local_server_info.server_id;
    nc[2].kind = "object";
    CosNaming_NamingContext_unbind(name_service, &nom, &ev);
    
    CORBA_free(oid);
  }
  
  CORBA_Object_release(name_service, &ev);
}



int
main(int argc, char* argv[])
{
  const GnomePlugin* plugin;
  PortableServer_POA root_poa;
  CORBA_Environment  ev;
  int                i;
  gpointer           impl_ptr;
  CORBA_Object       retval;
  PortableServer_POAManager pm;
  poptContext ctx;
  char **args;
  
  CORBA_exception_init(&ev);

  orb = gnome_CORBA_init_with_popt_table("loadshlib", VERSION, &argc, argv, options,
			 0, &ctx, GNORBA_INIT_SERVER_FUNC, &ev);
  if(!(id || rid)) {
    fprintf(stderr, "You must specify a GOAD ID or a Repository ID.\n\n");
    poptPrintHelp(ctx, stdout, 0);
    return 1;
  }

  args = poptGetArgs(ctx);

  g_return_val_if_fail(args, 1);
  plugin = gnome_plugin_use(args[0]);
  g_return_val_if_fail(plugin, 1);

  for(i = 0; plugin->plugin_object_list[i].repo_id; i++) {
    if(!strcmp(id, plugin->plugin_object_list[i].server_id)
       && !strcmp(rid, plugin->plugin_object_list[i].repo_id))
       break;
  }
  g_return_val_if_fail(plugin->plugin_object_list[i].repo_id, -1);
  root_poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(orb, "RootPOA", &ev);
  if (ev._major != CORBA_NO_EXCEPTION) {
    g_warning("goad_server_activate_shlib: activation function raises exception");
    switch( ev._major ) {
    case CORBA_SYSTEM_EXCEPTION:
      g_warning("sysex: %s.\n", CORBA_exception_id(&ev));
    case CORBA_USER_EXCEPTION:
      g_warning( "usrex: %s.\n", CORBA_exception_id( &ev ) );
    default:
      break;
    }
    return -1;
  }
  pm = PortableServer_POA__get_the_POAManager(root_poa, &ev);
  Exception(&ev);
  PortableServer_POAManager_activate(pm, &ev);
  Exception(&ev);

  retval = plugin->plugin_object_list[i].activate(root_poa, &impl_ptr, NULL, &ev);
  local_server_info.impl_ptr = impl_ptr;
  local_server_info.server_id = id;
  g_atexit(goad_server_unregister_atexit);

  fprintf(stdout, "%s\n", CORBA_ORB_object_to_string(orb, retval, &ev));

  gtk_main();

  return 0;
}
