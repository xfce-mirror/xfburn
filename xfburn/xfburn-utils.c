$Id$
/*
 * Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include <gtk/gtk.h>

#include "xfburn-utils.h"
#include "xfburn-settings.h"

/**************/
/* cd-burning */
/**************/
void
xfburn_device_content_free (XfburnDevice * device, gpointer user_data)
{
  DBG ("freeing '%s'", device->name);
  g_free (device->name);
  g_free (device->id);
  g_free (device->node_path);
}

void
xfburn_device_free (XfburnDevice * device)
{
  xfburn_device_content_free (device, NULL);
  g_free (device);
}

static gchar **
get_file_as_list (const gchar * file)
{
  /* from GnomeBaker */
  gchar **ret = NULL;
  gchar *contents = NULL;

  g_return_val_if_fail (file != NULL, NULL);
  if (g_file_get_contents (file, &contents, NULL, NULL))
    ret = g_strsplit (contents, "\n", 0);
  else
    g_critical ("Failed to get contents of file [%s]", file);

  g_free (contents);
  return ret;
}

static void
devices_for_each (gpointer key, gpointer value, gpointer user_data)
{
  /* DBG ("---- key [%s], value [%s]", (gchar *) key, (gchar *) value); */
  g_free (key);
  g_free (value);
}

static GHashTable *
get_cdrominfo (gchar ** proccdrominfo, gint deviceindex)
{
  /* from GnomeBaker */
  GHashTable *ret = NULL;
  gchar **info = proccdrominfo;

  g_return_val_if_fail (proccdrominfo != NULL, NULL);
  g_return_val_if_fail (deviceindex >= 1, NULL);

  g_message ("looking for device [%d]", deviceindex);

  while (*info != NULL) {
    g_strstrip (*info);
    if (strlen (*info) > 0) {
      if (strstr (*info, "drive name:") != NULL)
        ret = g_hash_table_new (g_str_hash, g_str_equal);

      if (ret != NULL) {
        gint columnindex = 0;
        gchar *key = NULL;
        gchar **columns = g_strsplit_set (*info, "\t", 0);
        gchar **column = columns;
        while (*column != NULL) {
          g_strstrip (*column);
          if (strlen (*column) > 0) {
            if (columnindex == 0)
              key = *column;
            else if (columnindex == deviceindex)
              g_hash_table_insert (ret, g_strdup (key), g_strdup (*column));
            ++columnindex;
          }
          ++column;
        }

        /* We must check if we found the device index we were
           looking for */
        if (columnindex <= deviceindex) {
          g_message ("Requested device index [%d] is out of bounds. " "All devices have been read.", deviceindex);
          g_hash_table_destroy (ret);
          ret = NULL;
          break;
        }

        g_strfreev (columns);
      }
    }
    ++info;
  }

  return ret;
}

static void
get_ide_device (const gchar * devicenode, const gchar * devicenodepath, gchar ** modelname, gchar ** deviceid)
{
  /* from GnomeBaker */
  gchar *contents = NULL;
  gchar *file = g_strdup_printf ("/proc/ide/%s/model", devicenode);

  g_return_if_fail (devicenode != NULL);
  g_return_if_fail (modelname != NULL);
  g_return_if_fail (deviceid != NULL);
  DBG ("probing [%s]", devicenode);

  if (g_file_get_contents (file, &contents, NULL, NULL)) {
    g_strstrip (contents);
    *modelname = g_strdup (contents);
    *deviceid = g_strdup (devicenodepath);
    g_free (contents);
  }
  else {
    g_critical ("Failed to open %s", file);
  }
  g_free (file);
}


