#ifndef GNORBA_H
#define GNORBA_H 1

#include <orb/orbit.h>
#include <libgnorba/name-service.h>
#include <gnome.h>

/* Almost the same as gnome_init, except it initializes ORBit for use
   with gtk+ too */
CORBA_ORB gnome_CORBA_init(char *app_id,
			   struct argp *app_parser,
			   int *argc, char **argv,
			   unsigned int flags,
			   int *arg_index,
			   CORBA_Environment *ev);
CORBA_Object gnome_get_name_service(void);

#endif
