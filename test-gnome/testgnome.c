/* testGNOME - program similar to testgtk which shows gnome lib functions.
 * 
 * Author : Richard Hestilow <hestgray@ionet.net>
 *
 * Copyright (C) 1998 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <gdk_imlib.h>

#include "testgnome.h"
#include "bomb.xpm"

gchar *authors[] = {
	"Richard Hestilow",
	"Federico Mena",
	NULL
};

void delete_event (GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(data);
}

void create_about (GtkWidget *widget, gpointer data)
{
        GtkWidget *about;

        about = gnome_about_new("GNOME Test Program", "0.01",
                                "(C) 1998 The Free Software Foundation",
                                authors,
                                "Program to display GNOME functions.",
                                NULL);
        gtk_widget_show (about);
}

void create_date_edit ()
{
	GtkWidget *datedit;
	GtkWidget *win;
	time_t curtime = time(NULL);

	datedit = gnome_date_edit_new(curtime,1,1);
	win = create_newwin(TRUE,"testGNOME","Date Edit");
	gnome_app_set_contents(GNOME_APP(win),datedit);
	gtk_widget_show(datedit);
	gtk_widget_show(win);
}

void quit_test (GtkWidget *widget, gpointer data)
{
        gtk_main_quit ();
}

void window_close (GtkWidget *widget, gpointer data)
{
        gtk_widget_destroy (GTK_WIDGET(data));
}

GnomeUIInfo file_menu[] = {
        { GNOME_APP_UI_ITEM, "Test", NULL, gtk_main_quit, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'A',
	  GDK_SHIFT_MASK, NULL },
        { GNOME_APP_UI_ITEM, "Exit", NULL, quit_test, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'X',
	  GDK_CONTROL_MASK, NULL },
        { GNOME_APP_UI_ITEM, "Close", NULL, window_close, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'X',
	  GDK_CONTROL_MASK, NULL },
        { GNOME_APP_UI_ENDOFINFO }
};
GnomeUIInfo help_menu[] = {
        { GNOME_APP_UI_HELP, NULL, NULL, NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
        { GNOME_APP_UI_ITEM, "About...", NULL, create_about, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0,
	  NULL },
        { GNOME_APP_UI_ENDOFINFO }
};
GnomeUIInfo main_menu[] = {
        { GNOME_APP_UI_SUBTREE, ("File"), NULL, file_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
        { GNOME_APP_UI_SUBTREE, ("Help"), NULL, help_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
        { GNOME_APP_UI_ENDOFINFO }
};

void color_changed_cb( GnomeColorSelector *widget, gchar **color )
{
        char *tmp;
        int r,g,b;

        tmp = malloc(24);
        if( !tmp )
        {
                g_warning( "Can't allocate memory for color\n" );
                return;
        }
        gnome_color_selector_get_color_int(
                widget, &r, &g, &b, 255 );

        sprintf( tmp, "#%02x%02x%02x", r, g, b );
        *color = tmp;
}

GtkWidget *create_newwin(gboolean normal, gchar *appname, gchar *title)
{
	GtkWidget *app;

	app = gnome_app_new (appname,title);
	if (!normal)
        {
                gtk_signal_connect(GTK_OBJECT(app), "delete_event",
				   GTK_SIGNAL_FUNC(quit_test), NULL);
        };

	gnome_app_create_menus_with_data (GNOME_APP(app), main_menu, app);
	gtk_menu_item_right_justify(GTK_MENU_ITEM(main_menu[1].widget));
	return GTK_WIDGET(app);
}

void create_calendar()
{
	GtkWidget *app;
	GtkWidget *cal;
	app=create_newwin(TRUE,"testGNOME","calendar");
	cal=gtk_calendar_new();
	gnome_app_set_contents(GNOME_APP(app),cal);
	gtk_widget_show(cal);
	gtk_widget_show(app);
}

void create_calc()
{
	GtkWidget *app,*calc;
	app = create_newwin(TRUE,"testGNOME","Calculator");
	calc = gnome_calculator_new();
	gnome_app_set_contents(GNOME_APP(app),calc);
	gtk_widget_show(calc);
	gtk_widget_show(app);
}

void create_clock()
{
	GtkWidget *app;
	GtkWidget *clock;

	app=create_newwin(TRUE,"testGNOME","Clock");
	clock=gtk_clock_new(0);
	gnome_app_set_contents(GNOME_APP(app),clock);
	gtk_clock_set_seconds(GTK_CLOCK(clock),0);
/* If I start the clock, and then close the window, it will sigsegv.
   FIXME
   gtk_clock_start(GTK_CLOCK(clock));
*/
	gtk_widget_show(clock);
	gtk_widget_show(app);
}

