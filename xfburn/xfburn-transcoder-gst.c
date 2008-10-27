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

#ifdef HAVE_GST

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <libburn.h>

#include <gst/gst.h>

#include "xfburn-global.h"
#include "xfburn-error.h"
#include "xfburn-settings.h"

#include "xfburn-transcoder-gst.h"

/** Prototypes **/
/* class initialization */
static void xfburn_transcoder_gst_class_init (XfburnTranscoderGstClass * klass);
static void xfburn_transcoder_gst_init (XfburnTranscoderGst * obj);
static void xfburn_transcoder_gst_finalize (GObject * object);
static void transcoder_interface_init (XfburnTranscoderInterface *iface, gpointer iface_data);

/* internals */
static void create_pipeline (XfburnTranscoderGst *trans);
static void delete_pipeline (XfburnTranscoderGst *trans);
static void recreate_pipeline (XfburnTranscoderGst *trans);
static const gchar * get_name (XfburnTranscoder *trans);
static XfburnAudioTrack * get_audio_track (XfburnTranscoder *trans, const gchar *fn, GError **error);

static struct burn_track * create_burn_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);

static gboolean prepare (XfburnTranscoder *trans, GError **error);
static gboolean transcode_next_track (XfburnTranscoderGst *trans, GError **error);
static void finish (XfburnTranscoder *trans);
static gboolean free_burning_resources (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);

static gboolean is_initialized (XfburnTranscoder *trans, GError **error);

/* gstreamer support functions */
static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data);
static void on_pad_added (GstElement *element, GstPad *pad, gboolean last, gpointer data);

#define XFBURN_TRANSCODER_GST_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFBURN_TYPE_TRANSCODER_GST, XfburnTranscoderGstPrivate))

enum {
  LAST_SIGNAL,
}; 

typedef enum {
  XFBURN_TRANSCODER_GST_STATE_IDLE,
  XFBURN_TRANSCODER_GST_STATE_IDENTIFYING,
  XFBURN_TRANSCODER_GST_STATE_TRANSCODE_START,
  XFBURN_TRANSCODER_GST_STATE_TRANSCODING,
} XfburnTranscoderGstState;

typedef struct {
  GstElement *pipeline;
  GstElement *source, *decoder, *conv, *sink;

  XfburnTranscoderGstState state;
  GCond *gst_cond;
  GMutex *gst_mutex;
  gboolean is_audio;
  gint64 duration;

  GError *error;

  GSList *tracks;

} XfburnTranscoderGstPrivate;


typedef struct {
  int fd_in;
  off_t size;
} XfburnAudioTrackGst;

#define SIGNAL_WAIT_TIMEOUT_MICROS 600000

#define SIGNAL_SEND_ITERATIONS 10
#define SIGNAL_SEND_TIMEOUT_MICROS 400000

#define STATE_CHANGE_TIMEOUT_NANOS 250000000

#define XFBURN_AUDIO_TRACK_GET_GST(atrack) ((XfburnAudioTrackGst *) (atrack)->data)

/*********************/
/* class declaration */
/*********************/
static GObject *parent_class = NULL;
//static guint signals[LAST_SIGNAL];

GtkType
xfburn_transcoder_gst_get_type ()
{
  static GtkType type = 0;

  if (type == 0) {
    static const GTypeInfo our_info = {
      sizeof (XfburnTranscoderGstClass),
      NULL,
      NULL,
      (GClassInitFunc) xfburn_transcoder_gst_class_init,
      NULL,
      NULL,
      sizeof (XfburnTranscoderGst),
      0,
      (GInstanceInitFunc) xfburn_transcoder_gst_init,
      NULL
    };
    static const GInterfaceInfo trans_info = {
      (GInterfaceInitFunc) transcoder_interface_init,
      NULL,
      NULL
    };

    type = g_type_register_static (G_TYPE_OBJECT, "XfburnTranscoderGst", &our_info, 0);

    g_type_add_interface_static (type, XFBURN_TYPE_TRANSCODER, &trans_info);
  }

  return type;
}

