#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gnorba.h"

CORBA_ORB gnome_orbit_orb;
CORBA_Principal request_cookie;

static void
orb_handle_connection(GIOPConnection *cnx, gint source, GdkInputCondition cond)
{

  /* The best way to know about an fd exception is if select()/poll()
     tells you about it, so we just relay that information on to ORBit
     if possible */

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

CORBA_boolean
gnome_ORBit_request_validate(CORBA_unsigned_long request_id,
			     CORBA_Principal *principal,
			     CORBA_char *operation)
{
  if(principal->_length == request_cookie._length
     && !(principal->_buffer[principal->_length - 1])
     && !strcmp(principal->_buffer, request_cookie._buffer))
    return CORBA_TRUE;
  else
    return CORBA_FALSE;
}

CORBA_ORB
gnome_CORBA_init(char *app_id,
		 struct argp *app_parser,
		 int *argc, char **argv,
		 unsigned int flags,
		 int *arg_index,
		 CORBA_Environment *ev)
{
  CORBA_ORB retval;
  GString *tmpstr;

  IIOPAddConnectionHandler = orb_add_connection;
  IIOPRemoveConnectionHandler = orb_remove_connection;

  gnome_init(app_id, app_parser, *argc, argv, flags, arg_index);

  gnome_orbit_orb = retval = CORBA_ORB_init(argc, argv, "orbit-local-orb", ev);

  tmpstr = g_string_new(NULL);

  g_string_sprintf(tmpstr, "/panel/Secret/cookie-DISPLAY-%s=",
		   getenv("DISPLAY"));

  request_cookie._buffer = gnome_config_private_get_string(tmpstr->str);

  g_assert(request_cookie._buffer && *request_cookie._buffer);

  request_cookie._length = strlen(request_cookie._buffer) + 1;

  g_string_free(tmpstr, TRUE);

  ORBit_set_request_validation_handler(&gnome_ORBit_request_validate);
  ORBit_set_default_principal(&request_cookie);

  return retval;
}

/* This routine bootstraps CORBA connectivity for a GNOME desktop session... */
CORBA_Object
gnome_get_name_service(void)
{
  static CORBA_Object name_service = CORBA_OBJECT_NIL;
  CORBA_Object retval;
  char *ior;
  GdkAtom propname, proptype;
  CORBA_Environment ev;
  gint len;

  g_return_val_if_fail(gnome_orbit_orb, CORBA_OBJECT_NIL);

  CORBA_exception_init(&ev);

  propname = gdk_atom_intern("CORBA_NAME_SERVICE", FALSE);
  proptype = gdk_atom_intern("STRING", FALSE);

  if(CORBA_Object_is_nil(name_service, &ev)) {

    /* First, try and see if another application has started the name service
       (and indicated its presence by setting a root window property */
    gdk_property_get(GDK_ROOT_PARENT(), propname, proptype,
		     0, 9999, FALSE, NULL,
		     NULL, NULL, (guchar **)&ior);
    if(!ior)
      goto out;

    name_service = CORBA_ORB_string_to_object(gnome_orbit_orb, ior, &ev);

    if(ev._major != CORBA_NO_EXCEPTION)
      name_service = CORBA_OBJECT_NIL;
  }

 out:
  if(!CORBA_Object_is_nil(name_service, &ev))
    retval = CORBA_Object_duplicate(name_service, &ev);
  else
    g_error("Could not get name service!");

  CORBA_exception_free(&ev);
  return retval;
}
