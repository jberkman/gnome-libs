#ifndef FORKPTY_H
#define FORKPTY_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <grp.h>
#include <utmp.h>

extern int forkpty(int *amaster, char *name, 
	struct termios *termp, struct winsize *winp);

#endif
