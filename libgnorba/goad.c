/*
 * goad.c:
 *
 * Author:
 *   Elliot Lee (sopwith@cuc.edu)
 */
#include <config.h>
#include "gnorba.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>

#define SERVER_LISTING_PATH "/CORBA/Servers"

typedef struct {
	gpointer impl_ptr;
	char     *id;
} ActiveServerInfo;


typedef struct {
	gchar*   cmd;
	gchar**  argv;
} ServerCmd;


extern CORBA_ORB gnome_orbit_orb; /* In orbitgtk.c */

static GSList *our_active_servers = NULL;

static void goad_server_list_read(const char *filename,
				  GArray *servinfo,
				  GString *tmpstr);
static GoadServerType goad_server_typename_to_type(const char *typename);
static CORBA_Object goad_server_activate_shlib(GoadServer *sinfo,
					       GoadActivationFlags flags,
					       CORBA_Environment *ev);
static CORBA_Object goad_server_activate_exe(GoadServer *sinfo,
					     GoadActivationFlags flags,
					     CORBA_Environment *ev);
static void goad_servers_unregister_atexit(void);

/**** goad_server_list_get

      Outputs: 'retval' - newly created server list.

      Description: Returns an array listing all the servers available
                   for activation.
 */
GoadServer *
goad_server_list_get(void)
{
  GArray *servinfo;
  GoadServer *retval;
  GString *tmpstr;
  DIR *dirh;

  servinfo = g_array_new(TRUE, FALSE, sizeof(GoadServer));
  tmpstr = g_string_new(NULL);

  dirh = opendir(GNOMESYSCONFDIR "/CORBA/servers");
  if(dirh) {
    struct dirent *dent;

    while((dent = readdir(dirh))) {
	    if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
		    continue;

      g_string_sprintf(tmpstr, "=" GNOMESYSCONFDIR "/CORBA/servers/%s",
		       dent->d_name);
		       
      goad_server_list_read(tmpstr->str, servinfo, tmpstr);
    }
    closedir(dirh);
  }

  goad_server_list_read("/CORBA/servers/", servinfo, tmpstr);

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

/**** goad_server_list_read
      Inputs: 'filename' - file to read entries from.
              'servinfo' - array to append entries onto.
	      'tmpstr' - GString for scratchpad use.

      Side effects: Adds entries to 'servinfo'. Modifies 'tmpstr'.

      Notes: Called by goad_server_list_get() only.

      Description: Adds GoadServer entries from 'filename' onto the
      array 'servinfo'

 */
static void
goad_server_list_read(const char *filename,
		      GArray *servinfo,
		      GString *tmpstr)
{
  gpointer iter;
  char *typename;
  GoadServer newval;
  GString*   dummy;

  dummy = g_string_new("");
  
  gnome_config_push_prefix(filename);
  iter = gnome_config_init_iterator_sections(filename);

  while((iter = gnome_config_iterator_next(iter, &newval.server_id, NULL))) {
    if (*filename == '=')
      g_string_sprintf(dummy, "=%s/=type",
		       newval.server_id);
    else
      g_string_sprintf(dummy, "%s/type",
		       newval.server_id);
    typename = gnome_config_get_string(dummy->str);
    newval.type = goad_server_typename_to_type(typename);
    g_free(typename);
    if(!newval.type) {
      g_warning("Server %s has invalid activation method.", newval.server_id);
      g_free(newval.server_id);
      continue;
    }

    if (*filename == '=')
      g_string_sprintf(dummy, "=%s/=repo_id",
		       newval.server_id);
    else
      g_string_sprintf(dummy, "%s/repo_id",
		       newval.server_id);
    newval.repo_id = gnome_config_get_string(dummy->str);

    if (*filename == '=')
      g_string_sprintf(dummy, "=%s/=description",
		       newval.description);
    else
      g_string_sprintf(dummy, "%s/description",
		       newval.description);
    newval.description = gnome_config_get_string(dummy->str);

    if (*filename == '=')
      g_string_sprintf(dummy, "=%s/=location_info",
		       newval.server_id);
    else
      g_string_sprintf(dummy, "%s/location_info",
		       newval.description);
    newval.location_info = gnome_config_get_string(dummy->str);
    g_array_append_val(servinfo, newval);
  }
  gnome_config_pop_prefix();
}

/**** goad_server_list_free

      Inputs: 'server_list' - an array of GoadServer structures.

      Description: 'Frees up all the memory associated with
                   'server_list' (which should have been received from
                   goad_server_list_get())

      Side effects: Invalidates the memory pointed to by
      'server_list'.
*/
void
goad_server_list_free (GoadServer *server_list)
{
  int i;

  for(i = 0; server_list[i].repo_id; i++) {
    g_free(server_list[i].repo_id);
    g_free(server_list[i].server_id);
    g_free(server_list[i].description);
    g_free(server_list[i].location_info);
  }

  g_free(server_list);
}

/**** goad_server_list_activate_with_id
      Inputs: 'server_list' - a server listing
                              returned by goad_server_list_get.
			      If NULL, we will call the function ourself
			      and use that.
	      'id' - the goad ID of the server that we want to activate.
              'flags' - information on how the application wants
	                the server to be activated.
      Description: Activates a CORBA server specified by 'repo_id', using
                   the 'flags' hints on how to activate that server.
                   Picks the first one on the list that matches
 */
CORBA_Object
goad_server_activate_with_id(GoadServer *server_list,
			     const char *server_id,
			     GoadActivationFlags flags)
{
  GoadServer *slist;
  CORBA_Object retval = CORBA_OBJECT_NIL;
  int i;

  g_return_val_if_fail(server_id, CORBA_OBJECT_NIL);
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_EXISTING_ONLY)
		   && (flags & GOAD_ACTIVATE_NEW_ONLY)), CORBA_OBJECT_NIL);
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_SHLIB)
		   && (flags & GOAD_ACTIVATE_REMOTE)), CORBA_OBJECT_NIL);

  if(server_list)
    slist = server_list;
  else
    slist = goad_server_list_get();

  for(i = 0; slist[i].repo_id; i++) {
    if(!strcmp(slist[i].server_id, server_id))
      break;
  }

  if(slist[i].repo_id)
    retval = goad_server_activate(&slist[i], flags);

  if(!server_list)
    goad_server_list_free(slist);

  return retval;
}

