/* $Id$ */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <stdlib.h>

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#ifdef HAVE_THUNAR_VFS
#include <thunar-vfs/thunar-vfs.h>
#endif

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-stock.h"
#include "xfburn-main-window.h"

/* globals */
GList *list_devices = NULL;

/* entry point */
int
main (int argc, char **argv)
{
  GtkWidget *mainwin;

#if DEBUG > 0
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
#endif
  
  g_set_application_name (_("Xfburn"));

  if (argc > 1 && (!strcmp (argv[1], "--version") || !strcmp (argv[1], "-V"))) {
    g_print ("\tThis is %s version %s for Xfce %s\n", PACKAGE, VERSION, xfce_version_string ());
    g_print ("\tbuilt with GTK+-%d.%d.%d, ", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    g_print ("linked with GTK+-%d.%d.%d.\n", gtk_major_version, gtk_minor_version, gtk_micro_version);

    exit (EXIT_SUCCESS);
  }

  gtk_init (&argc, &argv);

  xfburn_settings_init ();
  
#ifdef HAVE_THUNAR_VFS
  thunar_vfs_init ();
  g_message ("Using Thunar-VFS %d.%d.%d", THUNAR_VFS_MAJOR_VERSION, THUNAR_VFS_MINOR_VERSION, THUNAR_VFS_MICRO_VERSION);
#else
  g_message ("Thunar-VFS not available, using default implementation");
#endif
  
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  xfburn_stock_init ();
  xfburn_scan_devices ();
  mainwin = xfburn_main_window_new ();

  gtk_widget_show (mainwin);
  
  gtk_main ();
  
#ifdef HAVE_THUNAR_VFS
  thunar_vfs_shutdown ();
#endif
  
  xfburn_settings_flush ();
  xfburn_settings_free ();
  
  g_list_foreach (list_devices, (GFunc) xfburn_device_content_free, NULL);
  g_list_free (list_devices);
  return EXIT_SUCCESS;
}
