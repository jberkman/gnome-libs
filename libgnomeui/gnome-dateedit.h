#ifndef __GNOME_DATE_EDIT_H_
#define __GNOME_DATE_EDIT_H_ 

#include <gtk/gtkhbox.h>
#include <libgnome/gnome-defs.h>
 
BEGIN_GNOME_DECLS

#define GNOME_DATE_EDIT(obj)          GTK_CHECK_CAST (obj, gnome_date_edit_get_type(), GnomeDateEdit)
#define GNOME_DATE_EDIT_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_date_edit_get_type(), GnomeDateEditClass)
#define GNOME_IS_DATE_EDIT(obj)       GTK_CHECK_TYPE (obj, gnome_date_edit_get_type ())

typedef struct {
	GtkHBox hbox;

	GtkWidget *date_entry;
	GtkWidget *date_button;
	
	GtkWidget *time_entry;
	GtkWidget *time_popup;

	time_t    time;
	int       lower_hour;
	int       upper_hour;
} GnomeDateEdit;

typedef struct {
	GtkHBoxClass parent_class;
} GnomeDateEditClass;

GtkWidget *gnome_date_edit_new            (time_t the_time);

void      gnome_date_edit_set_time        (GnomeDateEdit *gde, time_t the_time);
void      gnome_date_edit_set_popup_range (GnomeDateEdit *gde, int low_hour, int up_hour);

#endif
