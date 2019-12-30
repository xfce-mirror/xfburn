/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 * Copyright (c) 2008-2009 David Mohr <david@mcbf.net>
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
#include "xfburn-udev-manager.h"
#include "xfburn-utils.h"

#include "xfburn-device-list.h"
#include "xfburn-cclosure-marshal.h"

/*- private prototypes -*/

typedef struct _XfburnDeviceListPrivate XfburnDeviceListPrivate;

struct _XfburnDeviceListPrivate {
  GList *devices;
  gint num_drives;
  gint num_burners;
  XfburnDevice *curr_device;

#ifdef HAVE_GUDEV
  gulong volume_changed_handlerid;
#endif
};

void get_libburn_device_list (XfburnDeviceList *devlist);
static void cb_combo_device_changed (GtkComboBox *combo, XfburnDeviceList *devlist);
static void cb_refresh_clicked (GtkButton *button, XfburnDeviceList *devlist);
#ifdef HAVE_GUDEV
static void cb_volumes_changed (XfburnUdevManager *udevman, XfburnDeviceList *devlist);
#endif
static XfburnDevice * get_selected_device (GtkComboBox *combo_device);
static void refresh (XfburnDeviceList *devlist);

/*- globals -*/
static GObjectClass *parent_class = NULL;

enum {
  PROP_0,
  PROP_NUM_DRIVES,
  PROP_NUM_BURNERS,
  PROP_DEVICES,
  PROP_CURRENT_DEVICE,
};

enum {
  VOLUME_CHANGE_START,
  VOLUME_CHANGE_END,
  LAST_SIGNAL,
};

enum {
  DEVICE_NAME_COLUMN,
  DEVICE_POINTER_COLUMN,
  DEVICE_N_COLUMNS,
};

static guint signals[LAST_SIGNAL];


/*****************/
/*- class setup -*/
/*****************/

