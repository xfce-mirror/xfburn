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

#ifndef __XFBURN_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG_H__
#define __XFBURN_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

#include "xfburn-progress-dialog.h"

G_BEGIN_DECLS
#define XFBURN_TYPE_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG         (xfburn_create_iso_from_composition_progress_dialog_get_type ())
#define XFBURN_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG, XfburnCreateIsoFromCompositionProgressDialog))
#define XFBURN_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG, XfburnCreateIsoFromCompositionProgressDialogClass))
#define XFBURN_IS_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG))
#define XFBURN_IS_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG))
#define XFBURN_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG, XfburnCreateIsoFromCompositionProgressDialogClass))
typedef struct XfburnCreateIsoFromCompositionProgressDialogPrivate XfburnCreateIsoFromCompositionProgressDialogPrivate;

typedef struct
{
  XfburnProgressDialog parent;
  XfburnCreateIsoFromCompositionProgressDialogPrivate *priv;
} XfburnCreateIsoFromCompositionProgressDialog;

typedef struct
{
  XfburnProgressDialogClass parent_class;
  /* Add Signal Functions Here */
} XfburnCreateIsoFromCompositionProgressDialogClass;

GtkType xfburn_create_iso_from_composition_progress_dialog_get_type ();
GtkWidget *xfburn_create_iso_from_composition_progress_dialog_new ();

#endif /* XFBURN_CREATE_ISO_FROM_COMPOSITION_PROGRESS_DIALOG_H */