void create_colorsel(GtkWidget *widget, gpointer data)
{
	GnomeColorSelector *colorsel;
	GtkWidget *colbutton;
	GtkWidget *app;
	gchar *dummy;
	app = create_newwin(TRUE,"testGNOME","Color Selection");
	colorsel = gnome_color_selector_new( (SetColorFunc)color_changed_cb,
					     &dummy);
	colbutton = gnome_color_selector_get_button( colorsel );
	gnome_app_set_contents(GNOME_APP(app),colbutton);
	gtk_widget_show(colbutton);
	gtk_widget_show(app);
}

/* 
 * GnomeDialog
 */

enum {
  modal, 
  just_hide,
  click_closes, 
  editable_enters
};

void toggle_boolean(GtkWidget * toggle, gboolean * setme)
{
  gboolean current = *setme;
  *setme = !current;
}

void block_until_clicked(GtkWidget *dialog)
{
  gint button;
  button = gnome_dialog_run(GNOME_DIALOG(dialog));
  g_print("Modal run ended, button %d clicked\n", button);
}

void set_to_null(GtkWidget * ignore, GnomeDialog ** d)
{
  *d = NULL;
}

void create_test_dialog (GtkWidget * ignored, gboolean * settings)
{
  static GnomeDialog * dialog = NULL;
  GtkWidget * entry;
  GtkWidget * button;

  if (dialog) {
    g_print("Previous dialog was not destroyed, destroying...\n");
    gtk_widget_destroy(GTK_WIDGET(dialog));
    dialog = NULL;
  }

  dialog = GNOME_DIALOG(gnome_dialog_new( "A Test Dialog", 
					  GNOME_STOCK_BUTTON_OK,
					  "Not a stock button",
					  GNOME_STOCK_BUTTON_CANCEL, NULL ));
  
  entry = gtk_entry_new();
  button = gtk_button_new_with_label("gnome_dialog_run");
  
  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		     GTK_SIGNAL_FUNC(block_until_clicked), 
		     dialog);
  
  gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
		     GTK_SIGNAL_FUNC(set_to_null),
		     &dialog);
  
  gtk_box_pack_start(GTK_BOX(dialog->vbox), entry, TRUE, TRUE, GNOME_PAD);
  gtk_box_pack_start(GTK_BOX(dialog->vbox), button, FALSE, FALSE, GNOME_PAD);
  
  if (settings[modal]) {
    g_print("Modal... ");
    gnome_dialog_set_modal(dialog);
  }
  if (settings[just_hide]) {
    g_print("Close hides... ");
    gnome_dialog_close_hides(dialog, TRUE);
  }
  if (settings[click_closes]) {
    g_print("Click closes... ");
    gnome_dialog_set_close(dialog, TRUE);
  }
  if (settings[editable_enters]) {
    g_print("Editable enters... ");
    gnome_dialog_editable_enters(dialog, GTK_EDITABLE(entry));
  }
  g_print("\n");

  gtk_widget_show_all(GTK_WIDGET(dialog));
}

