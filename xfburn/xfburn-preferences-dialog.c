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

#include <libxfce4ui/libxfce4ui.h>
#include <exo/exo.h>

#include "xfburn-preferences-dialog.h"
#include "xfburn-device-list.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"

#define FIFO_MAX_SIZE     16384.0  /* in kb, as float */

#define XFBURN_PREFERENCES_DIALOG_GET_PRIVATE(obj) (xfburn_preferences_dialog_get_instance_private (XFBURN_PREFERENCES_DIALOG (obj)))

typedef struct
{
  GtkWidget *notebook;
  GtkWidget *icon_bar;
  
  GtkWidget *chooser_button;
  GtkWidget *check_clean_tmpdir;
  GtkWidget *check_show_hidden;
  GtkWidget *check_show_human_readable;

  GtkWidget *treeview_devices;
  GtkWidget *button_scan;
#if 0
  GtkWidget *check_empty_speed_list;
#endif
  GtkWidget *scale_fifo;
} XfburnPreferencesDialogPrivate;

/* prototypes */
static void refresh_devices_list (XfburnPreferencesDialog * dialog);
static void scan_button_clicked_cb (GtkWidget * button, gpointer user_data);
static void xfburn_preferences_dialog_response_cb (XfburnPreferencesDialog * dialog, guint response_id, XfburnPreferencesDialogPrivate * priv);
static void cb_show_hidden_clicked (GtkButton * Button, gpointer user_data);

enum {
  SETTINGS_LIST_PIXBUF_COLUMN,
  SETTINGS_LIST_TEXT_COLUMN,
  SETTINGS_LIST_INDEX_COLUMN,
  SETTINGS_LIST_N_COLUMNS,
};

enum
{
  DEVICE_LIST_COLUMN_ICON,
  DEVICE_LIST_COLUMN_NAME,
  DEVICE_LIST_COLUMN_REV,
  DEVICE_LIST_COLUMN_NODE,
  DEVICE_LIST_COLUMN_CDR,
  DEVICE_LIST_COLUMN_CDRW,
  DEVICE_LIST_COLUMN_DVDR,
  DEVICE_LIST_COLUMN_DVDRAM,
  DEVICE_LIST_COLUMN_BD,
  DEVICE_LIST_N_COLUMNS
};

typedef struct
{
  XfburnPreferencesDialog *object;
} XfburnPreferencesDialogSignal;

/*********************************/
/* XfburnPreferencesDialog class */
/*********************************/
static XfceTitledDialogClass *parent_class = NULL;

G_DEFINE_TYPE_WITH_PRIVATE(XfburnPreferencesDialog, xfburn_preferences_dialog, XFCE_TYPE_TITLED_DIALOG);

