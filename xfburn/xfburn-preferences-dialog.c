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

#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-preferences-dialog.h"
#include "xfburn-global.h"
#include "xfburn-utils.h"

static void xfburn_preferences_dialog_class_init (XfburnPreferencesDialogClass * klass);
static void xfburn_preferences_dialog_init (XfburnPreferencesDialog * sp);
static void xfburn_preferences_dialog_finalize (GObject * object);

static void refresh_devices_list (XfburnPreferencesDialog * dialog);
static void cb_scan_button_clicked (GtkWidget * button, gpointer user_data);

struct XfburnPreferencesDialogPrivate
{
  GtkWidget *notebook;

  GtkWidget *chooser_button;
  GtkWidget *check_clean_tmpdir;
  GtkWidget *check_show_hidden;
  GtkWidget *check_show_human_readable;

  GtkWidget *treeview_devices;
  GtkWidget *button_scan;
};

typedef struct
{
  XfburnPreferencesDialog *object;
} XfburnPreferencesDialogSignal;

static GtkDialogClass *parent_class = NULL;

GtkType
xfburn_preferences_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnPreferencesDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_preferences_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnPreferencesDialog),
      0,
      (GInstanceInitFunc) xfburn_preferences_dialog_init,
    };

    type = g_type_register_static (GTK_TYPE_DIALOG, "XfburnPreferencesDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_preferences_dialog_class_init (XfburnPreferencesDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_preferences_dialog_finalize;

}

static void
xfburn_preferences_dialog_init (XfburnPreferencesDialog * obj)
{
  GtkBox *box = GTK_BOX (GTK_DIALOG (obj)->vbox);
  XfburnPreferencesDialogPrivate *priv;
  GtkWidget *vbox, *vbox2, *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *scrolled_window;
  GtkListStore *model;
  GtkTreeViewColumn *column_name;
  GtkCellRenderer *cell_icon, *cell_name;
  GtkWidget *button_close;

  obj->priv = g_new0 (XfburnPreferencesDialogPrivate, 1);
  priv = obj->priv;

  gtk_widget_set_size_request (GTK_WIDGET (obj), 450, 300);
  gtk_window_set_title (GTK_WINDOW (obj), _("Xfburn preferences"));
  
  priv->notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (priv->notebook), BORDER);
  gtk_box_pack_start (box, priv->notebook, TRUE, TRUE, BORDER);
  gtk_widget_show (priv->notebook);

  /* general tab */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
  gtk_container_add (GTK_CONTAINER (priv->notebook), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("General"));
  gtk_widget_show (label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (priv->notebook),
                              gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), 0), label);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  frame = xfce_framebox_new (_("Temporary directory"), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, BORDER);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 0);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox2);
  gtk_widget_show (vbox2);

  priv->chooser_button = gtk_file_chooser_button_new (_("Temporary directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_box_pack_start (GTK_BOX (vbox2), priv->chooser_button, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->chooser_button);

  priv->check_clean_tmpdir = gtk_check_button_new_with_mnemonic (_("_Clean temporary directory on exit"));
  gtk_box_pack_start (GTK_BOX (vbox2), priv->check_clean_tmpdir, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->check_clean_tmpdir);

  frame = xfce_framebox_new (_("File browser"), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, BORDER);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 0);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox2);
  gtk_widget_show (vbox2);

  priv->check_show_hidden = gtk_check_button_new_with_mnemonic (_("Show _hidden files"));
  gtk_box_pack_start (GTK_BOX (vbox2), priv->check_show_hidden, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->check_show_hidden);

  priv->check_show_human_readable = gtk_check_button_new_with_mnemonic (_("Show human_readable filesizes"));
  gtk_box_pack_start (GTK_BOX (vbox2), priv->check_show_human_readable, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->check_show_human_readable);

  /* devices tab */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
  gtk_container_add (GTK_CONTAINER (priv->notebook), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Devices"));
  gtk_widget_show (label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (priv->notebook),
                              gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), 1), label);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  frame = xfce_framebox_new (_("Detected devices"), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, BORDER);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 0);
  xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox2);
  gtk_widget_show (vbox2);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_box_pack_start (GTK_BOX (vbox2), scrolled_window, TRUE, TRUE, BORDER);

  model = gtk_list_store_new (DEVICE_LIST_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
                              G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
  priv->treeview_devices = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), DEVICE_LIST_COLUMN_NAME, GTK_SORT_ASCENDING);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->treeview_devices), TRUE);
  gtk_widget_show (priv->treeview_devices);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->treeview_devices);

  /* add columns */
  column_name = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column_name, _("Name"));
  gtk_tree_view_column_set_expand (column_name, TRUE);

  cell_icon = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column_name, cell_icon, FALSE);
  gtk_tree_view_column_set_attributes (column_name, cell_icon, "pixbuf", DEVICE_LIST_COLUMN_ICON, NULL);
  g_object_set (cell_icon, "xalign", 0.0, "ypad", 0, NULL);

  cell_name = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column_name, cell_name, TRUE);
  gtk_tree_view_column_set_attributes (column_name, cell_name, "text", DEVICE_LIST_COLUMN_NAME, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->treeview_devices), column_name);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview_devices), -1, _("Id"),
                                               gtk_cell_renderer_text_new (), "text", DEVICE_LIST_COLUMN_ID, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview_devices), -1, _("Node"),
                                               gtk_cell_renderer_text_new (), "text", DEVICE_LIST_COLUMN_NODE, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview_devices), -1, _("Write CD-R"),
                                               gtk_cell_renderer_toggle_new (), "active", DEVICE_LIST_COLUMN_CDR, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview_devices), -1, _("Write CD-RW"),
                                               gtk_cell_renderer_toggle_new (), "active", DEVICE_LIST_COLUMN_CDRW,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview_devices), -1, _("Write DVD-R"),
                                               gtk_cell_renderer_toggle_new (), "active", DEVICE_LIST_COLUMN_DVDR,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview_devices), -1, _("Write DVD-RAM"),
                                               gtk_cell_renderer_toggle_new (), "active", DEVICE_LIST_COLUMN_DVDRAM,
                                               NULL);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, BORDER);
  gtk_widget_show (hbox);

  priv->button_scan = xfce_create_mixed_button (GTK_STOCK_CDROM, _("Sc_an for devices"));
  gtk_box_pack_end (GTK_BOX (hbox), priv->button_scan, FALSE, FALSE, BORDER);
  g_signal_connect (G_OBJECT (priv->button_scan), "clicked", G_CALLBACK (cb_scan_button_clicked), obj);
  gtk_widget_show (priv->button_scan);

  /* action buttons */
  button_close = gtk_button_new_from_stock ("gtk-close");
  gtk_widget_show (button_close);
  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button_close, GTK_RESPONSE_CLOSE);
  GTK_WIDGET_SET_FLAGS (button_close, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (button_close);
  gtk_widget_grab_default (button_close);

  refresh_devices_list (obj);
}

