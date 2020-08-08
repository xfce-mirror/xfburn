/*
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

#ifndef __XFBURN_AUDIO_TRACK_H__
#define __XFBURN_AUDIO_TRACK_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <glib.h>
#include <glib-object.h>
#include <libburn.h>

G_BEGIN_DECLS

#define XFBURN_TYPE_AUDIO_TRACK         (xfburn_audio_track_get_type ())
#define XFBURN_AUDIO_TRACK(p)           ((XfburnAudioTrack *) p)

typedef struct
{
  gchar *inputfile;
  gint pos;
  gchar *artist;
  gchar *title;
  gboolean swap;

  gint length;

  int sectors;
  int fd;
  struct burn_source *src;

  /* implementations will add extra data here */
  GType type;
  gpointer data;
} XfburnAudioTrack;

GType xfburn_audio_track_get_type (void);

G_END_DECLS

#endif /* __XFBURN_AUDIO_TRACK_H__ */