static void
get_scsi_device (const gchar * devicenode, const gchar * devicenodepath, gchar ** modelname, gchar ** deviceid)
{
  /* from GnomeBaker */
  gchar **device_strs = NULL, **devices = NULL;

  g_return_if_fail (devicenode != NULL);
  g_return_if_fail (modelname != NULL);
  g_return_if_fail (deviceid != NULL);
  DBG ("probing [%s]", devicenode);

  if ((devices = get_file_as_list ("/proc/scsi/sg/devices")) == NULL) {
    g_critical (_("Failed to open /proc/scsi/sg/devices"));
  }
  else if ((device_strs = get_file_as_list ("/proc/scsi/sg/device_strs")) == NULL) {
    g_critical (_("Failed to open /proc/scsi/sg/device_strs"));
  }
  else {
    const gint scsicdromnum = atoi (&devicenode[strlen (devicenode) - 1]);
    gint cddevice = 0;
    gchar **device = devices;
    gchar **device_str = device_strs;
    while ((*device != NULL) && (*device_str) != NULL) {
      if ((strcmp (*device, "<no active device>") != 0) && (strlen (*device) > 0)) {
        gint scsihost, scsiid, scsilun, scsitype;
        if (sscanf (*device, "%d\t%*d\t%d\t%d\t%d", &scsihost, &scsiid, &scsilun, &scsitype) != 4) {
          g_critical (_("Error reading scsi information from /proc/scsi/sg/devices"));
        }
        /* 5 is the magic number according to lib-nautilus-burn */
        else if (scsitype == 5) {
          /* is the device the one we are looking for */
          if (cddevice == scsicdromnum) {
            gchar vendor[9], model[17];
            if (sscanf (*device_str, "%8c\t%16c", vendor, model) == 2) {
              vendor[8] = '\0';
              g_strstrip (vendor);

              model[16] = '\0';
              g_strstrip (model);

              *modelname = g_strdup_printf ("%s %s", vendor, model);
              *deviceid = g_strdup_printf ("%d,%d,%d", scsihost, scsiid, scsilun);
              break;
            }
          }
          ++cddevice;
        }
      }
      ++device_str;
      ++device;
    }
  }

  g_strfreev (devices);
  g_strfreev (device_strs);
}

void
xfburn_scan_devices ()
{
  /* adapted from GnomeBaker */
  gchar **info = NULL;

  /* clear current devices list */
  g_list_foreach (list_devices, (GFunc) xfburn_device_content_free, NULL);
  g_list_free (list_devices);
  list_devices = NULL;

#ifdef __linux__
  if (!(info = get_file_as_list ("/proc/sys/dev/cdrom/info"))) {
    g_critical ("Failed to open /proc/sys/dev/cdrom/info");
  }
  else {
    gint devicenum = 1;
    GHashTable *devinfo = NULL;

    while ((devinfo = get_cdrominfo (info, devicenum)) != NULL) {
      XfburnDevice *device_entry;
      const gchar *device = g_hash_table_lookup (devinfo, "drive name:");
      gchar *devicenodepath = g_strdup_printf ("/dev/%s", device);

      gchar *modelname = NULL, *deviceid = NULL;

      if (device[0] == 'h')
        get_ide_device (device, devicenodepath, &modelname, &deviceid);
      else
        get_scsi_device (device, devicenodepath, &modelname, &deviceid);

      device_entry = g_new0 (XfburnDevice, 1);
      device_entry->name = modelname;
      device_entry->id = deviceid;
      device_entry->node_path = devicenodepath;

      if (g_ascii_strcasecmp (g_hash_table_lookup (devinfo, "Can write CD-R:"), "1") == 0)
        device_entry->cdr = TRUE;
      if (g_ascii_strcasecmp (g_hash_table_lookup (devinfo, "Can write CD-RW:"), "1") == 0)
        device_entry->cdrw = TRUE;
      if (g_ascii_strcasecmp (g_hash_table_lookup (devinfo, "Can write DVD-R:"), "1") == 0)
        device_entry->dvdr = TRUE;
      if (g_ascii_strcasecmp (g_hash_table_lookup (devinfo, "Can write DVD-RAM:"), "1") == 0)
        device_entry->dvdram = TRUE;

      list_devices = g_list_append (list_devices, device_entry);

      g_message ("device [%d] found : %s (%s)", devicenum, modelname, devicenodepath);
      g_message ("device [%d] capabilities :%s%s%s%s", devicenum, device_entry->cdr ? " CD-R" : "",
                 device_entry->cdrw ? " CD-RW" : "", device_entry->dvdr ? " DVD-R" : "",
                 device_entry->dvdram ? " DVD-RAM" : "");

      g_hash_table_foreach (devinfo, devices_for_each, NULL);
      g_hash_table_destroy (devinfo);
      devinfo = NULL;
      ++devicenum;
    }
  }

  g_strfreev (info);
#else
#warning this program currently supports only Linux sorry :-(
#endif
}

