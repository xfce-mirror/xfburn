/* $Id$ */
/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#include <libxfce4util/libxfce4util.h>

#include "xfburn-global.h"
#include "xfburn-progress-dialog.h"

#include "xfburn-create-iso-from-composition-progress-dialog.h"

static void
xfburn_create_iso_from_composition_progress_dialog_class_init (XfburnCreateIsoFromCompositionProgressDialogClass * klass);
static void xfburn_create_iso_from_composition_progress_dialog_init (XfburnCreateIsoFromCompositionProgressDialog * sp);

static void cb_new_output (XfburnCreateIsoFromCompositionProgressDialog * dialog, const gchar * output, gpointer data);

/**********************/
/* class declaration */
/**********************/
static XfburnProgressDialogClass *parent_class = NULL;

GtkType
xfburn_create_iso_from_composition_progress_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnCreateIsoFromCompositionProgressDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_create_iso_from_composition_progress_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnCreateIsoFromCompositionProgressDialog),
      0,
      (GInstanceInitFunc) xfburn_create_iso_from_composition_progress_dialog_init,
    };

    type = g_type_register_static (XFBURN_TYPE_PROGRESS_DIALOG, "XfburnCreateIsoFromCompositionProgressDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_create_iso_from_composition_progress_dialog_class_init (XfburnCreateIsoFromCompositionProgressDialogClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_create_iso_from_composition_progress_dialog_init (XfburnCreateIsoFromCompositionProgressDialog * obj)
{
  g_signal_connect_after (G_OBJECT (obj), "output", G_CALLBACK (cb_new_output), NULL);
}

/*           */
/* internals */
/*           */
static void
cb_new_output (XfburnCreateIsoFromCompositionProgressDialog * dialog, const gchar * output, gpointer data)
{
  if (strstr (output, MKISOFS_DONE)) {
    xfburn_progress_dialog_set_status (XFBURN_PROGRESS_DIALOG (dialog), XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED);
  }
  else if (strstr (output, MKISOFS_RUNNING)) {
    gint percent, percent_fraction;
    
    if (sscanf (output, "%d.%d", &percent, &percent_fraction) == 2) {
      gdouble fraction;
      
      fraction = (percent + (percent_fraction * 0.01)) / 100;
      xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), fraction);
    }
  }
}

/*        */
/* public */
/*        */

GtkWidget *
xfburn_create_iso_from_composition_progress_dialog_new ()
{
  XfburnCreateIsoFromCompositionProgressDialog *obj;

  obj =
    XFBURN_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG (g_object_new
                                                        (XFBURN_TYPE_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG,
                                                         "show-buffers", FALSE, "title", _("Create ISO from composition"), NULL));

  return GTK_WIDGET (obj);
}
