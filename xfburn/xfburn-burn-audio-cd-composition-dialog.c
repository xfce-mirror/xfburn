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
#include "xfburn-settings.h"

#include "xfburn-device-box.h"
#include "xfburn-burn-audio-cd-composition-dialog.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-perform-burn.h"
#include "xfburn-audio-composition.h"
#include "xfburn-transcoder.h"

#define XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE(obj) (xfburn_burn_audio_cd_composition_dialog_get_instance_private (XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG (obj)))

typedef struct
{
  GSList *track_list;

  GtkWidget *frame_device;
  GtkWidget *device_box;
  GtkWidget *combo_mode;

  GtkWidget *entry;
  GtkWidget *check_eject;
  GtkWidget *check_burnfree;
  GtkWidget *check_dummy;
  GtkWidget *button_proceed;

  gint response;
} XfburnBurnAudioCdCompositionDialogPrivate;

enum {
  PROP_0,
  PROP_TRACK_LIST,
};

/* prototypes */
static GObject * xfburn_burn_audio_cd_composition_dialog_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties);
static void xfburn_burn_audio_cd_composition_dialog_finalize (GObject * object);

static void xfburn_burn_audio_cd_composition_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void xfburn_burn_audio_cd_composition_dialog_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);

static void cb_volume_changed (GtkWidget *device_box, gboolean device_changed, XfburnDevice *device, XfburnBurnAudioCdCompositionDialog * dialog);
static void cb_dialog_response (XfburnBurnAudioCdCompositionDialog * dialog, gint response_id,
                                XfburnBurnAudioCdCompositionDialogPrivate * priv);

/* globals */
static XfceTitledDialogClass *parent_class = NULL;

G_DEFINE_TYPE_WITH_PRIVATE(XfburnBurnAudioCdCompositionDialog, xfburn_burn_audio_cd_composition_dialog, XFCE_TYPE_TITLED_DIALOG);

static void
xfburn_burn_audio_cd_composition_dialog_class_init (XfburnBurnAudioCdCompositionDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  
  // object_class->constructor = xfburn_burn_audio_cd_composition_dialog_constructor;
  object_class->finalize = xfburn_burn_audio_cd_composition_dialog_finalize;
  object_class->get_property = xfburn_burn_audio_cd_composition_dialog_get_property;
  object_class->set_property = xfburn_burn_audio_cd_composition_dialog_set_property;

  /* properties */
  g_object_class_install_property (object_class, PROP_TRACK_LIST,
				   g_param_spec_pointer ("track-list", "Track List", "Track List", G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
xfburn_burn_audio_cd_composition_dialog_init(XfburnBurnAudioCdCompositionDialog *obj)
{
  XfburnBurnAudioCdCompositionDialogPrivate *priv = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE(obj);
  
  GdkPixbuf *icon = NULL;
  GtkBox *box;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *align;
  GtkWidget *button;
  //const char *comp_name;

  box = GTK_BOX (gtk_dialog_get_content_area((GTK_DIALOG (obj))));

  gtk_window_set_title (GTK_WINDOW (obj), _("Burn Composition"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  icon = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "stock_xfburn", GTK_ICON_SIZE_DIALOG, 0, NULL);
  gtk_window_set_icon (GTK_WINDOW (obj), icon);
  g_object_unref (icon);

  /* burning devices list */
  priv->device_box = xfburn_device_box_new (SHOW_CD_WRITERS | SHOW_CDRW_WRITERS |
                                            SHOW_SPEED_SELECTION | SHOW_MODE_SELECTION);
  g_signal_connect (G_OBJECT (priv->device_box), "volume-changed", G_CALLBACK (cb_volume_changed), obj);
  gtk_widget_show (priv->device_box);

  priv->frame_device = xfce_gtk_frame_box_new_with_content (_("Burning device"), priv->device_box);
  gtk_widget_show (priv->frame_device);
  gtk_box_pack_start (box, priv->frame_device, FALSE, FALSE, BORDER);

  /* composition name */
  /*
  comp_name = iso_image_get_volume_id (priv->image);
  if (strcmp (comp_name, _(DATA_COMPOSITION_DEFAULT_NAME)) == 0) {
    GtkWidget *label;
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show (vbox);

    frame = xfce_gtk_frame_box_new_with_content (_("Composition name"), vbox);
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
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox);

  frame = xfce_gtk_frame_box_new_with_content (_("Options"), vbox);
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

/*
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, BORDER * 4, 0);
  gtk_widget_show (align);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
*/
  /* action buttons */
  button = gtk_button_new_with_mnemonic (_("_Cancel"));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CANCEL);

  priv->button_proceed = button = xfce_gtk_button_new_mixed ("stock_xfburn", _("_Burn Composition"));

  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_OK);
  //gtk_box_pack_start (GTK_BOX (GTK_DIALOG(obj)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_set_can_default (button, TRUE);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);

  cb_volume_changed (priv->device_box, TRUE, xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box)), obj);
  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (cb_dialog_response), priv);
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
cb_volume_changed (GtkWidget *device_box, gboolean device_changed, XfburnDevice *device, XfburnBurnAudioCdCompositionDialog * dialog)
{
  XfburnBurnAudioCdCompositionDialogPrivate *priv = XFBURN_BURN_AUDIO_CD_COMPOSITION_DIALOG_GET_PRIVATE (dialog);
  gboolean valid_disc;

  g_object_get (G_OBJECT (priv->device_box), "valid", &valid_disc, NULL);

  gtk_widget_set_sensitive (priv->button_proceed, valid_disc);
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
    g_warning ("Unable to create disc object");
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("A problem with the burn backend occurred."));
    return;
  }

  //DBG ("Adding %d tracks to the session", n_tracks);
  for (i=0; i<n_tracks; i++) {
    //DBG ("Track %d has %d sectors", i, track_sectors[i]);
    burn_session_add_track (session, tracks[i], BURN_POS_END);
  }

  burn_options = burn_write_opts_new (drive);
  burn_write_opts_set_perform_opc (burn_options, 0);
  burn_write_opts_set_multi (burn_options, 0);
  burn_write_opts_set_simulate(burn_options, params->dummy ? 1 : 0);
  burn_write_opts_set_underrun_proof (burn_options, params->burnfree ? 1 : 0);

  if (!xfburn_set_write_mode (burn_options, params->write_mode, disc, WRITE_MODE_SAO)) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("The write mode is not supported currently."));
  } else {

    DBG ("Set speed to %d kb/s", params->speed);
    burn_drive_set_speed (drive, 0, params->speed);
    burn_drive_set_buffer_waiting (drive, 1, -1, -1, -1, 75, 95);

    xfburn_perform_burn_write (dialog_progress, drive, params->write_mode, burn_options, AUDIO_BYTES_PER_SECTOR, disc, srcs, track_sectors);
  }

  burn_write_opts_free (burn_options);
}

