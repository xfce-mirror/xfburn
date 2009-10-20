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

#ifndef __XFBURN_TRANSCODER_H__
#define __XFBURN_TRANSCODER_H__

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* !HAVE_CONFIG_H */

#include <glib.h>
#include <glib-object.h>

#include "xfburn-audio-track.h"

G_BEGIN_DECLS

#define XFBURN_TYPE_TRANSCODER         (xfburn_transcoder_get_type ())
#define XFBURN_TRANSCODER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), XFBURN_TYPE_TRANSCODER, XfburnTranscoder))
#define XFBURN_IS_TRANSCODER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XFBURN_TYPE_TRANSCODER))
//#define XFBURN_IS_TRANSCODER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), XFBURN_TYPE_TRANSCODER))
#define XFBURN_TRANSCODER_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), XFBURN_TYPE_TRANSCODER, XfburnTranscoderInterface))

#define XFBURN_AUDIO_TRACK_DELETE_DATA(atrack) { g_free (atrack->data); atrack->data = NULL; }

typedef struct {} XfburnTranscoder; /* dummy struct */

typedef struct
{
  GTypeInterface parent;

  /* required functions */
  const gchar * (*get_name) (XfburnTranscoder *trans);
  const gchar * (*get_description) (XfburnTranscoder *trans);
  gboolean (*is_initialized) (XfburnTranscoder *trans, GError **error);
  gboolean (*get_audio_track) (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);
  struct burn_track * (*create_burn_track) (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);

  /* optional functions */
  gboolean (*prepare) (XfburnTranscoder *trans, GError **error);
  void (*finish) (XfburnTranscoder *trans);
  gboolean (*free_burning_resources) (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);
  
} XfburnTranscoderInterface;

GType xfburn_transcoder_get_type (void);

void xfburn_transcoder_set_global (XfburnTranscoder *trans);
XfburnTranscoder *xfburn_transcoder_get_global (void);

const gchar *xfburn_transcoder_get_name (XfburnTranscoder *trans);
const gchar *xfburn_transcoder_get_description (XfburnTranscoder *trans);
gboolean xfburn_transcoder_is_initialized (XfburnTranscoder *trans, GError **error);
XfburnAudioTrack * xfburn_transcoder_get_audio_track (XfburnTranscoder *trans, const gchar *fn, GError **error);
struct burn_track *xfburn_transcoder_create_burn_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);

/* optional functions */
gboolean xfburn_transcoder_prepare (XfburnTranscoder *trans, GError **error);
void xfburn_transcoder_finish (XfburnTranscoder *trans);
gboolean xfburn_transcoder_free_burning_resources (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);

/* defined purely by the interface */
void xfburn_transcoder_free_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack);

G_END_DECLS

#endif /* __XFBURN_TRANSCODER_H__ */
