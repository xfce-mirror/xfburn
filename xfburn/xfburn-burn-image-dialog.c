/* $Id$ */
/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <gtk/gtk.h>

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-device-box.h"
#include "xfburn-stock.h"

#include "xfburn-burn-image-dialog.h"

#define XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_BURN_IMAGE_DIALOG, XfburnBurnImageDialogPrivate))

typedef struct
{
  GtkWidget *chooser_image;
  
  GtkWidget *device_box;

  GtkWidget *check_eject;
  GtkWidget *check_burnfree;
  GtkWidget *check_dummy;
} XfburnBurnImageDialogPrivate;

/* prototypes */
static void xfburn_burn_image_dialog_class_init (XfburnBurnImageDialogClass * klass);
static void xfburn_burn_image_dialog_init (XfburnBurnImageDialog * sp);

static void cb_device_changed (XfburnDeviceBox *box, XfburnDevice *device, XfburnBurnImageDialog * dialog);
static void cb_dialog_response (XfburnBurnImageDialog * dialog, gint response_id, gpointer user_data);

/*********************/
/* class declaration */
/*********************/
static XfceTitledDialogClass *parent_class = NULL;

GtkType
xfburn_burn_image_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnBurnImageDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_burn_image_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnBurnImageDialog),
      0,
      (GInstanceInitFunc) xfburn_burn_image_dialog_init,
    };

    type = g_type_register_static (XFCE_TYPE_TITLED_DIALOG, "XfburnBurnImageDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_burn_image_dialog_class_init (XfburnBurnImageDialogClass * klass)
{
  g_type_class_add_private (klass, sizeof (XfburnBurnImageDialogPrivate));
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_burn_image_dialog_init (XfburnBurnImageDialog * obj)
{
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  XfburnBurnImageDialogPrivate *priv = XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE (obj);
  
  GdkPixbuf *icon = NULL;
  GtkFileFilter *filter;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button;
  XfburnDevice *device;

  gtk_window_set_title (GTK_WINDOW (obj), _("Burn CD image"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  icon = gtk_widget_render_icon (GTK_WIDGET (obj), XFBURN_STOCK_BURN_CD, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_window_set_icon (GTK_WINDOW (obj), icon);
  g_object_unref (icon);
  
  /* file */
  priv->chooser_image = gtk_file_chooser_button_new (_("Image to burn"), GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_widget_show (priv->chooser_image);

  filter =  gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(priv->chooser_image), filter);
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("ISO images"));
  gtk_file_filter_add_pattern (filter, "*.iso");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (priv->chooser_image), filter);
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (priv->chooser_image), filter);

  frame = xfce_create_framebox_with_content (_("Image to burn"), priv->chooser_image);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  /* devices list */
  priv->device_box = xfburn_device_box_new (SHOW_CD_WRITERS | SHOW_CDRW_WRITERS | SHOW_DVD_WRITERS | SHOW_MODE_SELECTION | SHOW_SPEED_SELECTION);
  gtk_widget_show (priv->device_box);
  
  frame = xfce_create_framebox_with_content (_("Burning device"), priv->device_box);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

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

  /* action buttons */
  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CANCEL);

  button = xfce_create_mixed_button ("xfburn-burn-cd", _("_Burn image"));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);

  g_signal_connect (G_OBJECT (priv->device_box), "device-changed", G_CALLBACK (cb_device_changed), obj);
  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (cb_dialog_response), obj);

  device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));
  if (device)
    gtk_widget_set_sensitive (priv->check_dummy, device->dummy_write);
}

/*************/
/* internals */
/*************/
typedef struct {
  GtkWidget *dialog_progress;
  XfburnDevice *device;
  gchar *iso_path;
  gint speed;
  XfburnWriteMode write_mode;
  gboolean eject;
  gboolean dummy;
  gboolean burnfree;
} ThreadBurnIsoParams;

