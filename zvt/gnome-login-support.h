#ifndef _GNOME_LOGIN_SUPPORT_H
#define _GNOME_LOGIN_SUPPORT_H

#ifdef HAVE_OPENPTY
#    include <pty.h>
#else
int openpty (int *master_fd, int *slavefd, char *name, struct termios *termp, struct winsize *winp);
pid_t forkpty (int *master_fd, char *name, struct termios *termp, struct winsize *winp);
#endif

#ifndef HAVE_LOGIN_TTY
int login_tty (int fd);
#endif

#endif /* _GNOME_LOGIN_SUPPORT_H */