static void
xfburn_preferences_dialog_class_init (XfburnPreferencesDialogClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_preferences_dialog_init (XfburnPreferencesDialog * obj)
{
  GtkBox *box = GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (obj)));
  XfburnPreferencesDialogPrivate *priv = XFBURN_PREFERENCES_DIALOG_GET_PRIVATE (obj);
  
  GtkWidget *vbox, *vbox2, *vbox3, *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *scrolled_window;
  GtkListStore *icon_store, *store;
  gint x,y;
  GdkPixbuf *icon = NULL;
  GtkTreeIter iter;
  GtkTreeViewColumn *column_name;
  GtkCellRenderer *cell_icon, *cell_name;
  GtkWidget *button_close;
  gint idx;
  
  gtk_window_set_title (GTK_WINDOW (obj), _("Preferences"));
  xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (obj), _("Tune how Xfburn behaves"));
  gtk_window_set_default_size (GTK_WINDOW (obj), 775, 400);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (obj), TRUE);
  gtk_window_set_icon_name (GTK_WINDOW (obj), "preferences-system");
  
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (box, hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);
/*  // because exo icon bar isn't available disabling the side panel scrolled window
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  g_object_set (G_OBJECT (scrolled_window),
                "hscrollbar-policy", GTK_POLICY_NEVER,
                "shadow-type", GTK_SHADOW_IN,
                "vscrollbar-policy", GTK_POLICY_NEVER,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_window, FALSE, FALSE, 0);
  gtk_widget_show (scrolled_window);
*/
  /* icon bar */
  icon_store = gtk_list_store_new (SETTINGS_LIST_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT);
  // priv->icon_bar = exo_icon_view_new_with_model (GTK_TREE_MODEL (icon_store));
  // g_object_set_property (G_OBJECT (priv->icon_bar), "pixbuf-column", &pb_col);
  // g_object_set_property (G_OBJECT (priv->icon_bar), "text-column", &tx_col);
  // gtk_container_add (GTK_CONTAINER (scrolled_window), priv->icon_bar);
  // gtk_widget_show (priv->icon_bar);

  /* notebook */
  priv->notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (priv->notebook), BORDER);
  g_object_set (G_OBJECT (priv->notebook),
                "show-border", FALSE,
                "show-tabs", TRUE, // enabling the notebook tabs
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), priv->notebook, TRUE, TRUE, BORDER);
  gtk_widget_show (priv->notebook);

  /* general tab */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
  idx = gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), vbox, NULL);
  gtk_widget_show (vbox);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox2);

  frame = xfce_gtk_frame_box_new_with_content (_("Temporary directory"), vbox2);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, BORDER);
  gtk_widget_show (frame);

  priv->chooser_button = gtk_file_chooser_button_new (_("Temporary directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_box_pack_start (GTK_BOX (vbox2), priv->chooser_button, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->chooser_button);

  priv->check_clean_tmpdir = gtk_check_button_new_with_mnemonic (_("_Clean temporary directory on exit"));
  gtk_box_pack_start (GTK_BOX (vbox2), priv->check_clean_tmpdir, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->check_clean_tmpdir);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox2);

  frame = xfce_gtk_frame_box_new_with_content (_("File browser"), vbox2);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, BORDER);
  gtk_widget_show (frame);

  priv->check_show_hidden = gtk_check_button_new_with_mnemonic (_("Show _hidden files"));
  gtk_box_pack_start (GTK_BOX (vbox2), priv->check_show_hidden, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->check_show_hidden);
  g_signal_connect (G_OBJECT (priv->check_show_hidden), "clicked", G_CALLBACK (cb_show_hidden_clicked), NULL);

  priv->check_show_human_readable = gtk_check_button_new_with_mnemonic (_("Show human_readable filesizes"));
  gtk_box_pack_start (GTK_BOX (vbox2), priv->check_show_human_readable, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->check_show_human_readable);

  gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &x, &y);
  icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default(), "document-properties", x, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);

  gtk_list_store_append (icon_store, &iter);
  gtk_list_store_set (icon_store, &iter,
                      SETTINGS_LIST_PIXBUF_COLUMN, icon,
                      SETTINGS_LIST_TEXT_COLUMN, _("General"),
                      SETTINGS_LIST_INDEX_COLUMN, idx,
                      -1);
  g_object_unref (G_OBJECT (icon));
  
  /* devices tab */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
  idx = gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), vbox, NULL);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("General"));
  gtk_widget_show (label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (priv->notebook),
                              gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), 0), label);
  label = gtk_label_new (_("Devices"));
  gtk_widget_show (label);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (priv->notebook),
                              gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), 1), label);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox2);

  frame = xfce_gtk_frame_box_new_with_content (_("Detected devices"), vbox2);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, BORDER);
  gtk_widget_show (frame);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
  gtk_widget_show (scrolled_window);
  gtk_box_pack_start (GTK_BOX (vbox2), scrolled_window, TRUE, TRUE, BORDER);

  store = gtk_list_store_new (DEVICE_LIST_N_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                              G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN);
  priv->treeview_devices = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), DEVICE_LIST_COLUMN_NAME, GTK_SORT_ASCENDING);
  // gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->treeview_devices), TRUE);
  gtk_widget_show (priv->treeview_devices);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->treeview_devices);
  g_object_unref (store);
  
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
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview_devices), -1, _("Revision"),
                                               gtk_cell_renderer_text_new (), "text", DEVICE_LIST_COLUMN_REV, NULL);
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
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview_devices), -1, _("Write Blu-ray"),
                                               gtk_cell_renderer_toggle_new (), "active", DEVICE_LIST_COLUMN_BD,
                                               NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, BORDER);
  gtk_widget_show (hbox);

  priv->button_scan = xfce_gtk_button_new_mixed ("media-optical", _("Sc_an for devices"));
  gtk_box_pack_end (GTK_BOX (hbox), priv->button_scan, FALSE, FALSE, BORDER);
  g_signal_connect (G_OBJECT (priv->button_scan), "clicked", G_CALLBACK (scan_button_clicked_cb), obj);
  gtk_widget_show (priv->button_scan);

  gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &x, &y);
  icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default(), "media-optical", x, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
  if (!icon)
    icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default(), "media-cdrom", x, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
  if (!icon)
    icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default(), "media-optical", x, GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);

  
  gtk_list_store_append (icon_store, &iter);
  gtk_list_store_set (icon_store, &iter,
                      SETTINGS_LIST_PIXBUF_COLUMN, icon,
                      SETTINGS_LIST_TEXT_COLUMN, _("Devices"),
                      SETTINGS_LIST_INDEX_COLUMN, idx,
                      -1);
  if (icon)
    g_object_unref (G_OBJECT (icon));
  