G_DEFINE_TYPE_WITH_PRIVATE (XfburnDeviceList, xfburn_device_list, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (xfburn_device_list_get_instance_private (XFBURN_DEVICE_LIST (o)))

static void
xfburn_device_list_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (XFBURN_DEVICE_LIST (object));

  switch (property_id) {
    case PROP_NUM_DRIVES:
      g_value_set_int (value, priv->num_drives);
      break;
    case PROP_NUM_BURNERS:
      g_value_set_int (value, priv->num_burners);
      break;
    case PROP_DEVICES:
      g_value_set_pointer (value, priv->devices);
      break;
    case PROP_CURRENT_DEVICE:
      g_value_set_object (value, priv->curr_device);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
xfburn_device_list_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (XFBURN_DEVICE_LIST (object));

  switch (property_id) {
    case PROP_NUM_DRIVES:
      /* FIXME: should this be there? */
      priv->num_drives = g_value_get_int (value);
      break;
    case PROP_NUM_BURNERS:
      /* FIXME: should this be there? */
      priv->num_burners = g_value_get_int (value);
      break;
    case PROP_DEVICES:
      /* FIXME: should this be there? */
      priv->devices = g_value_get_pointer (value);
      break;
    case PROP_CURRENT_DEVICE:
      priv->curr_device = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

/* implement a singleton pattern */
static GObject*
xfburn_device_list_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
  GObject *object;
  static XfburnDeviceList *global = NULL;

  if (!global) {
    object = G_OBJECT_CLASS (parent_class)->constructor (type, n_construct_params, construct_params);

    global = XFBURN_DEVICE_LIST (object);
  } else {
    object = g_object_ref (G_OBJECT (global));
  }

  return object;
}

static void
xfburn_device_list_finalize (GObject *object)
{
  XfburnDeviceList *devlist = XFBURN_DEVICE_LIST (object);
  XfburnDeviceListPrivate *priv = GET_PRIVATE (devlist);

  g_list_free_full (priv->devices, (GDestroyNotify) g_object_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfburn_device_list_class_init (XfburnDeviceListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->get_property = xfburn_device_list_get_property;
  object_class->set_property = xfburn_device_list_set_property;
  object_class->constructor  = xfburn_device_list_constructor;
  object_class->finalize     = xfburn_device_list_finalize;

  signals[VOLUME_CHANGE_START] = g_signal_new ("volume-change-start", XFBURN_TYPE_DEVICE_LIST, G_SIGNAL_ACTION,
                                          0,
                                          NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN,
                                          G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
  signals[VOLUME_CHANGE_END] = g_signal_new ("volume-change-end", XFBURN_TYPE_DEVICE_LIST, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnDeviceListClass, volume_changed),
                                          NULL, NULL, xfburn_cclosure_marshal_VOID__BOOLEAN_OBJECT,
                                          G_TYPE_NONE, 2, G_TYPE_BOOLEAN, XFBURN_TYPE_DEVICE);
    
  g_object_class_install_property (object_class, PROP_NUM_BURNERS, 
                                   g_param_spec_int ("num-burners", _("Number of burners in the system"),
                                                     _("Number of burners in the system"), 0, G_MAXINT, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_NUM_DRIVES, 
                                   g_param_spec_int ("num-drives", _("Number of drives in the system"),
                                                     _("Number of drives in the system (readers and writers)"), 0, G_MAXINT, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_DEVICES, 
                                   g_param_spec_pointer ("devices", _("List of devices"),
                                                         _("List of devices"), G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_CURRENT_DEVICE, 
                                   g_param_spec_object ("current-device", _("Currently selected device"),
                                                        _("Currently selected device"), XFBURN_TYPE_DEVICE, G_PARAM_READWRITE));

  //klass->device_changed = cb_device_changed;
  //klass->volume_changed = cb_volume_changed
}

static void
xfburn_device_list_init (XfburnDeviceList *self)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (self);

#ifdef HAVE_GUDEV
  XfburnUdevManager *udevman = xfburn_udev_manager_get_global ();
#endif

  DBG ("Constructing device list");
  xfburn_console_libburn_messages ();

#ifdef HAVE_GUDEV
  priv->devices = xfburn_udev_manager_get_devices (udevman, &priv->num_drives, &priv->num_burners);
  if (priv->num_drives > 0 && priv->num_burners < 1)
    g_warning ("There are %d drives in your system, but none are capable of burning", priv->num_drives);
  priv->volume_changed_handlerid = g_signal_connect (G_OBJECT (xfburn_udev_manager_get_global ()), "volume-changed", G_CALLBACK (cb_volumes_changed), self);
#else
  get_libburn_device_list (self);
#endif

  if (priv->num_burners > 0) {
    priv->curr_device = XFBURN_DEVICE (priv->devices->data);
    xfburn_device_refresh_info (priv->curr_device, TRUE);
  }
}


/***************/
/*- internals -*/
/***************/

void
get_libburn_device_list (XfburnDeviceList *devlist)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (devlist);

  struct burn_drive_info *drives;
  guint i;
  gint ret; 
  guint num_drives;

  DBG ("Before scanning for drives");
  while ((ret = burn_drive_scan (&drives, &(num_drives))) == 0)
    usleep (1002);
  DBG ("After scanning for drives");

  priv->num_drives = (gint) num_drives;

  if (ret < 0)
    g_warning ("An error occurred while scanning for available drives");

  if (priv->num_drives < 1) {
    g_warning ("No drives were found! If this is in error, check the permissions");
  }

  for (i = 0; i < priv->num_drives; i++) {
    XfburnDevice *device = xfburn_device_new ();
    const gchar *name;
    char addr[BURN_DRIVE_ADR_LEN];
    char rev[5];

    name = xfburn_device_set_name (device, drives[i].vendor, drives[i].product);
    strncpy (rev, drives[i].revision, 5);
    rev[5] = '\0';

    g_object_set (device, "revision", rev, NULL);

    xfburn_device_fillin_libburn_info (device, &drives[i]);

    
    ret = burn_drive_d_get_adr (drives[i].drive, addr);
    if (ret <= 0)
      g_error ("Unable to get drive %s address (ret=%d). Please report this problem to libburn-hackers@pykix.org", name, ret);
    //DBG ("device->addr = %s", device->addr);

    g_object_set (device, "address", addr, NULL);

    if (xfburn_device_can_burn (device)) {
      priv->devices = g_list_append (priv->devices, device);
      priv->num_burners++;
    } else {
      g_message ("Ignoring reader '%s' at '%s'", name, addr);
      g_object_unref (device);
    }
    burn_drive_snooze(drives[i].drive, 0);
  }

  burn_drive_info_free (drives);

  if (priv->num_drives > 0 && priv->num_burners < 1)
    g_warning ("There are %d drives in your system, but none are capable of burning", priv->num_drives);

  DBG ("Done");
}

static XfburnDevice *
get_selected_device (GtkComboBox *combo_device)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  XfburnDevice * device = NULL;
  gboolean ret;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_device));
  ret = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_device), &iter);
  if (ret)
    gtk_tree_model_get (model, &iter, DEVICE_POINTER_COLUMN, &device, -1);

  return device;
}

