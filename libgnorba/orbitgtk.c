#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
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

/*** gnome_CORBA_init
     Description: Sets up the ORBit connection add/remove function pointers
                  to our routines, which inform the gtk main loop about
		  the CORBA connection fd's.

		  Calls gnome_init and CORBA_ORB_init with the specified params.

		  Sets up a cookie for requests.
 */
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

/**** gnome_name_service_get

      Outputs: 'retval' - an object reference to the name service.

      Description: This routine bootstraps CORBA connectivity for a
      GNOME desktop session...
 */
CORBA_Object
gnome_name_service_get(void)
{
  static CORBA_Object name_service = CORBA_OBJECT_NIL;
  CORBA_Object gnome_context       = CORBA_OBJECT_NIL;
  CORBA_Object server_context     = CORBA_OBJECT_NIL;
  CORBA_Object retval = NULL;
  char *ior;
  GdkAtom propname, proptype;
  CORBA_Environment ev;
  gint name_pid;
  CosNaming_NameComponent nc;
  CosNaming_Name          context_name;
  
  g_return_val_if_fail(gnome_orbit_orb, CORBA_OBJECT_NIL);

  CORBA_exception_init(&ev);

  propname = gdk_atom_intern("CORBA_NAME_SERVICE", FALSE);
  proptype = gdk_atom_intern("STRING", FALSE);

  if(CORBA_Object_is_nil(name_service, &ev)) {
    int iopipes[2];
    char iorbuf[2048];

    ior = 0;
    
    /* First, try and see if another application has started the name service
       (and indicated its presence by setting a root window property */
    gdk_property_get(GDK_ROOT_PARENT(), propname, proptype,
		     0, 9999, FALSE, NULL,
		     NULL, NULL, (guchar **)&ior);
    
    if (ior)
      {
	name_service = CORBA_ORB_string_to_object(gnome_orbit_orb, ior, &ev);
      }
    if (!CORBA_Object_is_nil(name_service, &ev))
      {
	CosNaming_NameComponent nc = {"GNOME", "subcontext"};
	CosNaming_Name          nom = {0, 1, &nc, CORBA_FALSE};
	CORBA_Object            gnome_context;
	
	gnome_context = CosNaming_NamingContext_resolve(name_service, &nom, &ev);
	if (ev._major != CORBA_NO_EXCEPTION)
	  {
	    g_warning("Resolving '/GNOME' context on running orbit name server:");
	    switch( ev._major )
	      {
	      case CORBA_SYSTEM_EXCEPTION:
		g_warning("sysex: %s.\n", CORBA_exception_id(&ev));
		break;
	      case CORBA_USER_EXCEPTION:
		g_warning("usrex: %s.\n", CORBA_exception_id( &ev ) );
	      default:
		break;
	      }
	  }
	else
	  {
	    goto out;
	  }
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
      /* strip newline if it's there */
      if (iorbuf[strlen(iorbuf)-1] == '\n')
	iorbuf[strlen(iorbuf)-1] = '\0';

      /*
	we have to save the strlen+1 to get the terminating '\0' in
	the property
      */
      gdk_property_change(GDK_ROOT_PARENT(), propname, proptype, 8,
			  GDK_PROP_MODE_REPLACE, iorbuf, strlen(iorbuf)+1);
      /*
	without flush, we won't set the property now. If the client
	dosn't read anything from the X server, the  property will
	never be set. 
      */
      XFlush(GDK_DISPLAY());
      name_service = CORBA_ORB_string_to_object(gnome_orbit_orb, iorbuf, &ev);
      if (name_service != CORBA_OBJECT_NIL)
	{
	  /*
	    Create the default context "/GNOME/servers"
	  */
	  context_name._maximum = 0;
	  context_name._length  = 1;
	  context_name._buffer = &nc;
	  context_name._release = CORBA_FALSE;
	  nc.id = "GNOME";
	  nc.kind = "subcontext";
	  gnome_context = CosNaming_NamingContext_bind_new_context(name_service, &context_name, &ev);
	  if (ev._major != CORBA_NO_EXCEPTION)
	    {
	      g_warning("Creating '/GNOME' context %s %d", __FILE__, __LINE__);
	      
	      switch( ev._major )
		{
		case CORBA_SYSTEM_EXCEPTION:
		  g_warning("sysex: %s.\n", CORBA_exception_id(&ev));
		  break;
		case CORBA_USER_EXCEPTION:
		  g_warning("usrex: %s.\n", CORBA_exception_id( &ev ) );
		default:
		  break;
		}
	    }
	  ev._major = CORBA_NO_EXCEPTION;
	  if (CORBA_Object_is_nil(gnome_context, &ev))
	    {
	      g_warning("gnome_name_server_get: '/GNOME' context is nil\n");
	      return CORBA_OBJECT_NIL;
	    }

	  nc.id="Servers";
	  nc.kind = "subcontext";
	  server_context = CosNaming_NamingContext_bind_new_context(gnome_context, &context_name, &ev);
	  if (ev._major != CORBA_NO_EXCEPTION)
	    {
	      g_warning("Creating '/GNOME/servers' context %s %d", __FILE__, __LINE__);
	      switch( ev._major )
		{
		case CORBA_SYSTEM_EXCEPTION:
		  g_warning("	sysex: %s.\n", CORBA_exception_id(&ev));
		  break;
		case CORBA_USER_EXCEPTION:
		  g_warning("usr	ex: %s.\n", CORBA_exception_id( &ev ) );
		default:
		  break;
		}
	    }
	  if (CORBA_Object_is_nil(server_context, &ev))
	    {
	      g_warning("gnome_name_server_get: '/GNOME/servers context is nil\n");
	      return CORBA_OBJECT_NIL;
	    }
	}
      else
	{
	  return CORBA_OBJECT_NIL;
	}
    } else if(fork()) {
      _exit(0); /* de-zombifier process, just exit */
    } else {
      /* Child of a child. We run the naming service */
      struct sigaction sa;
      struct rlimit rl;
      int    i;
      
      getrlimit(RLIMIT_NOFILE, &rl);
      i = rl.rlim_cur;
      
      sa.sa_handler = SIG_IGN;
      sigaction(SIGPIPE, &sa, 0);
      close(0);
      close(iopipes[0]);
      dup2(iopipes[1], 1);
      dup2(iopipes[1], 2);
      /* close all file descriptors */
      while (i > 2)
	{
	  close(i);
	  i--;
	}
      
      setsid();
      
      execlp("orbit-naming-server", "orbit-naming-server", NULL);
      _exit(1);
    }
  }

 out:
  if(!CORBA_Object_is_nil(name_service, &ev))
    retval = CORBA_Object_duplicate(name_service, &ev);
  else
    g_error("Could not get name service!");

  CORBA_exception_free(&ev);
  return retval;
}
