/* $Id$ */
/*
 * Copyright (c) 2006 Jean-François Wauthy (pollux@xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "xfburn-device-list.h"
#include "xfburn-device-box.h"

#define XFBURN_DEVICE_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_DEVICE_BOX, XfburnDeviceBoxPrivate))

enum {
  DEVICE_CHANGED,
  DISC_REFRESHED,
  LAST_SIGNAL,
};

enum {
  PROP_0,
  PROP_SHOW_WRITERS_ONLY,
  PROP_SHOW_SPEED_SELECTION,
  PROP_SHOW_MODE_SELECTION,
  PROP_DISC_STATUS,
  PROP_VALID,
  PROP_BLANK_MODE,
};

enum {
  DEVICE_NAME_COLUMN,
  DEVICE_POINTER_COLUMN,
  DEVICE_N_COLUMNS,
};

enum {
  SPEED_TEXT_COLUMN,
  SPEED_VALUE_COLUMN,
  SPEED_N_COLUMNS,
};

enum {
  MODE_TEXT_COLUMN,
  MODE_VALUE_COLUMN,
  MODE_N_COLUMNS,
};

/* private struct */
typedef struct
{
  gboolean show_writers_only;
  gboolean show_speed_selection;
  gboolean show_mode_selection;
  gboolean valid_disc;
  gboolean blank_mode;
  
  GtkWidget *combo_device;
  
  GtkWidget *hbox_speed_selection;
  GtkWidget *combo_speed;

  GtkWidget *status_label;

  GtkWidget *hbox_mode_selection;
  GtkWidget *combo_mode;
} XfburnDeviceBoxPrivate;

/* prototypes */
static void xfburn_device_box_class_init (XfburnDeviceBoxClass *);
static void xfburn_device_box_init (XfburnDeviceBox *);
static void xfburn_device_box_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfburn_device_box_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void update_status_label_visibility();
static void cb_speed_refresh_clicked (GtkButton *button, XfburnDeviceBox *box);
static gboolean check_disc_validity (XfburnDeviceBoxPrivate *priv);
static void cb_combo_device_changed (GtkComboBox *combo, XfburnDeviceBox *box);

/* globals */
static GtkVBoxClass *parent_class = NULL;

/*************************/
/* XfburnDeviceBox class */
/*************************/
static guint signals[LAST_SIGNAL];

GtkType
xfburn_device_box_get_type (void)
{
  static GtkType device_box_type = 0;

  if (!device_box_type)
    {
      static const GTypeInfo device_box_info = {
        sizeof (XfburnDeviceBoxClass),
        NULL,
        NULL,
        (GClassInitFunc) xfburn_device_box_class_init,
        NULL,
        NULL,
        sizeof (XfburnDeviceBox),
        0,
        (GInstanceInitFunc) xfburn_device_box_init
      };

      device_box_type = g_type_register_static (GTK_TYPE_VBOX, "XfburnDeviceBox", &device_box_info, 0);
    }

  return device_box_type;
}

