#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
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

static CORBA_boolean
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

/*
 * I bet these will require porting sooner or later
 */
static void
get_exclusive_lock (int fd)
{
  /* flock (fd, LOCK_EX); */
  struct flock lbuf;
  lbuf.l_type = F_WRLCK;
  lbuf.l_whence = SEEK_SET;
  lbuf.l_start = lbuf.l_len = 0L; /* Lock the whole file.  */
  fcntl (fd, F_SETLKW, &lbuf);

}

static void
release_lock (int fd)
{
  /* flock (fd, LOCK_UN); */
  struct flock lbuf;
  lbuf.l_type = F_UNLCK;
  lbuf.l_whence = SEEK_SET;
  lbuf.l_start = lbuf.l_len = 0L; /* Unlock the whole file.  */
  fcntl (fd, F_SETLKW, &lbuf);
}

/*
 * We assume that if we could get this far, then /tmp/orbit-$username is
 * secured (because of CORBA_ORB_init).
 */
static char *
get_cookie_reliably (void)
{
	char random_string [64];
	struct passwd *pwent;
	char *name;
	int fd;

	pwent = getpwuid(getuid());
	g_assert(pwent);
	
	name = g_copy_strings ("/tmp/orbit-", pwent->pw_name, "/cookie", NULL);

	/*
	 * Create the file exclusively with permissions rw for the
	 * user.  if this fails, it means the file already existed
	 */
	fd = open (name, O_CREAT|O_EXCL|O_WRONLY, S_IRUSR | S_IWUSR );
	if (fd >= 0){
		unsigned int i;

		get_exclusive_lock (fd);
		srandom (time (NULL));
		for (i = 0; i < sizeof (random_string)-2; i++)
			random_string [i] = (random () % (126-33)) + 33;
		
		random_string [sizeof(random_string)-1] = 0;

		write (fd, random_string, sizeof (random_string)-1);
		close (fd);
		release_lock (fd);
	} else {
		fd = open (name, O_RDONLY);
		if (fd == -1)
			g_error ("Could not open the cookie file");
		get_exclusive_lock (fd);
		memset (random_string, 0, sizeof (random_string));
		read (fd, random_string, sizeof (random_string)-1);
		release_lock (fd);
		close (fd);
	}
	g_free (name);
	return g_strdup (random_string);
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
  
  IIOPAddConnectionHandler = orb_add_connection;
  IIOPRemoveConnectionHandler = orb_remove_connection;

  gnome_init(app_id, app_parser, *argc, argv, flags, arg_index);

  gnome_orbit_orb = retval = CORBA_ORB_init(argc, argv, "orbit-local-orb", ev);

  request_cookie._buffer = get_cookie_reliably ();

  g_assert(request_cookie._buffer && *request_cookie._buffer);

  request_cookie._length = strlen(request_cookie._buffer) + 1;

  ORBit_set_request_validation_handler(&gnome_ORBit_request_validate);
  ORBit_set_default_principal(&request_cookie);

  return retval;
}

/* This routine bootstraps CORBA connectivity for a GNOME desktop session... */
CORBA_Object
gnome_get_name_service(void)
{
  static CORBA_Object name_service = CORBA_OBJECT_NIL;
  CORBA_Object retval = NULL;
  char *ior;
  GdkAtom propname, proptype;
  CORBA_Environment ev;

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
