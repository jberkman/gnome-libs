/*  GemVt - GNU Emulator of a Virtual Terminal
 *  Copyright (C) 1997	Tim Janik	<timj@psynet.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#define		__main_c__

#include	"gtktty.h"
#include	"gtkled.h"
#include	"config.h"
#include	"gemvt.xpm"
#include	"gem-r.xpm"
#include	"gem-g.xpm"
#include	"gem-b.xpm"
#include	<gtk/gtk.h>
#include	<signal.h>
#include	<unistd.h>
#include	<string.h>

#ifdef	GNOMIFY
#include	"gnome.h"
#endif	GNOMIFY


/* --- typedefs --- */
typedef struct
{
  GdkColor	*back_col;
  GdkColor	*fore_col;
  GdkColor	*dim_col;
  GdkColor	*bold_col;
  guint32	back_val;
  guint32	fore_val;
  guint32	dim_val;
  guint32	bold_val;
} GvtColorEntry;

/* --- prototypes --- */
static	RETSIGTYPE gvt_signal_handler	(int		sig_num);
static	gint	gvt_tty_key_press	(GtkTty         *tty,
					 const gchar    *char_code,
					 guint          length,
					 guint          keyval,
					 guint          key_state,
					 gpointer       data);
static	void	gvt_execute		(GtkWidget	*widget,
					 gpointer	user_data);
static	void	gvt_kill		(GtkButton	*button,
					 gpointer	user_data);
static	void	gvt_program_exec	(GtkTty		*tty,
					 const gchar	*prg_name,
					 gchar * const	argv[],
					 gchar * const	envp[],
					 gpointer	 user_data);
static	void	gvt_program_exit	(GtkTty		*tty,
					 const gchar	*prg_name,
					 gchar		exit_status,
					 guint		exit_signal,
					 gpointer	 user_data);
static	void	gvt_term_set_color_table(GtkTerm	*term,
					 GdkColormap	*cmap,
					 GvtColorEntry	*color_table,
					 guint		len,
					 guint		first);
static
GtkWidget*	gvt_gem_create		(GtkContainer	*parent,
					 gchar		 col);
static	void	gvt_gem_set_color	(GtkWidget	*gem,
					 gchar		 col);
static	void	gvt_gem_rotate		(GtkWidget	*gem);



/* --- external variables --- */
/* hm, seems like some systems miss the automatic declaration
 * of the environment char array, so we explicitly declare it
 * here, and implicitly assume every system has it...
 */
extern char **environ;

/* --- static variables --- */
static GvtColorEntry	color_table[] =
{
  { NULL, NULL, NULL, NULL,	0x000000, 0x000000, 0x000000, 0x000000 }, /* black */
  { NULL, NULL, NULL, NULL,	0xd00000, 0xd00000, 0x880000, 0xff0000 }, /* red */
  { NULL, NULL, NULL, NULL,	0x00d000, 0x00d000, 0x008800, 0x00ff00 }, /* green */
  { NULL, NULL, NULL, NULL,	0xd0d000, 0xd0d000, 0x888800, 0xffff00 }, /* orange */
  { NULL, NULL, NULL, NULL,	0x0000d0, 0x0000d0, 0x000088, 0x0000ff }, /* blue */
  { NULL, NULL, NULL, NULL,	0xd000d0, 0xd000d0, 0x880088, 0xff00ff }, /* pink */
  { NULL, NULL, NULL, NULL,	0x00d0d0, 0x00d0d0, 0x008888, 0x00ffff }, /* cyan */
  { NULL, NULL, NULL, NULL,	0xd0d0d0, 0xd0d0d0, 0x888888, 0xffffff }, /* white */
};
static GtkWidget	*status_label;
static GtkWidget	*time_label;
static GtkWidget	*signal_button;
static gchar		*prg_name;

