/*
 * goad.c:
 *
 * Author:
 *   Elliot Lee (sopwith@cuc.edu)
 */
#include <config.h>
#include <string.h>
#include <sys/types.h>

#include "gnorba.h"
#include <gmodule.h>
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <signal.h>

#define GOAD_MAGIC_FD 123

#define SHLIB_DEPENDENCIES 1
#ifdef SHLIB_DEPENDENCIES
#include <dlfcn.h>
#endif
#include <ctype.h>

#define SERVER_LISTING_PATH "/CORBA/servers"

typedef struct {
	gpointer impl_ptr;
	char     *id;
} ActiveServerInfo;


typedef struct {
	gchar*   cmd;
	gchar**  argv;
} ServerCmd;


extern CORBA_ORB _gnorba_gnome_orbit_orb; /* In orbitgtk.c */

static GSList *our_active_servers = NULL;
static const char *goad_activation_id = NULL;

static void goad_server_list_read(const char *filename,
				  GArray *servinfo,
				  GString *tmpstr,
				  GoadServerList *newl);
static GoadServerType goad_server_typename_to_type(const char *typename);
static CORBA_Object real_goad_server_activate(GoadServer *sinfo,
					      GoadActivationFlags flags,
					      const char **params,
					      GoadServerList *server_list);
static CORBA_Object goad_server_activate_shlib(GoadServer *sinfo,
					       GoadActivationFlags flags,
					       const char **params,
					       CORBA_Environment *ev);
static CORBA_Object goad_server_activate_exe(GoadServer *sinfo,
					     GoadActivationFlags flags,
					     const char **params,
					     CORBA_Environment *ev);
static CORBA_Object goad_server_activate_factory(GoadServer *sinfo,
						 GoadActivationFlags flags,
						 const char **params,
						 CORBA_Environment *ev,
						 GoadServerList *slist);
static void goad_servers_unregister_atexit(void);
void goad_register_arguments(void);

static int string_array_len(const char **array)
{
  int i;

  if(!array) return 0;

  for(i = 0; array[i]; i++) /* */ ;

  return i;
}

static gboolean string_in_array(const char *string, const char **array)
{
  int i;
  for(i = 0; array[i]; i++) {
    if(!strcmp(string, array[i]))
      return TRUE;
  }

  return FALSE;
}

/**
 * goad_server_list_get:
 *
 * Returns an array listing all the servers available
 * for activation.
 *
 * Returns a newly created server list.
 */
GoadServerList *
goad_server_list_get(void)
{
  GArray *servinfo;
  GoadServer *retval;
  GString *tmpstr;
  GString *usersrvpath;
  DIR *dirh;
  GoadServerList *newl;
  GHashTable *by_goad_id;
  int i;

  newl = g_new0(GoadServerList, 1);
  servinfo = g_array_new(TRUE, FALSE, sizeof(GoadServer));
  by_goad_id = g_hash_table_new(g_str_hash, g_str_equal);
  newl->by_goad_id = by_goad_id;
  tmpstr = g_string_new(NULL);

  /* User servers (preferred over system) */
  usersrvpath = g_string_new(NULL);
  g_string_sprintf(usersrvpath, "%s" SERVER_LISTING_PATH, gnome_user_dir);
  dirh = opendir(usersrvpath->str);
  if(dirh) {
    struct dirent *dent;
    
    while((dent = readdir(dirh))) {
	    if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
		    continue;
	    
	    g_string_sprintf(tmpstr, "=%s/%s", usersrvpath->str, dent->d_name);
	    
	    goad_server_list_read(tmpstr->str, servinfo, tmpstr, newl);
    }
    closedir(dirh);
  }
  g_string_free(usersrvpath, TRUE);
  
  /* System servers */
  dirh = opendir(GNOMESYSCONFDIR SERVER_LISTING_PATH);
  if(dirh) {
    struct dirent *dent;

    while((dent = readdir(dirh))) {
	    if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
		    continue;

      g_string_sprintf(tmpstr, "=" GNOMESYSCONFDIR SERVER_LISTING_PATH "/%s",
		       dent->d_name);
		       
      goad_server_list_read(tmpstr->str, servinfo, tmpstr, newl);
    }
    closedir(dirh);
  }

  goad_server_list_read(SERVER_LISTING_PATH "/", servinfo, tmpstr, newl);

  newl->list = (GoadServer *)servinfo->data;
  for(i = 0; newl->list[i].repo_id; i++)
    g_hash_table_insert(newl->by_goad_id,
			newl->list[i].server_id, &newl->list[i]);

  g_array_free(servinfo, FALSE);
  g_string_free(tmpstr, TRUE);

  return newl;
}

