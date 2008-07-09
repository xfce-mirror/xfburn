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

#include "xfburn-global.h"
#include "xfburn-progress-dialog.h"

#include "xfburn-hal-manager.h"

static void xfburn_hal_manager_class_init (XfburnHalManagerClass * klass);
static void xfburn_hal_manager_init (XfburnHalManager * sp);
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
    }
    DBG ("Connection to dbus or hal failed!");
  } else {
    libhal_ctx_set_device_added (hal_context, cb_device_added);
    libhal_ctx_set_device_removed (hal_context, cb_device_removed);
    libhal_ctx_set_device_property_modified (hal_context, cb_prop_modified);
  }

  dbus_error_free (&derror);

  priv->hal_context = hal_context;
  priv->dbus_connection = dbus_connection;
}

static void
xfburn_hal_manager_finalize (GObject * object)
{
  XfburnHalManagerPrivate *priv = XFBURN_HAL_MANAGER_GET_PRIVATE (object);

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

/*        */
/* public */
/*        */

GObject *
xfburn_hal_manager_new ()
{
  if (G_UNLIKELY (halman != NULL))
    g_error ("Trying to create a second instance of hal manager!");
  return g_object_new (XFBURN_TYPE_HAL_MANAGER, NULL);
}

void
xfburn_hal_manager_create_global ()
{
  halman = XFBURN_HAL_MANAGER (xfburn_hal_manager_new ());
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

#endif /* HAVE_HAL */
