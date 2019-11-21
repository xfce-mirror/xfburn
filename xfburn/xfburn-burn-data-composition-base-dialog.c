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
#include "xfburn-burn-data-composition-base-dialog.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-perform-burn.h"

#define XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG_GET_PRIVATE(obj) (xfburn_burn_data_composition_base_dialog_get_instance_private (XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG(obj)))

typedef struct
{
  IsoImage *image;

  GtkWidget *frame_device;
  GtkWidget *device_box;
  GtkWidget *combo_mode;

  GtkWidget *entry;
  GtkWidget *check_eject;
  GtkWidget *check_burnfree;
  GtkWidget *check_stream_recording;
  GtkWidget *check_only_iso;
  GtkWidget *hbox_iso;
  GtkWidget *entry_path_iso;
  GtkWidget *check_dummy;
  GtkWidget *button_proceed;
  /*
   * Disabled: change button_proceed functionality
  GtkWidget *label_proceed;
  GtkWidget *image_proceed;
  */

  gint response;

  gboolean show_volume_name;
} XfburnBurnDataCompositionBaseDialogPrivate;

enum {
  PROP_0,
  PROP_IMAGE,
  PROP_SHOW_VOLUME_NAME,
};

/*
 * Disabled: change button_proceed functionality
char *proceed_text[] = {
  "Burn Composition",
  "   Blank Disc   ",
};

char *proceed_image[] = {
  "xfburn-burn-cd",
  "xfburn-blank-cdrw",
};
*/

/* prototypes */
static void xfburn_burn_data_composition_base_dialog_finalize (GObject * object);

static void xfburn_burn_data_composition_base_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void xfburn_burn_data_composition_base_dialog_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);

/*
 * Disabled: change button_proceed functionality
GtkWidget * create_proceed_button (XfburnBurnDataCompositionBaseDialog * dialog, const gchar *stock, const gchar *text);
void update_proceed_button (XfburnBurnDataCompositionBaseDialog * dialog);
static void cb_proceed_clicked (GtkButton * button, XfburnBurnDataCompositionBaseDialog * dialog);
*/
static void cb_check_only_iso_toggled (GtkToggleButton * button, XfburnBurnDataCompositionBaseDialog * dialog);
static void cb_browse_iso (GtkButton * button, XfburnBurnDataCompositionBaseDialog * dialog);
static void cb_volume_changed (XfburnDeviceBox *box, gboolean device_changed, XfburnDevice *device, XfburnBurnDataCompositionBaseDialog * dialog);
static void cb_dialog_response (XfburnBurnDataCompositionBaseDialog * dialog, gint response_id,
                                XfburnBurnDataCompositionBaseDialogPrivate * priv);

/* globals */
static XfceTitledDialogClass *parent_class = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (XfburnBurnDataCompositionBaseDialog, xfburn_burn_data_composition_base_dialog, XFCE_TYPE_TITLED_DIALOG)

