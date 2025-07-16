/*
 * Copyright (c) 2006 Jean-François Wauthy (pollux@xfce.org)
 * Copyright (c) 2008-2009 David Mohr <david@mcbf.net>
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

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "xfburn-device-list.h"
#include "xfburn-device-box.h"
#include "xfburn-settings.h"
#include "xfburn-utils.h"
#include "xfburn-blank-dialog.h"
#include "xfburn-cclosure-marshal.h"

#define XFBURN_DEVICE_BOX_GET_PRIVATE(obj) (xfburn_device_box_get_instance_private (XFBURN_DEVICE_BOX (obj)))

enum {
  PROP_0,
  PROP_SHOW_WRITERS_ONLY,
  PROP_SHOW_SPEED_SELECTION,
  PROP_SHOW_MODE_SELECTION,
  PROP_VALID,
  PROP_BLANK_MODE,
};

enum {
  SPEED_TEXT_COLUMN,
  SPEED_VALUE_COLUMN,
  SPEED_N_COLUMNS,
};

enum {
  MODE_TEXT_COLUMN,
  MODE_VALUE_COLUMN,
  MODE_N_COLUMNS,
};

enum {
  VOLUME_CHANGED,
  LAST_SIGNAL
};

/* private struct */
typedef struct
{
  gboolean show_writers_only;
  gboolean show_speed_selection;
  gboolean show_mode_selection;
  gboolean valid_disc;
  gboolean blank_mode;


  GtkWidget *hbox_refresh;
  GtkWidget *disc_label;

  GtkWidget *hbox_speed_selection;
  GtkWidget *combo_speed;

  GtkWidget *status_label;
  gchar *status_text;

  GtkWidget *hbox_mode_selection;
  GtkWidget *combo_device;
  GtkWidget *combo_mode;

  gboolean have_asked_for_blanking;

  gulong handler_volume_change_start;
  gulong handler_volume_change_end;

  XfburnDeviceList *devlist;
} XfburnDeviceBoxPrivate;

/* prototypes */
static GObject * xfburn_device_box_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties);
static void xfburn_device_box_finalize (GObject * object);
static void xfburn_device_box_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfburn_device_box_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static gboolean ask_for_blanking (XfburnDeviceBoxPrivate *priv);
static void status_label_update (XfburnDeviceBoxPrivate *priv);
static gboolean check_disc_validity (XfburnDeviceBoxPrivate *priv);
static void refresh_drive_info (XfburnDeviceBox *box, XfburnDevice *device);

static void fill_combo_speed (XfburnDeviceBox *box, XfburnDevice *device);
static void fill_combo_mode (XfburnDeviceBox *box, XfburnDevice *device);

static void cb_volume_change_start (XfburnDeviceList *devlist, gboolean device_changed, XfburnDeviceBox *box);
static void cb_volume_change_end (XfburnDeviceList *devlist, gboolean device_changed, XfburnDevice *device, XfburnDeviceBox *box);

/* globals */
static GtkBoxClass *parent_class = NULL;

/*************************/
/* XfburnDeviceBox class */
/*************************/

static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_WITH_PRIVATE(XfburnDeviceBox, xfburn_device_box, GTK_TYPE_BOX);