void create_dialog()
{
  GtkWidget * app;
  GtkWidget * vbox;
  GtkWidget * hbox;
  GtkWidget * toggle;
  GtkWidget * button;
  static gboolean settings[4] = {FALSE, TRUE, FALSE, TRUE};

  app = create_newwin(TRUE,"testGNOME","Dialog Boxes");
  vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  hbox = gtk_hbox_new(FALSE, GNOME_PAD);
  
  gnome_app_set_contents(GNOME_APP(app),vbox);
  
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, GNOME_PAD);

  toggle = gtk_toggle_button_new_with_label("Modal");
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), settings[modal]);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled", 
		     GTK_SIGNAL_FUNC(toggle_boolean), &settings[modal]);
  gtk_box_pack_start(GTK_BOX(hbox), toggle, FALSE, FALSE, GNOME_PAD);

  toggle = gtk_toggle_button_new_with_label("Hide don't destroy");
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), settings[just_hide]);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled", 
		     GTK_SIGNAL_FUNC(toggle_boolean), &settings[just_hide]);
  gtk_box_pack_start(GTK_BOX(hbox), toggle, FALSE, FALSE, GNOME_PAD);

  toggle = gtk_toggle_button_new_with_label("Close on click");
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), settings[click_closes]);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled", 
		     GTK_SIGNAL_FUNC(toggle_boolean), &settings[click_closes]);
  gtk_box_pack_start(GTK_BOX(hbox), toggle, FALSE, FALSE, GNOME_PAD);

  toggle = gtk_toggle_button_new_with_label("Editable enters");
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(toggle), settings[editable_enters]);
  gtk_signal_connect(GTK_OBJECT(toggle), "toggled", 
		     GTK_SIGNAL_FUNC(toggle_boolean), &settings[editable_enters]);
  gtk_box_pack_start(GTK_BOX(hbox), toggle, FALSE, FALSE, GNOME_PAD);

  button = gtk_button_new_with_label("Create the dialog");
  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		     GTK_SIGNAL_FUNC(create_test_dialog), 
		     &settings[0]);

  gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, FALSE, GNOME_PAD);

  gtk_widget_show_all(app);
}

void create_file_entry()
{
	GtkWidget *app;
	GtkWidget *entry;

	app = create_newwin(TRUE,"testGNOME","File Entry");
	entry = gnome_file_entry_new("Foo","Bar");
	gnome_app_set_contents(GNOME_APP(app),entry);
	gtk_widget_show(entry);
	gtk_widget_show(app);
}

void create_number_entry()
{
	GtkWidget *app;
	GtkWidget *entry;

	app = create_newwin(TRUE,"testGNOME","Number Entry");
	entry = gnome_number_entry_new("Foo","Calculator");
	gnome_app_set_contents(GNOME_APP(app),entry);
	gtk_widget_show(entry);
	gtk_widget_show(app);
}
void create_font_sel()
{
	GtkWidget *fontsel;

	fontsel = gnome_font_selector_new();
	gtk_widget_show(fontsel);
}

void create_icon_list()
{
GtkWidget *app;
GtkWidget *iconlist;
GdkImlibImage *pix;
app = create_newwin(TRUE,"testGNOME","Icon List");

iconlist = gnome_icon_list_new();

pix = gdk_imlib_create_image_from_xpm_data((gchar **)bomb_xpm);
gdk_imlib_render (pix, pix->rgb_width, pix->rgb_height);

gnome_icon_list_append_imlib(GNOME_ICON_LIST(iconlist), pix, "Foo");
gnome_icon_list_append_imlib(GNOME_ICON_LIST(iconlist), pix, "Bar");
gnome_icon_list_append_imlib(GNOME_ICON_LIST(iconlist), pix, "LaLa");

gnome_app_set_contents(GNOME_APP(app),iconlist);
gtk_widget_show(iconlist);
gtk_widget_show(app);
}


static void
create_lamp_update(GtkAdjustment *adj, GnomeLamp *lamp)
{
	GdkColor c;
	
	c.red = (adj->value / 100) * 0xf000;
	c.green = (1.0 - (adj->value / 100)) * 0xf000;
	c.blue = (c.red < 0x8000) ? c.red + 0x8000 : c.green + 0x8000;
	gnome_lamp_set_color(lamp, &c);
}