static void
xfburn_transcoder_gst_class_init (XfburnTranscoderGstClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  g_type_class_add_private (klass, sizeof (XfburnTranscoderGstPrivate));
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = xfburn_transcoder_gst_finalize;

/*
  signals[VOLUME_CHANGED] = g_signal_new ("volume-changed", XFBURN_TYPE_TRANSCODER_GST, G_SIGNAL_ACTION,
                                          G_STRUCT_OFFSET (XfburnTranscoderGstClass, volume_changed),
                                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);
*/
}

static void
xfburn_transcoder_gst_init (XfburnTranscoderGst * obj)
{
  XfburnTranscoderGstPrivate *priv = XFBURN_TRANSCODER_GST_GET_PRIVATE (obj);

  create_pipeline (obj);

  /* the condition is used to signal that
   * gst has returned information */
  priv->gst_cond = g_cond_new ();

  /* if the mutex is locked, then we're not currently seeking
   * information from gst */
  priv->gst_mutex = g_mutex_new ();
  g_mutex_lock (priv->gst_mutex);
}

static void
xfburn_transcoder_gst_finalize (GObject * object)
{
  XfburnTranscoderGstPrivate *priv = XFBURN_TRANSCODER_GST_GET_PRIVATE (object);

  gst_element_set_state (priv->pipeline, GST_STATE_NULL);

  gst_object_unref (GST_OBJECT (priv->pipeline));
  priv->pipeline = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
transcoder_interface_init (XfburnTranscoderInterface *iface, gpointer iface_data)
{
  iface->get_name = get_name;
  iface->is_initialized = is_initialized;
  iface->get_audio_track = get_audio_track;
  iface->create_burn_track = create_burn_track;
  iface->free_burning_resources = free_burning_resources;
  iface->finish = finish;
  iface->prepare = prepare;
}

/*           */
/* internals */
/*           */

static void 
create_pipeline (XfburnTranscoderGst *trans)
{
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (trans);

  GstElement *pipeline, *source, *decoder, *conv, *sink;
  GstBus *bus;
  GstCaps *caps;

  priv->state = XFBURN_TRANSCODER_GST_STATE_IDLE;

  priv->pipeline = pipeline = gst_pipeline_new ("transcoder");

  priv->source  = source   = gst_element_factory_make ("filesrc",       "file-source");
  priv->decoder = decoder  = gst_element_factory_make ("decodebin",     "decoder");
  priv->conv    = conv     = gst_element_factory_make ("audioconvert",  "converter");
  priv->sink    = sink     = gst_element_factory_make ("fdsink",        "audio-output");
  //priv->sink    = sink     = gst_element_factory_make ("fakesink",        "audio-output");
  //DBG ("\npipeline = %p\nsource = %p\ndecoder = %p\nconv = %p\nsink = %p", pipeline, source, decoder, conv, sink);

  if (!pipeline || !source || !decoder || !conv || !sink) {
    g_set_error (&(priv->error), XFBURN_ERROR, XFBURN_ERROR_GST_CREATION,
                 _("A pipeline element could not be created"));
    return;
  }


  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_call, trans);
  gst_object_unref (bus);

  gst_bin_add_many (GST_BIN (pipeline),
                    source, decoder, conv, sink, NULL);

  gst_element_link (source, decoder);

  /* setup caps for raw pcm data */
  caps = gst_caps_new_simple ("audio/x-raw-int",
            "rate", G_TYPE_INT, 44100,
            "channels", G_TYPE_INT, 2,
            "endianness", G_TYPE_INT, G_LITTLE_ENDIAN,
            "width", G_TYPE_INT, 16,
            "depth", G_TYPE_INT, 16,
            "signed", G_TYPE_BOOLEAN, TRUE,
            NULL);

  if (!gst_element_link_filtered (conv, sink, caps)) {
    g_set_error (&(priv->error), XFBURN_ERROR, XFBURN_ERROR_GST_CREATION,
                 _("Could not setup filtered gstreamer link"));
    gst_caps_unref (caps);
    return;
  }
  gst_caps_unref (caps);

  g_signal_connect (decoder, "new-decoded-pad", G_CALLBACK (on_pad_added), conv);
}