//  exo_mutual_binding_new (G_OBJECT (priv->notebook), "page", G_OBJECT (priv->icon_bar), "active");


  /* below the device list */
#if 0
  priv->check_empty_speed_list = gtk_check_button_new_with_mnemonic (_("Show warning on _empty speed list"));
  gtk_box_pack_start (GTK_BOX (vbox2), priv->check_empty_speed_list, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->check_empty_speed_list);
#endif

  /* fifo */
  vbox3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox3);

  frame = xfce_gtk_frame_box_new_with_content (_("FIFO buffer size (in kb)"), vbox3);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, BORDER);
  gtk_widget_show (frame);

  priv->scale_fifo = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, FIFO_MAX_SIZE, 32.0);
  gtk_scale_set_value_pos (GTK_SCALE (priv->scale_fifo), GTK_POS_LEFT);
  gtk_range_set_value (GTK_RANGE (priv->scale_fifo), 0);
  gtk_box_pack_start (GTK_BOX (vbox3), priv->scale_fifo, FALSE, FALSE, BORDER/2);
  gtk_widget_show (priv->scale_fifo);

  
  /* action buttons */
  button_close = gtk_button_new ();
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,0);

  label = gtk_image_new_from_icon_name ("gtk-close", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("_Close"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  gtk_container_add (GTK_CONTAINER(button_close), hbox);
  gtk_widget_show (button_close);

  gtk_dialog_add_action_widget (GTK_DIALOG (obj), button_close, GTK_RESPONSE_CLOSE);
  gtk_widget_set_can_default (button_close, TRUE);
  gtk_widget_grab_focus (button_close);
  gtk_widget_grab_default (button_close);

  g_signal_connect (G_OBJECT (obj), "response", G_CALLBACK (xfburn_preferences_dialog_response_cb), priv);
  
  refresh_devices_list (obj);
  
  g_object_unref (icon_store);
}

/* internals */
static void
xfburn_preferences_dialog_load_settings (XfburnPreferencesDialog * dialog) 
{
  XfburnPreferencesDialogPrivate *priv = XFBURN_PREFERENCES_DIALOG_GET_PRIVATE (dialog);
  
  gchar *temp_dir;
  
  temp_dir = xfburn_settings_get_string ("temporary-dir", "/tmp");
  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (priv->chooser_button), temp_dir);
  g_free (temp_dir);
  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_clean_tmpdir),
                                xfburn_settings_get_boolean ("clean-temporary-dir", TRUE));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_show_hidden),
                                xfburn_settings_get_boolean ("show-hidden-files", FALSE));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_show_human_readable),
                                xfburn_settings_get_boolean ("human-readable-units", TRUE));
#if 0
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->check_empty_speed_list),
                                xfburn_settings_get_boolean ("show-empty-speed-list-notice", TRUE));
