#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gnorba.h"

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

static CORBA_ORB orb;

CORBA_ORB
gnome_CORBA_ORB_init(int *argc, char **argv, CORBA_ORBid orb_identifier,
		CORBA_Environment *ev)
{
  CORBA_ORB retval;
  IIOPAddConnectionHandler = orb_add_connection;
  IIOPRemoveConnectionHandler = orb_remove_connection;

  orb = retval = CORBA_ORB_init(argc, argv, orb_identifier, ev);

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

  g_return_val_if_fail(orb, CORBA_OBJECT_NIL);

  CORBA_exception_init(&ev);

  propname = gdk_atom_intern("CORBA_NAME_SERVICE", FALSE);
  proptype = gdk_atom_intern("XA_STRING", FALSE);

  if(CORBA_Object_is_nil(name_service, &ev)) {
    int iopipes[2];
    GdkAtom return_proptype;
    gint fmt, len;
    char iorbuf[2048];

    /* First, try and see if another application has started the name service
       (and indicated its presence by setting a root window property */
    if(gdk_property_get(GDK_ROOT_PARENT(), propname, proptype,
			0, UINT_MAX, FALSE, &return_proptype,
			&fmt, &len, (guchar **)&ior)) {
      name_service = CORBA_ORB_string_to_object(orb, ior, &ev);
      if(ev._major != CORBA_NO_EXCEPTION)
	name_service = CORBA_OBJECT_NIL;
      goto out;
    }

    /* Since we're pretty sure no name server is running, we start it ourself
       and tell the (GNOME session) world about it */
    /* fork & get the ior from orbit-name-service stdout */
    pipe(iopipes);

    if(fork()) {
      FILE *iorfh;

      /* Parent */
      close(iopipes[1]);

      iorfh = fdopen(iopipes[0], "r");

      while(fgets(iorbuf, sizeof(iorbuf), iorfh) && strncmp(iorbuf, "IOR:", 4))
	/* Just read lines until we get what we're looking for */ ;

      if(strncmp(iorbuf, "IOR:", 4)) {
	name_service = CORBA_OBJECT_NIL;
	goto out;
      }

      fclose(iorfh);

      gdk_property_change(GDK_ROOT_PARENT(), propname, proptype, 8,
			  GDK_PROP_MODE_REPLACE, iorbuf, strlen(iorbuf));
      name_service = CORBA_ORB_string_to_object(orb, iorbuf, &ev);
      
      if(ev._major != CORBA_NO_EXCEPTION)
	name_service = CORBA_OBJECT_NIL;

    } else if(fork()) {
      _exit(0); /* de-zombifier process, so we exit */
    } else {
      /* Child of a child. We run the naming service */
      close(0);
      close(iopipes[0]);
      dup2(iopipes[1], 1);
      dup2(iopipes[1], 2);
      execlp("gnome-naming-server", "gnome-naming-server", NULL);
      _exit(1);
    }
  }

 out:
  retval = CORBA_Object_duplicate(name_service, &ev);
  CORBA_exception_free(&ev);
  return retval;
}
