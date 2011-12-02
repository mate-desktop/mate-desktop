/* mate-rr-config.c
 *
 * Copyright 2007, 2008, Red Hat, Inc.
 * 
 * This file is part of the Mate Library.
 * 
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 * 
 * Author: Soren Sandmann <sandmann@redhat.com>
 */

#define MATE_DESKTOP_USE_UNSTABLE_API

#include <config.h>
#include <glib/gi18n-lib.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <X11/Xlib.h>
#include <gdk/gdkx.h>

#undef MATE_DISABLE_DEPRECATED
#include "libmateui/mate-rr-config.h"

#include "edid.h"
#include "mate-rr-private.h"

#define CONFIG_INTENDED_BASENAME "monitors.xml"
#define CONFIG_BACKUP_BASENAME "monitors.xml.backup"

/* In version 0 of the config file format, we had several <configuration>
 * toplevel elements and no explicit version number.  So, the filed looked
 * like
 *
 *   <configuration>
 *     ...
 *   </configuration>
 *   <configuration>
 *     ...
 *   </configuration>
 *
 * Since version 1 of the config file, the file has a toplevel <monitors>
 * element to group all the configurations.  That element has a "version"
 * attribute which is an integer. So, the file looks like this:
 *
 *   <monitors version="1">
 *     <configuration>
 *       ...
 *     </configuration>
 *     <configuration>
 *       ...
 *     </configuration>
 *   </monitors>
 */

/* A helper wrapper around the GMarkup parser stuff */
static gboolean parse_file_gmarkup (const gchar *file,
				    const GMarkupParser *parser,
				    gpointer data,
				    GError **err);

typedef struct CrtcAssignment CrtcAssignment;

static gboolean         crtc_assignment_apply (CrtcAssignment   *assign,
					       guint32           timestamp,
					       GError          **error);
static CrtcAssignment  *crtc_assignment_new   (MateRRScreen    *screen,
					       MateOutputInfo **outputs,
					       GError          **error);
static void             crtc_assignment_free  (CrtcAssignment   *assign);
static void             output_free           (MateOutputInfo  *output);
static MateOutputInfo *output_copy           (MateOutputInfo  *output);

typedef struct Parser Parser;

/* Parser for monitor configurations */
struct Parser
{
    int			config_file_version;
    MateOutputInfo *	output;
    MateRRConfig *	configuration;
    GPtrArray *		outputs;
    GPtrArray *		configurations;
    GQueue *		stack;
};

static int
parse_int (const char *text)
{
    return strtol (text, NULL, 0);
}

static guint
parse_uint (const char *text)
{
    return strtoul (text, NULL, 0);
}

static gboolean
stack_is (Parser *parser,
	  const char *s1,
	  ...)
{
    GList *stack = NULL;
    const char *s;
    GList *l1, *l2;
    va_list args;
    
    stack = g_list_prepend (stack, (gpointer)s1);
    
    va_start (args, s1);
    
    s = va_arg (args, const char *);
    while (s)
    {
	stack = g_list_prepend (stack, (gpointer)s);
	s = va_arg (args, const char *);
    }
	
    l1 = stack;
    l2 = parser->stack->head;
    
    while (l1 && l2)
    {
	if (strcmp (l1->data, l2->data) != 0)
	{
	    g_list_free (stack);
	    return FALSE;
	}
	
	l1 = l1->next;
	l2 = l2->next;
    }
    
    g_list_free (stack);
    
    return (!l1 && !l2);
}

static void
handle_start_element (GMarkupParseContext *context,
		      const gchar         *name,
		      const gchar        **attr_names,
		      const gchar        **attr_values,
		      gpointer             user_data,
		      GError             **err)
{
    Parser *parser = user_data;

    if (strcmp (name, "output") == 0)
    {
	int i;
	g_assert (parser->output == NULL);

	parser->output = g_new0 (MateOutputInfo, 1);
	parser->output->rotation = 0;
	
	for (i = 0; attr_names[i] != NULL; ++i)
	{
	    if (strcmp (attr_names[i], "name") == 0)
	    {
		parser->output->name = g_strdup (attr_values[i]);
		break;
	    }
	}

	if (!parser->output->name)
	{
	    /* This really shouldn't happen, but it's better to make
	     * something up than to crash later.
	     */
	    g_warning ("Malformed monitor configuration file");
	    
	    parser->output->name = g_strdup ("default");
	}	
	parser->output->connected = FALSE;
	parser->output->on = FALSE;
	parser->output->primary = FALSE;
    }
    else if (strcmp (name, "configuration") == 0)
    {
	g_assert (parser->configuration == NULL);
	
	parser->configuration = g_new0 (MateRRConfig, 1);
	parser->configuration->clone = FALSE;
	parser->configuration->outputs = NULL;
    }
    else if (strcmp (name, "monitors") == 0)
    {
	int i;

	for (i = 0; attr_names[i] != NULL; i++)
	{
	    if (strcmp (attr_names[i], "version") == 0)
	    {
		parser->config_file_version = parse_int (attr_values[i]);
		break;
	    }
	}
    }

    g_queue_push_tail (parser->stack, g_strdup (name));
}

static void
handle_end_element (GMarkupParseContext *context,
		    const gchar         *name,
		    gpointer             user_data,
		    GError             **err)
{
    Parser *parser = user_data;
    
    if (strcmp (name, "output") == 0)
    {
	/* If no rotation properties were set, just use MATE_RR_ROTATION_0 */
	if (parser->output->rotation == 0)
	    parser->output->rotation = MATE_RR_ROTATION_0;
	
	g_ptr_array_add (parser->outputs, parser->output);

	parser->output = NULL;
    }
    else if (strcmp (name, "configuration") == 0)
    {
	g_ptr_array_add (parser->outputs, NULL);
	parser->configuration->outputs =
	    (MateOutputInfo **)g_ptr_array_free (parser->outputs, FALSE);
	parser->outputs = g_ptr_array_new ();
	g_ptr_array_add (parser->configurations, parser->configuration);
	parser->configuration = NULL;
    }
    
    g_free (g_queue_pop_tail (parser->stack));
}

#define TOPLEVEL_ELEMENT (parser->config_file_version > 0 ? "monitors" : NULL)