/* --- main() --- */
int
main	(int	argc,
	 char	*argv[])
{
  register gchar	*home_dir;
  register gchar	*rc_file;
  register guchar	*string;
  register GtkWidget	*window;
  register GtkWidget	*main_vbox;
  register GtkWidget	*tty_vbox;
  register GtkWidget	*tty_hbox;
  register GtkWidget	*status_hbox;
  register GtkWidget	*led_hbox;
  GtkWidget		*led[8];
  register GtkWidget	*label;
  register GtkWidget	*tty;
  register GtkWidget	*exec_hbox;
  register GtkWidget	*entry;
  register GtkWidget	*button;
  register GtkWidget	*gem;
  register guint	i;
  

  prg_name = g_strdup (argv[0]);
  
  /* Gtk+/GNOME initialization
   */
#ifdef	GNOMIFY
  gnome_init(&argc, &argv);
#else	/* !GNOMIFY */
  gtk_init (&argc, &argv);
#endif	/* !GNOMIFY */

  /* parse ~/.gtkrc, (this file also ommitted by the gnome_init stuff
   */
  home_dir = getenv ("HOME");
  if (!home_dir) /* fatal! */
    home_dir = ".";
  
  string = "gtkrc";
  rc_file = g_new (gchar, strlen (home_dir) + 2 + strlen (string) + 1);
  *rc_file = 0;
  strcat (rc_file, home_dir);
  strcat (rc_file, "/.");
  strcat (rc_file, string);

  printf("%s\n", rc_file);
  gtk_rc_parse (rc_file);
  g_free (rc_file);
  
  string = strrchr (prg_name, '/');
  if (string)
    string++;
  else
    string = prg_name;
  rc_file = g_new (gchar, strlen (home_dir) + 2 + strlen (string) + 2 + 1);
  *rc_file = 0;
  strcat (rc_file, home_dir);
  strcat (rc_file, "/.");
  strcat (rc_file, string);
  strcat (rc_file, "rc");

  /* parse ~/.<prgname>rc
   */
  printf("%s\n", rc_file);
  /* FIXME: invoke rc-parser here, this requires GScanner in GLib first ;( */
  g_free (rc_file);

  
  /* catch system signals
   */
  gvt_signal_handler (0);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "GemVt");
  gtk_window_set_policy (GTK_WINDOW (window),
			 TRUE,
			 TRUE,
			 TRUE);
  gtk_signal_connect_object (GTK_OBJECT (window),
			     "destroy",
			     GTK_SIGNAL_FUNC (gtk_main_quit),
			     NULL);
  
  main_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 10);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
  gtk_widget_show (main_vbox);

  tty_hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (tty_hbox), 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), tty_hbox, TRUE, TRUE, 0);
  gtk_widget_show (tty_hbox);

  tty_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (tty_vbox), 0);
  gtk_box_pack_start (GTK_BOX (tty_hbox), tty_vbox, FALSE, FALSE, 0);
  gtk_widget_show (tty_vbox);
  
  status_hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (tty_vbox), status_hbox, FALSE, FALSE, 0);
  gtk_widget_show (status_hbox);

  status_label = gtk_label_new ("No Program");
  gtk_box_pack_start (GTK_BOX (status_hbox), status_label, TRUE, TRUE, 10);
  gtk_widget_show (status_label);
  
  time_label = gtk_label_new ("System: 0  User: 0");
  gtk_box_pack_start (GTK_BOX (status_hbox), time_label, TRUE, TRUE, 10);
  gtk_widget_show (time_label);
  
  led_hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (status_hbox), led_hbox, FALSE, FALSE, 0);
  gtk_widget_show (led_hbox);
  for (i = 0; i < 8; i++)
  {
    led[i] = gtk_led_new ();
    gtk_box_pack_start (GTK_BOX (led_hbox), led[i], TRUE, TRUE, 5);
    gtk_widget_show (led[i]);
  }
  
  string =
    "Hi, this is GemVt bothering you ;)\n\r"
    "Enter `-bash' or something the like and\n\r"
    "Have Fun!!!\n\r";
  
  tty = gtk_tty_new (80, 25, 99);
  gtk_tty_put_out (GTK_TTY (tty), string, strlen (string));
  gtk_box_pack_start (GTK_BOX (tty_vbox), tty, TRUE, TRUE, 5);
  for (i = 0; i < 8; i++)
  {
    gtk_tty_add_update_led (GTK_TTY (tty), GTK_LED(led[i]), 1 << i);
  }
  gtk_widget_show (tty);
  gtk_signal_connect (GTK_OBJECT (tty),
		      "key_press",
		      GTK_SIGNAL_FUNC (gvt_tty_key_press),
		      NULL);
  gvt_term_set_color_table (GTK_TERM (tty),
			    /* gdk_colormap_get_system (), */
			    gtk_widget_get_colormap (GTK_WIDGET (tty)),
			    color_table,
			    sizeof (color_table) / sizeof (GvtColorEntry),
			    0);
  
  exec_hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (main_vbox), exec_hbox, FALSE, TRUE, 0);
  gtk_widget_show (exec_hbox);

  label = gtk_label_new ("Execute:");
  gtk_box_pack_start (GTK_BOX (exec_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (exec_hbox), entry, TRUE, TRUE, 5);
  gtk_widget_show (entry);
  gtk_signal_connect (GTK_OBJECT (entry),
		      "activate",
		      GTK_SIGNAL_FUNC (gvt_execute),
		      tty);
  
  gtk_signal_connect (GTK_OBJECT (tty),
		      "program_exec",
		      GTK_SIGNAL_FUNC (gvt_program_exec),
		      entry);
  gtk_signal_connect (GTK_OBJECT (tty),
		      "program_exit",
		      GTK_SIGNAL_FUNC (gvt_program_exit),
		      entry);

  signal_button = gtk_button_new_with_label ("Send Signal - 1) TERM 2) KILL");
  gtk_box_pack_start (GTK_BOX (exec_hbox), signal_button, FALSE, TRUE, 0);
  gtk_widget_set_sensitive (signal_button, FALSE);
  gtk_widget_show (signal_button);
  gtk_signal_connect (GTK_OBJECT (signal_button),
		      "clicked",
		      GTK_SIGNAL_FUNC (gvt_kill),
		      tty);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (exec_hbox), button, FALSE, TRUE, 5);
  gtk_widget_show (button);
  gem = gvt_gem_create (GTK_CONTAINER (button), 'r');
  gtk_container_add (GTK_CONTAINER (button), gem);
  gtk_signal_connect_object (GTK_OBJECT (button),
			     "clicked",
			     GTK_SIGNAL_FUNC (gvt_gem_rotate),
			     (GtkObject*) gem);

  button = gtk_button_new_with_label ("Close");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, TRUE, 5);
  gtk_widget_show (button);
  gtk_signal_connect_object (GTK_OBJECT (button),
			     "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     (GtkObject*) window);
  gtk_signal_connect_object (GTK_OBJECT (window),
			     "delete_event",
			     GTK_SIGNAL_FUNC (gtk_button_clicked),
			     GTK_OBJECT (button));
  
  gtk_widget_show (window);
  
  
  /* gtk's main loop
   */
  gtk_main ();
  
  /* exit program
   */
  return 0;
}


