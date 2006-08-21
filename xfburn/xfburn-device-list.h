/* $Id$ */
/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#ifndef __XFBURN_DEVICE_LIST_H__
#define __XFBURN_DEVICE_LIST_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef struct
{
  gchar *name;
  gchar *id;
  gchar *node_path;
  gboolean cdr;
  gboolean cdrw;
  gboolean dvdr;
  gboolean dvdram;
} XfburnDevice;

void xfburn_device_list_init ();
XfburnDevice * xfburn_device_lookup_by_name (const gchar * name);
GList * xfburn_device_list_get_list ();
void xfburn_device_list_free ();

gint xfburn_device_query_cdstatus (XfburnDevice * device);
gchar * xfburn_device_cdstatus_to_string (gint status);

void xfburn_device_free (XfburnDevice * device);

#endif /* __XFBURN_DEVICE_LIST_H__ */
