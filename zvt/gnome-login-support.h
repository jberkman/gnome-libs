#ifndef _GNOME_LOGIN_SUPPORT_H
#define _GNOME_LOGIN_SUPPORT_H

#ifdef HAVE_OPENPTY
# ifdef HAVE_PTY_H
#    include <pty.h>
# else
#    ifdef HAVE_UTIL_H /* OpenBSD */
#      include <util.h>
#    else
#      ifdef HAVE_LIBUTIL_H /* FreeBSD */
#        include <libutil.h>
#      endif
#    endif
# endif
#else
int openpty (int *master_fd, int *slavefd, char *name, struct termios *termp, struct winsize *winp);
pid_t forkpty (int *master_fd, char *name, struct termios *termp, struct winsize *winp);
#endif

#ifndef HAVE_LOGIN_TTY
int login_tty (int fd);
#endif

#endif /* _GNOME_LOGIN_SUPPORT_H */