static void
xfburn_burn_data_composition_base_dialog_class_init (XfburnBurnDataCompositionBaseDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  
  object_class->finalize = xfburn_burn_data_composition_base_dialog_finalize;
  object_class->get_property = xfburn_burn_data_composition_base_dialog_get_property;
  object_class->set_property = xfburn_burn_data_composition_base_dialog_set_property;

  /* properties */
  g_object_class_install_property (object_class, PROP_IMAGE,
				   g_param_spec_pointer ("image", _("Image"), _("Image"), G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SHOW_VOLUME_NAME,
				   g_param_spec_boolean ("show-volume-name", _("Show volume name"), _("Show a text entry for the name of the volume"), FALSE, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
xfburn_burn_data_composition_base_dialog_init(XfburnBurnDataCompositionBaseDialog *obj)
{
  XfburnBurnDataCompositionBaseDialogPrivate *priv = xfburn_burn_data_composition_base_dialog_get_instance_private(obj);
  GdkPixbuf *icon = NULL;
  GtkBox *box;
  GtkWidget *img;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *align;
  GtkWidget *button;
  gchar *default_path;
  gchar *tmp_dir;
  const char *comp_name;
  gint x,y;

  box = GTK_BOX (gtk_dialog_get_content_area((GTK_DIALOG (obj))));

  gtk_window_set_title (GTK_WINDOW (obj), _("Burn Composition"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &x, &y);
  icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default(), "stock_xfburn", x, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);

  gtk_window_set_icon (GTK_WINDOW (obj), icon);
  g_object_unref (icon);

  /* burning devices list */
  priv->device_box = xfburn_device_box_new (SHOW_CD_WRITERS | SHOW_CDRW_WRITERS | SHOW_MODE_SELECTION | SHOW_SPEED_SELECTION);
  gtk_widget_show (priv->device_box);

  g_signal_connect (G_OBJECT (priv->device_box), "volume-changed", G_CALLBACK (cb_volume_changed), obj);

  priv->frame_device = xfce_gtk_frame_box_new_with_content (_("Burning device"), priv->device_box);
  gtk_widget_show (priv->frame_device);
  gtk_box_pack_start (box, priv->frame_device, FALSE, FALSE, BORDER);

  /* composition name */
  if (priv->show_volume_name) {
    GtkWidget *label;
    comp_name = iso_image_get_volume_id (priv->image);
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

  priv->check_stream_recording = gtk_check_button_new_with_mnemonic (_("Stream _Recording"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_stream_recording), TRUE);
  gtk_widget_show (priv->check_stream_recording);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_stream_recording, FALSE, FALSE, BORDER);

  /* create ISO ? */
  priv->check_only_iso = gtk_check_button_new_with_mnemonic (_("Only create _ISO"));
  gtk_widget_show (priv->check_only_iso);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_only_iso, FALSE, FALSE, BORDER);
  g_signal_connect (G_OBJECT (priv->check_only_iso), "toggled", G_CALLBACK (cb_check_only_iso_toggled), obj);
/*
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, BORDER * 4, 0);
  gtk_widget_show (align);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
*/
  priv->hbox_iso = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER*4);
  gtk_widget_show (priv->hbox_iso);
  gtk_container_add (GTK_CONTAINER(vbox), priv->hbox_iso);
  gtk_widget_set_sensitive (priv->hbox_iso, FALSE);

  priv->entry_path_iso = gtk_entry_new ();
  tmp_dir = xfburn_settings_get_string ("temporary-dir", g_get_tmp_dir ());
  default_path = g_build_filename (tmp_dir, "xfburn.iso", NULL);
  gtk_entry_set_text (GTK_ENTRY (priv->entry_path_iso), default_path);
  g_free (default_path);
  g_free (tmp_dir);
  gtk_widget_show (priv->entry_path_iso);
  gtk_box_pack_start (GTK_BOX (priv->hbox_iso), priv->entry_path_iso, FALSE, FALSE, 0);

  img = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (img);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (priv->hbox_iso), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_browse_iso), obj);

  /* action buttons */
  button = gtk_button_new_with_mnemonic (_("_Cancel"));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CANCEL);

  priv->button_proceed = button = xfce_gtk_button_new_mixed ("stock_xfburn", _("_Burn Composition"));
  /*
   * Disabled: change button_proceed functionality
  button = create_proceed_button (obj, "xfburn-burn-cd", "");
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_proceed_clicked), obj);
  */

  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_OK);
  //gtk_box_pack_start (GTK_BOX (GTK_DIALOG(obj)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_set_can_default (button, TRUE);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);

  cb_volume_changed (XFBURN_DEVICE_BOX (priv->device_box), TRUE, xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box)), obj);
  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (cb_dialog_response), priv);
}