static void
thread_burn_iso (ThreadBurnIsoParams * params)
{
  GtkWidget *dialog_progress = params->dialog_progress;

  struct burn_disc *disc;
  struct burn_session *session;
  struct burn_track *track;

  gint fd;
  struct stat stbuf;
  off_t fixed_size = 0;
  struct burn_source *data_src;

  struct burn_drive_info *drive_info = NULL;
  struct burn_drive *drive;

  struct burn_write_opts * burn_options;
  enum burn_disc_status disc_state;
  enum burn_drive_status status;
  struct burn_progress progress;
  gint ret;
  time_t time_start;

  if (!burn_initialize ()) {
    g_critical ("Unable to initialize libburn");
    g_free (params->iso_path);
    g_free (params);
    return;
  }

  disc = burn_disc_create ();
  session = burn_session_create ();
  track = burn_track_create ();

  ret = burn_disc_add_session (disc, session, BURN_POS_END);
  if (ret == 0) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to create disc object"));
    goto end;
  }

  burn_track_define_data (track, 0, 300*1024, 1, BURN_MODE1);

  fd = open (params->iso_path, O_RDONLY);
  if (fd >= 0)
    if (fstat (fd, &stbuf) != -1)
      if( (stbuf.st_mode & S_IFMT) == S_IFREG)
	fixed_size = stbuf.st_size;

  if (fixed_size == 0) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to determine image size"));
    goto end;
  }

  data_src = burn_fd_source_new(fd, -1, fixed_size);

  if (data_src == NULL) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot open image"));
    goto end;
  }
  if (burn_track_set_source (track, data_src) != BURN_SOURCE_OK) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot attach source object to track object"));
    goto end;
  }
  
  burn_session_add_track (session, track, BURN_POS_END);
  burn_source_free (data_src);

  if (!xfburn_device_grab (params->device, &drive_info)) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to grab drive"));

    goto end;
  }

  drive = drive_info->drive;

  while (burn_drive_get_status (drive, NULL) != BURN_DRIVE_IDLE)
    usleep(100001);
 
  /* Evaluate drive and media */
  while ((disc_state = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
    usleep(100001);
  if (disc_state == BURN_DISC_APPENDABLE && params->write_mode != WRITE_MODE_TAO) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot append data to multisession disc in this write mode (use TAO instead)"));
    goto cleanup;
  } else if (disc_state != BURN_DISC_BLANK) {
    if (disc_state == BURN_DISC_FULL)
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Closed media with data detected. Need blank or appendable media"));
    else if (disc_state == BURN_DISC_EMPTY) 
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("No media detected in drive"));
    else
      xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot recognize state of drive and media"));
    goto cleanup;
  }
 
  burn_options = burn_write_opts_new (drive);
  burn_write_opts_set_perform_opc (burn_options, 0);
  burn_write_opts_set_multi (burn_options, 0);

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
    goto cleanup;
  }

  burn_write_opts_set_simulate(burn_options, params->dummy ? 1 : 0);
  DBG ("TODO set speed");
  burn_drive_set_speed (drive, 0, 0);
  burn_write_opts_set_underrun_proof (burn_options, params->burnfree ? 1 : 0);

  burn_disc_write (burn_options, disc); 
  burn_write_opts_free (burn_options);

  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Burning image..."));

  while (burn_drive_get_status (drive, NULL) == BURN_DRIVE_SPAWNING)
    usleep(1002);
  time_start = time (NULL);
  while ((status = burn_drive_get_status (drive, &progress)) != BURN_DRIVE_IDLE) {
    time_t time_now = time (NULL);

    switch (status) {
    case BURN_DRIVE_WRITING:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Burning composition..."));
      if (progress.sectors > 0 && progress.sector >= 0) {
	gdouble percent = 0.0;

	percent = (gdouble) (progress.buffer_capacity - progress.buffer_available) / (gdouble) progress.buffer_capacity;
	xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), percent);

	percent = 1.0 + ((gdouble) progress.sector+1.0) / ((gdouble) progress.sectors) * 98.0;
	xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), percent / 100.0);

	xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						  ((gdouble) (progress.sector * 2048) / (gdouble) (time_now - time_start)) / (150 * 1024));
      }
      break;
    case BURN_DRIVE_WRITING_LEADIN:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Writing Lead-In..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    case BURN_DRIVE_WRITING_LEADOUT:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Writing Lead-Out..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    case BURN_DRIVE_WRITING_PREGAP:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Writing pregap..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    case BURN_DRIVE_CLOSING_TRACK:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Closing track..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    case BURN_DRIVE_CLOSING_SESSION:
      xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), 
						   XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Closing session..."));
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), -1);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog_progress), -1); 
      break;
    default:
      DBG ("Status not implemented: %d", status);
      break;
    }    

    usleep (500000);
  }

  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED, _("Done"));

 cleanup:
  burn_drive_release (drive, params->eject ? 1 : 0);
 end:
  burn_track_free (track);
  burn_session_free (session);
  burn_disc_free (disc);
  burn_finish ();

  g_free (params->iso_path);
  g_free (params);
}

static void
cb_device_changed (XfburnDeviceBox *box, XfburnDevice *device, XfburnBurnImageDialog * dialog) 
{
  XfburnBurnImageDialogPrivate *priv = XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE (dialog);

  gtk_widget_set_sensitive (priv->check_dummy, device->dummy_write);
}

static void
cb_dialog_response (XfburnBurnImageDialog * dialog, gint response_id, gpointer user_data)
{

  if (response_id == GTK_RESPONSE_OK) {
    XfburnBurnImageDialogPrivate *priv = XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE (dialog);

    XfburnDevice *device;
    gchar *iso_path;
    gint speed;
    XfburnWriteMode write_mode;

    GtkWidget *dialog_progress;
    ThreadBurnIsoParams *params = NULL;

    iso_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->chooser_image));

    device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));
    speed = xfburn_device_box_get_speed (XFBURN_DEVICE_BOX (priv->device_box));
    write_mode = xfburn_device_box_get_mode (XFBURN_DEVICE_BOX (priv->device_box));
       
    dialog_progress = xfburn_progress_dialog_new (GTK_WINDOW (dialog));
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), gtk_window_get_transient_for (GTK_WINDOW (dialog)));
    gtk_widget_hide (GTK_WIDGET (dialog));
    
    gtk_widget_show (dialog_progress);
    
    params = g_new0 (ThreadBurnIsoParams, 1);
    params->dialog_progress = dialog_progress;
    params->device = device;
    params->iso_path = iso_path;
    params->speed = speed;
    params->write_mode = write_mode;
    params->eject = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject));
    params->dummy = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_dummy));
    params->burnfree = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_burnfree));
    g_thread_create ((GThreadFunc) thread_burn_iso, params, FALSE, NULL);
  }
}

/* public */
GtkWidget *
xfburn_burn_image_dialog_new ()
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_BURN_IMAGE_DIALOG, NULL));

  return obj;
}