static void
create_lamp(void)
{
	GtkWidget *w, *lamp, *vbox, *hbox, *scale, *app;
	GtkAdjustment *adj;

	app = create_newwin(TRUE, "testGNOME", "Lamp/Beacon");
	vbox = gtk_vbox_new(0, FALSE);
	hbox = gtk_hbox_new(0, FALSE);
	gtk_widget_show(hbox);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);
	lamp = gnome_lamp_new();
	gtk_box_pack_start_defaults(GTK_BOX(hbox), lamp);
	gtk_widget_show(lamp);
	w = gnome_lamp_new_with_type(GNOME_LAMP_INPUT);
	gtk_widget_show(w);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), w);
	w = gnome_lamp_new();
	gnome_lamp_set_sequence(GNOME_LAMP(w), "RRRYYY");
	gtk_widget_show(w);
	gtk_box_pack_start_defaults(GTK_BOX(hbox), w);
	adj = (GtkAdjustment *)gtk_adjustment_new(0.0, 0.0, 110.0,
						  1.0, 10.0, 10.0);
	create_lamp_update(adj, GNOME_LAMP(lamp));
	gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
			   (GtkSignalFunc)create_lamp_update, lamp);
	scale = gtk_hscale_new(adj);
	gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), scale);
	gtk_widget_show(scale);
	gtk_widget_show(vbox);
	gnome_app_set_contents(GNOME_APP(app), vbox);
	gnome_lamp_set_window_type(GTK_WINDOW(app), GNOME_LAMP_INPUT);
	gtk_widget_show(app);
}

void create_less()
{
	GtkWidget *app;
	GtkWidget *less;

	app = create_newwin(TRUE,"testGNOME","Less");
	less = gnome_less_new();
	gnome_app_set_contents(GNOME_APP(app),less);
	gnome_less_set_fixed_font(GNOME_LESS(less), TRUE);
	gtk_widget_show(less);
	gtk_widget_show(app);
	gnome_less_show_command(GNOME_LESS(less),"fortune");
}

void create_pixmap()
{
	GtkWidget *app;
	GdkImlibImage *pix;
	GtkWidget *pixmap;
	app = create_newwin(TRUE,"testGNOME","Pixmap");
	pix = gdk_imlib_create_image_from_xpm_data((gchar **)bomb_xpm);
	gdk_imlib_render (pix, pix->rgb_width, pix->rgb_height);
	pixmap = gtk_pixmap_new(pix->pixmap,pix->shape_mask);
		
	gnome_app_set_contents(GNOME_APP(app),pixmap);
	gtk_widget_show(pixmap);
	gtk_widget_show(app);
}

void create_property_box()
{

/* this is broken, I dunno why. FIXME 
   GtkWidget *vbox;
   GtkWidget *label;
   GtkWidget *check;
   GtkWidget *propbox;

   vbox=gtk_vbox_new(GNOME_PAD,FALSE);
   check=gtk_check_button_new_with_label("Option 1");
   gtk_box_pack_start(GTK_BOX(vbox),check,FALSE,FALSE,10);
   check=gtk_check_button_new_with_label("Another Option");
   gtk_box_pack_start(GTK_BOX(vbox),check,FALSE,FALSE,GNOME_PAD);


   propbox=gnome_property_box_new();
   label=gtk_label_new("foo");

   gnome_property_box_append_page(GNOME_PROPERTY_BOX(propbox),gtk_label_new("Bar"),label);
   gtk_widget_show_all(propbox); */
}

/* gnome-app-util */

void
make_entry_hbox(GtkBox * box,
		gchar * buttontext, gchar * entrydefault, 
		GtkSignalFunc callback, gpointer entrydata)
{
  GtkWidget * hbox;
  GtkWidget * entry;
  GtkWidget * button;

  hbox = gtk_hbox_new(TRUE, GNOME_PAD);
  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), entrydefault);
  gtk_object_set_user_data(GTK_OBJECT(entry), entrydata);
  gtk_box_pack_end(GTK_BOX(hbox), entry, TRUE, TRUE, GNOME_PAD);
  button = gtk_button_new_with_label(buttontext);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, GNOME_PAD);
  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		     callback, entry);

  gtk_box_pack_start(box, hbox, TRUE, TRUE, GNOME_PAD);
}

