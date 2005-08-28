/*
 * Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XFBURN_UTILS_H__
#define __XFBURN_UTILS_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <linux/cdrom.h>

#include "xfburn-global.h"

void xfburn_busy_cursor (GtkWidget *);
void xfburn_default_cursor (GtkWidget *);

void xfburn_device_content_free (XfburnDevice * device, gpointer user_data);
void xfburn_device_free (XfburnDevice * device);
XfburnDevice * xfburn_device_lookup_by_name (const gchar * name);
gint xfburn_device_query_cdstatus (XfburnDevice * device);

void xfburn_scan_devices ();

gchar *xfburn_humanreadable_filesize (guint64);
guint64 xfburn_calc_dirsize (const gchar *);

void xfburn_browse_for_file (GtkEntry *entry, GtkWindow *parent);
#endif
