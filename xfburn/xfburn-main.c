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
#include <libxfce4ui/libxfce4ui.h>

#ifdef HAVE_GST
#include <gst/gst.h>
#endif

#include "xfburn-main.h"
#include "xfburn-global.h"
#include "xfburn-device-list.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-burn-image-dialog.h"
#include "xfburn-main-window.h"
#include "xfburn-blank-dialog.h"
#include "xfburn-udev-manager.h"
#include "xfburn-transcoder-basic.h"
#include "xfburn-transcoder-gst.h"


/* internal prototypes */
static gboolean parse_option (const gchar *option_name, const gchar *value,
			      gpointer data, GError **error);
static void xfburn_main_enter_main_window (void);
static void print_available_transcoders (void);

/* globals */
static int window_counter = 0;


/* command line parameters */
static gchar *image_filename = NULL;
static gboolean show_version = FALSE;
static gboolean other_action = FALSE;
static gboolean show_main = FALSE;
static gboolean add_data_composition = FALSE;
static gboolean add_audio_composition = FALSE;
static gboolean blank = FALSE;
static gchar *transcoder_selection = NULL;
static gchar *initial_dir = NULL;

static GOptionEntry optionentries[] = {
  { "burn-image", 'i', G_OPTION_FLAG_OPTIONAL_ARG /* || G_OPTION_FLAG_FILENAME */, G_OPTION_ARG_CALLBACK, &parse_option,
    N_("Open the burn image dialog, optionally followed by the image filename"), NULL },
  { "blank", 'b', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &parse_option,
    N_("Open the blank disc dialog"), NULL },
  { "data-composition", 'd', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &parse_option,
    N_("Start a data composition, optionally followed by files/directories to be added to the composition"), NULL },
  { "audio-composition", 'a', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &parse_option,
    N_("Start an audio composition, optionally followed by files/directories to be added to the composition"), NULL },
  { "transcoder", 't', 0, G_OPTION_ARG_STRING, &transcoder_selection,
    N_("Select the transcoder, run with --transcoder=list to see the available ones"), NULL },
  { "directory", 'D', G_OPTION_FLAG_OPTIONAL_ARG , G_OPTION_ARG_CALLBACK, &parse_option,
    N_("Start the file browser in the specified directory, or the current directory if none is specified (the default is to start in your home directory)"), NULL },
  { "version", 'V', 0 , G_OPTION_ARG_NONE, &show_version,
    N_("Display program version and exit"), NULL },
  { "main", 'm', 0, G_OPTION_ARG_NONE, &show_main,
    N_("Show main program even when other action is specified on the command line."), NULL },
  { NULL, ' ', 0, 0, NULL, NULL, NULL }
};


/* public functions */
void
xfburn_main_enter_window (void)
{
  /* if a main window is present, then it is in control */
  if (window_counter != -42)
    window_counter++;
}

void
xfburn_main_leave_window (void)
{
  /* if a main window is present, then it is in control */
  if (window_counter == -42)
    return;

  window_counter--;
  if (window_counter <= 0)
    g_idle_add ((GSourceFunc) gtk_main_quit, NULL );
}

static void
xfburn_main_enter_main_window (void)
{
  /* mark the window_counter as having a main window */
  window_counter = -42;
}


const gchar *
xfburn_main_get_initial_dir ()
{
  if (initial_dir)
    return initial_dir;
  else
    return xfce_get_homedir ();
}

gboolean
xfburn_main_has_initial_dir ()
{
  if (initial_dir)
    return TRUE;
  else
    return FALSE;
}


/* private functions */

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
  } else if (strcmp (option_name, "-D") == 0 || strcmp (option_name, "--directory") == 0) {
    if (value == NULL)
      initial_dir = g_get_current_dir ();
    else
      initial_dir = g_strdup(value);
  } else {
    g_set_error (error, 0, G_OPTION_ERROR_FAILED, "Invalid command line option. Please report, this is a bug.");
    return FALSE;
  }

  return TRUE;
}


static void
print_available_transcoders (void)
{
  g_print ("Valid transcoders are:\n");
  g_print ("\tbasic\tCan only burn uncompressed CD quality .wav files.\n");
#ifdef HAVE_GST
  g_print ("\tgst\tUses gstreamer, and can burn all formats supported by it.\n");
#endif
}

