/*  GemVt - GNU Emulator of a Virtual Terminal
 *  Copyright (C) 1997  Tim Janik
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
#include	"gvtgui.h"
#include	"gem-r.xpm"
#include	"gem-g.xpm"
#include	"gem-b.xpm"
#include	<string.h>




/* -- defines --- */
#define	LABEL_PADDING	(5)


/* --- prototypes --- */
static void	gvt_status_bar_gem_state	(GtkWidget    *status_bar,
						 GvtStateType state);
static void	gvt_status_bar_label_state	(GtkWidget    *status_bar);


/* --- functions --- */
GtkWidget*
gvt_status_bar_new (GtkWidget	   *parent,
		    GtkTty	   *tty)
{
  GtkWidget	*status_bar;
  GtkWidget	*hbox_main;
  GtkWidget	*vbox_gems;
  GdkPixmap	*xpm_map;
  GdkBitmap	*bit_map;
  GtkWidget	*pixmap;
  GtkWidget	*label;
  GtkWidget	*hbox_labels;
  GtkWidget	*hbox_leds;
  register guint i;
  
  g_assert (parent != NULL);
  g_assert (GTK_IS_CONTAINER (parent));
  g_assert (tty != NULL);
  g_assert (GTK_IS_TTY (tty));
  
  status_bar = gtk_vbox_new (FALSE, 0);
  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), status_bar, TRUE, TRUE, 0);
  else
    gtk_container_add (GTK_CONTAINER (parent), status_bar);
  gtk_object_set_data (GTK_OBJECT (status_bar), "tty", tty);
  
  hbox_main = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (status_bar), hbox_main, TRUE, TRUE, 3);
  gtk_widget_show (hbox_main);
  
  vbox_gems = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_main), vbox_gems, FALSE, FALSE, 3);
  gtk_widget_show (vbox_gems);
  
  bit_map = NULL;
  xpm_map = gdk_pixmap_create_from_xpm_d (vbox_gems->window,
					  &bit_map,
					  &gtk_widget_get_style (vbox_gems)->bg[GTK_STATE_NORMAL],
					  gem_r_xpm);
  pixmap = gtk_pixmap_new (xpm_map, bit_map);
  gtk_box_pack_start (GTK_BOX (vbox_gems), pixmap, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (status_bar), "gem-red", pixmap);
  
  bit_map = NULL;
  xpm_map = gdk_pixmap_create_from_xpm_d (vbox_gems->window,
					  &bit_map,
					  &gtk_widget_get_style (vbox_gems)->bg[GTK_STATE_NORMAL],
					  gem_g_xpm);
  pixmap = gtk_pixmap_new (xpm_map, bit_map);
  gtk_box_pack_start (GTK_BOX (vbox_gems), pixmap, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (status_bar), "gem-green", pixmap);
  
  bit_map = NULL;
  xpm_map = gdk_pixmap_create_from_xpm_d (vbox_gems->window,
					  &bit_map,
					  &gtk_widget_get_style (vbox_gems)->bg[GTK_STATE_NORMAL],
					  gem_b_xpm);
  pixmap = gtk_pixmap_new (xpm_map, bit_map);
  gtk_box_pack_start (GTK_BOX (vbox_gems), pixmap, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (status_bar), "gem-blue", pixmap);
  
  gvt_status_bar_gem_state (status_bar, GVT_STATE_NONE);
  
  hbox_labels = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox_main), hbox_labels, FALSE, FALSE, 0);
  gtk_widget_show (hbox_labels);
  
  label = gtk_label_new ("-program-");
  gtk_box_pack_start (GTK_BOX (hbox_labels), label, TRUE, TRUE, LABEL_PADDING);
  gtk_widget_show (label);
  gtk_object_set_data (GTK_OBJECT (status_bar), "label-program", label);
  
  label = gtk_label_new ("-status-");
  gtk_box_pack_start (GTK_BOX (hbox_labels), label, TRUE, TRUE, LABEL_PADDING + 6);
  gtk_widget_show (label);
  gtk_object_set_data (GTK_OBJECT (status_bar), "label-status", label);

  label = gtk_label_new ("-time-");
  gtk_box_pack_start (GTK_BOX (hbox_labels), label, TRUE, TRUE, LABEL_PADDING);
  if (GTK_TTY_CLASS (GTK_OBJECT (tty)->klass)->meassure_time)
    gtk_widget_show (label);
  gtk_object_set_data (GTK_OBJECT (status_bar), "label-time", label);
  
  gvt_status_bar_label_state (status_bar);
  
  hbox_leds = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_end (GTK_BOX (hbox_main), hbox_leds, FALSE, FALSE, 0);
  gtk_widget_show (hbox_leds);
  for (i = 0; i < 8; i++)
  {
    GtkWidget *led;
    
    led = gtk_led_new ();
    gtk_box_pack_start (GTK_BOX (hbox_leds), led, TRUE, TRUE, 5);
    gtk_widget_show (led);
    
    gtk_tty_add_update_led (tty, GTK_LED(led), 1 << i);
  }
  
  /* gtk_widget_show (status_bar); */
  
  return status_bar;
}