static GoadActivationFlags
goad_activation_combine_flags(GoadServer *sinfo,
			      GoadActivationFlags user_flags)
{
  GoadActivationFlags retval;

  retval = (user_flags & ~(GOAD_ACTIVATE_REMOTE|GOAD_ACTIVATE_SHLIB|GOAD_ACTIVATE_NEW_ONLY|GOAD_ACTIVATE_EXISTING_ONLY));
  
  if(sinfo->flags & (GOAD_ACTIVATE_REMOTE|GOAD_ACTIVATE_SHLIB))
    retval |= sinfo->flags & (GOAD_ACTIVATE_REMOTE|GOAD_ACTIVATE_SHLIB);
  else
    retval |= user_flags & (GOAD_ACTIVATE_REMOTE|GOAD_ACTIVATE_SHLIB);

  if(sinfo->flags & (GOAD_ACTIVATE_NEW_ONLY|GOAD_ACTIVATE_EXISTING_ONLY))
    retval |=
      sinfo->flags & (GOAD_ACTIVATE_NEW_ONLY|GOAD_ACTIVATE_EXISTING_ONLY);
  else
    retval |= user_flags & (GOAD_ACTIVATE_NEW_ONLY|GOAD_ACTIVATE_EXISTING_ONLY);

  return retval;
}
  
static GoadActivationFlags
goad_server_flagstring_to_flags(char *flstr)
{
  char **ctmp;
  GoadActivationFlags retval = 0;
  int i;

  if(!flstr) return retval;

  g_strstrip(flstr);

  if(!*flstr) return retval;

  ctmp = g_strsplit(flstr, "|", 0);

  for(i = 0; ctmp[i]; i++) {
    if(!strcasecmp(ctmp[i], "new_only")) {
      if(retval & GOAD_ACTIVATE_EXISTING_ONLY)
	g_warning("Can't combine existing_only and new_only activation flags");
      else
	retval |= GOAD_ACTIVATE_NEW_ONLY;
    } else if(!strcasecmp(ctmp[i], "existing_only")) {
      if(retval & GOAD_ACTIVATE_NEW_ONLY)
	g_warning("Can't combine existing_only and new_only activation flags");
      else
	retval |= GOAD_ACTIVATE_EXISTING_ONLY;
    } else if(!strcasecmp(ctmp[i], "shlib")) {
      if(retval & GOAD_ACTIVATE_REMOTE)
	g_warning("Can't combine shlib and remote activation flags");
      else
	retval |= GOAD_ACTIVATE_SHLIB;
    } else if(!strcasecmp(ctmp[i], "remote")) {
      if(retval & GOAD_ACTIVATE_SHLIB)
	g_warning("Can't combine shlib and remote activation flags");
      else
	retval |= GOAD_ACTIVATE_REMOTE;
    } else
      g_warning("Unknown activation flag %s", ctmp[i]);
  }

  g_strfreev(ctmp);

  return retval;
}

