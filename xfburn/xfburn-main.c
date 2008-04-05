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
#include "xfburn-device-list.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-stock.h"
#include "xfburn-main-window.h"

/* entry point */
int
main (int argc, char **argv)
{
  GtkWidget *mainwin;
  gint n_drives;

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

  g_thread_init (NULL);
  gdk_threads_init ();
  gdk_threads_enter ();

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
  n_drives = xfburn_device_list_init ();
  if (n_drives < 1) {
    GtkMessageDialog *dialog = (GtkMessageDialog *) gtk_message_dialog_new (NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_CLOSE,
                                    ((const gchar *) _("No drives are currently available!")));
    gtk_message_dialog_format_secondary_text (dialog,
                                    _("Maybe there is a mounted media in the drive?\n\nPlease unmount and restart the application."));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (GTK_WIDGET (dialog));
  }

  mainwin = xfburn_main_window_new ();

  gtk_widget_show (mainwin);
  
  gtk_main ();

#ifdef HAVE_THUNAR_VFS
  thunar_vfs_shutdown ();
#endif
  
  xfburn_settings_flush ();
  xfburn_settings_free ();
  
  xfburn_device_list_free ();

  gdk_threads_leave ();

  return EXIT_SUCCESS;
}
