/*
 * Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
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

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-directory-browser.h"

/* prototypes */
static void xfburn_directory_browser_class_init (XfburnDirectoryBrowserClass *);
static void xfburn_directory_browser_init (XfburnDirectoryBrowser *);

/* globals */
static GtkTreeViewClass *parent_class = NULL;

/***************************/
/* XfburnDirectoryBrowser class */
/***************************/
GtkType
xfburn_directory_browser_get_type (void)
{
  static GtkType directory_browser_type = 0;

  if (!directory_browser_type) {
    static const GTypeInfo directory_browser_info = {
      sizeof (XfburnDirectoryBrowserClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_directory_browser_class_init,
      NULL,
      NULL,
      sizeof (XfburnDirectoryBrowser),
      0,
      (GInstanceInitFunc) xfburn_directory_browser_init
    };

    directory_browser_type = g_type_register_static (GTK_TYPE_TREE_VIEW, "XfburnDirectoryBrowser", &directory_browser_info, 0);
  }

  return directory_browser_type;
}

static void
xfburn_directory_browser_class_init (XfburnDirectoryBrowserClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_directory_browser_init (XfburnDirectoryBrowser * directory_browser)
{
  
}

/* public methods */
GtkWidget *
xfburn_directory_browser_new (void)
{
  return g_object_new (xfburn_directory_browser_get_type (), NULL);
}
