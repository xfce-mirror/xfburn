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

#include "xfburn-filebrowser.h"

/* prototypes */
static void xfburn_file_browser_class_init (XfburnFileBrowserClass *);
static void xfburn_file_browser_init (XfburnFileBrowser *);

/* globals */
static GtkHPanedClass *parent_class = NULL;

/***************************/
/* XfburnFileBrowser class */
/***************************/
GtkType
xfburn_file_browser_get_type (void)
{
  static GtkType file_browser_type = 0;

  if (!file_browser_type) {
    static const GTypeInfo file_browser_info = {
      sizeof (XfburnFileBrowserClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_file_browser_class_init,
      NULL,
      NULL,
      sizeof (XfburnFileBrowser),
      0,
      (GInstanceInitFunc) xfburn_file_browser_init
    };

    file_browser_type = g_type_register_static (GTK_TYPE_HPANED, "XfburnFileBrowser", &file_browser_info, 0);
  }

  return file_browser_type;
}

static void
xfburn_file_browser_class_init (XfburnFileBrowserClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_file_browser_init (XfburnFileBrowser * file_browser)
{
  GtkWidget *hpaned;
  GtkWidget *scrolled_window;
    
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_paned_add1 (GTK_PANED (file_browser), scrolled_window);
  gtk_widget_show (scrolled_window);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_paned_add2 (GTK_PANED (file_browser), scrolled_window);
  gtk_widget_show (scrolled_window);
  

}

/* public methods */
GtkWidget *
xfburn_file_browser_new (void)
{
  return g_object_new (xfburn_file_browser_get_type (), NULL);
}
