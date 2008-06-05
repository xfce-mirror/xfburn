/* $Id$ */
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

#include <libburn.h>

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
  XFBURN_PROFILE_BD_RE = 0x43,
};

typedef struct
{
  gchar *name;
  gchar *node_path;
  gint buffer_size;
  gboolean dummy_write;
  
  gboolean cdr;
  gboolean cdrw;
  GSList *supported_cdr_speeds;

  gint tao_block_types;
  gint sao_block_types;
  gint raw_block_types;
  gint packet_block_types;

  gboolean dvdr;
  gboolean dvdram;

  gchar addr[BURN_DRIVE_ADR_LEN];
} XfburnDevice;

gint xfburn_device_list_init ();
XfburnDevice * xfburn_device_lookup_by_name (const gchar * name);
GList * xfburn_device_list_get_list ();
enum burn_disc_status xfburn_device_list_get_disc_status ();
int xfburn_device_list_get_profile_no ();
const char * xfburn_device_list_get_profile_name ();
gboolean xfburn_device_list_disc_is_erasable ();
void xfburn_device_list_free ();

gboolean xfburn_device_refresh_supported_speeds (XfburnDevice * device);
gboolean xfburn_device_grab (XfburnDevice * device, struct burn_drive_info **drive_info);
void xfburn_device_free (XfburnDevice * device);

#endif /* __XFBURN_DEVICE_LIST_H__ */