/**** goad_server_activate_with_repo_id
      Inputs: 'server_list' - a server listing
                              returned by goad_server_list_get.
			      If NULL, we will call the function ourself
			      and use that.
	      'repo_id' - the repository ID of the interface that we want
	                  to activate a server for.
              'flags' - information on how the application wants
	                the server to be activated.
      Description: Activates a CORBA server specified by 'repo_id', using
                   the 'flags' hints on how to activate that server.
                   Picks the first one on the list that meets criteria.

		   This is done by possibly making three passes through the list,
		   the first pass checking for existing objects only,
		   the second pass taking into account any activation method
		   preferences, and the last pass just doing "best we can get"
		   service.
 */
CORBA_Object
goad_server_activate_with_repo_id(GoadServer *server_list,
				  const char *repo_id,
				  GoadActivationFlags flags)
{
  GoadServer *slist;
  CORBA_Object retval = CORBA_OBJECT_NIL;
  int i;
  enum { PASS_CHECK_EXISTING = 0, PASS_PREFER, PASS_FALLBACK, PASS_DONE } passnum;

  g_return_val_if_fail(repo_id, CORBA_OBJECT_NIL);
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_EXISTING_ONLY)
		   && (flags & GOAD_ACTIVATE_NEW_ONLY)), CORBA_OBJECT_NIL);
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_SHLIB)
		   && (flags & GOAD_ACTIVATE_REMOTE)), CORBA_OBJECT_NIL);

  if(server_list)
    slist = server_list;
  else
    slist = goad_server_list_get();
  
  /* (unvalidated assumption) If we need to only activate existing objects, then
     we don't want to bother checking activation methods, because
     we won't be activating anything. :)
                       OR
     if the app has not specified any activation method preferences,
     then we obviously don't need to bother with that pass through the list.
  */

  for(passnum = PASS_CHECK_EXISTING; passnum < PASS_DONE; passnum++) {
    
    for(i = 0; slist[i].repo_id; i++) {
      if(passnum == PASS_PREFER) {
	/* Check the type */
	if(((flags & GOAD_ACTIVATE_SHLIB)
	    && slist[i].type != GOAD_SERVER_SHLIB))
	  continue;
	    
	if((flags & GOAD_ACTIVATE_REMOTE)
	   && slist[i].type != GOAD_SERVER_EXE)
	  continue;
      }
	
      if(strcmp(repo_id, slist[i].repo_id))
	continue;
	
      /* entry matched */
      if(passnum == PASS_CHECK_EXISTING) {
	retval = goad_server_activate(&slist[i], flags | GOAD_ACTIVATE_EXISTING_ONLY);
      }
      else {
	retval = goad_server_activate(&slist[i], flags | GOAD_ACTIVATE_NEW_ONLY);
      }
      if (retval != CORBA_OBJECT_NIL)
	break;
    }
    /* If we got something, out of here.	
       If we were asked to check existing servers and we are done that,	
       out of here.	
       If we were not asked to do any special activation method checking,	
       then we've done all the needed passes, out of here.
    */
      
    if(retval != CORBA_OBJECT_NIL
       || ((passnum == PASS_CHECK_EXISTING)
	   && (flags & GOAD_ACTIVATE_EXISTING_ONLY))
       || ((passnum == PASS_PREFER)
	   && !(flags & (GOAD_ACTIVATE_SHLIB|GOAD_ACTIVATE_REMOTE))))
      break;
  }

  if(!server_list)
    goad_server_list_free(slist);

  return retval;
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
  CORBA_Environment       ev;
  CORBA_Object            name_service;
  CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  CosNaming_Name          nom = {0, 3, nc, CORBA_FALSE};

  CORBA_Object retval = CORBA_OBJECT_NIL;

  g_return_val_if_fail(sinfo, CORBA_OBJECT_NIL);
  /* make sure they passed in a sane 'flags' */
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_SHLIB)
		   && (flags & GOAD_ACTIVATE_REMOTE)), CORBA_OBJECT_NIL);
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_EXISTING_ONLY)
		   && (flags & GOAD_ACTIVATE_NEW_ONLY)), CORBA_OBJECT_NIL);

  CORBA_exception_init(&ev);

  /* First, do name service lookup (if not specifically told not to) */
  if(!(flags & GOAD_ACTIVATE_NEW_ONLY)) {

    name_service = gnome_name_service_get();
    g_assert(name_service != CORBA_OBJECT_NIL);

    nc[2].id = sinfo->server_id;
    nc[2].kind = "object";
    retval = CosNaming_NamingContext_resolve(name_service, &nom, &ev);
    if (ev._major == CORBA_USER_EXCEPTION
	&& strcmp(CORBA_exception_id(&ev), ex_CosNaming_NamingContext_NotFound) == 0) {
      retval = CORBA_OBJECT_NIL;
    }
    else if (ev._major != CORBA_NO_EXCEPTION) {
      g_warning("goad_server_activate: %s %d: unexpected exception:", __FILE__, __LINE__);
      switch( ev._major ) {
	case CORBA_SYSTEM_EXCEPTION:
	  g_warning("sysex: %s.\n", CORBA_exception_id(&ev));
	case CORBA_USER_EXCEPTION:
	  g_warning( "usrex: %s.\n", CORBA_exception_id( &ev ) );
	default:
	  break;
	}
    }
    ev._major = CORBA_NO_EXCEPTION;
    CORBA_Object_release(name_service, &ev);
    if (ev._major != CORBA_NO_EXCEPTION) {
	retval = CORBA_OBJECT_NIL;
      }
    
    if(!CORBA_Object_is_nil(retval, &ev) || (flags & GOAD_ACTIVATE_EXISTING_ONLY))
      goto out;
  }
  
  switch(sinfo->type) {
  case GOAD_SERVER_SHLIB:
    if (flags & GOAD_ACTIVATE_REMOTE) {
      GoadServer fake_sinfo;
      gchar cmdline[1024];
      
      fake_sinfo.type = GOAD_SERVER_EXE;
      fake_sinfo.repo_id     = "loadshlib";
      fake_sinfo.server_id          = "loadshlib";
      fake_sinfo.description = "DLL Loader for GOAD";
      g_snprintf(cmdline, sizeof(cmdline), "loadshlib -i %s -r %s %s",
		 sinfo->server_id, sinfo->repo_id, sinfo->location_info);
      fake_sinfo.location_info = cmdline;
      retval = goad_server_activate_exe(&fake_sinfo, flags, &ev);
    } else {
      retval = goad_server_activate_shlib(sinfo, flags, &ev);
    }
    break;
  case GOAD_SERVER_EXE:
    retval = goad_server_activate_exe(sinfo, flags, &ev);
    break;
  case GOAD_SERVER_RELAY:
    g_warning("Relay interface not yet defined (write an RFC :). Relay objects NYI");
    break;
  }
