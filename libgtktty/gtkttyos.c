/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * GtkTty (gtkttyos.c): very os specific functions for GtkTty
 * Copyright (C) 1997 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <sys/types.h>
#include <fcntl.h>
#include <grp.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>

#if defined(__FreeBSD__)
#  include <sys/ioctl_compat.h>
#endif	/* __FreeBSD__ */

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif


/* --- pty names to search --- */
#define	PTY_TEMPLATE	"/dev/*ty??"
#define	PTY_SUFFIX_1	"pqrs"
#define	PTY_SUFFIX_2	"0123456789abcdef"


/* --- opaque system specific data structure --- */
typedef struct
{
  struct termios	tty_termios;
  struct winsize	tty_winsize;
  
} GtkTtyOsData;


/* --- functions --- */
static gpointer
gtk_tty_os_get_hintp (void)
{
  GtkTtyOsData	*osdat;
  
  osdat = g_new0 (GtkTtyOsData, 1);
  
  if (tcgetattr (STDIN_FILENO, &osdat->tty_termios) < 0)
  {
    g_warning ("tcgetattr(stdin,) failed: %s", g_strerror (errno));
    
    osdat->tty_termios.c_iflag = BRKINT | ICRNL | IMAXBEL;

    osdat->tty_termios.c_oflag = CREAD | OPOST | ONLCR | NL0  | CR0 | TAB0 | BS0 | FF0;
#ifdef	VT0
    /* FreeBSD doesn't have it?
     */
    osdat->tty_termios.c_oflag |= VT0;
#endif	/* VT0 */

    osdat->tty_termios.c_cflag = CS8;

    osdat->tty_termios.c_lflag = ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;

    osdat->tty_termios.c_cc[VERASE] = '\b';
    osdat->tty_termios.c_cc[VKILL] = '\025';
  }
  
  if (ioctl (STDIN_FILENO, TIOCGWINSZ, (gchar*) &osdat->tty_winsize) < 0)
  {
    g_warning ("ioctl(stdin,TIOCGWINSZ,) failed: %s", g_strerror (errno));
    
    osdat->tty_winsize.ws_row = 25;
    osdat->tty_winsize.ws_col = 80;
    osdat->tty_winsize.ws_xpixel = 0;
    osdat->tty_winsize.ws_ypixel = 0;
  }
  
  return osdat;
}

static void
gtk_tty_os_open_pty (GtkTty *tty)
{
  gchar			pty_name[] = PTY_TEMPLATE;
  gchar			*suf_1;
  gchar			*suf_2;
  gchar			*s1, *s2, *pt;
  
  pt = strchr (pty_name, '*');
  s1 = strchr (pty_name, '?');
  s2 = strchr (s1 + 1, '?');
  
  *pt = 'p';
  
  tty->pty_fd = -1;
  for (suf_1 = PTY_SUFFIX_1; tty->pty_fd < 0 && *suf_1; suf_1++)
  {
    *s1 = *suf_1;
    
    for (suf_2 = PTY_SUFFIX_2; tty->pty_fd < 0 && *suf_2; suf_2++)
    {
      *s2 = *suf_2;
      
      tty->pty_fd = open (pty_name, O_RDWR | O_NONBLOCK);
      if (tty->pty_fd >= 0)
      {
	gint tty_fd;

	*pt = 't';

	tty_fd = open (pty_name, O_RDWR);
	if (tty_fd < 0)
	{
	  close (tty->pty_fd);
	  tty->pty_fd = -1;

	  *pt = 'p';
	}
	else
	  close (tty_fd);
      }
    }
  }
  
  if (tty->pty_fd < 0)
    return;
  
  tty->tty_name = g_strdup (pty_name);
}

static void
gtk_tty_os_close_pty (GtkTty *tty)
{
  if (tty->pty_fd >= 0)
  {
    close (tty->pty_fd);
    tty->pty_fd = -1;
  }
  
  if (tty->tty_name)
  {
    g_free (tty->tty_name);
    tty->tty_name = NULL;
  }
}

static void
gtk_tty_os_setup_tty (GtkTty	*tty,
		      FILE	*f_tty,
		      gpointer	os_data)
{
  struct group	*tty_group;
  gint		tty_gid;
  gint		tty_fd;
  gint		err;
  GtkTtyOsData	*osdat = os_data;
  
  tty_fd = fileno (f_tty);
  if (tty_fd < 0)
  {
    /* this is definitely *not* expected! */
    _exit (-1);
  }
  
  gtk_tty_os_close_except (tty_fd);
  
  if (setsid () < 0)
  {
    fprintf (f_tty, "hum? setsid() failed: %s\n", g_strerror (errno));
    _exit (-1);
  }
  
  tty_group = getgrnam ("tty");
  if (tty_group)
    tty_gid = tty_group->gr_gid;
  else
    tty_gid = -1;
  
  /* this will not work until we are setuid root, which we would not
   * want to be in most cases; so ignore any errors...
   */
  (void) fchown (tty_fd, getuid(), tty_gid);
  (void) fchmod (tty_fd, S_IRUSR | S_IWUSR | S_IWGRP);
  
  if (tcsetattr (tty_fd, TCSANOW, &osdat->tty_termios) < 0)
  {
    fprintf (f_tty, "tcsetattr(\"%s\",) failed: %s\n", tty->tty_name, g_strerror (errno));
    _exit (-1);
  }
  
  err = gtk_tty_os_winsize (tty, os_data, tty_fd,
			    GTK_TERM (tty)->term_width,
			    GTK_TERM (tty)->term_height,
			    0, 0);
  if (err)
  {
    fprintf (f_tty, "setting tty dimensions failed: %s\n", g_strerror (err));
    _exit (-1);
  }
  
  if (!freopen (tty->tty_name, "r", stdin))
  {
    fprintf (f_tty, "redirect stdin to %s failed: %s\n", tty->tty_name, g_strerror (errno));
    exit (-1);
  }
  
#ifdef	TIOCSCTTY
  /* SunOs seems to have not defined this...
   * maybe this is even linux specific - don't know
   */
  if (ioctl (STDIN_FILENO, TIOCSCTTY, 0) < 0)
  {
    fprintf (f_tty, "setting controlling tty failed: %s\n", g_strerror (errno));
    exit (-1);
  }
#endif	/* TIOCSCTTY */
  
  if (dup2 (tty_fd, STDOUT_FILENO) < 0)
  {
    fprintf (f_tty, "redirect stdout to %s failed: %s\n", tty->tty_name, g_strerror (errno));
    exit (-1);
  }
  
  if (dup2 (tty_fd, STDERR_FILENO) < 0)
  {
    fprintf (f_tty, "redirect stderr to %s failed: %s\n", tty->tty_name, g_strerror (errno));
    exit (-1);
  }
  
  if (tty_fd != STDIN_FILENO &&
      tty_fd != STDOUT_FILENO &&
      tty_fd != STDERR_FILENO)
  {
    (void) fclose (f_tty);
    (void) close (tty_fd);
  }
}