#endif
  gtk_range_set_value (GTK_RANGE (priv->scale_fifo),
                                (double) xfburn_settings_get_int ("fifo-size", FIFO_DEFAULT_SIZE));
}

static void
xfburn_preferences_dialog_save_settings (XfburnPreferencesDialog *dialog)
{
  XfburnPreferencesDialogPrivate *priv = XFBURN_PREFERENCES_DIALOG_GET_PRIVATE (dialog);
  gchar *temp_dir;
  
  temp_dir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->chooser_button));
  xfburn_settings_set_string ("temporary-dir", temp_dir);
  g_free (temp_dir);
  
  xfburn_settings_set_boolean ("clean-temporary-dir", 
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_clean_tmpdir)));
  xfburn_settings_set_boolean ("show-hidden-files", 
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_show_hidden)));
  xfburn_settings_set_boolean ("human-readable-units", 
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_show_human_readable)));
#if 0
  xfburn_settings_set_boolean ("show-empty-speed-list-notice", 
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->check_empty_speed_list)));
#endif
  xfburn_settings_set_int ("fifo-size", 
                               ((((int) gtk_range_get_value (GTK_RANGE (priv->scale_fifo)) / 32) * 32))); /* this should round to multiples of 1024 */
}

static void
refresh_devices_list (XfburnPreferencesDialog * dialog)
{
  XfburnPreferencesDialogPrivate *priv = XFBURN_PREFERENCES_DIALOG_GET_PRIVATE (dialog);
  GtkTreeModel *model;
  GList *device;
  XfburnDeviceList *devlist;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->treeview_devices));

  gtk_list_store_clear (GTK_LIST_STORE (model));

  devlist = xfburn_device_list_new ();
  g_object_get (G_OBJECT (devlist), "devices", &device, NULL);
  g_object_unref (devlist);

  while (device) {
    GtkTreeIter iter;
    XfburnDevice *device_data;
    gchar *name, *addr, *rev;
    gboolean cdr, cdrw, dvdr, dvdram, bd;

    device_data = (XfburnDevice *) device->data;

    g_object_get (G_OBJECT (device_data), "name", &name, "address", &addr,
                  "revision", &rev, "cdr", &cdr, "cdrw", &cdrw, "dvdr", &dvdr,
                  "dvdram", &dvdram, "bd", &bd, NULL);

    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        DEVICE_LIST_COLUMN_NAME, name,
                        DEVICE_LIST_COLUMN_REV, rev,
                        DEVICE_LIST_COLUMN_NODE, addr,
                        DEVICE_LIST_COLUMN_CDR, cdr,
                        DEVICE_LIST_COLUMN_CDRW, cdrw,
                        DEVICE_LIST_COLUMN_DVDR, dvdr,
                        DEVICE_LIST_COLUMN_DVDRAM, dvdram,
                        DEVICE_LIST_COLUMN_BD, bd,
                        -1);

    g_free (name);
    g_free (addr);
    g_free (rev);

    device = g_list_next (device);
  }
}

static void
xfburn_preferences_dialog_response_cb (XfburnPreferencesDialog * dialog, guint response_id, XfburnPreferencesDialogPrivate * priv)
{
  if (response_id == GTK_RESPONSE_CLOSE) {
    xfburn_preferences_dialog_save_settings (dialog);
  }
}

static void
scan_button_clicked_cb (GtkWidget * button, gpointer user_data)
{
  refresh_devices_list (user_data);
}

static void 
cb_show_hidden_clicked (GtkButton * Button, gpointer user_data)
{
  xfce_dialog_show_warning(NULL, NULL, _("Changing this setting only takes full effect after a program restart."));
}

/* public */
GtkWidget *
xfburn_preferences_dialog_new (void)
{
  GtkWidget *obj;

  obj = GTK_WIDGET (g_object_new (XFBURN_TYPE_PREFERENCES_DIALOG, NULL));

  xfburn_preferences_dialog_load_settings (XFBURN_PREFERENCES_DIALOG (obj));
  return obj;
}