static void*
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
  gboolean abort_burn = FALSE;
  XfburnTranscoder *trans;
  GError *error = NULL;

  struct burn_drive_info *drive_info = NULL;

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
      abort_burn = TRUE;
      break;
    }
    srcs[i] = atrack->src;

    track_sectors[i] = atrack->sectors;

    track_list = g_slist_next (track_list);
  }

  if (!abort_burn) {
    if (!xfburn_transcoder_prepare (trans, &error)) {
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), error->message);
      g_error_free (error);
      abort_burn = TRUE;
    }
  }

  if (!abort_burn) {
    if (!xfburn_device_grab (params->device, &drive_info)) {
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to grab the drive."));
    } else {
      thread_burn_prep_and_burn (params, drive_info->drive, disc, session, n_tracks, track_sectors, tracks, srcs);
      xfburn_device_release (drive_info, params->eject);
    }
  }

  xfburn_transcoder_finish (trans);

  for (j=0; j<i; j++) {
    burn_track_free (tracks[j]);
  }
  g_free (srcs);
  g_free (tracks);
  g_free (track_sectors);

  g_object_unref (trans);

  burn_session_free (session);
  burn_disc_free (disc);

  /* FIXME: free track_list here? */
  g_free (params);
  return NULL;
}

static void
cb_dialog_response (XfburnBurnAudioCdCompositionDialog * dialog, gint response_id, XfburnBurnAudioCdCompositionDialogPrivate * priv)
{
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *dialog_progress;

    ThreadBurnCompositionParams *params = NULL;
    XfburnDevice *device;
    gint speed;
    XfburnWriteMode write_mode;
    //struct burn_source * src_fifo = NULL;

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

    device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));
    speed = xfburn_device_box_get_speed (XFBURN_DEVICE_BOX (priv->device_box));
    /* cdrskin burns audio with SAO, but we assume auto knows that */
    write_mode = WRITE_MODE_AUTO;

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
    g_thread_new ("audio_cd_burn", (GThreadFunc) thread_burn_composition, params);
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
