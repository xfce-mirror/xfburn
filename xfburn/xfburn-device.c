/*
 *  Copyright (C) 2009 David Mohr <david@mcbf.net>
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
 *  
 */

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <libxfce4util/libxfce4util.h>
#include <unistd.h>

#include "xfburn-device.h"
#include "xfburn-udev-manager.h"

/*- globals -*/

enum {
  PROP_0,
  PROP_NAME,
  PROP_ADDRESS,
  PROP_REVISION,
  PROP_ACCESSIBLE,
  PROP_SUPPORTED_SPEEDS,
  PROP_DISC_STATUS,
  PROP_PROFILE_NO,
  PROP_PROFILE_NAME,
  PROP_ERASABLE,
  PROP_CDR,
  PROP_CDRW,
  PROP_DVDR,
  PROP_DVDPLUSR,
  PROP_DVDRAM,
  PROP_BD,
  PROP_TAO_BLOCK_TYPES,
  PROP_SAO_BLOCK_TYPES,
  PROP_RAW_BLOCK_TYPES,
  PROP_PACKET_BLOCK_TYPES,
};

/*- private prototypes -*/

#define CAN_BURN(priv) ((priv)->cdr || (priv)->cdrw || (priv)->dvdr || (priv)->dvdram)
#define DEVICE_INFO_PRINTF(device) "%s can burn: %d [cdr: %d, cdrw: %d, dvdr: %d, dvdram: %d]", (device)->name, CAN_BURN (device), (device)->cdr, (device)->cdrw, (device)->dvdr, (device)->dvdram


/*****************/
/*- class setup -*/
/*****************/
typedef struct _XfburnDevicePrivate XfburnDevicePrivate;

struct _XfburnDevicePrivate {
  gchar *name;
  gchar *addr;
  gchar *rev;
  gboolean details_known;

  gint buffer_size;
  gboolean dummy_write;
  
  gboolean cdr;
  gboolean cdrw;
  gboolean dvdr;
  gboolean dvdplusr;
  gboolean dvdram;
  gboolean bd;

  GSList *supported_speeds;

  gint tao_block_types;
  gint sao_block_types;
  gint raw_block_types;
  gint packet_block_types;

  enum burn_disc_status disc_status;
  int profile_no;
  char profile_name[80];
  int is_erasable;
};

