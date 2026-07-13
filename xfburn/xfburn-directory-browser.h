/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XFBURN_DIRECTORY_BROWSER_H__
#define __XFBURN_DIRECTORY_BROWSER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_DIRECTORY_BROWSER (xfburn_directory_browser_get_type ())
G_DECLARE_FINAL_TYPE (XfburnDirectoryBrowser, xfburn_directory_browser, XFBURN, DIRECTORY_BROWSER, GtkTreeView)

enum
{
  DIRECTORY_BROWSER_COLUMN_ICON,
  DIRECTORY_BROWSER_COLUMN_FILE,
  DIRECTORY_BROWSER_COLUMN_HUMANSIZE,
  DIRECTORY_BROWSER_COLUMN_SIZE,
  DIRECTORY_BROWSER_COLUMN_TYPE,
  DIRECTORY_BROWSER_COLUMN_PATH,
  DIRECTORY_BROWSER_N_COLUMNS
};

GtkWidget *xfburn_directory_browser_new (void);

void xfburn_directory_browser_load_path (XfburnDirectoryBrowser * browser, const gchar * path);
void xfburn_directory_browser_refresh (XfburnDirectoryBrowser * browser);

gchar *xfburn_directory_browser_get_selection (XfburnDirectoryBrowser * browser);

G_END_DECLS

#endif
