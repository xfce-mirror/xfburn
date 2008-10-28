/* $Id$ */
/*
 *  Copyright (c) 2005-2007 Jean-François Wauthy (pollux@xfce.org)
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

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-stock.h"
#include "xfburn-settings.h"

#include "xfburn-device-box.h"
#include "xfburn-burn-audio-cd-composition-dialog.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-perform-burn.h"
#include "xfburn-audio-composition.h"
#include "xfburn-transcoder.h"

#define XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_BURN_AUDIO_CD_COMPOSITION_DIALOG, XfburnBurnAudioCdCompositionDialogPrivate))

typedef struct
{
  GSList *track_list;

  GtkWidget *frame_device;
  GtkWidget *device_box;
  GtkWidget *combo_mode;

  GtkWidget *entry;
  GtkWidget *check_eject;
  GtkWidget *check_burnfree;
  GtkWidget *check_only_iso;
  GtkWidget *hbox_iso;
  GtkWidget *entry_path_iso;
  GtkWidget *check_dummy;
  GtkWidget *button_proceed;

  gint response;
} XfburnBurnAudioCdCompositionDialogPrivate;

enum {
  PROP_0,
  PROP_TRACK_LIST,
};

/* prototypes */
static void xfburn_burn_audio_cd_composition_dialog_class_init (XfburnBurnAudioCdCompositionDialogClass * klass);
static GObject * xfburn_burn_audio_cd_composition_dialog_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties);
static void xfburn_burn_audio_cd_composition_dialog_finalize (GObject * object);

static void xfburn_burn_audio_cd_composition_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void xfburn_burn_audio_cd_composition_dialog_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);

static void cb_check_only_iso_toggled (GtkToggleButton * button, XfburnBurnAudioCdCompositionDialog * dialog);
static void cb_browse_iso (GtkButton * button, XfburnBurnAudioCdCompositionDialog * dialog);
static void cb_disc_refreshed (GtkWidget *device_box, XfburnDevice *device, XfburnBurnAudioCdCompositionDialog * dialog);
static void cb_dialog_response (XfburnBurnAudioCdCompositionDialog * dialog, gint response_id,
                                XfburnBurnAudioCdCompositionDialogPrivate * priv);

/* globals */
static XfceTitledDialogClass *parent_class = NULL;

