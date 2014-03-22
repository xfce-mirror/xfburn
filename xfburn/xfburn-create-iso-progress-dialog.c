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

#include "xfburn-create-iso-progress-dialog.h"

static void xfburn_create_iso_progress_dialog_class_init (XfburnCreateIsoProgressDialogClass * klass);
static void xfburn_create_iso_progress_dialog_init (XfburnCreateIsoProgressDialog * sp);

static void cb_new_output (XfburnCreateIsoProgressDialog * dialog, const gchar * output, gpointer data);

/*********************/
/* class declaration */
/*********************/
static XfburnProgressDialogClass *parent_class = NULL;

GType
xfburn_create_iso_progress_dialog_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnCreateIsoProgressDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_create_iso_progress_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnCreateIsoProgressDialog),
      0,
      (GInstanceInitFunc) xfburn_create_iso_progress_dialog_init,
      NULL
    };

    type = g_type_register_static (XFBURN_TYPE_PROGRESS_DIALOG, "XfburnCreateIsoProgressDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_create_iso_progress_dialog_class_init (XfburnCreateIsoProgressDialogClass * klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
xfburn_create_iso_progress_dialog_init (XfburnCreateIsoProgressDialog * obj)
{
  g_signal_connect_after (G_OBJECT (obj), "output", G_CALLBACK (cb_new_output), NULL);
}

/*           */
/* internals */
/*           */
static void
cb_new_output (XfburnCreateIsoProgressDialog * dialog, const gchar * output, gpointer data)
{
  static gint readcd_end = -1;

  if (strstr (output, READCD_DONE)) {
    xfburn_progress_dialog_set_status (XFBURN_PROGRESS_DIALOG (dialog), XFBURN_PROGRESS_DIALOG_STATUS_COMPLETED);
  }
  else if (strstr (output, READCD_PROGRESS)) {
    gint readcd_done = -1;
    gdouble fraction;

    sscanf (output, "%*s %d", &readcd_done);
    fraction = ((gdouble) readcd_done) / readcd_end;

    xfburn_progress_dialog_set_progress_bar_fraction (XFBURN_PROGRESS_DIALOG (dialog), fraction * 100.0);
  }
  else if (strstr (output, READCD_CAPACITY)) {
    xfburn_progress_dialog_set_action_text (XFBURN_PROGRESS_DIALOG (dialog), _("Reading CD..."));
    sscanf (output, "%*s %d", &readcd_end);
  }
}

/*        */
/* public */
/*        */

GtkWidget *
xfburn_create_iso_progress_dialog_new (void)
{
  XfburnCreateIsoProgressDialog *obj;

  obj = XFBURN_CREATE_ISO_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_CREATE_ISO_PROGRESS_DIALOG,
                                                         "show-buffers", FALSE, "title", _("Create ISO from CD"), NULL));

  return GTK_WIDGET (obj);
}