RETSIGTYPE
gvt_signal_handler (int	sig_num)
{
  static gboolean	in_call = FALSE;
  
  if (in_call)
  {
    fprintf (stderr,
	     "\naborting on another signal: `%s'\n",
	     g_strsignal (sig_num));
    fflush (stderr);
    gtk_exit (-1);
  }
  else
    in_call = TRUE;
  
  signal (SIGINT,	gvt_signal_handler);
  signal (SIGTRAP,	gvt_signal_handler);
  signal (SIGABRT,	gvt_signal_handler);
  signal (SIGBUS,	gvt_signal_handler);
  signal (SIGSEGV,	gvt_signal_handler);
  signal (SIGPIPE,	gvt_signal_handler);
  /* signal (SIGTERM,	gvt_signal_handler); */
  
  if (sig_num > 0) {
    fprintf (stderr, "%s: (pid: %d) caught signal: `%s'\n",
	     prg_name,
	     getpid (),
	     g_strsignal (sig_num));
    fflush (stderr);
    g_debug (prg_name);
  }
  in_call = FALSE;
}

static void
gvt_execute (GtkWidget	    *widget,
	     gpointer	    user_data)
{
  GtkEntry	*entry;
  GtkTty	*tty;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_ENTRY (widget));
  g_return_if_fail (user_data != NULL);
  g_return_if_fail (GTK_IS_TTY (user_data));
  
  entry = GTK_ENTRY (widget);
  tty = GTK_TTY (user_data);
  
  if (tty->pid)
    g_warning ("Program already running: %d", tty->pid);
  else
  {
    guchar	*prg = NULL;
    GList	*args;
    guchar	*p;
    guchar	*s, *e, *f;
    
    s = g_strdup (gtk_entry_get_text (entry));
    f = s;
    e = s + strlen (s);
    
    p = strchr (s, ' ');
    if (p)
    {
      *p = 0;
      prg = s;
      s = p + 1;
    }
    else if (s < e)
    {
      prg = s;
      s = e;
    }
    
    args = NULL;
    p = strchr (s, ' ');
    while (p || s < e)
    {
      if (p)
      {
	*p = 0;
	args = g_list_append (args, s);
	s = p + 1;
      }
      else if (s < e)
      {
	args = g_list_append (args, s);
	s = e;
      }

      p = strchr (s, ' ');
    }
    
    if (prg)
    {
      gchar **argv;
      GList *list;
      guint i;

      argv = g_new (gchar*, g_list_length (args) + 1 + 1);

      argv[0] = prg;

      if (prg[0] == '-')
	prg++;

      list = args;
      i = 1;
      while (list)
      {
	argv[i++] = list->data;
	list = list->next;
      }
      argv[i] = NULL;

      putenv ("TERM=");
      
      gtk_tty_execute (tty, prg, argv, environ);
      
      g_free (argv);
    }

    g_list_free (args);
    g_free (f);
  }
}

