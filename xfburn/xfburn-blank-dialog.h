/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#ifndef __XFBURN_BLANK_DIALOG_H__
#define __XFBURN_BLANK_DIALOG_H__

#include "xfburn-global.h"

#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_BLANK_DIALOG (xfburn_blank_dialog_get_type ())
G_DECLARE_FINAL_TYPE (XfburnBlankDialog, xfburn_blank_dialog, XFBURN, BLANK_DIALOG, XfceTitledDialog)

GtkWidget *xfburn_blank_dialog_new (void);
GtkWidget * xfburn_blank_dialog_new_eject (gboolean eject);

G_END_DECLS
#endif /* XFBURN_BLANK_DIALOG_H */
