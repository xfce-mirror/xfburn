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

#include "xfburn-progress-dialog.h"

#include "xfburn-burn-data-composition-progress-dialog.h"

static void xfburn_burn_data_composition_progress_dialog_class_init (XfburnBurnDataCompositionProgressDialogClass * klass);
static void xfburn_burn_data_composition_progress_dialog_init (XfburnBurnDataCompositionProgressDialog * sp);

static void cb_new_output (XfburnBurnDataCompositionProgressDialog * dialog, const gchar * output, gpointer data);

static XfburnProgressDialogClass *parent_class = NULL;

GtkType
xfburn_burn_data_composition_progress_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnBurnDataCompositionProgressDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_burn_data_composition_progress_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnBurnDataCompositionProgressDialog),
      0,
      (GInstanceInitFunc) xfburn_burn_data_composition_progress_dialog_init,
    };

    type = g_type_register_static (XFBURN_TYPE_PROGRESS_DIALOG, "XfburnBurnDataCompositionProgressDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_burn_data_composition_progress_dialog_class_init (XfburnBurnDataCompositionProgressDialogClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_burn_data_composition_progress_dialog_init (XfburnBurnDataCompositionProgressDialog * obj)
{
  g_signal_connect_after (G_OBJECT (obj), "output", G_CALLBACK (cb_new_output), NULL);
}

/*           */
/* internals */
/*           */
static void
cb_new_output (XfburnBurnDataCompositionProgressDialog * dialog, const gchar * output, gpointer data)
{
  if (strstr (output, CDRECORD_FIXATING_TIME)) {
    xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Finishing"));
    xfburn_progress_dialog_set_fifo_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), -1);
    xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), -1);
    xfburn_progress_dialog_set_status (XFBURN_PROGRESS_DIALOG (dialog), XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED);
  }
  else if (strstr (output, CDRECORD_FIXATING)) {
    xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Fixating..."));
  }
  else if (strstr (output, CDRECORD_COPY)) {
    gint current = 0, fifo = 0, buf = 0;
    gfloat speed = 0;

    if (sscanf (output, "%*s %*d: %d %*s %*s (%*s %d%%) [%*s %d%%] %fx.", &current, &fifo, &buf, &speed) == 4) {

      xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Writing composition..."));
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog), speed);
      xfburn_progress_dialog_set_fifo_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), ((gdouble) fifo) / 100);
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), ((gdouble) buf) / 100);
    }
  } else if (strstr (output, MKISOFS_RUNNING)) {
    gfloat percent = 0;
    if (sscanf (output, "%f%% done", &percent) == 1) {
      gdouble fraction;

      fraction = (gdouble) (percent / 100);

      xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Writing composition..."));
      xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), fraction);
    }
  }
}

/*        */
/* public */
/*        */

GtkWidget *
xfburn_burn_data_composition_progress_dialog_new ()
{
  XfburnBurnDataCompositionProgressDialog *obj;

  obj = XFBURN_BURN_COMPOSITION_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_BURN_COMPOSITION_PROGRESS_DIALOG, 
                                                               "title", _("Burn data composition"), NULL));

  return GTK_WIDGET (obj);
}