static void 
delete_pipeline (XfburnTranscoderGst *trans)
{
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (trans);

  DBG ("Deleting pipeline");
  if (gst_element_set_state (priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE)
    g_warning ("Failed to change state to null, deleting pipeline anyways");
  
  /* give gstreamer a chance to do something */
  g_thread_yield ();

  g_object_unref (priv->pipeline);
  priv->pipeline = NULL;
}

static void recreate_pipeline (XfburnTranscoderGst *trans)
{
  delete_pipeline (trans);
  create_pipeline (trans);
}

static gchar *
state_to_str (GstState st)
{
  switch (st) {
    case GST_STATE_VOID_PENDING:
      return "void";
    case GST_STATE_NULL:
      return "null";
    case GST_STATE_READY:
      return "ready";
    case GST_STATE_PAUSED:
      return "paused";
    case GST_STATE_PLAYING:
      return "playing";
  }
  return "invalid";
}

static gboolean
bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
  XfburnTranscoderGst *trans = XFBURN_TRANSCODER_GST (data);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (trans);

  GstFormat fmt;
  guint secs;
  //gint64 frames;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS: {
      GError *error = NULL;

      DBG ("End of stream");

      if (!transcode_next_track (trans, &error)) {
        g_warning ("Error while switching track: %s", error->message);
        g_error_free (error);
      }

      break;
    }
    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;
      int i;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_warning ("Gstreamer error: %s\n", error->message);
      g_error_free (error);

      switch (priv->state) {
        case XFBURN_TRANSCODER_GST_STATE_IDLE:
          DBG ("Ignoring gstreamer error while idling.");
          break;
        case XFBURN_TRANSCODER_GST_STATE_IDENTIFYING:
          priv->is_audio = FALSE;
          DBG ("Trying to lock mutex (error)");
          for (i=0; i<SIGNAL_SEND_ITERATIONS; i++) {
            if (g_mutex_trylock (priv->gst_mutex))
              break;
            g_usleep (SIGNAL_SEND_TIMEOUT_MICROS / SIGNAL_SEND_ITERATIONS);
            if (i==9) {
              recreate_pipeline (trans);
              g_warning ("Noone was there to listen to the gstreamer error: %s", error->message);
            }
          }
          g_cond_signal (priv->gst_cond);
          g_mutex_unlock (priv->gst_mutex);
          DBG ("Releasing mutex (error)");

          recreate_pipeline (trans);
          break;
        case XFBURN_TRANSCODER_GST_STATE_TRANSCODE_START:
        case XFBURN_TRANSCODER_GST_STATE_TRANSCODING:
          g_error ("Gstreamer error while transcoding!");
          break;
      }
      break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
      GstState state;
      GstState pending, old_state;

      //if (GST_MESSAGE_SRC (msg) == GST_OBJECT (priv->sink))
        //DBG ("RIGHT SOURCE");
      //DBG ("source is %p", GST_MESSAGE_SRC (msg));

      gst_message_parse_state_changed (msg, &old_state, &state, &pending);
      if (pending != 0)
        DBG ("New state is %s, old is %s and pending is %s", state_to_str(state), state_to_str(old_state), state_to_str(pending));
      else
        DBG ("New state is %s, old is %s", state_to_str(state), state_to_str(old_state));

      switch (priv->state) {
        case XFBURN_TRANSCODER_GST_STATE_IDLE:
        case XFBURN_TRANSCODER_GST_STATE_TRANSCODING:
          DBG ("Not identifying, ignoring state change");
          break;
        case XFBURN_TRANSCODER_GST_STATE_IDENTIFYING:

          if (state != GST_STATE_PAUSED)
            break;
          
          if (!g_mutex_trylock (priv->gst_mutex)) {
            DBG ("Lock held by another thread, not doing anything");
            return TRUE;
          } else {
            DBG ("Locked mutex");
          }

          fmt = GST_FORMAT_TIME;
          if (!gst_element_query_duration (priv->pipeline, &fmt, &priv->duration)) {
            g_mutex_unlock (priv->gst_mutex);
            DBG ("Could not query stream length!");
            return TRUE;
          }

          secs = priv->duration / 1000000000;
          //DBG ("Length is %lldns = %ds = %lld bytes\n", priv->duration, secs, priv->duration * 176400 /1000000000);
          if (gst_element_set_state (priv->pipeline, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
            DBG ("Failed to set state!");
          }

          priv->is_audio = TRUE;
          g_cond_signal (priv->gst_cond);
          g_mutex_unlock (priv->gst_mutex);
          DBG ("Releasing mutex (success)");
          priv->state = XFBURN_TRANSCODER_GST_STATE_IDLE;

        case XFBURN_TRANSCODER_GST_STATE_TRANSCODE_START:
          if (state != GST_STATE_PLAYING)
            break;

          if (strcmp (GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)), "decoder") != 0)
            break;
          
          if (!g_mutex_trylock (priv->gst_mutex)) {
            DBG ("Lock held by another thread, can't signal transcoding start!");
            break;
          } else {
            DBG ("Locked mutex");
          }

          g_cond_signal (priv->gst_cond);
          g_mutex_unlock (priv->gst_mutex);
          break;
      }

      break;
    }
    default:
      DBG ("bus call: %s (%d) ", GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_TYPE (msg));
      break;
  }

  return TRUE;
}

