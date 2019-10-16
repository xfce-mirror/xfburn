/*
 *  Copyright (c) 2005-2007 Jean-François Wauthy (pollux@xfce.org)
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

#include "xfburn-burn-data-composition-base-dialog.h"
#include "xfburn-burn-data-cd-composition-dialog.h"
#include "xfburn-progress-dialog.h"

typedef struct
{
  gboolean dummy; /* An empty private struct is not allowed */
} XfburnBurnDataCdCompositionDialogPrivate;

/* prototypes */
// static void xfburn_burn_data_cd_composition_dialog_class_init (XfburnBurnDataCdCompositionDialogClass * klass, gpointer data);
// static void xfburn_burn_data_cd_composition_dialog_init (XfburnBurnDataCdCompositionDialog * obj, gpointer data);
static void xfburn_burn_data_cd_composition_dialog_finalize (GObject * object);
G_DEFINE_TYPE_WITH_PRIVATE(XfburnBurnDataCdCompositionDialog, xfburn_burn_data_cd_composition_dialog, XFBURN_TYPE_BURN_DATA_COMPOSITION_BASE_DIALOG);

/* globals */
static XfceTitledDialogClass *parent_class = NULL;

static void
xfburn_burn_data_cd_composition_dialog_class_init (XfburnBurnDataCdCompositionDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  
  object_class->finalize = xfburn_burn_data_cd_composition_dialog_finalize;
}

static void
xfburn_burn_data_cd_composition_dialog_init (XfburnBurnDataCdCompositionDialog * obj)
{  
}

static void
xfburn_burn_data_cd_composition_dialog_finalize (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* internals */
/* empty for now */

/* public */
GtkWidget *
xfburn_burn_data_cd_composition_dialog_new (IsoImage *image, gboolean has_default_name)
{
  XfburnBurnDataCdCompositionDialog *obj;

  obj = XFBURN_BURN_DATA_CD_COMPOSITION_DIALOG (g_object_new (XFBURN_TYPE_BURN_DATA_CD_COMPOSITION_DIALOG, "image", image, 
                                                                                                           "show-volume-name", has_default_name,
                                                                                                           NULL));
  
  return GTK_WIDGET (obj);
}
