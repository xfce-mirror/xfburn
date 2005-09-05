/* $Id$ */
/*
 * Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
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

#ifndef __XFBURN_GLOBAL_H__
#define __XFBURN_GLOBAL_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#define BORDER 5

#define CDRECORD_OPC "Performing OPC..."

#define CDRECORD_COPY "Track"
#define CDRECORD_AVERAGE_WRITE_SPEED "Average write speed"
#define CDRECORD_FIXATING "Fixating..."
#define CDRECORD_FIXATING_TIME "Fixating time:"

#define CDRECORD_BLANKING "Blanking "
#define CDRECORD_BLANKING_TIME "Blanking time:"

#define CDRECORD_CANNOT_BLANK "Cannot blank disk, aborting"
#define CDRECORD_INCOMPATIBLE_MEDIUM "cannot format medium - incmedium"
#define CDRECORD_NO_DISK_WRONG_DISK "No disk / Wrong disk!"

#define READCD_CAPACITY "end:"
#define READCD_PROGRESS "addr:"
#define READCD_DONE "Time total:"

#define CDRDAO_LENGTH "length"
#define CDRDAO_FLUSHING "Flushing cache..."
#define CDRDAO_INSERT "Please insert a recordable medium and hit enter"
#define CDRDAO_INSERT_AGAIN "Cannot determine disk status - hit enter to try again."
#define CDRDAO_DONE "CD copying finished successfully."

#define MKISOFS_ABORTING "Aborting."
#define MKISOFS_RUNNING " done, estimate"
#define MKISOFS_DONE "extents written"
typedef struct
{
  gchar *name;
  gchar *id;
  gchar *node_path;
  gboolean cdr;
  gboolean cdrw;
  gboolean dvdr;
  gboolean dvdram;
} XfburnDevice;

extern GList *list_devices;

#endif
