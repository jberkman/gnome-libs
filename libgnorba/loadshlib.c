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

int
main(int argc, char* argv[])
{
  CORBA_Environment  ev;
  CORBA_Object       retval;
  poptContext ctx;
  char **args;

  CORBA_exception_init(&ev);

  orb = gnome_CORBA_init_with_popt_table("loadshlib", VERSION, &argc,
					 argv, options, 0, &ctx,
					 GNORBA_INIT_SERVER_FUNC, &ev);

  if(!(id || rid)) {
    fprintf(stderr, "You must specify a GOAD ID or a Repository ID.\n\n");
    poptPrintHelp(ctx, stdout, 0);
    return 1;
  }

  args = poptGetArgs(ctx);

  CORBA_Object_release(goad_server_activate_with_id(NULL, args[0],
						    GOAD_ACTIVATE_SHLIB, NULL), &ev);

  gtk_main();

  return 0;
}
