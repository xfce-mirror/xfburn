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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"
#include "xfburn-copy-cd-progress-dialog.h"
#include "xfburn-create-iso-progress-dialog.h"
#include "xfburn-device-box.h"

#include "xfburn-copy-cd-dialog.h"

#define XFBURN_COPY_CD_DIALOG_GET_PRIVATE(obj) (xfburn_copy_cd_dialog_get_instance_private (XFBURN_COPY_CD_DIALOG (obj)))

typedef struct
{
  GtkWidget *device_box_src;
  GtkWidget *frame_burn;
  GtkWidget *device_box_dest;

  GtkWidget *check_eject;
  GtkWidget *check_onthefly;
  GtkWidget *check_only_iso;
  GtkWidget *hbox_iso;
  GtkWidget *entry_path_iso;
  GtkWidget *check_dummy;
} XfburnCopyCdDialogPrivate;

/* prototypes */
static void xfburn_copy_cd_dialog_class_init (XfburnCopyCdDialogClass * klass);
static void xfburn_copy_cd_dialog_init (XfburnCopyCdDialog * sp);

static void cb_device_changed (XfburnDeviceBox *box, const gchar *device_name, XfburnCopyCdDialogPrivate *priv);
static void cb_check_only_iso_toggled (GtkToggleButton * button, XfburnCopyCdDialog * dialog);
static void cb_browse_iso (GtkButton * button, XfburnCopyCdDialog * dialog);
static void cb_dialog_response (XfburnCopyCdDialog * dialog, gint response_id, XfburnCopyCdDialogPrivate * priv);

/*********************/
/* class declaration */
/*********************/
static XfceTitledDialogClass *parent_class = NULL;

G_DEFINE_TYPE_WITH_PRIVATE(XfburnCopyCdDialog, xfburn_copy_cd_dialog, XFCE_TYPE_TITLED_DIALOG);

static void
xfburn_copy_cd_dialog_class_init (XfburnCopyCdDialogClass * klass)
{  
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_copy_cd_dialog_init (XfburnCopyCdDialog * obj)
{
  XfburnCopyCdDialogPrivate *priv = XFBURN_COPY_CD_DIALOG_GET_PRIVATE (obj);
  GtkBox *box = GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (obj)));
  GtkWidget *img;
  GdkPixbuf *icon = NULL;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *align;
  GtkWidget *button;
  gchar *default_path, *tmp_dir;
  gint x,y;

  gtk_window_set_title (GTK_WINDOW (obj), _("Copy data CD"));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &x, &y);
  icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default(), "stock_xfburn-data-copy", x, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
  gtk_window_set_icon (GTK_WINDOW (obj), icon);
  g_object_unref (icon);

  /* reader devices list */
  priv->device_box_src = xfburn_device_box_new (FALSE, FALSE);
  gtk_widget_show (priv->device_box_src);
  
  frame = xfce_create_framebox_with_content (_("CD Reader device"), priv->device_box_src);
  gtk_widget_show (frame);
  gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

  /* burning devices list */
  priv->device_box_dest = xfburn_device_box_new (TRUE, TRUE);
  gtk_widget_show (priv->device_box_dest);

  priv->frame_burn = xfce_create_framebox_with_content (_("Burning device"), priv->device_box_dest);
  gtk_widget_show (priv->frame_burn);
  gtk_box_pack_start (box, priv->frame_burn, FALSE, FALSE, BORDER);
    
  /* options */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
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

  priv->check_onthefly = gtk_check_button_new_with_mnemonic (_("On the _fly"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_onthefly), TRUE);
  gtk_widget_show (priv->check_onthefly);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_onthefly, FALSE, FALSE, BORDER);

  priv->check_only_iso = gtk_check_button_new_with_mnemonic (_("Only create _ISO"));
  gtk_widget_show (priv->check_only_iso);
  gtk_box_pack_start (GTK_BOX (vbox), priv->check_only_iso, FALSE, FALSE, BORDER);
  g_signal_connect (G_OBJECT (priv->check_only_iso), "toggled", G_CALLBACK (cb_check_only_iso_toggled), obj);

  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, BORDER * 4, 0);
  gtk_widget_show (align);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  priv->hbox_iso = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (priv->hbox_iso);
  gtk_container_add (GTK_CONTAINER (align), priv->hbox_iso);
  gtk_widget_set_sensitive (priv->hbox_iso, FALSE);

  priv->entry_path_iso = gtk_entry_new ();
  gtk_widget_show (priv->entry_path_iso);
  tmp_dir = xfburn_settings_get_string ("temporary-dir", "/tmp");
  default_path = g_build_filename (tmp_dir, "xfburn.iso", NULL);
  gtk_entry_set_text (GTK_ENTRY (priv->entry_path_iso), default_path);
  g_free (default_path);
  g_free (tmp_dir);
  gtk_box_pack_start (GTK_BOX (priv->hbox_iso), priv->entry_path_iso, FALSE, FALSE, 0);

  img = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (img);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (priv->hbox_iso), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_browse_iso), obj);

  /* action buttons */
  button = gtk_button_new_with_mnemonic (_("_Cancel"));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_CANCEL);

  button = xfce_create_mixed_button ("xfburn-data-copy", _("_Copy CD"));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button, GTK_RESPONSE_OK);
  gtk_widget_set_can_default (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);

  g_signal_connect (G_OBJECT (priv->device_box_src), "device-changed", G_CALLBACK (cb_device_changed), priv);
  g_signal_connect (G_OBJECT (priv->device_box_dest), "device-changed", G_CALLBACK (cb_device_changed), priv);
  
  /* check if the selected devices are the same */
  cb_device_changed (XFBURN_DEVICE_BOX (priv->device_box_dest), NULL, priv);
  
  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (cb_dialog_response), priv);
}

