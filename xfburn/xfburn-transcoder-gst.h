/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *  Copyright (c) 2008      David Mohr (david@mcbf.net)
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

#ifndef __XFBURN_TRANSCODER_GST_H__
#define __XFBURN_TRANSCODER_GST_H__

#ifdef HAVE_GST

#include <gtk/gtk.h>

#include "xfburn-transcoder.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_TRANSCODER_GST (xfburn_transcoder_gst_get_type ())
G_DECLARE_FINAL_TYPE (XfburnTranscoderGst, xfburn_transcoder_gst, XFBURN, TRANSCODER_GST, GObject)

GObject *xfburn_transcoder_gst_new (void);

G_END_DECLS

#endif /* HAVE_GST */
#endif /* XFBURN_TRANSCODER_GST_H */
