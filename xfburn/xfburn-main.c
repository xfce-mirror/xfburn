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
#include "xfburn-burn-image-dialog.h"
#include "xfburn-main-window.h"
#include "xfburn-blank-dialog.h"
#include "xfburn-hal-manager.h"


/* internal prototypes */
static gboolean parse_option (const gchar *option_name, const gchar *value,
			      gpointer data, GError **error);

/* command line parameters */
static gchar *image_filename = NULL;
static gboolean show_version = FALSE;
static gboolean other_action = FALSE;
static gboolean show_main = FALSE;
static gboolean add_data_composition = FALSE;
static gboolean add_audio_composition = FALSE;
static gboolean blank = FALSE;

static GOptionEntry optionentries[] = {
  { "burn-image", 'i', G_OPTION_FLAG_OPTIONAL_ARG /* || G_OPTION_FLAG_FILENAME */, G_OPTION_ARG_CALLBACK, &parse_option, 
    "Open the burn image dialog. The filename of the image can optionally be specified as a parameter", NULL },
  { "blank", 'b', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &parse_option, 
    "Open the blank disc dialog.", NULL },
  { "data-composition", 'd', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &parse_option, 
    "Start a data composition. Optionally followed by files/directories to be added to the composition.", NULL },
  { "audio-composition", 'a', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &parse_option, 
    "Start an audio composition. Optionally followed by files/directories to be added to the composition.", NULL },
  { "version", 'V', G_OPTION_FLAG_NO_ARG , G_OPTION_ARG_NONE, &show_version, 
    "Display program version and exit", NULL },
  { "main", 'm', G_OPTION_FLAG_NO_ARG , G_OPTION_ARG_NONE, &show_main, 
    "Show main program even when other action is specified on the command line.", NULL },
  { NULL },
};

static gboolean parse_option (const gchar *option_name, const gchar *value,
                              gpointer data, GError **error)
{
  if (strcmp (option_name, "-i") == 0 || strcmp (option_name, "--burn-image") == 0) {
    if (value == NULL)
      image_filename = "";
    else
      image_filename = g_strdup(value);
  } else if (strcmp (option_name, "-d") == 0 || strcmp (option_name, "--data-composition") == 0) {
    add_data_composition = TRUE;
  } else if (strcmp (option_name, "-a") == 0 || strcmp (option_name, "--audio-composition") == 0) {
    add_audio_composition = TRUE;
  } else if (strcmp (option_name, "-b") == 0 || strcmp (option_name, "--blank") == 0) {
    blank = TRUE;
  } else {
    g_set_error (error, 0, G_OPTION_ERROR_FAILED, "Invalid command line option. Please report, this is a bug.");
    return FALSE;
  }

  return TRUE;
}

/* globals */
static int window_counter = 0;

void
xfburn_main_enter_window ()
{
  /* if a main window is present, then it is in control */
  if (window_counter != -42)
    window_counter++;
}

void 
xfburn_main_leave_window ()
{
  /* if a main window is present, then it is in control */
  if (window_counter == -42)
    return;

  window_counter--;
  if (window_counter <= 0)
    g_idle_add ((GSourceFunc) gtk_main_quit, NULL );
}

void
xfburn_main_enter_main_window ()
{
  /* mark the window_counter as having a main window */
  window_counter = -42;
}


