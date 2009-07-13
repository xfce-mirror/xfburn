/* $Id$ */
/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *  Copyright (c) 2008      David Mohr (squisher@xfce.org)
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
#include "xfburn-settings.h"
#include "xfburn-main.h"

#include "xfburn-burn-image-dialog.h"
#include "xfburn-perform-burn.h"

#define XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_BURN_IMAGE_DIALOG, XfburnBurnImageDialogPrivate))

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

typedef struct
{
  GtkWidget *chooser_image;
  GtkWidget *image_label;
  
  GtkWidget *device_box;

  GtkWidget *check_eject;
  GtkWidget *check_burnfree;
  GtkWidget *check_dummy;

  GtkWidget *burn_button;

  ThreadBurnIsoParams *params;
} XfburnBurnImageDialogPrivate;

/* prototypes */
static void xfburn_burn_image_dialog_class_init (XfburnBurnImageDialogClass * klass);
static void xfburn_burn_image_dialog_init (XfburnBurnImageDialog * sp);

void burn_image_dialog_error (XfburnBurnImageDialog * dialog, const gchar * msg_error);
static void cb_volume_change_end (XfburnDeviceList *devlist, gboolean device_changed, XfburnDevice *device, XfburnBurnImageDialog * dialog);
static void cb_dialog_response (XfburnBurnImageDialog * dialog, gint response_id, gpointer user_data);

static void update_image_label (GtkFileChooser *chooser, XfburnBurnImageDialog * dialog);
static void check_burn_button (XfburnBurnImageDialog * dialog);
static gboolean check_media (XfburnBurnImageDialog * dialog, ThreadBurnIsoParams *params, struct burn_drive *drive, struct burn_write_opts * burn_options);
static void cb_clicked_ok (GtkButton * button, gpointer user_data);

/*********************/
/* class declaration */
/*********************/
static XfceTitledDialogClass *parent_class = NULL;

GType
xfburn_burn_image_dialog_get_type ()
{
  static GType type = 0;

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
      NULL
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
  XfburnDeviceList *devlist;
  XfburnDevice *device;

  gtk_window_set_title (GTK_WINDOW (obj), _("Burn image"));
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
  
  /* red label for image */
  priv->image_label = gtk_label_new ("");
  gtk_widget_show (priv->image_label);
  gtk_box_pack_start (GTK_BOX (box), priv->image_label, FALSE, FALSE, 0);
  update_image_label (GTK_FILE_CHOOSER (priv->chooser_image), obj);
  g_signal_connect (G_OBJECT (priv->chooser_image), "selection-changed", G_CALLBACK (update_image_label), obj);
    
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

  priv->burn_button = xfce_create_mixed_button ("xfburn-burn-cd", _("_Burn image"));
  gtk_widget_show (priv->burn_button);
  g_signal_connect (G_OBJECT (priv->burn_button), "clicked", G_CALLBACK (cb_clicked_ok), obj);
  gtk_container_add (GTK_CONTAINER( GTK_DIALOG(obj)->action_area), priv->burn_button);
  //gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (priv->burn_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (priv->burn_button);
  gtk_widget_grab_default (priv->burn_button);

  devlist = xfburn_device_list_new ();

  g_signal_connect (G_OBJECT (devlist), "volume-change-end", G_CALLBACK (cb_volume_change_end), obj);
  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (cb_dialog_response), obj);
  device = xfburn_device_list_get_current_device (devlist);

  cb_volume_change_end (devlist, TRUE, device, obj);

  if (device)
    gtk_widget_set_sensitive (priv->check_dummy, xfburn_device_can_dummy_write (device));

  g_object_unref (G_OBJECT (devlist));

}