static void
gtk_tty_os_close_except (gint	ex_fd)
{
  guint	i = 0;
  guint max_fd = 0;
  
#ifdef	HAVE_GETRLIMIT
  struct rlimit rl;
  
#if	defined	(RLIMIT_NOFILE)
  if (getrlimit (RLIMIT_NOFILE, &rl) >= 0)
    i = rl.rlim_max;
#endif	/* RLIMIT_NOFILE */
  max_fd = MAX (max_fd, i);
  
#if	defined (RLIMIT_OFILE)
  if (getrlimit (RLIMIT_OFILE, &rl) >= 0)
    i = rl.rlim_max;
#endif	/* RLIMIT_OFILE */
  max_fd = MAX (max_fd, i);
  
#endif	/* HAVE_GETRLIMIT */
  
#if	defined (NOFILE)
  i = NOFILE;
#endif	/* NOFILE */
  max_fd = MAX (max_fd, i);
  
#ifdef	HAVE_GETDTABLESIZE
  i = getdtablesize ();
#endif	/* HAVE_GETDTABLESIZE */
  max_fd = MAX (max_fd, i);
  
  if (!max_fd)
    max_fd = 1024;
  
  while (max_fd--)
    if (max_fd != ex_fd)
      (void) close (max_fd);
}

static gint
gtk_tty_os_winsize (GtkTty	   *tty,
		    gpointer	   os_data,
		    gint	   tty_fd,
		    guint	   width,
		    guint	   height,
		    guint	   xpix,
		    guint	   ypix)
{
  GtkTtyOsData	*osdat = os_data;
  
  osdat->tty_winsize.ws_row = height;
  osdat->tty_winsize.ws_col = width;
  osdat->tty_winsize.ws_xpixel = xpix;
  osdat->tty_winsize.ws_ypixel = ypix;
  
  if (ioctl(tty_fd, TIOCSWINSZ, &osdat->tty_winsize) < 0)
    return errno;
  else
    return 0;
}

static	void
gtk_tty_os_wait (GtkTty		*tty)
{
  gint		err;
  int		status;
  gboolean	restart = FALSE;
#ifdef	HAVE_WAIT4
  struct rusage res_usage;
#endif	/* HAVE_WAIT4 */
  
  g_assert (tty->pid > 0); /* paranoid */


wait_restart:
  do
  {
#ifdef	HAVE_WAIT4
    memset (&res_usage, 0, sizeof (res_usage));
    err = wait4 (tty->pid, &status, WNOHANG, &res_usage);
#else	/* not HAVE_WAIT4 */
    err = waitpid (tty->pid, &status, WNOHANG);
#endif	/* not HAVE_WAIT4 */
  }
  while (err == -1 && errno == EINTR);
  
  tty->exit_status = 0;
  tty->exit_signal = 0;
  tty->sys_usec = 0;
  tty->sys_sec = 0;
  tty->user_usec = 0;
  tty->user_sec = 0;
  
  if (err < 0)
    g_warning ("obtain child status failed: %s", g_strerror (errno));
  else if (err && err != tty->pid)
    g_warning ("hum? wrong child: %d instead of %d", err, tty->pid);
  else
  {
    if (WIFEXITED (status))
    {
      tty->pid = 0;
      tty->exit_status = WEXITSTATUS (status);
    }
    
    if (WIFSIGNALED (status))
    {
      tty->pid = 0;
      tty->exit_signal = WTERMSIG (status);
    }
    
    if (err == 0)
    {
      if (!restart)
      {
	restart = TRUE;
	g_warning ("unexpected: no child...");
      }
      
      goto wait_restart;
    }
    else
    {
#ifdef	HAVE_WAIT4
      tty->sys_usec = res_usage.ru_stime.tv_usec;
      tty->sys_sec = res_usage.ru_stime.tv_sec;
      tty->user_usec = res_usage.ru_utime.tv_usec;
      tty->user_sec = res_usage.ru_utime.tv_sec;
#endif	/* HAVE_WAIT4 */
    }
  }
}
