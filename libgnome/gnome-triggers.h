#ifndef __GNOME_TRIGGERS_H__
#define __GNOME_TRIGGERS_H__

#include "gnome-defs.h"
#include <glib.h>
BEGIN_GNOME_DECLS

enum _GnomeTriggerType { GTRIG_NONE, GTRIG_FUNCTION, GTRIG_COMMAND, GTRIG_MEDIAPLAY };
typedef enum _GnomeTriggerType GnomeTriggerType;
typedef void (*GnomeTriggerActionFunction)(char *msg, char *level, char *supinfo[]);

struct _GnomeTrigger {
	GnomeTriggerType type;
	union {
	/* These will be passed the same info as
	   gnome_triggers_do got */
	  GnomeTriggerActionFunction function;
	  gchar *command;
	  struct {
	    gchar *file;
	    int cache_id;
	  } media;
	} u;
        gchar *level;
};
typedef struct _GnomeTrigger * GnomeTrigger;

/* Must be called before doing any triggers stuff */
void gnome_triggers_init(void);
gint gnome_triggers_readfile(gchar *filename);

/* The optional arguments in some of these functions are just
   a list of strings that help us know
   what type of event happened. For example,

   gnome_triggers_do("System is out of disk space on /dev/hda1!",
		     "warning", "system", "device", "disk", "/dev/hda1");
*/

void gnome_triggers_add_trigger(GnomeTrigger nt, ...);
void gnome_triggers_vadd_trigger(GnomeTrigger nt,
				 char *supinfo[]);

void gnome_triggers_do(char *msg, char *level, ...);

void gnome_triggers_vdo(char *msg, char *level, char *supinfo[]);

void gnome_triggers_destroy(void);

END_GNOME_DECLS

#endif /* __GNOME_TRIGGERS_H__ */