static void
xfburn_device_box_class_init (XfburnDeviceBoxClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = xfburn_device_box_constructor;
  object_class->finalize     = xfburn_device_box_finalize;
  object_class->set_property = xfburn_device_box_set_property;
  object_class->get_property = xfburn_device_box_get_property;

  signals[VOLUME_CHANGED] = g_signal_new ("volume_changed", XFBURN_TYPE_DEVICE_BOX, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnDeviceBoxClass, volume_changed),
                                          NULL, NULL, xfburn_cclosure_marshal_VOID__BOOLEAN_OBJECT,
                                          G_TYPE_NONE, 2, G_TYPE_BOOLEAN, XFBURN_TYPE_DEVICE);

  g_object_class_install_property (object_class, PROP_SHOW_WRITERS_ONLY,
                                   g_param_spec_boolean ("show-writers-only", _("Show writers only"),
                                                        _("Show writers only"),
                                                        FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_SHOW_SPEED_SELECTION,
                                   g_param_spec_boolean ("show-speed-selection", _("Show speed selection"),
                                                        _("Show speed selection combo"),
                                                        FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_SHOW_MODE_SELECTION,
                                   g_param_spec_boolean ("show-mode-selection", _("Show mode selection"),
                                                        _("Show mode selection combo"),
                                                        FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_VALID,
                                   g_param_spec_boolean ("valid", _("Is it a valid combination"),
                                                        _("Is the combination of hardware and disc valid to burn the composition?"),
                                                        FALSE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_BLANK_MODE,
                                   g_param_spec_boolean ("blank-mode", _("Blank mode"),
                                                        _("The blank mode shows different disc status messages than regular mode"),
                                                        FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static GObject *
xfburn_device_box_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
  GObject *gobj = G_OBJECT_CLASS (parent_class)->constructor(type, n_construct_properties, construct_properties);
  XfburnDeviceBoxPrivate *priv = xfburn_device_box_get_instance_private(XFBURN_DEVICE_BOX(gobj));
  refresh_drive_info (XFBURN_DEVICE_BOX(gobj), xfburn_device_list_get_current_device (priv->devlist));
  return gobj;
}

static void 
xfburn_device_box_init (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = xfburn_device_box_get_instance_private(box);

  GtkWidget *label;
  GtkListStore *store = NULL;
  GtkCellRenderer *cell;
  XfburnDeviceList *devlist;
  gint n_burners;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (box), GTK_ORIENTATION_VERTICAL);

  priv->devlist = devlist = xfburn_device_list_new ();
  priv->handler_volume_change_start = g_signal_connect (G_OBJECT (devlist), "volume-change-start", G_CALLBACK (cb_volume_change_start), box);
  priv->handler_volume_change_end   = g_signal_connect (G_OBJECT (devlist), "volume-change-end", G_CALLBACK (cb_volume_change_end), box);

  /* devices */
  priv->combo_device = xfburn_device_list_get_device_combo (devlist);
  gtk_box_pack_start (GTK_BOX (box), priv->combo_device, FALSE, FALSE, BORDER);

  /* disc label */
  priv->hbox_refresh = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (priv->hbox_refresh);
  gtk_box_pack_start (GTK_BOX (box), priv->hbox_refresh, TRUE, TRUE, BORDER);

  priv->disc_label = gtk_label_new ("");
  gtk_widget_show (priv->disc_label);
  gtk_box_pack_start (GTK_BOX (priv->hbox_refresh), priv->disc_label, TRUE, TRUE, BORDER);

  /* refresh */
  gtk_box_pack_start (GTK_BOX (priv->hbox_refresh), xfburn_device_list_get_refresh_button (devlist), FALSE, FALSE, BORDER);

  /* speed */
  priv->hbox_speed_selection = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (priv->hbox_speed_selection);
  gtk_box_pack_start (GTK_BOX (box), priv->hbox_speed_selection, TRUE, TRUE, BORDER);

  label = gtk_label_new_with_mnemonic (_("_Speed:"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (priv->hbox_speed_selection), label, FALSE, FALSE, BORDER);

  store = gtk_list_store_new (SPEED_N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
  priv->combo_speed = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo_speed), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo_speed), cell, "text", SPEED_TEXT_COLUMN, NULL);
  gtk_widget_show (priv->combo_speed);
  gtk_box_pack_start (GTK_BOX (priv->hbox_speed_selection), priv->combo_speed, TRUE, TRUE, BORDER);
  if (priv->show_speed_selection) {
    gtk_widget_show (priv->hbox_speed_selection);
    fill_combo_speed (XFBURN_DEVICE_BOX (box), xfburn_device_list_get_current_device (devlist));
  }

  /* mode */
  priv->hbox_mode_selection = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (priv->hbox_mode_selection);
  gtk_box_pack_start (GTK_BOX (box), priv->hbox_mode_selection, FALSE, FALSE, BORDER);

  label = gtk_label_new_with_mnemonic (_("Write _mode:"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (priv->hbox_mode_selection), label, FALSE, FALSE, BORDER);

  store = gtk_list_store_new (MODE_N_COLUMNS, G_TYPE_STRING, G_TYPE_INT);
  priv->combo_mode = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo_mode), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo_mode), cell, "text", MODE_TEXT_COLUMN, NULL);
  gtk_widget_show (priv->combo_mode);
  gtk_box_pack_start (GTK_BOX (priv->hbox_mode_selection), priv->combo_mode, TRUE, TRUE, BORDER);
  g_object_get (G_OBJECT (devlist), "num-burners", &n_burners, NULL);
  gtk_widget_set_sensitive (priv->combo_mode, n_burners > 0);

  if (priv->show_mode_selection) {
    gtk_widget_show (priv->hbox_mode_selection);
    fill_combo_mode (XFBURN_DEVICE_BOX (box), xfburn_device_list_get_current_device (devlist));
  }

  /* status label */
  priv->status_label = gtk_label_new ("");
  priv->status_text = "";
  gtk_widget_show (priv->status_label);
  gtk_box_pack_start (GTK_BOX (box), priv->status_label, FALSE, FALSE, 0);

  priv->have_asked_for_blanking = FALSE;

}

static void
xfburn_device_box_finalize (GObject * object)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (object);

  g_signal_handler_disconnect (priv->devlist, priv->handler_volume_change_start);
  g_signal_handler_disconnect (priv->devlist, priv->handler_volume_change_end);

  g_object_unref (G_OBJECT (priv->devlist));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfburn_device_box_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (object);

  switch (prop_id) {
    case PROP_SHOW_WRITERS_ONLY:
      g_value_set_boolean (value, priv->show_writers_only);
      break;
    case PROP_SHOW_SPEED_SELECTION:
      g_value_set_boolean (value, priv->show_speed_selection);
      break;
    case PROP_SHOW_MODE_SELECTION:
      g_value_set_boolean (value, priv->show_mode_selection);
      break;
    case PROP_VALID:
      g_value_set_boolean (value, priv->valid_disc);
      break;
    case PROP_BLANK_MODE:
      g_value_set_boolean (value, priv->blank_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
xfburn_device_box_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (object);

  switch (prop_id) {
    case PROP_SHOW_WRITERS_ONLY:
      priv->show_writers_only = g_value_get_boolean (value);
      break;
    case PROP_SHOW_SPEED_SELECTION:
      priv->show_speed_selection = g_value_get_boolean (value);
      break;
    case PROP_SHOW_MODE_SELECTION:
      priv->show_mode_selection = g_value_get_boolean (value);
      break;
    case PROP_BLANK_MODE:
      priv->blank_mode = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/*************/
/* internals */
/*************/
static void
fill_combo_speed (XfburnDeviceBox *box, XfburnDevice *device)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_speed));
  GSList *el = NULL;
  int profile_no = 0;
  int factor;
  GtkTreeIter iter_max;

  gtk_list_store_clear (GTK_LIST_STORE (model));

  if (device == NULL)
    return;

  g_object_get (G_OBJECT (device),
                "profile-no", &profile_no,
                "supported-speeds", &el,
                NULL);

  if (el == NULL) {
    /* a valid disc is in the drive, but no speed list is present */
    GtkTreeIter iter;
    gchar *str;

    str = _("default");
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, SPEED_TEXT_COLUMN, str, SPEED_VALUE_COLUMN, -1, -1);
  }

  /* check profile, so we can convert from 'kb/s' into 'x' rating */
  if (profile_no != 0) {
    factor = xfburn_media_profile_to_kb (profile_no);
  } else {
    factor = 1;
  }

  while (el) {
    gint write_speed = GPOINTER_TO_INT (el->data);
    GtkTreeIter iter;
    gchar *str = NULL;
    gint speed;

    speed = write_speed / factor;
    str = g_strdup_printf ("%d", speed);

    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, SPEED_TEXT_COLUMN, str, SPEED_VALUE_COLUMN, write_speed, -1);
    g_free (str);

    el = g_slist_next (el);
  }

  gtk_list_store_append (GTK_LIST_STORE (model), &iter_max);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter_max, SPEED_TEXT_COLUMN, _("Max"), SPEED_VALUE_COLUMN, 0, -1);
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_speed), gtk_tree_model_iter_n_children (model, NULL) - 1);
}

