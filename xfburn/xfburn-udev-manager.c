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

#ifdef HAVE_GUDEV

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gio/gio.h>
#include <gudev/gudev.h>

#include <errno.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include "xfburn-global.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-device-list.h"

#include "xfburn-udev-manager.h"

static void xfburn_udev_manager_finalize (GObject * object);

static GObject * xfburn_udev_manager_new (void);

static void cb_device_monitor_uevent(GUdevClient  *client, const gchar  *action, GUdevDevice  *udevice, XfburnUdevManager *obj);

#define XFBURN_UDEV_MANAGER_GET_PRIVATE(obj) (xfburn_udev_manager_get_instance_private (XFBURN_UDEV_MANAGER (obj)))

enum {
  VOLUME_CHANGED,
  LAST_SIGNAL,
}; 

typedef struct {
  GUdevClient *client;
  GVolumeMonitor *volume_monitor;
  gchar *error;
} XfburnUdevManagerPrivate;

typedef struct {
  GMainLoop *loop;
  GCancellable *cancel;
  guint timeout_id;
  gboolean result;
  GError *error;
} XfburnUdevManagerGioOperation;

static XfburnUdevManager *instance = NULL;

/*********************/
/* class declaration */
/*********************/
static XfburnProgressDialogClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE(XfburnUdevManager, xfburn_udev_manager, G_TYPE_OBJECT);

static void
xfburn_udev_manager_class_init (XfburnUdevManagerClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_udev_manager_finalize;

  signals[VOLUME_CHANGED] = g_signal_new ("volume-changed", XFBURN_TYPE_UDEV_MANAGER, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnUdevManagerClass, volume_changed),
                                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);
}

static void
xfburn_udev_manager_init (XfburnUdevManager * obj)
{
  XfburnUdevManagerPrivate *priv = XFBURN_UDEV_MANAGER_GET_PRIVATE (obj);
  const gchar* const subsystems[] = { "block", NULL };

  priv->error = NULL;
  priv->volume_monitor = NULL;
  priv->client = g_udev_client_new (subsystems);
  if (G_LIKELY (priv->client != NULL)) {
    g_signal_connect (G_OBJECT (priv->client), "uevent",
		      G_CALLBACK (cb_device_monitor_uevent), obj);

    priv->volume_monitor = g_volume_monitor_get();
    if (priv->volume_monitor != NULL) {
      //g_signal_connect (G_OBJECT (priv->volume_monitor), "volume-added", G_CALLBACK (cb_volumes_changed), box);
      //g_signal_connect (G_OBJECT (priv->volume_monitor), "volume-removed", G_CALLBACK (cb_volumes_changed), box);
    } else {
      g_warning ("Error trying to access g_volume_monitor!");
    }
  }
}

static void
xfburn_udev_manager_finalize (GObject * object)
{
  XfburnUdevManagerPrivate *priv = XFBURN_UDEV_MANAGER_GET_PRIVATE (object);

  g_object_unref (priv->volume_monitor);
  g_object_unref (priv->client);

  G_OBJECT_CLASS (parent_class)->finalize (object);
  instance = NULL;
}

static void cb_device_monitor_uevent(GUdevClient  *client,
				     const gchar  *action,
				     GUdevDevice  *udevice,
				     XfburnUdevManager *obj)
{
  DBG ("UDEV: device uevent: %s", action);

  if (g_str_equal (action, "remove") || g_str_equal (action, "add") ||
      g_str_equal (action, "change"))
    g_signal_emit (instance, signals[VOLUME_CHANGED], 0);

}

static GObject *
xfburn_udev_manager_new (void)
{
  if (G_UNLIKELY (instance != NULL))
    g_error ("Trying to create a second instance of udev manager!");
  return g_object_new (XFBURN_TYPE_UDEV_MANAGER, NULL);
}

/*        */
/* public */
/*        */

gchar *
xfburn_udev_manager_create_global (void)
{
  XfburnUdevManagerPrivate *priv;

  instance = XFBURN_UDEV_MANAGER (xfburn_udev_manager_new ());

  priv = XFBURN_UDEV_MANAGER_GET_PRIVATE (instance);

  if (priv->error) {
    gchar *error_msg, *ret;

    error_msg = g_strdup (priv->error);
    xfburn_udev_manager_shutdown ();
    ret = g_strdup_printf ("Failed to initialize %s!", error_msg);
    g_free (error_msg);
    return ret;
  } else
    return NULL;
}

XfburnUdevManager *
xfburn_udev_manager_get_global (void)
{
  if (G_UNLIKELY (instance == NULL))
    g_error ("There is no instance of a udev manager!");
  return instance;
}

void
xfburn_udev_manager_shutdown (void)
{
  if (G_UNLIKELY (instance == NULL))
    g_error ("There is no instance of a udev manager!");
  g_object_unref (instance);
  instance = NULL;
}

void
xfburn_udev_manager_send_volume_changed (void)
{
  //gdk_threads_enter ();
  g_signal_emit (instance, signals[VOLUME_CHANGED], 0);
  //gdk_threads_leave ();
}

