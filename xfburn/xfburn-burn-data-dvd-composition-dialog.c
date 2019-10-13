/*
 *  Copyright (c) 2007 Jean-François Wauthy (pollux@xfce.org)
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#include "xfburn-global.h"
#include "xfburn-utils.h"
#include "xfburn-settings.h"

#include "xfburn-device-box.h"
#include "xfburn-burn-data-dvd-composition-dialog.h"
#include "xfburn-progress-dialog.h"
#include "xfburn-burn-data-composition-base-dialog.h"

#define XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_BURN_DATA_DVD_COMPOSITION_DIALOG, XfburnBurnDataDvdCompositionDialogPrivate))

typedef struct
{
  gboolean dummy;
} XfburnBurnDataDvdCompositionDialogPrivate;

/* prototypes */
static void xfburn_burn_data_dvd_composition_dialog_class_init (XfburnBurnDataDvdCompositionDialogClass * klass, gpointer data);
static void xfburn_burn_data_dvd_composition_dialog_init (XfburnBurnDataDvdCompositionDialog * obj, gpointer data);
static void xfburn_burn_data_dvd_composition_dialog_finalize (GObject * object);

/* globals */
static XfceTitledDialogClass *parent_class = NULL;

GType
xfburn_burn_data_dvd_composition_dialog_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnBurnDataDvdCompositionDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_burn_data_dvd_composition_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnBurnDataDvdCompositionDialog),
      0,
      (GInstanceInitFunc) xfburn_burn_data_dvd_composition_dialog_init,
      NULL
    };

    type = g_type_register_static (XFBURN_TYPE_BURN_DATA_COMPOSITION_BASE_DIALOG, "XfburnBurnDataDvdCompositionDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_burn_data_dvd_composition_dialog_class_init (XfburnBurnDataDvdCompositionDialogClass * klass, gpointer data)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  g_type_class_add_private (klass, sizeof (XfburnBurnDataDvdCompositionDialogPrivate));
  
  object_class->finalize = xfburn_burn_data_dvd_composition_dialog_finalize;
}

static void
xfburn_burn_data_dvd_composition_dialog_init (XfburnBurnDataDvdCompositionDialog * obj, gpointer data)
{
  //XfburnBurnDataDvdCompositionDialogPrivate *priv = XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG_GET_PRIVATE (obj);
  
}

static void
xfburn_burn_data_dvd_composition_dialog_finalize (GObject * object)
{
  //XfburnBurnDataDvdCompositionDialogPrivate *priv = XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* internals */

/* public */
GtkWidget *
xfburn_burn_data_dvd_composition_dialog_new (IsoImage *image)
{
  XfburnBurnDataDvdCompositionDialog *obj;

  obj = XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG (g_object_new (XFBURN_TYPE_BURN_DATA_DVD_COMPOSITION_DIALOG, "image", image, NULL));
  
  return GTK_WIDGET (obj);
}