static void
on_pad_added (GstElement *element, GstPad *pad, gboolean last, gpointer data)
{
  GstCaps *caps;
  GstStructure *str;
  GstPad *audiopad;
  GstElement *audio = (GstElement *) data;

  // only link once
  audiopad = gst_element_get_static_pad (audio, "sink");
  if (GST_PAD_IS_LINKED (audiopad)) {
    DBG ("pads are already linked!");
    g_object_unref (audiopad);
    return;
  }

  DBG ("linking pads");

  // check media type
  caps = gst_pad_get_caps (pad);
  str = gst_caps_get_structure (caps, 0);
  if (!g_strrstr (gst_structure_get_name (str), "audio")) {
    DBG ("not audio!");
    /* FIXME: report this as an error? */
    gst_caps_unref (caps);
    gst_object_unref (audiopad);
    return;
  }
  gst_caps_unref (caps);

  // link'n'play
  gst_pad_link (pad, audiopad);
}


static const gchar * 
get_name (XfburnTranscoder *trans)
{
  return "gstreamer";
}

static XfburnAudioTrack *
get_audio_track (XfburnTranscoder *trans, const gchar *fn, GError **error)
{
  XfburnTranscoderGst *tgst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (tgst);

  XfburnAudioTrack *atrack;
  XfburnAudioTrackGst *gtrack;
  GTimeVal tv;
  off_t size;

  priv->is_audio = FALSE;
  DBG ("setting filename for gstreamer");

  priv->state = XFBURN_TRANSCODER_GST_STATE_IDENTIFYING;
  g_object_set (G_OBJECT (priv->source), "location", fn, NULL);
  if (gst_element_set_state (priv->pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    g_warning ("Supposedly failed to change gstreamer state, ignoring it.");
    /*
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_GST_STATE,
                 _("Failed to change state!"));
    return NULL;
    */
  }
  DBG ("Waiting for signal");
  g_get_current_time (&tv);
  g_time_val_add (&tv, SIGNAL_WAIT_TIMEOUT_MICROS);
  if (!g_cond_timed_wait (priv->gst_cond, priv->gst_mutex, &tv)) {
    recreate_pipeline (tgst);
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_GST_TIMEOUT,
                 _("Gstreamer did not like this file (detection timed out)"));
    return NULL;
  }
  DBG ("Got a signal");

  if (!priv->is_audio) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_NOT_AUDIO_FORMAT,
                 _("This is not an audio file"));
    return NULL;
  }

  atrack = g_new0 (XfburnAudioTrack, 1);
  /* FIXME: when do we free inputfile?? */
  atrack->inputfile = g_strdup (fn);
  atrack->pos = -1;
  atrack->length = priv->duration / 1000000000;

  size = (priv->duration / (float) 1000000000) * (float) PCM_BYTES_PER_SECS;
  atrack->sectors = size / AUDIO_BYTES_PER_SECTORS;
  if (size % AUDIO_BYTES_PER_SECTORS > 0)
    atrack->sectors++;
  DBG ("Track length = %d secs => size = %.0f bytes => %d sectors", atrack->length, (float) size, atrack->sectors);

  gtrack = g_new0 (XfburnAudioTrackGst, 1);
  atrack->data = (gpointer) gtrack;

  gtrack->size = size;

  return atrack;
}



