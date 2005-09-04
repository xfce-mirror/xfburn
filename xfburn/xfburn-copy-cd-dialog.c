/* $Id$ */
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-copy-cd-progress-dialog.h"

#include "xfburn-copy-cd-dialog.h"

/* prototypes */
static void xfburn_copy_cd_dialog_class_init (XfburnCopyCdDialogClass * klass);
static void xfburn_copy_cd_dialog_init (XfburnCopyCdDialog * sp);
static void xfburn_copy_cd_dialog_finalize (GObject * object);

static void cb_combo_device_changed (GtkComboBox *combo, XfburnCopyCdDialogPrivate *priv);
static void cb_check_only_iso_toggled (GtkToggleButton * button, XfburnCopyCdDialog * dialog);
static void cb_browse_iso (GtkButton * button, XfburnCopyCdDialog * dialog);
static void cb_dialog_response (XfburnCopyCdDialog * dialog, gint response_id, XfburnCopyCdDialogPrivate * priv);

/* structures */
struct XfburnCopyCdDialogPrivate
{
  GtkWidget *combo_source_device;
  GtkWidget *frame_burn;
  GtkWidget *combo_dest_device;
  GtkWidget *combo_speed;

  GtkWidget *check_eject;
  GtkWidget *check_onthefly;
  GtkWidget *check_only_iso;
  GtkWidget *hbox_iso;
  GtkWidget *entry_path_iso;
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
  gchar *default_path, *tmp_dir;
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
  g_signal_connect (G_OBJECT (priv->combo_source_device), "changed", G_CALLBACK (cb_combo_device_changed), priv);

  /* burning devices list */
  priv->frame_burn = xfce_framebox_new (_("Burning device"), TRUE);
  gtk_widget_show (priv->frame_burn);
  gtk_box_pack_start (box, priv->frame_burn, FALSE, FALSE, BORDER);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  xfce_framebox_add (XFCE_FRAMEBOX (priv->frame_burn), vbox);

  priv->combo_dest_device = gtk_combo_box_new_text ();
  gtk_widget_show (priv->combo_dest_device);
  gtk_box_pack_start (GTK_BOX (vbox), priv->combo_dest_device, FALSE, FALSE, BORDER);
  g_signal_connect (G_OBJECT (priv->combo_dest_device), "changed", G_CALLBACK (cb_combo_device_changed), priv);
  
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
  
  /* options */
  frame = xfce_framebox_new (_("Options"), TRUE);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

  priv->check_eject = gtk_check_button_new_with_mnemonic (_("E_ject disk"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_eject), TRUE);
  gtk_widget_show (priv->check_eject);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_eject, FALSE, FALSE, BORDER);

  priv->check_dummy = gtk_check_button_new_with_mnemonic (_("_Dummy write"));
  gtk_widget_show (priv->check_dummy);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_dummy, FALSE, FALSE, BORDER);

  priv->check_onthefly = gtk_check_button_new_with_mnemonic (_("On the _fly"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_onthefly), TRUE);
  gtk_widget_show (priv->check_onthefly);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_onthefly, FALSE, FALSE, BORDER);

  priv->check_only_iso = gtk_check_button_new_with_mnemonic (_("Only create _ISO"));
  gtk_widget_show (priv->check_only_iso);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_only_iso, FALSE, FALSE, BORDER);
  g_signal_connect (G_OBJECT (priv->check_only_iso), "toggled", G_CALLBACK (cb_check_only_iso_toggled), obj);

  priv->hbox_iso = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (priv->hbox_iso);
  gtk_box_pack_start (GTK_BOX (vbox), priv->hbox_iso, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (priv->hbox_iso, FALSE);

  priv->entry_path_iso = gtk_entry_new ();
  gtk_widget_show (priv->entry_path_iso);
  tmp_dir = xfburn_settings_get_string ("temporary-dir", "/tmp");
  default_path = g_build_filename (tmp_dir, "xfburn.iso", NULL);
  gtk_entry_set_text (GTK_ENTRY (priv->entry_path_iso), default_path);
  g_free (default_path);
  g_free (tmp_dir);
  gtk_box_pack_start (GTK_BOX (priv->hbox_iso), priv->entry_path_iso, FALSE, FALSE, 0);

  img = gtk_image_new_from_stock (GTK_STOCK_DIRECTORY, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (img);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (priv->hbox_iso), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_browse_iso), obj);

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

  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (cb_dialog_response), priv);
  
  /* load devices in combos */
  device = list_devices;
  while (device) {
    XfburnDevice *device_data = (XfburnDevice *) device->data;

    gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_source_device), device_data->name);
    gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_dest_device), device_data->name);
    device = g_list_next (device);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_source_device), 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_dest_device), 0);

}

