/*
 * Geometry string parsing code
 * Copyright (C) 1998 the Free Software Foundation
 *
 * Author: Miguel de Icaza
 */
#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include "gnome-geometry.h"

static int
get_number (char **geometry)
{
	int value = 0;

	while (**geometry && isdigit (**geometry)){
		value = value * 10 + (**geometry - '0');
		(*geometry)++;
	}
	return value;
}

/*
 * Returns 1 if the geometry was successfully parsed, 0 otherwise
 * values are filled with the corresponding values.
 * if no value was found, the value is set to zero.
 */
int
gnome_parse_geometry (char *geometry, int *xpos, int *ypos, int *width, int *height)
{
	int value;
	int substract;

	g_return_val_if_fail (xpos != NULL, 0);
	g_return_val_if_fail (ypos != NULL, 0);
	g_return_val_if_fail (width != NULL, 0);
	g_return_val_if_fail (height != NULL, 0);
	
	*xpos = *ypos = *width = *height = -1;

	if (!geometry)
		return 0;

	if (*geometry == '=')
		geometry++;
	if (!*geometry)
		return 0;
	if (isdigit (*geometry))
		*width = get_number (&geometry);
	if (!*geometry)
		return 1;
	if (*geometry == 'x' || *geometry == 'X'){
		geometry++;
		*height = get_number (&geometry);
	}
	if (!*geometry)
		return 1;
	if (*geometry == '+'){
		substract = 0;
		geometry++;
	} else if (*geometry == '-'){
		substract = gdk_screen_width ();
		geometry++;
	} else
		return 0;
	*xpos = get_number (&geometry);
	if (substract)
		*xpos = substract - *xpos;
	if (!*geometry)
		return 1;
	if (*geometry == '+'){
		substract = 0;
		geometry++;
	} else if (*geometry == '-'){
		substract = gdk_screen_height ();
		geometry++;
	} else
		return 0;
	*ypos = get_number (&geometry);
	if (substract)
		*ypos = substract - *ypos;
	return 1;
}