static struct burn_track *
create_burn_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error)
{
  XfburnTranscoderGst *gst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (gst);
  
  struct burn_track *track;

  XfburnAudioTrackGst *gtrack = XFBURN_AUDIO_TRACK_GET_GST (atrack);
  int pipe_fd[2];
  struct burn_source *src_fifo;

  if (pipe (pipe_fd) != 0) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_PIPE, g_strerror (errno));
    return NULL;
  }

  atrack->fd = pipe_fd[0];

  DBG ("track %d fd = %d", atrack->pos, atrack->fd);

  atrack->src = burn_fd_source_new (atrack->fd, -1 , 0);
  if (atrack->src == NULL) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_BURN_SOURCE,
                 _("Could not create burn_source from %s!"), atrack->inputfile);
    XFBURN_AUDIO_TRACK_DELETE_DATA (atrack);
    atrack->fd = -1;
    close (pipe_fd[0]);  close (pipe_fd[1]);
    return NULL;
  }
  
  /* install fifo,
    * its size will be a bit bigger in audio mode but that shouldn't matter */
  src_fifo = burn_fifo_source_new (atrack->src, AUDIO_BYTES_PER_SECTORS, xfburn_settings_get_int ("fifo-size", FIFO_DEFAULT_SIZE) / 2, 0);
  burn_source_free (atrack->src);
  atrack->src = src_fifo;


  track = burn_track_create ();
  
  if (burn_track_set_source (track, atrack->src) != BURN_SOURCE_OK) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_BURN_SOURCE,
                 _("Could not add source to track %s!"), atrack->inputfile);
    XFBURN_AUDIO_TRACK_DELETE_DATA (atrack);
    burn_source_free (atrack->src);
    atrack->fd = -1;
    close (pipe_fd[0]);  close (pipe_fd[1]);
    return NULL;
  }

  if (burn_track_set_size (track, gtrack->size) <= 0) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_BURN_TRACK,
                 _("Could not set track size for %s!"), atrack->inputfile);
    XFBURN_AUDIO_TRACK_DELETE_DATA (atrack);
    burn_source_free (atrack->src);
    atrack->fd = -1;
    close (pipe_fd[0]);  close (pipe_fd[1]);
    return NULL;
  }

  gtrack->fd_in = pipe_fd[1];

  //burn_track_set_byte_swap (track, TRUE);

  burn_track_define_data (track, 0, 0, 1, BURN_AUDIO);

  priv->tracks = g_slist_prepend (priv->tracks, atrack);

  return track;
}

