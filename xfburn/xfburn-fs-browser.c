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

#include "xfburn-fs-browser.h"

/* prototypes */
static void xfburn_fs_browser_class_init (XfburnFsBrowserClass * klass);
static void xfburn_fs_browser_init (XfburnFsBrowser * sp);

/* globals */
static GtkTreeViewClass *parent_class = NULL;

GType
xfburn_fs_browser_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnFsBrowserClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_fs_browser_class_init,
      NULL,
      NULL,
      sizeof (XfburnFsBrowser),
      0,
      (GInstanceInitFunc) xfburn_fs_browser_init,
    };

    type = g_type_register_static (GTK_TYPE_TREE_VIEW, "XfburnFsBrowser", &our_info, 0);
  }

  return type;
}

static void
xfburn_fs_browser_class_init (XfburnFsBrowserClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);

}

static void
xfburn_fs_browser_init (XfburnFsBrowser * obj)
{

}

GtkWidget *
xfburn_fs_browser_new ()
{
  return g_object_new (XFBURN_TYPE_FS_BROWSER, NULL);
}