/* internals */
static void
refresh_devices_list (XfburnPreferencesDialog * dialog)
{
  GtkTreeModel *model;
  XfburnPreferencesDialogPrivate *priv;
  GList *device;

  priv = dialog->priv;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->treeview_devices));

  gtk_list_store_clear (GTK_LIST_STORE (model));

  device = list_devices;
  while (device) {
    GtkTreeIter iter;
    XfburnDevice *device_data;

    device_data = (XfburnDevice *) device->data;

    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        DEVICE_LIST_COLUMN_NAME, device_data->name,
                        DEVICE_LIST_COLUMN_ID, device_data->id,
                        DEVICE_LIST_COLUMN_NODE, device_data->node_path,
                        DEVICE_LIST_COLUMN_CDR, device_data->cdr,
                        DEVICE_LIST_COLUMN_CDRW, device_data->cdrw,
                        DEVICE_LIST_COLUMN_DVDR, device_data->dvdr, DEVICE_LIST_COLUMN_DVDRAM, device_data->dvdram, -1);

    device = g_list_next (device);
  }
}

static void
cb_scan_button_clicked (GtkWidget * button, gpointer user_data)
{
  xfburn_scan_devices ();
  refresh_devices_list (user_data);
}

static void
xfburn_preferences_dialog_finalize (GObject * object)
{
  XfburnPreferencesDialog *cobj;
  cobj = XFBURN_PREFERENCES_DIALOG (object);

  /* Free private members, etc. */

  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* public */
GtkWidget *
xfburn_preferences_dialog_new ()
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_PREFERENCES_DIALOG, NULL));

  return obj;
}