void
make_button_hbox(GtkBox * box, gchar * buttontext,
		 GtkSignalFunc callback, gpointer data)
{
  GtkWidget * hbox;
  GtkWidget * entry;
  GtkWidget * button;

  hbox = gtk_hbox_new(TRUE, GNOME_PAD);
  button = gtk_button_new_with_label(buttontext);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, GNOME_PAD);
  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		     callback, data);

  gtk_box_pack_start(box, hbox, TRUE, TRUE, GNOME_PAD);
}

void
message_cb(GtkWidget * button, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_app_message(app, gtk_entry_get_text(e));
}

void 
flash_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_app_flash(app, gtk_entry_get_text(e));
}

void 
error_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_app_error(app, gtk_entry_get_text(e));
}

void 
warning_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_app_warning(app, gtk_entry_get_text(e));
}

void
reply_cb(gint reply, gpointer thedata)
{
  gchar * data = (gchar *)thedata;
  gchar * s = NULL;
  if (reply == GNOME_YES) {
    s = g_copy_strings(_("The user chose Yes/OK with data:\n"),
		       data, NULL);
  }
  else if (reply == GNOME_NO) {
    s = g_copy_strings(_("The user chose No/Cancel with data:\n"),
		       data, NULL);
  }

  if (s) {
    g_print(s);
    g_print("\n");
    gnome_ok_dialog(s); 
    g_free(s);
  }
  else {
    gnome_error_dialog(_("Weird number in reply callback"));
  }
}

static gchar * data_string = "A test data string";

void
question_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_app_question(app, gtk_entry_get_text(e),
		     reply_cb, data_string);
}

void
question_modal_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_app_question_modal(app, gtk_entry_get_text(e),
			   reply_cb, data_string);
}
 
void
ok_cancel_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_app_ok_cancel(app, gtk_entry_get_text(e),
		      reply_cb, data_string);
}

void
ok_cancel_modal_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_app_ok_cancel_modal(app, gtk_entry_get_text(e),
			    reply_cb, data_string);
}


static void
string_cb(gchar * string, gpointer data)
{
  gchar * s = g_copy_strings("Got string \"", string, "\" and data \"",
			     data, "\"", NULL);
  g_free(string);
  g_print(s);
  g_print("\n");
  gnome_ok_dialog(s);
  g_free(s);
}

void
request_string_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_app_request_string(app, gtk_entry_get_text(e),
			   string_cb, data_string);
}

gdouble 
percent_cb(gpointer ignore) 
{
  static gdouble progress = 0.0;
  progress += 0.05;
  if (progress > 1.0) progress = 0.0;
  return progress;
}

void
cancel_cb(gpointer ignore)
{
  gnome_ok_dialog("Progress cancelled!");
}

void 
stop_progress_cb(GnomeDialog * d, gint button, GnomeAppProgressKey key)
{
  gnome_app_progress_done(key);
}

void
progress_timeout_cb(GtkWidget * b, GnomeApp * app)
{
  GtkWidget * dialog;
  GnomeAppProgressKey key;

  dialog = gnome_dialog_new("Progress Timeout Test", "Stop test", NULL);
  gnome_dialog_set_close(GNOME_DIALOG(dialog), TRUE);

  key = gnome_app_progress_timeout(app, "Progress!", 200,
				   percent_cb,
				   cancel_cb,
				   NULL);

  gtk_signal_connect(GTK_OBJECT(dialog), "clicked", 
		     GTK_SIGNAL_FUNC(stop_progress_cb), key);
  gtk_signal_connect_object(GTK_OBJECT(app), "destroy",
			    GTK_SIGNAL_FUNC(gnome_dialog_close), 
			    GTK_OBJECT(dialog));

  gtk_widget_show(dialog);
}

void
bar_push_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_appbar_push(GNOME_APPBAR(app->statusbar), 
		    gtk_entry_get_text(e));
}

void
bar_set_status_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_appbar_set_status(GNOME_APPBAR(app->statusbar), 
			  gtk_entry_get_text(e));
}

void
bar_set_default_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_appbar_set_default(GNOME_APPBAR(app->statusbar), 
			   gtk_entry_get_text(e));
}

void
bar_set_prompt_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_appbar_set_prompt(GNOME_APPBAR(app->statusbar), 
			  gtk_entry_get_text(e), FALSE);
}