/*************/
/* internals */
/*************/
static struct burn_write_opts * 
make_burn_options(ThreadBurnIsoParams *params, struct burn_drive *drive) {
  struct burn_write_opts * burn_options;

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
  /*
  case WRITE_MODE_RAW16:
    burn_write_opts_set_write_type (burn_options, BURN_WRITE_RAW, BURN_BLOCK_RAW16);
    break;
  case WRITE_MODE_RAW96P:
    burn_write_opts_set_write_type (burn_options, BURN_WRITE_RAW, BURN_BLOCK_RAW96P);
    break;
  case WRITE_MODE_RAW96R:
    burn_write_opts_set_write_type (burn_options, BURN_WRITE_RAW, BURN_BLOCK_RAW96R);
    break;
  */
  default:
    burn_write_opts_free (burn_options);
    return NULL;
  }

  burn_write_opts_set_simulate(burn_options, params->dummy ? 1 : 0);
  burn_write_opts_set_underrun_proof (burn_options, params->burnfree ? 1 : 0);

  return burn_options;
}

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
  struct burn_source *fifo_src;
  int sectors[1];

  struct burn_drive *drive;
  struct burn_drive_info *drive_info = NULL;
  struct burn_write_opts * burn_options;
  struct burn_source **fifos = NULL;

  gint ret;

  disc = burn_disc_create ();
  session = burn_session_create ();
  track = burn_track_create ();

  ret = burn_disc_add_session (disc, session, BURN_POS_END);
  if (ret == 0) {
    g_warning ("Unable to create disc object");
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("An error occurred in the burn backend"));
    goto end;
  }

  burn_track_define_data (track, 0, 300*1024, 1, BURN_MODE1);

  fd = open (params->iso_path, O_RDONLY);
  if (fd >= 0)
    if (fstat (fd, &stbuf) != -1)
      if( (stbuf.st_mode & S_IFMT) == S_IFREG)
	fixed_size = stbuf.st_size;

  if (fixed_size == 0) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to determine image size."));
    goto end;
  }

  data_src = burn_fd_source_new(fd, -1, fixed_size);

  if (data_src == NULL) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot open image."));
    goto end;
  }

  fifo_src = burn_fifo_source_new (data_src, 2048, xfburn_settings_get_int ("fifo-size", FIFO_DEFAULT_SIZE) / 2, 0);
  burn_source_free (data_src);

  if (burn_track_set_source (track, fifo_src) != BURN_SOURCE_OK) {
    g_warning ("Cannot attach source object to track object");
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("An error occurred in the burn backend"));
    goto end;
  }
  
  burn_session_add_track (session, track, BURN_POS_END);

  if (!xfburn_device_grab (params->device, &drive_info)) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to grab the drive."));

    goto end;
  }

  drive = drive_info->drive;

  burn_options = make_burn_options (params, drive);
  if (burn_options == NULL) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Burn mode is not currently implemented."));
    goto cleanup;
  }

  DBG ("Set speed to %d kb/s", params->speed);
  burn_drive_set_speed (drive, 0, params->speed);

  // this assumes that an iso image can only have one track
  sectors[0] = burn_disc_get_sectors (disc);
  
  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Burning image..."));

  fifos = g_new(struct burn_source *,1);
  fifos[0] = fifo_src;

  xfburn_perform_burn_write (dialog_progress, drive, params->write_mode, burn_options, DATA_BYTES_PER_SECTOR, disc, fifos, sectors);

  g_free (fifos);

  burn_source_free (fifo_src);
  burn_write_opts_free (burn_options);

 cleanup:
  xfburn_device_release (drive_info, params->eject);
 end:
  burn_track_free (track);
  burn_session_free (session);
  burn_disc_free (disc);

  g_free (params->iso_path);
  g_free (params);
}

/**
 * Error message wrapper, so the appearance can be customized later
 **/
void
burn_image_dialog_error (XfburnBurnImageDialog * dialog, const gchar * msg_error)
{
  xfce_err (msg_error);
}

static void
cb_volume_change_end (XfburnDeviceList *devlist, gboolean device_changed, XfburnDevice *device, XfburnBurnImageDialog * dialog)
{
  XfburnBurnImageDialogPrivate *priv = XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE (dialog);

  if (device_changed)
    gtk_widget_set_sensitive (priv->check_dummy, xfburn_device_can_dummy_write (device));
  check_burn_button (dialog);
}

static void
cb_dialog_response (XfburnBurnImageDialog * dialog, gint response_id, gpointer user_data)
{

  if (response_id == GTK_RESPONSE_OK) {
    XfburnBurnImageDialogPrivate *priv = XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE (dialog);

    GtkWidget *dialog_progress;

    dialog_progress = xfburn_progress_dialog_new (GTK_WINDOW (dialog));
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), gtk_window_get_transient_for (GTK_WINDOW (dialog)));
    gtk_widget_hide (GTK_WIDGET (dialog));
    
    priv->params->dialog_progress = dialog_progress;
    gtk_widget_show (dialog_progress);
    
    g_thread_create ((GThreadFunc) thread_burn_iso, priv->params, FALSE, NULL);
  } else {
    xfburn_main_leave_window ();
  }
}

static void
update_image_label (GtkFileChooser *chooser, XfburnBurnImageDialog * dialog)
{
  XfburnBurnImageDialogPrivate *priv = XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE (dialog);

  if (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser)) == NULL) {
    gtk_label_set_markup (GTK_LABEL(priv->image_label),
                          _("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">Please select an image to burn</span>"));   
  } else {
    gtk_label_set_text (GTK_LABEL(priv->image_label), "");
    check_burn_button (dialog);
  }
}

static void
check_burn_button (XfburnBurnImageDialog * dialog)
{
  XfburnBurnImageDialogPrivate *priv = XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE (dialog);
  gboolean valid_disc;
  gchar *filename;

  g_object_get (G_OBJECT (priv->device_box), "valid", &valid_disc, NULL);
  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->chooser_image));
  if (filename != NULL) {
    gtk_widget_set_sensitive (priv->burn_button, valid_disc);
    g_free (filename);
  } else {
    gtk_widget_set_sensitive (priv->burn_button, FALSE);
  }
}

