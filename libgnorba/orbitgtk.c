#include <unistd.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gnorba.h"

static void
orb_handle_connection(GIOPConnection *cnx, gint source, GdkInputCondition cond)
{
  switch(cond) {
  case GDK_INPUT_EXCEPTION:
    giop_main_handle_connection_exception(cnx);
    break;
  default:
    giop_main_handle_connection(cnx);
  }
}

static void orb_add_connection(GIOPConnection *cnx)
{
  cnx->user_data = (gpointer)gtk_input_add_full(GIOP_CONNECTION_GET_FD(cnx),
						GDK_INPUT_READ|GDK_INPUT_EXCEPTION,
						(GdkInputFunction)orb_handle_connection,
						NULL, cnx, NULL);
}

static void orb_remove_connection(GIOPConnection *cnx)
{
  gtk_input_remove((guint)cnx->user_data);
  cnx->user_data = (gpointer)-1;
}

static CORBA_ORB orb;

CORBA_ORB
gnorba_ORB_init(int *argc, char **argv, CORBA_ORBid orb_identifier,
		CORBA_Environment *ev)
{
  CORBA_ORB retval;
  IIOPAddConnectionHandler = orb_add_connection;
  IIOPRemoveConnectionHandler = orb_remove_connection;

  orb = retval = CORBA_ORB_init(argc, argv, orb_identifier, ev);

  return retval;
}

CORBA_Object
gnorba_get_name_service(void)
{
  static CORBA_Object name_service = CORBA_OBJECT_NIL;

  if(!name_service) {
    char *ior;
    int pid;
    CORBA_Environment ev;

    CORBA_exception_init(&ev);

    /* Here: fork & get the ior from orbit-name-service stdout */
    g_error("NYI");

    gdk_property_change(GDK_ROOT_PARENT(),
			gdk_atom_intern("CORBA_NAME_SERVICE", FALSE),
			gdk_atom_intern("XA_STRING", FALSE), 8,
			GDK_PROP_MODE_REPLACE, ior, strlen(ior));
    name_service = CORBA_ORB_string_to_object(orb, ior, &ev);
  }

  return name_service;
}
