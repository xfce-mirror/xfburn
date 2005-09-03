/* $Id$ */
/*
 *  Copyright (c) 2005 Jean-François Wauthy (pollux@xfce.org)
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

#include "xfburn-progress-dialog.h"

#include "xfburn-blank-cd-progress-dialog.h"

static void xfburn_blank_cd_progress_dialog_class_init (XfburnBlankCdProgressDialogClass * klass);
static void xfburn_blank_cd_progress_dialog_init (XfburnBlankCdProgressDialog * sp);
static void xfburn_blank_cd_progress_dialog_finalize (GObject * object);

struct XfburnBlankCdProgressDialogPrivate
{
  /* Place Private Members Here */
};

static XfburnProgressDialogClass *parent_class = NULL;

GtkType
xfburn_blank_cd_progress_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnBlankCdProgressDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_blank_cd_progress_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnBlankCdProgressDialog),
      0,
      (GInstanceInitFunc) xfburn_blank_cd_progress_dialog_init,
    };

    type = g_type_register_static (XFBURN_TYPE_PROGRESS_DIALOG, "XfburnBlankCdProgressDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_blank_cd_progress_dialog_class_init (XfburnBlankCdProgressDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_blank_cd_progress_dialog_finalize;

}

static void
xfburn_blank_cd_progress_dialog_init (XfburnBlankCdProgressDialog * obj)
{
  obj->priv = g_new0 (XfburnBlankCdProgressDialogPrivate, 1);
  /* Initialize private members, etc. */
}

static void
xfburn_blank_cd_progress_dialog_finalize (GObject * object)
{
  XfburnBlankCdProgressDialog *cobj;
  cobj = XFBURN_BLANK_CD_PROGRESS_DIALOG (object);

  /* Free private members, etc. */

  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*           */
/* internals */
/*           */

/*        */
/* public */
/*        */

GtkWidget *
xfburn_blank_cd_progress_dialog_new ()
{
  XfburnBlankCdProgressDialog *obj;

  obj = XFBURN_BLANK_CD_PROGRESS_DIALOG (g_object_new (XFBURN_TYPE_BLANK_CD_PROGRESS_DIALOG, NULL));

  return GTK_WIDGET (obj);
}
