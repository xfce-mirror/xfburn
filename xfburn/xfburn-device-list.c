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

#include "xfburn-global.h"
#include "xfburn-hal-manager.h"

#include "xfburn-device-list.h"

static GList *devices = NULL;
static enum burn_disc_status disc_status;
static int profile_no = 0;
static char profile_name[80];
static int is_erasable = 0;

#define CAN_BURN_CONDITION device->cdr || device->cdrw || device->dvdr || device->dvdram
#define DEVICE_INFO_PRINTF "%s can burn: %d [cdr: %d, cdrw: %d, dvdr: %d, dvdram: %d]", device->name, CAN_BURN_CONDITION, device->cdr, device->cdrw, device->dvdr, device->dvdram

/*************/
/* internals */
/*************/
static void
device_content_free (XfburnDevice * device, gpointer user_data)
{
  g_free (device->name);

  g_slist_free (device->supported_cdr_speeds);
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

/* sort the speed list in ascending order */
static gint
cmp_ints (gconstpointer a, gconstpointer b)
{
  return GPOINTER_TO_INT(a) - GPOINTER_TO_INT(b);
}

static void
refresh_disc (XfburnDevice * device, struct burn_drive_info *drive_info)
{
  gint ret;

  /* check if there is a disc in the drive */
  while ((disc_status = burn_disc_get_status (drive_info->drive)) == BURN_DISC_UNREADY)
    usleep(100001);

  DBG ("disc_status = %d", disc_status);

  if ((ret = burn_disc_get_profile(drive_info->drive, &profile_no, profile_name)) != 1) {
    g_warning ("no profile could be retrieved");
  }
  is_erasable = burn_disc_erasable (drive_info->drive);
  DBG ("profile_no = 0x%x (%s), %s erasable", profile_no, profile_name, (is_erasable ? "" : "NOT"));
}

static void
refresh_speed_list (XfburnDevice * device, struct burn_drive_info *drive_info)
{
  struct burn_speed_descriptor *speed_list = NULL;
  gint ret;

  /* fill new list */
  ret = burn_drive_get_speedlist (drive_info->drive, &speed_list);
  /* speed_list = NULL; DEBUG */ 

  if (ret > 0 && speed_list != NULL) {
    struct burn_speed_descriptor *el = speed_list;

    while (el) {
      gint speed = el->write_speed;
      
      /* FIXME: why do we need no_speed_duplicate? */
      if (speed > 0 && no_speed_duplicate (device->supported_cdr_speeds, speed)) {
          device->supported_cdr_speeds = g_slist_prepend (device->supported_cdr_speeds, GINT_TO_POINTER (speed));
      } 

      el = el->next;
    }

    burn_drive_free_speedlist (&speed_list); 
    device->supported_cdr_speeds = g_slist_sort (device->supported_cdr_speeds, &cmp_ints);
  } else if (ret == 0 || speed_list == NULL) {
    g_warning ("reported speed list is empty for device:");
    g_warning (DEVICE_INFO_PRINTF);
  } else {
    /* ret < 0 */
    g_error ("severe error while retrieving speed list");
  }
}

void
fillin_libburn_device_info (XfburnDevice *device, struct burn_drive_info *drives)
{
  device->accessible = TRUE;

  device->cdr = drives->write_cdr;
  device->cdrw = drives->write_cdrw;

  device->dvdr = drives->write_dvdr;
  device->dvdram = drives->write_dvdram;
  
  device->buffer_size = drives->buffer_size;
  device->dummy_write = drives->write_simulate;

  /* write modes */
  device->tao_block_types = drives->tao_block_types;
  device->sao_block_types = drives->sao_block_types;
  device->raw_block_types = drives->raw_block_types;
  device->packet_block_types = drives->packet_block_types;

  DBG (DEVICE_INFO_PRINTF);
}

gint
get_libburn_device_list ()
{
  struct burn_drive_info *drives;
  guint i;
  gint ret; 
  gboolean can_burn;
  guint n_drives = 0;
  guint n_burners = 0;

  *profile_name = '\0';

  if (!burn_initialize ()) {
    g_critical ("Unable to initialize libburn");
    return -1;
  }
    
  while ((ret = burn_drive_scan (&drives, &n_drives)) == 0)
    usleep (1002);

  if (ret < 0)
    g_warning ("An error occurred while scanning for available drives!");

  if (n_drives < 1) {
    g_warning ("No drives were found! If this is in error, check the permissions.");
  }

  for (i = 0; i < n_drives; i++) {
    XfburnDevice *device = g_new0 (XfburnDevice, 1);
    gint ret = 0;
    
    device->name = g_strconcat (drives[i].vendor, " ", drives[i].product, NULL);

    fillin_libburn_device_info (device, &drives[i]);

    can_burn = CAN_BURN_CONDITION;
    
    ret = burn_drive_get_adr (&(drives[i]), device->addr);
    if (ret <= 0)
      g_error ("Unable to get drive %s address (ret=%d). Please report this problem to libburn-hackers@pykix.org", device->name, ret);
    DBG ("device->addr = %s", device->addr);

    if (can_burn) {
      devices = g_list_append (devices, device);
      n_burners++;
    }
  }

  burn_drive_info_free (drives);
  burn_finish ();

  if (n_drives > 0 && n_burners < 1)
    g_warning ("There are %d drives in your system, but none are capable of burning!", n_drives);
  
  return n_burners;
}

/**************/
/* public API */
/**************/
GList *
xfburn_device_list_get_list ()
{
  return devices;
}

enum burn_disc_status
xfburn_device_list_get_disc_status ()
{
  return disc_status;
}

int
xfburn_device_list_get_profile_no ()
{
  return profile_no;
}

const char *
xfburn_device_list_get_profile_name ()
{
  return profile_name;
}

gboolean
xfburn_device_list_disc_is_erasable ()
{
  return is_erasable != 0;
}

gboolean
xfburn_device_refresh_info (XfburnDevice * device, gboolean get_speed_info)
{
  struct burn_drive_info *drive_info = NULL;
  gboolean ret;
#ifdef HAVE_HAL
  XfburnHalManager *halman = xfburn_hal_manager_get_instance ();
#endif

  if (G_UNLIKELY (device == NULL)) {
    DBG ("Hmm, why can we refresh when there is no drive?");
    return FALSE;
  }

  /* reset other internal structures */
  profile_no = 0;
  *profile_name = '\0';
  is_erasable = 0;

  /* empty previous speed list */
  g_slist_free (device->supported_cdr_speeds);
  device->supported_cdr_speeds = NULL;

  if (!device->accessible) {
#ifdef HAVE_HAL 
    if (!xfburn_hal_manager_check_ask_umount (halman, device))
      return FALSE;
#else
    return FALSE;
#endif
  }

  if (!burn_initialize ()) {
    g_critical ("Unable to initialize libburn");
    return FALSE;
  }

  if (!xfburn_device_grab (device, &drive_info)) {
    ret = FALSE;
    g_warning ("Couldn't grab drive in order to update speed list.");
    disc_status = BURN_DISC_UNGRABBED;
  } else {
    if (!device->accessible)
      fillin_libburn_device_info (device, drive_info);
    ret = TRUE;
    refresh_disc (device, drive_info);
    if (get_speed_info)
      refresh_speed_list (device, drive_info);

    burn_drive_release (drive_info->drive, 0);
  }

  burn_finish ();

  return ret;
}

gint
xfburn_device_list_init ()
{
#ifdef HAVE_HAL
  XfburnHalManager *halman = xfburn_hal_manager_get_instance ();
#endif
  int n_drives;

  if (devices) {
    g_list_foreach (devices, (GFunc) device_content_free, NULL);
    g_list_free (devices);
    devices = NULL;
  }

#ifdef HAVE_HAL
  n_drives = xfburn_hal_manager_get_devices (halman, &devices);
  if (n_drives == -1) {
    /* some error occurred while checking hal properties, so try libburn instead */
    n_drives = get_libburn_device_list ();
  }
#else
  n_drives = get_libburn_device_list ();
#endif

  return n_drives;
}

gboolean
xfburn_device_grab (XfburnDevice * device, struct burn_drive_info **drive_info)
{
  int ret = 0;
  gchar drive_addr[BURN_DRIVE_ADR_LEN];
  int i;
  const int max_checks = 4;

  ret = burn_drive_convert_fs_adr (device->addr, drive_addr);
  if (ret <= 0) {
    g_error ("Device address does not lead to a CD burner '%s' (ret=%d).", device->addr, ret);
    return FALSE;
  }

  /* we need to try to grab several times, because
   * the drive might be busy detecting the media */
  for (i=0; i<max_checks; i++) {
    ret = burn_drive_scan_and_grab (drive_info, drive_addr, 0);
    if (ret > 0)
      break;
    else if  (i < (max_checks-1))
      usleep((i+1)*100001);
  }

  if (ret <= 0) {
    g_warning ("Unable to grab drive at path '%s' (ret=%d).", device->addr, ret);
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