static GoadServerType
goad_server_typename_to_type(const char *typename)
{
  if(!strcasecmp(typename, "shlib"))
    return GOAD_SERVER_SHLIB;
  else if(!strcmp(typename, "exe"))
    return GOAD_SERVER_EXE;
  else if(!strcasecmp(typename, "relay"))
    return GOAD_SERVER_RELAY;
  else if(!strcasecmp(typename, "factory"))
    return GOAD_SERVER_FACTORY;
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
		      GString *tmpstr,
		      GoadServerList *newl)
{
  gpointer iter;
  char *typename;
  GoadServer newval, *nvptr;
  GString*   dummy;

  dummy = g_string_new("");
  
  gnome_config_push_prefix(filename);
  iter = gnome_config_init_iterator_sections(filename);

  while((iter = gnome_config_iterator_next(iter, &newval.server_id, NULL))) {
    int vlen;

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
      g_string_sprintf(dummy, "=%s/=flags",
		       newval.server_id);
    else
      g_string_sprintf(dummy, "%s/flags",
		       newval.server_id);
    typename = gnome_config_get_string(dummy->str);
    newval.flags = goad_server_flagstring_to_flags(typename);
    g_free(typename);

    if (*filename == '=')
      g_string_sprintf(dummy, "=%s/=repo_id",
		       newval.server_id);
    else
      g_string_sprintf(dummy, "%s/repo_id",
		       newval.server_id);
    gnome_config_get_vector(dummy->str, &vlen, &newval.repo_id);
    newval.repo_id = g_realloc(newval.repo_id, (vlen + 1)*sizeof(char *));
    newval.repo_id[vlen] = NULL;

    if (*filename == '=')
      g_string_sprintf(dummy, "=%s/=description",
		       newval.server_id);
    else
      g_string_sprintf(dummy, "%s/description",
		       newval.server_id);
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
  /*forget the config information about this file, otherwise we
    take up gobs of memory*/
  if (*filename == '=')
	  g_string_sprintf(dummy, "%s=", filename);
  else
	  g_string_assign(dummy,filename);
  gnome_config_drop_file(dummy->str);
  g_string_free(dummy,TRUE);
}

/**
 * goad_server_list_free:
 * @server_list: an array of GoadServer structures.
 *
 * Frees up all the memory associated with @server_list
 * (which should have been received from goad_server_list_get ())
 *
 * Side effects: Invalidates the memory pointed to by
 * 'server_list'.
 */
void
goad_server_list_free (GoadServerList *server_list)
{
  int i;
  GoadServer *sl;

  sl = server_list->list;

  if(sl) {
    for(i = 0; sl[i].repo_id; i++) {
      g_strfreev(sl[i].repo_id);
      g_free(sl[i].server_id);
      g_free(sl[i].description);
      g_free(sl[i].location_info);
    }

    g_free(sl);
  }

  g_hash_table_destroy(server_list->by_goad_id);

  g_free(server_list);
}

/**
 * goad_server_list_activate_with_id:
 * @server_list: a server listing returned by goad_server_list_get.
 * If NULL, we will call the function ourself and use that.
 * @id: the goad ID of the server that we want to activate.
 * @flags: information on how the application wants the server to be activated.
 *
 * Activates a CORBA server specified by 'repo_id', using
 * the 'flags' hints on how to activate that server.
 * Picks the first one on the list that matches.
 */
CORBA_Object
goad_server_activate_with_id (GoadServerList *server_list,
			      const char *server_id,
			      GoadActivationFlags flags,
			      const char **params)
{
  GoadServerList *my_servlist;
  GoadServer *slist;
  CORBA_Object retval = CORBA_OBJECT_NIL;

  g_return_val_if_fail(server_id, CORBA_OBJECT_NIL);
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_EXISTING_ONLY)
		   && (flags & GOAD_ACTIVATE_NEW_ONLY)), CORBA_OBJECT_NIL);
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_SHLIB)
		   && (flags & GOAD_ACTIVATE_REMOTE)), CORBA_OBJECT_NIL);

  /* XXX TODO: try getting a running server from the name service first */
  if(server_list)
    my_servlist = server_list;
  else
    my_servlist = goad_server_list_get();

  g_return_val_if_fail(my_servlist, CORBA_OBJECT_NIL);

  slist = my_servlist->list;

  if(!slist)
    goto errout;

  slist = g_hash_table_lookup(my_servlist->by_goad_id, server_id);

  if(slist)
    retval = real_goad_server_activate(slist, flags, params, my_servlist);

 errout:
  if(!server_list)
    goad_server_list_free(my_servlist);

  return retval;
}

/**
 * goad_server_activate_with_repo_id:
 * @server_list: a server listing returned by goad_server_list_get.
 *               If NULL, we will call the function ourself and use that.
 * @repo_id: the repository ID of the interface that we want to activate a server for.
 * @flags: information on how the application wants the server to be activated.
 *
 * Activates a CORBA server specified by 'repo_id', using the 'flags'
 * hints on how to activate that server.  Picks the first one on the
 * list that meets criteria.
 *
 * This is done by possibly making three passes through the list, the
 * first pass checking for existing objects only, the second pass
 * taking into account any activation method preferences, and the last
 * pass just doing "best we can get" service.
 */
