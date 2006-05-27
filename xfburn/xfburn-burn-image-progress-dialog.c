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

#include "xfburn-global.h"
#include "xfburn-progress-dialog.h"

#include "xfburn-burn-image-progress-dialog.h"

static void xfburn_burn_image_progress_dialog_class_init (XfburnBurnImageProgressDialogClass * klass);
static void xfburn_burn_image_progress_dialog_init (XfburnBurnImageProgressDialog * sp);
static void xfburn_burn_image_progress_dialog_finalize (GObject * object);

static void cb_new_output (XfburnBurnImageProgressDialog * dialog, const gchar * output,
                           XfburnBurnImageProgressDialogPrivate * priv);

struct XfburnBurnImageProgressDialogPrivate
{
  /* Place Private Members Here */
};

static XfburnProgressDialogClass *parent_class = NULL;

GtkType
xfburn_burn_image_progress_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnBurnImageProgressDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_burn_image_progress_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnBurnImageProgressDialog),
      0,
      (GInstanceInitFunc) xfburn_burn_image_progress_dialog_init,
    };

    type = g_type_register_static (XFBURN_TYPE_PROGRESS_DIALOG, "XfburnBurnImageProgressDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_burn_image_progress_dialog_class_init (XfburnBurnImageProgressDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_burn_image_progress_dialog_finalize;

}

static void
xfburn_burn_image_progress_dialog_init (XfburnBurnImageProgressDialog * obj)
{
  obj->priv = g_new0 (XfburnBurnImageProgressDialogPrivate, 1);
  /* Initialize private members, etc. */

  g_signal_connect_after (G_OBJECT (obj), "output", G_CALLBACK (cb_new_output), obj->priv);
}

static void
xfburn_burn_image_progress_dialog_finalize (GObject * object)
{
  XfburnBurnImageProgressDialog *cobj;
  cobj = XFBURN_BURN_IMAGE_PROGRESS_DIALOG (object);

  /* Free private members, etc. */

  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*           */
/* internals */
/*           */
static void
cb_new_output (XfburnBurnImageProgressDialog * dialog, const gchar * output,
               XfburnBurnImageProgressDialogPrivate * priv)
{
  if (strstr (output, CDRECORD_FIXATING_TIME)) {
    xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("no info"));
    xfburn_progress_dialog_set_fifo_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), -1);
    xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), -1);
    xfburn_progress_dialog_set_status (XFBURN_PROGRESS_DIALOG (dialog), XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED);
  }
  else if (strstr (output, CDRECORD_FIXATING)) {
    xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Fixating..."));
  }
  else if (strstr (output, CDRECORD_COPY)) {
    gfloat current = 0, total = 0;
    gint fifo = 0, buf = 0, speed = 0, speed_decimal = 0;

    if (sscanf
        (output, "%*s %*d %*s %f %*s %f %*s %*s %*s %d %*s %*s %d %*s %d.%d", &current, &total, &fifo, &buf,
         &speed, &speed_decimal) == 6 && total > 0) {
      gdouble fraction;

      fraction = (gdouble) (current / total);

      xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Writing ISO..."));
      xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), fraction);
      xfburn_progress_dialog_set_writing_speed (XFBURN_PROGRESS_DIALOG (dialog), speed + (0.1 * speed_decimal));
      xfburn_progress_dialog_set_fifo_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), ((gdouble) fifo) / 100);
      xfburn_progress_dialog_set_buffer_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), ((gdouble) buf) / 100);
    }
  }
}

/*        */
/* public */
/*        */

GtkWidget *
xfburn_burn_image_progress_dialog_new ()
{
  XfburnBurnImageProgressDialog *obj;

  obj = XFBURN_BURN_IMAGE_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_BURN_IMAGE_PROGRESS_DIALOG,
                                                         "title", _("Burn CD image"), NULL));

  return GTK_WIDGET (obj);
}