GtkType
xfburn_burn_audio_cd_composition_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnBurnAudioCdCompositionDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_burn_audio_cd_composition_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnBurnAudioCdCompositionDialog),
      0,
      NULL,
      NULL
    };

    type = g_type_register_static (XFCE_TYPE_TITLED_DIALOG, "XfburnBurnAudioCdCompositionDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_burn_audio_cd_composition_dialog_class_init (XfburnBurnAudioCdCompositionDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  g_type_class_add_private (klass, sizeof (XfburnBurnAudioCdCompositionDialogPrivate));
  
  object_class->constructor = xfburn_burn_audio_cd_composition_dialog_constructor;
  object_class->finalize = xfburn_burn_audio_cd_composition_dialog_finalize;
  object_class->get_property = xfburn_burn_audio_cd_composition_dialog_get_property;
  object_class->set_property = xfburn_burn_audio_cd_composition_dialog_set_property;

  /* properties */
  g_object_class_install_property (object_class, PROP_TRACK_LIST,
				   g_param_spec_pointer ("track-list", "Track List", "Track List", G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static GObject *
xfburn_burn_audio_cd_composition_dialog_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
  GObject *gobj;
  XfburnBurnAudioCdCompositionDialog *obj;
  XfburnBurnAudioCdCompositionDialogPrivate *priv;
  
  GdkPixbuf *icon = NULL;
  GtkBox *box;
  GtkWidget *img;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *align;
  GtkWidget *button;
  gchar *default_path;
  gchar *tmp_dir;
  //const char *comp_name;

  gobj = G_OBJECT_CLASS (parent_class)->constructor (type, n_construct_properties, construct_properties);
  obj = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG (gobj);
  priv = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE (obj);
  box = GTK_BOX (GTK_DIALOG (obj)->vbox);

  gtk_window_set_title (GTK_WINDOW (obj), _("Burn Composition"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  icon = gtk_widget_render_icon (GTK_WIDGET (obj), XFBURN_STOCK_BURN_CD, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_window_set_icon (GTK_WINDOW (obj), icon);
  g_object_unref (icon);

  /* burning devices list */
  priv->device_box = xfburn_device_box_new (SHOW_CD_WRITERS | SHOW_CDRW_WRITERS | SHOW_SPEED_SELECTION);
  g_signal_connect (G_OBJECT (priv->device_box), "disc-refreshed", G_CALLBACK (cb_disc_refreshed), obj);
  g_signal_connect (G_OBJECT (priv->device_box), "device-changed", G_CALLBACK (cb_disc_refreshed), obj);
  gtk_widget_show (priv->device_box);

  priv->frame_device = xfce_create_framebox_with_content (_("Burning device"), priv->device_box);
  gtk_widget_show (priv->frame_device);
  gtk_box_pack_start (box, priv->frame_device, FALSE, FALSE, BORDER);

  /* composition name */
  /*
  comp_name = iso_image_get_volume_id (priv->image);
  if (strcmp (comp_name, _(DATA_COMPOSITION_DEFAULT_NAME)) == 0) {
    GtkWidget *label;
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);

    frame = xfce_create_framebox_with_content (_("Composition name"), vbox);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label), _("<small>Would you like to change the default composition name?</small>"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, BORDER/2);

    priv->entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (priv->entry), comp_name);
    gtk_box_pack_start (GTK_BOX (vbox), priv->entry, FALSE, FALSE, BORDER);
    gtk_widget_show (priv->entry);
  } else {
    priv->entry = NULL;
  }
  */

  /* options */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);

  frame = xfce_create_framebox_with_content (_("Options"), vbox);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  priv->check_eject = gtk_check_button_new_with_mnemonic (_("E_ject disk"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_eject), TRUE);
  gtk_widget_show (priv->check_eject);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_eject, FALSE, FALSE, BORDER);

  priv->check_dummy = gtk_check_button_new_with_mnemonic (_("_Dummy write"));
  gtk_widget_show (priv->check_dummy);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_dummy, FALSE, FALSE, BORDER);

  priv->check_burnfree = gtk_check_button_new_with_mnemonic (_("Burn_Free"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_burnfree), TRUE);
  gtk_widget_show (priv->check_burnfree);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_burnfree, FALSE, FALSE, BORDER);

  /* create ISO ? */
  priv->check_only_iso = gtk_check_button_new_with_mnemonic (_("Only create _ISO"));
  gtk_widget_show (priv->check_only_iso);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_only_iso, FALSE, FALSE, BORDER);
  g_signal_connect (G_OBJECT (priv->check_only_iso), "toggled", G_CALLBACK (cb_check_only_iso_toggled), obj);

  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, BORDER * 4, 0);
  gtk_widget_show (align);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);

  priv->hbox_iso = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (priv->hbox_iso);
  gtk_container_add (GTK_CONTAINER (align), priv->hbox_iso);
  gtk_widget_set_sensitive (priv->hbox_iso, FALSE);

  priv->entry_path_iso = gtk_entry_new ();
  tmp_dir = xfburn_settings_get_string ("temporary-dir", g_get_tmp_dir ());
  default_path = g_build_filename (tmp_dir, "xfburn.iso", NULL);
  gtk_entry_set_text (GTK_ENTRY (priv->entry_path_iso), default_path);
  g_free (default_path);
  g_free (tmp_dir);
  gtk_widget_show (priv->entry_path_iso);
  gtk_box_pack_start (GTK_BOX (priv->hbox_iso), priv->entry_path_iso, FALSE, FALSE, 0);

  img = gtk_image_new_from_stock (GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (img);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (priv->hbox_iso), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_browse_iso), obj);

  /* action buttons */
  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CANCEL);

  priv->button_proceed = button = xfce_create_mixed_button ("xfburn-burn-cd", _("_Burn Composition"));

  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_OK);
  //gtk_box_pack_start (GTK_BOX (GTK_DIALOG(obj)->action_area), button, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);

  cb_disc_refreshed (priv->device_box, xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box)), obj);
  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (cb_dialog_response), priv);

  return gobj;
}

