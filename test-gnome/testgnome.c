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

#include <gnome.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "bomb.xpm"

gchar *authors[] = {
	"Richard Hestilow",
	NULL
};

GtkWidget *create_newwin(gboolean normal, gchar *appname, gchar *title);

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
	if (normal==TRUE)
        {
                gtk_signal_connect(GTK_OBJECT(app), "delete_event",
				   GTK_SIGNAL_FUNC(window_close), NULL);
        }
	else
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

void create_dialog()
{
	GtkWidget *dialog;
	dialog = gnome_dialog_new("Close this window?",	GNOME_STOCK_BUTTON_YES,
				  GNOME_STOCK_BUTTON_NO,NULL);
	gnome_dialog_button_connect(GNOME_DIALOG(dialog),0,
				    GTK_SIGNAL_FUNC(delete_event),dialog);
	gtk_widget_show(dialog);
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

void create_font_sel()
{
	GtkWidget *fontsel;

	fontsel = gnome_font_selector_new();
	gtk_widget_show(fontsel);
}

/* Icon lists seem too complicated to implement right now. FIXME 
void create_icon_list()
{
GtkWidget *app;
GtkWidget *iconlist;

app = create_newwin(TRUE,"testGNOME","Icon List");
iconlist = gnome_icon_list_new();
gnome_app_set_contents(GNOME_APP(app),iconlist);
gtk_widget_show(iconlist);
gtk_widget_show(app);
}
*/

void create_less()
{
	GtkWidget *app;
	GtkWidget *less;

	app = create_newwin(TRUE,"testGNOME","Less");
	less = gnome_less_new();
	gnome_app_set_contents(GNOME_APP(app),less);
	gnome_less_fixed_font(GNOME_LESS(less));
	gtk_widget_show(less);
	gtk_widget_show(app);
	gnome_less_show_command(GNOME_LESS(less),"fortune");
}

void create_pixmap()
{
	GtkWidget *app;
	GtkWidget *pixmap;

	app = create_newwin(TRUE,"testGNOME","Pixmap");
	pixmap = gnome_pixmap_new_from_file("bomb.xpm"); /* Change to xpm data
							    later */
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

int main (int argc, char *argv[])
{
	struct {
		char *label;
		void (*func) ();
	} buttons[] =
	  {
		  { "calendar", create_calendar },
		  { "calculator", create_calc },
		  { "clock",	create_clock },
		  { "color-sel", create_colorsel },
		  { "date edit", create_date_edit },
		  { "dialog", create_dialog },
		  { "file entry", create_file_entry },
		  { "font sel", create_font_sel },
/*	{ "icon list", create_icon_list }, */
		  { "less", create_less },
		  { "pixmap", create_pixmap },
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
}