static void
xfburn_device_box_class_init (XfburnDeviceBoxClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  g_type_class_add_private (klass, sizeof (XfburnDeviceBoxPrivate));

  parent_class = g_type_class_peek_parent (klass);
  
  object_class->set_property = xfburn_device_box_set_property;
  object_class->get_property = xfburn_device_box_get_property;
  
  signals[DEVICE_CHANGED] = g_signal_new ("device-changed", XFBURN_TYPE_DEVICE_BOX, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnDeviceBoxClass, device_changed),
                                          NULL, NULL, g_cclosure_marshal_VOID__STRING,
                                          G_TYPE_NONE, 1, G_TYPE_STRING);
  signals[DISC_REFRESHED] = g_signal_new ("disc-refreshed", XFBURN_TYPE_DEVICE_BOX, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnDeviceBoxClass, disc_refreshed),
                                          NULL, NULL, g_cclosure_marshal_VOID__STRING,
                                          G_TYPE_NONE, 1, G_TYPE_STRING);
    
    
  g_object_class_install_property (object_class, PROP_SHOW_WRITERS_ONLY, 
                                   g_param_spec_boolean ("show-writers-only", _("Show writers only"),
                                                        _("Show writers only"), FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SHOW_SPEED_SELECTION, 
                                   g_param_spec_boolean ("show-speed-selection", _("Show speed selection"),
                                                        _("Show speed selection combo"), 
                                                        FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SHOW_MODE_SELECTION, 
                                   g_param_spec_boolean ("show-mode-selection", _("Show mode selection"),
                                                        _("Show mode selection combo"), 
                                                        FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DISC_STATUS,
                                   g_param_spec_int ("disc-status", _("Disc status"),
                                                      _("The status of the disc in the drive"), 
                                                      0, BURN_DISC_UNSUITABLE, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_VALID, 
                                   g_param_spec_boolean ("valid", _("Is it a valid combination"),
                                                        _("Is the combination of hardware and disc valid to burn the composition?"), 
                                                        FALSE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_BLANK_MODE, 
                                   g_param_spec_boolean ("blank-mode", _("Blank mode"),
                                                        _("The blank mode shows different disc status messages than regular mode"), 
                                                        FALSE, G_PARAM_READWRITE));
}

static void
xfburn_device_box_init (XfburnDeviceBox * box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);

  GtkWidget *label, *img, *button;
  GList *device = NULL;
  GtkListStore *store = NULL;
  GtkCellRenderer *cell;
  
  /* devices */
  store = gtk_list_store_new (DEVICE_N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
  priv->combo_device = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo_device), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo_device), cell, "text", DEVICE_NAME_COLUMN, NULL);
  gtk_widget_show (priv->combo_device);
  gtk_box_pack_start (GTK_BOX (box), priv->combo_device, FALSE, FALSE, BORDER);

  device = xfburn_device_list_get_list ();

  while (device) {
    XfburnDevice *device_data = (XfburnDevice *) device->data;
    GtkTreeIter iter;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, DEVICE_NAME_COLUMN, device_data->name, DEVICE_POINTER_COLUMN, device_data, -1);

    device = g_list_next (device);
  }
  
  /* speed */
  priv->hbox_speed_selection = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (priv->hbox_speed_selection);
  gtk_box_pack_start (GTK_BOX (box), priv->hbox_speed_selection, FALSE, FALSE, BORDER);

  label = gtk_label_new_with_mnemonic (_("_Speed:"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (priv->hbox_speed_selection), label, FALSE, FALSE, BORDER);

  store = gtk_list_store_new (SPEED_N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
  priv->combo_speed = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo_speed), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo_speed), cell, "text", SPEED_TEXT_COLUMN, NULL);
  gtk_widget_show (priv->combo_speed);
  gtk_box_pack_start (GTK_BOX (priv->hbox_speed_selection), priv->combo_speed, TRUE, TRUE, BORDER);

  img = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (img);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (priv->hbox_speed_selection), button, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_speed_refresh_clicked), box);

  /* mode */
  priv->hbox_mode_selection = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (priv->hbox_mode_selection);
  gtk_box_pack_start (GTK_BOX (box), priv->hbox_mode_selection, FALSE, FALSE, BORDER);

  label = gtk_label_new_with_mnemonic (_("Write _mode:"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (priv->hbox_mode_selection), label, FALSE, FALSE, BORDER);

  store = gtk_list_store_new (MODE_N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
  priv->combo_mode = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo_mode), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo_mode), cell, "text", MODE_TEXT_COLUMN, NULL);
  gtk_widget_show (priv->combo_mode);
  gtk_box_pack_start (GTK_BOX (priv->hbox_mode_selection), priv->combo_mode, TRUE, TRUE, BORDER);

  /* status label */
  priv->status_label = gtk_label_new ("");
  gtk_widget_show (priv->status_label);
  gtk_box_pack_start (GTK_BOX (box), priv->status_label, FALSE, FALSE, 0);


  g_signal_connect (G_OBJECT (priv->combo_device), "changed", G_CALLBACK (cb_combo_device_changed), box);
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_device), 0);
}