static void
status_label_update (XfburnDeviceBoxPrivate *priv)
{
  gchar * text;
  gboolean sensitive;

  sensitive = gtk_widget_get_sensitive (priv->combo_device);

  if (sensitive)
    text = g_strdup_printf ("<span weight=\"bold\" foreground=\"darkred\" stretch=\"semiexpanded\">%s</span>", priv->status_text);
  else
    text = g_strdup_printf ("<span weight=\"bold\" foreground=\"gray\" stretch=\"semiexpanded\">%s</span>", priv->status_text);

  gtk_label_set_markup (GTK_LABEL(priv->status_label), text);
  g_free (text);
}


static gboolean
ask_for_blanking (XfburnDeviceBoxPrivate *priv)
{
  gboolean do_blank;

  if (priv->have_asked_for_blanking)
    return FALSE;

  priv->have_asked_for_blanking = TRUE;

  gdk_threads_enter ();
  do_blank = xfburn_ask_yes_no (GTK_MESSAGE_QUESTION, _("A full, but erasable disc is in the drive"),
                                         _("Do you want to blank the disc, so that it can be used for the upcoming burn process?"));

  if (do_blank) {
    GtkDialog *blank_dialog = GTK_DIALOG (xfburn_blank_dialog_new_eject (FALSE));
    gtk_dialog_run (blank_dialog);
    gtk_widget_destroy (GTK_WIDGET (blank_dialog));
  }
  gdk_threads_leave ();
  DBG ("done asking to blank");
  return FALSE;
}