static void
xfburn_burn_audio_cd_composition_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  XfburnBurnAudioCdCompositionDialogPrivate *priv = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE (object);

  switch (prop_id) {
  case PROP_TRACK_LIST:
    g_value_set_pointer (value, priv->track_list);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
xfburn_burn_audio_cd_composition_dialog_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  XfburnBurnAudioCdCompositionDialogPrivate *priv = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE (object);

  switch (prop_id) {
  case PROP_TRACK_LIST:
    priv->track_list= g_value_get_pointer (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
xfburn_burn_audio_cd_composition_dialog_finalize (GObject * object)
{
  //XfburnBurnAudioCdCompositionDialogPrivate *priv = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* internals */
static void
cb_check_only_iso_toggled (GtkToggleButton * button, XfburnBurnAudioCdCompositionDialog * dialog)
{
  XfburnBurnAudioCdCompositionDialogPrivate *priv = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE (dialog);
  gboolean valid_disc;

  gtk_widget_set_sensitive (priv->frame_device, !gtk_toggle_button_get_active (button));
  xfburn_device_box_set_sensitive (XFBURN_DEVICE_BOX (priv->device_box), !gtk_toggle_button_get_active (button));
  
  gtk_widget_set_sensitive (priv->hbox_iso, gtk_toggle_button_get_active (button));
  gtk_widget_set_sensitive (priv->check_eject, !gtk_toggle_button_get_active (button));
  gtk_widget_set_sensitive (priv->check_burnfree, !gtk_toggle_button_get_active (button));
  gtk_widget_set_sensitive (priv->check_dummy, !gtk_toggle_button_get_active (button));
  if (!gtk_toggle_button_get_active (button)) {
    g_object_get (G_OBJECT (priv->device_box), "valid", &valid_disc, NULL);
    gtk_widget_set_sensitive (priv->button_proceed, valid_disc);
  } else {
    gtk_widget_set_sensitive (priv->button_proceed, TRUE);
  }
}

static void
cb_browse_iso (GtkButton * button, XfburnBurnAudioCdCompositionDialog * dialog)
{
  XfburnBurnAudioCdCompositionDialogPrivate *priv = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE (dialog);
  
  xfburn_browse_for_file (GTK_ENTRY (priv->entry_path_iso), GTK_WINDOW (dialog));
}

static void
cb_disc_refreshed (GtkWidget *device_box, XfburnDevice *device, XfburnBurnAudioCdCompositionDialog * dialog)
{
  XfburnBurnAudioCdCompositionDialogPrivate *priv = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE (dialog);
  gboolean valid_disc;

  g_object_get (G_OBJECT (priv->device_box), "valid", &valid_disc, NULL);

  gtk_widget_set_sensitive (priv->button_proceed, valid_disc);
}

typedef struct {
  GtkWidget *dialog_progress;
  struct burn_source *src;
  gchar *iso_path;
} ThreadWriteIsoParams;

static void
thread_write_iso (ThreadWriteIsoParams * params)
{
  GtkWidget *dialog_progress = params->dialog_progress;
  gint fd;
  guchar buf[2048];
  glong size = 0;
  glong written = 0;
  guint i = 0;
  int (*read_fn) (struct burn_source *src, unsigned char *buf, int size);

  fd = open (params->iso_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    /* could not create destination */
    gchar err[256];
    gchar *error_msg = NULL;

    strerror_r (errno, err, 256);

    error_msg = g_strdup_printf (_("Could not create destination ISO file: %s"), err);
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), error_msg);
    g_free (error_msg);

    goto end;
  }

  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Writing ISO..."));

  size = (glong) params->src->get_size (params->src);
  if (params->src->read == NULL)
    read_fn = params->src->read_xt;
  else
    read_fn = params->src->read;

  /* FIXME: is size really always 2048? */
  while (read_fn (params->src, buf, 2048) == 2048) {
    if (write (fd, buf, 2048) < 2048) {
      /* an error occured while writing */
      gchar err[256];
      gchar *error_msg = NULL;
      
      strerror_r (errno, err, 256);
    
      error_msg = g_strdup_printf (_("An error occured while writing ISO: %s"), err);
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), error_msg);
      g_free (error_msg);
      goto cleanup;
    } else {
      written += 2048;
      i++;

      if (i >= 1000) {
	i = 0;
	gdouble percent = 0;

	percent = ((gdouble) written / (gdouble) size);

	xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), percent);
      }
    }
  }
  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED, _("Done"));

 cleanup:
  close (fd);
 end:
  burn_source_free (params->src);
  g_free (params->iso_path);
  g_free (params);
}