static gboolean 
check_media (XfburnBurnImageDialog * dialog, ThreadBurnIsoParams *params, struct burn_drive *drive, struct burn_write_opts * burn_options)
{
  enum burn_disc_status disc_state;
  struct stat st;
  int ret;

  while (burn_drive_get_status (drive, NULL) != BURN_DRIVE_IDLE)
    usleep(100001);

  /* Evaluate drive and disc */
  while ((disc_state = burn_disc_get_status(drive)) == BURN_DISC_UNREADY)
    usleep(100001);
  if (disc_state == BURN_DISC_APPENDABLE && params->write_mode != WRITE_MODE_TAO) {
    burn_image_dialog_error (dialog, _("Cannot append data to multisession disc in this write mode (use TAO instead)"));
    return FALSE;
  } else if (disc_state != BURN_DISC_BLANK) {
    if (disc_state == BURN_DISC_FULL)
      burn_image_dialog_error (dialog, _("Closed disc with data detected. Need blank or appendable disc"));
    else if (disc_state == BURN_DISC_EMPTY) 
      burn_image_dialog_error (dialog, _("No disc detected in drive"));
    else {
      burn_image_dialog_error (dialog, _("Cannot recognize state of drive and disc"));
      DBG ("disc_state = %d", disc_state);
    }
    return FALSE;
  }

  /* check if the image fits on the inserted disc */
  ret = stat (params->iso_path, &st);
  if (ret == 0) {
    off_t disc_size;
    disc_size = burn_disc_available_space (drive, burn_options);
    if (st.st_size > disc_size) {
      burn_image_dialog_error (dialog, _("The selected image does not fit on the inserted disc"));
      return FALSE;
    }
  } else {
    burn_image_dialog_error (dialog, _("Failed to get image size"));
    return FALSE;
  }

  return TRUE;
}

static void 
cb_clicked_ok (GtkButton *button, gpointer user_data)
{
  XfburnBurnImageDialog * dialog = (XfburnBurnImageDialog *) user_data;
  XfburnBurnImageDialogPrivate *priv = XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE (dialog);
  FILE *fp;
  char *iso_path;
  gboolean checks_passed = FALSE;
  XfburnDevice *device;
  gint speed;
  XfburnWriteMode write_mode;
  struct burn_write_opts * burn_options;

  ThreadBurnIsoParams *params = NULL;
  struct burn_drive_info *drive_info = NULL;

  /* check if the image file really exists and can be opened */
  iso_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->chooser_image));

  fp = fopen(iso_path, "r");
  if (fp == NULL) {
    burn_image_dialog_error (dialog, _("Make sure you selected a valid file and you have the proper permissions to access it."));

    return;
  }
  fclose(fp);

  device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));
  speed = xfburn_device_box_get_speed (XFBURN_DEVICE_BOX (priv->device_box));
  write_mode = xfburn_device_box_get_mode (XFBURN_DEVICE_BOX (priv->device_box));
      
  params = g_new0 (ThreadBurnIsoParams, 1);
  params->device = device;
  params->iso_path = iso_path;
  params->speed = speed;
  params->write_mode = write_mode;
  params->eject = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject));
  params->dummy = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_dummy));
  params->burnfree = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_burnfree));
  
  if (!xfburn_device_grab (device, &drive_info)) {
    burn_image_dialog_error (dialog, _("Unable to grab the drive."));

    g_free (params->iso_path);
    g_free (params);
    return;
  }

  burn_options = make_burn_options (params, drive_info->drive);

  if (burn_options == NULL)
    burn_image_dialog_error (dialog, _("The write mode is not currently supported"));
  else {
    checks_passed = check_media (dialog, params, drive_info->drive, burn_options);

    burn_write_opts_free (burn_options);
  }

  xfburn_device_release (drive_info, 0);

  priv->params = params;

  if (checks_passed)
    gtk_dialog_response (GTK_DIALOG(dialog), GTK_RESPONSE_OK);
  else {
    g_free (params->iso_path);
    g_free (params);
  }
}


/* public */
GtkWidget *
xfburn_burn_image_dialog_new ()
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_BURN_IMAGE_DIALOG, NULL));

  xfburn_main_enter_window ();

  return obj;
}

void xfburn_burn_image_dialog_set_filechooser_name ( GtkWidget * dialog, gchar *name)
{
  XfburnBurnImageDialogPrivate *priv = XFBURN_BURN_IMAGE_DIALOG_GET_PRIVATE (XFBURN_BURN_IMAGE_DIALOG (dialog));

  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (priv->chooser_image), name);
}
