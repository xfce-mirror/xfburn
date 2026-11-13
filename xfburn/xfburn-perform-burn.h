/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 * Copyright (c) 2008 David Mohr (squisher@xfce.org)
 * Copyright (c) 2026 Dominik Klein
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

#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>

#include <libburn.h>

#include "xfburn-device-box.h"

typedef struct _XfburnVerifyData XfburnVerifyData;

XfburnVerifyData *xfburn_verify_data_new (gint sector_size, off_t max_bytes);
void xfburn_verify_data_free (XfburnVerifyData *verify_data);
struct burn_source *xfburn_verify_wrap_source (struct burn_source *inner, gboolean swap, XfburnVerifyData *verify_data);

gboolean xfburn_set_write_mode (struct burn_write_opts *opts, XfburnWriteMode write_mode, struct burn_disc *disc,
                                XfburnWriteMode fallback);

void xfburn_perform_burn_write (GtkWidget *dialog_progress,
                                struct burn_drive *drive, XfburnWriteMode write_mode, struct burn_write_opts *burn_options, int sector_size,
                                struct burn_disc *disc, struct burn_source **fifos, int *track_sectors,
                                XfburnVerifyData *verify_data);

#endif /* __XFBURN_PERFORM_BURN_H__ */
