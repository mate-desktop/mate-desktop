/* mate-monitor-info.c - what MATE needs to know about each monitor
 *
 * Copyright (C) 2026 Kerem Soke
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * SECTION:mate-monitor-info
 * @short_description: monitor ids and facts for MATE desktop components
 *
 * #MateMonitorInfo answers, for each #GdkMonitor, what the panel, the
 * file manager's desktop and the preferences need: the monitor id that
 * per-monitor settings are stored under, a name to show, whether it is
 * built in, the primary monitor, mirroring, the work area and who draws
 * the desktop background.
 *
 * On a Wayland compositor that offers mate_output_manager_v1 (see
 * mate-wayland-protocols) the answers come from the compositor. Otherwise
 * they come from GDK: the id is the monitor's make and model, numbered
 * "+1", "+2" by position for duplicates; the work area is
 * gdk_monitor_get_workarea(); the background owner is unknown and no
 * capability is reported.
 */

#include <config.h>
#include <string.h>
#include <unistd.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include "mate-monitor-info.h"

#ifdef HAVE_WAYLAND
#include <gdk/gdkwayland.h>
#include "mate-output-v1-client-protocol.h"
#endif

typedef struct {
	MateMonitorInfo *info;
	GdkMonitor *monitor;
#ifdef HAVE_WAYLAND
	struct mate_output_v1 *proxy;
#endif
	gboolean has_state;
	gchar *id;
	gchar *display_name;
	gchar *mirror_of;
	gboolean internal;
	gboolean primary;
	gboolean has_workarea;
	GdkRectangle workarea;
	MateBackgroundOwner owner;
} Entry;

struct _MateMonitorInfo {
	GObject parent_instance;

	GdkDisplay *display;
	GHashTable *entries;
	guint capabilities;
#ifdef HAVE_WAYLAND
	struct wl_registry *registry;
	struct mate_output_manager_v1 *manager;
#endif
};

