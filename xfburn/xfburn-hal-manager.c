/* $Id: xfburn-hal-manager.c 4382 2006-11-01 17:08:37Z dmohr $ */
/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *  Copyright (c) 2008      David Mohr (dmohr@mcbf.net)
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

#ifdef HAVE_HAL

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libhal-storage.h>

#include <errno.h>

#include <libxfce4util/libxfce4util.h>
#ifdef HAVE_THUNAR_VFS
# include <thunar-vfs/thunar-vfs.h>
#endif
#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-global.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-device-list.h"

#include "xfburn-hal-manager.h"

static void xfburn_hal_manager_class_init (XfburnHalManagerClass * klass);
static void xfburn_hal_manager_init (XfburnHalManager * obj);
static void xfburn_hal_manager_finalize (GObject * object);

static void hal_finalize (LibHalContext  *hal_context);
static void cb_device_added (LibHalContext *ctx, const char *udi);
static void cb_device_removed (LibHalContext *ctx, const char *udi);
static void cb_prop_modified (LibHalContext *ctx, const char *udi, const char *key,
                              dbus_bool_t is_removed, dbus_bool_t is_added);

#define XFBURN_HAL_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_HAL_MANAGER, XfburnHalManagerPrivate))

enum {
  VOLUME_CHANGED,
  LAST_SIGNAL,
}; 

typedef struct {
  LibHalContext  *hal_context;
  DBusConnection *dbus_connection;
  gchar *error;

#ifdef HAVE_THUNAR_VFS
  ThunarVfsVolumeManager *thunar_volman;
#endif
} XfburnHalManagerPrivate;

static XfburnHalManager *halman = NULL;

/*********************/
/* class declaration */
/*********************/
static XfburnProgressDialogClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];

GtkType
xfburn_hal_manager_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnHalManagerClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_hal_manager_class_init,
      NULL,
      NULL,
      sizeof (XfburnHalManager),
      0,
      (GInstanceInitFunc) xfburn_hal_manager_init,
      NULL
    };

    type = g_type_register_static (G_TYPE_OBJECT, "XfburnHalManager", &our_info, 0);
  }

  return type;
}

static void
xfburn_hal_manager_class_init (XfburnHalManagerClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  g_type_class_add_private (klass, sizeof (XfburnHalManagerPrivate));
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_hal_manager_finalize;

  signals[VOLUME_CHANGED] = g_signal_new ("volume-changed", XFBURN_TYPE_HAL_MANAGER, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnHalManagerClass, volume_changed),
                                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);
}

static void
xfburn_hal_manager_init (XfburnHalManager * obj)
{
  XfburnHalManagerPrivate *priv = XFBURN_HAL_MANAGER_GET_PRIVATE (obj);
  LibHalContext  *hal_context = NULL;
  DBusError derror;
  //GError *error = NULL;

  DBusConnection *dbus_connection;
  priv->error = NULL;

  //if (halman != NULL)
  //  g_error ("The HAL context was already there when trying to create a hal-manager!");
  
  /* dbus & hal init code taken from exo */
  /* initialize D-Bus error */
  dbus_error_init (&derror);

  /* try to connect to the system bus */
  dbus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, &derror);
  if (G_LIKELY (dbus_connection != NULL)) {
    /* try to allocate a new HAL context */
    hal_context = libhal_ctx_new ();
    if (G_LIKELY (hal_context != NULL)) {
      /* setup the D-Bus connection for the HAL context */
      if (libhal_ctx_set_dbus_connection (hal_context, dbus_connection)) {

        /* try to initialize the HAL context */
        libhal_ctx_init (hal_context, &derror);
      } else {
        dbus_set_error_const (&derror, DBUS_ERROR_NO_MEMORY, g_strerror (ENOMEM));
      }
    } else {
      /* record the allocation failure of the context */
      dbus_set_error_const (&derror, DBUS_ERROR_NO_MEMORY, g_strerror (ENOMEM));
    }
  }

  /* check if we failed */
  if (dbus_error_is_set (&derror)) {
    /* check if a HAL context was allocated */
    if (G_UNLIKELY (hal_context != NULL)) {
      /* drop the allocated HAL context */
      hal_finalize (hal_context);
      hal_context = NULL;
      priv->error = "HAL";
    } else {
      priv->error = "DBus";
    }
    dbus_error_free (&derror);
  } else {
    if (!libhal_ctx_set_device_added (hal_context, cb_device_added))
      g_warning ("Could not setup HAL callback for device_added");

    if (!libhal_ctx_set_device_removed (hal_context, cb_device_removed))
      g_warning ("Could not setup HAL callback for device_removed");

    if (!libhal_ctx_set_device_property_modified (hal_context, cb_prop_modified))
      g_warning ("Could not setup HAL callback for prop_modified");
  }

  priv->hal_context = hal_context;
  priv->dbus_connection = dbus_connection;