static gboolean
check_disc_validity (XfburnDeviceBoxPrivate *priv)
{
  enum burn_disc_status disc_status = BURN_DISC_UNREADY;
  int profile_no = 0;
  gchar *profile_name = NULL;
  gboolean is_erasable = FALSE;
  XfburnDevice *device = xfburn_device_list_get_current_device (priv->devlist);

  if (device == NULL) {
    gtk_widget_set_sensitive (priv->combo_speed, FALSE);
    priv->valid_disc = FALSE;
    return priv->valid_disc;
  }

  g_object_get (G_OBJECT (device), "disc-status", &disc_status, "profile-no", &profile_no,
                                   "erasable", &is_erasable, "profile-name", &profile_name,
                                   NULL);
  if (profile_name != NULL) {
    gtk_label_set_text (GTK_LABEL (priv->disc_label), profile_name);
    g_free (profile_name);
  }

  DBG ("blank_mode = %d", priv->blank_mode);

  if (!priv->blank_mode) {
    /* for burning */
    switch (profile_no) {
      case XFBURN_PROFILE_NONE:
        /* Very old CD burner (MMC-1) or unsuitable medium.
         * An unsuitable medium will later be marked invalid because it is not
         * BURN_DISC_BLANK. So for now it is a valid disc on probation.
         */
      case XFBURN_PROFILE_CDR:
      case XFBURN_PROFILE_CDRW:
      case XFBURN_PROFILE_DVDRAM:
      case XFBURN_PROFILE_DVD_MINUS_R:
      case XFBURN_PROFILE_DVD_MINUS_RW_OVERWRITE:
      case XFBURN_PROFILE_DVD_MINUS_RW_SEQUENTIAL:
      case XFBURN_PROFILE_DVD_MINUS_R_DL:
      case XFBURN_PROFILE_DVD_PLUS_R:
      case XFBURN_PROFILE_DVD_PLUS_R_DL:
      case XFBURN_PROFILE_DVD_PLUS_RW:
      case XFBURN_PROFILE_BD_R:
      case XFBURN_PROFILE_BD_RE:
        priv->valid_disc = TRUE;
        break;
      case XFBURN_PROFILE_CDROM:
      case XFBURN_PROFILE_DVDROM:
      case XFBURN_PROFILE_BDROM:
        g_warning ("Read-only profile 0x%x!", profile_no);
        priv->valid_disc = FALSE;
        break;
      default:
        g_warning ("Unknown disc profile 0x%x!", profile_no);
        priv->valid_disc = TRUE;
        break;
    }

    if (!priv->valid_disc) {
        priv->status_text = _("Drive can't burn on the inserted disc");
    } else {
      priv->valid_disc = (disc_status == BURN_DISC_BLANK);
      if (!priv->valid_disc) {
        switch (disc_status) {
          case BURN_DISC_EMPTY:
            priv->status_text = _("Drive is empty");
            break;
          case BURN_DISC_APPENDABLE:
            priv->status_text = _("Sorry, multisession is not yet supported");
            break;
          case BURN_DISC_FULL:
            priv->status_text = _("Inserted disc is full");
            break;
          case BURN_DISC_UNSUITABLE:
            priv->status_text = _("Inserted disc is unsuitable");
            break;
          case BURN_DISC_UNGRABBED:
            priv->status_text = _("Cannot access drive (it might be in use)");
            break;
          default:
            /* if there is no detected device, then don't print an error message as it is expected to not have a disc status */
            if (device != NULL) {
              priv->status_text = _("Error determining disc");
              DBG ("weird disc_status = %d", disc_status);
            }
        }
      }
    }

    if (!priv->valid_disc && disc_status == BURN_DISC_FULL && is_erasable) {
      g_idle_add ((GSourceFunc) ask_for_blanking, priv);
    }
  }
  else { /* priv->blank_mode == TRUE */
    priv->valid_disc = is_erasable && (disc_status != BURN_DISC_BLANK);

    if (!priv->valid_disc) {
      switch (profile_no) {
        case XFBURN_PROFILE_CDR:
        case XFBURN_PROFILE_DVD_MINUS_R:
        case XFBURN_PROFILE_DVD_MINUS_R_DL:
        case XFBURN_PROFILE_DVD_PLUS_R:
        case XFBURN_PROFILE_DVD_PLUS_R_DL:
          priv->status_text = _("Write-once disc, no blanking possible");
          break;
        case XFBURN_PROFILE_DVD_PLUS_RW:
          priv->status_text = _("DVD+RW does not need blanking");
          break;
        default:
          switch (disc_status) {
            case BURN_DISC_EMPTY:
              priv->status_text = _("Drive is empty");
              break;
            case BURN_DISC_BLANK:
              priv->status_text = _("Inserted disc is already blank");
              break;
            case BURN_DISC_UNSUITABLE:
              priv->status_text = _("Inserted disc is unsuitable");
              break;
            case BURN_DISC_UNGRABBED:
              priv->status_text = _("Cannot access drive (it might be in use)");
              break;
            default:
              priv->status_text = _("Error determining disc");
              DBG ("weird disc_status = %d", disc_status);
          }
      }
    }
  }

  gtk_widget_set_sensitive (priv->combo_speed, priv->valid_disc);

  if (priv->valid_disc)
    priv->status_text = "";

  status_label_update (priv);
  return priv->valid_disc;
}