/* internals */
static void
cb_check_only_iso_toggled (GtkToggleButton * button, XfburnCopyCdDialog * dialog)
{
  XfburnCopyCdDialogPrivate *priv = XFBURN_COPY_CD_DIALOG_GET_PRIVATE (dialog);
  gboolean sensitive = gtk_toggle_button_get_active (button);
  
  gtk_widget_set_sensitive (priv->hbox_iso, sensitive);
  
  gtk_widget_set_sensitive (priv->frame_burn, !sensitive);
  
  gtk_widget_set_sensitive (priv->check_eject, !sensitive);
  gtk_widget_set_sensitive (priv->check_dummy, !sensitive);
  gtk_widget_set_sensitive (priv->check_onthefly, !sensitive);
}

static void
cb_device_changed (XfburnDeviceBox *box, const gchar *device_name, XfburnCopyCdDialogPrivate *priv)
{
  gchar *source_device_name = NULL, *dest_device_name = NULL;
  
  source_device_name = xfburn_device_box_get_selected (XFBURN_DEVICE_BOX (priv->device_box_src));
  dest_device_name = xfburn_device_box_get_selected (XFBURN_DEVICE_BOX (priv->device_box_dest));
  
  if (source_device_name && dest_device_name && !strcmp (source_device_name, dest_device_name)) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_onthefly), FALSE);
    gtk_widget_set_sensitive (priv->check_onthefly, FALSE);
  } else
    gtk_widget_set_sensitive (priv->check_onthefly, TRUE);
  
  g_free (source_device_name);
  g_free (dest_device_name);
}

static void
cb_browse_iso (GtkButton * button, XfburnCopyCdDialog * dialog)
{
  XfburnCopyCdDialogPrivate *priv = XFBURN_COPY_CD_DIALOG_GET_PRIVATE (dialog);
  
  xfburn_browse_for_file (GTK_ENTRY (priv->entry_path_iso), GTK_WINDOW (dialog));
}

static void
cb_dialog_response (XfburnCopyCdDialog * dialog, gint response_id, XfburnCopyCdDialogPrivate * priv)
{
  if (response_id == GTK_RESPONSE_OK) {
    gchar *command;
    XfburnDevice *device_burn;
    XfburnDevice *device_read;
    GtkWidget *dialog_progress = NULL;
  
    device_read = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box_src));
        
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_only_iso))) {
      command = g_strconcat ("readcd dev=", device_read->node_path, " f=", 
                             gtk_entry_get_text (GTK_ENTRY (priv->entry_path_iso)), NULL);
      
      dialog_progress = xfburn_create_iso_progress_dialog_new ();
    } else {
      gchar *speed;
      gchar *source_device = NULL;
      
      device_burn = xfburn_device_box_get_selected_device (XFBURN_DEVICE_BOX (priv->device_box_dest));
      speed = xfburn_device_box_get_speed (XFBURN_DEVICE_BOX (priv->device_box_dest));
      
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
      
      dialog_progress = xfburn_copy_cd_progress_dialog_new ();
    }
    
    gtk_window_set_transient_for (GTK_WINDOW (dialog_progress), gtk_window_get_transient_for (GTK_WINDOW (dialog)));
    gtk_widget_hide (GTK_WIDGET (dialog));
        
    g_object_set_data (G_OBJECT (dialog_progress), "command", command);
    gtk_dialog_run (GTK_DIALOG (dialog_progress));
   
    g_free (command);
  }
}

/* public */
GtkWidget *
xfburn_copy_cd_dialog_new (void)
{
  XfburnCopyCdDialog *obj;

  obj = XFBURN_COPY_CD_DIALOG (g_object_new (XFBURN_TYPE_COPY_CD_DIALOG, NULL));

  return GTK_WIDGET (obj);
}
