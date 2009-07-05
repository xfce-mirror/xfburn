/*
 *  Copyright (C) 2009 David Mohr <david@mcbf.net>
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
 *  
 */

#ifndef __XFBURN_DEVICE__
#define __XFBURN_DEVICE__

#include <glib-object.h>
#include <libburn.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_DEVICE xfburn_device_get_type()

#define XFBURN_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_DEVICE, XfburnDevice))

#define XFBURN_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_DEVICE, XfburnDeviceClass))

#define XFBURN_IS_DEVICE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_DEVICE))

#define XFBURN_IS_DEVICE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_DEVICE))

#define XFBURN_DEVICE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_DEVICE, XfburnDeviceClass))

typedef struct {
  GObject parent;
} XfburnDevice;

typedef struct {
  GObjectClass parent_class;
} XfburnDeviceClass;

GType xfburn_device_get_type (void);

XfburnDevice* xfburn_device_new (void);

void xfburn_device_fillin_libburn_info (XfburnDevice *device, struct burn_drive_info *drive);
gboolean xfburn_device_refresh_info (XfburnDevice * device, gboolean get_speed_info);
gboolean xfburn_device_grab (XfburnDevice * device, struct burn_drive_info **drive_info);
gboolean xfburn_device_release (struct burn_drive_info *drive_info, gboolean eject);
const gchar * xfburn_device_set_name (XfburnDevice *device, const gchar *vendor, const gchar *product);
gboolean xfburn_device_can_burn (XfburnDevice *device);
gboolean xfburn_device_can_dummy_write (XfburnDevice *device);

G_END_DECLS

#endif /* __XFBURN_DEVICE__ */

