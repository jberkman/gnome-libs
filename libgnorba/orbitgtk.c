#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gnorba.h"

CORBA_ORB gnome_orbit_orb;
CORBA_Principal request_cookie;

extern void goad_register_arguments(void);

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
  cnx->user_data = GINT_TO_POINTER (gtk_input_add_full(GIOP_CONNECTION_GET_FD(cnx),
						GDK_INPUT_READ|GDK_INPUT_EXCEPTION,
						(GdkInputFunction)orb_handle_connection,
						NULL, cnx, NULL));
}

static void orb_remove_connection(GIOPConnection *cnx)
{
  gtk_input_remove(GPOINTER_TO_UINT (cnx->user_data));
  cnx->user_data = GINT_TO_POINTER (-1);
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
	
	name = g_strconcat ("/tmp/orbit-", pwent->pw_name, "/cookie", NULL);

	/*
	 * Create the file exclusively with permissions rw for the
	 * user.  if this fails, it means the file already existed
	 */
	fd = open (name, O_CREAT|O_EXCL|O_WRONLY, S_IRUSR | S_IWUSR );
	if (fd >= 0){
		unsigned int i;
		int v;

		get_exclusive_lock (fd);
		srandom (time (NULL));
		for (i = 0; i < sizeof (random_string)-2; i++)
			random_string [i] = (random () % (126-33)) + 33;
		
		random_string [sizeof(random_string)-1] = 0;

		while (1){
			v = write (fd, random_string, sizeof (random_string)-1);
			if (v == sizeof (random_string)-1)
				break;
			if (v == 0)
				g_error (_("Can not write to the cookie file"));
			if (v == -1 && errno != EINTR){
				perror (_("Unknown error while writing cookie file"));
				exit (1);
			}
		}
		close (fd);
		release_lock (fd);
	} else {
		int v;
		
		fd = open (name, O_RDONLY);
		if (fd == -1)
			g_error (_("Could not open the cookie file"));
		get_exclusive_lock (fd);

		memset (random_string, 0, sizeof (random_string));

		while (1){
			v = read (fd, random_string, sizeof (random_string)-1);
			if (v == 0)
				g_error (_("Can not read the cookie file\n"));
			if (v == -1 && errno != EINTR){
				perror (_("While reading the cookie file\n"));
				exit (1);
			}
			break;
		}
		release_lock (fd);
		close (fd);
	}
	g_free (name);
	return g_strdup (random_string);
}

/*** do_CORBA_init
     Description: Sets up the ORBit connection add/remove function pointers
                  to our routines, which inform the gtk main loop about
		  the CORBA connection fd's.

		  Calls gnome_init and CORBA_ORB_init with the specified params.

		  Sets up a cookie for requests.
 */
static CORBA_ORB
do_CORBA_init(int *argc, char **argv,
	      GnorbaInitFlags flags,
	      CORBA_Environment *ev)
{
  CORBA_ORB retval;

  IIOPAddConnectionHandler = orb_add_connection;
  IIOPRemoveConnectionHandler = orb_remove_connection;

  gnome_orbit_orb = retval = CORBA_ORB_init(argc, argv, "orbit-local-orb", ev);

  if(!(flags & GNORBA_INIT_DISABLE_COOKIES)) {
    request_cookie._buffer = get_cookie_reliably ();
    
    g_assert(request_cookie._buffer && *request_cookie._buffer);
    
    request_cookie._length = strlen(request_cookie._buffer) + 1;
    
    ORBit_set_request_validation_handler(&gnome_ORBit_request_validate);
    ORBit_set_default_principal(&request_cookie);
  }

  return retval;
}

CORBA_ORB
gnome_CORBA_init(const char *app_id,
		 const char *app_version,
		 int *argc, char **argv,
		 GnorbaInitFlags gnorba_flags,
		 CORBA_Environment *ev)
{
  CORBA_ORB retval;

  if(gnorba_flags & GNORBA_INIT_SERVER_FUNC)
    goad_register_arguments();

  gnome_init(app_id, app_version, *argc, argv);
  retval = do_CORBA_init(argc, argv, gnorba_flags, ev);

  return retval;
}

CORBA_ORB
gnome_CORBA_init_with_popt_table(const char *app_id,
				 const char *app_version,
				 int *argc, char **argv,
				 const struct poptOption *options,
				 int popt_flags,
				 poptContext *return_ctx,
				 GnorbaInitFlags gnorba_flags,
				 CORBA_Environment *ev)
{
  CORBA_ORB retval;

  if(gnorba_flags & GNORBA_INIT_SERVER_FUNC)
    goad_register_arguments();

  gnome_init_with_popt_table(app_id, app_version, *argc, argv, options,
			     popt_flags, return_ctx);
  retval = do_CORBA_init(argc, argv, gnorba_flags, ev);

  return retval;
}

CORBA_ORB
gnome_CORBA_ORB(void)
{
  return gnome_orbit_orb;
}

