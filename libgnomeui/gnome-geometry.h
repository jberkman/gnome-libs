#ifndef __GNOME_GEOMETRY_H_
#define __GNOME_GEOMETRY_H_ 

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

int gnome_parse_geometry (char *geometry, int *xpos, int *ypos, int *width, int *height);

END_GNOME_DECLS

#endif
