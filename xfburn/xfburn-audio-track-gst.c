/*
 *  Copyright (c) 2008      David Mohr (dmohr@mcbf.net)
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

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <libxfce4util/libxfce4util.h>
#include "xfburn-audio-track-gst.h"

/* prototypes */
gpointer audio_track_gst_copy (gpointer boxed);
void audio_track_gst_free (gpointer boxed);

/* implementations */
GType
xfburn_audio_track_gst_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    type = g_boxed_type_register_static ("XfburnAudioTrackGst", audio_track_gst_copy, audio_track_gst_free);
  }

  return type;
}


gpointer 
audio_track_gst_copy (gpointer boxed)
{
  XfburnAudioTrackGst *copy;

  //DBG ("copying...");

  copy = g_new0(XfburnAudioTrackGst, 1);

  memcpy (copy, boxed, sizeof (XfburnAudioTrackGst));

  return copy;
}

void 
audio_track_gst_free (gpointer boxed)
{
  //XfburnAudioTrackGst *atrack = XFBURN_AUDIO_TRACK_GST (boxed);

  //DBG ("Freeing audio track gst");

  g_free (boxed);
}
