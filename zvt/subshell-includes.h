
/* {{{ Declarations */

#include <stdio.h>      
#include <fcntl.h>	/* for close-on-exec stuff	      */
#include <sys/signal.h> 
#include <stdlib.h>	/* For errno, putenv, etc.	      */
#include <errno.h>	/* For errno on SunOS systems	      */
#include <termios.h>	/* tcgetattr(), struct termios, etc.  */
#if (!defined(__IBMC__) && !defined(__IBMCPP__))
#include <sys/types.h>	/* Required by unistd.h below	      */
#endif
#include <sys/ioctl.h>	/* For ioctl() (surprise, surprise)   */
#include <fcntl.h>	/* For open(), etc.		      */
#include <string.h>	/* strstr(), strcpy(), etc.	      */
#include <signal.h>	/* sigaction(), sigprocmask(), etc.   */
#ifndef SCO_FLAVOR
#	include <sys/time.h>	/* select(), gettimeofday(), etc.     */
#endif /* SCO_FLAVOR */

#include <unistd.h>	/* For pipe, fork, setsid, access etc */


#ifdef HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

/*#ifdef HAVE_SYS_WAIT_H*/
#   include <sys/wait.h> /* For waitpid() */
/*#endif*/

#ifndef WEXITSTATUS
#   define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif

#ifndef WIFEXITED
#   define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifdef HAVE_GRANTPT
#   include <stropts.h> /* For I_PUSH			      */
#else
#   include <grp.h>	/* For the group struct & getgrnam()  */
#endif

#ifdef SCO_FLAVOR
#   include <grp.h>	/* For the group struct & getgrnam()  */
#endif /* SCO_FLAVOR */

#include <sys/socket.h>

#include "gnome-login-support.h"
#include "vt.h"
#include "subshell.h"
#include "gnome-pty.h"
#include <glib.h>
