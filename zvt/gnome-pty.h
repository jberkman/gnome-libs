#ifndef GNOME_PTY_H
#define GNOME_PTY_H

typedef enum {
	GNOME_PTY_OPEN_PTY = 1,
	GNOME_PTY_OPEN_NO_DB_UPDATE,
	GNOME_PTY_CLOSE_PTY
} GnomePtyOps;

void *update_dbs         (char *login_name, char *display_name, char *term_name);
void write_logout_record (void *data);


#ifdef HAVE_UTMPX_H
#    define USE_SYSV_UTMP
#else
#    ifdef HAVE_SETUTENT
#        define USE_SYSV_UTMP
#    endif
#endif

#endif