static void
xfburn_burn_data_composition_base_dialog_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  XfburnBurnDataCompositionBaseDialogPrivate *priv = XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG_GET_PRIVATE (object);

  switch (prop_id) {
  case PROP_IMAGE:
    g_value_set_pointer (value, priv->image);
    break;
  case PROP_SHOW_VOLUME_NAME:
    g_value_set_boolean (value, priv->show_volume_name);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
xfburn_burn_data_composition_base_dialog_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  XfburnBurnDataCompositionBaseDialogPrivate *priv = XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG_GET_PRIVATE (object);

  switch (prop_id) {
  case PROP_IMAGE:
    priv->image = g_value_get_pointer (value);
    break;
  case PROP_SHOW_VOLUME_NAME:
    priv->show_volume_name = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
xfburn_burn_data_composition_base_dialog_finalize (GObject * object)
{
  XfburnBurnDataCompositionBaseDialogPrivate *priv = XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG_GET_PRIVATE (object);

  iso_image_unref (priv->image);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* internals */
/*
 * Disabled: change button_proceed functionality
GtkWidget *
create_proceed_button (XfburnBurnDataCompositionBaseDialog * dialog, const gchar *stock, const gchar *text)
{
  XfburnBurnDataCompositionBaseDialogPrivate *priv = XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG_GET_PRIVATE (dialog);
  GtkWidget *button, *align, *image, *hbox, *label;

  priv->button_proceed = button = gtk_button_new ();
  priv->label_proceed = label = gtk_label_new_with_mnemonic (text);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);

  priv->image_proceed = image = gtk_image_new_from_stock (stock, GTK_ICON_SIZE_BUTTON);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (button), align);
  gtk_container_add (GTK_CONTAINER (align), hbox);
  gtk_widget_show_all (align);

  return button;
}

void 
update_proceed_button (XfburnBurnDataCompositionBaseDialog * dialog)
{
  XfburnBurnDataCompositionBaseDialogPrivate *priv = XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG_GET_PRIVATE (dialog);

  gtk_label_set_text (GTK_LABEL (priv->label_proceed), _(proceed_text[priv->response]));
  gtk_image_set_from_stock (GTK_IMAGE (priv->image_proceed), proceed_image[priv->response], GTK_ICON_SIZE_BUTTON);
}
*/

static void
cb_check_only_iso_toggled (GtkToggleButton * button, XfburnBurnDataCompositionBaseDialog * dialog)
{
  XfburnBurnDataCompositionBaseDialogPrivate *priv = XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG_GET_PRIVATE (dialog);
  gboolean valid_disc;

  gtk_widget_set_sensitive (priv->frame_device, !gtk_toggle_button_get_active (button));
  xfburn_device_box_set_sensitive (XFBURN_DEVICE_BOX (priv->device_box), !gtk_toggle_button_get_active (button));
  
  gtk_widget_set_sensitive (priv->hbox_iso, gtk_toggle_button_get_active (button));
  gtk_widget_set_sensitive (priv->check_eject, !gtk_toggle_button_get_active (button));
  gtk_widget_set_sensitive (priv->check_burnfree, !gtk_toggle_button_get_active (button));
  gtk_widget_set_sensitive (priv->check_stream_recording, !gtk_toggle_button_get_active (button));
  gtk_widget_set_sensitive (priv->check_dummy, !gtk_toggle_button_get_active (button));
  if (!gtk_toggle_button_get_active (button)) {
    g_object_get (G_OBJECT (priv->device_box), "valid", &valid_disc, NULL);
    gtk_widget_set_sensitive (priv->button_proceed, valid_disc);
  } else {
    gtk_widget_set_sensitive (priv->button_proceed, TRUE);
  }
}

/*
 * Disabled: change button_proceed functionality
static void
cb_proceed_clicked (GtkButton * button, XfburnBurnDataCompositionBaseDialog * dialog)
{
}
*/

static void
cb_browse_iso (GtkButton * button, XfburnBurnDataCompositionBaseDialog * dialog)
{
  XfburnBurnDataCompositionBaseDialogPrivate *priv = XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG_GET_PRIVATE (dialog);
  
  xfburn_browse_for_file (GTK_ENTRY (priv->entry_path_iso), GTK_WINDOW (dialog));
}

static void
cb_volume_changed (XfburnDeviceBox *box, gboolean device_changed, XfburnDevice *device, XfburnBurnDataCompositionBaseDialog * dialog)
{
  XfburnBurnDataCompositionBaseDialogPrivate *priv = XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG_GET_PRIVATE (dialog);
  gboolean valid_disc;

  g_object_get (box, "valid", &valid_disc, NULL);

  /*
   * Disabled: change button_proceed functionality
  if (!valid_disc && xfburn_device_list_get_disc_status () == BURN_DISC_FULL && xfburn_device_list_disc_is_erasable ()) {
    priv->response = XFBURN_BURN_DATA_COMPOSITION_DIALOG_BLANK;
    valid_disc = TRUE;
  } else {
    priv->response = XFBURN_BURN_DATA_COMPOSITION_DIALOG_BURN;
  }
  update_proceed_button (dialog);
  */
  gtk_widget_set_sensitive (priv->button_proceed, valid_disc);
}

typedef struct {
  GtkWidget *dialog_progress;
  struct burn_source *src;
  gchar *iso_path;
} ThreadWriteIsoParams;

static void*
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
	gdouble percent = 0;
	i = 0;

	percent = ((gdouble) written / (gdouble) size) * 100.0;

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
  return NULL;
}

typedef struct {
  GtkWidget *dialog_progress;
  XfburnDevice *device;
  struct burn_source *src;
  gboolean is_fifo;
  gint speed;
  XfburnWriteMode write_mode;
  gboolean eject;
  gboolean dummy;
  gboolean burnfree;
  gboolean stream_recording;
} ThreadBurnCompositionParams;

static void 
thread_burn_prep_and_burn (ThreadBurnCompositionParams * params, struct burn_drive *drive,
                           struct burn_disc *disc, struct burn_session *session, struct burn_track *track)
{
  GtkWidget *dialog_progress = params->dialog_progress;

  struct burn_write_opts * burn_options;
  struct burn_source **fifos = NULL;
  gint ret;
  int sectors[1];

  ret = burn_disc_add_session (disc, session, BURN_POS_END);
  if (ret == 0) {
    g_warning ("Unable to create disc object");
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("An error occurred in the burn backend."));
    return;
  }

  if (burn_track_set_source (track, params->src) != BURN_SOURCE_OK) {
    g_warning ("Cannot attach source object to track object");
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("An error occurred in the burn backend."));
    return;
  }
  
  burn_session_add_track (session, track, BURN_POS_END);

  burn_options = burn_write_opts_new (drive);
  burn_write_opts_set_perform_opc (burn_options, 0);
  burn_write_opts_set_multi (burn_options, 0);
  /* enable this when there is some time for testing
  DBG ("Enabling multisession");
  burn_write_opts_set_multi(burn_options, 1);
  */
  burn_write_opts_set_simulate(burn_options, params->dummy ? 1 : 0);
  burn_write_opts_set_underrun_proof (burn_options, params->burnfree ? 1 : 0);
  /*
   * 32 is the number of blocks to skip. This will increase reliability with BD disks that
   * are used for multisessions. xfburn doesn't support this, but other programs may be using
   * the disc that way, so we respect that here.
   */
  burn_write_opts_set_stream_recording (burn_options, params->stream_recording ? 32 : 0);

  if (!xfburn_set_write_mode (burn_options, params->write_mode, disc, WRITE_MODE_TAO)) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("The write mode is not supported currently."));
  } else {

    DBG ("Set speed to %d kb/s", params->speed);
    burn_drive_set_speed (drive, 0, params->speed);

    sectors[0] = burn_disc_get_sectors (disc);

    if (params->is_fifo) {
      fifos = g_new(struct burn_source *,1);
      fifos[0] = params->src;
    }

    xfburn_perform_burn_write (dialog_progress, drive, params->write_mode, burn_options, DATA_BYTES_PER_SECTOR, disc, fifos, sectors);

    if (params->is_fifo) {
      g_free (fifos);
    }
  }

  burn_write_opts_free (burn_options);
}