static void
fill_combo_mode (XfburnDeviceBox *box, XfburnDevice *device)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_mode));
  GtkTreeIter iter;
  gint block_types = 0;

  gtk_list_store_clear (GTK_LIST_STORE (model));

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, MODE_TEXT_COLUMN, _("Automatic"), MODE_VALUE_COLUMN, WRITE_MODE_AUTO, -1);

  if (device)
    g_object_get (G_OBJECT (device), "tao-block-types", &block_types, NULL);

  if (block_types) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, MODE_TEXT_COLUMN, _("With Track Gaps / Track-At-Once (TAO)"), MODE_VALUE_COLUMN, WRITE_MODE_TAO, -1);
  }

  if (device)
    g_object_get (G_OBJECT (device), "sao-block-types", &block_types, NULL);
  if (block_types & BURN_BLOCK_SAO) {
    gtk_list_store_append (GTK_LIST_STORE (model), &iter);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter, MODE_TEXT_COLUMN, _("Multi Session / Session-At-Once (SAO)"), MODE_VALUE_COLUMN, WRITE_MODE_SAO, -1);
  }

  gtk_widget_set_sensitive (priv->combo_mode, device != NULL);
  gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_mode), 0);
}

static void
cb_volume_change_start (XfburnDeviceList *devlist, gboolean device_changed, XfburnDeviceBox *box)
{
  if (gtk_widget_get_realized (GTK_WIDGET(box)))
    xfburn_busy_cursor (GTK_WIDGET (box));
}