XfburnDevice *
xfburn_device_lookup_by_name (const gchar * name)
{
  GList *device;

  device = list_devices;

  while (device) {
    XfburnDevice *device_data = (XfburnDevice *) device->data;

    if (g_ascii_strcasecmp (device_data->name, name) == 0)
      return device_data;

    device = g_list_next (device);
  }

  return NULL;
}

/* CDS_NO_DISC
 * CDS_TRAY_OPEN
 * CDS_DRIVE_NOT_READY
 * CDS_DISC_OK
 */
gint
xfburn_device_query_cdstatus (XfburnDevice * device)
{
  int fd, ret;
   
  /* adapted from GnomeBaker */
  g_return_val_if_fail (device != NULL, FALSE);

  fd = open (device->node_path, O_RDONLY | O_NONBLOCK);

  ret = ioctl (fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
  
  
  if (ret == -1)
    g_critical ("xfburn_device_query_cdstatus - ioctl failed");
  
  return ret;
}

/***********/
/* cursors */
/***********/
void
xfburn_busy_cursor (GtkWidget * widget)
{
  GdkCursor *cursor;

  g_return_if_fail (widget != NULL);
  cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (gtk_widget_get_parent_window (widget), cursor);
  gdk_cursor_destroy (cursor);
  gdk_flush ();
}

void
xfburn_default_cursor (GtkWidget * widget)
{
  g_return_if_fail (widget != NULL);
  gdk_window_set_cursor (gtk_widget_get_parent_window (widget), NULL);
  gdk_flush ();
}

/*******************/
/* for filebrowser */
/*******************/
gchar *
xfburn_humanreadable_filesize (guint64 size)
{
  if (!xfburn_settings_get_boolean ("human-readable-units", TRUE))
    return g_strdup_printf ("%lu B", (long unsigned int) size);
  
  /* copied from GnomeBaker */

  gchar *ret = NULL;
  const gchar *unit_list[5] = { "B ", "KB", "MB", "GB", "TB" };
  gint unit = 0;
  gdouble human_size = (gdouble) size;

  while (human_size > 1024) {
    human_size = human_size / 1024;
    unit++;
  }

  if ((human_size - (gulong) human_size) > 0.1)
    ret = g_strdup_printf ("%.2f %s", human_size, unit_list[unit]);
  else
    ret = g_strdup_printf ("%.0f %s", human_size, unit_list[unit]);
  return ret;
}

guint64
xfburn_calc_dirsize (const gchar * dirname)
{
  /* copied from GnomeBaker */
  guint64 size = 0;

  GDir *dir = g_dir_open (dirname, 0, NULL);
  if (dir != NULL) {
    const gchar *name = g_dir_read_name (dir);
    while (name != NULL) {
      /* build up the full path to the name */
      gchar *fullname = g_build_filename (dirname, name, NULL);
      struct stat s;

      if (stat (fullname, &s) == 0) {
        /* see if the name is actually a directory or a regular file */
        if (s.st_mode & S_IFDIR)
          size += (guint64) s.st_size + xfburn_calc_dirsize (fullname);
        else if (s.st_mode & S_IFREG)
          size += (guint64) s.st_size;
      }

      g_free (fullname);
      name = g_dir_read_name (dir);
    }

    g_dir_close (dir);
  }

  return size;
}

void
xfburn_browse_for_file (GtkEntry *entry, GtkWindow *parent)
{
  GtkWidget *dialog;
  const gchar *text;
  
  text = gtk_entry_get_text (entry);

  dialog = xfce_file_chooser_new (_("Select command"), parent, XFCE_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,
                                  GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
  if (strlen (text) > 0)
    xfce_file_chooser_set_filename (XFCE_FILE_CHOOSER (dialog), text);
  
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    gchar *filename = NULL;
    
    filename = xfce_file_chooser_get_filename (XFCE_FILE_CHOOSER (dialog));
    gtk_entry_set_text (entry, filename);
    g_free (filename);
  } 

  gtk_widget_destroy (dialog);
}