static gint
gvt_tty_key_press (GtkTty         *tty,
		   const gchar    *char_code,
		   guint          length,
		   guint          keyval,
		   guint          key_state,
		   gpointer       data)
{
  /* returning TRUE means let GtkTty handle the keypress
   * if we return FALSE we have to do a gtk_tty_put_in (tty, char_code, length)
   * ourselves.
   * for the moment we just put out '*'s if someone presses a key but there is
   * nochild...
   */
					
  if (tty->pid != 0)
    return FALSE;
  else
  {
    /* we check the length of the original code to make sure
     * we don't react on shift and stuff...
     */
    if (keyval < 127)
      gtk_tty_put_in (tty, char_code, length);
    
    return TRUE;
  }
}

GtkWidget*
gvt_gem_create (GtkContainer *parent,
		gchar	     col)
{
  GtkWidget	*vbox;
  GtkWidget	*pixmap;
  GdkPixmap	*xpm_map;
  GdkBitmap	*bit_map;

  g_assert (parent != NULL);
  g_assert (GTK_IS_CONTAINER (parent));

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (parent, vbox);
  gtk_widget_show (vbox);
  gtk_object_set_data (GTK_OBJECT (vbox), "active-gem", NULL);

  bit_map = NULL;
  xpm_map = gdk_pixmap_create_from_xpm_d (vbox->window,
					  &bit_map,
					  &gtk_widget_get_style (vbox)->bg[GTK_STATE_NORMAL],
					  gem_r_xpm);
  pixmap = gtk_pixmap_new (xpm_map, bit_map);
  gtk_box_pack_start (GTK_BOX (vbox), pixmap, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (pixmap), "gem-color", "red");
  gtk_object_set_data (GTK_OBJECT (vbox), "red-gem", pixmap);

  bit_map = NULL;
  xpm_map = gdk_pixmap_create_from_xpm_d (vbox->window,
					  &bit_map,
					  &gtk_widget_get_style (vbox)->bg[GTK_STATE_NORMAL],
					  gem_g_xpm);
  pixmap = gtk_pixmap_new (xpm_map, bit_map);
  gtk_box_pack_start (GTK_BOX (vbox), pixmap, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (pixmap), "gem-color", "green");
  gtk_object_set_data (GTK_OBJECT (vbox), "green-gem", pixmap);

  bit_map = NULL;
  xpm_map = gdk_pixmap_create_from_xpm_d (vbox->window,
					  &bit_map,
					  &gtk_widget_get_style (vbox)->bg[GTK_STATE_NORMAL],
					  gem_b_xpm);
  pixmap = gtk_pixmap_new (xpm_map, bit_map);
  gtk_box_pack_start (GTK_BOX (vbox), pixmap, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (pixmap), "gem-color", "blue");
  gtk_object_set_data (GTK_OBJECT (vbox), "blue-gem", pixmap);

  gvt_gem_set_color (vbox, col);

  return vbox;
}

static void
gvt_gem_set_color (GtkWidget      *gem,
		   gchar	   col)
{
  GtkObject	*object;
  GtkWidget	*pixmap;
  GtkWidget	*new_pixmap;

  g_assert (gem != NULL);
  g_assert (GTK_IS_OBJECT (gem));

  object = GTK_OBJECT (gem);

  pixmap = gtk_object_get_data (object, "active-gem");

  switch (col)
  {
  case  'r':
    new_pixmap = gtk_object_get_data (object, "red-gem");
    break;

  case  'g':
    new_pixmap = gtk_object_get_data (object, "green-gem");
    break;

  case  'b':
    new_pixmap = gtk_object_get_data (object, "blue-gem");
    break;

  default:
    new_pixmap = NULL;
  }

  if (pixmap)
  {
    gtk_container_block_resize (GTK_CONTAINER (pixmap->parent));
    gtk_widget_hide (pixmap);
  }

  gtk_object_set_data (object, "active-gem", new_pixmap);
  if (new_pixmap)
    gtk_widget_show (new_pixmap);

  if (pixmap)
    gtk_container_unblock_resize (GTK_CONTAINER (pixmap->parent));
}