static void
xfburn_device_box_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (object);

  switch (prop_id) {
    case PROP_SHOW_WRITERS_ONLY:
      g_value_set_boolean (value, priv->show_writers_only);
      break;
    case PROP_SHOW_SPEED_SELECTION:
      g_value_set_boolean (value, priv->show_speed_selection);
      break;
    case PROP_SHOW_MODE_SELECTION:
      g_value_set_boolean (value, priv->show_mode_selection);
      break;
    case PROP_DISC_STATUS:
      g_value_set_int (value, xfburn_device_list_get_disc_status());
      break;
    case PROP_VALID:
      g_value_set_boolean (value, priv->valid_disc);
      break;
    case PROP_BLANK_MODE:
      g_value_set_boolean (value, priv->blank_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
xfburn_device_box_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (object);
  
  switch (prop_id) {
    case PROP_SHOW_WRITERS_ONLY:
      priv->show_writers_only = g_value_get_boolean (value);
      break;
    case PROP_SHOW_SPEED_SELECTION:
      priv->show_speed_selection = g_value_get_boolean (value);
      if (priv->show_speed_selection)
        gtk_widget_show (priv->hbox_speed_selection);
      else
        gtk_widget_hide (priv->hbox_speed_selection);
      update_status_label_visibility (priv);
      break;
    case PROP_SHOW_MODE_SELECTION:
      priv->show_mode_selection = g_value_get_boolean (value);
      if (priv->show_mode_selection)
        gtk_widget_show (priv->hbox_mode_selection);
      else
        gtk_widget_hide (priv->hbox_mode_selection);
      update_status_label_visibility (priv);
      break;
    case PROP_BLANK_MODE:
      priv->blank_mode = g_value_get_boolean (value);
      if (priv->blank_mode) {
        priv->show_speed_selection = FALSE;
        priv->show_mode_selection = FALSE;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/*************/
/* internals */
/*************/

static void
update_status_label_visibility (XfburnDeviceBoxPrivate *priv)
{
  /*
  if (priv->show_mode_selection || priv->show_speed_selection)
    gtk_widget_show (priv->status_label);
  else
    gtk_widget_hide (priv->status_label);
  */

}

static void
fill_combo_speed (XfburnDeviceBox *box, XfburnDevice *device)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_speed));
  GSList *el = device->supported_cdr_speeds;

  gtk_list_store_clear (GTK_LIST_STORE (model));

  if (!check_disc_validity (priv))
    return;

  while (el) {
    gint speed = GPOINTER_TO_INT (el->data);
    GtkTreeIter iter;
    gchar *str = NULL;

    str = g_strdup_printf ("%d", speed);

    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, SPEED_TEXT_COLUMN, str, SPEED_VALUE_COLUMN, speed, -1);
    g_free (str);

    el = g_slist_next (el);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_speed), gtk_tree_model_iter_n_children (model, NULL) - 1);
}

