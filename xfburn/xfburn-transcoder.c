/* $Id$ */
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
xfburn_transcoder_get_type ()
{
  static GtkType type = 0;

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

gboolean
xfburn_transcoder_is_audio_file (XfburnTranscoder *trans, const gchar *fn, GError **error)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);
  if (iface->is_audio_file)
    return iface->is_audio_file (trans, fn, error);
  
  g_warning ("Falling back to base implementation for xfburn_transcoder_is_audio_file, which always says false.");
  g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_IMPLEMENTED, "xfburn_transcoder_is_audio_file is not implemented");
  return FALSE;
}

struct burn_track *
xfburn_transcoder_create_burn_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);
  if (iface->create_burn_track)
    return iface->create_burn_track (trans, atrack, error);
  
  g_warning ("Falling back to empty base implementation for xfburn_transcoder_create_burn_track.");
  g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_IMPLEMENTED, "xfburn_transcoder_create_burn_track is not implemented");
  return NULL;
}

gboolean
xfburn_transcoder_clear (XfburnTranscoder *trans, GError **error)
{
  XfburnTranscoderInterface *iface = XFBURN_TRANSCODER_GET_INTERFACE (trans);
  if (iface->clear)
    return iface->clear (trans, error);
  
  g_warning ("Falling back to empty base implementation for xfburn_transcoder_clear.");
  g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_IMPLEMENTED, "xfburn_transcoder_clear is not implemented");
  return FALSE;
}


void 
xfburn_transcoder_set_global (XfburnTranscoder *trans)
{
  transcoder = trans;
}

XfburnTranscoder *
xfburn_transcoder_get_global ()
{
  g_object_ref (transcoder);
  return transcoder;
}