G_DEFINE_TYPE_WITH_PRIVATE (XfburnDevice, xfburn_device, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (xfburn_device_get_instance_private (o))

static void
xfburn_device_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  XfburnDevicePrivate *priv = GET_PRIVATE (XFBURN_DEVICE (object));

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_ADDRESS:
      g_value_set_string (value, priv->addr);
      break;
    case PROP_REVISION:
      g_value_set_string (value, priv->rev);
      break;
    case PROP_SUPPORTED_SPEEDS:
      g_value_set_pointer (value, priv->supported_speeds);
      break;
    case PROP_DISC_STATUS:
      g_value_set_int (value, priv->disc_status);
      break;
    case PROP_PROFILE_NO:
      g_value_set_int (value, priv->profile_no);
      break;
    case PROP_PROFILE_NAME:
      g_value_set_string (value, priv->profile_name);
      break;
    case PROP_ERASABLE:
      g_value_set_boolean (value, priv->is_erasable);
      break;
    case PROP_CDR:
      g_value_set_boolean (value, priv->cdr);
      break;
    case PROP_CDRW:
      g_value_set_boolean (value, priv->cdrw);
      break;
    case PROP_DVDR:
      g_value_set_boolean (value, priv->dvdr);
      break;
    case PROP_DVDPLUSR:
      g_value_set_boolean (value, priv->dvdplusr);
      break;
    case PROP_DVDRAM:
      g_value_set_boolean (value, priv->dvdram);
      break;
    case PROP_BD:
      g_value_set_boolean (value, priv->bd);
      break;
    case PROP_TAO_BLOCK_TYPES:
      g_value_set_int (value, priv->tao_block_types);
      break;
    case PROP_SAO_BLOCK_TYPES:
      g_value_set_int (value, priv->sao_block_types);
      break;
    case PROP_RAW_BLOCK_TYPES:
      g_value_set_int (value, priv->raw_block_types);
      break;
    case PROP_PACKET_BLOCK_TYPES:
      g_value_set_int (value, priv->packet_block_types);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
xfburn_device_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  XfburnDevicePrivate *priv = GET_PRIVATE (XFBURN_DEVICE (object));

  switch (property_id) {
    case PROP_NAME:
      priv->name = g_value_dup_string (value);
      break;
    case PROP_ADDRESS:
      priv->addr = g_value_dup_string (value);
      break;
    case PROP_REVISION:
      priv->rev = g_value_dup_string (value);
      break;
    case PROP_SUPPORTED_SPEEDS:
      priv->supported_speeds = g_value_get_pointer (value);
      break;
    case PROP_DISC_STATUS:
      priv->disc_status = g_value_get_int (value);
      break;
    case PROP_PROFILE_NO:
      priv->profile_no = g_value_get_int (value);
      break;
    case PROP_PROFILE_NAME:
      strncpy (priv->profile_name, g_value_get_string(value), 80);
      break;
    case PROP_ERASABLE:
      priv->is_erasable = g_value_get_boolean (value);
      break;
    case PROP_CDR:
      priv->cdr = g_value_get_boolean (value);
      break;
    case PROP_CDRW:
      priv->cdrw = g_value_get_boolean (value);
      break;
    case PROP_DVDR:
      priv->dvdr = g_value_get_boolean (value);
      break;
    case PROP_DVDPLUSR:
      priv->dvdplusr = g_value_get_boolean (value);
      break;
    case PROP_DVDRAM:
      priv->dvdram = g_value_get_boolean (value);
      break;
    case PROP_BD:
      priv->bd = g_value_get_boolean (value);
      break;
    case PROP_TAO_BLOCK_TYPES:
      priv->tao_block_types = g_value_get_int (value);
      break;
    case PROP_SAO_BLOCK_TYPES:
      priv->sao_block_types = g_value_get_int (value);
      break;
    case PROP_RAW_BLOCK_TYPES:
      priv->raw_block_types = g_value_get_int (value);
      break;
    case PROP_PACKET_BLOCK_TYPES:
      priv->packet_block_types = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
xfburn_device_finalize (GObject *object)
{
  XfburnDevice *dev = XFBURN_DEVICE (object);
  XfburnDevicePrivate *priv = GET_PRIVATE (dev);

  g_free (priv->name);

  g_slist_free (priv->supported_speeds);

  G_OBJECT_CLASS (xfburn_device_parent_class)->finalize (object);
}

static void
xfburn_device_class_init (XfburnDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = xfburn_device_get_property;
  object_class->set_property = xfburn_device_set_property;
  object_class->finalize = xfburn_device_finalize;

  g_object_class_install_property (object_class, PROP_NAME, 
                                   g_param_spec_string ("name", _("Display name"),
                                                        _("Display name"), NULL, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_ADDRESS, 
                                   g_param_spec_string ("address", _("Device address"),
                                                        _("Device address"), "", G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_REVISION, 
                                   g_param_spec_string ("revision", _("Device revision"),
                                                        _("Device Revision"), "", G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SUPPORTED_SPEEDS, 
                                   g_param_spec_pointer ("supported-speeds", _("Burn speeds supported by the device"),
                                                        _("Burn speeds supported by the device"), G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_DISC_STATUS, 
                                   g_param_spec_int ("disc-status", _("Disc status"),
                                                     _("Disc status"), 0, 6, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_PROFILE_NO, 
                                   g_param_spec_int ("profile-no", _("Profile no. as reported by libburn"),
                                                     _("Profile no. as reported by libburn"), 0, 0xffff, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_PROFILE_NAME, 
                                   g_param_spec_string ("profile-name", _("Profile name as reported by libburn"),
                                                        _("Profile name as reported by libburn"), "", G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_ERASABLE, 
                                   g_param_spec_boolean ("erasable", _("Is the disc erasable"),
                                                        _("Is the disc erasable"), FALSE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_CDR, 
                                   g_param_spec_boolean ("cdr", _("Can burn CDR"),
                                                        _("Can burn CDR"), FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_CDRW, 
                                   g_param_spec_boolean ("cdrw", _("Can burn CDRW"),
                                                        _("Can burn CDRW"), FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DVDR, 
                                   g_param_spec_boolean ("dvdr", _("Can burn DVDR"),
                                                        _("Can burn DVDR"), FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DVDPLUSR, 
                                   g_param_spec_boolean ("dvdplusr", _("Can burn DVDPLUSR"),
                                                        _("Can burn DVDPLUSR"), FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DVDRAM, 
                                   g_param_spec_boolean ("dvdram", _("Can burn DVDRAM"),
                                                        _("Can burn DVDRAM"), FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_BD,
                                   g_param_spec_boolean ("bd", _("Can burn Blu-ray"),
                                                        _("Can burn Blu-ray"), FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_TAO_BLOCK_TYPES, 
                                   g_param_spec_int ("tao-block-types", _("libburn TAO block types"),
                                                     _("libburn TAO block types"), 0, G_MAXINT, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_SAO_BLOCK_TYPES, 
                                   g_param_spec_int ("sao-block-types", _("libburn SAO block types"),
                                                     _("libburn SAO block types"), 0, G_MAXINT, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_RAW_BLOCK_TYPES, 
                                   g_param_spec_int ("raw-block-types", _("libburn RAW block types"),
                                                     _("libburn RAW block types"), 0, G_MAXINT, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_PACKET_BLOCK_TYPES, 
                                   g_param_spec_int ("packet-block-types", _("libburn PACKET block types"),
                                                     _("libburn PACKET block types"), 0, G_MAXINT, 0, G_PARAM_READABLE));
}

static void
xfburn_device_init (XfburnDevice *self)
{
  /* FIXME: initialize profile_name, or is that handled by the property? */
}


/***************/
/*- internals -*/
/***************/

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
  XfburnDevicePrivate *priv = GET_PRIVATE (device);
  gint ret;

  /* check if there is a disc in the drive */
  while ((priv->disc_status = burn_disc_get_status (drive_info->drive)) == BURN_DISC_UNREADY)
    usleep(100001);

  DBG ("disc_status = %d", priv->disc_status);

  if ((ret = burn_disc_get_profile(drive_info->drive, &(priv->profile_no), priv->profile_name)) != 1) {
    g_warning ("no profile could be retrieved");
  }
  priv->is_erasable = burn_disc_erasable (drive_info->drive);
  DBG ("profile_no = 0x%x (%s), %s erasable", priv->profile_no, priv->profile_name, (priv->is_erasable ? "" : "NOT"));

#if 0 /* this doesn't seem to work */
  if (burn_disc_read_atip (drive_info->drive) == 1) {
    int start, end;

    if (burn_drive_get_start_end_lba (drive_info->drive, &start, &end, 0) == 1) {
      DBG ("lba start = %d, end = %d", start, end);
    } else
      DBG ("Getting start/end lba failed");
  } else
    DBG ("Reading ATIP failed");
#endif
}

static void
refresh_speed_list (XfburnDevice * device, struct burn_drive_info *drive_info)
{
  XfburnDevicePrivate *priv = GET_PRIVATE (device);

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
      if (speed > 0 && no_speed_duplicate (priv->supported_speeds, speed)) {
          priv->supported_speeds = g_slist_prepend (priv->supported_speeds, GINT_TO_POINTER (speed));
      } 

      el = el->next;
    }

    burn_drive_free_speedlist (&speed_list); 
    priv->supported_speeds = g_slist_sort (priv->supported_speeds, &cmp_ints);
  } else if (ret == 0 || speed_list == NULL) {
    g_warning ("reported speed list is empty for device:");
    // FIXME
    //g_warning (DEVICE_INFO_PRINTF (priv));
    g_warning ("%s can burn: %d [cdr: %d, cdrw: %d, dvdr: %d, dvdram: %d]", (priv)->name, ((priv)->cdr || (priv)->cdrw || (priv)->dvdr || (priv)->dvdram), (priv)->cdr, (priv)->cdrw, (priv)->dvdr, (priv)->dvdram);
  } else {
    /* ret < 0 */
    g_error ("severe error while retrieving speed list");
  }
}

/*******************/
/*- public methods-*/
/*******************/

void
xfburn_device_fillin_libburn_info (XfburnDevice *device, struct burn_drive_info *drive)
{
  XfburnDevicePrivate *priv = GET_PRIVATE (device);

  priv->details_known = TRUE;

  priv->cdr = drive->write_cdr;
  priv->cdrw = drive->write_cdrw;

  priv->dvdr = drive->write_dvdr;
  priv->dvdram = drive->write_dvdram;
  
  priv->buffer_size = drive->buffer_size;
  priv->dummy_write = drive->write_simulate;

  DBG ("libburn will determine BD support based on the disk in the drive");

  /* write modes */
  priv->tao_block_types = drive->tao_block_types;
  priv->sao_block_types = drive->sao_block_types;
  priv->raw_block_types = drive->raw_block_types;
  priv->packet_block_types = drive->packet_block_types;

  DBG (DEVICE_INFO_PRINTF (priv));
}

gboolean
xfburn_device_grab (XfburnDevice * device, struct burn_drive_info **drive_info)
{
  XfburnDevicePrivate *priv = GET_PRIVATE (device);
  int ret = 0;
  gchar drive_addr[BURN_DRIVE_ADR_LEN];
  int i;
  const int max_checks = 4;
#ifdef HAVE_GUDEV
  XfburnUdevManager *udevman = xfburn_udev_manager_get_global ();
#endif

  ret = burn_drive_convert_fs_adr (priv->addr, drive_addr);
  if (ret <= 0) {
    g_error ("Device address does not lead to a burner '%s' (ret=%d).", priv->addr, ret);
    return FALSE;
  }

  /* we need to try to grab several times, because
   * the drive might be busy detecting the disc */
  for (i=1; i<=max_checks; i++) {
    ret = burn_drive_scan_and_grab (drive_info, drive_addr, 0);
    //DBG ("grab (%s)-> %d", drive_addr, ret);
    if (ret > 0)
      break;
    else if  (i < max_checks) {
#ifdef HAVE_GUDEV
      if (!xfburn_udev_manager_check_ask_umount (udevman, device))
        usleep(i*100001);
#else
      usleep(i*100001);
#endif
    }
  }

  if (ret <= 0) {
    g_warning ("Unable to grab the drive at path '%s' (ret=%d).", priv->addr, ret);
    return FALSE;
  }

  return TRUE;
}

gboolean
xfburn_device_refresh_info (XfburnDevice * device, gboolean get_speed_info)
{
  XfburnDevicePrivate *priv = GET_PRIVATE (device);

  struct burn_drive_info *drive_info = NULL;
  gboolean ret;

  if (G_UNLIKELY (device == NULL)) {
    g_warning ("Hmm, why can we refresh when there is no drive?");
    return FALSE;
  }

  /* reset other internal structures */
  priv->profile_no = 0;
  *(priv->profile_name) = '\0';
  priv->is_erasable = 0;

  /* empty previous speed list */
  g_slist_free (priv->supported_speeds);
  priv->supported_speeds = NULL;

  if (!xfburn_device_grab (device, &drive_info)) {
    ret = FALSE;
    g_warning ("Couldn't grab drive in order to update speed list.");
    priv->disc_status = BURN_DISC_UNGRABBED;
  } else {
    if (!priv->details_known)
      xfburn_device_fillin_libburn_info (device, drive_info);

    ret = TRUE;
    refresh_disc (device, drive_info);
    if (get_speed_info)
      refresh_speed_list (device, drive_info);

    xfburn_device_release (drive_info, 0);
  }

  return ret;
}

gboolean 
xfburn_device_release (struct burn_drive_info *drive_info, gboolean eject)
{
  int ret;

  burn_drive_snooze (drive_info->drive, 0);
  burn_drive_release (drive_info->drive, eject);

  ret = burn_drive_info_forget (drive_info, 0);

  if (G_LIKELY (ret == 1))
    return TRUE;
  else if (ret == 2) {
    DBG ("drive_info already forgotten");
    return TRUE;
  } else if (ret == 0) {
    DBG ("could not forget drive_info");
    return FALSE;
  } else if (ret < 0) {
    DBG ("some other failure while forgetting drive_info");
    return FALSE;
  }

  /* we should never reach this */
  return FALSE;
}

const gchar *
xfburn_device_set_name (XfburnDevice *device, const gchar *vendor, const gchar *product)
{
  XfburnDevicePrivate *priv = GET_PRIVATE (device);

  priv->name = g_strconcat (vendor, " ", product, NULL);

  return priv->name;
}

gboolean
xfburn_device_can_burn (XfburnDevice *device)
{
  XfburnDevicePrivate *priv = GET_PRIVATE (device);

  return CAN_BURN (priv);
}

gboolean 
xfburn_device_can_dummy_write (XfburnDevice *device)
{
  XfburnDevicePrivate *priv = GET_PRIVATE (device);

  return priv->dummy_write;
}

XfburnDevice*
xfburn_device_new (void)
{
  return g_object_new (XFBURN_TYPE_DEVICE, NULL);
}