static gboolean
check_disc_validity (XfburnDeviceBoxPrivate *priv)
{
  enum burn_disc_status disc_status = xfburn_device_list_get_disc_status ();

  if (!priv->blank_mode) {
    priv->valid_disc = (disc_status == BURN_DISC_BLANK) || (disc_status == BURN_DISC_APPENDABLE);

    if (!priv->valid_disc) {
      switch (disc_status) {
        case BURN_DISC_EMPTY:
          gtk_label_set_markup (GTK_LABEL(priv->status_label), _("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">Drive is empty!</span>"));
          break;
        case BURN_DISC_FULL:
          gtk_label_set_markup (GTK_LABEL(priv->status_label), _("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">Inserted disc is full!</span>"));
          break;
        case BURN_DISC_UNSUITABLE:
          gtk_label_set_markup (GTK_LABEL(priv->status_label), _("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">Inserted disc is unsuitable!</span>"));
          break;
        default:
          gtk_label_set_markup (GTK_LABEL(priv->status_label), _("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">Error determining disc!</span>"));
          DBG ("weird disc_status = %d", disc_status);
      }
    }
  } else {
    priv->valid_disc = (disc_status == BURN_DISC_FULL) || (disc_status == BURN_DISC_APPENDABLE);

    if (!priv->valid_disc) {
      switch (disc_status) {
        case BURN_DISC_EMPTY:
          gtk_label_set_markup (GTK_LABEL(priv->status_label), _("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">Drive is empty!</span>"));
          break;
        case BURN_DISC_BLANK:
          gtk_label_set_markup (GTK_LABEL(priv->status_label), _("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">Inserted disc is already blank!</span>"));
          break;
        case BURN_DISC_UNSUITABLE:
          gtk_label_set_markup (GTK_LABEL(priv->status_label), _("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">Inserted disc is unsuitable!</span>"));
          break;
        default:
          gtk_label_set_markup (GTK_LABEL(priv->status_label), _("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">Error determining disc!</span>"));
          DBG ("weird disc_status = %d", disc_status);
      }
    }
  }

  gtk_widget_set_sensitive (priv->combo_speed, priv->valid_disc);
  if (priv->valid_disc)
    gtk_label_set_text (GTK_LABEL(priv->status_label), "");
  return priv->valid_disc;
}

static void
fill_combo_mode (XfburnDeviceBox *box, XfburnDevice *device)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_mode));
  GtkTreeIter iter;

  gtk_list_store_clear (GTK_LIST_STORE (model));

  if (device->tao_block_types) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, MODE_TEXT_COLUMN, "TAO", MODE_VALUE_COLUMN, WRITE_MODE_TAO, -1);
  }
  if (device->sao_block_types & BURN_BLOCK_SAO) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, MODE_TEXT_COLUMN, "SAO", MODE_VALUE_COLUMN, WRITE_MODE_SAO, -1);
  }
  /*
   * RAW modes are not supported by libburn yet
   *
  if (device->raw_block_types & BURN_BLOCK_RAW16) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, MODE_TEXT_COLUMN, "RAW16", MODE_VALUE_COLUMN, WRITE_MODE_RAW16, -1);
  }
  if (device->raw_block_types & BURN_BLOCK_RAW96P) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, MODE_TEXT_COLUMN, "RAW96P", MODE_VALUE_COLUMN, WRITE_MODE_RAW96P, -1);
  }
  if (device->raw_block_types & BURN_BLOCK_RAW96R) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, MODE_TEXT_COLUMN, "RAW96R", MODE_VALUE_COLUMN, WRITE_MODE_RAW96R, -1);
  }
  if (device->packet_block_types) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, MODE_TEXT_COLUMN, "packet", MODE_VALUE_COLUMN, WRITE_MODE_PACKET, -1);
  }
  */
  
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_mode), 0);
}

static void
cb_speed_refresh_clicked (GtkButton *button, XfburnDeviceBox *box)
{
  XfburnDevice *device = NULL;
  
  device = xfburn_device_box_get_selected_device (box);
  xfburn_device_refresh_supported_speeds (device);

  fill_combo_speed (box, device);

  g_signal_emit (G_OBJECT (box), signals[DISC_REFRESHED], 0, device);
}

static void
cb_combo_device_changed (GtkComboBox *combo, XfburnDeviceBox *box)
{
  XfburnDevice *device;
  
  device = xfburn_device_box_get_selected_device (box);
  xfburn_device_refresh_supported_speeds (device);

  fill_combo_speed (box, device);
  fill_combo_mode (box,device);

  g_signal_emit (G_OBJECT (box), signals[DEVICE_CHANGED], 0, device);
}

/******************/
/* public methods */
/******************/
GtkWidget *
xfburn_device_box_new (XfburnDeviceBoxFlags flags)
{
  GtkWidget *obj;

  obj = g_object_new (xfburn_device_box_get_type (), 
		      "show-writers-only", ((flags & SHOW_CD_WRITERS) != 0), 
                      "show-speed-selection", ((flags & SHOW_SPEED_SELECTION) != 0), 
		      "show-mode-selection", ((flags & SHOW_MODE_SELECTION) != 0), NULL);

  return obj;
}

gchar *
xfburn_device_box_get_selected (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name = NULL;
  gboolean ret;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_device));
  ret = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo_device), &iter);
  if (ret)
    gtk_tree_model_get (model, &iter, DEVICE_NAME_COLUMN, &name, -1);

  return name;
}

XfburnDevice *
xfburn_device_box_get_selected_device (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  GtkTreeModel *model;
  GtkTreeIter iter;
  XfburnDevice * device = NULL;
  gboolean ret;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_device));
  ret = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo_device), &iter);
  if (ret)
    gtk_tree_model_get (model, &iter, DEVICE_POINTER_COLUMN, &device, -1);

  return device;
}

gint
xfburn_device_box_get_speed (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint speed = -1;
  gboolean ret;

  g_return_val_if_fail (priv->show_speed_selection, -1);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_speed));
  ret = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo_speed), &iter);
  if (ret)
    gtk_tree_model_get (model, &iter, SPEED_VALUE_COLUMN, &speed, -1);

  return speed;
}

XfburnWriteMode
xfburn_device_box_get_mode (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint mode = -1;
  gboolean ret;

  g_return_val_if_fail (priv->show_mode_selection, -1);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_mode));
  ret = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo_mode), &iter);
  if (ret)
    gtk_tree_model_get (model, &iter, SPEED_VALUE_COLUMN, &mode, -1);

  return mode;
}