#if 0 
  /*
   * This goes _before_ "out:" - don't want to reregister
   * the object with the name service if we got it from there ;-)
   */

  if(!CORBA_Object_is_nil(retval, &ev) && !(flags & GOAD_ACTIVATE_NO_NS_REGISTER)) {
    
    name_service = gnome_name_service_get();
    nc[2].id = sinfo->server_id;
    nc[2].kind = "object";
    CosNaming_NamingContext_bind(name_service, &nom, retval, &ev);
    if (ev._major != CORBA_NO_EXCEPTION)
      {
	g_warning("goad_server_activate: Exception during server registration");
	switch( ev._major )
	  {
	  case CORBA_SYSTEM_EXCEPTION:
	    g_warning("sysex: %s.\n", CORBA_exception_id(&ev));
	  case CORBA_USER_EXCEPTION:
	    g_warning( "usr	ex: %s.\n", CORBA_exception_id(&ev));
	  default:
	    break;
	  }
      }
  }
#endif    
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
                    'our_active_servers' list, so we can unregister
                    the server from the name service when we exit.

      Description: Loads the plugin specified in 'sinfo'. Looks for an
                   object of id 'sinfo->server_id' in it, and activates it if
                   found.
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
    if(!strcmp(sinfo->server_id, plugin->plugin_object_list[i].server_id)
       && !strcmp(sinfo->repo_id, plugin->plugin_object_list[i].repo_id))
       break;
  }
  g_return_val_if_fail(plugin->plugin_object_list[i].repo_id, CORBA_OBJECT_NIL);

  poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(gnome_orbit_orb,
								 "RootPOA", ev);

  retval = plugin->plugin_object_list[i].activate(poa, &impl_ptr, ev);
  if (ev->_major != CORBA_NO_EXCEPTION) {
    g_warning("goad_server_activate_shlib: activation function raises exception");
    switch( ev->_major ) {
    case CORBA_SYSTEM_EXCEPTION:
      g_warning("sysex: %s.\n", CORBA_exception_id(ev));
    case CORBA_USER_EXCEPTION:
      g_warning( "usrex: %s.\n", CORBA_exception_id( ev ) );
    default:
      break;
    }
    return CORBA_OBJECT_NIL;
  }

  local_server_info = g_new(ActiveServerInfo, 1);
  local_server_info->impl_ptr = impl_ptr;
  local_server_info->id = g_strdup(sinfo->server_id);
  
  if(!our_active_servers)
    g_atexit(goad_servers_unregister_atexit);
  
  our_active_servers = g_slist_prepend(our_active_servers, local_server_info);
  return retval;
}


