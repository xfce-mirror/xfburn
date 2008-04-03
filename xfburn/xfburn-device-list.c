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

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <glib.h>
#include <libxfce4util/libxfce4util.h>

#include <stdlib.h>
#include <unistd.h>

#include <libburn.h>

#include "xfburn-device-list.h"

#define CDR_1X_SPEED 150

/* private */
static gint supported_cdr_speeds[] = {2, 4, 6, 8, 10, 12, 16, 20, 24, 32, 40, 48, 52, -1};
static GList *devices = NULL;

/*************/
/* internals */
/*************/
static void
device_content_free (XfburnDevice * device, gpointer user_data)
{
  g_free (device->name);
  g_free (device->node_path);

  g_slist_free (device->supported_cdr_speeds);
}

static gint
get_closest_supported_cdr_speed (gint speed)
{
  /* TODO: need some fixing */
  gint i = 0;
  gint previous = 0;

  while (supported_cdr_speeds[i] != -1) {
    if (speed < (supported_cdr_speeds[i] * CDR_1X_SPEED)) {
      return supported_cdr_speeds[previous];
    } else
      previous = i;
    i++;
  }

  return 0;
}

static gboolean
no_speed_duplicate (GSList *speed_list, gint speed)
{
  GSList *el = speed_list;

  while (el) {
    gint el_speed = GPOINTER_TO_INT (el->data);

    if (el_speed == speed)
      return FALSE;

    el = g_slist_next (el);
  }

  return TRUE;
}

static void
refresh_supported_speeds (XfburnDevice * device, struct burn_drive_info *drive_info)
{
  struct burn_speed_descriptor *speed_list = NULL;
  gint ret;

  /* empty previous list */
  g_slist_free (device->supported_cdr_speeds);
  device->supported_cdr_speeds = NULL;

  /* fill new list */
  ret = burn_drive_get_speedlist (drive_info->drive, &speed_list);

  if (ret > 0) {
    struct burn_speed_descriptor *el = speed_list;

    while (el) {
      gint speed = -1;
      
      speed = get_closest_supported_cdr_speed (el->write_speed);
      if (speed > 0 && no_speed_duplicate (device->supported_cdr_speeds, speed)) {
	device->supported_cdr_speeds = g_slist_prepend (device->supported_cdr_speeds, GINT_TO_POINTER (speed));
	DBG ("added speed: %d\n", speed);
      }

      el = el->next;
    }

    burn_drive_free_speedlist (&speed_list);  
  } else if (ret == 0) {
    g_warning ("reported speed list is empty");
  } else {
    g_error ("severe error while retrieving speed list");
  }
}

/**************/
/* public API */
/**************/
GList *
xfburn_device_list_get_list ()
{
  return devices;
}

void
xfburn_device_refresh_supported_speeds (XfburnDevice * device)
{
  struct burn_drive_info *drive_info = NULL;

  if (!burn_initialize ()) {
    g_critical ("Unable to initialize libburn");
    return;
  }

  if (!xfburn_device_grab (device, &drive_info)) {
    g_error ("Couldn't grab drive in order to update speed list.");
    return;
  }

  refresh_supported_speeds (device, drive_info);

  burn_drive_release (drive_info->drive, 0);

  burn_finish ();
}

gint
xfburn_device_list_init ()
{
  struct burn_drive_info *drives;
  gint i;
  guint n_drives = 0;

  if (!burn_initialize ()) {
    g_critical ("Unable to initialize libburn");
    return;
  }
    
  if (devices) {
    g_list_foreach (devices, (GFunc) device_content_free, NULL);
    g_list_free (devices);
    devices = NULL;
  }

  while (!burn_drive_scan (&drives, &n_drives))
    usleep (1002);

  for (i = 0; i < n_drives; i++) {
    XfburnDevice *device = g_new0 (XfburnDevice, 1);
    gint ret = 0;
    
    device->name = g_strconcat (drives[i].vendor, " ", drives[i].product, NULL);
    device->node_path = g_strdup (drives[i].location);

    device->cdr = drives[i].write_cdr;
    device->cdrw = drives[i].write_cdrw;

    device->buffer_size = drives[i].buffer_size;
    device->dummy_write = drives[i].write_simulate;

    /* write modes */
    device->tao_block_types = drives[i].tao_block_types;
    device->sao_block_types = drives[i].sao_block_types;
    device->raw_block_types = drives[i].raw_block_types;
    device->packet_block_types = drives[i].packet_block_types;

    device->dvdr = drives[i].write_dvdr;
    device->dvdram = drives[i].write_dvdram;

    ret = burn_drive_get_adr (&(drives[i]), device->addr);
    if (ret <= 0)
      g_error ("Unable to get drive %s address (ret=%d). Please report this problem to libburn-hackers@pykix.org", device->name, ret);

    refresh_supported_speeds (device, &(drives[i]));
        
    devices = g_list_append (devices, device);
  }

  burn_drive_info_free (drives);
  burn_finish ();
  
  return n_drives;
}

gboolean
xfburn_device_grab (XfburnDevice * device, struct burn_drive_info **drive_info)
{
  gint ret;
  gchar drive_addr[BURN_DRIVE_ADR_LEN];

  ret = burn_drive_convert_fs_adr (device->addr, drive_addr);
  if (ret <= 0) {
    g_error ("Device address does not lead to a CD burner '%s' (ret=%d).", device->addr, ret);
    return FALSE;
  }

  ret = burn_drive_scan_and_grab (drive_info, drive_addr, 1);
  if (ret <= 0) {
    g_error ("Unable to grab drive at path '%s' (ret=%d).", device->addr, ret);
    return FALSE;
  }

  return TRUE;
}

void
xfburn_device_free (XfburnDevice * device)
{
  device_content_free (device, NULL);
  g_free (device);
}


XfburnDevice *
xfburn_device_lookup_by_name (const gchar * name)
{
  GList *device;

  device = devices;

  while (device) {
    XfburnDevice *device_data = (XfburnDevice *) device->data;

    if (g_ascii_strcasecmp (device_data->name, name) == 0)
      return device_data;

    device = g_list_next (device);
  }

  return NULL;
}

void
xfburn_device_list_free ()
{
  g_list_foreach (devices, (GFunc) xfburn_device_free, NULL);
  g_list_free (devices);
  
  devices = NULL;
}

