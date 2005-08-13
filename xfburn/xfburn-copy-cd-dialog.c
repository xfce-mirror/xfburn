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

#include "xfburn-copy-cd-dialog.h"

/* prototypes */
static void xfburn_copy_cd_dialog_class_init (XfburnCopyCdDialogClass * klass);
static void xfburn_copy_cd_dialog_init (XfburnCopyCdDialog * sp);
static void xfburn_copy_cd_dialog_finalize (GObject * object);

/* structures */
struct XfburnCopyCdDialogPrivate
{
  /* Place Private Members Here */
};

/* globals */
static GtkDialogClass *parent_class = NULL;

GtkType
xfburn_copy_cd_dialog_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnCopyCdDialogClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_copy_cd_dialog_class_init,
      NULL,
      NULL,
      sizeof (XfburnCopyCdDialog),
      0,
      (GInstanceInitFunc) xfburn_copy_cd_dialog_init,
    };

    type = g_type_register_static (GTK_TYPE_DIALOG, "XfburnCopyCdDialog", &our_info, 0);
  }

  return type;
}

static void
xfburn_copy_cd_dialog_class_init (XfburnCopyCdDialogClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = xfburn_copy_cd_dialog_finalize;

}

static void
xfburn_copy_cd_dialog_init (XfburnCopyCdDialog * obj)
{
  obj->priv = g_new0 (XfburnCopyCdDialogPrivate, 1);
}

static void
xfburn_copy_cd_dialog_finalize (GObject * object)
{
  XfburnCopyCdDialog *cobj;
  cobj = XFBURN_COPY_CD_DIALOG (object);

  g_free (cobj->priv);
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
xfburn_copy_cd_dialog_new ()
{
  XfburnCopyCdDialog *obj;

  obj = XFBURN_COPY_CD_DIALOG (g_object_new (XFBURN_TYPE_COPY_CD_DIALOG, NULL));

  return GTK_WIDGET (obj);
}