/**** goad_servers_unregister_atexit

      Description: For each shlib server that we had started, try to
                   unregister it from the name service.

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


static ServerCmd*
get_cmd(gchar* line)
{
  static ServerCmd retval;
  gchar* ptr;
  gint   argc = 1;
  gchar* tok;
  
  retval.argv = (gchar**)malloc(sizeof(char*) * 2);
  retval.argv[1] = 0;

  ptr = line;

  while (ptr && isspace(*ptr))
    ptr++;

  if (!*ptr)
    return &retval;

  retval.cmd = strtok(ptr, " \t");
  retval.argv[0] = retval.cmd;
  while (1)
    {
      tok = strtok(0, " \t");
      if (!tok)
	return &retval;
      argc++;
      retval.argv = (char**)realloc(retval.argv, argc+1 * sizeof(char*));
      retval.argv[argc-1] = tok;
      retval.argv[argc] = 0;
    }
}



/**** goad_server_activate_exe
      Description: 
      Inputs: 'sinfo' - information on the program to be run.
              'flags' - information about how the program should be run, etc.
	                (no flags applicable at the present time)
	      'ev' - exception information (passed in to save us
	      creating another one)
      Outputs: 'retval' - an objref to the newly-started server.
      Pre-conditions: Assumes sinfo->type == GOAD_SERVER_EXE

      Description: Calls setsid to daemonize the server. Expects the
		   server to register itself with the nameing
		   service. Returns after the server printed it's IOR
		   string.  Ignores SIGPIPE in the child, so that the
		   server doesn't die if it writes to a closed fd.  */

