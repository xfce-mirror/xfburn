/* $Id$ */
/*
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

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-blank-cd-progress-dialog.h"
#include "xfburn-stock.h"

#include "xfburn-blank-cd-dialog.h"


static void xfburn_blank_cd_dialog_class_init (XfburnBlankCdDialogClass * klass);
static void xfburn_blank_cd_dialog_init (XfburnBlankCdDialog * sp);
static void xfburn_blank_cd_dialog_finalize (GObject * object);

static void xfburn_blank_cd_dialog_response_cb (XfburnBlankCdDialog * dialog, gint response_id, gpointer user_data);

struct XfburnBlankCdDialogPrivate
{
  GtkWidget *combo_device;
  GtkWidget *combo_type;
  GtkWidget *combo_speed;
  GtkWidget *check_force;
  GtkWidget *check_eject;
};

static GtkDialogClass *parent_class = NULL;

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

    type = g_type_register_static (GTK_TYPE_DIALOG, "XfburnBlankCdDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_blank_cd_dialog_class_init (XfburnBlankCdDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_blank_cd_dialog_finalize;

}

static void
xfburn_blank_cd_dialog_init (XfburnBlankCdDialog * obj)
{
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  XfburnBlankCdDialogPrivate *priv;
  GList *device;
  GtkWidget *img;
  GdkPixbuf *icon = NULL;
  GtkWidget *heading;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  int i;

  obj->priv = g_new0 (XfburnBlankCdDialogPrivate, 1);

  priv = obj->priv;
  
  gtk_window_set_title (GTK_WINDOW (obj), _("Blank CD-RW"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  
  heading = xfce_heading_new ();
  xfce_heading_set_title (XFCE_HEADING (heading), _("Blank CD-RW"));
  icon = gtk_widget_render_icon (heading, XFBURN_STOCK_BLANK_CDRW, GTK_ICON_SIZE_DIALOG, NULL);
  xfce_heading_set_icon (XFCE_HEADING (heading), icon);
  g_object_unref (icon);
  gtk_widget_show (heading);
  gtk_box_pack_start (box, heading, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);

  frame = xfce_create_framebox_with_content (_("Burning device"), vbox);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  /* devices list */
  priv->combo_device = gtk_combo_box_new_text ();
  gtk_widget_show (priv->combo_device);
  gtk_box_pack_start (GTK_BOX (vbox), priv->combo_device, FALSE, FALSE, BORDER);

  device = list_devices;
  while (device) {
    XfburnDevice *device_data = (XfburnDevice *) device->data;

    gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_device), device_data->name);

    device = g_list_next (device);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_device), 0);

  /* speed */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, BORDER);

  label = gtk_label_new_with_mnemonic (_("_Speed :"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, BORDER);

  priv->combo_speed = gtk_combo_box_new_text ();
  gtk_widget_show (priv->combo_speed);
  gtk_box_pack_start (GTK_BOX (hbox), priv->combo_speed, TRUE, TRUE, BORDER);

  for (i = 2; i <= 24; i += 2) {
    gchar *str;

    str = g_strdup_printf ("%d", i);
    gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_speed), str);
    g_free (str);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_speed), 4);

  img = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (img);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  /* blank type */
  priv->combo_type = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Fast"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Complete"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Reopen last session"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Erase last session"));
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

  priv->check_force = gtk_check_button_new_with_mnemonic (_("_Force"));
  gtk_widget_show (priv->check_force);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_force, FALSE, FALSE, BORDER);

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

static void
xfburn_blank_cd_dialog_finalize (GObject * object)
{
  XfburnBlankCdDialog *cobj;
  cobj = XFBURN_BLANK_CD_DIALOG (object);
  
  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfburn_blank_cd_dialog_response_cb (XfburnBlankCdDialog * dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    XfburnBlankCdDialogPrivate *priv;
    gchar *command;
    XfburnDevice *device;
    gchar *device_name, *blank_type, *speed;
    
    priv = dialog->priv;

    device_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_device));
    device = xfburn_device_lookup_by_name (device_name);

    speed = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_speed));

    switch (gtk_combo_box_get_active (GTK_COMBO_BOX (priv->combo_type))) {
    case 0:
      blank_type = g_strdup ("fast");
      break;
    case 1:
      blank_type = g_strdup ("all");
      break;
    case 2:
      blank_type = g_strdup ("unclose");
      break;
    case 3:
      blank_type = g_strdup ("session");
      break;
    default:
      blank_type = g_strdup ("fast");
    }

    command = g_strconcat ("cdrecord -v gracetime=2", " dev=", device->node_path, " blank=", blank_type, " speed=", speed,
                        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject)) ? " -eject" : "",
                        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_force)) ? " -force" : "", NULL);
    
    g_free (device_name);
    g_free (blank_type);
    g_free (speed);
    
    GtkWidget *dialog_progress;
        
    dialog_progress = xfburn_blank_cd_progress_dialog_new ();
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), gtk_window_get_transient_for (GTK_WINDOW (dialog)));
    gtk_widget_hide (GTK_WIDGET (dialog));
    
    g_object_set_data (G_OBJECT (dialog_progress), "command", command);
    gtk_dialog_run (GTK_DIALOG (dialog_progress));
    
    g_free (command);
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