void
bar_set_prompt_modal_cb(GtkWidget * b, GtkEntry * e)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(e));
  gnome_appbar_set_prompt(GNOME_APPBAR(app->statusbar), 
			  gtk_entry_get_text(e), TRUE);
}

void
bar_pop_cb(GtkWidget * b, GnomeApp * app)
{
  gnome_appbar_pop(GNOME_APPBAR(app->statusbar));
}

void
bar_clear_stack_cb(GtkWidget * b, GnomeApp * app)
{
  gnome_appbar_clear_stack(GNOME_APPBAR(app->statusbar));
}

void
bar_refresh_cb(GtkWidget * b, GnomeApp * app)
{
  gnome_appbar_refresh(GNOME_APPBAR(app->statusbar));
}

void
bar_clear_prompt_cb(GtkWidget * b, GnomeApp * app)
{
  gnome_appbar_clear_prompt(GNOME_APPBAR(app->statusbar));
}

void
bar_progress_cb(GtkWidget * b, GtkSpinButton * sb)
{
  GnomeApp * app = gtk_object_get_user_data(GTK_OBJECT(sb));
  gdouble value = gtk_spin_button_get_value_as_float(sb);

  gnome_appbar_set_progress(GNOME_APPBAR(app->statusbar), value);
}

void
dialog_ok_cb(GtkWidget * b, GtkEntry * e)
{
  gnome_ok_dialog (gtk_entry_get_text(e));
}

void
dialog_error_cb(GtkWidget * b, GtkEntry * e)
{
  gnome_error_dialog (gtk_entry_get_text(e));
}

void
dialog_warning_cb(GtkWidget * b, GtkEntry * e)
{
  gnome_warning_dialog (gtk_entry_get_text(e));
}

void
dialog_question_cb(GtkWidget * b, GtkEntry * e)
{
  gnome_question_dialog (gtk_entry_get_text(e),
			 reply_cb, data_string);
}


void
dialog_question_modal_cb(GtkWidget * b, GtkEntry * e)
{
  gnome_question_dialog_modal (gtk_entry_get_text(e),
			       reply_cb, data_string);
}

void
dialog_ok_cancel_cb(GtkWidget * b, GtkEntry * e)
{
  gnome_ok_cancel_dialog (gtk_entry_get_text(e),
			  reply_cb, data_string);
}

void
dialog_ok_cancel_modal_cb(GtkWidget * b, GtkEntry * e)
{
  gnome_ok_cancel_dialog_modal (gtk_entry_get_text(e),
				reply_cb, data_string);
}

void
dialog_request_string_cb(GtkWidget * b, GtkEntry * e)
{
  gnome_request_string_dialog (gtk_entry_get_text(e),
			       string_cb, data_string);
}

void
dialog_request_password_cb(GtkWidget * b, GtkEntry * e)
{
  gnome_request_password_dialog (gtk_entry_get_text(e),
				 string_cb, data_string);
}

