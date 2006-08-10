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

#include <gtk/gtk.h>

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-burn-image-progress-dialog.h"
#include "xfburn-device-box.h"
#include "xfburn-stock.h"
#include "xfburn-write-mode-combo-box.h"

#include "xfburn-burn-image-dialog.h"


static void xfburn_burn_image_dialog_class_init (XfburnBurnImageDialogClass * klass);
static void xfburn_burn_image_dialog_init (XfburnBurnImageDialog * sp);
static void xfburn_burn_image_dialog_finalize (GObject * object);

static void xfburn_burn_image_dialog_response_cb (XfburnBurnImageDialog * dialog, gint response_id, gpointer user_data);

struct XfburnBurnImageDialogPrivate
{
  GtkWidget *chooser_image;
  
  GtkWidget *device_box;
  GtkWidget *combo_speed;
  GtkWidget *combo_mode;

  GtkWidget *check_eject;
  GtkWidget *check_burnfree;
  GtkWidget *check_dummy;
};

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
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_burn_image_dialog_finalize;

}

static void
xfburn_burn_image_dialog_init (XfburnBurnImageDialog * obj)
{
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  XfburnBurnImageDialogPrivate *priv;
  GdkPixbuf *icon = NULL;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;

  obj->priv = g_new0 (XfburnBurnImageDialogPrivate, 1);

  priv = obj->priv;

  gtk_window_set_title (GTK_WINDOW (obj), _("Burn CD image"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  icon = gtk_widget_render_icon (GTK_WIDGET (obj), XFBURN_STOCK_BURN_CD, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_window_set_icon (GTK_WINDOW (obj), icon);
  g_object_unref (icon);
  
  /* file */
  priv->chooser_image = gtk_file_chooser_button_new (_("Image to burn"), GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_widget_show (priv->chooser_image);

  frame = xfce_create_framebox_with_content (_("Image to burn"), priv->chooser_image);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  /* devices list */
  priv->device_box = xfburn_device_box_new (TRUE, TRUE);
  gtk_widget_show (priv->device_box);
  
  frame = xfce_create_framebox_with_content (_("Burning device"), priv->device_box);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  /* mode */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (priv->device_box), hbox, FALSE, FALSE, BORDER);

  label = gtk_label_new_with_mnemonic (_("Write _mode :"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, BORDER);

  priv->combo_mode = xfburn_write_mode_combo_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), priv->combo_mode, TRUE, TRUE, BORDER);
  gtk_widget_show (priv->combo_mode);

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

  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (xfburn_burn_image_dialog_response_cb), obj);
}

/* internals */
static void
xfburn_burn_image_dialog_finalize (GObject * object)
{
  XfburnBurnImageDialog *cobj;
  cobj = XFBURN_BURN_IMAGE_DIALOG (object);

  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfburn_burn_image_dialog_response_cb (XfburnBurnImageDialog * dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    XfburnBurnImageDialogPrivate *priv;
    XfburnDevice *device;
    gchar *iso_path, *speed, *write_mode;
    gchar *command;
    GtkWidget *dialog_progress;
    
    priv = dialog->priv;

    iso_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->chooser_image));

    device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));

    speed = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_speed));

    write_mode = xfburn_write_mode_combo_box_get_cdrecord_param (XFBURN_WRITE_MODE_COMBO_BOX (priv->combo_mode));

    command = g_strconcat ("cdrecord -v gracetime=2", " dev=", device->node_path, " ", write_mode, " speed=", speed,
                           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject)) ? " -eject" : "",
                           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_dummy)) ? " -dummy" : "",
                           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_burnfree)) ? " driveropts=burnfree"
                           : "", " '", iso_path, "'", NULL);
    g_free (write_mode);
    g_free (speed);
    g_free (iso_path);
       
    dialog_progress = xfburn_burn_image_progress_dialog_new (GTK_WINDOW (dialog));
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), gtk_window_get_transient_for (GTK_WINDOW (dialog)));
    gtk_widget_hide (GTK_WIDGET (dialog));
    
    g_object_set_data (G_OBJECT (dialog_progress), "command", command);
    gtk_dialog_run (GTK_DIALOG (dialog_progress));
    
    g_free (command);
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