static gboolean
prepare (XfburnTranscoder *trans, GError **error)
{
  XfburnTranscoderGst *gst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (gst);
  gboolean ret;
  GTimeVal tv;

  priv->tracks = g_slist_reverse (priv->tracks);

  priv->state = XFBURN_TRANSCODER_GST_STATE_TRANSCODE_START;
  ret = transcode_next_track (gst, error);

  DBG ("Waiting for start signal");
  g_get_current_time (&tv);
  g_time_val_add (&tv, SIGNAL_WAIT_TIMEOUT_MICROS);
  if (!g_cond_timed_wait (priv->gst_cond, priv->gst_mutex, &tv)) {
    recreate_pipeline (gst);
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_GST_TIMEOUT,
                 _("Gstreamer did not want to start transcoding (timed out)"));
    return FALSE;
  }
  DBG ("Got the start signal");

  priv->state = XFBURN_TRANSCODER_GST_STATE_TRANSCODING;

  /* give gstreamer a tiny bit of time.
   * FIXME: what's a good time here?    
   *  or rather, we should wait for the state change... */
  g_usleep (100000);

  return ret;
}

static gboolean
transcode_next_track (XfburnTranscoderGst *trans, GError **error)
{
  XfburnTranscoderGst *gst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (gst);

  XfburnAudioTrack *atrack;
  XfburnAudioTrackGst *gtrack;

  if (!priv->tracks)
    /* we're done transcoding, so just return without error */
    return TRUE;

  atrack = (XfburnAudioTrack *) priv->tracks->data;
  gtrack = XFBURN_AUDIO_TRACK_GET_GST (atrack);

  g_object_set (G_OBJECT (priv->source), "location", atrack->inputfile, NULL);
  g_object_set (G_OBJECT (priv->sink), "fd", gtrack->fd_in, NULL);
  DBG ("now transcoding %s -> %d", atrack->inputfile, gtrack->fd_in);

  if (gst_element_set_state (priv->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_GST_STATE,
                 _("Failed to change songs while transcoding"));
    return FALSE;
  }

  priv->tracks = g_slist_next (priv->tracks);

  return TRUE;
}

static void 
finish (XfburnTranscoder *trans)
{
  XfburnTranscoderGst *gst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (gst);

  GstState state;
  GstClock *clock;
  GstClockTime tv;

  DBG ("Done transcoding!");
  priv->state = XFBURN_TRANSCODER_GST_STATE_IDLE;

  clock = gst_element_get_clock (priv->pipeline);
  tv = gst_clock_get_time (clock);
  g_object_unref (clock);
  tv += STATE_CHANGE_TIMEOUT_NANOS;

  if (gst_element_get_state (priv->pipeline, &state, NULL, tv) == GST_STATE_CHANGE_FAILURE) {
    DBG ("Could not query pipeline state, recreating it");
    recreate_pipeline (gst);
    return;
  }
  
  if ((state != GST_STATE_READY) &&
      (gst_element_set_state (priv->pipeline, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE)) {
    DBG ("Could not make pipeline ready, recreating it");
    recreate_pipeline (gst);
  }
}

static gboolean
free_burning_resources (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error)
{
  /*
  XfburnTranscoderGst *gst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (gst);
  */
  
  XfburnAudioTrackGst *gtrack = XFBURN_AUDIO_TRACK_GET_GST (atrack);
  
  close (gtrack->fd_in);

  g_free (gtrack);
  atrack->data = NULL;

  return TRUE;
}


static gboolean 
is_initialized (XfburnTranscoder *trans, GError **error)
{
  XfburnTranscoderGst *gst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (gst);

  if (priv->error) {
    *error = priv->error;
    return FALSE;
  }

  return TRUE;
}




/*        */
/* public */
/*        */

GObject *
xfburn_transcoder_gst_new ()
{
  return g_object_new (XFBURN_TYPE_TRANSCODER_GST, NULL);
}
#endif /* HAVE_GST */