enum {
	CHANGED,
	N_SIGNALS
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE (MateMonitorInfo, mate_monitor_info, G_TYPE_OBJECT)

static void
entry_free (gpointer data)
{
	Entry *entry = data;

#ifdef HAVE_WAYLAND
	if (entry->proxy)
		mate_output_v1_destroy (entry->proxy);
#endif
	g_free (entry->id);
	g_free (entry->display_name);
	g_free (entry->mirror_of);
	g_free (entry);
}

#ifdef HAVE_WAYLAND

static void
output_id (void *data, struct mate_output_v1 *proxy, const char *id)
{
	Entry *entry = data;

	g_free (entry->id);
	entry->id = g_strdup (id);
}

static void
output_edid_identity (void *data, struct mate_output_v1 *proxy,
                      const char *vendor, uint32_t product, uint32_t serial,
                      const char *serial_string)
{
}

static void
output_edid (void *data, struct mate_output_v1 *proxy, int32_t fd,
             uint32_t size)
{
	close (fd);
}

static void
output_display_name (void *data, struct mate_output_v1 *proxy,
                     const char *name)
{
	Entry *entry = data;

	g_free (entry->display_name);
	entry->display_name = g_strdup (name);
}

static void
output_internal (void *data, struct mate_output_v1 *proxy, uint32_t internal)
{
	((Entry *) data)->internal = internal != 0;
}

static void
output_primary (void *data, struct mate_output_v1 *proxy, uint32_t primary)
{
	((Entry *) data)->primary = primary != 0;
}

static void
output_mirror_of (void *data, struct mate_output_v1 *proxy, const char *id)
{
	Entry *entry = data;

	g_free (entry->mirror_of);
	entry->mirror_of = g_strdup (id);
}

static void
output_workarea (void *data, struct mate_output_v1 *proxy, int32_t x,
                 int32_t y, uint32_t width, uint32_t height)
{
	Entry *entry = data;

	entry->has_workarea = TRUE;
	entry->workarea.x = x;
	entry->workarea.y = y;
	entry->workarea.width = (int) width;
	entry->workarea.height = (int) height;
}

static void
output_background_owner (void *data, struct mate_output_v1 *proxy,
                         uint32_t owner)
{
	Entry *entry = data;

	entry->owner = owner <= MATE_OUTPUT_V1_BACKGROUND_OWNER_COMPOSITOR
	               ? (MateBackgroundOwner) owner
	               : MATE_BACKGROUND_OWNER_UNKNOWN;
}

static void
output_done (void *data, struct mate_output_v1 *proxy)
{
	Entry *entry = data;

	entry->has_state = TRUE;
	g_signal_emit (entry->info, signals[CHANGED], 0, entry->monitor);
}

static const struct mate_output_v1_listener output_listener = {
	output_id,
	output_edid_identity,
	output_edid,
	output_display_name,
	output_internal,
	output_primary,
	output_mirror_of,
	output_workarea,
	output_background_owner,
	output_done,
};

static void
manager_capabilities (void *data, struct mate_output_manager_v1 *manager,
                      uint32_t capabilities)
{
	MateMonitorInfo *info = data;

	info->capabilities = capabilities;
	g_signal_emit (info, signals[CHANGED], 0, NULL);
}

static void
manager_finished (void *data, struct mate_output_manager_v1 *manager)
{
}

static const struct mate_output_manager_v1_listener manager_listener = {
	manager_capabilities,
	manager_finished,
};

static void
registry_global (void *data, struct wl_registry *registry, uint32_t name,
                 const char *interface, uint32_t version)
{
	MateMonitorInfo *info = data;

	if (info->manager ||
	    strcmp (interface, mate_output_manager_v1_interface.name) != 0)
		return;
	info->manager = wl_registry_bind (registry, name,
	                                  &mate_output_manager_v1_interface, 1);
	mate_output_manager_v1_add_listener (info->manager, &manager_listener,
	                                     info);
}

static void
registry_global_remove (void *data, struct wl_registry *registry,
                        uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_global,
	registry_global_remove,
};

static struct wl_display *
wayland_display (MateMonitorInfo *info)
{
	if (!GDK_IS_WAYLAND_DISPLAY (info->display))
		return NULL;
	return gdk_wayland_display_get_wl_display (info->display);
}

#endif

static void
track_monitor (MateMonitorInfo *info, GdkMonitor *monitor)
{
	Entry *entry;

	if (g_hash_table_contains (info->entries, monitor))
		return;
	entry = g_new0 (Entry, 1);
	entry->info = info;
	entry->monitor = monitor;
	entry->owner = MATE_BACKGROUND_OWNER_UNKNOWN;
	g_hash_table_insert (info->entries, monitor, entry);
#ifdef HAVE_WAYLAND
	if (info->manager && GDK_IS_WAYLAND_MONITOR (monitor)) {
		struct wl_output *output = gdk_wayland_monitor_get_wl_output (monitor);

		if (output) {
			entry->proxy = mate_output_manager_v1_get_output (info->manager,
			                                                  output);
			mate_output_v1_add_listener (entry->proxy, &output_listener,
			                             entry);
		}
	}
#endif
}

static void
on_monitor_added (GdkDisplay *display, GdkMonitor *monitor,
                  MateMonitorInfo *info)
{
	track_monitor (info, monitor);
	g_signal_emit (info, signals[CHANGED], 0, monitor);
}

static void
on_monitor_removed (GdkDisplay *display, GdkMonitor *monitor,
                    MateMonitorInfo *info)
{
	g_hash_table_remove (info->entries, monitor);
	g_signal_emit (info, signals[CHANGED], 0, NULL);
}

static void
mate_monitor_info_dispose (GObject *object)
{
	MateMonitorInfo *info = MATE_MONITOR_INFO (object);

	if (info->display) {
		g_signal_handlers_disconnect_by_data (info->display, info);
		info->display = NULL;
	}
	g_clear_pointer (&info->entries, g_hash_table_destroy);
#ifdef HAVE_WAYLAND
	g_clear_pointer (&info->manager, mate_output_manager_v1_destroy);
	g_clear_pointer (&info->registry, wl_registry_destroy);
#endif
	G_OBJECT_CLASS (mate_monitor_info_parent_class)->dispose (object);
}

static void
mate_monitor_info_class_init (MateMonitorInfoClass *klass)
{
	G_OBJECT_CLASS (klass)->dispose = mate_monitor_info_dispose;

	/**
	 * MateMonitorInfo::changed:
	 * @info: the object
	 * @monitor: (nullable): the monitor whose facts changed, or %NULL
	 *   when monitors went away or the capabilities changed
	 */
	signals[CHANGED] = g_signal_new ("changed", G_TYPE_FROM_CLASS (klass),
	                                 G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
	                                 G_TYPE_NONE, 1, GDK_TYPE_MONITOR);
}

static void
mate_monitor_info_init (MateMonitorInfo *info)
{
	info->entries = g_hash_table_new_full (NULL, NULL, NULL, entry_free);
}

static MateMonitorInfo *
monitor_info_new (GdkDisplay *display)
{
	MateMonitorInfo *info = g_object_new (MATE_TYPE_MONITOR_INFO, NULL);
	int n;

	info->display = display;
#ifdef HAVE_WAYLAND
	struct wl_display *wl = wayland_display (info);

	if (wl) {
		info->registry = wl_display_get_registry (wl);
		wl_registry_add_listener (info->registry, &registry_listener, info);
		wl_display_roundtrip (wl);
	}
#endif
	n = gdk_display_get_n_monitors (display);
	for (int i = 0; i < n; i++)
		track_monitor (info, gdk_display_get_monitor (display, i));
#ifdef HAVE_WAYLAND
	if (wl && info->manager)
		wl_display_roundtrip (wl);
#endif
	g_signal_connect (display, "monitor-added",
	                  G_CALLBACK (on_monitor_added), info);
	g_signal_connect (display, "monitor-removed",
	                  G_CALLBACK (on_monitor_removed), info);
	return info;
}

/**
 * mate_monitor_info_get_for_display:
 * @display: a #GdkDisplay
 *
 * Returns: (transfer none): the #MateMonitorInfo of @display, created on
 *   first use and kept as long as the display
 */
MateMonitorInfo *
mate_monitor_info_get_for_display (GdkDisplay *display)
{
	MateMonitorInfo *info;

	g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
	info = g_object_get_data (G_OBJECT (display), "mate-monitor-info");
	if (!info) {
		info = monitor_info_new (display);
		g_object_set_data_full (G_OBJECT (display), "mate-monitor-info",
		                        info, g_object_unref);
	}
	return info;
}

/**
 * mate_monitor_info_get_default:
 *
 * Returns: (transfer none): the #MateMonitorInfo of the default display
 */
MateMonitorInfo *
mate_monitor_info_get_default (void)
{
	return mate_monitor_info_get_for_display (gdk_display_get_default ());
}

static Entry *
entry_of (MateMonitorInfo *info, GdkMonitor *monitor)
{
	Entry *entry;

	track_monitor (info, monitor);
	entry = g_hash_table_lookup (info->entries, monitor);
	return entry && entry->has_state ? entry : NULL;
}

/**
 * mate_monitor_info_has_protocol:
 * @info: a #MateMonitorInfo
 *
 * Returns: whether the compositor offers mate_output_manager_v1
 */
gboolean
mate_monitor_info_has_protocol (MateMonitorInfo *info)
{
	g_return_val_if_fail (MATE_IS_MONITOR_INFO (info), FALSE);
#ifdef HAVE_WAYLAND
	return info->manager != NULL;
#else
	return FALSE;
#endif
}

/**
 * mate_monitor_info_has_capability:
 * @info: a #MateMonitorInfo
 * @capability: a #MateMonitorCapability
 *
 * Returns: whether the compositor reports @capability; always %FALSE
 *   without mate_output_manager_v1
 */
gboolean
mate_monitor_info_has_capability (MateMonitorInfo       *info,
                                  MateMonitorCapability  capability)
{
	g_return_val_if_fail (MATE_IS_MONITOR_INFO (info), FALSE);
	return (info->capabilities & capability) == (guint) capability;
}

static void
append_id_part (GString *id, const char *text)
{
	for (const char *c = text; c && *c; c++)
		g_string_append_c (id, g_ascii_isalnum (*c) || *c == '_' ||
		                       *c == '.' || *c == '-' ? *c : '_');
}

static gchar *
base_id (GdkMonitor *monitor)
{
	const char *make = gdk_monitor_get_manufacturer (monitor);
	const char *model = gdk_monitor_get_model (monitor);
	GString *id = g_string_new (NULL);

	append_id_part (id, make);
	if (make && *make && model && *model)
		g_string_append_c (id, '-');
	append_id_part (id, model);
	if (id->len == 0)
		g_string_assign (id, "monitor");
	return g_string_free (id, FALSE);
}

static int
compare_position (GdkMonitor *a, int index_a, GdkMonitor *b, int index_b)
{
	GdkRectangle ra, rb;

	gdk_monitor_get_geometry (a, &ra);
	gdk_monitor_get_geometry (b, &rb);
	if (ra.x != rb.x)
		return ra.x < rb.x ? -1 : 1;
	if (ra.y != rb.y)
		return ra.y < rb.y ? -1 : 1;
	if (ra.width != rb.width)
		return ra.width < rb.width ? -1 : 1;
	if (ra.height != rb.height)
		return ra.height < rb.height ? -1 : 1;
	return index_a == index_b ? 0 : (index_a < index_b ? -1 : 1);
}

static gchar *
fallback_id (GdkMonitor *monitor)
{
	GdkDisplay *display = gdk_monitor_get_display (monitor);
	gchar *base = base_id (monitor);
	int n = gdk_display_get_n_monitors (display);
	int own = -1, duplicates = 0;

	for (int i = 0; i < n; i++) {
		if (gdk_display_get_monitor (display, i) == monitor)
			own = i;
	}
	for (int i = 0; i < n && own >= 0; i++) {
		GdkMonitor *other = gdk_display_get_monitor (display, i);
		gchar *other_base;

		if (other == monitor)
			continue;
		other_base = base_id (other);
		if (g_strcmp0 (base, other_base) == 0 &&
		    compare_position (other, i, monitor, own) < 0)
			duplicates++;
		g_free (other_base);
	}
	if (duplicates > 0) {
		gchar *id = g_strdup_printf ("%s+%d", base, duplicates);

		g_free (base);
		return id;
	}
	return base;
}

/**
 * mate_monitor_info_get_id:
 * @info: a #MateMonitorInfo
 * @monitor: a #GdkMonitor
 *
 * The id per-monitor settings are stored under, such as the
 * org.mate.background.monitor path. It is the same string the compositor
 * uses, and for a monitor whose make and model are unique it is the same
 * with or without the protocol.
 *
 * Returns: (transfer full): the monitor id
 */
gchar *
mate_monitor_info_get_id (MateMonitorInfo *info, GdkMonitor *monitor)
{
	Entry *entry;

	g_return_val_if_fail (MATE_IS_MONITOR_INFO (info), NULL);
	g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);
	entry = entry_of (info, monitor);
	if (entry && entry->id && *entry->id)
		return g_strdup (entry->id);
	return fallback_id (monitor);
}

