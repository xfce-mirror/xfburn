/* $Id$ */
/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#include <libxfcegui4/libxfcegui4.h>

#include <libburn.h>

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-device-box.h"
#include "xfburn-stock.h"

#include "xfburn-blank-cd-dialog.h"

#define XFBURN_BLANK_CD_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_BLANK_CD_DIALOG, XfburnBlankCdDialogPrivate))

typedef struct
{
  GtkWidget *device_box;
  GtkWidget *combo_type;
  
  GtkWidget *check_eject;
} XfburnBlankCdDialogPrivate;

static void xfburn_blank_cd_dialog_class_init (XfburnBlankCdDialogClass * klass);
static void xfburn_blank_cd_dialog_init (XfburnBlankCdDialog * sp);

static void xfburn_blank_cd_dialog_response_cb (XfburnBlankCdDialog * dialog, gint response_id, gpointer user_data);

static XfceTitledDialogClass *parent_class = NULL;

GtkType
xfburn_blank_cd_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnBlankCdDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_blank_cd_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnBlankCdDialog),
      0,
      (GInstanceInitFunc) xfburn_blank_cd_dialog_init,
    };

    type = g_type_register_static (XFCE_TYPE_TITLED_DIALOG, "XfburnBlankCdDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_blank_cd_dialog_class_init (XfburnBlankCdDialogClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);
  
  g_type_class_add_private (klass, sizeof (XfburnBlankCdDialogPrivate));
}

static void
xfburn_blank_cd_dialog_init (XfburnBlankCdDialog * obj)
{
  XfburnBlankCdDialogPrivate *priv = XFBURN_BLANK_CD_DIALOG_GET_PRIVATE (obj);
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  GdkPixbuf *icon = NULL;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button;
  
  gtk_window_set_title (GTK_WINDOW (obj), _("Blank CD-RW"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  
  icon = gtk_widget_render_icon (GTK_WIDGET (obj), XFBURN_STOCK_BLANK_CDRW, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_window_set_icon (GTK_WINDOW (obj), icon);
  g_object_unref (icon);

  /* devices list */
  priv->device_box = xfburn_device_box_new (SHOW_CDRW_WRITERS);
  gtk_widget_show (priv->device_box);

  frame = xfce_create_framebox_with_content (_("Burning device"), priv->device_box);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  /* blank type */
  priv->combo_type = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Fast"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Complete"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_type), 0);
  gtk_widget_show (priv->combo_type);

  frame = xfce_create_framebox_with_content (_("Blank type"), priv->combo_type);
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

  /* action buttons */
  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CANCEL);

  button = xfce_create_mixed_button ("xfburn-blank-cdrw", _("_Blank"));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);

  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (xfburn_blank_cd_dialog_response_cb), obj);
}

typedef struct {
  GtkWidget *dialog_progress;
  XfburnDevice *device;
  gint blank_type;
  gboolean eject;
} ThreadBlankParams;

static void
thread_blank (ThreadBlankParams * params)
{
  GtkWidget *dialog_progress = params->dialog_progress;

  struct burn_drive_info *drive_info = NULL;
  struct burn_drive *drive;
  enum burn_disc_status disc_state;
  struct burn_progress progress;

  if (!burn_initialize ()) {
    g_critical ("Unable to initialize libburn");
    g_free (params);
    return;
  }

  if (!xfburn_device_grab (params->device, &drive_info)) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Unable to grab drive"));

    goto end;
  }

  drive = drive_info->drive;

  while (burn_drive_get_status (drive, NULL) != BURN_DRIVE_IDLE) {
    usleep (1001);
  }

  while ( (disc_state = burn_disc_get_status (drive)) == BURN_DISC_UNREADY)
    usleep (1001);

  switch (disc_state) {
  case BURN_DISC_BLANK:
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("The inserted disc is already blank"));
    goto cleanup;
  case BURN_DISC_FULL:
  case BURN_DISC_APPENDABLE:
    /* these ones we can blank */
    xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Ready"));
    break;
  case BURN_DISC_EMPTY:
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("No disc detected in the drive"));
    goto cleanup;
  default:
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Cannot recognize drive and media state"));
    goto cleanup;
  }

  if (!burn_disc_erasable (drive)) {
    xfburn_progress_dialog_burning_failed (XFBURN_PROGRESS_DIALOG (dialog_progress), _("Media is not erasable"));
    goto cleanup;
  }

  burn_disc_erase(drive, params->blank_type);
  sleep(1);

  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_RUNNING, _("Blanking disc..."));

  while (burn_drive_get_status (drive, &progress) != BURN_DRIVE_IDLE) {
    if(progress.sectors>0 && progress.sector>=0) {
      gdouble percent = 1.0 + ((gdouble) progress.sector+1.0) / ((gdouble) progress.sectors) * 98.0;
      
      xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog_progress), percent / 100.0);
    }
    usleep(500000);
  }

  xfburn_progress_dialog_set_status_with_text (XFBURN_PROGRESS_DIALOG (dialog_progress), XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED, _("Done"));

 cleanup:
  burn_drive_release (drive, params->eject ? 1 : 0);
 end:
  burn_finish ();
  g_free (params);
}

static void
xfburn_blank_cd_dialog_response_cb (XfburnBlankCdDialog * dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    XfburnBlankCdDialogPrivate *priv = XFBURN_BLANK_CD_DIALOG_GET_PRIVATE (dialog);
    XfburnDevice *device;
    gint blank_type;

    GtkWidget *dialog_progress;
    ThreadBlankParams *params = NULL;

    device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));

    switch (gtk_combo_box_get_active (GTK_COMBO_BOX (priv->combo_type))) {
    case 0:
      /* fast blanking */
      blank_type = 1;
      break;
    case 1:
      /* normal blanking */
      blank_type = 0;
      break;
    default:
      blank_type = 1;
    }
        
    dialog_progress = xfburn_progress_dialog_new (GTK_WINDOW (dialog));
    gtk_widget_hide (GTK_WIDGET (dialog));

    gtk_widget_show (dialog_progress);

    params = g_new0 (ThreadBlankParams, 1);
    params->dialog_progress = dialog_progress;
    params->device = device;
    params->blank_type = blank_type;
    params->eject = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject)); 
    g_thread_create ((GThreadFunc) thread_blank, params, FALSE, NULL);
  }
}
   

/* public */
GtkWidget *
xfburn_blank_cd_dialog_new ()
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_BLANK_CD_DIALOG, NULL));

  return obj;
}
