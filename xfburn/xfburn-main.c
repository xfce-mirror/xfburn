$Id$
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

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-main-window.h"

/* globals */
static GtkIconFactory *icon_factory = NULL;

GList *list_devices = NULL;

/* internals */
static void
xfburn_setup_icons (void)
{
  GtkIconSource *source = NULL;
  GtkIconSet *set = NULL;
  GdkPixbuf *icon = NULL;

  if (icon_factory)
    return;

  icon_factory = gtk_icon_factory_new ();

  icon = xfce_pixbuf_new_from_file_at_size (DATADIR "/pixmaps/baker-audio-copy.png", 24, 24, NULL);
  if (icon) {
    set = gtk_icon_set_new ();
    source = gtk_icon_source_new ();
    gtk_icon_source_set_pixbuf (source, icon);
    gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
    gtk_icon_set_add_source (set, source);
    g_object_unref (G_OBJECT (icon));
    gtk_icon_factory_add (icon_factory, "xfburn-audio-copy", set);
    gtk_icon_set_unref (set);
  }

  icon = xfce_pixbuf_new_from_file_at_size (DATADIR "/pixmaps/baker-blank-cdrw.png", 24, 24, NULL);
  if (icon) {
    set = gtk_icon_set_new ();
    source = gtk_icon_source_new ();
    gtk_icon_source_set_pixbuf (source, icon);
    gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
    gtk_icon_set_add_source (set, source);
    g_object_unref (G_OBJECT (icon));
    gtk_icon_factory_add (icon_factory, "xfburn-blank-cdrw", set);
    gtk_icon_set_unref (set);
  }

  icon = xfce_pixbuf_new_from_file_at_size (DATADIR "/pixmaps/baker-blank-dvdrw.png", 24, 24, NULL);
  if (icon) {
    set = gtk_icon_set_new ();
    source = gtk_icon_source_new ();
    gtk_icon_source_set_pixbuf (source, icon);
    gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
    gtk_icon_set_add_source (set, source);
    g_object_unref (G_OBJECT (icon));
    gtk_icon_factory_add (icon_factory, "xfburn-blank-dvdrw", set);
    gtk_icon_set_unref (set);
  }

  icon = xfce_pixbuf_new_from_file_at_size (DATADIR "/pixmaps/baker-burn-cd.png", 24, 24, NULL);
  if (icon) {
    set = gtk_icon_set_new ();
    source = gtk_icon_source_new ();
    gtk_icon_source_set_pixbuf (source, icon);
    gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
    gtk_icon_set_add_source (set, source);
    g_object_unref (G_OBJECT (icon));
    gtk_icon_factory_add (icon_factory, "xfburn-burn-cd", set);
    gtk_icon_set_unref (set);
  }

  icon = xfce_pixbuf_new_from_file_at_size (DATADIR "/pixmaps/baker-cd.png", 24, 24, NULL);
  if (icon) {
    set = gtk_icon_set_new ();
    source = gtk_icon_source_new ();
    gtk_icon_source_set_pixbuf (source, icon);
    gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
    gtk_icon_set_add_source (set, source);
    g_object_unref (G_OBJECT (icon));
    gtk_icon_factory_add (icon_factory, "xfburn-cd", set);
    gtk_icon_set_unref (set);
  }

  icon = xfce_pixbuf_new_from_file_at_size (DATADIR "/pixmaps/baker-data-copy.png", 24, 24, NULL);
  if (icon) {
    set = gtk_icon_set_new ();
    source = gtk_icon_source_new ();
    gtk_icon_source_set_pixbuf (source, icon);
    gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
    gtk_icon_set_add_source (set, source);
    g_object_unref (G_OBJECT (icon));
    gtk_icon_factory_add (icon_factory, "xfburn-data-copy", set);
    gtk_icon_set_unref (set);
  }

  icon = xfce_pixbuf_new_from_file_at_size (DATADIR "/pixmaps/baker-import-session.png", 24, 24, NULL);
  if (icon) {
    set = gtk_icon_set_new ();
    source = gtk_icon_source_new ();
    gtk_icon_source_set_pixbuf (source, icon);
    gtk_icon_source_set_size (source, GTK_ICON_SIZE_DIALOG);
    gtk_icon_set_add_source (set, source);
    g_object_unref (G_OBJECT (icon));
    gtk_icon_factory_add (icon_factory, "xfburn-import-session", set);
    gtk_icon_set_unref (set);
  }

  gtk_icon_factory_add_default (icon_factory);
}

/* entry point */
int
main (int argc, char **argv)
{
  GtkWidget *mainwin;

  g_set_application_name (_("Xfburn"));

  if (argc > 1 && (!strcmp (argv[1], "--version") || !strcmp (argv[1], "-V"))) {
    g_print ("\tThis is %s version %s for Xfce %s\n", PACKAGE, VERSION, xfce_version_string ());
    g_print ("\tbuilt with GTK+-%d.%d.%d, ", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    g_print ("linked with GTK+-%d.%d.%d.\n", gtk_major_version, gtk_minor_version, gtk_micro_version);

    exit (EXIT_SUCCESS);
  }

  xfburn_settings_init ();
  gtk_init (&argc, &argv);

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  xfburn_setup_icons ();
  xfburn_scan_devices ();
  mainwin = xfburn_main_window_new ();

  gtk_widget_show (mainwin);

  gtk_main ();
  
  xfburn_settings_flush ();
  xfburn_settings_free ();
  
  g_list_foreach (list_devices, (GFunc) xfburn_device_content_free, NULL);
  g_list_free (list_devices);
  return EXIT_SUCCESS;
}