static void
xfburn_copy_cd_dialog_finalize (GObject * object)
{
  XfburnCopyCdDialog *cobj;
  cobj = XFBURN_COPY_CD_DIALOG (object);
  
  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* internals */
static void
cb_check_only_iso_toggled (GtkToggleButton * button, XfburnCopyCdDialog * dialog)
{
  XfburnCopyCdDialogPrivate *priv = dialog->priv;
  gboolean sensitive = gtk_toggle_button_get_active (button);
  
  gtk_widget_set_sensitive (priv->hbox_iso, sensitive);
  
  gtk_widget_set_sensitive (priv->frame_burn, !sensitive);
  
  gtk_widget_set_sensitive (priv->check_eject, !sensitive);
  gtk_widget_set_sensitive (priv->check_dummy, !sensitive);
  gtk_widget_set_sensitive (priv->check_onthefly, !sensitive);
}

static void
cb_combo_device_changed (GtkComboBox *combo, XfburnCopyCdDialogPrivate *priv)
{
  gchar *source_device_name, *dest_device_name;
  
  source_device_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_source_device));
  dest_device_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_dest_device));
  
  if (source_device_name && dest_device_name && !strcmp (source_device_name, dest_device_name)) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_onthefly), FALSE);
    gtk_widget_set_sensitive (priv->check_onthefly, FALSE);
  } else
    gtk_widget_set_sensitive (priv->check_onthefly, TRUE);
}

static void
cb_browse_iso (GtkButton * button, XfburnCopyCdDialog * dialog)
{
  xfburn_browse_for_file (GTK_ENTRY (dialog->priv->entry_path_iso), GTK_WINDOW (dialog));
}

static void
cb_dialog_response (XfburnCopyCdDialog * dialog, gint response_id, XfburnCopyCdDialogPrivate * priv)
{
  if (response_id == GTK_RESPONSE_OK) {
    gchar *source_device_name;
    gchar *command;
    XfburnDevice *device_burn;
    XfburnDevice *device_read;
  
    source_device_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_source_device));
    device_read = xfburn_device_lookup_by_name (source_device_name);
        
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_only_iso))) {
      command = g_strconcat ("readcd dev=", device_read->node_path, " f=/tmp/xfburn.iso", NULL);
    } else {
      gchar *dest_device_name, *speed;
      gchar *source_device = NULL;
      
      dest_device_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_dest_device));
      device_burn = xfburn_device_lookup_by_name (dest_device_name);
      speed = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_speed));
      
      if (device_burn != device_read)
        source_device = g_strconcat (" --source-device ", device_read->node_path, NULL);
      else
        source_device = g_strdup ("");
      
      command = g_strconcat ("cdrdao copy -n -v 2", source_device, 
                             " --device ", device_burn->node_path, " --speed ", speed, 
                             gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_eject)) ? " --eject" : "",
                             gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_dummy)) ? " --simulate" : "",
                             device_burn != device_read && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_onthefly)) ? " --on-the-fly" : "",
                             " --datafile /tmp/xfburn.bin", NULL);
      
      g_free (source_device);
      g_free (speed);
      g_free (dest_device_name);
    }
    g_free (source_device_name);
    
    GtkWidget *dialog_progress;
        
    dialog_progress = xfburn_copy_cd_progress_dialog_new ();
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), gtk_window_get_transient_for (GTK_WINDOW (dialog)));
    gtk_widget_hide (GTK_WIDGET (dialog));
        
    g_object_set_data (G_OBJECT (dialog_progress), "command", command);
    gtk_dialog_run (GTK_DIALOG (dialog_progress));
   
    g_free (command);
  }
}

/* public */
GtkWidget *
xfburn_copy_cd_dialog_new ()
{
  XfburnCopyCdDialog *obj;

  obj = XFBURN_COPY_CD_DIALOG (g_object_new (XFBURN_TYPE_COPY_CD_DIALOG, NULL));

  return GTK_WIDGET (obj);
}
