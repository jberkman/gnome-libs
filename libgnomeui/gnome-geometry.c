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
get_number (const char **geometry)
{
	int value = 0;
	int mult  = 1;
	
	if (**geometry == '-'){
		mult = -1;
		(*geometry)++;
	}
	while (**geometry && isdigit (**geometry)){
		value = value * 10 + (**geometry - '0');
		(*geometry)++;
	}
	return value * mult;
}

/*
 * Returns 1 if the geometry was successfully parsed, 0 otherwise
 * values are filled with the corresponding values.
 * if no value was found, the value is set to zero.
 */
gboolean
gnome_parse_geometry (const gchar *geometry, gint *xpos, 
		      gint *ypos, gint *width, gint *height)
{
	int subtract;

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
		subtract = 0;
		geometry++;
	} else if (*geometry == '-'){
		subtract = gdk_screen_width ();
		geometry++;
	} else
		return 0;
	*xpos = get_number (&geometry);
	if (subtract)
		*xpos = subtract - *xpos;
	if (!*geometry)
		return 1;
	if (*geometry == '+'){
		subtract = 0;
		geometry++;
	} else if (*geometry == '-'){
		subtract = gdk_screen_height ();
		geometry++;
	} else
		return 0;
	*ypos = get_number (&geometry);
	if (subtract)
		*ypos = subtract - *ypos;
	return 1;
}

/* lifted from gnomecal */

#define BUFSIZE 32

gchar * gnome_geometry_string (GdkWindow * window)
{
  gint x, y, w, h;
  gchar *buffer = g_malloc (BUFSIZE + 1);
  
  gdk_window_get_origin (window, &x, &y);
  gdk_window_get_size (window, &w, &h);

  g_snprintf (buffer, BUFSIZE, "%dx%d+%d+%d", w, h, x, y);
  
#ifdef GNOME_ENABLE_DEBUG
  g_print("Geometry string: %s\n", buffer);
#endif

  return buffer;
}
