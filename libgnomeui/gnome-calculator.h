/* GnomeCalculator - double precision simple calculator widget
 *
 * Author: George Lebl <jirka@5z.com>
 */

#ifndef GNOME_CALCULATOR_H
#define GNOME_CALCULATOR_H

#include <gdk/gdk.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkfeatures.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS


#define GNOME_CALCULATOR(obj)         GTK_CHECK_CAST (obj, gnome_calculator_get_type (), GnomeCalculator)
#define GNOME_CALCULATOR_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_calculator_get_type (), GnomeCalculatorClass)
#define GNOME_IS_CALCULATOR(obj)      GTK_CHECK_TYPE (obj, gnome_calculator_get_type ())


typedef struct _GnomeCalculator      GnomeCalculator;
typedef struct _GnomeCalculatorClass GnomeCalculatorClass;

typedef enum {
	GNOME_CALCULATOR_DEG,
	GNOME_CALCULATOR_RAD,
	GNOME_CALCULATOR_GRAD
} GnomeCalculatorMode;

struct _GnomeCalculator {
	GtkVBox vbox;

	gdouble result;
	gchar result_string[13];
	gdouble memory;

	GtkWidget *display;

	GnomeCalculatorMode mode;

	guint add_digit : 1;	/*add a digit instead of starting a new
				  number*/
	guint error : 1;
	guint invert : 1;
	GtkWidget *invert_button;

	GList *stack;
	GtkAccelGroup *accel;
};

struct _GnomeCalculatorClass {
	GtkVBoxClass parent_class;

	void (* result_changed)(GnomeCalculator *gc,
				gdouble result);
};


guint		 gnome_calculator_get_type	(void);
GtkWidget	*gnome_calculator_new		(void);
void		 gnome_calculator_clear		(GnomeCalculator *gc,
						 gint reset);
void		 gnome_calculator_set		(GnomeCalculator *gc,
						 gdouble result);

#define gnome_calculator_get_result(gc) (GNOME_CALCULATOR(gc)->result)

END_GNOME_DECLS

#endif