static void*
thread_burn_composition (ThreadBurnCompositionParams * params)
{
  GtkWidget *dialog_progress = params->dialog_progress;

  struct burn_disc *disc;
  struct burn_session *session;
  struct burn_track *track;

  struct burn_drive_info *drive_info = NULL;

  disc = burn_disc_create ();
  session = burn_session_create ();
  track = burn_track_create ();

  if (!xfburn_device_grab (params->device, &drive_info)) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to grab the drive."));
  } else {
    thread_burn_prep_and_burn (params, drive_info->drive, disc, session, track);
    xfburn_device_release (drive_info, params->eject);
  }

  burn_track_free (track);
  burn_session_free (session);
  burn_disc_free (disc);
  burn_source_free (params->src);

  g_free (params);
  return NULL;
}

static void
cb_dialog_response (XfburnBurnDataCompositionBaseDialog * dialog, gint response_id, XfburnBurnDataCompositionBaseDialogPrivate * priv)
{
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *dialog_progress;

    struct burn_source * src = NULL;
    IsoWriteOpts *write_opts;

    /* If the name was the default, update the image volume id and volset id */
    if (priv->entry != NULL) {
      const gchar * comp_name = gtk_entry_get_text (GTK_ENTRY (priv->entry));
      iso_image_set_volume_id (priv->image, comp_name);
      iso_image_set_volset_id (priv->image, comp_name);
    }

    /* Sets profile 0 [basic] */
    iso_write_opts_new (&write_opts, 0);

    iso_write_opts_set_iso_level (write_opts, 3);
    iso_write_opts_set_rockridge (write_opts, 1);
    iso_write_opts_set_joliet (write_opts, 1);
    iso_write_opts_set_allow_deep_paths (write_opts, 1);
    /* not sure if we want that
    iso_write_opts_set_omit_version_numbers (write_opts, TRUE);
    */

    if (iso_image_create_burn_source (priv->image, write_opts, &src) < 0) {
      /* could not create source */
      xfce_dialog_show_error (NULL, NULL, _("Could not create ISO source structure."));
      return;
    }

    /* iso_image_create_burn_source copies the data it needs */
    iso_write_opts_free (write_opts);

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
      g_thread_new ("burn_iso", (GThreadFunc) thread_write_iso, params);
    }
    else {
      ThreadBurnCompositionParams *params = NULL;
      XfburnDevice *device;
      gint speed;
      XfburnWriteMode write_mode;
      struct burn_source * src_fifo = NULL;

      device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));
      speed = xfburn_device_box_get_speed (XFBURN_DEVICE_BOX (priv->device_box));
      write_mode = xfburn_device_box_get_mode (XFBURN_DEVICE_BOX (priv->device_box));

      src_fifo = burn_fifo_source_new (src, 2048, xfburn_settings_get_int ("fifo-size", FIFO_DEFAULT_SIZE) / 2, 0);
      burn_source_free (src);

      /* burn composition */
      params = g_new0 (ThreadBurnCompositionParams, 1);
      params->dialog_progress = dialog_progress;
      params->device = device;
      params->src = src_fifo;
      params->is_fifo = TRUE;
      params->speed = speed;
      params->write_mode = write_mode;
      params->eject = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject));
      params->dummy = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_dummy));
      params->burnfree = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_burnfree));
      params->stream_recording = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_stream_recording));
      g_thread_new ("burn_composition", (GThreadFunc) thread_burn_composition, params);
    }
  }
}

/* public */
GtkWidget *
xfburn_burn_data_composition_base_dialog_new (IsoImage *image)
{
  XfburnBurnDataCompositionBaseDialog *obj;

  obj = XFBURN_BURN_DATA_COMPOSITION_BASE_DIALOG (g_object_new (XFBURN_TYPE_BURN_DATA_COMPOSITION_BASE_DIALOG, "image", image, NULL));
  
  return GTK_WIDGET (obj);
}