/**
 * mate_monitor_info_get_display_name:
 * @info: a #MateMonitorInfo
 * @monitor: a #GdkMonitor
 *
 * Returns: (transfer full): a name to show the user
 */
gchar *
mate_monitor_info_get_display_name (MateMonitorInfo *info, GdkMonitor *monitor)
{
	Entry *entry;
	const char *model;

	g_return_val_if_fail (MATE_IS_MONITOR_INFO (info), NULL);
	g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);
	entry = entry_of (info, monitor);
	if (entry && entry->display_name && *entry->display_name)
		return g_strdup (entry->display_name);
	model = gdk_monitor_get_model (monitor);
	if (model && *model)
		return g_strdup (model);
	return g_strdup (gdk_monitor_get_manufacturer (monitor));
}

/**
 * mate_monitor_info_is_internal:
 * @info: a #MateMonitorInfo
 * @monitor: a #GdkMonitor
 *
 * Returns: whether @monitor is a built-in panel; %FALSE when not known
 */
gboolean
mate_monitor_info_is_internal (MateMonitorInfo *info, GdkMonitor *monitor)
{
	Entry *entry;

	g_return_val_if_fail (MATE_IS_MONITOR_INFO (info), FALSE);
	entry = entry_of (info, monitor);
	return entry ? entry->internal : FALSE;
}