void create_app_util()
{
  GnomeApp * app;
  GnomeAppBar * bar;
  GtkBox * vbox;
  GtkWidget * label;
  GtkWidget * sw;

  GtkWidget * hbox, * entry, * button;
  GtkAdjustment * adj;

  app = 
    GNOME_APP(gnome_app_new("testGNOME", 
			    "gnome-app-util/gnome-appbar/gnome-dialog-util test"));

  bar = GNOME_APPBAR(gnome_appbar_new(TRUE, TRUE, GNOME_PREFERENCES_USER));
  gnome_app_set_statusbar(app, GTK_WIDGET(bar));

  vbox = GTK_BOX(gtk_vbox_new(TRUE, GNOME_PAD));
  sw   = gtk_scrolled_window_new(NULL, NULL);

  gtk_container_set_focus_vadjustment (GTK_CONTAINER (vbox),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(sw)));

  gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(vbox));

  gnome_app_set_contents(app, sw);

  label = gtk_label_new("App Util Functions");
  gtk_box_pack_start(vbox, label, TRUE, TRUE, GNOME_PAD);

  label = gtk_label_new("Note: these functions should change behavior\n"
			"according to user preferences for (non)interactive\n"
			"appbar vs. dialogs.");
  gtk_box_pack_start(vbox, label, TRUE, TRUE, GNOME_PAD);

  make_entry_hbox(vbox, "Message", "This is a message", 
		  GTK_SIGNAL_FUNC(message_cb), app);
  make_entry_hbox(vbox, "Flash", "Should disappear shortly", 
		  GTK_SIGNAL_FUNC(flash_cb), app);
  make_entry_hbox(vbox, "Error", "an error", 
		  GTK_SIGNAL_FUNC(error_cb), app);
  make_entry_hbox(vbox, "Warning", "Warning!",
		  GTK_SIGNAL_FUNC(warning_cb), app);
  make_entry_hbox(vbox, "Question", "Is this a question?",
		  GTK_SIGNAL_FUNC(question_cb), app);
  make_entry_hbox(vbox, "Modal Question", "This should be a modal question",
		  GTK_SIGNAL_FUNC(question_modal_cb), app);
  make_entry_hbox(vbox, "OK-Cancel", "An OK-Cancel",
		  GTK_SIGNAL_FUNC(ok_cancel_cb), app);
  make_entry_hbox(vbox, "Modal OK-Cancel", "Modal OK-Cancel",
		  GTK_SIGNAL_FUNC(ok_cancel_modal_cb), app);
  make_entry_hbox(vbox, "Request string", "Enter a string:",
		  GTK_SIGNAL_FUNC(request_string_cb), app);
  make_button_hbox(vbox, "Timeout Progress", 
		   GTK_SIGNAL_FUNC(progress_timeout_cb), app);

  label = gtk_label_new("App Bar Functions");
  gtk_box_pack_start(vbox, label, TRUE, TRUE, GNOME_PAD);
  
  make_entry_hbox(vbox, "AppBar push", "This text was pushed",
		  GTK_SIGNAL_FUNC(bar_push_cb), app);
  make_entry_hbox(vbox, "AppBar set status", "This is a status",
		  GTK_SIGNAL_FUNC(bar_set_status_cb), app);
  make_entry_hbox(vbox, "AppBar set default", "Default text",
		  GTK_SIGNAL_FUNC(bar_set_default_cb), app);
  make_entry_hbox(vbox, "AppBar set prompt", "a prompt",
		  GTK_SIGNAL_FUNC(bar_set_prompt_cb), app);
  make_entry_hbox(vbox, "AppBar set modal prompt", "a modal prompt",
		  GTK_SIGNAL_FUNC(bar_set_prompt_modal_cb), app);

  make_button_hbox(vbox, "AppBar pop", 
		   GTK_SIGNAL_FUNC(bar_pop_cb), app);
  make_button_hbox(vbox, "AppBar clear stack", 
		   GTK_SIGNAL_FUNC(bar_clear_stack_cb), app);
  make_button_hbox(vbox, "AppBar refresh", 
		   GTK_SIGNAL_FUNC(bar_refresh_cb), app);  
  make_button_hbox(vbox, "AppBar clear prompt", 
		   GTK_SIGNAL_FUNC(bar_clear_prompt_cb), app);

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.5, 0.0, 1.0, 0.01, 0.05, 0.05));
  entry = gtk_spin_button_new(adj, 0.5, 3);
  gtk_object_set_user_data(GTK_OBJECT(entry), app);
  hbox = gtk_hbox_new(TRUE, GNOME_PAD);
  button = gtk_button_new_with_label("AppBar set progress");
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, GNOME_PAD);
  gtk_box_pack_end(GTK_BOX(hbox), entry, TRUE, TRUE, GNOME_PAD);
  gtk_box_pack_start(vbox, hbox, TRUE, TRUE, GNOME_PAD);
  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		     GTK_SIGNAL_FUNC(bar_progress_cb), entry);

  label = gtk_label_new("Dialog Util Functions");
  gtk_box_pack_start(vbox, label, TRUE, TRUE, GNOME_PAD);

  make_entry_hbox(vbox, "OK dialog", "Hi, is this OK?",
		  GTK_SIGNAL_FUNC(dialog_ok_cb), app);
  make_entry_hbox(vbox, "Error dialog", "An error! An error!",
		  GTK_SIGNAL_FUNC(dialog_error_cb), app);
  make_entry_hbox(vbox, "Warning dialog", "I'm warning you...", 
		  GTK_SIGNAL_FUNC(dialog_warning_cb), app);
  make_entry_hbox(vbox, "OK-Cancel dialog", "OK or should I cancel?",
		  GTK_SIGNAL_FUNC(dialog_ok_cancel_cb), app);
  make_entry_hbox(vbox, "Modal OK-Cancel dialog", "Modally OK",
		  GTK_SIGNAL_FUNC(dialog_ok_cancel_modal_cb), app);
  make_entry_hbox(vbox, "Question dialog", "Are you sure?",
		  GTK_SIGNAL_FUNC(dialog_question_cb), app);
  make_entry_hbox(vbox, "Modal question dialog", "Modal - are you sure?",
		  GTK_SIGNAL_FUNC(dialog_question_modal_cb), app);
  make_entry_hbox(vbox, "Request string dialog", "Enter a string",
		  GTK_SIGNAL_FUNC(dialog_request_string_cb), app);
  make_entry_hbox(vbox, "Request password dialog", "Enter password",
		  GTK_SIGNAL_FUNC(dialog_request_password_cb), app);

  gtk_widget_set_usize(GTK_WIDGET(app), 640, 480);

  gtk_widget_show_all(GTK_WIDGET(app));
}

