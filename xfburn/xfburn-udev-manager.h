/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *  Copyright (c) 2008      David Mohr (david@mcbf.net)
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

#ifndef __XFBURN_UDEV_MANAGER_H__
#define __XFBURN_UDEV_MANAGER_H__

#ifdef HAVE_GUDEV

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

#include "xfburn-progress-dialog.h"
#include "xfburn-device-list.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_UDEV_MANAGER         (xfburn_udev_manager_get_type ())
#define XFBURN_UDEV_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_UDEV_MANAGER, XfburnUdevManager))
#define XFBURN_UDEV_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_UDEV_MANAGER, XfburnUdevManagerClass))
#define XFBURN_IS_UDEV_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_UDEV_MANAGER))
#define XFBURN_IS_UDEV_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_UDEV_MANAGER))
#define XFBURN_UDEV_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_UDEV_MANAGER, XfburnUdevManagerClass))

typedef struct
{
  GObject parent;
} XfburnUdevManager;

typedef struct
{
  XfburnProgressDialogClass parent_class;
  
  void (*volume_changed) (XfburnUdevManager *udevman);
} XfburnUdevManagerClass;

GType xfburn_udev_manager_get_type (void);
//GObject *xfburn_udev_manager_new (); /* use _create_global / _get_instance instead */
gchar *xfburn_udev_manager_create_global (void);
XfburnUdevManager * xfburn_udev_manager_get_global (void);
void xfburn_udev_manager_shutdown (void);
void xfburn_udev_manager_send_volume_changed (void);
GList *xfburn_udev_manager_get_devices (XfburnUdevManager *udevman, gint *drives, gint *burners);
gboolean xfburn_udev_manager_check_ask_umount (XfburnUdevManager *udevman, XfburnDevice *device);

G_END_DECLS

#endif /* HAVE_GUDEV */

#endif /* XFBURN_UDEV_MANAGER_H */