static void
cb_combo_device_changed (GtkComboBox *combo, XfburnDeviceList *devlist)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (devlist);
  XfburnDevice *device;

  g_signal_emit (G_OBJECT (devlist), signals[VOLUME_CHANGE_START], 0, TRUE);
  device = get_selected_device (combo);

  if (device != NULL) {
    priv->curr_device = device;
    xfburn_device_refresh_info (device, TRUE);
  }
  g_signal_emit (G_OBJECT (devlist), signals[VOLUME_CHANGE_END], 0, TRUE, device);
}

#ifdef HAVE_GUDEV
static void
cb_volumes_changed (XfburnUdevManager *udevman, XfburnDeviceList *devlist)
{
  DBG ("Udev volume changed");
  refresh (devlist);
}
#endif

static void
cb_refresh_clicked (GtkButton *button, XfburnDeviceList *devlist)
{
  refresh (devlist);
}

static void
refresh (XfburnDeviceList *devlist)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (devlist);

  g_signal_emit (G_OBJECT (devlist), signals[VOLUME_CHANGE_START], 0, FALSE);
  usleep (1000001);
  xfburn_device_refresh_info (priv->curr_device, TRUE);
  g_signal_emit (G_OBJECT (devlist), signals[VOLUME_CHANGE_END], 0, FALSE, priv->curr_device);
}




/**************/
/* public API */
/**************/


XfburnDevice *
xfburn_device_list_lookup_by_name (XfburnDeviceList *devlist, const gchar * name)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (devlist);
  GList *device;

  device = priv->devices;

  while (device) {
    XfburnDevice *device_data = XFBURN_DEVICE (device->data);
    gchar *devname;

    g_object_get (device, "name", &devname, NULL);

    if (g_ascii_strcasecmp (devname, name) == 0) {
      g_free (devname);
      return device_data;
    }

    g_free (devname);

    device = g_list_next (device);
  }

  return NULL;
}

GtkWidget *
xfburn_device_list_get_device_combo (XfburnDeviceList *devlist)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (devlist);

  GtkWidget *combo_device;
  GList *device = NULL;
  GtkListStore *store = NULL;
  GtkCellRenderer *cell;

  int i, selected=-1;

  store = gtk_list_store_new (DEVICE_N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
  combo_device = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_device), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_device), cell, "text", DEVICE_NAME_COLUMN, NULL);
  gtk_widget_show (combo_device);

  device = priv->devices;
  i = 0;
  while (device) {
    XfburnDevice *device_data = (XfburnDevice *) device->data;
    GtkTreeIter iter;
    const gchar *name;

    g_object_get (G_OBJECT (device_data), "name", &name, NULL);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, DEVICE_NAME_COLUMN, name, DEVICE_POINTER_COLUMN, device_data, -1);

    if (device_data == priv->curr_device)
        selected = i;

    device = g_list_next (device);
    i++;
  }
  /* FIXME: this might have to change once reading devices need to get selected as well */
  gtk_widget_set_sensitive (combo_device, priv->num_drives > 0);

  gtk_combo_box_set_active (GTK_COMBO_BOX (combo_device), selected);

  g_signal_connect (G_OBJECT (combo_device), "changed", G_CALLBACK (cb_combo_device_changed), devlist);

  return combo_device;
}

GtkWidget *
xfburn_device_list_get_refresh_button (XfburnDeviceList *devlist)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (devlist);
  GtkWidget *img, *button;

  img = gtk_image_new_from_icon_name ("view-refresh", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (img);
  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), img);
  gtk_widget_show (button);
  gtk_widget_set_sensitive (button, priv->num_burners > 0);

  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (cb_refresh_clicked), devlist);

  return button;
}

/*
gchar *
xfburn_device_list_get_selected (XfburnDeviceList *devlist)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (devlist);
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name = NULL;
  gboolean ret;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_device));
  ret = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo_device), &iter);
  if (ret)
    gtk_tree_model_get (model, &iter, DEVICE_NAME_COLUMN, &name, -1);

  return name;
}
*/

/* Note that the device is NOT ref'ed, so if the caller wants to hold on to a copy,
   g_object_ref () has to be called manually, or just use the property "current-device" */
XfburnDevice *
xfburn_device_list_get_current_device (XfburnDeviceList *devlist)
{
  XfburnDeviceListPrivate *priv = GET_PRIVATE (devlist);

  return priv->curr_device;
}


XfburnDeviceList*
xfburn_device_list_new (void)
{
  return g_object_new (XFBURN_TYPE_DEVICE_LIST, NULL);
}


