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

#ifndef __XFBURN_TRANSCODER_BASIC_H__
#define __XFBURN_TRANSCODER_BASIC_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <gtk/gtk.h>

#include "xfburn-transcoder.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_TRANSCODER_BASIC         (xfburn_transcoder_basic_get_type ())
#define XFBURN_TRANSCODER_BASIC(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_TRANSCODER_BASIC, XfburnTranscoderBasic))
#define XFBURN_TRANSCODER_BASIC_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), XFBURN_TYPE_TRANSCODER_BASIC, XfburnTranscoderBasicClass))
#define XFBURN_IS_TRANSCODER_BASIC(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_TRANSCODER_BASIC))
#define XFBURN_IS_TRANSCODER_BASIC_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_TRANSCODER_BASIC))
#define XFBURN_TRANSCODER_BASIC_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), XFBURN_TYPE_TRANSCODER_BASIC, XfburnTranscoderBasicClass))

typedef struct
{
  GObject parent;
} XfburnTranscoderBasic;

typedef struct
{
  GObjectClass parent_class;
  
} XfburnTranscoderBasicClass;

GType xfburn_transcoder_basic_get_type (void);
GObject *xfburn_transcoder_basic_new (void);

G_END_DECLS

#endif /* XFBURN_TRANSCODER_BASIC_H */