static void
gvt_gem_rotate (GtkWidget      *gem)
{
  GtkObject	*object;
  GtkWidget	*pixmap;
  gchar		*old_col;
  gchar		col;

  g_assert (gem != NULL);
  g_assert (GTK_IS_OBJECT (gem));

  object = GTK_OBJECT (gem);

  pixmap = gtk_object_get_data (object, "active-gem");

  if (pixmap)
  {
    g_assert (GTK_IS_OBJECT (pixmap));

    old_col = gtk_object_get_data (GTK_OBJECT (pixmap), "gem-color");
  }
  else
    old_col = NULL;

  if (!old_col)
    old_col = "red";

  switch (old_col[0])
  {
  default:
  case  'r':
    col = 'g';
    break;

  case  'g':
    col = 'b';
    break;

  case  'b':
    col = 'r';
    break;
  }

  gvt_gem_set_color (gem, col);
}

static void
gvt_kill (GtkButton	 *button,
	  gpointer	 user_data)
{
  GtkTty *tty;
  static guint	 pid = 0;

  g_return_if_fail (user_data != NULL);
  g_return_if_fail (GTK_IS_TTY (user_data));

  tty = user_data;

  if (tty->pid)
  {
    if (pid == tty->pid)
    {
      kill (pid, SIGKILL);
      pid = 0;
    }
    else
    {
      pid = tty->pid;
      kill (pid, SIGTERM);
    }
  }
  else
    pid = 0;
}

static void
gvt_program_exec (GtkTty	 *tty,
		  const gchar	 *prg_name,
		  gchar * const	 argv[],
		  gchar * const	 envp[],
		  gpointer	 user_data)
{
  GtkEntry *entry;
  gchar *string;
  gchar *text = "Executing `%s'...";

  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  g_return_if_fail (user_data != NULL);
  g_return_if_fail (GTK_IS_ENTRY (user_data));

  entry = GTK_ENTRY (user_data);

  gtk_widget_set_sensitive (signal_button, TRUE);

  string = g_new (gchar, strlen (text) + strlen (prg_name) + 1);
  sprintf (string, text, prg_name);
  gtk_label_set (GTK_LABEL (status_label), string);
  g_free (string);

  gtk_label_set (GTK_LABEL (time_label), "System: 0  User: 0");

  if (GTK_WIDGET_HAS_FOCUS (entry))
    gtk_widget_grab_focus (GTK_WIDGET (tty));
}

static void
gvt_program_exit (GtkTty	 *tty,
		  const gchar	 *prg_name,
		  gchar		 exit_status,
		  guint		 exit_signal,
		  gpointer	 user_data)
{
  GtkEntry *entry;

  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  g_return_if_fail (user_data != NULL);
  g_return_if_fail (GTK_IS_ENTRY (user_data));

  entry = GTK_ENTRY (user_data);

  gtk_widget_set_sensitive (signal_button, FALSE);

  if (exit_signal)
  {
    gchar *text = "Program `%s' got %s";
    gchar *string;

    string = g_new (gchar, strlen (text) + strlen (prg_name) + strlen (g_strsignal (exit_signal)) + 1);
    sprintf (string, text, prg_name, g_strsignal (exit_signal));
    gtk_label_set (GTK_LABEL (status_label), string);
    g_free (string);
  }
  else
  {
    gchar *text = "Program `%s' exited with status %+d";
    gchar *string;
    
    string = g_new (gchar, strlen (text) + strlen (prg_name) + 10 + 1);
    sprintf (string, text, prg_name, exit_status);
    gtk_label_set (GTK_LABEL (status_label), string);
    g_free (string);
  }

  if (1)
  {
    gchar string[256];

    sprintf (string,
	     "System: %f  User: %f",
	     tty->sys_sec + tty->sys_usec/1000000.0,
	     tty->user_sec + tty->user_usec/1000000.0);
    gtk_label_set (GTK_LABEL (time_label), string);
  }
  
  if (GTK_WIDGET_HAS_FOCUS (tty))
    gtk_widget_grab_focus (GTK_WIDGET (entry));
}

