/* mate-monitor-info.h - what MATE needs to know about each monitor
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

#ifndef MATE_MONITOR_INFO_H
#define MATE_MONITOR_INFO_H

#ifndef MATE_DESKTOP_USE_UNSTABLE_API
#error    MateMonitorInfo is unstable API. You must define MATE_DESKTOP_USE_UNSTABLE_API before including mate-monitor-info.h
#endif

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define MATE_TYPE_MONITOR_INFO (mate_monitor_info_get_type ())
G_DECLARE_FINAL_TYPE (MateMonitorInfo, mate_monitor_info, MATE, MONITOR_INFO, GObject)

/**
 * MateMonitorCapability:
 * @MATE_MONITOR_CAPABILITY_WALLPAPER: the compositor draws org.mate.background
 * @MATE_MONITOR_CAPABILITY_PER_OUTPUT_WALLPAPER: it draws an output's own
 *   org.mate.background.monitor settings
 * @MATE_MONITOR_CAPABILITY_SPANNED_WALLPAPER: it spans pictures over the
 *   outputs that share the settings
 *
 * What the compositor draws, from mate_output_manager_v1. None without it.
 */
typedef enum {
	MATE_MONITOR_CAPABILITY_WALLPAPER = 1 << 0,
	MATE_MONITOR_CAPABILITY_PER_OUTPUT_WALLPAPER = 1 << 1,
	MATE_MONITOR_CAPABILITY_SPANNED_WALLPAPER = 1 << 2,
} MateMonitorCapability;

/**
 * MateBackgroundOwner:
 * @MATE_BACKGROUND_OWNER_UNKNOWN: the compositor does not say
 * @MATE_BACKGROUND_OWNER_NONE: nobody draws a background
 * @MATE_BACKGROUND_OWNER_CLIENT: a client's desktop surface draws it
 * @MATE_BACKGROUND_OWNER_COMPOSITOR: the compositor draws it
 */
typedef enum {
	MATE_BACKGROUND_OWNER_UNKNOWN = -1,
	MATE_BACKGROUND_OWNER_NONE = 0,
	MATE_BACKGROUND_OWNER_CLIENT = 1,
	MATE_BACKGROUND_OWNER_COMPOSITOR = 2,
} MateBackgroundOwner;

MateMonitorInfo     *mate_monitor_info_get_default         (void);
MateMonitorInfo     *mate_monitor_info_get_for_display     (GdkDisplay            *display);

gboolean             mate_monitor_info_has_protocol        (MateMonitorInfo       *info);
gboolean             mate_monitor_info_has_capability      (MateMonitorInfo       *info,
                                                            MateMonitorCapability  capability);

gchar               *mate_monitor_info_get_id              (MateMonitorInfo       *info,
                                                            GdkMonitor            *monitor);
gchar               *mate_monitor_info_get_display_name    (MateMonitorInfo       *info,
                                                            GdkMonitor            *monitor);
gboolean             mate_monitor_info_is_internal         (MateMonitorInfo       *info,
                                                            GdkMonitor            *monitor);
gboolean             mate_monitor_info_is_primary          (MateMonitorInfo       *info,
                                                            GdkMonitor            *monitor);
gchar               *mate_monitor_info_get_mirror_of       (MateMonitorInfo       *info,
                                                            GdkMonitor            *monitor);
void                 mate_monitor_info_get_workarea        (MateMonitorInfo       *info,
                                                            GdkMonitor            *monitor,
                                                            GdkRectangle          *workarea);
MateBackgroundOwner  mate_monitor_info_get_background_owner (MateMonitorInfo      *info,
                                                            GdkMonitor            *monitor);

G_END_DECLS

#endif
