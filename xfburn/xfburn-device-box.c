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
  LAST_SIGNAL,
};

enum {
  PROP_0,
  PROP_SHOW_WRITERS_ONLY,
  PROP_SHOW_SPEED_SELECTION,
};

/* private struct */
typedef struct
{
  gboolean show_writers_only;
  gboolean show_speed_selection;
  
  GtkWidget *combo_device;
  
  GtkWidget *hbox_speed_selection;
  GtkWidget *combo_speed;
} XfburnDeviceBoxPrivate;

/* prototypes */
static void xfburn_device_box_class_init (XfburnDeviceBoxClass *);
static void xfburn_device_box_init (XfburnDeviceBox *);
static void xfburn_device_box_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfburn_device_box_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

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
    
  g_object_class_install_property (object_class, PROP_SHOW_WRITERS_ONLY, 
                                   g_param_spec_boolean ("show-writers-only", _("Show writers only"),
                                                        _("Show writers only"), FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SHOW_SPEED_SELECTION, 
                                   g_param_spec_boolean ("show-speed-selection", _("Show speed selection"),
                                                        _("Show speed selection list and refresh button"), 
                                                        FALSE, G_PARAM_READWRITE));
}

static void
xfburn_device_box_init (XfburnDeviceBox * box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  
  GtkWidget *label, *img, *button;
  GList *device = NULL;
  gint i;
  
  /* devices */
  priv->combo_device = gtk_combo_box_new_text ();
  gtk_widget_show (priv->combo_device);
  gtk_box_pack_start (GTK_BOX (box), priv->combo_device, FALSE, FALSE, BORDER);

  device = xfburn_device_list_get_list ();
  while (device) {
    XfburnDevice *device_data = (XfburnDevice *) device->data;

    gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_device), device_data->name);

    device = g_list_next (device);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_device), 0);
  
  g_signal_connect (G_OBJECT (priv->combo_device), "changed", G_CALLBACK (cb_combo_device_changed), box);
  
  /* speed */
  priv->hbox_speed_selection = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (priv->hbox_speed_selection);
  gtk_box_pack_start (GTK_BOX (box), priv->hbox_speed_selection, FALSE, FALSE, BORDER);

  label = gtk_label_new_with_mnemonic (_("_Speed :"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (priv->hbox_speed_selection), label, FALSE, FALSE, BORDER);

  priv->combo_speed = gtk_combo_box_new_text ();
  gtk_widget_show (priv->combo_speed);
  gtk_box_pack_start (GTK_BOX (priv->hbox_speed_selection), priv->combo_speed, TRUE, TRUE, BORDER);

  for (i = 2; i <= 52; i += 2) {
    gchar *str = NULL;

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
  gtk_box_pack_start (GTK_BOX (priv->hbox_speed_selection), button, FALSE, FALSE, 0);
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
cb_combo_device_changed (GtkComboBox *combo, XfburnDeviceBox *box)
{
  gchar *device_name = NULL;
  
  gtk_combo_box_get_active_text (combo);
  
  g_signal_emit (G_OBJECT (box), signals[DEVICE_CHANGED], 0, device_name);
  
  g_free (device_name);
}

/******************/
/* public methods */
/******************/
GtkWidget *
xfburn_device_box_new (gboolean show_writers_only, gboolean show_speed_selection)
{
  GtkWidget *obj;

  obj = g_object_new (xfburn_device_box_get_type (), "show-writers-only", show_writers_only, 
                      "show-speed-selection", show_speed_selection, NULL);

  return obj;
}

gchar *
xfburn_device_box_get_selected (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  
  return gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_device));
}

XfburnDevice *
xfburn_device_box_get_selected_device (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  gchar *device_name = NULL;
  XfburnDevice * device = NULL;
  
  device_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_device));
  device = xfburn_device_lookup_by_name (device_name);
  g_free (device_name);
  
  return device;
}

gchar *
xfburn_device_box_get_speed (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  
  g_return_val_if_fail (priv->show_speed_selection == TRUE, NULL);
  if (priv->show_speed_selection)
    return gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->combo_speed));
  else
    return NULL;
}