void
gvt_term_set_color_table (GtkTerm       *term,
			  GdkColormap	*cmap,
			  GvtColorEntry *color_table,
			  guint         len,
			  guint		first)
{
  guint i;
  
  g_assert (term && GTK_IS_TERM (term));
  g_assert (cmap);
  g_assert (color_table);
  
  for (i = 0; i < len; i++)
  {
    GdkColor	*dim;
    GdkColor	*bold;
    
    if (!color_table[i].back_col)
    {
      color_table[i].back_col = g_new0 (GdkColor, 1);
      color_table[i].back_col->red = color_table[i].back_val >> 8 & 0xff00;
      color_table[i].back_col->red += color_table[i].back_col->red >> 8;
      color_table[i].back_col->green = color_table[i].back_val & 0xff00;
      color_table[i].back_col->green += color_table[i].back_col->green >> 8;
      color_table[i].back_col->blue = color_table[i].back_val & 0xff;
      color_table[i].back_col->blue += color_table[i].back_col->blue << 8;
      
      if (!gdk_color_alloc (cmap, color_table[i].back_col))
      {
	g_warning ("failed to allocate background color #%06X (%d)",
		   color_table[i].back_val,
		   i);
	g_free (color_table[i].back_col);
	color_table[i].back_col = NULL;
	continue;
      }
    }

    if (!color_table[i].fore_col)
    {
      color_table[i].fore_col = g_new0 (GdkColor, 1);
      color_table[i].fore_col->red = color_table[i].back_val >> 8 & 0xff00;
      color_table[i].fore_col->red += color_table[i].fore_col->red >> 8;
      color_table[i].fore_col->green = color_table[i].back_val & 0xff00;
      color_table[i].fore_col->green += color_table[i].fore_col->green >> 8;
      color_table[i].fore_col->blue = color_table[i].back_val & 0xff;
      color_table[i].fore_col->blue += color_table[i].fore_col->blue << 8;
      
      if (!gdk_color_alloc (cmap, color_table[i].fore_col))
      {
	g_warning ("failed to allocate foreground color #%06X (%d)",
		   color_table[i].fore_val,
		   i);
	g_free (color_table[i].fore_col);
	color_table[i].fore_col = NULL;
	continue;
      }
    }

    if (!color_table[i].dim_col)
    {
      color_table[i].dim_col = g_new0 (GdkColor, 1);
      color_table[i].dim_col->red = color_table[i].back_val >> 8 & 0xff00;
      color_table[i].dim_col->red += color_table[i].dim_col->red >> 8;
      color_table[i].dim_col->green = color_table[i].back_val & 0xff00;
      color_table[i].dim_col->green += color_table[i].dim_col->green >> 8;
      color_table[i].dim_col->blue = color_table[i].back_val & 0xff;
      color_table[i].dim_col->blue += color_table[i].dim_col->blue << 8;
      
      if (!gdk_color_alloc (cmap, color_table[i].dim_col))
      {
	g_warning ("failed to allocate dimmed foreground color #%06X (%d) (using foreground)",
		   color_table[i].dim_val,
		   i);
	g_free (color_table[i].dim_col);
	color_table[i].dim_col = NULL;
	dim = color_table[i].fore_col;
      }
      else
	dim = color_table[i].dim_col;
    }
    else
      dim = color_table[i].dim_col;
    
    if (!color_table[i].bold_col)
    {
      color_table[i].bold_col = g_new0 (GdkColor, 1);
      color_table[i].bold_col->red = color_table[i].back_val >> 8 & 0xff00;
      color_table[i].bold_col->red += color_table[i].bold_col->red >> 8;
      color_table[i].bold_col->green = color_table[i].back_val & 0xff00;
      color_table[i].bold_col->green += color_table[i].bold_col->green >> 8;
      color_table[i].bold_col->blue = color_table[i].back_val & 0xff;
      color_table[i].bold_col->blue += color_table[i].bold_col->blue << 8;
      
      if (!gdk_color_alloc (cmap, color_table[i].bold_col))
      {
	g_warning ("failed to allocate bold foreground color #%06X (%d) (using foregound)",
		   color_table[i].bold_val,
		   i);
	g_free (color_table[i].bold_col);
	color_table[i].bold_col = NULL;
	bold = color_table[i].fore_col;
      }
      else
	bold = color_table[i].bold_col;
    }
    else
      bold = color_table[i].bold_col;
    
    gtk_term_set_color (term,
			first + i,
			color_table[i].back_col,
			color_table[i].fore_col,
			dim,
			bold);
  }
}
