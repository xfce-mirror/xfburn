/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#ifndef __XFBURN_DEVICE_LIST_H__
#define __XFBURN_DEVICE_LIST_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <libburn.h>

#include "xfburn-device.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_DEVICE_LIST xfburn_device_list_get_type()

#define XFBURN_DEVICE_LIST(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_DEVICE_LIST, XfburnDeviceList))

#define XFBURN_DEVICE_LIST_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_DEVICE_LIST, XfburnDeviceListClass))

#define XFBURN_IS_DEVICE_LIST(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_DEVICE_LIST))

#define XFBURN_IS_DEVICE_LIST_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_DEVICE_LIST))

#define XFBURN_DEVICE_LIST_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_DEVICE_LIST, XfburnDeviceListClass))

typedef struct {
  GObject parent;
} XfburnDeviceList;

typedef struct {
  GObjectClass parent_class;
  
  void (*volume_changed) (XfburnDeviceList *devlist, guint device_changed, XfburnDevice *device);
} XfburnDeviceListClass;


/* what kind of recordable discs are there */
/* usused so far */
enum XfburnDiscTypes {
  /* record-once types */
  XFBURN_CDR,
  XFBURN_DVD_R, /* we don't need a distinction for record once between + / - */
  XFBURN_REWRITABLE, /* marker, everything after this is rewritable */
  XFBURN_CDRW,
  XFBURN_DVD_RAM,
  XFBURN_DVD_MINUS_RW,
  XFBURN_DVD_PLUS_RW,
};

enum XfburnDiscProfiles {
  XFBURN_PROFILE_NONE = 0x0,
  XFBURN_PROFILE_CDR = 0x09,
  XFBURN_PROFILE_CDRW = 0x0a,
  XFBURN_PROFILE_DVD_MINUS_R = 0x11,
  XFBURN_PROFILE_DVDRAM = 0x12,
  XFBURN_PROFILE_DVD_MINUS_RW_OVERWRITE = 0x13,
  XFBURN_PROFILE_DVD_MINUS_RW_SEQUENTIAL = 0x14,
  XFBURN_PROFILE_DVD_MINUS_R_DL = 0x15,
  XFBURN_PROFILE_DVD_PLUS_RW = 0x1a,
  XFBURN_PROFILE_DVD_PLUS_R = 0x1b,
  XFBURN_PROFILE_DVD_PLUS_R_DL = 0x2b,
  XFBURN_PROFILE_BD_R = 0x41,
  XFBURN_PROFILE_BD_RE = 0x43,
  /* Read-only profiles */
  XFBURN_PROFILE_CDROM = 0x08,
  XFBURN_PROFILE_DVDROM = 0x10,
  XFBURN_PROFILE_BDROM = 0x40,
};

GType xfburn_device_list_get_type (void);

XfburnDeviceList* xfburn_device_list_new (void);

XfburnDevice * xfburn_device_list_lookup_by_name (XfburnDeviceList *devlist, const gchar * name);
gchar * xfburn_device_list_get_selected (XfburnDeviceList *devlist);
GtkWidget * xfburn_device_list_get_refresh_button (XfburnDeviceList *devlist);
GtkWidget * xfburn_device_list_get_device_combo (XfburnDeviceList *devlist);
XfburnDevice * xfburn_device_list_get_current_device (XfburnDeviceList *devlist);

#endif /* __XFBURN_DEVICE_LIST_H__ */