#ifdef HAVE_THUNAR_VFS
  /* FIXME: for some weird reason the hal callbacks don't actually work, 
   *        unless we also fetch an instance of thunar_vfs_volman. Why??
   *    Not terrible though, because we'll need to use it eventually anyways */
  priv->thunar_volman = thunar_vfs_volume_manager_get_default ();
  if (priv->thunar_volman != NULL) {
    //g_signal_connect (G_OBJECT (priv->thunar_volman), "volumes-added", G_CALLBACK (cb_volumes_changed), box);
    //g_signal_connect (G_OBJECT (priv->thunar_volman), "volumes-removed", G_CALLBACK (cb_volumes_changed), box);
  } else {
    g_warning ("Error trying to access thunar-vfs-volume-manager!");
  }
  /*
  */
#endif
}

static void
xfburn_hal_manager_finalize (GObject * object)
{
  XfburnHalManagerPrivate *priv = XFBURN_HAL_MANAGER_GET_PRIVATE (object);

#ifdef HAVE_THUNAR_VFS
  g_object_unref (priv->thunar_volman);
#endif

  hal_finalize (priv->hal_context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
  halman = NULL;
}

/*           */
/* internals */
/*           */
static void
hal_finalize (LibHalContext  *hal_context)
{
  DBusError derror;

  dbus_error_init (&derror);
  libhal_ctx_shutdown (hal_context, &derror);
  if (dbus_error_is_set (&derror)) {
    DBG ("Error shutting hal down!");
  }
  dbus_error_free (&derror);
  libhal_ctx_free (hal_context);
}

static void cb_device_added (LibHalContext *ctx, const char *udi)
{
  DBG ("HAL: device added");
  g_signal_emit (halman, signals[VOLUME_CHANGED], 0);
}

static void cb_device_removed (LibHalContext *ctx, const char *udi)
{
  DBG ("HAL: device removed");
  g_signal_emit (halman, signals[VOLUME_CHANGED], 0);
}

static void cb_prop_modified (LibHalContext *ctx, const char *udi,
                              const char *key, dbus_bool_t is_removed, dbus_bool_t is_added)
{
  /* Lets ignore this for now,
   * way too many of these get triggered when a disc is
   * inserted or removed!
  DBG ("HAL: property modified");
  g_signal_emit (halman, signals[VOLUME_CHANGED], 0);
  */
}

GObject *
xfburn_hal_manager_new ()
{
  if (G_UNLIKELY (halman != NULL))
    g_error ("Trying to create a second instance of hal manager!");
  return g_object_new (XFBURN_TYPE_HAL_MANAGER, NULL);
}

/*        */
/* public */
/*        */

gchar *
xfburn_hal_manager_create_global ()
{
  XfburnHalManagerPrivate *priv;

  halman = XFBURN_HAL_MANAGER (xfburn_hal_manager_new ());

  priv = XFBURN_HAL_MANAGER_GET_PRIVATE (halman);

  if (priv->error) {
    xfburn_hal_manager_shutdown ();
    return g_strdup_printf ("Failed to initialize %s!", priv->error);
  } else
    return NULL;
}

XfburnHalManager *
xfburn_hal_manager_get_instance ()
{
  if (G_UNLIKELY (halman == NULL))
    g_error ("There is no instance of a hal manager!");
  return halman;
}

void
xfburn_hal_manager_shutdown ()
{
  if (G_UNLIKELY (halman == NULL))
    g_error ("There is no instance of a hal manager!");
  g_object_unref (halman);
  halman = NULL;
}

void
xfburn_hal_manager_send_volume_changed ()
{
  //gdk_threads_enter ();
  g_signal_emit (halman, signals[VOLUME_CHANGED], 0);
  //gdk_threads_leave ();
}

int 
xfburn_hal_manager_get_devices (XfburnHalManager *halman, GList **device_list)
{
  XfburnHalManagerPrivate *priv = XFBURN_HAL_MANAGER_GET_PRIVATE (halman);
  char **all_devices, **devices;
  int num;
  DBusError error;
  int n_devices = 0;

  dbus_error_init (&error);

  all_devices = libhal_get_all_devices (priv->hal_context, &num, &error);

  if (dbus_error_is_set (&error)) {
    g_warning ("Could not get list of devices from HAL: %s", error.message);
    return -1;
  }

  for (devices = all_devices; *devices != NULL; devices++) {
    dbus_bool_t exists;
    char **cap_list, **caps;
    gboolean writer = FALSE;
    int write_speed;

    exists = libhal_device_property_exists (priv->hal_context, *devices, "info.capabilities", &error);
    if (dbus_error_is_set (&error)) {
      g_warning ("Error checking HAL property for %s: %s", *devices, error.message);
      dbus_error_free (&error);
      return -1;
    }

    if (!exists)
      continue;

    cap_list = libhal_device_get_property_strlist (priv->hal_context, *devices, "info.capabilities", &error);
    if (dbus_error_is_set (&error)) {
      g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
      dbus_error_free (&error);
      return -1;
    }

    for (caps = cap_list; *caps != NULL; caps++) {
      if (strcmp (*caps, "storage.cdrom") == 0) {
        exists = libhal_device_property_exists (priv->hal_context, *devices, "storage.cdrom.write_speed", &error);
        if (dbus_error_is_set (&error)) {
          g_warning ("Error checking HAL property for %s: %s", *devices, error.message);
          dbus_error_free (&error);
          return -1;
        }

        if (!exists)
          break;

        write_speed = libhal_device_get_property_int (priv->hal_context, *devices, "storage.cdrom.write_speed", &error);
        if (dbus_error_is_set (&error)) {
          g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
          dbus_error_free (&error);
          return -1;
        }

        if (write_speed > 0)
          writer = TRUE;
        break;
      }
    }
    libhal_free_string_array (cap_list);

    if (writer) {
      XfburnDevice *device = g_new0 (XfburnDevice, 1);
      char *str, *str_vendor;
      gboolean dvdr = FALSE;
      /*
      libhal_device_print (priv->hal_context, *devices, &error);
      printf ("\n");

      if (dbus_error_is_set (&error)) {
        g_warning ("Error printing HAL device %s: %s", *devices, error.message);
        dbus_error_free (&error);
        return -1;
      }
      */

      device->accessible = FALSE;
      str_vendor = libhal_device_get_property_string (priv->hal_context, *devices, "storage.vendor", &error);
      if (dbus_error_is_set (&error)) {
        g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
        dbus_error_free (&error);
        g_free (device);
        continue;
      }

      str = libhal_device_get_property_string (priv->hal_context, *devices, "storage.model", &error);
      if (dbus_error_is_set (&error)) {
        g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
        dbus_error_free (&error);
        g_free (device);
        continue;
      }

      device->name = g_strconcat (str_vendor, " ", str, NULL);
      libhal_free_string (str_vendor);
      libhal_free_string (str);

      str = libhal_device_get_property_string (priv->hal_context, *devices, "block.device", &error);
      if (dbus_error_is_set (&error)) {
        g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
        dbus_error_free (&error);
        g_free (device);
        continue;
      }

      g_strlcpy (device->addr, str, BURN_DRIVE_ADR_LEN);
      libhal_free_string (str);

      device->cdr = libhal_device_get_property_bool (priv->hal_context, *devices, "storage.cdrom.cdr", &error);
      if (dbus_error_is_set (&error)) {
        g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
        dbus_error_free (&error);
        g_free (device);
        continue;
      }

      device->cdr = libhal_device_get_property_bool (priv->hal_context, *devices, "storage.cdrom.cdr", &error);
      if (dbus_error_is_set (&error)) {
        g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
        dbus_error_free (&error);
        g_free (device);
        continue;
      }

      device->cdrw = libhal_device_get_property_bool (priv->hal_context, *devices, "storage.cdrom.cdrw", &error);
      if (dbus_error_is_set (&error)) {
        g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
        dbus_error_free (&error);
        g_free (device);
        continue;
      }

      device->dvdr = libhal_device_get_property_bool (priv->hal_context, *devices, "storage.cdrom.dvdr", &error);
      if (dbus_error_is_set (&error)) {
        g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
        dbus_error_free (&error);
        g_free (device);
        continue;
      }

      dvdr = libhal_device_get_property_bool (priv->hal_context, *devices, "storage.cdrom.dvdplusr", &error);
      if (dbus_error_is_set (&error)) {
        g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
        dbus_error_free (&error);
        g_free (device);
        continue;
      }
      device->dvdr |= dvdr;

      device->dvdram = libhal_device_get_property_bool (priv->hal_context, *devices, "storage.cdrom.dvdram", &error);
      if (dbus_error_is_set (&error)) {
        g_warning ("Error getting HAL property for %s: %s", *devices, error.message);
        dbus_error_free (&error);
        g_free (device);
        continue;
      }

      DBG ("Found drive '%s' at '%s'", device->name, device->addr);
      *device_list = g_list_append (*device_list, device);
      n_devices++;
    }
  }

  libhal_free_string_array (all_devices);

  return n_devices;
}

gboolean
xfburn_hal_manager_check_ask_umount (XfburnHalManager *halman, XfburnDevice *device)
{
  XfburnHalManagerPrivate *priv = XFBURN_HAL_MANAGER_GET_PRIVATE (halman);
  LibHalVolume *vol;
#ifdef HAVE_THUNAR_VFS
  const char *mp;
  ThunarVfsInfo *th_info;
  ThunarVfsVolume *th_vol;
  ThunarVfsPath *th_path;
#endif
  gboolean unmounted = FALSE;
  
  vol = libhal_volume_from_device_file (priv->hal_context, device->addr);
  if (vol == NULL) {
    /* if we can't get a volume, then we're assuming that there is no disc in the drive */
    return TRUE;
  }

  if (!libhal_volume_is_mounted (vol))
    return TRUE;

#ifdef HAVE_THUNAR_VFS
  mp = libhal_volume_get_mount_point (vol);
  DBG ("%s is mounted at %s", device->addr, mp);


  th_path = thunar_vfs_path_new (mp, NULL);
  if (!th_path) {
    g_warning ("Error getting thunar path for %s!", mp);
    return FALSE;
  }

  th_info = thunar_vfs_info_new_for_path (th_path, NULL);
  thunar_vfs_path_unref (th_path);
  if (!th_info) {
    g_warning ("Error getting thunar info for %s!", mp);
    return FALSE;
  }

  th_vol = thunar_vfs_volume_manager_get_volume_by_info (priv->thunar_volman, th_info);
  thunar_vfs_info_unref (th_info);

  if (!th_vol) {
    g_warning ("Error getting thunar volume for %s!", mp);
    return FALSE;
  }

  if (!thunar_vfs_volume_is_mounted (th_vol)) {
    return FALSE;
  }

  /* FIXME: ask if we should unmount? */
  unmounted = thunar_vfs_volume_unmount (th_vol, NULL, NULL);
  if (unmounted)
    g_message ("Unmounted %s", mp);
  else {
    xfce_err ("Failed to unmount %s. Drive cannot be used for burning.", mp);
    DBG ("Failed to unmount %s", mp);
  }

#endif
  return unmounted;
}

#endif /* HAVE_HAL */
