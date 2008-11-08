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

#if 0

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-global.h"
#include "xfburn-utils.h"

#include "xfburn-device-box.h"
#include "xfburn-stock.h"

#include "xfburn-format-dvd-dialog.h"

#define XFBURN_FORMAT_DVD_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_FORMAT_DVD_DIALOG, XfburnFormatDvdDialogPrivate))

typedef struct
{
  GtkWidget *device_box;
  GtkWidget *check_eject;
} XfburnFormatDvdDialogPrivate;

static void xfburn_format_dvd_dialog_class_init (XfburnFormatDvdDialogClass * klass);
static void xfburn_format_dvd_dialog_init (XfburnFormatDvdDialog * sp);

static void xfburn_format_dvd_dialog_response_cb (XfburnFormatDvdDialog * dialog, gint response_id, gpointer user_data);

static XfceTitledDialogClass *parent_class = NULL;

GtkType
xfburn_format_dvd_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnFormatDvdDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_format_dvd_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnFormatDvdDialog),
      0,
      (GInstanceInitFunc) xfburn_format_dvd_dialog_init,
      NULL
    };

    type = g_type_register_static (XFCE_TYPE_TITLED_DIALOG, "XfburnFormatDvdDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_format_dvd_dialog_class_init (XfburnFormatDvdDialogClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);
  
  g_type_class_add_private (klass, sizeof (XfburnFormatDvdDialogPrivate));
}

static void
xfburn_format_dvd_dialog_init (XfburnFormatDvdDialog * obj)
{
  XfburnFormatDvdDialogPrivate *priv = XFBURN_FORMAT_DVD_DIALOG_GET_PRIVATE (obj);
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  GdkPixbuf *icon = NULL;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button;
  
  gtk_window_set_title (GTK_WINDOW (obj), _("Format DVD+RW"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  
  icon = gtk_widget_render_icon (GTK_WIDGET (obj), XFBURN_STOCK_FORMAT_DVDRW, GTK_ICON_SIZE_DIALOG, NULL);
  gtk_window_set_icon (GTK_WINDOW (obj), icon);
  g_object_unref (icon);

  /* devices list */
  priv->device_box = xfburn_device_box_new (SHOW_DVD_WRITERS | SHOW_SPEED_SELECTION);
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

  /* action buttons */
  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CANCEL);

  button = xfce_create_mixed_button (XFBURN_STOCK_FORMAT_DVDRW/*"xfburn-blank-cdrw"*/, _("_Format"));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);

  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (xfburn_format_dvd_dialog_response_cb), obj);
}

static void
xfburn_format_dvd_dialog_response_cb (XfburnFormatDvdDialog * dialog, gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    XfburnFormatDvdDialogPrivate *priv = XFBURN_FORMAT_DVD_DIALOG_GET_PRIVATE (dialog);
    gchar *command;
    XfburnDevice *device;
    gint speed;
    //GtkWidget *dialog_progress;
    
    device = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box));
    speed = xfburn_device_box_get_speed (XFBURN_DEVICE_BOX (priv->device_box));

    command = g_strconcat ("cdrecord -v gracetime=2 -format", " dev=", device->node_path, " speed=", speed,
                           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject)) ? " -eject" : "", NULL);
    
    
    
        
    /*
    dialog_progress = xfburn_format_dvd_progress_dialog_new ();
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), gtk_window_get_transient_for (GTK_WINDOW (dialog)));
    gtk_widget_hide (GTK_WIDGET (dialog));
    
    g_object_set_data (G_OBJECT (dialog_progress), "command", command);
    gtk_dialog_run (GTK_DIALOG (dialog_progress));
    */
    g_free (command);

    xfce_warn ("Sorry i don't have DVD+RW discs in order to implement and test that function currently.");
  }
}

/* public */
GtkWidget *
xfburn_format_dvd_dialog_new ()
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_FORMAT_DVD_DIALOG, NULL));

  return obj;
}
#endif