void
gvt_status_bar_update (GtkWidget      *status_bar)
{
  GtkTty    *tty;

  g_assert (status_bar != NULL);
  g_assert (GTK_IS_CONTAINER (status_bar));

  tty = gtk_object_get_data (GTK_OBJECT (status_bar), "tty");
  g_assert (tty != NULL);
  
  if (gtk_object_get_data (GTK_OBJECT (tty), "program"))
    gvt_status_bar_gem_state (status_bar, tty->pid ? GVT_STATE_RUNNING : GVT_STATE_DEAD);
  else
    gvt_status_bar_gem_state (status_bar, tty->pid ? GVT_STATE_RUNNING : GVT_STATE_NONE);
  
  gvt_status_bar_label_state (status_bar);
}

static void
gvt_status_bar_gem_state (GtkWidget    *status_bar,
			  GvtStateType state)
{
  GtkWidget *gems[4] = { NULL, NULL, NULL };
  GtkWidget *parent;
  guint show = 0;
  register guint i;
  
  gems[0] = gtk_object_get_data (GTK_OBJECT (status_bar), "gem-red");
  gems[1] = gtk_object_get_data (GTK_OBJECT (status_bar), "gem-green");
  gems[2] = gtk_object_get_data (GTK_OBJECT (status_bar), "gem-blue");
  parent = gems[0]->parent;
  g_assert (parent);
  
  switch (state)
  {
  case	GVT_STATE_NONE:
    show = 2;
    break;
    
  case	GVT_STATE_RUNNING:
    show = 1;
    break;
    
  case	GVT_STATE_DEAD:
    show = 0;
    break;
    
  default:
    g_assert_not_reached ();
  }
  
  gtk_container_block_resize (GTK_CONTAINER (parent));
  
  for (i = 0; i < 3; i++)
  {
    if (i == show)
      gtk_widget_show (gems[i]);
    else
      gtk_widget_hide (gems[i]);
  }
  
  gtk_container_unblock_resize (GTK_CONTAINER (parent));
}

static void
gvt_status_bar_label_state (GtkWidget *status_bar)
{
#define	SAVE_BUFFER_LENGTH	(64)
  GtkTty    *tty;
  GtkWidget *parent;
  GtkWidget *label_program;
  GtkWidget *label_status;
  GtkWidget *label_time;
  gchar *program;
  gchar	state[SAVE_BUFFER_LENGTH];
  gchar sys_time[SAVE_BUFFER_LENGTH];
  gchar user_time[SAVE_BUFFER_LENGTH];
  gchar *buffer;
  register guint i;
  
  tty = gtk_object_get_data (GTK_OBJECT (status_bar), "tty");
  g_assert (tty != NULL);
  label_program = gtk_object_get_data (GTK_OBJECT (status_bar), "label-program");
  g_assert (label_program != NULL);
  label_status = gtk_object_get_data (GTK_OBJECT (status_bar), "label-status");
  g_assert (label_status != NULL);
  label_time = gtk_object_get_data (GTK_OBJECT (status_bar), "label-time");
  g_assert (label_time != NULL);
  parent = label_program->parent;
  g_assert (parent);
  
  program = gtk_object_get_data (GTK_OBJECT (tty), "program");

  if (!program)
  {
    strcpy (state, "None");
    sys_time[0] = '0';
    sys_time[1] = 0;
    user_time[0] = '0';
    user_time[1] = 0;
  }
  else
  {
    if (tty->pid)
    {
      sprintf (state, "running (%d)", tty->pid);
      sys_time[0] = '-';
      sys_time[1] = 0;
      user_time[0] = '-';
      user_time[1] = 0;
    }
    else
    {
      if (tty->exit_signal)
	strcpy (state, g_strsignal (tty->exit_signal));
      else
	sprintf (state, "Exit [%+d]", tty->exit_status);
      
      sprintf (sys_time, "%f", tty->sys_sec + tty->sys_usec/1000000.0);
      for (i = strlen (sys_time) - 1; sys_time[i] == '0'; i--)
	sys_time[i] = 0;
      sprintf (user_time, "%f", tty->user_sec + tty->user_usec/1000000.0);
      for (i = strlen (user_time) - 1; user_time[i] == '0'; i--)
	user_time[i] = 0;
    }
  }
  
  gtk_container_block_resize (GTK_CONTAINER (parent));
    
  if (program)
  {
    register guint l;

    l = strlen (program);
    if (l > 32)
      buffer = g_strconcat ("Program: .../", program + 32 - l, NULL);
    else
      buffer = g_strconcat ("Program: ", program, NULL);
  }
  else
    buffer = g_strconcat ("Program: ", "None", NULL);
  gtk_label_set (GTK_LABEL (label_program), buffer);
  g_free (buffer);
  
  buffer = g_strconcat ("Status: ", state, NULL);
  gtk_label_set (GTK_LABEL (label_status), buffer);
  g_free (buffer);
  
  buffer = g_strconcat ("System: ", sys_time, "	 User: ", user_time, NULL);
  gtk_label_set (GTK_LABEL (label_time), buffer);
  g_free (buffer);

  gtk_container_unblock_resize (GTK_CONTAINER (parent));
}
