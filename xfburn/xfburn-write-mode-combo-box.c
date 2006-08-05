/* $Id$ */
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

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>

#include "xfburn-write-mode-combo-box.h"

/* global */
typedef struct _Setting Setting;
  
static void xfburn_write_mode_combo_box_class_init (XfburnWriteModeComboBoxClass * klass);
static void xfburn_write_mode_combo_box_init (XfburnWriteModeComboBox * combo);

/* private */
static struct
{
  gchar *write_mode, *param;
} write_modes[] = { 
  {N_("Default"), "-tao"},
  {"TAO", "-tao"},
  {"SAO", "-sao"},
  {"DAO", "-dao"},
  {"RAW96P", "-raw96p"},
  {"RAW16", "-raw16"},
};

/**********************/
/* object declaration */
/**********************/
static GtkComboBoxClass *parent_class = NULL;

GType
xfburn_write_mode_combo_box_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnWriteModeComboBoxClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_write_mode_combo_box_class_init,
      NULL,
      NULL,
      sizeof (XfburnWriteModeComboBox),
      0,
      (GInstanceInitFunc) xfburn_write_mode_combo_box_init,
    };

    type = g_type_register_static (GTK_TYPE_COMBO_BOX, "XfburnWriteModeComboBox", &our_info, 0);
  }

  return type;
}

static void
xfburn_write_mode_combo_box_class_init (XfburnWriteModeComboBoxClass * klass)
{ 
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_write_mode_combo_box_init (XfburnWriteModeComboBox *combo)
{
  GtkCellRenderer *cell;
  GtkListStore *store;

  guint n;

  store = gtk_list_store_new (1, G_TYPE_STRING); 
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell, "text", 0, NULL);
 
  for (n = 0; n < G_N_ELEMENTS (write_modes); ++n) {
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _(write_modes[n].write_mode));
  }
  
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
}

/******************/
/* public methods */
/******************/
GtkWidget *
xfburn_write_mode_combo_box_new ()
{
  return GTK_WIDGET (g_object_new (XFBURN_TYPE_WRITE_MODE_COMBO_BOX, NULL));
}

gchar *
xfburn_write_mode_combo_box_get_cdrecord_param (XfburnWriteModeComboBox *combo)
{
  gchar *param = NULL;
  
  param = write_modes[gtk_combo_box_get_active (GTK_COMBO_BOX (combo))].param;
  
  return g_strdup (param);
}
