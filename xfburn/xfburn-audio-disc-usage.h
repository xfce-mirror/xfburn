/*
 * Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XFBURN_AUDIO_DISC_USAGE_H__
#define __XFBURN_AUDIO_DISC_USAGE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include "xfburn-disc-usage.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_AUDIO_DISC_USAGE            (xfburn_audio_disc_usage_get_type ())
#define XFBURN_AUDIO_DISC_USAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFBURN_TYPE_AUDIO_DISC_USAGE, XfburnAudioDiscUsage))
#define XFBURN_AUDIO_DISC_USAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFBURN_TYPE_AUDIO_DISC_USAGE, XfburnAudioDiscUsageClass))
#define XFBURN_IS_AUDIO_DISC_USAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFBURN_TYPE_AUDIO_DISC_USAGE))
#define XFBURN_IS_AUDIO_DISC_USAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFBURN_TYPE_AUDIO_DISC_USAGE))
#define XFBURN_AUDIO_DISC_USAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFBURN_TYPE_AUDIO_DISC_USAGE, XfburnAudioDiscUsageClass))

typedef struct
{
  XfburnDiscUsage disc_usage;
} XfburnAudioDiscUsage;

typedef struct
{
  XfburnDiscUsageClass parent_class;
} XfburnAudioDiscUsageClass;

GType xfburn_audio_disc_usage_get_type (void);
GtkWidget *xfburn_audio_disc_usage_new (void);

G_END_DECLS

#endif
