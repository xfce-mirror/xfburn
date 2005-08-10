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

#include "xfburn-blank-cd-dialog.h"
#include "xfburn-global.h"
#include "xfburn-utils.h"

static void xfburn_blank_cd_dialog_class_init (XfburnBlankCdDialogClass * klass);
static void xfburn_blank_cd_dialog_init (XfburnBlankCdDialog * sp);
static void xfburn_blank_cd_dialog_finalize (GObject * object);

static void xfburn_blank_cd_dialog_response_cb (XfburnBlankCdDialog * dialog, gint response_id, gpointer user_data);

struct XfburnBlankCdDialogPrivate
{
  gchar *command;
  XfburnDevice *device;
  
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
  GtkWidget *header;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  int i;

  obj->priv = g_new0 (XfburnBlankCdDialogPrivate, 1);

  priv = obj->priv;
  priv->command = NULL;
  priv->device = NULL;
  
  gtk_window_set_title (GTK_WINDOW (obj), _("Blank CD-RW"));

  img = gtk_image_new_from_stock ("xfburn-blank-cdrw", GTK_ICON_SIZE_LARGE_TOOLBAR);
  header = xfce_create_header_with_image (img, _("Blank CD-RW"));
  gtk_widget_show (header);
  gtk_box_pack_start (box, header, FALSE, FALSE, 0);

  frame = xfce_framebox_new (_("Burning device"), TRUE);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

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
  frame = xfce_framebox_new (_("Blank type"), TRUE);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  priv->combo_type = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Fast"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Complete"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Reopen last session"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_type), _("Erase last session"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_type), 0);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), priv->combo_type);
  gtk_widget_show (priv->combo_type);

  /* options */
  frame = xfce_framebox_new (_("Options"), TRUE);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

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

  g_free (cobj->priv->command);
  
  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfburn_blank_cd_dialog_response_cb (XfburnBlankCdDialog * dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    XfburnBlankCdDialogPrivate *priv;
    gchar *device_name, *blank_type, *speed;
    gchar *temp;
    
    priv = dialog->priv;

    device_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_device));
    priv->device = xfburn_device_lookup_by_name (device_name);

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

    temp = g_strconcat ("cdrecord", " dev=", priv->device->node_path, " blank=", blank_type, " speed=", speed,
                        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject)) ? " -eject" : "",
                        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_force)) ? " -force" : "", NULL);
    g_free (priv->command);
    priv->command = temp;

    g_free (device_name);
    g_free (blank_type);
    g_free (speed);
  }
}

/* public */
XfburnDevice *
xfburn_blank_cd_dialog_get_device (XfburnBlankCdDialog *dialog)
{
  return dialog->priv->device;
}

gchar *
xfburn_blank_cd_dialog_get_command (XfburnBlankCdDialog *dialog)
{
  return g_strdup (dialog->priv->command);
}

GtkWidget *
xfburn_blank_cd_dialog_new ()
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_BLANK_CD_DIALOG, NULL));

  return obj;
}