/* entry point */
int
main (int argc, char **argv)
{
  GtkWidget *mainwin;
  gint n_burners;
  GError *error = NULL;
#ifdef HAVE_GUDEV
  gchar *error_msg;
#endif
  XfburnTranscoder *transcoder;
  XfburnDeviceList *devlist;

#if DEBUG > 0
  /* I have to disable this until GtkTreeView gets fixed,
   * and doesn't complain anymore when a DnD doesn't add any
   * rows
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
   */
#endif

  g_set_application_name (_("Xfburn"));

  gdk_threads_init ();
  gdk_threads_enter ();

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  if (!gtk_init_with_args (&argc, &argv, NULL, optionentries, GETTEXT_PACKAGE, &error)) {
    if (error != NULL) {
      g_print (_("%s: %s\nTry %s --help to see a full list of available command line options.\n"), PACKAGE, error->message, PACKAGE_NAME);
      g_error_free (error);
      gdk_threads_leave ();
      return EXIT_FAILURE;
    }
  }

  if (!burn_initialize ()) {
    g_critical ("Unable to initialize libburn");
    xfce_dialog_show_error (NULL, NULL, _("Unable to initialize the burning backend."));
    gdk_threads_leave ();
    return EXIT_FAILURE;
  }

#ifdef HAVE_GST
  if (!gst_init_check (&argc, &argv, &error)) {
    g_critical ("Failed to initialize gstreamer!");
    /* I'm assuming this pretty much never happens. If it does, we should make this a soft failure and fall back to basic */
    gdk_threads_leave ();
    burn_finish ();
    return EXIT_FAILURE;
  }
#endif

  if (show_version) {
#ifdef HAVE_GST
    const char *nano_str;
    guint gst_major, gst_minor, gst_micro, gst_nano;
#endif

    g_print ("%s version %s for Xfce %s\n", PACKAGE, VERSION, xfce_version_string ());
    g_print ("\tbuilt with GTK+-%d.%d.%d, ", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
    g_print ("linked with GTK+-%d.%d.%d.\n", gtk_major_version, gtk_minor_version, gtk_micro_version);

#ifdef HAVE_GST
    gst_version (&gst_major, &gst_minor, &gst_micro, &gst_nano);

    if (gst_nano == 1)
      nano_str = " (CVS)";
    else if (gst_nano == 2)
      nano_str = " (Prerelease)";
    else
      nano_str = "";

    g_print ("\tGStreamer support (built with %d.%d.%d, linked against %d.%d.%d%s)\n",
             GST_VERSION_MAJOR, GST_VERSION_MINOR, GST_VERSION_MICRO,
             gst_major, gst_minor, gst_micro, nano_str);

#endif
    exit (EXIT_SUCCESS);
  }

  if (transcoder_selection && strcmp (transcoder_selection, "list") == 0) {
    print_available_transcoders();
    gdk_threads_leave ();
    burn_finish ();
    return EXIT_SUCCESS;
  }

  DBG ("%s version %s for Xfce %s\n", PACKAGE, VERSION, xfce_version_string ());

  xfburn_settings_init ();

#ifdef HAVE_GUDEV
  error_msg = xfburn_udev_manager_create_global ();
  if (error_msg) {
    xfce_dialog_show_error (NULL, NULL, "%s", error_msg);
    gdk_threads_leave ();
    burn_finish ();
    return EXIT_FAILURE;
  } else {
    g_message ("Using UDEV");
  }
#endif

  devlist = xfburn_device_list_new ();
  g_object_get (devlist, "num-burners", &n_burners, NULL);

  if (n_burners < 1) {
    GtkMessageDialog *dialog = (GtkMessageDialog *) gtk_message_dialog_new (NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_CLOSE,
                                    ((const gchar *) _("No burners are currently available")));
    gtk_message_dialog_format_secondary_text (dialog,
                                    _("Possibly the disc(s) are in use, and cannot get accessed.\n\n"
                                      "Please unmount and restart the application.\n\n"
                                      "If no disc is in the drive, check that you have read and write access to the drive with the current user."));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (GTK_WIDGET (dialog));
  }


  /*----------Transcoder--------------------------------------------------*/

  if (!transcoder_selection) {
    /* select the best available */
#ifdef HAVE_GST
    transcoder = XFBURN_TRANSCODER (xfburn_transcoder_gst_new ());
#else
    transcoder = XFBURN_TRANSCODER (xfburn_transcoder_basic_new ());
#endif
#ifdef HAVE_GST
  } else if (strcmp (transcoder_selection, "gst") == 0) {
    transcoder = XFBURN_TRANSCODER (xfburn_transcoder_gst_new ());
#endif
  } else if (strcmp (transcoder_selection, "basic") == 0) {
    transcoder = XFBURN_TRANSCODER (xfburn_transcoder_basic_new ());
  } else {
    g_print ("'%s' is an invalid transcoder selection.\n",
             transcoder_selection);
    g_print ("\n");
    print_available_transcoders();
    gdk_threads_leave ();
    burn_finish ();
    return EXIT_FAILURE;
  }

  if (!xfburn_transcoder_is_initialized (transcoder, &error)) {
    xfce_dialog_show_warning(NULL, NULL, _("Failed to initialize %s transcoder: %s\n\t(falling back to basic implementation)"), xfburn_transcoder_get_name (transcoder), error->message);
    g_error_free (error);
    g_object_unref (transcoder);
    transcoder = XFBURN_TRANSCODER (xfburn_transcoder_basic_new ());
  } else {
    g_message ("Using %s transcoder.", xfburn_transcoder_get_name (transcoder));
  }
  xfburn_transcoder_set_global (transcoder);


  /*----------evaluate parsed command line action options-------------------------*/

  /* heuristic for file names on the commandline */
  if (argc == 2 && !add_data_composition && !add_audio_composition) {
    /* exactly one filename, assume it is an image */
      image_filename = argv[1];
  } else if (argc > 2 && !add_data_composition && !add_audio_composition) {
    /* several file names, for now just open up a data composition */
    /* TODO: auto-detect music files for audio compositions */
    add_data_composition = TRUE;
  }

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
        xfce_dialog_show_error (NULL, NULL, _("Image file '%s' does not exist."), image_fullname);
    }

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  } else if (blank) {
    GtkWidget *dialog = xfburn_blank_dialog_new ();

    other_action = TRUE;
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }


  /*----------main window--------------------------------------------------*/

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


  /*----------shutdown--------------------------------------------------*/

  g_object_unref (devlist);
  g_object_unref (transcoder);

#ifdef HAVE_GUDEV
  xfburn_udev_manager_shutdown ();
#endif

  xfburn_settings_flush ();
  xfburn_settings_free ();

  burn_finish ();

  gdk_threads_leave ();

  return EXIT_SUCCESS;
}