/**
 * mate_monitor_info_is_primary:
 * @info: a #MateMonitorInfo
 * @monitor: a #GdkMonitor
 *
 * Returns: whether @monitor is the primary monitor
 */
gboolean
mate_monitor_info_is_primary (MateMonitorInfo *info, GdkMonitor *monitor)
{
	Entry *entry;

	g_return_val_if_fail (MATE_IS_MONITOR_INFO (info), FALSE);
	entry = entry_of (info, monitor);
	return entry ? entry->primary : gdk_monitor_is_primary (monitor);
}

/**
 * mate_monitor_info_get_mirror_of:
 * @info: a #MateMonitorInfo
 * @monitor: a #GdkMonitor
 *
 * Returns: (transfer full) (nullable): the id of the monitor whose picture
 *   @monitor shows, or %NULL when it mirrors nothing or this is not known
 */
gchar *
mate_monitor_info_get_mirror_of (MateMonitorInfo *info, GdkMonitor *monitor)
{
	Entry *entry;

	g_return_val_if_fail (MATE_IS_MONITOR_INFO (info), NULL);
	entry = entry_of (info, monitor);
	if (entry && entry->mirror_of && *entry->mirror_of)
		return g_strdup (entry->mirror_of);
	return NULL;
}

/**
 * mate_monitor_info_get_workarea:
 * @info: a #MateMonitorInfo
 * @monitor: a #GdkMonitor
 * @workarea: (out): the work area, in the same coordinates as
 *   gdk_monitor_get_geometry()
 *
 * The part of the monitor panels leave free.
 */