CORBA_Object
goad_server_activate_exe(GoadServer *sinfo,
			 GoadActivationFlags flags,
			 CORBA_Environment *ev)
{
  gint                    iopipes[2];
  gchar                   iorbuf[2048];
  CORBA_Object            retval = CORBA_OBJECT_NIL;

  pipe(iopipes);
  /* fork & get the ior from stdout */

  if(fork()) {
    int     status;
    FILE*   iorfh;
    
    close(iopipes[1]);
    iorfh = fdopen(iopipes[0], "r");

    while (fgets(iorbuf, sizeof(iorbuf), iorfh) && strncmp(iorbuf, "IOR:", 4))
#if 0
      g_message("srv output: '%s'", iorbuf)
#endif
	;

    if (strncmp(iorbuf, "IOR:", 4)) {
      retval = CORBA_OBJECT_NIL;
      goto out;
    }
    if (iorbuf[strlen(iorbuf)-1] == '\n')
      iorbuf[strlen(iorbuf)-1] = '\0';
    retval = CORBA_ORB_string_to_object(gnome_orbit_orb, iorbuf, ev);
#if 0
    if (ev->_major != CORBA_NO_EXCEPTION) {
      g_warning("goad_server_activate_exe: %s %d:", __FILE__, __LINE__);
      switch( ev->_major ) {
      case CORBA_SYSTEM_EXCEPTION:
	g_warning("syse	x: %s.\n", CORBA_exception_id(ev));
      case CORBA_USER_EXCEPTION:
	g_warning("usrex: %s.\n", CORBA_exception_id( ev ) );
      default:
	break;
      }
      retval = CORBA_OBJECT_NIL;
    }
#endif
    fclose(iorfh);
    wait(&status);
  } else if(fork()) {
    _exit(0); /* de-zombifier process, just exit */
  } else {
    char **args, **real_args;
    int i;
    struct sigaction sa;

    close(0);
    close(iopipes[0]);
    dup2(iopipes[1], 1);
    dup2(iopipes[1], 2);
    
    setsid();
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);
    args = g_strsplit(sinfo->location_info, " ", -1);
    for(i = 0; args[i]; i++) /**/ ;

    args = g_realloc(args, sizeof(char *) * (i+3));
    args[i] = "--activate-goad-server";
    args[i+1] = sinfo->server_id;
    args[i+2] = NULL;

    execvp(args[0], args);
    _exit(1);
  }
