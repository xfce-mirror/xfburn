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

#include <unistd.h>
#include <libxfce4util/libxfce4util.h>

#include "xfburn-global.h"

#include "xfburn-transcoder.h"
#include "xfburn-error.h"

static void xfburn_transcoder_base_init (XfburnTranscoderInterface * iface);

/*
enum {
  LAST_SIGNAL,
}; 
*/

XfburnTranscoder *transcoder = NULL;

/*************************/
/* interface declaration */
/*************************/
//static guint signals[LAST_SIGNAL];

GType
xfburn_transcoder_get_type (void)
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnTranscoderInterface),
      (GBaseInitFunc) xfburn_transcoder_base_init ,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      NULL,
      NULL
    };

    type = g_type_register_static (G_TYPE_INTERFACE, "XfburnTranscoderInterface", &our_info, 0);
    g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
  }

  return type;
}

static void
xfburn_transcoder_base_init (XfburnTranscoderInterface * iface)
{
  static gboolean initialized = FALSE;
  
  if (!initialized) {
    /*
    signals[VOLUME_CHANGED] = g_signal_new ("volume-changed", XFBURN_TYPE_TRANSCODER, G_SIGNAL_ACTION,
                                            G_STRUCT_OFFSET (XfburnTranscoderClass, volume_changed),
                                            NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                            G_TYPE_NONE, 0);
    */
    initialized = TRUE;
  }
}

/*           */
/* internals */
/*           */

/*        */
/* public */
/*        */

const gchar *
xfburn_transcoder_get_name (XfburnTranscoder *trans)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);
  if (iface->get_name)
    return iface->get_name (trans);
  else
    return _("none");
}

const gchar *
xfburn_transcoder_get_description (XfburnTranscoder *trans)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);
  if (iface->get_description)
    return iface->get_description (trans);
  else
    return _("none");
}

gboolean 
xfburn_transcoder_is_initialized (XfburnTranscoder *trans, GError **error)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);
  if (iface->is_initialized)
    return iface->is_initialized (trans, error);
  
  g_warning ("Falling back to base implementation for xfburn_transcoder_is_initialized, which always says false.");
  g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_IMPLEMENTED, _("not implemented"));
  return FALSE;
}

XfburnAudioTrack *
xfburn_transcoder_get_audio_track (XfburnTranscoder *trans, const gchar *fn, GError **error)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);

  if (iface->get_audio_track) {
    XfburnAudioTrack *atrack = g_new0 (XfburnAudioTrack, 1);
    gboolean valid_track;

    atrack->inputfile = g_strdup (fn);
    atrack->pos = -1;

    valid_track = iface->get_audio_track (trans, atrack, error);
    
    if (valid_track)
      return atrack;
    
    /* not a valid track, clean up */
    g_boxed_free (XFBURN_TYPE_AUDIO_TRACK, atrack);
    return NULL;
  }
  
  g_warning ("Falling back to base implementation for xfburn_transcoder_get_audio_track, which always says false.");
  g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_IMPLEMENTED, _("not implemented"));
  return NULL;
}

struct burn_track *
xfburn_transcoder_create_burn_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);
  if (iface->create_burn_track)
    return iface->create_burn_track (trans, atrack, error);
  
  g_warning ("Falling back to empty base implementation for xfburn_transcoder_create_burn_track.");
  g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_IMPLEMENTED, _("not implemented"));
  return NULL;
}

gboolean
xfburn_transcoder_prepare (XfburnTranscoder *trans, GError **error)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);

  if (iface->prepare)
    return iface->prepare (trans, error);
  
  /* this function is not required by the interface */
  return TRUE;
}

void
xfburn_transcoder_finish (XfburnTranscoder *trans)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);

  if (iface->finish)
    iface->finish (trans);
  
  /* this function is not required by the interface */
}

gboolean
xfburn_transcoder_free_burning_resources (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);

  /* these are part of XfburnAudioTrack, and will be present for all implementations */
  close (atrack->fd);
  burn_source_free (atrack->src);

  /* allow for additional resource deallocation */
  if (iface->free_burning_resources)
    return iface->free_burning_resources (trans, atrack, error);
  
  /* this function is not required by the interface */
  return TRUE;
}


void
xfburn_transcoder_free_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack)
{
  //XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);

  g_free (atrack->inputfile);

  if (atrack->artist)
    g_free (atrack->artist);
  if (atrack->title)
    g_free (atrack->title);
}

void 
xfburn_transcoder_set_global (XfburnTranscoder *trans)
{
  transcoder = trans;
}

XfburnTranscoder *
xfburn_transcoder_get_global (void)
{
  g_object_ref (transcoder);
  return transcoder;
}