GList * 
xfburn_udev_manager_get_devices (XfburnUdevManager *udevman, gint *drives, gint *burners)
{
  XfburnUdevManagerPrivate *priv = XFBURN_UDEV_MANAGER_GET_PRIVATE (udevman);
  GList *devices, *l;
  GList *device_list = NULL;

  (*drives) = 0;
  (*burners) = 0;

  if (priv->client == NULL)
    return NULL;

  devices = g_udev_client_query_by_subsystem (priv->client, "block");
  for (l = devices; l != NULL; l = l->next) {
    const gchar *id_type = g_udev_device_get_property (l->data, "ID_TYPE");
    if (g_strcmp0 (id_type, "cd") == 0) {
      gboolean cdr = g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_CD_R");
      gboolean cdrw = g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_CD_RW");
      gboolean dvdr = g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_DVD_R")
      	           || g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_DVD_RW")
                   || g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_DVD_PLUS_R")
                   || g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_DVD_PLUS_RW")
                   || g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_DVD_PLUS_R_DL");
      gboolean dvdram = g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_DVD_RAM");
      gboolean bdr = g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_BD")
                  || g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_BD_R")
                  || g_udev_device_get_property_as_boolean(l->data, "ID_CDROM_BD_RE");

      (*drives)++;

      if (cdr || dvdr || dvdram || bdr) {
        XfburnDevice *device;
        const gchar *addr, *name, *rev, *str_model, *str_vendor;

        device = xfburn_device_new ();

	/* vendor */
	str_vendor = g_udev_device_get_sysfs_attr(l->data, "device/vendor");
        if (str_vendor == NULL)
	  str_vendor = g_udev_device_get_property (l->data, "ID_VENDOR_FROM_DATABASE");
        if (str_vendor == NULL)
          str_vendor = g_udev_device_get_property (l->data, "ID_VENDOR");
	if (str_vendor == NULL)
          str_vendor = g_udev_device_get_sysfs_attr (l->data, "manufacturer");

        /* model */
	str_model = g_udev_device_get_sysfs_attr(l->data, "device/model");
        if (str_model == NULL)
          str_model = g_udev_device_get_property (l->data, "ID_MODEL_FROM_DATABASE");
        if (str_model == NULL)
          str_model = g_udev_device_get_property (l->data, "ID_MODEL");
        if (str_model == NULL)
          str_model = g_udev_device_get_sysfs_attr (l->data, "product");

        name = xfburn_device_set_name (device, str_vendor, str_model);

        /* revision */
	rev = g_udev_device_get_sysfs_attr (l->data, "device/rev");
        if (rev == NULL)
          rev = g_udev_device_get_property (l->data, "ID_REVISION");
        g_object_set (G_OBJECT (device), "revision", rev, NULL);

        addr = g_udev_device_get_device_file(l->data);
#ifdef DEBUG_NULL_DEVICE
        g_object_set (G_OBJECT (device), "address", "stdio:/dev/null", NULL);
#else
        g_object_set (G_OBJECT (device), "address", addr, NULL);
#endif

        g_object_set (G_OBJECT (device), "cdr", cdr, NULL);
        g_object_set (G_OBJECT (device), "cdrw", cdrw, NULL);
        g_object_set (G_OBJECT (device), "dvdr", dvdr, NULL);
        g_object_set (G_OBJECT (device), "dvdram", dvdram, NULL);
        g_object_set (G_OBJECT (device), "bd", bdr, NULL);

        if (!xfburn_device_can_burn (device)) {
          g_message ("Ignoring reader '%s' at '%s'", name, addr);
          g_object_unref (device);
        } else {
          (*burners)++;
          device_list = g_list_append (device_list, device);
          DBG ("Found writer '%s' at '%s'", name, addr);
        }
      }
    }
    g_object_unref (l->data);
  }
  g_list_free (devices);
  return device_list;
}

static void
xfburn_udev_manager_gio_operation_end (gpointer callback_data)
{
  XfburnUdevManagerGioOperation *operation = callback_data;

  if (!operation->loop)
    return;
  if (!g_main_loop_is_running (operation->loop))
    return;
  g_main_loop_quit (operation->loop);	
}

static void
cb_device_umounted (GMount *mount,
                    XfburnUdevManagerGioOperation *operation)
{
  xfburn_udev_manager_gio_operation_end (operation);
  operation->result = TRUE;
}

static void
cb_device_umount_finish (GObject *source,
                         GAsyncResult *result,
                         gpointer user_data)
{
  XfburnUdevManagerGioOperation *op = user_data;

  if (!op->loop)
    return;

  op->result = g_mount_unmount_with_operation_finish (G_MOUNT (source),
				     		    result,
				     		    &op->error);

  g_debug("Umount operation completed (result = %d)", op->result);

  if (op->error) {
    if (op->error->code == G_IO_ERROR_NOT_MOUNTED) {
      /* That can happen sometimes */
      g_error_free (op->error);
      op->error = NULL;
      op->result = TRUE;
    }
    /* Since there was an error. The "unmounted" signal won't be 
     * emitted by GVolumeMonitor and therefore we'd get stuck if
     * we didn't get out of the loop. */
    xfburn_udev_manager_gio_operation_end (op);
  } else if (!op->result) {
    xfburn_udev_manager_gio_operation_end (op);
  }
}