CORBA_Object
goad_server_activate_with_repo_id(GoadServerList *server_list,
				  const char *repo_id,
				  GoadActivationFlags flags,
				  const char **params)
{
  GoadServerList *my_slist;
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
    my_slist = server_list;
  else
    my_slist = goad_server_list_get();

  g_return_val_if_fail(my_slist, retval);
  slist = my_slist->list;

  if(!slist) goto errout;
  
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

      if(!string_in_array(repo_id, (const char **)slist[i].repo_id))
	continue;
	
      /* entry matched */
      if(passnum == PASS_CHECK_EXISTING) {
	retval = real_goad_server_activate(&slist[i], flags | GOAD_ACTIVATE_EXISTING_ONLY,
				      params, my_slist);
      }
      else {
	retval = real_goad_server_activate(&slist[i], flags | GOAD_ACTIVATE_NEW_ONLY,
				      params, my_slist);
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

 errout:
  if(!server_list)
    goad_server_list_free(my_slist);

  return retval;
}

/**
 * goad_server_activate:
 * @sinfo: information on the server to be "activated"
 * @flags: information on how the application wants the server to be activated.
 *
 * Activates a CORBA server specified by 'sinfo', using the 'flags'
 * hints on how to activate that server.
 *
 * Returns a CORBA_Object that points to this server, or CORBA_OBJECT_NIL
 * if the activation failed.
 */
CORBA_Object
goad_server_activate(GoadServer *sinfo,
		     GoadActivationFlags flags,
		     const char **params)
{
  return real_goad_server_activate(sinfo, flags, params, NULL);
}

/* Allows using an already-read server list */
static CORBA_Object
real_goad_server_activate(GoadServer *sinfo,
			  GoadActivationFlags flags,
			  const char **params,
			  GoadServerList *server_list)
{
  CORBA_Environment       ev;
  CORBA_Object            name_service;
  CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  CosNaming_Name          nom;

  CORBA_Object retval = CORBA_OBJECT_NIL;

  nom._maximum = 0;
  nom._length = 3;
  nom._buffer = nc;
  nom._release = CORBA_FALSE;

  g_return_val_if_fail(sinfo, CORBA_OBJECT_NIL);

  /* make sure they passed in a sane 'flags' */
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_SHLIB)
		   && (flags & GOAD_ACTIVATE_REMOTE)), CORBA_OBJECT_NIL);
  g_return_val_if_fail(!((flags & GOAD_ACTIVATE_EXISTING_ONLY)
		   && (flags & GOAD_ACTIVATE_NEW_ONLY)), CORBA_OBJECT_NIL);

  flags = goad_activation_combine_flags(sinfo, flags);

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
      g_warning("goad_server_activate: %s %d: unexpected exception %s:", __FILE__, __LINE__, ev._repo_id);
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

      fake_sinfo = *sinfo;
      fake_sinfo.type = GOAD_SERVER_EXE;

      g_snprintf(cmdline, sizeof(cmdline), "loadshlib -i %s -r %s %s",
		 sinfo->server_id, sinfo->repo_id[0], sinfo->location_info);
      fake_sinfo.location_info = cmdline;
      retval = goad_server_activate_exe(&fake_sinfo, flags, params, &ev);
    } else {
      retval = goad_server_activate_shlib(sinfo, flags, params, &ev);
    }
    break;
  case GOAD_SERVER_EXE:
    retval = goad_server_activate_exe(sinfo, flags, params, &ev);
    break;
  case GOAD_SERVER_RELAY:
    g_warning("Relay interface not yet defined (write an RFC :). Relay objects NYI");
    break;
  case GOAD_SERVER_FACTORY:
    retval = goad_server_activate_factory(sinfo, flags, params, &ev,
					  server_list);
  }
 out:
  CORBA_exception_free(&ev);
  return retval;
}

/**
 * goad_server_activate_shlib:
 * @sinfo: information on the plugin to be loaded.
 * @flags: information about how the plugin should be loaded, etc.
 * @ev: exception information (passed in to save us creating another one)
 *
 * Pre-conditions: Assumes sinfo->type == GOAD_SERVER_SHLIB
 *
 * Side effects: May add information on the newly created server to
 * 'our_active_servers' list, so we can unregister the server from the
 * name service when we exit.
 *
 * Loads the plugin specified in 'sinfo'. Looks for an
 * object of id 'sinfo->server_id' in it, and activates it if
 * found.
 */
