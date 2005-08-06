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

#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-preferences-dialog.h"

#define BORDER 5

static void xfburn_preferences_dialog_class_init (XfburnPreferencesDialogClass * klass);
static void xfburn_preferences_dialog_init (XfburnPreferencesDialog * sp);
static void xfburn_preferences_dialog_finalize (GObject * object);

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

  obj->priv = g_new0 (XfburnPreferencesDialogPrivate, 1);
  priv = obj->priv;

  gtk_widget_set_size_request (GTK_WIDGET (obj), 450, 300);

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
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, BORDER);
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
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->treeview_devices), TRUE);
  gtk_widget_show (priv->treeview_devices);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->treeview_devices);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, BORDER);
  gtk_widget_show (hbox);

  priv->button_scan = xfce_create_mixed_button (GTK_STOCK_CDROM, _("Sc_an for devices"));
  gtk_box_pack_end (GTK_BOX (hbox), priv->button_scan, FALSE, FALSE, BORDER);
  gtk_widget_show (priv->button_scan);
}

/* internals */
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
