/*
 *  Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
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

#include "xfburn-global.h"

#include "xfburn-copy-cd-dialog.h"

/* prototypes */
static void xfburn_copy_cd_dialog_class_init (XfburnCopyCdDialogClass * klass);
static void xfburn_copy_cd_dialog_init (XfburnCopyCdDialog * sp);
static void xfburn_copy_cd_dialog_finalize (GObject * object);

/* structures */
struct XfburnCopyCdDialogPrivate
{
  GtkWidget *combo_source_device;
  GtkWidget *combo_dest_device;
  GtkWidget *combo_speed;
  GtkWidget *combo_mode;

  GtkWidget *check_eject;
  GtkWidget *check_burnfree;
  GtkWidget *check_only_iso;
  GtkWidget *check_dummy;
};

/* globals */
static GtkDialogClass *parent_class = NULL;

GtkType
xfburn_copy_cd_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnCopyCdDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_copy_cd_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnCopyCdDialog),
      0,
      (GInstanceInitFunc) xfburn_copy_cd_dialog_init,
    };

    type = g_type_register_static (GTK_TYPE_DIALOG, "XfburnCopyCdDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_copy_cd_dialog_class_init (XfburnCopyCdDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_copy_cd_dialog_finalize;

}

static void
xfburn_copy_cd_dialog_init (XfburnCopyCdDialog * obj)
{
  XfburnCopyCdDialogPrivate *priv;
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  GList *device;
  GtkWidget *img;
  GtkWidget *header;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  int i;
  
  obj->priv = g_new0 (XfburnCopyCdDialogPrivate, 1);
  priv = obj->priv;

  gtk_window_set_title (GTK_WINDOW (obj), _("Copy data CD"));

  img = gtk_image_new_from_stock ("xfburn-data-copy", GTK_ICON_SIZE_LARGE_TOOLBAR);
  header = xfce_create_header_with_image (img, _("Copy data CD"));
  gtk_widget_show (header);
  gtk_box_pack_start (box, header, FALSE, FALSE, 0);

  /* reader devices list */
  frame = xfce_framebox_new (_("CD Reader device"), TRUE);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

  priv->combo_source_device = gtk_combo_box_new_text ();
  gtk_widget_show (priv->combo_source_device);
  gtk_box_pack_start (GTK_BOX (vbox), priv->combo_source_device, FALSE, FALSE, BORDER);

  device = list_devices;
  while (device) {
    XfburnDevice *device_data = (XfburnDevice *) device->data;

    gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_source_device), device_data->name);

    device = g_list_next (device);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_source_device), 0);

  
  /* burning devices list */
  frame = xfce_framebox_new (_("Burning device"), TRUE);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

  priv->combo_dest_device = gtk_combo_box_new_text ();
  gtk_widget_show (priv->combo_dest_device);
  gtk_box_pack_start (GTK_BOX (vbox), priv->combo_dest_device, FALSE, FALSE, BORDER);

  device = list_devices;
  while (device) {
    XfburnDevice *device_data = (XfburnDevice *) device->data;

    gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_dest_device), device_data->name);

    device = g_list_next (device);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_dest_device), 0);

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

  for (i = 2; i <= 52; i += 2) {
    gchar *str;

    str = g_strdup_printf ("%d", i);
    gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_speed), str);
    g_free (str);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_speed), 19);

  img = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (img);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  /* mode */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, BORDER);

  label = gtk_label_new_with_mnemonic (_("Write _mode :"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, BORDER);

  priv->combo_mode = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_mode), _("default"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_mode), "tao");
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_mode), "dao");
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_mode), "raw96p");
  gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_mode), "raw16");
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_mode), 0);
  gtk_box_pack_start (GTK_BOX (hbox), priv->combo_mode, TRUE, TRUE, BORDER);
  gtk_widget_show (priv->combo_mode);
  
  /* options */
  frame = xfce_framebox_new (_("Options"), TRUE);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

  priv->check_eject = gtk_check_button_new_with_mnemonic (_("Eject disk"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_eject), TRUE);
  gtk_widget_show (priv->check_eject);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_eject, FALSE, FALSE, BORDER);

  priv->check_dummy = gtk_check_button_new_with_mnemonic (_("Dummy write"));
  gtk_widget_show (priv->check_dummy);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_dummy, FALSE, FALSE, BORDER);

  priv->check_burnfree = gtk_check_button_new_with_mnemonic (_("BurnFree"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_burnfree), TRUE);
  gtk_widget_show (priv->check_burnfree);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_burnfree, FALSE, FALSE, BORDER);
  
  priv->check_only_iso = gtk_check_button_new_with_mnemonic (_("Only create ISO"));
  gtk_widget_show (priv->check_only_iso);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_only_iso, FALSE, FALSE, BORDER);

  /* action buttons */
  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CANCEL);

  button = xfce_create_mixed_button ("xfburn-data-copy", _("_Copy CD"));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);

 // g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (xfburn_burn_image_dialog_response_cb), obj);
}

static void
xfburn_copy_cd_dialog_finalize (GObject * object)
{
  XfburnCopyCdDialog *cobj;
  cobj = XFBURN_COPY_CD_DIALOG (object);

  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
xfburn_copy_cd_dialog_new ()
{
  XfburnCopyCdDialog *obj;

  obj = XFBURN_COPY_CD_DIALOG (g_object_new (XFBURN_TYPE_COPY_CD_DIALOG, NULL));

  return GTK_WIDGET (obj);
}