static void
cb_gio_operation_cancelled (GCancellable *cancel,
                            XfburnUdevManagerGioOperation *operation)
{
  operation->result = FALSE;
  g_cancellable_cancel (operation->cancel);
  if (operation->loop && g_main_loop_is_running (operation->loop))
    g_main_loop_quit (operation->loop);
}

static gboolean
cb_gio_operation_timeout (gpointer callback_data)
{
  XfburnUdevManagerGioOperation *operation = callback_data;

  g_warning ("Volume/Disc operation timed out");

  xfburn_udev_manager_gio_operation_end (operation);
  operation->timeout_id = 0;
  operation->result = FALSE;
  return FALSE;
}

/* @Return TRUE if the drive is now accessible, FALSE if not.
 */
gboolean
xfburn_udev_manager_check_ask_umount (XfburnUdevManager *udevman, XfburnDevice *device)
{
  XfburnUdevManagerPrivate *priv = XFBURN_UDEV_MANAGER_GET_PRIVATE (udevman);
  gboolean unmounted = FALSE;
  gchar *device_file;
  GList *mounts;
  GMount *mount = NULL;
  GList *i;
  gchar *mp;

  if (priv->volume_monitor == NULL)
    return FALSE;

  mounts = g_volume_monitor_get_mounts (priv->volume_monitor);

  g_object_get (G_OBJECT (device), "address", &device_file, NULL);
  for (i = mounts; i != NULL; i = i->next) {
    GVolume *v;

    mount = G_MOUNT (i->data);
    v = g_mount_get_volume (mount);
    if (v != NULL) {
      char *devname = NULL;
      gboolean match;

      devname = g_volume_get_identifier (v, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
      match = g_str_equal (devname, device_file);

      g_free (devname);
      g_object_unref (v);

      if (match)
        break;
    }
    g_object_unref (mount);
    mount = NULL;
  }
  g_list_free (mounts);

  if (mount == NULL)
    return TRUE;

  if (g_mount_can_unmount (mount)) {
    /* FIXME: ask if we should unmount? */
    gulong umount_sig;
    XfburnUdevManagerGioOperation *op;

    op = g_new0 (XfburnUdevManagerGioOperation, 1);
    op->cancel = g_cancellable_new();

    umount_sig = g_signal_connect_after (mount,
                                         "unmounted",
                                         G_CALLBACK (cb_device_umounted),
                                         op);

    /* NOTE: we own a reference to mount
     * object so no need to ref it even more */
    g_mount_unmount_with_operation (mount,
                                    G_MOUNT_UNMOUNT_NONE,
                                    NULL,
                                    op->cancel,
                                    cb_device_umount_finish,
                                    op);
    DBG("Waiting for end of async operation");
    g_cancellable_reset (op->cancel);
    g_signal_connect (op->cancel,
                      "cancelled",
                      G_CALLBACK (cb_gio_operation_cancelled),
                      op);

    /* put a timeout (15 sec) */
    op->timeout_id = g_timeout_add_seconds (15,
                                            cb_gio_operation_timeout,
                                            op);

    op->loop = g_main_loop_new (NULL, FALSE);

    GDK_THREADS_LEAVE ();
    g_main_loop_run (op->loop);
    GDK_THREADS_ENTER ();

    g_main_loop_unref (op->loop);
    op->loop = NULL;

    if (op->timeout_id) {
      g_source_remove (op->timeout_id);
      op->timeout_id = 0;
    }

    if (op->error) {
      g_warning ("Medium op finished with an error: %s", op->error->message);

      if (op->error->code == G_IO_ERROR_FAILED_HANDLED) {
        DBG("Error already handled and displayed by GIO");

        /* means we shouldn't display any error message since 
         * that was already done */
        g_error_free (op->error);
        op->error = NULL;
      } else
        g_error_free (op->error);

      op->error = NULL;
    }

    unmounted = op->result;

    if (op->cancel) {
      g_cancellable_cancel (op->cancel);
      g_object_unref (op->cancel);
    }
    if (op->timeout_id) {
      g_source_remove (op->timeout_id);
    }
    if (op->loop && g_main_loop_is_running (op->loop))
      g_main_loop_quit (op->loop);
    g_signal_handler_disconnect (mount, umount_sig);
  }

  mp = g_mount_get_name(mount);
  if (unmounted)
    g_message ("Unmounted '%s'", mp);
  else {
    xfce_dialog_show_error (NULL, NULL, _("Failed to unmount '%s'. Drive cannot be used for burning."), mp);
    DBG ("Failed to unmount '%s'", mp);
  }

  g_free(mp);
  g_object_unref (mount);

  return unmounted;
}

#endif /* HAVE_GUDEV */
