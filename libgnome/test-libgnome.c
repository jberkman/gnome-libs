#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libgnome.h"

#include <gobject/gvaluetypes.h>
#include <gobject/gparamspecs.h>

static int foo = 0;

static struct poptOption options[] = {
    {"foo", '\0', POPT_ARG_INT, &foo, 0, N_("Test option."), NULL},
    {NULL}
};

static void
get_property (GObject *object, guint param_id, GValue *value,
	      GParamSpec *pspec)
{
    GnomeProgram *program;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_PROGRAM (object));

    program = GNOME_PROGRAM (object);

    switch (param_id) {
    default:
	g_message (G_STRLOC);
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
set_property (GObject *object, guint param_id,
	      const GValue *value, GParamSpec *pspec)
{
    GnomeProgram *program;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GNOME_IS_PROGRAM (object));

    program = GNOME_PROGRAM (object);

    switch (param_id) {
    default:
	g_message (G_STRLOC);
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	break;
    }
}

static void
test_pre_args_parse (GnomeProgram *program, const GnomeModuleInfo *mod_info)
{
    g_message (G_STRLOC ": %p", program);
}

static void
test_post_args_parse (GnomeProgram *program, const GnomeModuleInfo *mod_info)
{
    g_message (G_STRLOC ": %p", program);
}

static void
test_init_pass (const GnomeModuleInfo *mod_info)
{
    g_message (G_STRLOC);
}

static void
test_constructor (GType type, guint n_construct_properties,
		  GObjectConstructParam *construct_properties,
		  const GnomeModuleInfo *mod_info)
{
    GnomeProgramClass *pclass;
    guint test_id;

    pclass = GNOME_PROGRAM_CLASS (g_type_class_peek (type));

    test_id = gnome_program_install_property
	(pclass, get_property, set_property,
	 g_param_spec_boolean ("test", NULL, NULL,
			       FALSE,
			       (G_PARAM_READABLE | G_PARAM_WRITABLE)));

    g_message (G_STRLOC ": %p - %d", pclass, test_id);
}

GnomeModuleInfo test_moduleinfo = {
    "test", VERSION, "Test Application",
    NULL,
    test_pre_args_parse, test_post_args_parse,
    NULL,
    test_init_pass, test_constructor,
    NULL, NULL
};

static void
test_file_locate (GnomeProgram *program) 
{
    GSList *locations = NULL, *c;

    gnome_program_locate_file (program, GNOME_FILE_DOMAIN_CONFIG,
			       "test.config", FALSE, &locations);

    for (c = locations; c; c = c->next)
	g_message (G_STRLOC ": `%s'", (gchar *) c->data);
}

static void
test_properties (GnomeProgram *program) 
{
    g_object_set (G_OBJECT (program), "test", TRUE, NULL);
}

int
main (int argc, char **argv)
{
    GValue value = { 0, };
    GnomeProgram *program;
    gchar *app_prefix, *app_id, *app_version;
    const gchar *human_readable_name;
    gchar *gnome_path;

    program = gnome_program_init ("test-libgnome", VERSION, argc, argv,
				  GNOME_PARAM_POPT_TABLE, options,
				  GNOME_PARAM_HUMAN_READABLE_NAME,
				  _("The Application Name"),
				  GNOME_PARAM_MODULE_INFO,
				  &test_moduleinfo, NULL);

    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (program), "app_prefix", &value);
    app_prefix = g_value_dup_string (&value);
    g_value_unset (&value);

    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (program), "app_id", &value);
    app_id = g_value_dup_string (&value);
    g_value_unset (&value);

    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (program), "app_version", &value);
    app_version = g_value_dup_string (&value);
    g_value_unset (&value);

    g_value_init (&value, G_TYPE_STRING);
    g_object_get_property (G_OBJECT (program), "gnome_path", &value);
    gnome_path = g_value_dup_string (&value);
    g_value_unset (&value);

    human_readable_name = gnome_program_get_human_readable_name (program);

    g_message (G_STRLOC ": %d - `%s' - `%s' - `%s' - `%s'", foo, app_prefix,
	       app_id, app_version, human_readable_name);

    g_message (G_STRLOC ": GNOME_PATH == `%s'", gnome_path);

    test_file_locate (program);

    test_properties (program);

    return 0;
}