out:
  return retval;
}

int
goad_server_register(CORBA_Object name_server,
		     CORBA_Object server,
		     const char* name,
		     const char* kind,
		     CORBA_Environment* ev)
{
  CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  CosNaming_Name          nom = {0, 3, nc, CORBA_FALSE};
  CORBA_Object            old_server;

  nc[2].id   = (char *)name;
  nc[2].kind = (char *)kind;

  CORBA_exception_free(ev);

  old_server = CosNaming_NamingContext_resolve(name_server, &nom, ev);
#if 0
  if (ev->_major != CORBA_NO_EXCEPTION) {
    switch( ev->_major ) {
    case CORBA_SYSTEM_EXCEPTION:
      g_warning("sysex: %s.\n", CORBA_exception_id(ev));
    case CORBA_USER_EXCEPTION:
      g_warning( "usrex: %s.\n", CORBA_exception_id(ev));
    default:
      break;
    }
  }
#endif

  if (ev->_major == CORBA_USER_EXCEPTION &&
      !strcmp(CORBA_exception_id(ev),ex_CosNaming_NamingContext_NotFound)) {
    CosNaming_NamingContext_bind(name_server, &nom, server, ev);
    if (ev->_major != CORBA_NO_EXCEPTION) {
      switch( ev->_major ) {
      case CORBA_SYSTEM_EXCEPTION:
	g_warning("sysex: %s.\n", CORBA_exception_id(ev));
      case CORBA_USER_EXCEPTION:
	g_warning( "usrex: %s.\n", CORBA_exception_id(ev));
      default:
	break;
      }
    }
  }
  else {
    CORBA_Object_release(old_server, ev);
    return -2;
  }
  if (ev->_major != CORBA_NO_EXCEPTION) {
#if 0
    switch( ev->_major ) {
    case CORBA_SYSTEM_EXCEPTION:
      g_warning("sysex: %s.\n", CORBA_exception_id(ev));
    case CORBA_USER_EXCEPTION:
      g_warning( "usrex: %s.\n", CORBA_exception_id(ev));
    default:
      break;
    }
#endif
    return -1;
  }
  return 0;
}

int
goad_server_unregister(CORBA_Object name_server,
		       const char* name,
		       const char* kind,
		       CORBA_Environment* ev)
{
  CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  CosNaming_Name          nom = {0, 3, nc, CORBA_FALSE};

  nc[2].id   = (char *)name;
  nc[2].kind = (char *)kind;
  CosNaming_NamingContext_unbind(name_server, &nom, ev);
  if (ev->_major != CORBA_NO_EXCEPTION) {
#if 0
    switch( ev->_major ) {
    case CORBA_SYSTEM_EXCEPTION:
      g_warning("sysex: %s.\n", CORBA_exception_id(ev));
    case CORBA_USER_EXCEPTION:
      g_warning( "usr	ex: %s.\n", CORBA_exception_id(ev));
    default:
      break;
    }
#endif
    return -1;
  }
  return 0;
}

const char *goad_activation_id = NULL;

const char *
goad_server_activation_id(void)
{
  return goad_activation_id;
}

void goad_register_arguments(void); /* shut up gcc */

void
goad_register_arguments(void)
{
  static const struct poptOption options[] = {
    {"activate-goad-server", '\0', POPT_ARG_STRING, &goad_activation_id, 0,
    N_("(Internal use only) GOAD server ID to activate"), "GOAD_ID"},
    {NULL, '\0', 0, NULL, 0}
  };

  gnomelib_register_popt_table(options, "Gnome Object Activation Directory");
}
