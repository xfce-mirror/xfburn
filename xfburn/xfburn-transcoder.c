/* $Id$ */
/*
 *  Copyright (c) 2005-2006 Jean-François Wauthy (pollux@xfce.org)
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-global.h"

#include "xfburn-transcoder.h"

static void xfburn_transcoder_class_init (XfburnTranscoderClass * klass);
static void xfburn_transcoder_init (XfburnTranscoder * obj);
static void xfburn_transcoder_finalize (GObject * object);

#define XFBURN_TRANSCODER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_TRANSCODER, XfburnTranscoderPrivate))

enum {
  LAST_SIGNAL,
}; 

typedef struct {
  gboolean dummy;
} XfburnTranscoderPrivate;

/*********************/
/* class declaration */
/*********************/
static GObject *parent_class = NULL;
static guint signals[LAST_SIGNAL];

GtkType
xfburn_transcoder_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnTranscoderClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_transcoder_class_init,
      NULL,
      NULL,
      sizeof (XfburnTranscoder),
      0,
      (GInstanceInitFunc) xfburn_transcoder_init,
      NULL
    };

    type = g_type_register_static (G_TYPE_OBJECT, "XfburnTranscoder", &our_info, 0);
  }

  return type;
}

static void
xfburn_transcoder_class_init (XfburnTranscoderClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  g_type_class_add_private (klass, sizeof (XfburnTranscoderPrivate));
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_transcoder_finalize;

/*
  signals[VOLUME_CHANGED] = g_signal_new ("volume-changed", XFBURN_TYPE_TRANSCODER, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnTranscoderClass, volume_changed),
                                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);
*/
}

static void
xfburn_transcoder_init (XfburnTranscoder * obj)
{
  //XfburnTranscoderPrivate *priv = XFBURN_TRANSCODER_GET_PRIVATE (obj);
}

static void
xfburn_transcoder_finalize (GObject * object)
{
  //XfburnTranscoderPrivate *priv = XFBURN_TRANSCODER_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*           */
/* internals */
/*           */

/*        */
/* public */
/*        */

GObject *
xfburn_transcoder_new ()
{
  return g_object_new (XFBURN_TYPE_TRANSCODER, NULL);
}
