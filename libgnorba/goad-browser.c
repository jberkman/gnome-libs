#include <gnome.h>
#include "libgnorba/gnorba.h"

static GtkWidget *gb_create_main_window(CORBA_ORB orb, CORBA_Environment *ev);
static void gb_create_server_list(GtkWidget *w, GtkCList *clist);
static gboolean gb_is_server_active_p(CORBA_Object ns, GoadServer *s,
				      CORBA_Environment *ev);

static void gb_activate_server(GtkWidget *w, GtkCList *clist);

static char *activate_id = NULL, *activate_repo_id = NULL;

static GdkPixmap *pm_active, *pm_inactive;
static GdkBitmap *pm_active_mask, *pm_inactive_mask;
static GoadServer *slist = NULL;
static GtkWidget *mainwin;

static guint status_ctx = -1;

static const struct poptOption options[] = {
  {"activate-id", 'i', POPT_ARG_STRING, &activate_id, 0, N_("GOAD ID of the server to activate"), N_("ID")},
  {"activate-repo-id", 'r', POPT_ARG_STRING, &activate_repo_id, 0, N_("Repository ID of the server to activate"), N_("REPO-ID")},
  {NULL, '\0', 0, NULL, 0}
};

static const gchar *column_titles[] = {"Server ID", "Active?",
				       "Description", "Repo ID",
				       "Type", "Location Info"};
CORBA_ORB orb;

int main(int argc, char *argv[])
{
  char *strior;
  CORBA_Object obj = CORBA_OBJECT_NIL;
  CORBA_Environment ev;

  CORBA_exception_init(&ev);

  orb = gnome_CORBA_init_with_popt_table("goad-browser", VERSION,
					 &argc, argv, options, 0, NULL,
					 0, &ev);

  if(activate_id) {
    obj = goad_server_activate_with_id(NULL, activate_id, 0);
  } else if(activate_repo_id) {
    obj = goad_server_activate_with_repo_id(NULL, activate_repo_id, 0);
  } else {
    /* setup then main loop */

    gb_create_main_window(orb, &ev);
    gtk_widget_show_all(mainwin);
    gtk_main();
  }

  if(!CORBA_Object_is_nil(obj, &ev)) {
    strior = CORBA_ORB_object_to_string(orb, obj, &ev);
    printf("%s\n", strior);
    CORBA_free(strior);
    CORBA_Object_release(obj, &ev);
  }

  return 0;
}

static GtkWidget *
gb_create_main_window(CORBA_ORB orb, CORBA_Environment *ev)
{
  static GnomeUIInfo filemenu[] = {
    GNOMEUIINFO_ITEM_STOCK("E_xit", NULL, gtk_main_quit, GNOME_STOCK_MENU_EXIT),
    GNOMEUIINFO_END
  };
  static GnomeUIInfo mainmenu[] = {
    GNOMEUIINFO_SUBTREE("_File", filemenu),
    GNOMEUIINFO_END
  };
  static GnomeUIInfo toolbar[] = {
    GNOMEUIINFO_ITEM_STOCK("Activate", "Start this server",
			   gb_activate_server, GNOME_STOCK_PIXMAP_EXEC),
    GNOMEUIINFO_ITEM_STOCK("Refresh", "Refresh the server list",
			   gb_create_server_list, GNOME_STOCK_PIXMAP_REFRESH),
    GNOMEUIINFO_END
  };

  GtkWidget *wtmp, *clist;

  mainwin = gnome_app_new("goad-browser",
			  "GNOME Object Activation Directory Browser");

  gtk_window_set_policy(GTK_WINDOW(mainwin), TRUE, TRUE, TRUE);

  gdk_imlib_load_file_to_pixmap(gnome_pixmap_file("yes.xpm"), &pm_active, &pm_active_mask);
  gdk_imlib_load_file_to_pixmap(gnome_pixmap_file("no.xpm"), &pm_inactive, &pm_inactive_mask);

  gtk_widget_push_visual(gdk_imlib_get_visual());
  gtk_widget_push_colormap(gdk_imlib_get_colormap());

  clist = gtk_clist_new_with_titles(6, (gchar **)column_titles);

  gtk_widget_pop_colormap();
  gtk_widget_pop_visual();

  wtmp = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wtmp),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(wtmp), clist);

  gnome_app_set_contents(GNOME_APP(mainwin), wtmp);

  gtk_widget_show_all(mainwin);

  gnome_app_create_menus_with_data(GNOME_APP(mainwin), mainmenu, clist);
  gnome_app_create_toolbar_with_data(GNOME_APP(mainwin), toolbar, clist);
  gnome_app_set_statusbar(GNOME_APP(mainwin), gtk_statusbar_new());

  status_ctx =
    gtk_statusbar_get_context_id(GTK_STATUSBAR(GNOME_APP(mainwin)->statusbar),
				 "GtkStatusbar API sucks");

  gb_create_server_list(NULL, GTK_CLIST(clist));

  return mainwin;
}

