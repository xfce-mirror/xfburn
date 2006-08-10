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
#include "xfburn-device-box.h"
#include "xfburn-stock.h"

#include "xfburn-blank-cd-dialog.h"


static void xfburn_blank_cd_dialog_class_init (XfburnBlankCdDialogClass * klass);
static void xfburn_blank_cd_dialog_init (XfburnBlankCdDialog * sp);
static void xfburn_blank_cd_dialog_finalize (GObject * object);

static void xfburn_blank_cd_dialog_response_cb (XfburnBlankCdDialog * dialog, gint response_id, gpointer user_data);

struct XfburnBlankCdDialogPrivate
{
  GtkWidget *device_box;
  GtkWidget *combo_type;
  
  GtkWidget *check_force;
  GtkWidget *check_eject;
};

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
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_blank_cd_dialog_finalize;

}

static void
xfburn_blank_cd_dialog_init (XfburnBlankCdDialog * obj)
{
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  XfburnBlankCdDialogPrivate *priv;
  GdkPixbuf *icon = NULL;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button;

  obj->priv = g_new0 (XfburnBlankCdDialogPrivate, 1);

  priv = obj->priv;
  
  gtk_window_set_title (GTK_WINDOW (obj), _("Blank CD-RW"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  
  icon = gtk_widget_render_icon (GTK_WIDGET (obj), XFBURN_STOCK_BLANK_CDRW, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_window_set_icon (GTK_WINDOW (obj), icon);
  g_object_unref (icon);

  /* devices list */
  priv->device_box = xfburn_device_box_new (TRUE, TRUE);
  gtk_widget_show (priv->device_box);

  frame = xfce_create_framebox_with_content (_("Burning device"), priv->device_box);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

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
    gchar *blank_type, *speed;
    
    priv = dialog->priv;

    device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));
    speed = xfburn_device_box_get_speed (XFBURN_DEVICE_BOX (priv->device_box));

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