void
mate_monitor_info_get_workarea (MateMonitorInfo *info, GdkMonitor *monitor,
                                GdkRectangle *workarea)
{
	Entry *entry;
	GdkRectangle geometry;

	g_return_if_fail (MATE_IS_MONITOR_INFO (info));
	g_return_if_fail (workarea != NULL);
	entry = entry_of (info, monitor);
	if (!entry || !entry->has_workarea) {
		gdk_monitor_get_workarea (monitor, workarea);
		return;
	}
	gdk_monitor_get_geometry (monitor, &geometry);
	*workarea = entry->workarea;
	workarea->x += geometry.x;
	workarea->y += geometry.y;
}

/**
 * mate_monitor_info_get_background_owner:
 * @info: a #MateMonitorInfo
 * @monitor: a #GdkMonitor
 *
 * Returns: who draws the desktop background on @monitor now
 */
MateBackgroundOwner
mate_monitor_info_get_background_owner (MateMonitorInfo *info,
                                        GdkMonitor      *monitor)
{
	Entry *entry;

	g_return_val_if_fail (MATE_IS_MONITOR_INFO (info),
	                      MATE_BACKGROUND_OWNER_UNKNOWN);
	entry = entry_of (info, monitor);
	return entry ? entry->owner : MATE_BACKGROUND_OWNER_UNKNOWN;
}