static void
cb_volume_change_end (XfburnDeviceList *devlist, gboolean device_changed, XfburnDevice *device, XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);

  g_return_if_fail (XFBURN_IS_DEVICE_LIST (devlist));
  g_return_if_fail (XFBURN_IS_DEVICE_BOX (box));

  xfburn_device_list_refresh_device_combo (devlist, priv->combo_device);
  refresh_drive_info (box, device);

  if (gtk_widget_get_realized (GTK_WIDGET (box)))
    xfburn_default_cursor (GTK_WIDGET (box));

  g_signal_emit (box, signals[VOLUME_CHANGED], 0, device_changed, device);
}

static void
refresh_drive_info (XfburnDeviceBox *box, XfburnDevice *device)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);

  if (priv->show_speed_selection)
    fill_combo_speed (box, device);

  if (priv->show_mode_selection)
    fill_combo_mode (box, device);

  /* FIXME: where to put this? */
  check_disc_validity (priv);
}

/******************/
/* public methods */
/******************/
GtkWidget *
xfburn_device_box_new (XfburnDeviceBoxFlags flags)
{
  GtkWidget *obj;

  obj = g_object_new (xfburn_device_box_get_type (),
                      "show-writers-only", ((flags & SHOW_CD_WRITERS) != 0),
                      "show-speed-selection", ((flags & SHOW_SPEED_SELECTION) != 0),
                      "show-mode-selection", ((flags & SHOW_MODE_SELECTION) != 0),
                      "blank-mode", ((flags & BLANK_MODE) != 0),
                      NULL);

  return obj;
}

gint
xfburn_device_box_get_speed (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);

  GtkTreeModel *model;
  GtkTreeIter iter;
  gint speed = -1;
  gboolean ret;

  g_return_val_if_fail (priv->show_speed_selection, -1);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_speed));
  ret = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo_speed), &iter);
  if (ret)
    gtk_tree_model_get (model, &iter, SPEED_VALUE_COLUMN, &speed, -1);

  return speed;
}

void xfburn_device_box_set_sensitive (XfburnDeviceBox *box, gboolean sensitivity)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);

  /* why do we need to explicitly set this? It gets grayed out even
   * without this call! */
  gtk_widget_set_sensitive (priv->combo_device, sensitivity);
  status_label_update (priv);
}

XfburnWriteMode
xfburn_device_box_get_mode (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);

  GtkTreeModel *model;
  GtkTreeIter iter;
  gint mode = -1;
  gboolean ret;

  g_return_val_if_fail (priv->show_mode_selection, -1);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo_mode));
  ret = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo_mode), &iter);
  if (ret)
    gtk_tree_model_get (model, &iter, SPEED_VALUE_COLUMN, &mode, -1);

  return mode;
}


XfburnDevice *
xfburn_device_box_get_selected_device (XfburnDeviceBox *box)
{
  XfburnDeviceBoxPrivate *priv = XFBURN_DEVICE_BOX_GET_PRIVATE (box);

  return xfburn_device_list_get_current_device (priv->devlist);
}
