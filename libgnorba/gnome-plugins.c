#include "gnorba.h"
#include <gmodule.h>
#include <dirent.h>

/* relative to GNOMELIBDIR */
#define PLUGIN_DIR "CORBA/plugins"

static GHashTable *id_to_handle = NULL;

/**** gnome_plugin_get_available_plugins
      Description: Returns an array of strings for plugin ID's

      Notes: g_string_array_free() the returned value after you're
      done with it.
 */
char **gnome_plugin_get_available_plugins(void)
{
  DIR *dirh;
  char *dirname;
  char **retval;
  GPtrArray *dirents;
  struct dirent *dent;

  g_assert(g_module_supported());

  g_return_val_if_fail(dirname = gnome_libdir_file("CORBA/plugins"),
		       NULL);

  g_return_val_if_fail(dirh = opendir(dirname),
		       NULL);

  g_free(dirname);

  while((dent = readdir(dirh))) {
    g_ptr_array_add(dirents, g_strdup(dent->d_name));
  }

  retval = (char **)dirents->pdata;

  g_ptr_array_free(dirents, FALSE);

  return retval;
}

/**** gnome_plugin_use
      Inputs: 'plugin_id' - a plugin_id from the list (or a well-known plugin_id)
      Description: Loads the plugin and returns the GNOME_Plugin
      structure from it.
*/

const GNOME_Plugin *
gnome_plugin_use(const char *plugin_id)
{
  char *rel_filename, *abs_filename, *ctmp;
  GModule *gmod;
  GNOME_Plugin *retval;

  g_assert(g_module_supported());

  g_return_if_fail(plugin_id);

  rel_filename = g_copy_strings("CORBA/plugins", plugin_id);
  abs_filename = gnome_libdir_file(rel_filename);
  g_free(rel_filename);

  gmod = g_module_open(abs_filename, G_MODULE_BIND_LAZY);
  g_free(abs_filename);

  if(!gmod)
    goto error;

  g_module_make_resident(gmod); /* Either this or we have to
				   keep a list of active plugins around
				   and provide a way to free them - no fun */

  if(!g_module_symbol(gmod, "GNOME_Plugins_list", &retval))
    goto error;

  return retval;

 error:
  ctmp = g_module_error();
  g_warning(ctmp);
  g_free(ctmp);
  return NULL;
}
