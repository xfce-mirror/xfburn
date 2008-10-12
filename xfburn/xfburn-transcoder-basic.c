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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include "xfburn-global.h"
#include "xfburn-error.h"

#include "xfburn-transcoder-basic.h"

/** Prototypes **/
/* class initialization */
static void xfburn_transcoder_basic_class_init (XfburnTranscoderBasicClass * klass);
static void xfburn_transcoder_basic_init (XfburnTranscoderBasic * obj);
static void xfburn_transcoder_basic_finalize (GObject * object);
static void transcoder_interface_init (XfburnTranscoderInterface *iface, gpointer iface_data);

/* internals */
static gboolean is_audio_file (XfburnTranscoder *trans, const gchar *fn, GError **error);
static gboolean has_audio_ext (const gchar *path);
static gboolean is_valid_wav (const gchar *path);
static gboolean valid_wav_headers (guchar header[44]);


#define XFBURN_TRANSCODER_BASIC_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_TRANSCODER_BASIC, XfburnTranscoderBasicPrivate))

enum {
  LAST_SIGNAL,
}; 

typedef struct {
  gboolean dummy;
} XfburnTranscoderBasicPrivate;

/*********************/
/* class declaration */
/*********************/
static GObject *parent_class = NULL;
//static guint signals[LAST_SIGNAL];

GtkType
xfburn_transcoder_basic_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnTranscoderBasicClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_transcoder_basic_class_init,
      NULL,
      NULL,
      sizeof (XfburnTranscoderBasic),
      0,
      (GInstanceInitFunc) xfburn_transcoder_basic_init,
      NULL
    };
    static const GInterfaceInfo trans_info = {
      (GInterfaceInitFunc) transcoder_interface_init,
      NULL,
      NULL
    };

    type = g_type_register_static (G_TYPE_OBJECT, "XfburnTranscoderBasic", &our_info, 0);

    g_type_add_interface_static (type, XFBURN_TYPE_TRANSCODER, &trans_info);
  }

  return type;
}

static void
xfburn_transcoder_basic_class_init (XfburnTranscoderBasicClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  g_type_class_add_private (klass, sizeof (XfburnTranscoderBasicPrivate));
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_transcoder_basic_finalize;

/*
  signals[VOLUME_CHANGED] = g_signal_new ("volume-changed", XFBURN_TYPE_TRANSCODER_BASIC, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnTranscoderBasicClass, volume_changed),
                                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);
*/
}

static void
xfburn_transcoder_basic_init (XfburnTranscoderBasic * obj)
{
  //XfburnTranscoderBasicPrivate *priv = XFBURN_TRANSCODER_BASIC_GET_PRIVATE (obj);
}

static void
xfburn_transcoder_basic_finalize (GObject * object)
{
  //XfburnTranscoderBasicPrivate *priv = XFBURN_TRANSCODER_BASIC_GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
transcoder_interface_init (XfburnTranscoderInterface *iface, gpointer iface_data)
{
  iface->is_audio_file = is_audio_file;
}
/*           */
/* internals */
/*           */

static gboolean
is_audio_file (XfburnTranscoder *trans, const gchar *fn, GError **error)
{
  if (!has_audio_ext (fn)) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_AUDIO_EXT,
                 "File %s does not have a .wav extension", fn);
    return FALSE;
  }
  if (!is_valid_wav (fn)) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_AUDIO_FORMAT,
                 "File %s does not contain uncompressed PCM wave audio", fn);
    return FALSE;
  }

  return TRUE;
}

static gboolean 
has_audio_ext (const gchar *path)
{
  int len = strlen (path);
  const gchar *ext = path + len - 3;

  return (strcmp (ext, "wav") == 0);
}

static gboolean 
is_valid_wav (const gchar *path)
{
  int fd;
  guchar header[44];
  gboolean ret;

  fd = open (path, 0);

  if (fd == -1) {
    xfce_warn (_("Could not open %s!"), path);
    return FALSE;
  }

  read (fd, header, 44);

  ret = valid_wav_headers (header);

  close (fd);

  return ret;
}

/*
 * Simple check of .wav headers, most info from
 * http://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 *
 * I did not take particular care to make sure this check is complete.
 * As far as I can tell yes, but not much effort was put into verifying it.
 * FIXME: this works on x86, and does not consider endianness!
 */
static gboolean
valid_wav_headers (guchar header[44])
{
  /* check if first 4 bytes are RIFF or RIFX */
  if (header[0] == 'R' && header[1] == 'I' && header[2] == 'F') {
    if (!(header[3] == 'X' || header[3] == 'F')) {
      g_warning ("File not in riff format");
      return FALSE;
    }
  }

  /* check if bytes 8-11 are WAVE */
  if (!(header[8] == 'W' && header[9] == 'A' && header[10] == 'V' && header[11] == 'E')) {
    g_warning ("RIFF file not in WAVE format");
    return FALSE;
  }

  /* subchunk starts with 'fmt ' */
  if (!(header[12] == 'f' && header[13] == 'm' && header[14] == 't' && header[15] == ' ')) {
    g_warning ("Could not find format subchunk");
    return FALSE;
  }

  /* check for PCM format */
  if (header[16] != 16 || header[20] != 1) {
    g_warning ("Not in PCM format");
    return FALSE;
  }

  /* check for stereo */
  if (header[22] != 2) {
    g_warning ("Not in stereo");
    return FALSE;
  }

  /* check for 44100 Hz sample rate,
   * being lazy here and just compare the bytes to what I know they should be */
  if (!(header[24] == 0x44 && header[25] == 0xAC && header[26] == 0 && header[27] == 0)) {
    g_warning ("Does not have a sample rate of 44100 Hz");
    return FALSE;
  }

  return TRUE;
}

/*        */
/* public */
/*        */

GObject *
xfburn_transcoder_basic_new ()
{
  return g_object_new (XFBURN_TYPE_TRANSCODER_BASIC, NULL);
}
