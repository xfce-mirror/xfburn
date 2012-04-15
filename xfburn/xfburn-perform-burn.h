/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 * Copyright (c) 2008 David Mohr (squisher@xfce.org)
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

#ifndef __XFBURN_PERFORM_BURN_H__
#define __XFBURN_PERFORM_BURN_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>

#include <libburn.h>

#include "xfburn-device-box.h"

gboolean xfburn_set_write_mode (struct burn_write_opts *opts, XfburnWriteMode write_mode, struct burn_disc *disc,
                                XfburnWriteMode fallback);

void xfburn_perform_burn_write (GtkWidget *dialog_progress, 
                                struct burn_drive *drive, XfburnWriteMode write_mode, struct burn_write_opts *burn_options, int sector_size,
                                struct burn_disc *disc, struct burn_source **fifos, int *track_sectors);

#endif /* __XFBURN_PERFORM_BURN_H__ */
