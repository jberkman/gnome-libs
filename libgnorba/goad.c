#include "gnorba.h"
#include <dirent.h>

const GoadServer *
goad_servers_list(void)
{
  g_assert(!"NYI");
}

CORBA_Object
goad_server_activate(GoadServer *sinfo, GoadActivationFlags flags)
{
  g_assert(!"NYI");
}

/* Picks the first one on the list that meets criteria */
CORBA_Object
goad_server_activate_with_repo_id(const char *repo_id,
				  GoadActivationFlags flags)
{
  g_assert(!"NYI");
}

const GoadServer *goad_servers_list(void)
{
}

CORBA_Object
goad_server_activate(GoadServer *sinfo, GoadActivationFlags flags)
{
}

/* Picks the first one on the list that meets criteria */
CORBA_Object
goad_server_activate_with_repo_id(const char *repo_id,
				  GoadActivationFlags flags)
{
}
