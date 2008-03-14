/* $Id$ */
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

#if 0
#ifndef __XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG_H__
#define __XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>

#include <libisofs.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_BURN_DATA_DVD_COMPOSITION_DIALOG         (xfburn_burn_data_dvd_composition_dialog_get_type ())
#define XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_BURN_DATA_DVD_COMPOSITION_DIALOG, XfburnBurnDataDvdCompositionDialog))
#define XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_BURN_DATA_DVD_COMPOSITION_DIALOG, XfburnBurnDataDvdCompositionDialogClass))
#define XFBURN_IS_BURN_DATA_DVD_COMPOSITION_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_BURN_DATA_DVD_COMPOSITION_DIALOG))
#define XFBURN_IS_BURN_DATA_DVD_COMPOSITION_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_BURN_DATA_DVD_COMPOSITION_DIALOG))
#define XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_BURN_DATA_DVD_COMPOSITION_DIALOG, XfburnBurnDataDvdCompositionDialogClass))

typedef struct
{
  XfceTitledDialog parent;
} XfburnBurnDataDvdCompositionDialog;

typedef struct
{
  XfceTitledDialogClass parent_class;
} XfburnBurnDataDvdCompositionDialogClass;

GtkType xfburn_burn_data_dvd_composition_dialog_get_type ();

GtkWidget *xfburn_burn_data_dvd_composition_dialog_new (struct iso_volset * volume_set);

G_END_DECLS
#endif /* XFBURN_BURN_DATA_DVD_COMPOSITION_DIALOG_H */
#endif