typedef struct {
  GtkWidget *dialog_progress;
  XfburnDevice *device;
  GSList *tracks;
  gint speed;
  XfburnWriteMode write_mode;
  gboolean eject;
  gboolean dummy;
  gboolean burnfree;
} ThreadBurnCompositionParams;

static void 
thread_burn_prep_and_burn (ThreadBurnCompositionParams * params, struct burn_drive *drive,
                           struct burn_disc *disc, struct burn_session *session, int n_tracks, 
                           int track_sectors[], struct burn_track **tracks, struct burn_source **srcs)
{
  GtkWidget *dialog_progress = params->dialog_progress;

  struct burn_write_opts * burn_options;
  gint ret,i;

  ret = burn_disc_add_session (disc, session, BURN_POS_END);
  if (ret == 0) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to create disc object"));
    return;
  }

  for (i=0; i<n_tracks; i++) {
    burn_session_add_track (session, tracks[i], BURN_POS_END);
  }

  burn_options = burn_write_opts_new (drive);
  burn_write_opts_set_perform_opc (burn_options, 0);
  burn_write_opts_set_multi (burn_options, 0);

  /* keep all modes here, just in case we want to change the default sometimes */
  switch (params->write_mode) {
  case WRITE_MODE_TAO:
    burn_write_opts_set_write_type (burn_options, BURN_WRITE_TAO, BURN_BLOCK_MODE1);
    break;
  case WRITE_MODE_SAO:
    burn_write_opts_set_write_type (burn_options, BURN_WRITE_SAO, BURN_BLOCK_SAO);
    break;
  case WRITE_MODE_RAW16:
    burn_write_opts_set_write_type (burn_options, BURN_WRITE_RAW, BURN_BLOCK_RAW16);
    break;
  case WRITE_MODE_RAW96P:
    burn_write_opts_set_write_type (burn_options, BURN_WRITE_RAW, BURN_BLOCK_RAW96P);
    break;
  case WRITE_MODE_RAW96R:
    burn_write_opts_set_write_type (burn_options, BURN_WRITE_RAW, BURN_BLOCK_RAW96R);
    break;
  default:
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("The write mode is not supported currently"));
    return;
  }

  burn_write_opts_set_simulate(burn_options, params->dummy ? 1 : 0);
  DBG ("Set speed to %d kb/s", params->speed);
  burn_drive_set_speed (drive, 0, params->speed);
  burn_write_opts_set_underrun_proof (burn_options, params->burnfree ? 1 : 0);
  burn_drive_set_buffer_waiting (drive, 1, -1, -1, -1, 75, 95);

  // FIXME: perform_burn_write only takes one fifo as an argument right now
  //xfburn_perform_burn_write (dialog_progress, drive, params->write_mode, burn_options, disc, srcs, track_sectors);
  xfburn_perform_burn_write (dialog_progress, drive, params->write_mode, burn_options, disc, NULL, track_sectors);

  burn_write_opts_free (burn_options);
}