/* entry point */
int
main (int argc, char **argv)
{
  GtkWidget *mainwin;
  gint n_drives;
  GError *error = NULL;
  gchar *error_msg;

#if DEBUG > 0
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
#endif
  
  g_set_application_name (_("Xfburn"));

  g_thread_init (NULL);
  gdk_threads_init ();
  gdk_threads_enter ();

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  if (!gtk_init_with_args (&argc, &argv, "", optionentries, GETTEXT_PACKAGE, &error)) {
    if (error != NULL) {
      g_print (_("%s: %s\nTry %s --help to see a full list of available command line options.\n"), PACKAGE, error->message, PACKAGE_NAME);
      g_error_free (error);
      return 1;
    }
  }

  if (show_version) {
    g_print ("\tThis is %s version %s for Xfce %s\n", PACKAGE, VERSION, xfce_version_string ());
    g_print ("\tbuilt with GTK+-%d.%d.%d, ", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    g_print ("linked with GTK+-%d.%d.%d.\n", gtk_major_version, gtk_minor_version, gtk_micro_version);

    exit (EXIT_SUCCESS);
  }


  xfburn_settings_init ();
  
#ifdef HAVE_THUNAR_VFS
  thunar_vfs_init ();
  g_message ("Using Thunar-VFS %d.%d.%d", THUNAR_VFS_MAJOR_VERSION, THUNAR_VFS_MINOR_VERSION, THUNAR_VFS_MICRO_VERSION);
#else
  g_message ("Thunar-VFS not available, using default implementation");
#endif
  
#ifdef HAVE_HAL
  error_msg = xfburn_hal_manager_create_global ();
  if (error_msg) {
    xfce_err (error_msg);
#ifdef HAVE_THUNAR_VFS
    thunar_vfs_shutdown ();
#endif
    gdk_threads_leave ();
    return EXIT_FAILURE;
  }
#endif

  xfburn_stock_init ();
  n_drives = xfburn_device_list_init ();
#if 0
#ifdef HAVE_THUNAR_VFS
  if (n_drives < 1) {
    //XfburnHalManager *halman = xfburn_hal_manager_get_instance();
    ThunarVfsVolumeManager *volman;
    GList *volume_list;
    gboolean unmounted = FALSE;
    gchar *mp_name;

    volman = thunar_vfs_volume_manager_get_default();
    volume_list = thunar_vfs_volume_manager_get_volumes (volman);

    while (volume_list) {
      ThunarVfsVolume *vol = THUNAR_VFS_VOLUME (volume_list->data);
      ThunarVfsPath *mp;

      switch (thunar_vfs_volume_get_kind (vol)) {
        case THUNAR_VFS_VOLUME_KIND_CDROM:
        case THUNAR_VFS_VOLUME_KIND_CDR:
        case THUNAR_VFS_VOLUME_KIND_DVDROM:
        case THUNAR_VFS_VOLUME_KIND_DVDRAM:
        case THUNAR_VFS_VOLUME_KIND_DVDR:
        case THUNAR_VFS_VOLUME_KIND_DVDRW:
        case THUNAR_VFS_VOLUME_KIND_DVDPLUSR:
        case THUNAR_VFS_VOLUME_KIND_DVDPLUSRW:
        case THUNAR_VFS_VOLUME_KIND_AUDIO_CD:
          if (thunar_vfs_volume_is_mounted (vol)) {
            mp = thunar_vfs_volume_get_mount_point (vol);
            mp_name = thunar_vfs_path_dup_string (mp);
            if (thunar_vfs_volume_unmount (vol, NULL, NULL)) {
              unmounted = TRUE;
              g_message ("Unmounted drive %s", mp_name);
            } else {
              g_message ("Failed to unmounted drive %s", mp_name);
            }
            g_free (mp_name);
          }
          break;
        default:
          break;
      }

      volume_list = g_list_next (volume_list);
    }

    g_object_unref (volman);

    if (unmounted)
      n_drives = xfburn_device_list_init ();
    else
      g_debug ("Could not umount any optical drives.");
  }
#endif
#endif

  if (n_drives < 1) {
    GtkMessageDialog *dialog = (GtkMessageDialog *) gtk_message_dialog_new (NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_CLOSE,
                                    ((const gchar *) _("No drives are currently available!")));
    gtk_message_dialog_format_secondary_text (dialog,
                                    _("Maybe there is a mounted media in the drive?\n\nPlease unmount and restart the application.\n\nIf no media is inserted, check that you have r/w access to the drive with the current user."));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (GTK_WIDGET (dialog));
  }


  /* evaluate parsed command line options */

  if (show_main) {
    xfburn_main_enter_main_window ();
  }

  if (image_filename != NULL) {
    GtkWidget *dialog = xfburn_burn_image_dialog_new ();
    other_action = TRUE;

    DBG ("image_filename = '%s'\n", image_filename);

    if (*image_filename != '\0') {
      gchar *image_fullname;

      if (!g_path_is_absolute (image_filename))
	image_fullname  = g_build_filename (g_get_current_dir (), image_filename, NULL);
      else
	image_fullname = image_filename;

      if (g_file_test (image_fullname, G_FILE_TEST_EXISTS))
	xfburn_burn_image_dialog_set_filechooser_name (dialog, image_fullname);
      else
	xfce_err ( g_strdup_printf ( _("Image file '%s' does not exist!"), image_fullname));
    }

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  } else if (blank) {
    GtkWidget *dialog = xfburn_blank_dialog_new ();

    other_action = TRUE;
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }


  /* main window */
  if (!other_action || show_main) {
    xfburn_main_enter_main_window ();
    mainwin = xfburn_main_window_new ();

    gtk_widget_show (mainwin);
  
    if (add_data_composition)
      xfburn_main_window_add_data_composition_with_files (XFBURN_MAIN_WINDOW (mainwin), argc-1, argv+1);

    if (add_audio_composition)
      xfburn_main_window_add_audio_composition_with_files (XFBURN_MAIN_WINDOW (mainwin), argc-1, argv+1);
  }

  gtk_main ();

  /* shutdown */
#ifdef HAVE_THUNAR_VFS
  thunar_vfs_shutdown ();
#endif
  
#ifdef HAVE_HAL
  xfburn_hal_manager_shutdown ();
#endif

  xfburn_settings_flush ();
  xfburn_settings_free ();
  
  xfburn_device_list_free ();

  gdk_threads_leave ();

  return EXIT_SUCCESS;
}
