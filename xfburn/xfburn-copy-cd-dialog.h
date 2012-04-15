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

#ifndef __XFBURN_COPY_CD_DIALOG_H__
#define __XFBURN_COPY_CD_DIALOG_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

#include <libxfce4ui/libxfce4ui.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_COPY_CD_DIALOG         (xfburn_copy_cd_dialog_get_type ())
#define XFBURN_COPY_CD_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_COPY_CD_DIALOG, XfburnCopyCdDialog))
#define XFBURN_COPY_CD_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_COPY_CD_DIALOG, XfburnCopyCdDialogClass))
#define XFBURN_IS_COPY_CD_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_COPY_CD_DIALOG))
#define XFBURN_IS_COPY_CD_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_COPY_CD_DIALOG))
#define XFBURN_COPY_CD_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_COPY_CD_DIALOG, XfburnCopyCdDialogClass))

typedef struct
{
  XfceTitledDialog parent;
} XfburnCopyCdDialog;

typedef struct
{
  XfceTitledDialogClass parent_class;
} XfburnCopyCdDialogClass;

GType xfburn_copy_cd_dialog_get_type (void);

GtkWidget *xfburn_copy_cd_dialog_new (void);

G_END_DECLS

#endif /* XFBURN_COPY_CD_DIALOG_H */
