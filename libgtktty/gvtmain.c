/*  GemVt - GNU Emulator of a Virtual Terminal
 *  Copyright (C) 1997	Tim Janik
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
#include	"config.h"
#include	"gtktty.h"
#include	"gtkled.h"
#include	"gvtgui.h"
#include	"gvtconf.h"
#include	"gemvt.xpm"
#include	<gtk/gtk.h>
#include	<signal.h>
#include	<unistd.h>
#include	<string.h>

#ifdef	HAVE_GNOME
#  include	<gnome.h>
#endif	HAVE_GNOME

#ifdef	HAVE_LIBGLE
#  include	<gle/gle.h>
#endif	/* HAVE_LIBGLE */


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

enum
{
  GVT_VOIDLINE,
  GVT_ONLINE,
  GVT_OFFLINE
};

/* --- prototypes --- */
static	RETSIGTYPE gvt_signal_handler	(int		sig_num);
static  gint    gvt_tty_key_press       (GtkTty         *tty,
					 const gchar    *char_code,
					 guint          length,
					 guint          keyval,
					 guint          key_state,
					 gpointer       data);
static gint	gvt_tty_button_press	(GtkWidget	*widget,
					 GdkEventButton	*event);
static	void	gvt_execute		(GtkWidget	*widget,
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
static	void	gtk_menu_position_func	(GtkMenu	*menu,
					 gint		*x,
					 gint		*y,
					 gpointer	user_data);
static void	gvt_menu_adjust		(GtkMenu	*menu);
static	void	gvt_menus_setup		(void);
static	void	gvt_menus_shutdown	(void);



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
static GtkMenu		*menu_1 = NULL;
static GtkMenu		*menu_2 = NULL;
static GtkMenu		*menu_3 = NULL;
static GtkTty		*menu_tty;
static gint		menu_pos_x, menu_pos_y;
static GvtConfig	config =
{
  TRUE	/* have_status_bar */,
  FALSE	/* prefix_dash */,
  NULL	/* strings */,
};


/* --- main() --- */
int
main	(int	argc,
	 char	*argv[])
{
  gchar	*home_dir;
  gchar	*rc_file;
  guchar	*string;
  GtkWidget	*window;
  GtkWidget	*main_vbox;
  GtkWidget	*tty_vbox;
  GtkWidget	*status_hbox;
  GtkWidget	*tty_hbox;
  GtkWidget	*status_bar;
  GtkWidget	*label;
  GtkWidget	*tty;
  GtkWidget	*exec_hbox;
  GtkWidget	*entry;
  guint	id;
  gint		exit_status;
  

  prg_name = g_strdup (argv[0]);
  
  /* parse arguments
   */
  exit_status = gvt_config_args (&config, stderr, argc, argv);
  if (exit_status > -129)
    return exit_status;

  
  /* Gtk+/GNOME/GLE initialization
   */
#ifdef	HAVE_GNOME
  gnome_init(&argc, &argv);
#else	/* !HAVE_GNOME */
  gtk_init (&argc, &argv);
#endif	/* !HAVE_GNOME */

#ifdef	HAVE_LIBGLE
  gle_init (&argc, &argv);
#endif	/* HAVE_LIBGLE */

  
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

  /* printf("%s\n", rc_file);
   */
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
  /* printf("%s\n", rc_file);
   */
  /* FIXME: invoke rc-parser here */
  g_free (rc_file);

  
  /* catch system signals
   */
  gvt_signal_handler (0);


  /* build gui
   */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "GemVt");
  gtk_window_set_policy (GTK_WINDOW (window),
			 TRUE,
			 TRUE,
			 TRUE);
  gtk_signal_connect_object (GTK_OBJECT (window),
			     "delete_event",
			     GTK_SIGNAL_FUNC (gtk_main_quit),
			     NULL);
  
  main_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), 2);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
  gtk_widget_show (main_vbox);

  tty_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (tty_hbox), 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), tty_hbox, TRUE, TRUE, 0);
  gtk_widget_show (tty_hbox);

  tty_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (tty_vbox), 0);
  gtk_box_pack_start (GTK_BOX (tty_hbox), tty_vbox, FALSE, FALSE, 0);
  gtk_widget_show (tty_vbox);
  
  status_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (status_hbox), 0);
  gtk_box_pack_start (GTK_BOX (tty_vbox), status_hbox, FALSE, TRUE, 0);
  gtk_widget_show (status_hbox);

  string =
    "Hi, this is GemVt bothering you ;)\n\r"
    "Enter `-bash' or something the like and\n\r"
    "Have Fun!!!\n\r";
  
  tty = gtk_tty_new (80, 25, 99);
  gtk_tty_put_out (GTK_TTY (tty), string, strlen (string));
  gtk_box_pack_start (GTK_BOX (tty_vbox), tty, TRUE, TRUE, 0);
  gtk_widget_show (tty);
  id = gtk_signal_connect (GTK_OBJECT (tty),
			   "key_press",
			   GTK_SIGNAL_FUNC (gvt_tty_key_press),
			   NULL);
  gtk_object_set_data (GTK_OBJECT (tty), "key_press_hid", (gpointer) id);
  gtk_signal_connect (GTK_OBJECT (tty),
		      "button_press_event",
		      GTK_SIGNAL_FUNC (gvt_tty_button_press),
		      NULL);
  gvt_term_set_color_table (GTK_TERM (tty),
			    /* gdk_colormap_get_system (), */
			    gtk_widget_get_colormap (GTK_WIDGET (tty)),
			    color_table,
			    sizeof (color_table) / sizeof (GvtColorEntry),
			    0);
  
  status_bar = gvt_status_bar_new (status_hbox, GTK_TTY (tty));
  if (config.have_status_bar)
    gtk_widget_show (status_bar);
  gtk_object_set_data (GTK_OBJECT (tty), "status-bar", status_bar);
  
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
  gtk_widget_grab_focus (GTK_WIDGET (entry));
  
  gtk_widget_show (window);

  gvt_menus_setup ();


  /* gtk's main loop
   */
  gtk_main ();

  gtk_widget_destroy (window);
  gvt_menus_shutdown ();

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
  
  if (sig_num > 0)
   {
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

static void
gvt_program_exec (GtkTty	 *tty,
		  const gchar	 *prg_name,
		  gchar * const	 argv[],
		  gchar * const	 envp[],
		  gpointer	 user_data)
{
  GtkWidget *status_bar;
  GtkEntry *entry;
  register guint id;

  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  g_return_if_fail (user_data != NULL);
  g_return_if_fail (GTK_IS_ENTRY (user_data));

  entry = GTK_ENTRY (user_data);
  status_bar = gtk_object_get_data (GTK_OBJECT (tty), "status-bar");
  g_assert (status_bar != NULL);

  id = (gint) gtk_object_get_data (GTK_OBJECT (tty), "key_press_hid");
  if (id)
  {
    gtk_signal_disconnect (GTK_OBJECT (tty), id);
    gtk_object_set_data (GTK_OBJECT (tty), "key_press_hid", (gpointer) 0);
  }

  gtk_object_set_data (GTK_OBJECT (tty), "program", (gpointer) prg_name);

  gvt_status_bar_update (status_bar);

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
  GtkWidget *status_bar;
  register guint id;

  g_return_if_fail (tty != NULL);
  g_return_if_fail (GTK_IS_TTY (tty));
  g_return_if_fail (user_data != NULL);
  g_return_if_fail (GTK_IS_ENTRY (user_data));

  entry = GTK_ENTRY (user_data);
  status_bar = gtk_object_get_data (GTK_OBJECT (tty), "status-bar");
  g_assert (status_bar != NULL);

  id = (gint) gtk_object_get_data (GTK_OBJECT (tty), "key_press_hid");
  g_assert (id == 0);
  id = gtk_signal_connect (GTK_OBJECT (tty),
			   "key_press",
			   GTK_SIGNAL_FUNC (gvt_tty_key_press),
			   NULL);
  gtk_object_set_data (GTK_OBJECT (tty), "key_press_hid", (gpointer) id);

  gvt_status_bar_update (status_bar);

  gtk_object_set_data (GTK_OBJECT (tty), "program", NULL);

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
      color_table[i].fore_col->red = color_table[i].fore_val >> 8 & 0xff00;
      color_table[i].fore_col->red += color_table[i].fore_col->red >> 8;
      color_table[i].fore_col->green = color_table[i].fore_val & 0xff00;
      color_table[i].fore_col->green += color_table[i].fore_col->green >> 8;
      color_table[i].fore_col->blue = color_table[i].fore_val & 0xff;
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
      color_table[i].dim_col->red = color_table[i].dim_val >> 8 & 0xff00;
      color_table[i].dim_col->red += color_table[i].dim_col->red >> 8;
      color_table[i].dim_col->green = color_table[i].dim_val & 0xff00;
      color_table[i].dim_col->green += color_table[i].dim_col->green >> 8;
      color_table[i].dim_col->blue = color_table[i].dim_val & 0xff;
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
      color_table[i].bold_col->red = color_table[i].bold_val >> 8 & 0xff00;
      color_table[i].bold_col->red += color_table[i].bold_col->red >> 8;
      color_table[i].bold_col->green = color_table[i].bold_val & 0xff00;
      color_table[i].bold_col->green += color_table[i].bold_col->green >> 8;
      color_table[i].bold_col->blue = color_table[i].bold_val & 0xff;
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

static gint
gvt_tty_key_press (GtkTty         *tty,
		   const gchar    *char_code,
		   guint          length,
		   guint          keyval,
		   guint          key_state,
		   gpointer       data)
{
  GtkWidget *status_bar;
  register guint id;

  /* return wether we handled the key press:
   * FALSE) let GtkTty handle the keypress,
   * TRUE)  we do something like gtk_tty_put_in (tty, char_code, length) on
   *        our own.
   */

  status_bar = gtk_object_get_data (GTK_OBJECT (tty), "status-bar");

  gvt_status_bar_update (status_bar);

  id = (gint) gtk_object_get_data (GTK_OBJECT (tty), "key_press_hid");
  if (id)
  {
    gtk_signal_disconnect (GTK_OBJECT (tty), id);
    gtk_object_set_data (GTK_OBJECT (tty), "key_press_hid", (gpointer) 0);
  }

  return FALSE;
}

static gint
gvt_tty_button_press (GtkWidget      *widget,
		      GdkEventButton *event)
{
  GtkTty *tty;

  /* 1) return wether we handled the event
   * 2) if we handled it we need to prevent gtktty region selection,
   *    therefor we stop the emission of the signal
   */

  tty = GTK_TTY (widget);

  if ((event->state & (GDK_SHIFT_MASK |
		      GDK_LOCK_MASK |
		      GDK_CONTROL_MASK |
		      GDK_MOD1_MASK)) ==
      GDK_CONTROL_MASK)
  {
    switch (event->button)
    {
    case  1:
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");
      gdk_window_get_pointer (NULL, &menu_pos_x, &menu_pos_y, NULL);
      menu_tty = tty;
      gvt_menu_adjust (menu_1);
      gtk_menu_popup (menu_1,
		      NULL, NULL,
		      gtk_menu_position_func, NULL,
		      1, event->time);
      
      return TRUE;
    }
  }

  return FALSE;
}

static void
gtk_menu_position_func (GtkMenu        *menu,
			gint           *x,
			gint           *y,
			gpointer       user_data)
{
  *x = menu_pos_x;
  *y = menu_pos_y;
}

static void
gvt_menu_toggle_status_bar (GtkWidget *widget,
			    gpointer  data)
{
  GtkWidget *status_bar;

  status_bar = gtk_object_get_data (GTK_OBJECT (menu_tty), "status-bar");
  g_assert (status_bar != NULL);

  if (GTK_WIDGET_VISIBLE (status_bar))
    gtk_widget_hide (status_bar);
  else
    gtk_widget_show (status_bar);
}

static void
gvt_menu_tty_redraw (GtkWidget *widget,
		     gpointer  data)
{
  gtk_widget_draw (GTK_WIDGET (menu_tty), NULL);
}

static void
gvt_menu_send_signal (GtkWidget *widget,
		      gpointer  data)
{
  g_assert (menu_tty != NULL);
  g_assert (menu_tty->pid > 0);

  kill (menu_tty->pid, (gint) data);
}


static void
gvt_menu_adjust (GtkMenu *menu)
{
  GList *list, *free_list;

  g_assert (menu != NULL);

  list = free_list = gtk_container_children (GTK_CONTAINER (menu));

  while (list)
  {
    gint ac;

    ac = (gint) gtk_object_get_data (GTK_OBJECT (list->data), "activate");

    switch (ac)
    {
    case  GVT_ONLINE:
      gtk_widget_set_sensitive (GTK_WIDGET (list->data), menu_tty->pid > 0);
      break;

    case  GVT_OFFLINE:
      gtk_widget_set_sensitive (GTK_WIDGET (list->data), menu_tty->pid == 0);
      break;
    }

    list = list->next;
  }

  g_list_free (free_list);
}

static void
gvt_menus_setup (void)
{
  GtkWidget *menu;

  if (!menu_1)
  {
    GtkWidget *item;

    menu = gtk_widget_new (gtk_menu_get_type (),
			   "GtkObject::signal::destroy", gtk_widget_destroyed, &menu_1,
			   NULL);
    menu_1 = GTK_MENU (menu);
    item = gtk_menu_item_new_with_label ("Main Options");
    gtk_widget_set_sensitive (item, FALSE);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new ();
    gtk_widget_set_sensitive (item, FALSE);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("Redraw");
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_tty_redraw),
		       NULL);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("Toggle Status Bar");
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_toggle_status_bar),
		       NULL);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new ();
    gtk_widget_set_sensitive (item, FALSE);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGSTOP");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGSTOP);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGCONT");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGCONT);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGINT");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGINT);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGHUP");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGHUP);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGTERM");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGTERM);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("SIGKILL");
    gtk_object_set_data (GTK_OBJECT (item), "activate", (gpointer) GVT_ONLINE);
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gvt_menu_send_signal),
		       (gpointer) SIGKILL);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new ();
    gtk_widget_set_sensitive (item, FALSE);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
    item = gtk_menu_item_new_with_label ("Exit");
    gtk_signal_connect(GTK_OBJECT (item), "activate",
		       GTK_SIGNAL_FUNC (gtk_main_quit),
		       NULL);
    gtk_widget_show (item);
    gtk_menu_append (GTK_MENU (menu), item);
  }

  if (!menu_2)
  {
    menu = gtk_widget_new (gtk_menu_get_type (),
			   "GtkObject::signal::destroy", gtk_widget_destroyed, &menu_2,
			   NULL);
    menu_2 = GTK_MENU (menu);
  }

  if (!menu_3)
  {
    menu = gtk_widget_new (gtk_menu_get_type (),
			   "GtkObject::signal::destroy", gtk_widget_destroyed, &menu_3,
			   NULL);
    menu_3 = GTK_MENU (menu);
  }
}

static void
gvt_menus_shutdown (void)
{
  if (menu_1)
    gtk_object_sink (GTK_OBJECT (menu_1));
  if (menu_2)
    gtk_object_sink (GTK_OBJECT (menu_2));
  if (menu_3)
    gtk_object_sink (GTK_OBJECT (menu_3));
}
