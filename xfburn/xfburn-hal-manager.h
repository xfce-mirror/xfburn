/* $Id: xfburn-hal-manager.h 4352 2006-08-11 16:02:53Z pollux $ */
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

#ifndef __XFBURN_HAL_MANAGER_H__
#define __XFBURN_HAL_MANAGER_H__

#ifdef HAVE_HAL

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

#include "xfburn-progress-dialog.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_HAL_MANAGER         (xfburn_hal_manager_get_type ())
#define XFBURN_HAL_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_HAL_MANAGER, XfburnHalManager))
#define XFBURN_HAL_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_HAL_MANAGER, XfburnHalManagerClass))
#define XFBURN_IS_HAL_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_HAL_MANAGER))
#define XFBURN_IS_HAL_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_HAL_MANAGER))
#define XFBURN_HAL_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_HAL_MANAGER, XfburnHalManagerClass))

typedef struct
{
  GObject parent;
} XfburnHalManager;

typedef struct
{
  XfburnProgressDialogClass parent_class;
  
  void (*volume_changed) (XfburnHalManager *halman);
} XfburnHalManagerClass;

GtkType xfburn_hal_manager_get_type ();
GObject *xfburn_hal_manager_new ();
void xfburn_hal_manager_create_global ();
XfburnHalManager * xfburn_hal_manager_get_instance ();
void xfburn_hal_manager_shutdown ();

G_END_DECLS

#endif /* HAVE_HAL */

#endif /* XFBURN_HAL_MANAGER_H */