static void
handle_text (GMarkupParseContext *context,
	     const gchar         *text,
	     gsize                text_len,
	     gpointer             user_data,
	     GError             **err)
{
    Parser *parser = user_data;
    
    if (stack_is (parser, "vendor", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	parser->output->connected = TRUE;
	
	strncpy (parser->output->vendor, text, 3);
	parser->output->vendor[3] = 0;
    }
    else if (stack_is (parser, "clone", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	if (strcmp (text, "yes") == 0)
	    parser->configuration->clone = TRUE;
    }
    else if (stack_is (parser, "product", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	parser->output->connected = TRUE;

	parser->output->product = parse_int (text);
    }
    else if (stack_is (parser, "serial", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	parser->output->connected = TRUE;

	parser->output->serial = parse_uint (text);
    }
    else if (stack_is (parser, "width", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	parser->output->on = TRUE;

	parser->output->width = parse_int (text);
    }
    else if (stack_is (parser, "x", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	parser->output->on = TRUE;

	parser->output->x = parse_int (text);
    }
    else if (stack_is (parser, "y", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	parser->output->on = TRUE;

	parser->output->y = parse_int (text);
    }
    else if (stack_is (parser, "height", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	parser->output->on = TRUE;

	parser->output->height = parse_int (text);
    }
    else if (stack_is (parser, "rate", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	parser->output->on = TRUE;

	parser->output->rate = parse_int (text);
    }
    else if (stack_is (parser, "rotation", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	if (strcmp (text, "normal") == 0)
	{
	    parser->output->rotation |= MATE_RR_ROTATION_0;
	}
	else if (strcmp (text, "left") == 0)
	{
	    parser->output->rotation |= MATE_RR_ROTATION_90;
	}
	else if (strcmp (text, "upside_down") == 0)
	{
	    parser->output->rotation |= MATE_RR_ROTATION_180;
	}
	else if (strcmp (text, "right") == 0)
	{
	    parser->output->rotation |= MATE_RR_ROTATION_270;
	}
    }
    else if (stack_is (parser, "reflect_x", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	if (strcmp (text, "yes") == 0)
	{
	    parser->output->rotation |= MATE_RR_REFLECT_X;
	}
    }
    else if (stack_is (parser, "reflect_y", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	if (strcmp (text, "yes") == 0)
	{
	    parser->output->rotation |= MATE_RR_REFLECT_Y;
	}
    }
    else if (stack_is (parser, "primary", "output", "configuration", TOPLEVEL_ELEMENT, NULL))
    {
	if (strcmp (text, "yes") == 0)
	{
	    parser->output->primary = TRUE;
	}
    }
    else
    {
	/* Ignore other properties so we can expand the format in the future */
    }
}

static void
parser_free (Parser *parser)
{
    int i;
    GList *list;

    g_assert (parser != NULL);

    if (parser->output)
	output_free (parser->output);

    if (parser->configuration)
	mate_rr_config_free (parser->configuration);

    for (i = 0; i < parser->outputs->len; ++i)
    {
	MateOutputInfo *output = parser->outputs->pdata[i];

	output_free (output);
    }

    g_ptr_array_free (parser->outputs, TRUE);

    for (i = 0; i < parser->configurations->len; ++i)
    {
	MateRRConfig *config = parser->configurations->pdata[i];

	mate_rr_config_free (config);
    }

    g_ptr_array_free (parser->configurations, TRUE);

    for (list = parser->stack->head; list; list = list->next)
	g_free (list->data);
    g_queue_free (parser->stack);
    
    g_free (parser);
}

static MateRRConfig **
configurations_read_from_file (const gchar *filename, GError **error)
{
    Parser *parser = g_new0 (Parser, 1);
    MateRRConfig **result;
    GMarkupParser callbacks = {
	handle_start_element,
	handle_end_element,
	handle_text,
	NULL, /* passthrough */
	NULL, /* error */
    };

    parser->config_file_version = 0;
    parser->configurations = g_ptr_array_new ();
    parser->outputs = g_ptr_array_new ();
    parser->stack = g_queue_new ();
    
    if (!parse_file_gmarkup (filename, &callbacks, parser, error))
    {
	result = NULL;
	
	g_assert (parser->outputs);
	goto out;
    }

    g_assert (parser->outputs);
    
    g_ptr_array_add (parser->configurations, NULL);
    result = (MateRRConfig **)g_ptr_array_free (parser->configurations, FALSE);
    parser->configurations = g_ptr_array_new ();
    
    g_assert (parser->outputs);
out:
    parser_free (parser);

    return result;
}

MateRRConfig *
mate_rr_config_new_current (MateRRScreen *screen)
{
    MateRRConfig *config = g_new0 (MateRRConfig, 1);
    GPtrArray *a = g_ptr_array_new ();
    MateRROutput **rr_outputs;
    int i;
    int clone_width = -1;
    int clone_height = -1;
    int last_x;

    g_return_val_if_fail (screen != NULL, NULL);

    rr_outputs = mate_rr_screen_list_outputs (screen);

    config->clone = FALSE;
    
    for (i = 0; rr_outputs[i] != NULL; ++i)
    {
	MateRROutput *rr_output = rr_outputs[i];
	MateOutputInfo *output = g_new0 (MateOutputInfo, 1);
	MateRRMode *mode = NULL;
	const guint8 *edid_data = mate_rr_output_get_edid_data (rr_output);
	MateRRCrtc *crtc;

	output->name = g_strdup (mate_rr_output_get_name (rr_output));
	output->connected = mate_rr_output_is_connected (rr_output);

	if (!output->connected)
	{
	    output->x = -1;
	    output->y = -1;
	    output->width = -1;
	    output->height = -1;
	    output->rate = -1;
	    output->rotation = MATE_RR_ROTATION_0;
	}
	else
	{
	    MonitorInfo *info = NULL;

	    if (edid_data)
		info = decode_edid (edid_data);

	    if (info)
	    {
		memcpy (output->vendor, info->manufacturer_code,
			sizeof (output->vendor));
		
		output->product = info->product_code;
		output->serial = info->serial_number;
		output->aspect = info->aspect_ratio;
	    }
	    else
	    {
		strcpy (output->vendor, "???");
		output->product = 0;
		output->serial = 0;
	    }

	    if (mate_rr_output_is_laptop (rr_output))
		output->display_name = g_strdup (_("Laptop"));
	    else
		output->display_name = make_display_name (info);
		
	    g_free (info);
		
	    crtc = mate_rr_output_get_crtc (rr_output);
	    mode = crtc? mate_rr_crtc_get_current_mode (crtc) : NULL;
	    
	    if (crtc && mode)
	    {
		output->on = TRUE;
		
		mate_rr_crtc_get_position (crtc, &output->x, &output->y);
		output->width = mate_rr_mode_get_width (mode);
		output->height = mate_rr_mode_get_height (mode);
		output->rate = mate_rr_mode_get_freq (mode);
		output->rotation = mate_rr_crtc_get_current_rotation (crtc);

		if (output->x == 0 && output->y == 0) {
			if (clone_width == -1) {
				clone_width = output->width;
				clone_height = output->height;
			} else if (clone_width == output->width &&
				   clone_height == output->height) {
				config->clone = TRUE;
			}
		}
	    }
	    else
	    {
		output->on = FALSE;
		config->clone = FALSE;
	    }

	    /* Get preferred size for the monitor */
	    mode = mate_rr_output_get_preferred_mode (rr_output);
	    
	    if (!mode)
	    {
		MateRRMode **modes = mate_rr_output_list_modes (rr_output);
		
		/* FIXME: we should pick the "best" mode here, where best is
		 * sorted wrt
		 *
		 * - closest aspect ratio
		 * - mode area
		 * - refresh rate
		 * - We may want to extend randrwrap so that get_preferred
		 *   returns that - although that could also depend on
		 *   the crtc.
		 */
		if (modes[0])
		    mode = modes[0];
	    }
	    
	    if (mode)
	    {
		output->pref_width = mate_rr_mode_get_width (mode);
		output->pref_height = mate_rr_mode_get_height (mode);
	    }
	    else
	    {
		/* Pick some random numbers. This should basically never happen */
		output->pref_width = 1024;
		output->pref_height = 768;
	    }
	}

        output->primary = mate_rr_output_get_is_primary (rr_output);
 
	g_ptr_array_add (a, output);
    }

    g_ptr_array_add (a, NULL);
    
    config->outputs = (MateOutputInfo **)g_ptr_array_free (a, FALSE);

    /* Walk the outputs computing the right-most edge of all
     * lit-up displays
     */
    last_x = 0;
    for (i = 0; config->outputs[i] != NULL; ++i)
    {
	MateOutputInfo *output = config->outputs[i];

	if (output->on)
	{
	    last_x = MAX (last_x, output->x + output->width);
	}
    }

    /* Now position all off displays to the right of the
     * on displays
     */
    for (i = 0; config->outputs[i] != NULL; ++i)
    {
	MateOutputInfo *output = config->outputs[i];

	if (output->connected && !output->on)
	{
	    output->x = last_x;
	    last_x = output->x + output->width;
	}
    }
    
    g_assert (mate_rr_config_match (config, config));
    
    return config;
}

static void
output_free (MateOutputInfo *output)
{
    if (output->display_name)
	g_free (output->display_name);

    if (output->name)
	g_free (output->name);
    
    g_free (output);
}

static MateOutputInfo *
output_copy (MateOutputInfo *output)
{
    MateOutputInfo *copy = g_new0 (MateOutputInfo, 1);

    *copy = *output;

    copy->name = g_strdup (output->name);
    copy->display_name = g_strdup (output->display_name);

    return copy;
}

static void
outputs_free (MateOutputInfo **outputs)
{
    int i;

    g_assert (outputs != NULL);

    for (i = 0; outputs[i] != NULL; ++i)
	output_free (outputs[i]);

    g_free (outputs);
}

void
mate_rr_config_free (MateRRConfig *config)
{
    g_return_if_fail (config != NULL);
    outputs_free (config->outputs);
    
    g_free (config);
}

static void
configurations_free (MateRRConfig **configurations)
{
    int i;

    g_assert (configurations != NULL);

    for (i = 0; configurations[i] != NULL; ++i)
	mate_rr_config_free (configurations[i]);

    g_free (configurations);
}

static gboolean
parse_file_gmarkup (const gchar          *filename,
		    const GMarkupParser  *parser,
		    gpointer             data,
		    GError              **err)
{
    GMarkupParseContext *context = NULL;
    gchar *contents = NULL;
    gboolean result = TRUE;
    gsize len;

    if (!g_file_get_contents (filename, &contents, &len, err))
    {
	result = FALSE;
	goto out;
    }
    
    context = g_markup_parse_context_new (parser, 0, data, NULL);

    if (!g_markup_parse_context_parse (context, contents, len, err))
    {
	result = FALSE;
	goto out;
    }

    if (!g_markup_parse_context_end_parse (context, err))
    {
	result = FALSE;
	goto out;
    }

out:
    if (contents)
	g_free (contents);

    if (context)
	g_markup_parse_context_free (context);

    return result;
}

static gboolean
output_match (MateOutputInfo *output1, MateOutputInfo *output2)
{
    g_assert (output1 != NULL);
    g_assert (output2 != NULL);

    if (strcmp (output1->name, output2->name) != 0)
	return FALSE;

    if (strcmp (output1->vendor, output2->vendor) != 0)
	return FALSE;

    if (output1->product != output2->product)
	return FALSE;

    if (output1->serial != output2->serial)
	return FALSE;

    if (output1->connected != output2->connected)
	return FALSE;
    
    return TRUE;
}

static gboolean
output_equal (MateOutputInfo *output1, MateOutputInfo *output2)
{
    g_assert (output1 != NULL);
    g_assert (output2 != NULL);

    if (!output_match (output1, output2))
	return FALSE;

    if (output1->on != output2->on)
	return FALSE;

    if (output1->on)
    {
	if (output1->width != output2->width)
	    return FALSE;
	
	if (output1->height != output2->height)
	    return FALSE;
	
	if (output1->rate != output2->rate)
	    return FALSE;
	
	if (output1->x != output2->x)
	    return FALSE;
	
	if (output1->y != output2->y)
	    return FALSE;
	
	if (output1->rotation != output2->rotation)
	    return FALSE;
    }

    return TRUE;
}

static MateOutputInfo *
find_output (MateRRConfig *config, const char *name)
{
    int i;

    for (i = 0; config->outputs[i] != NULL; ++i)
    {
	MateOutputInfo *output = config->outputs[i];
	
	if (strcmp (name, output->name) == 0)
	    return output;
    }

    return NULL;
}

/* Match means "these configurations apply to the same hardware
 * setups"
 */
gboolean
mate_rr_config_match (MateRRConfig *c1, MateRRConfig *c2)
{
    int i;

    for (i = 0; c1->outputs[i] != NULL; ++i)
    {
	MateOutputInfo *output1 = c1->outputs[i];
	MateOutputInfo *output2;

	output2 = find_output (c2, output1->name);
	if (!output2 || !output_match (output1, output2))
	    return FALSE;
    }
    
    return TRUE;
}

/* Equal means "the configurations will result in the same
 * modes being set on the outputs"
 */
gboolean
mate_rr_config_equal (MateRRConfig  *c1,
		       MateRRConfig  *c2)
{
    int i;

    for (i = 0; c1->outputs[i] != NULL; ++i)
    {
	MateOutputInfo *output1 = c1->outputs[i];
	MateOutputInfo *output2;

	output2 = find_output (c2, output1->name);
	if (!output2 || !output_equal (output1, output2))
	    return FALSE;
    }
    
    return TRUE;
}

static MateOutputInfo **
make_outputs (MateRRConfig *config)
{
    GPtrArray *outputs;
    MateOutputInfo *first_on;
    int i;

    outputs = g_ptr_array_new ();

    first_on = NULL;
    
    for (i = 0; config->outputs[i] != NULL; ++i)
    {
	MateOutputInfo *old = config->outputs[i];
	MateOutputInfo *new = output_copy (old);

	if (old->on && !first_on)
	    first_on = old;
	
	if (config->clone && new->on)
	{
	    g_assert (first_on);

	    new->width = first_on->width;
	    new->height = first_on->height;
	    new->rotation = first_on->rotation;
	    new->x = 0;
	    new->y = 0;
	}

	g_ptr_array_add (outputs, new);
    }

    g_ptr_array_add (outputs, NULL);

    return (MateOutputInfo **)g_ptr_array_free (outputs, FALSE);
}

gboolean
mate_rr_config_applicable (MateRRConfig  *configuration,
			    MateRRScreen  *screen,
			    GError        **error)
{
    MateOutputInfo **outputs;
    CrtcAssignment *assign;
    gboolean result;

    g_return_val_if_fail (configuration != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    outputs = make_outputs (configuration);
    assign = crtc_assignment_new (screen, outputs, error);

    if (assign)
    {
	result = TRUE;
	crtc_assignment_free (assign);
    }
    else
    {
	result = FALSE;
    }

    outputs_free (outputs);

    return result;
}

/* Database management */

static void
ensure_config_directory (void)
{
    g_mkdir_with_parents (g_get_user_config_dir (), 0700);
}

char *
mate_rr_config_get_backup_filename (void)
{
    ensure_config_directory ();
    return g_build_filename (g_get_user_config_dir (), CONFIG_BACKUP_BASENAME, NULL);
}

char *
mate_rr_config_get_intended_filename (void)
{
    ensure_config_directory ();
    return g_build_filename (g_get_user_config_dir (), CONFIG_INTENDED_BASENAME, NULL);
}

static const char *
get_rotation_name (MateRRRotation r)
{
    if (r & MATE_RR_ROTATION_0)
	return "normal";
    if (r & MATE_RR_ROTATION_90)
	return "left";
    if (r & MATE_RR_ROTATION_180)
	return "upside_down";
    if (r & MATE_RR_ROTATION_270)
	return "right";

    return "normal";
}

static const char *
yes_no (int x)
{
    return x? "yes" : "no";
}

static const char *
get_reflect_x (MateRRRotation r)
{
    return yes_no (r & MATE_RR_REFLECT_X);
}

static const char *
get_reflect_y (MateRRRotation r)
{
    return yes_no (r & MATE_RR_REFLECT_Y);
}

static void
emit_configuration (MateRRConfig *config,
		    GString *string)
{
    int j;

    g_string_append_printf (string, "  <configuration>\n");

    g_string_append_printf (string, "      <clone>%s</clone>\n", yes_no (config->clone));
    
    for (j = 0; config->outputs[j] != NULL; ++j)
    {
	MateOutputInfo *output = config->outputs[j];
	
	g_string_append_printf (
	    string, "      <output name=\"%s\">\n", output->name);
	
	if (output->connected && *output->vendor != '\0')
	{
	    g_string_append_printf (
		string, "          <vendor>%s</vendor>\n", output->vendor);
	    g_string_append_printf (
		string, "          <product>0x%04x</product>\n", output->product);
	    g_string_append_printf (
		string, "          <serial>0x%08x</serial>\n", output->serial);
	}
	
	/* An unconnected output which is on does not make sense */
	if (output->connected && output->on)
	{
	    g_string_append_printf (
		string, "          <width>%d</width>\n", output->width);
	    g_string_append_printf (
		string, "          <height>%d</height>\n", output->height);
	    g_string_append_printf (
		string, "          <rate>%d</rate>\n", output->rate);
	    g_string_append_printf (
		string, "          <x>%d</x>\n", output->x);
	    g_string_append_printf (
		string, "          <y>%d</y>\n", output->y);
	    g_string_append_printf (
		string, "          <rotation>%s</rotation>\n", get_rotation_name (output->rotation));
	    g_string_append_printf (
		string, "          <reflect_x>%s</reflect_x>\n", get_reflect_x (output->rotation));
	    g_string_append_printf (
		string, "          <reflect_y>%s</reflect_y>\n", get_reflect_y (output->rotation));
            g_string_append_printf (
                string, "          <primary>%s</primary>\n", yes_no (output->primary));
	}
	
	g_string_append_printf (string, "      </output>\n");
    }
    
    g_string_append_printf (string, "  </configuration>\n");
}

void
mate_rr_config_sanitize (MateRRConfig *config)
{
    int i;
    int x_offset, y_offset;
    gboolean found;

    /* Offset everything by the top/left-most coordinate to
     * make sure the configuration starts at (0, 0)
     */
    x_offset = y_offset = G_MAXINT;
    for (i = 0; config->outputs[i]; ++i)
    {
	MateOutputInfo *output = config->outputs[i];

	if (output->on)
	{
	    x_offset = MIN (x_offset, output->x);
	    y_offset = MIN (y_offset, output->y);
	}
    }

    for (i = 0; config->outputs[i]; ++i)
    {
	MateOutputInfo *output = config->outputs[i];
	
	if (output->on)
	{
	    output->x -= x_offset;
	    output->y -= y_offset;
	}
    }

    /* Only one primary, please */
    found = FALSE;
    for (i = 0; config->outputs[i]; ++i)
    {
        if (config->outputs[i]->primary)
        {
            if (found)
            {
                config->outputs[i]->primary = FALSE;
            }
            else
            {
                found = TRUE;
            }
        }
    }
}


gboolean
mate_rr_config_save (MateRRConfig *configuration, GError **error)
{
    MateRRConfig **configurations;
    GString *output;
    int i;
    gchar *intended_filename;
    gchar *backup_filename;
    gboolean result;

    g_return_val_if_fail (configuration != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    output = g_string_new ("");

    backup_filename = mate_rr_config_get_backup_filename ();
    intended_filename = mate_rr_config_get_intended_filename ();

    configurations = configurations_read_from_file (intended_filename, NULL); /* NULL-GError */
    
    g_string_append_printf (output, "<monitors version=\"1\">\n");

    if (configurations)
    {
	for (i = 0; configurations[i] != NULL; ++i)
	{
	    if (!mate_rr_config_match (configurations[i], configuration))
		emit_configuration (configurations[i], output);
	}

	configurations_free (configurations);
    }

    emit_configuration (configuration, output);

    g_string_append_printf (output, "</monitors>\n");

    /* backup the file first */
    rename (intended_filename, backup_filename); /* no error checking because the intended file may not even exist */

    result = g_file_set_contents (intended_filename, output->str, -1, error);

    if (!result)
	rename (backup_filename, intended_filename); /* no error checking because the backup may not even exist */

    g_free (backup_filename);
    g_free (intended_filename);

    return result;
}

static MateRRConfig *
mate_rr_config_copy (MateRRConfig *config)
{
    MateRRConfig *copy = g_new0 (MateRRConfig, 1);
    int i;
    GPtrArray *array = g_ptr_array_new ();

    copy->clone = config->clone;
    
    for (i = 0; config->outputs[i] != NULL; ++i)
	g_ptr_array_add (array, output_copy (config->outputs[i]));

    g_ptr_array_add (array, NULL);
    copy->outputs = (MateOutputInfo **)g_ptr_array_free (array, FALSE);

    return copy;
}

static MateRRConfig *
config_new_stored (MateRRScreen *screen, const char *filename, GError **error)
{
    MateRRConfig *current;
    MateRRConfig **configs;
    MateRRConfig *result;

    g_return_val_if_fail (screen != NULL, NULL);
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);
    
    current = mate_rr_config_new_current (screen);
    
    configs = configurations_read_from_file (filename, error);

    result = NULL;
    if (configs)
    {
	int i;
	
	for (i = 0; configs[i] != NULL; ++i)
	{
	    if (mate_rr_config_match (configs[i], current))
	    {
		result = mate_rr_config_copy (configs[i]);
		break;
	    }
	}

	if (result == NULL)
	    g_set_error (error, MATE_RR_ERROR, MATE_RR_ERROR_NO_MATCHING_CONFIG,
			 _("none of the saved display configurations matched the active configuration"));

	configurations_free (configs);
    }

    mate_rr_config_free (current);
    
    return result;
}

MateRRConfig *
mate_rr_config_new_stored (MateRRScreen *screen, GError **error)
{
    char *intended_filename;
    MateRRConfig *config;

    intended_filename = mate_rr_config_get_intended_filename ();

    config = config_new_stored (screen, intended_filename, error);

    g_free (intended_filename);

    return config;
}

#ifndef MATE_DISABLE_DEPRECATED_SOURCE
gboolean
mate_rr_config_apply (MateRRConfig *config,
		       MateRRScreen *screen,
		       GError       **error)
{
    return mate_rr_config_apply_with_time (config, screen, GDK_CURRENT_TIME, error);
}
#endif

gboolean
mate_rr_config_apply_with_time (MateRRConfig *config,
				 MateRRScreen *screen,
				 guint32        timestamp,
				 GError       **error)
{
    CrtcAssignment *assignment;
    MateOutputInfo **outputs;
    gboolean result = FALSE;

    outputs = make_outputs (config);

    assignment = crtc_assignment_new (screen, outputs, error);

    outputs_free (outputs);
    
    if (assignment)
    {
	if (crtc_assignment_apply (assignment, timestamp, error))
	    result = TRUE;

	crtc_assignment_free (assignment);

	gdk_flush ();
    }

    return result;
}

#ifndef MATE_DISABLE_DEPRECATED_SOURCE
/**
 * mate_rr_config_apply_stored:
 * @screen: A #MateRRScreen
 * @error: Location to store error, or %NULL
 *
 * See the documentation for mate_rr_config_apply_from_filename().  This
 * function simply calls that other function with a filename of
 * mate_rr_config_get_intended_filename().

 * @Deprecated: 2.26: Use mate_rr_config_apply_from_filename() instead and pass it
 * the filename from mate_rr_config_get_intended_filename().
 */
gboolean
mate_rr_config_apply_stored (MateRRScreen *screen, GError **error)
{
    char *filename;
    gboolean result;

    filename = mate_rr_config_get_intended_filename ();
    result = mate_rr_config_apply_from_filename_with_time (screen, filename, GDK_CURRENT_TIME, error);
    g_free (filename);

    return result;
}
#endif

#ifndef MATE_DISABLE_DEPRECATED_SOURCE
/* mate_rr_config_apply_from_filename:
 * @screen: A #MateRRScreen
 * @filename: Path of the file to look in for stored RANDR configurations.
 * @error: Location to store error, or %NULL
 *
 * First, this function refreshes the @screen to match the current RANDR
 * configuration from the X server.  Then, it tries to load the file in
 * @filename and looks for suitable matching RANDR configurations in the file;
 * if one is found, that configuration will be applied to the current set of
 * RANDR outputs.
 *
 * Typically, @filename is the result of mate_rr_config_get_intended_filename() or
 * mate_rr_config_get_backup_filename().
 *
 * Returns: TRUE if the RANDR configuration was loaded and applied from
 * $(XDG_CONFIG_HOME)/monitors.xml, or FALSE otherwise:
 *
 * If the current RANDR configuration could not be refreshed, the @error will
 * have a domain of #MATE_RR_ERROR and a corresponding error code.
 *
 * If the file in question is loaded successfully but the configuration cannot
 * be applied, the @error will have a domain of #MATE_RR_ERROR.  Note that an
 * error code of #MATE_RR_ERROR_NO_MATCHING_CONFIG is not a real error; it
 * simply means that there were no stored configurations that match the current
 * set of RANDR outputs.
 *
 * If the file in question cannot be loaded, the @error will have a domain of
 * #G_FILE_ERROR.  Note that an error code of G_FILE_ERROR_NOENT is not really
 * an error, either; it means that there was no stored configuration file and so
 * nothing is changed.
 *
 * @Deprecated: 2.28: use mate_rr_config_apply_from_filename_with_time() instead.
 */
gboolean
mate_rr_config_apply_from_filename (MateRRScreen *screen, const char *filename, GError **error)
{
    return mate_rr_config_apply_from_filename_with_time (screen, filename, GDK_CURRENT_TIME, error);
}
#endif

/* mate_rr_config_apply_from_filename_with_time:
 * @screen: A #MateRRScreen
 * @filename: Path of the file to look in for stored RANDR configurations.
 * @timestamp: X server timestamp from the event that causes the screen configuration to change (a user's button press, for example)
 * @error: Location to store error, or %NULL
 *
 * First, this function refreshes the @screen to match the current RANDR
 * configuration from the X server.  Then, it tries to load the file in
 * @filename and looks for suitable matching RANDR configurations in the file;
 * if one is found, that configuration will be applied to the current set of
 * RANDR outputs.
 *
 * Typically, @filename is the result of mate_rr_config_get_intended_filename() or
 * mate_rr_config_get_backup_filename().
 *
 * Returns: TRUE if the RANDR configuration was loaded and applied from
 * $(XDG_CONFIG_HOME)/monitors.xml, or FALSE otherwise:
 *
 * If the current RANDR configuration could not be refreshed, the @error will
 * have a domain of #MATE_RR_ERROR and a corresponding error code.
 *
 * If the file in question is loaded successfully but the configuration cannot
 * be applied, the @error will have a domain of #MATE_RR_ERROR.  Note that an
 * error code of #MATE_RR_ERROR_NO_MATCHING_CONFIG is not a real error; it
 * simply means that there were no stored configurations that match the current
 * set of RANDR outputs.
 *
 * If the file in question cannot be loaded, the @error will have a domain of
 * #G_FILE_ERROR.  Note that an error code of G_FILE_ERROR_NOENT is not really
 * an error, either; it means that there was no stored configuration file and so
 * nothing is changed.
 */
gboolean
mate_rr_config_apply_from_filename_with_time (MateRRScreen *screen, const char *filename, guint32 timestamp, GError **error)
{
    MateRRConfig *stored;
    GError *my_error;

    g_return_val_if_fail (screen != NULL, FALSE);
    g_return_val_if_fail (filename != NULL, FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    my_error = NULL;
    if (!mate_rr_screen_refresh (screen, &my_error)) {
	    if (my_error) {
		    g_propagate_error (error, my_error);
		    return FALSE; /* This is a genuine error */
	    }

	    /* This means the screen didn't change, so just proceed */
    }

    stored = config_new_stored (screen, filename, error);

    if (stored)
    {
	gboolean result;

	result = mate_rr_config_apply_with_time (stored, screen, timestamp, error);

	mate_rr_config_free (stored);
	
	return result;
    }
    else
    {
	return FALSE;
    }
}

/*
 * CRTC assignment
 */
typedef struct CrtcInfo CrtcInfo;

struct CrtcInfo
{
    MateRRMode    *mode;
    int        x;
    int        y;
    MateRRRotation rotation;
    GPtrArray *outputs;
};

struct CrtcAssignment
{
    MateRRScreen *screen;
    GHashTable *info;
    MateRROutput *primary;
};

static gboolean
can_clone (CrtcInfo *info,
	   MateRROutput *output)
{
    int i;

    for (i = 0; i < info->outputs->len; ++i)
    {
	MateRROutput *clone = info->outputs->pdata[i];

	if (!mate_rr_output_can_clone (clone, output))
	    return FALSE;
    }

    return TRUE;
}

static gboolean
crtc_assignment_assign (CrtcAssignment   *assign,
			MateRRCrtc      *crtc,
			MateRRMode      *mode,
			int               x,
			int               y,
			MateRRRotation   rotation,
                        gboolean          primary,
			MateRROutput    *output,
			GError          **error)
{
    CrtcInfo *info = g_hash_table_lookup (assign->info, crtc);
    guint32 crtc_id;
    const char *output_name;

    crtc_id = mate_rr_crtc_get_id (crtc);
    output_name = mate_rr_output_get_name (output);

    if (!mate_rr_crtc_can_drive_output (crtc, output))
    {
	g_set_error (error, MATE_RR_ERROR, MATE_RR_ERROR_CRTC_ASSIGNMENT,
		     _("CRTC %d cannot drive output %s"), crtc_id, output_name);
	return FALSE;
    }

    if (!mate_rr_output_supports_mode (output, mode))
    {
	g_set_error (error, MATE_RR_ERROR, MATE_RR_ERROR_CRTC_ASSIGNMENT,
		     _("output %s does not support mode %dx%d@%dHz"),
		     output_name,
		     mate_rr_mode_get_width (mode),
		     mate_rr_mode_get_height (mode),
		     mate_rr_mode_get_freq (mode));
	return FALSE;
    }

    if (!mate_rr_crtc_supports_rotation (crtc, rotation))
    {
	g_set_error (error, MATE_RR_ERROR, MATE_RR_ERROR_CRTC_ASSIGNMENT,
		     _("CRTC %d does not support rotation=%s"),
		     crtc_id,
		     get_rotation_name (rotation));
	return FALSE;
    }

    if (info)
    {
	if (!(info->mode == mode	&&
	      info->x == x		&&
	      info->y == y		&&
	      info->rotation == rotation))
	{
	    g_set_error (error, MATE_RR_ERROR, MATE_RR_ERROR_CRTC_ASSIGNMENT,
			 _("output %s does not have the same parameters as another cloned output:\n"
			   "existing mode = %d, new mode = %d\n"
			   "existing coordinates = (%d, %d), new coordinates = (%d, %d)\n"
			   "existing rotation = %s, new rotation = %s"),
			 output_name,
			 mate_rr_mode_get_id (info->mode), mate_rr_mode_get_id (mode),
			 info->x, info->y,
			 x, y,
			 get_rotation_name (info->rotation), get_rotation_name (rotation));
	    return FALSE;
	}

	if (!can_clone (info, output))
	{
	    g_set_error (error, MATE_RR_ERROR, MATE_RR_ERROR_CRTC_ASSIGNMENT,
			 _("cannot clone to output %s"),
			 output_name);
	    return FALSE;
	}

	g_ptr_array_add (info->outputs, output);

	if (primary && !assign->primary)
	{
	    assign->primary = output;
	}

	return TRUE;
    }
    else
    {	
	CrtcInfo *info = g_new0 (CrtcInfo, 1);
	
	info->mode = mode;
	info->x = x;
	info->y = y;
	info->rotation = rotation;
	info->outputs = g_ptr_array_new ();
	
	g_ptr_array_add (info->outputs, output);
	
	g_hash_table_insert (assign->info, crtc, info);
	    
        if (primary && !assign->primary)
        {
            assign->primary = output;
        }

	return TRUE;
    }
}

static void
crtc_assignment_unassign (CrtcAssignment *assign,
			  MateRRCrtc         *crtc,
			  MateRROutput       *output)
{
    CrtcInfo *info = g_hash_table_lookup (assign->info, crtc);

    if (info)
    {
	g_ptr_array_remove (info->outputs, output);

        if (assign->primary == output)
        {
            assign->primary = NULL;
        }

	if (info->outputs->len == 0)
	    g_hash_table_remove (assign->info, crtc);
    }
}

static void
crtc_assignment_free (CrtcAssignment *assign)
{
    g_hash_table_destroy (assign->info);

    g_free (assign);
}

typedef struct {
    guint32 timestamp;
    gboolean has_error;
    GError **error;
} ConfigureCrtcState;

static void
configure_crtc (gpointer key,
		gpointer value,
		gpointer data)
{
    MateRRCrtc *crtc = key;
    CrtcInfo *info = value;
    ConfigureCrtcState *state = data;

    if (state->has_error)
	return;

    if (!mate_rr_crtc_set_config_with_time (crtc,
					     state->timestamp,
					     info->x, info->y,
					     info->mode,
					     info->rotation,
					     (MateRROutput **)info->outputs->pdata,
					     info->outputs->len,
					     state->error))
	state->has_error = TRUE;
}

static gboolean
mode_is_rotated (CrtcInfo *info)
{
    if ((info->rotation & MATE_RR_ROTATION_270)		||
	(info->rotation & MATE_RR_ROTATION_90))
    {
	return TRUE;
    }
    return FALSE;
}

static gboolean
crtc_is_rotated (MateRRCrtc *crtc)
{
    MateRRRotation r = mate_rr_crtc_get_current_rotation (crtc);

    if ((r & MATE_RR_ROTATION_270)		||
	(r & MATE_RR_ROTATION_90))
    {
	return TRUE;
    }

    return FALSE;
}

static void
accumulate_error (GString *accumulated_error, GError *error)
{
    g_string_append_printf (accumulated_error, "    %s\n", error->message);
    g_error_free (error);
}

/* Check whether the given set of settings can be used
 * at the same time -- ie. whether there is an assignment
 * of CRTC's to outputs.
 *
 * Brute force - the number of objects involved is small
 * enough that it doesn't matter.
 */
static gboolean
real_assign_crtcs (MateRRScreen *screen,
		   MateOutputInfo **outputs,
		   CrtcAssignment *assignment,
		   GError **error)
{
    MateRRCrtc **crtcs = mate_rr_screen_list_crtcs (screen);
    MateOutputInfo *output;
    int i;
    gboolean tried_mode;
    GError *my_error;
    GString *accumulated_error;
    gboolean success;

    output = *outputs;
    if (!output)
	return TRUE;

    /* It is always allowed for an output to be turned off */
    if (!output->on)
    {
	return real_assign_crtcs (screen, outputs + 1, assignment, error);
    }

    success = FALSE;
    tried_mode = FALSE;
    accumulated_error = g_string_new (NULL);

    for (i = 0; crtcs[i] != NULL; ++i)
    {
	MateRRCrtc *crtc = crtcs[i];
	int crtc_id = mate_rr_crtc_get_id (crtc);
	int pass;

	g_string_append_printf (accumulated_error,
				_("Trying modes for CRTC %d\n"),
				crtc_id);

	/* Make two passes, one where frequencies must match, then
	 * one where they don't have to
	 */
	for (pass = 0; pass < 2; ++pass)
	{
	    MateRROutput *mate_rr_output = mate_rr_screen_get_output_by_name (screen, output->name);
	    MateRRMode **modes = mate_rr_output_list_modes (mate_rr_output);
	    int j;

	    for (j = 0; modes[j] != NULL; ++j)
	    {
		MateRRMode *mode = modes[j];
		int mode_width;
		int mode_height;
		int mode_freq;

		mode_width = mate_rr_mode_get_width (mode);
		mode_height = mate_rr_mode_get_height (mode);
		mode_freq = mate_rr_mode_get_freq (mode);

		g_string_append_printf (accumulated_error,
					_("CRTC %d: trying mode %dx%d@%dHz with output at %dx%d@%dHz (pass %d)\n"),
					crtc_id,
					mode_width, mode_height, mode_freq,
					output->width, output->height, output->rate,
					pass);

		if (mode_width == output->width	&&
		    mode_height == output->height &&
		    (pass == 1 || mode_freq == output->rate))
		{
		    tried_mode = TRUE;

		    my_error = NULL;
		    if (crtc_assignment_assign (
			    assignment, crtc, modes[j],
			    output->x, output->y,
			    output->rotation,
                            output->primary,
			    mate_rr_output,
			    &my_error))
		    {
			my_error = NULL;
			if (real_assign_crtcs (screen, outputs + 1, assignment, &my_error)) {
			    success = TRUE;
			    goto out;
			} else
			    accumulate_error (accumulated_error, my_error);

			crtc_assignment_unassign (assignment, crtc, mate_rr_output);
		    } else
			accumulate_error (accumulated_error, my_error);
		}
	    }
	}
    }

out:

    if (success)
	g_string_free (accumulated_error, TRUE);
    else {
	char *str;

	str = g_string_free (accumulated_error, FALSE);

	if (tried_mode)
	    g_set_error (error, MATE_RR_ERROR, MATE_RR_ERROR_CRTC_ASSIGNMENT,
			 _("could not assign CRTCs to outputs:\n%s"),
			 str);
	else
	    g_set_error (error, MATE_RR_ERROR, MATE_RR_ERROR_CRTC_ASSIGNMENT,
			 _("none of the selected modes were compatible with the possible modes:\n%s"),
			 str);

	g_free (str);
    }

    return success;
}

static void
crtc_info_free (CrtcInfo *info)
{
    g_ptr_array_free (info->outputs, TRUE);
    g_free (info);
}

static void
get_required_virtual_size (CrtcAssignment *assign, int *width, int *height)
{
    GList *active_crtcs = g_hash_table_get_keys (assign->info);
    GList *list;
    int d;

    if (!width)
	width = &d;
    if (!height)
	height = &d;
    
    /* Compute size of the screen */
    *width = *height = 1;
    for (list = active_crtcs; list != NULL; list = list->next)
    {
	MateRRCrtc *crtc = list->data;
	CrtcInfo *info = g_hash_table_lookup (assign->info, crtc);
	int w, h;

	w = mate_rr_mode_get_width (info->mode);
	h = mate_rr_mode_get_height (info->mode);
	
	if (mode_is_rotated (info))
	{
	    int tmp = h;
	    h = w;
	    w = tmp;
	}
	
	*width = MAX (*width, info->x + w);
	*height = MAX (*height, info->y + h);
    }

    g_list_free (active_crtcs);
}

static CrtcAssignment *
crtc_assignment_new (MateRRScreen *screen, MateOutputInfo **outputs, GError **error)
{
    CrtcAssignment *assignment = g_new0 (CrtcAssignment, 1);

    assignment->info = g_hash_table_new_full (
	g_direct_hash, g_direct_equal, NULL, (GFreeFunc)crtc_info_free);

    if (real_assign_crtcs (screen, outputs, assignment, error))
    {
	int width, height;
	int min_width, max_width, min_height, max_height;
	int required_pixels, min_pixels, max_pixels;

	get_required_virtual_size (assignment, &width, &height);

	mate_rr_screen_get_ranges (
	    screen, &min_width, &max_width, &min_height, &max_height);

	required_pixels = width * height;
	min_pixels = min_width * min_height;
	max_pixels = max_width * max_height;

	if (required_pixels < min_pixels || required_pixels > max_pixels)
	{
	    g_set_error (error, MATE_RR_ERROR, MATE_RR_ERROR_BOUNDS_ERROR,
			 /* Translators: the "requested", "minimum", and
			  * "maximum" words here are not keywords; please
			  * translate them as usual. */
			 _("required virtual size does not fit available size: "
			   "requested=(%d, %d), minimum=(%d, %d), maximum=(%d, %d)"),
			 width, height,
			 min_width, min_height,
			 max_width, max_height);
	    goto fail;
	}

	assignment->screen = screen;
	
	return assignment;
    }

fail:
    crtc_assignment_free (assignment);
    
    return NULL;
}

static gboolean
crtc_assignment_apply (CrtcAssignment *assign, guint32 timestamp, GError **error)
{
    MateRRCrtc **all_crtcs = mate_rr_screen_list_crtcs (assign->screen);
    int width, height;
    int i;
    int min_width, max_width, min_height, max_height;
    int width_mm, height_mm;
    gboolean success = TRUE;

    /* Compute size of the screen */
    get_required_virtual_size (assign, &width, &height);

    mate_rr_screen_get_ranges (
	assign->screen, &min_width, &max_width, &min_height, &max_height);

    /* We should never get here if the dimensions don't fit in the virtual size,
     * but just in case we do, fix it up.
     */
    width = MAX (min_width, width);
    width = MIN (max_width, width);
    height = MAX (min_height, height);
    height = MIN (max_height, height);

    /* FMQ: do we need to check the sizes instead of clamping them? */

    /* Grab the server while we fiddle with the CRTCs and the screen, so that
     * apps that listen for RANDR notifications will only receive the final
     * status.
     */

    gdk_x11_display_grab (gdk_screen_get_display (assign->screen->gdk_screen));

    /* Turn off all crtcs that are currently displaying outside the new screen,
     * or are not used in the new setup
     */
    for (i = 0; all_crtcs[i] != NULL; ++i)
    {
	MateRRCrtc *crtc = all_crtcs[i];
	MateRRMode *mode = mate_rr_crtc_get_current_mode (crtc);
	int x, y;

	if (mode)
	{
	    int w, h;
	    mate_rr_crtc_get_position (crtc, &x, &y);

	    w = mate_rr_mode_get_width (mode);
	    h = mate_rr_mode_get_height (mode);

	    if (crtc_is_rotated (crtc))
	    {
		int tmp = h;
		h = w;
		w = tmp;
	    }
	    
	    if (x + w > width || y + h > height || !g_hash_table_lookup (assign->info, crtc))
	    {
		if (!mate_rr_crtc_set_config_with_time (crtc, timestamp, 0, 0, NULL, MATE_RR_ROTATION_0, NULL, 0, error))
		{
		    success = FALSE;
		    break;
		}
		
	    }
	}
    }

    /* The 'physical size' of an X screen is meaningless if that screen
     * can consist of many monitors. So just pick a size that make the
     * dpi 96.
     *
     * Firefox and Evince apparently believe what X tells them.
     */
    width_mm = (width / 96.0) * 25.4 + 0.5;
    height_mm = (height / 96.0) * 25.4 + 0.5;

    if (success)
    {
	ConfigureCrtcState state;

	mate_rr_screen_set_size (assign->screen, width, height, width_mm, height_mm);

	state.timestamp = timestamp;
	state.has_error = FALSE;
	state.error = error;
	
	g_hash_table_foreach (assign->info, configure_crtc, &state);

	success = !state.has_error;
    }

    mate_rr_screen_set_primary_output (assign->screen, assign->primary);

    gdk_x11_display_ungrab (gdk_screen_get_display (assign->screen->gdk_screen));

    return success;
}
