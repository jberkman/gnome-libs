#ifndef GNORBA_H
#define GNORBA_H 1

#include <orb/orbit.h>
#include <libgnorba/name-service.h>

CORBA_ORB gnome_CORBA_ORB_init(int *argc, char **argv,
			       CORBA_ORBid orb_identifier,
			       CORBA_Environment *ev);
CORBA_Object gnome_get_name_service(void);

#endif
