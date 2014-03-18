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

#ifndef __XFBURN_DEVICE_BOX_H__
#define __XFBURN_DEVICE_BOX_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "xfburn-global.h"
#include "xfburn-device-list.h"

G_BEGIN_DECLS
#define XFBURN_TYPE_DEVICE_BOX            (xfburn_device_box_get_type ())
#define XFBURN_DEVICE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_DEVICE_BOX, XfburnDeviceBox))
#define XFBURN_DEVICE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_DEVICE_BOX, XfburnDeviceBoxClass))
#define XFBURN_IS_DEVICE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_DEVICE_BOX))
#define XFBURN_IS_DEVICE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_DEVICE_BOX))
#define XFBURN_DEVICE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_DEVICE_BOX, XfburnDeviceBoxClass))

typedef struct
{
  GtkVBox window;
} XfburnDeviceBox;

typedef struct
{
  GtkVBoxClass parent_class;

  void (*volume_changed) (XfburnDeviceBox *box, gboolean device_changed, XfburnDevice *device);
} XfburnDeviceBoxClass;

typedef enum
{
  SHOW_CD_WRITERS = 1 << 0,
  SHOW_CDRW_WRITERS = 1 << 1,
  SHOW_DVD_WRITERS = 1 << 2,

  SHOW_CD_READERS = 1 << 4,
  SHOW_DVD_READERS = 1 << 5,

  SHOW_SPEED_SELECTION = 1 << 7,
  SHOW_MODE_SELECTION = 1 << 8,
  BLANK_MODE = 1 << 9,
} XfburnDeviceBoxFlags;

typedef enum
{
  WRITE_MODE_AUTO,
  WRITE_MODE_TAO,
  WRITE_MODE_SAO,
  WRITE_MODE_RAW16,
  WRITE_MODE_RAW96P,
  WRITE_MODE_RAW96R,
  WRITE_MODE_PACKET,
} XfburnWriteMode;

GType xfburn_device_box_get_type (void);

GtkWidget *xfburn_device_box_new (XfburnDeviceBoxFlags);

XfburnDevice *xfburn_device_box_get_selected_device (XfburnDeviceBox *box);

gint xfburn_device_box_get_speed (XfburnDeviceBox *box);
XfburnWriteMode xfburn_device_box_get_mode (XfburnDeviceBox *box);
void xfburn_device_box_set_sensitive (XfburnDeviceBox *box, gboolean sensitivity);

G_END_DECLS
#endif