static void
gb_create_server_list(GtkWidget *w, GtkCList *clist)
{
  int i, currow, j;
  gchar *columns[6];
  int maxw[6];

  GdkFont *font;

  CORBA_Object ns;
  CORBA_Environment ev;

  CORBA_exception_init(&ev);

  gtk_clist_freeze(clist);
  gtk_clist_clear(clist);

  if(slist)
    goad_server_list_free(slist);

  slist = goad_server_list_get();

  ns = gnome_name_service_get();

  font = gtk_widget_get_style(GTK_WIDGET(clist))->font;

  columns[1] = NULL;
  memset(maxw, 0, sizeof(maxw));

  for(i = 0; slist[i].repo_id; i++) {

    columns[0] = slist[i].server_id;
    columns[2] = slist[i].description;
    columns[3] = slist[i].repo_id;

    switch(slist[i].type) {
    case GOAD_SERVER_SHLIB: columns[4] = "shlib"; break;
    case GOAD_SERVER_EXE: columns[4] = "exe"; break;
    case GOAD_SERVER_RELAY: columns[4] = "relay"; break;
    default:
      columns[4] = NULL;
    }

    columns[5] = slist[i].location_info;

    currow = gtk_clist_append(clist, columns);
    gtk_clist_set_row_data(clist, currow, &slist[i]);

    if(gb_is_server_active_p(ns, &slist[i], &ev))
      gtk_clist_set_pixmap(clist, currow, 1, pm_active, pm_active_mask);
    else
      gtk_clist_set_pixmap(clist, currow, 1, pm_inactive, pm_inactive_mask);

    for(j = 0; j < 6; j++) {
      if(columns[j])
	maxw[j] = MAX(gdk_string_width(font, columns[j]), maxw[j]);
    }
  }

  for(j = 0; j < 6; j++)
    maxw[j] = MAX(gdk_string_width(font, column_titles[j]), maxw[j]);

  for(i = j = 0; j < 6; i += maxw[j], j++)
    gtk_clist_set_column_width(clist, j, maxw[j] + GNOME_PAD_SMALL);

  gtk_widget_set_usize(GTK_WIDGET(clist), i + 50, 300);

  gtk_clist_thaw(clist);

  CORBA_Object_release(ns, &ev);
  CORBA_exception_free(&ev);
}

static gboolean
gb_is_server_active_p(CORBA_Object ns, GoadServer *s, CORBA_Environment *ev)
{
  CORBA_Object otmp;
  static CosNaming_NameComponent nc[3] = {{"GNOME", "subcontext"},
				   {"Servers", "subcontext"}};
  static CosNaming_Name          nom = {0, 3, nc, CORBA_FALSE};
  gboolean retval;

  nc[2].id = s->server_id;
  nc[2].kind = "server";

  otmp = CosNaming_NamingContext_resolve(ns, &nom, ev);

  if (ev->_major != CORBA_NO_EXCEPTION)
    otmp = CORBA_OBJECT_NIL;

  CORBA_exception_free(ev);

  if(CORBA_Object_is_nil(otmp, ev))
    retval = FALSE;
  else {
    retval = TRUE;
    CORBA_Object_release(otmp, ev);
  }

  return retval;
}

static void
gb_activate_server(GtkWidget *w, GtkCList *clist)
{
  GoadServer *serv;
  char *iorstr;
  CORBA_Environment ev;
  CORBA_Object obj;

  if(!clist->selection) return;

  CORBA_exception_init(&ev);

  serv = gtk_clist_get_row_data(clist, GPOINTER_TO_INT(clist->selection->data));
  if(!serv) return;

  obj = goad_server_activate(serv, 0);

  if(!CORBA_Object_is_nil(obj, &ev)) {
    iorstr = CORBA_ORB_object_to_string(orb, obj, &ev);
    gtk_statusbar_pop(GTK_STATUSBAR(GNOME_APP(mainwin)->statusbar), status_ctx);
    gtk_statusbar_push(GTK_STATUSBAR(GNOME_APP(mainwin)->statusbar), status_ctx, iorstr);
    g_print("%s\n", iorstr);
    CORBA_free(iorstr);
    gb_create_server_list(NULL, clist);
  }

  CORBA_exception_free(&ev);
}