static CORBA_Object
goad_server_activate_shlib(GoadServer *sinfo,
			   GoadActivationFlags flags,
			   const char **params,
			   CORBA_Environment *ev)
{
  const GnomePlugin *plugin;
  int i;
  PortableServer_POA poa;
  CORBA_Object retval;
  ActiveServerInfo *local_server_info;
  gpointer impl_ptr;
  GModule *gmod;

#define SHLIB_DEPENDENCIES 1
#ifdef SHLIB_DEPENDENCIES
  FILE* lafile;
  gchar* ptr;
  gchar line[128];
  gint len = strlen(sinfo->location_info);
  void* handle;
  
  if (!strcmp(sinfo->location_info + (len - 2), "la")) {
    /* find inter-library depencies in the .la file */
    lafile = fopen(sinfo->location_info, "r");
    if (!lafile)
      goto normal_loading;	/* bail out and let the normal loading
				   code handle the error */
    while ((ptr = fgets(line, sizeof(line), lafile))) {
      if (!strncmp(line, "dependency_libs='", strlen("dependency_libs='")))
	break;
    }
    fclose(lafile);
    if (!ptr) {
      /* no dependcy line, just load the lib */
      goto normal_loading;
    }
    ptr = line + strlen("dependency_libs='") + 1;
    /* ptr now is on the '-' of the first lib */ 
    while (1) {
      gchar*  libpath;
      gchar*  libstart;
      gchar   libname[128];
      ptr += 2;
      libstart = ptr;
      while (*ptr != '\'' && !isspace(*ptr))
	ptr++;
      if (*ptr == '\'')
	break;
      memset(libname, 0, sizeof(libname));
      strcpy(libname, "lib");
      strncat(libname, libstart, ptr - libstart);
      strcat(libname, ".so");
      fprintf(stderr,"Using '%s' for dlopen\n", libname);
#ifdef RTLD_GLOBAL
      handle = dlopen(libname, RTLD_GLOBAL | RTLD_LAZY);
#else
      handle = dlopen(libname, RTLD_LAZY);
#endif /*RTLD_GLOBAL*/

      if (!handle) {
	g_warning("Cannot load %s: %s", libname, dlerror());
      }
      ptr += 1;
    }
normal_loading:
    sinfo->location_info[len-1] = 'o';
    sinfo->location_info[len-2] = 's';
  }
#endif
  gmod = g_module_open(sinfo->location_info, G_MODULE_BIND_LAZY);
  if(!gmod && *sinfo->location_info != '/') {
    char *ctmp;
    ctmp = gnome_libdir_file(sinfo->location_info);
    gmod = g_module_open(ctmp, G_MODULE_BIND_LAZY);
    g_free(ctmp);
  }

  g_return_val_if_fail(gmod, CORBA_OBJECT_NIL);
  g_module_make_resident(gmod);
  g_return_val_if_fail(g_module_symbol(gmod, "GNOME_Plugin_info",
				       (gpointer *)&plugin),
		       CORBA_OBJECT_NIL);

  for(i = 0; plugin->plugin_object_list[i].repo_id; i++) {
    if(!strcmp(sinfo->server_id, plugin->plugin_object_list[i].server_id))
       break;
  }
  g_return_val_if_fail(plugin->plugin_object_list[i].repo_id, CORBA_OBJECT_NIL);

  poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(_gnorba_gnome_orbit_orb,
								 "RootPOA", ev);

  retval = plugin->plugin_object_list[i].activate(poa,
						  plugin->plugin_object_list[i].server_id, NULL, &impl_ptr, ev);

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

#if 0  
  local_server_info = g_new(ActiveServerInfo, 1);
  local_server_info->impl_ptr = impl_ptr;
  local_server_info->id = g_strdup(sinfo->server_id);

  if(!our_active_servers)
    g_atexit(goad_servers_unregister_atexit);
  
  our_active_servers = g_slist_prepend(our_active_servers, local_server_info);
#endif
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
  CosNaming_Name nom;

  CORBA_Object name_service;
  PortableServer_ObjectId *oid;
  PortableServer_POA poa;

  nom._maximum = 0;
  nom._length = 3;
  nom._buffer = nc;
  nom._release = CORBA_FALSE;

  CORBA_exception_free(ev); /* Clear previous exceptions */

  name_service = gnome_name_service_get();

  poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(_gnorba_gnome_orbit_orb,
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


/* Talks to the factory and asks it for info */
static CORBA_Object
goad_server_activate_factory(GoadServer *sinfo,
			     GoadActivationFlags flags,
			     const char **params,
			     CORBA_Environment *ev,
			     GoadServerList *slist)
{
  CORBA_Object factory_obj, retval;
  GNOME_stringlist sl;

  factory_obj = goad_server_activate_with_id(slist, sinfo->location_info,
					     flags & ~(GOAD_ACTIVATE_ASYNC|GOAD_ACTIVATE_NEW_ONLY), NULL);

  if(factory_obj == CORBA_OBJECT_NIL)
    return CORBA_OBJECT_NIL;

  sl._length = string_array_len(params);
  sl._buffer = (char **)params;
  sl._release = CORBA_FALSE;

  retval = GNOME_GenericFactory_create_object(factory_obj,
					      sinfo->server_id, &sl, ev);

  if(ev->_major != CORBA_NO_EXCEPTION)
    retval = CORBA_OBJECT_NIL;

  CORBA_Object_release(factory_obj, ev);

  return retval;
}

typedef struct {
  GMainLoop *mloop;
  char iorbuf[2048];
  char *do_srv_output;
  FILE *fh;
} EXEActivateInfo;

static gboolean
handle_exepipe(GIOChannel      *source,
	       GIOCondition     condition,
	       EXEActivateInfo *data)
{
  gboolean retval = TRUE;

  *data->iorbuf = '\0';
  if(!(condition & G_IO_IN)
     || !fgets(data->iorbuf, sizeof(data->iorbuf), data->fh)) {
    retval = FALSE;
  }

  if(retval && !strncmp(data->iorbuf, "IOR:", 4))
    retval = FALSE;

  if(data->do_srv_output)
    g_message("srv output: '%s' (cond", data->iorbuf);

  if(!retval)
    g_main_quit(data->mloop);

  return retval;
}


/**
 * goad_server_activate_exe:
 * @sinfo: information on the program to be run.
 * @flags: information about how the program should be run, etc.
 * (no flags applicable at the present time)
 * @ev: exception information (passed in to save us creating another one)
 *
 * Pre-conditions: Assumes sinfo->type == GOAD_SERVER_EXE
 *
 * Calls setsid to daemonize the server. Expects the server to
 * register itself with the nameing service. Returns after the server
 * printed it's IOR string.  Ignores SIGPIPE in the child, so that the
 * server doesn't die if it writes to a closed fd.
 *
 * Returns an objref to the newly-started server.
 */
CORBA_Object
goad_server_activate_exe(GoadServer *sinfo,
			 GoadActivationFlags flags,
			 const char **params,
			 CORBA_Environment *ev)
{
  gint                    iopipes[2];
  CORBA_Object            retval = CORBA_OBJECT_NIL;
  int childpid;

  pipe(iopipes);
  /* fork & get the ior from stdout */

  if((childpid = fork())) {
    int     status;
    FILE*   iorfh;
    char *do_srv_output;
    EXEActivateInfo ai;
    guint watchid;
    GIOChannel *gioc;

    waitpid(childpid, &status, 0); /* de-zombify */

    close(iopipes[1]);
    ai.fh = iorfh = fdopen(iopipes[0], "r");

    if(flags & GOAD_ACTIVATE_ASYNC)
      goto no_wait;

    ai.do_srv_output = getenv("GOAD_DEBUG_EXERUN");

    ai.mloop = g_main_new(FALSE);
    gioc = g_io_channel_unix_new(iopipes[0]);
    watchid = g_io_add_watch(gioc, G_IO_IN|G_IO_HUP|G_IO_NVAL|G_IO_ERR,
			     (GIOFunc)&handle_exepipe, &ai);
    g_io_channel_unref(gioc);
    g_main_run(ai.mloop);
    g_main_destroy(ai.mloop);

    g_strstrip(ai.iorbuf);
    if (strncmp(ai.iorbuf, "IOR:", 4)) {
      retval = CORBA_OBJECT_NIL;
      goto out;
    }
    if (ai.iorbuf[strlen(ai.iorbuf)-1] == '\n')
      ai.iorbuf[strlen(ai.iorbuf)-1] = '\0';
    retval = CORBA_ORB_string_to_object(_gnorba_gnome_orbit_orb,
					ai.iorbuf, ev);

  no_wait:
    fclose(iorfh);
  } else if(fork()) {
    _exit(0); /* de-zombifier process, just exit */
  } else {
    char **args, **real_args;
    int i;
    struct sigaction sa;

    close(0);
    if(iopipes[1] != GOAD_MAGIC_FD) {
      dup2(iopipes[1], GOAD_MAGIC_FD);
      close(iopipes[1]);
    }
    
    setsid();
    memset(&sa, 0, sizeof(sa));
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

/**
 * goad_server_register:
 * @name_server: points to a running name_server
 * @server: the server we wnat to register.
 * @name:
 * @kind:
 * @ev: CORBA_Environment to return errors
 *
 * Registers @server in the @name_server with @name.
 */
int
goad_server_register(CORBA_Object name_server,
		     CORBA_Object server,
		     const char* name,
		     const char* kind,
		     CORBA_Environment* ev)
{
  CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  CosNaming_Name          nom;
  CORBA_Object            old_server, orig_ns = name_server;
  static int did_print_ior = 0;

  nom._maximum = 0;
  nom._length = 3;
  nom._buffer = nc;
  nom._release = CORBA_FALSE;

  CORBA_exception_free(ev);

  if(!did_print_ior && goad_activation_id) {
    CORBA_char *strior;
    FILE *iorout;
    struct sigaction oldaction, myaction;
    int fl;

    iorout = fdopen(GOAD_MAGIC_FD, "a");
    if(iorout) {
      memset(&myaction, '\0', sizeof(myaction));
      myaction.sa_handler = SIG_IGN;
      
      sigaction(SIGPIPE, &myaction, &oldaction);
      strior = CORBA_ORB_object_to_string(gnome_CORBA_ORB(), server, ev);
      if(ev->_major == CORBA_NO_EXCEPTION) {
	fprintf(iorout, "%s\n", strior);
	CORBA_free(strior);
      }
      fflush(iorout);
      fclose(iorout);
      sigaction(SIGPIPE, &oldaction, NULL);
    }

    did_print_ior = 1;
  }

  nc[2].id   = (char *)name;
  nc[2].kind = (char *)kind;

  CORBA_exception_free(ev);

  if(name_server == CORBA_OBJECT_NIL)
    name_server = gnome_name_service_get();

  old_server = CosNaming_NamingContext_resolve(name_server, &nom, ev);

  if(ev->_major == CORBA_NO_EXCEPTION) {
    CORBA_Object_release(old_server, ev);
    CosNaming_NamingContext_unbind(name_server, &nom, ev);
    g_return_val_if_fail(ev->_major == CORBA_NO_EXCEPTION, -3);
  } else if (ev->_major != CORBA_USER_EXCEPTION 
	     || strcmp(CORBA_exception_id(ev),
		       ex_CosNaming_NamingContext_NotFound)) {

    if(orig_ns == CORBA_OBJECT_NIL)
      CORBA_Object_release(name_server, ev);

    return -2;
  }

  CORBA_exception_free(ev);

  CosNaming_NamingContext_bind(name_server, &nom, server, ev);

  if (ev->_major != CORBA_NO_EXCEPTION) {

    if(orig_ns == CORBA_OBJECT_NIL)
      CORBA_Object_release(name_server, ev);

    return -1;
  }

  if(orig_ns == CORBA_OBJECT_NIL)
    CORBA_Object_release(name_server, ev);

  CORBA_exception_free(ev);

  return 0;
}

/**
 * goad_server_unregister:
 * @name_server: points to a running name_server
 * @server: the server we wnat to remove from the name server registration.
 * @name:
 * @kind:
 * @ev: CORBA_Environment to return errors
 *
 * Removes the registration of  @server in the @name_server.
 */
int
goad_server_unregister(CORBA_Object name_server,
		       const char* name,
		       const char* kind,
		       CORBA_Environment* ev)
{
  CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  CosNaming_Name          nom;
  CORBA_Object orig_ns = name_server;

  nom._maximum = 0;
  nom._length = 3;
  nom._buffer = nc;
  nom._release = CORBA_FALSE;

  if(name_server == CORBA_OBJECT_NIL)
    name_server = gnome_name_service_get();

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
    if(orig_ns == CORBA_OBJECT_NIL)
      CORBA_Object_release(name_server, ev);

    return -1;
  }

  if(orig_ns == CORBA_OBJECT_NIL)
    CORBA_Object_release(name_server, ev);

  CORBA_exception_free(ev);
  return 0;
}

/**
 * goad_server_activation_id:
 *
 * Returns the activation_id for GOAD
 */
const char *
goad_server_activation_id(void)
{
  return goad_activation_id;
}

/**
 * goad_register_arguments:
 *
 * Internal gnome function.  Used to register the arguments for
 * the popt parser
 */
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
