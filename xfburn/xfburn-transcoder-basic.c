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

#include <errno.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <libburn.h>

#include "xfburn-global.h"
#include "xfburn-error.h"

#include "xfburn-transcoder-basic.h"

/** Prototypes **/
/* class initialization */
static void xfburn_transcoder_basic_finalize (GObject * object);
static void transcoder_interface_init (XfburnTranscoderInterface *iface, gpointer iface_data);

/* internals */
static const gchar * get_name (XfburnTranscoder *trans);
static const gchar * get_description (XfburnTranscoder *trans);
static gboolean is_initialized (XfburnTranscoder *trans, GError **error);

static gboolean get_audio_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);
static gboolean has_audio_ext (const gchar *path);
static gboolean is_valid_wav (const gchar *path);
static gboolean valid_wav_headers (guchar header[44]);

static struct burn_track * create_burn_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);
static gboolean needs_swap (char header[44]);

enum {
  LAST_SIGNAL,
}; 

/* globals */

static const gchar *errormsg_libburn_setup = N_("An error occurred while setting the burning backend up");

/*********************/
/* class declaration */
/*********************/
static GObject *parent_class = NULL;
//static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_EXTENDED(
  XfburnTranscoderBasic,
  xfburn_transcoder_basic,
  G_TYPE_OBJECT,
  0,
  G_IMPLEMENT_INTERFACE(XFBURN_TYPE_TRANSCODER, transcoder_interface_init)
);

static void
xfburn_transcoder_basic_class_init (XfburnTranscoderBasicClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
    
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
}

static void
xfburn_transcoder_basic_finalize (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
transcoder_interface_init (XfburnTranscoderInterface *iface, gpointer iface_data)
{
  iface->get_name = get_name;
  iface->get_description = get_description;
  iface->is_initialized = is_initialized;
  iface->get_audio_track = get_audio_track;
  iface->create_burn_track = create_burn_track;
}
/*           */
/* internals */
/*           */

static const gchar * 
get_name (XfburnTranscoder *trans)
{
  return _("basic");
}

static const gchar * 
get_description (XfburnTranscoder *trans)
{
  return _("The basic transcoder is built in,\n"
           "and does not require any library.\n"
           "But it can only handle uncompressed\n"
           ".wav files.\n"
           "If you would like to create audio\n"
           "compositions from different types of\n"
           "audio files, please compile with\n"
           "gstreamer support.");
}

static gboolean
is_initialized (XfburnTranscoder *trans, GError **error)
{
  /* there is nothing to check, because there is nothing to initialize */
  return TRUE;
}

static gboolean
get_audio_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error)
{
  struct stat s;

  if (!has_audio_ext (atrack->inputfile)) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_AUDIO_EXT,
                 _("File %s does not have a .wav extension"), atrack->inputfile);
    return FALSE;
  }
  if (!is_valid_wav (atrack->inputfile)) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_AUDIO_FORMAT,
                 _("File %s does not contain uncompressed PCM wave audio"), atrack->inputfile);
    return FALSE;
  }

  if (stat (atrack->inputfile, &s) != 0) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_STAT,
                 _("Could not stat %s: %s"), atrack->inputfile, g_strerror (errno));
    return FALSE;
  }

  atrack->length = (s.st_size - 44) / PCM_BYTES_PER_SECS;
  atrack->sectors = (s.st_size / AUDIO_BYTES_PER_SECTOR);
  if (s.st_size % AUDIO_BYTES_PER_SECTOR > 0)
    atrack->sectors++;

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
  int hread;
  guchar header[44];
  gboolean ret;

  fd = open (path, 0);

  if (fd == -1) {
    xfce_dialog_show_warning(NULL, "", _("Could not open %s."), path);
    return FALSE;
  }

  ret = FALSE;
  hread = read (fd, header, 44);
  if (hread == 44) {
    ret = valid_wav_headers (header);
  }

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


static struct burn_track *
create_burn_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error)
{
  char header[44];
  int thead=0;
  struct burn_track *track;

  atrack->fd = open (atrack->inputfile, 0);
  if (atrack->fd == -1) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_COULD_NOT_OPEN_FILE,
                 _("Could not open %s: %s"), atrack->inputfile, g_strerror (errno));
    return NULL;
  }

  /* advance the fd so that libburn skips the header,
   * also allows us to check for byte swapping */
  thead = read (atrack->fd, header, 44);
  if( thead < 44 ) {
    g_warning ("Could not read header from %s!", atrack->inputfile);
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_BURN_SOURCE,
		 "%s",
                 _(errormsg_libburn_setup));
    return NULL;
  }


  atrack->src = burn_fd_source_new (atrack->fd, -1 , 0);
  if (atrack->src == NULL) {
    g_warning ("Could not create burn_source from %s!", atrack->inputfile);
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_BURN_SOURCE,
		 "%s",
                 _(errormsg_libburn_setup));
    return NULL;
  }

  track = burn_track_create ();
  
  if (burn_track_set_source (track, atrack->src) != BURN_SOURCE_OK) {
    g_warning ("Could not add source to track %s!", atrack->inputfile);
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_BURN_SOURCE,
		 "%s",
                 _(errormsg_libburn_setup));
                 
    return NULL;
  }

  if (needs_swap (header))
    burn_track_set_byte_swap (track, TRUE);

  burn_track_define_data (track, 0, 0, 1, BURN_AUDIO);

  return track;
}

static gboolean
needs_swap (char header[44])
{
  if (header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'X')
    return TRUE;
  else 
    return FALSE;
}

/*        */
/* public */
/*        */

GObject *
xfburn_transcoder_basic_new (void)
{
  return g_object_new (XFBURN_TYPE_TRANSCODER_BASIC, NULL);
}