int main (int argc, char *argv[])
{
	struct {
		char *label;
		void (*func) ();
	} buttons[] =
	  {
		  { "app-util/appbar/dialog-util", create_app_util },
		  { "calendar", create_calendar },
		  { "calculator", create_calc },
#ifdef GTK_HAVE_ACCEL_GROUP
		  { "canvas", create_canvas },
#endif
		  { "clock",	create_clock },
		  { "color-sel", create_colorsel },
		  { "date edit", create_date_edit },
		  { "dialog", create_dialog },
		  { "file entry", create_file_entry },
		  { "number entry", create_number_entry },
		  { "font sel", create_font_sel },
		  { "icon list", create_icon_list }, 
		  { "lamp", create_lamp },
		  { "less", create_less },
		  { "pixmap", create_pixmap },
		  { "(Reload preferences)", gnome_preferences_load },
/*	{ "prop box", create_property_box }, */
	  };
	int nbuttons = sizeof (buttons) / sizeof (buttons[0]);
	GtkWidget *app;
	GtkWidget *box1; 
	GtkWidget *box2;
	GtkWidget *button;
	GtkWidget *scrolled_window;
	int i;

	gnome_init ("testGNOME", NULL, argc, argv, 0, NULL);

	gtk_init (&argc, &argv);

	app = create_newwin(FALSE,"testGNOME", "testGNOME");
	gtk_widget_set_usize (app, 200,300);
	box1 = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents(GNOME_APP(app),box1);
	gtk_widget_show (box1);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_border_width (GTK_CONTAINER (scrolled_window), 10);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	GTK_WIDGET_UNSET_FLAGS (GTK_SCROLLED_WINDOW (scrolled_window)->vscrollbar,
				GTK_CAN_FOCUS);
	gtk_box_pack_start (GTK_BOX (box1), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show (scrolled_window);
	box2 = gtk_vbox_new (FALSE, 0);
	gtk_container_border_width (GTK_CONTAINER (box2), 10);
	gtk_container_add (GTK_CONTAINER (scrolled_window), box2);
	gtk_container_set_focus_vadjustment (GTK_CONTAINER (box2),gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(scrolled_window)));
	gtk_widget_show (box2);
	for (i = 0; i < nbuttons; i++)
	{
		button = gtk_button_new_with_label (buttons[i].label);
		if (buttons[i].func)
			gtk_signal_connect (GTK_OBJECT (button),
					    "clicked",
					    GTK_SIGNAL_FUNC(buttons[i].func),
					    NULL);
		else
			gtk_widget_set_sensitive (button, FALSE);
		gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
		gtk_widget_show (button);
	}

	gtk_widget_show (app);
	gtk_main();
	return 0;
}