static void
thread_burn_composition (ThreadBurnCompositionParams * params)
{
  GtkWidget *dialog_progress = params->dialog_progress;

  struct burn_disc *disc;
  struct burn_session *session;
  struct burn_track **tracks;
  struct burn_source **srcs;
  int n_tracks;
  int i,j;
  GSList *track_list;
  int *track_sectors;
  gboolean abort = FALSE;
  XfburnTranscoder *trans;
  GError *error = NULL;

  struct burn_drive_info *drive_info = NULL;

  if (!burn_initialize ()) {
    g_critical ("Unable to initialize libburn");
    /* FIXME: free contents of params! */
    g_free (params);
    return;
  }

  disc = burn_disc_create ();
  session = burn_session_create ();

  n_tracks = g_slist_length (params->tracks);
  track_sectors = g_new (int, n_tracks);
  tracks = g_new (struct burn_track *, n_tracks);
  srcs = g_new (struct burn_source *, n_tracks);
  trans = xfburn_transcoder_get_global ();

  track_list = params->tracks;
  for (i=0; i<n_tracks; i++) {
    XfburnAudioTrack *atrack = track_list->data;

    tracks[i] = xfburn_transcoder_create_burn_track (trans, atrack, &error);
    if (tracks[i] == NULL) {
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), error->message);
      g_error_free (error);
      abort = TRUE;
      break;
    }
    srcs[i] = atrack->src;

    track_sectors[i] = atrack->sectors;

    track_list = g_slist_next (track_list);
  }

  if (!abort) {
    if (!xfburn_transcoder_prepare (trans, &error)) {
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), error->message);
      g_error_free (error);
      abort = TRUE;
    }
  }

  if (!abort) {
    if (!xfburn_device_grab (params->device, &drive_info)) {
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to grab drive"));
    } else {
      thread_burn_prep_and_burn (params, drive_info->drive, disc, session, n_tracks, track_sectors, tracks, srcs);
      burn_drive_release (drive_info->drive, params->eject ? 1 : 0);
    }
  }

  xfburn_transcoder_finish (trans);

  for (j=0; j<i; j++) {
    burn_track_free (tracks[j]);
  }
  g_free (srcs);
  g_free (tracks);
  g_free (track_sectors);

  for (track_list = params->tracks; track_list; track_list = g_slist_next (track_list))
    xfburn_transcoder_free_burning_resources (trans, (XfburnAudioTrack *) track_list->data, NULL);
  g_object_unref (trans);

  burn_session_free (session);
  burn_disc_free (disc);
  burn_finish ();

  /* FIXME: free track_list here? */
  g_free (params);
}

static void
cb_dialog_response (XfburnBurnAudioCdCompositionDialog * dialog, gint response_id, XfburnBurnAudioCdCompositionDialogPrivate * priv)
{
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *dialog_progress;

    struct burn_source * src = NULL;

    /* If the name was the default, update the image volume id and volset id */
    /*
    if (priv->entry != NULL) {
      const gchar * comp_name = gtk_entry_get_text (GTK_ENTRY (priv->entry));
      iso_image_set_volume_id (priv->image, comp_name);
      iso_image_set_volset_id (priv->image, comp_name);
    }
    */

    dialog_progress = xfburn_progress_dialog_new (GTK_WINDOW (dialog));
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), gtk_window_get_transient_for (GTK_WINDOW (dialog)));
    gtk_widget_hide (GTK_WIDGET (dialog));
    
    gtk_widget_show (dialog_progress);

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_only_iso))) {
      ThreadWriteIsoParams *params = NULL;

      /* create a new iso */
      params = g_new0 (ThreadWriteIsoParams, 1);
      params->dialog_progress = dialog_progress;
      params->src = src;
      params->iso_path = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->entry_path_iso)));
      g_thread_create ((GThreadFunc) thread_write_iso, params, FALSE, NULL);
    }
    else {
      ThreadBurnCompositionParams *params = NULL;
      XfburnDevice *device;
      gint speed;
      XfburnWriteMode write_mode;
      //struct burn_source * src_fifo = NULL;

      device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));
      speed = xfburn_device_box_get_speed (XFBURN_DEVICE_BOX (priv->device_box));
      /* cdrskin burns audio with SAO */
      write_mode = WRITE_MODE_TAO;

      /* burn composition */
      params = g_new0 (ThreadBurnCompositionParams, 1);
      params->dialog_progress = dialog_progress;
      params->device = device;
      params->tracks = priv->track_list;
      params->speed = speed;
      params->write_mode = write_mode;
      params->eject = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject));
      params->dummy = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_dummy));
      params->burnfree = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_burnfree));
      g_thread_create ((GThreadFunc) thread_burn_composition, params, FALSE, NULL);
    }
  }
}

/* public */
GtkWidget *
xfburn_burn_audio_cd_composition_dialog_new (GSList *track_list)
{
  XfburnBurnAudioCdCompositionDialog *obj;

  obj = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG (g_object_new (XFBURN_TYPE_BURN_AUDIO_CD_COMPOSITION_DIALOG, "track-list", track_list, NULL));
  
  return GTK_WIDGET (obj);
}
