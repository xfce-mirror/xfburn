/* $Id$ */
/*
 *  Copyright (c) 2006 Jean-François Wauthy (pollux@xfce.org)
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

#ifndef __XFBURN_WRITE_MODE_COMBO_BOX_H__
#define __XFBURN_WRITE_MODE_COMBO_BOX_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_WRITE_MODE_COMBO_BOX         (xfburn_write_mode_combo_box_get_type ())
#define XFBURN_WRITE_MODE_COMBO_BOX(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_WRITE_MODE_COMBO_BOX, XfburnWriteModeComboBox))
#define XFBURN_WRITE_MODE_COMBO_BOX_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_WRITE_MODE_COMBO_BOX, XfburnWriteModeComboBoxClass))
#define XFBURN_IS_WRITE_MODE_COMBO_BOX(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_WRITE_MODE_COMBO_BOX))
#define XFBURN_IS_WRITE_MODE_COMBO_BOX_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_WRITE_MODE_COMBO_BOX))
#define XFBURN_WRITE_MODE_COMBO_BOX_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_WRITE_MODE_COMBO_BOX, XfburnWriteModeComboBoxClass))

typedef struct
{
  GtkComboBox parent;
} XfburnWriteModeComboBox;

typedef struct
{
  GtkComboBoxClass parent_class;
} XfburnWriteModeComboBoxClass;

GType xfburn_write_mode_combo_box_get_type ();

GtkWidget *xfburn_write_mode_combo_box_new ();

gchar * xfburn_write_mode_combo_box_get_cdrecord_param (XfburnWriteModeComboBox *combo);

G_END_DECLS
#endif
