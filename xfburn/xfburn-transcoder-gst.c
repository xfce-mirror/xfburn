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
#include <math.h>

#include <errno.h>

#include <libxfce4util/libxfce4util.h>

#include <libburn.h>

#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#include <gst/pbutils/pbutils.h>

#include "xfburn-global.h"
#include "xfburn-error.h"
#include "xfburn-settings.h"

#include "xfburn-transcoder-gst.h"
#include "xfburn-audio-track.h"
#include "xfburn-audio-track-gst.h"


/* 
 * Don't define it for no debugging output (don't set to 0, I was too lazy to clean up).
 * Set DEBUG_GST >= 1 to be able to inspect the data just before it gets to the fd,
 *                    and to get a lot more gst debugging output.
 * Set DEBUG_GST >= 2 to also get notification of all bus messages.
 * Set DEBUG_GST >= 3 to also get a lot of gst state change messages, as well as seing how fast data passes through gst
 */
#define DEBUG_GST 2


/** Prototypes **/
/* class initialization */
static void xfburn_transcoder_gst_finalize (GObject * object);
static void transcoder_interface_init (XfburnTranscoderInterface *iface, gpointer iface_data);

/* internals */
static void create_pipeline (XfburnTranscoderGst *trans);
static void delete_pipeline (XfburnTranscoderGst *trans);
static void recreate_pipeline (XfburnTranscoderGst *trans);
static const gchar * get_name (XfburnTranscoder *trans);
static const gchar * get_description (XfburnTranscoder *trans);
static gboolean get_audio_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);

static struct burn_track * create_burn_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error);

static gboolean prepare (XfburnTranscoder *trans, GError **error);
static gboolean transcode_next_track (XfburnTranscoderGst *trans, GError **error);
static void finish (XfburnTranscoder *trans);

static gboolean is_initialized (XfburnTranscoder *trans, GError **error);

/* gstreamer support functions */
static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data);
static void on_pad_added (GstElement *element, GstPad *pad, gpointer data);

#ifdef DEBUG_GST
static void cb_handoff (GstElement *element, GstBuffer *buffer, gpointer data);
#endif

#define XFBURN_TRANSCODER_GST_GET_PRIVATE(obj) (xfburn_transcoder_gst_get_instance_private ( XFBURN_TRANSCODER_GST (obj)))

enum {
  LAST_SIGNAL,
}; 

typedef enum {
  XFBURN_TRANSCODER_GST_STATE_IDLE,
  XFBURN_TRANSCODER_GST_STATE_TRANSCODE_START,
  XFBURN_TRANSCODER_GST_STATE_TRANSCODING,
} XfburnTranscoderGstState;

typedef struct {
  GstElement *pipeline;
  GstElement *source, *decoder, *resample, *conv1, *conv2, *sink;

  XfburnTranscoderGstState state;
  GCond gst_cond;
  GMutex gst_mutex;
  gboolean gst_done;
  gint64 duration;

  GstDiscoverer *discoverer;

  GError *error;

  GSList *tracks;
  XfburnAudioTrack *curr_track;

} XfburnTranscoderGstPrivate;


/* constants */

#define SIGNAL_WAIT_TIMEOUT_MS 1500

#define SIGNAL_SEND_ITERATIONS 10
/* SIGNAL_SEND_TIMEOUT_MICROS is the total time,
 * which gets divided into SIGNAL_SEND_ITERATIONS probes */
#define SIGNAL_SEND_TIMEOUT_MICROS 1500000

#define STATE_CHANGE_TIMEOUT_NANOS 750000000

#define XFBURN_AUDIO_TRACK_GET_GST(atrack) ((XfburnAudioTrackGst *) (atrack)->data)

/* globals */
#if DEBUG_GST > 0 && DEBUG > 0
static guint64 total_size = 0;
#endif

static const gchar *errormsg_gst_setup = N_("An error occurred setting gstreamer up for transcoding");
static const gchar *errormsg_libburn_setup = N_("An error occurred while setting the burning backend up");
static const gchar *errormsg_missing_plugin = N_("%s is missing.\n"
                                              "\n"
                                              "You do not have a decoder installed to handle this file.\n"
                                              "Probably you need to look at the gst-plugins-* packages\n"
                                              "for the necessary plugins.\n");

/*********************/
/* class declaration */
/*********************/
static GObject *parent_class = NULL;
//static guint signals[LAST_SIGNAL];

G_DEFINE_TYPE_EXTENDED(
  XfburnTranscoderGst,
  xfburn_transcoder_gst,
  G_TYPE_OBJECT,
  0,
  G_ADD_PRIVATE(XfburnTranscoderGst)
  G_IMPLEMENT_INTERFACE(XFBURN_TYPE_TRANSCODER, transcoder_interface_init)
);

static void
xfburn_transcoder_gst_class_init (XfburnTranscoderGstClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
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
  g_cond_init (&priv->gst_cond);

  /* if the mutex is locked, then we're not currently seeking
   * information from gst */
  g_mutex_init (&priv->gst_mutex);
  /* Actual lock acquired in prepare() and relased in finish() */

  priv->discoverer = gst_discoverer_new(GST_SECOND, NULL);
}

static void
xfburn_transcoder_gst_finalize (GObject * object)
{
  XfburnTranscoderGstPrivate *priv = XFBURN_TRANSCODER_GST_GET_PRIVATE (object);

  gst_element_set_state (priv->pipeline, GST_STATE_NULL);

  gst_object_unref (GST_OBJECT (priv->pipeline));
  g_object_unref (G_OBJECT (priv->discoverer));
  priv->pipeline = NULL;

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
  /* free_burning_resources is not needed */
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

  GstElement *pipeline, *source, *decoder, *resample, *conv1, *conv2, *sink;
#if DEBUG_GST > 0 && DEBUG > 0
  GstElement *id;
#endif
  GstBus *bus;
  GstCaps *caps;

  priv->state = XFBURN_TRANSCODER_GST_STATE_IDLE;

  priv->pipeline = pipeline = gst_pipeline_new ("transcoder");

  priv->source   = source   = gst_element_factory_make ("filesrc",       "file-source");
  priv->decoder  = decoder  = gst_element_factory_make ("decodebin",     "decoder");
  priv->conv1    = conv1    = gst_element_factory_make ("audioconvert",  "converter1");
  priv->resample = resample = gst_element_factory_make ("audioresample", "resampler");
  priv->conv2    = conv2    = gst_element_factory_make ("audioconvert",  "converter2");
#if DEBUG_GST > 0 && DEBUG > 0
                  id       = gst_element_factory_make ("identity",      "debugging-identity");
#endif
  priv->sink    = sink     = gst_element_factory_make ("fdsink",        "audio-output");
  //priv->sink    = sink     = gst_element_factory_make ("fakesink",        "audio-output");

  if (!pipeline || !source || !decoder || !resample || !conv1 || !conv2 || !sink) {
    g_warning ("A pipeline element could not be created");
    g_set_error (&(priv->error), XFBURN_ERROR, XFBURN_ERROR_GST_CREATION,
		    "%s",
                 _(errormsg_gst_setup));
    return;
  }

#if DEBUG_GST > 0 && DEBUG > 0
  if (!id) {
    g_warning ("The debug identity element could not be created");
    g_set_error (&(priv->error), XFBURN_ERROR, XFBURN_ERROR_GST_CREATION,
		    "%s",
                 _(errormsg_gst_setup));
    return;
  }
#endif


  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_call, trans);
  gst_object_unref (bus);

  gst_bin_add_many (GST_BIN (pipeline),
                    source, decoder, conv1, resample, conv2, sink, NULL);
#if DEBUG_GST > 0 && DEBUG > 0
  gst_bin_add (GST_BIN (pipeline), id);
#endif

  gst_element_link (source, decoder);
  gst_element_link (conv1, resample);
  gst_element_link (resample, conv2);

  /* setup caps for raw pcm data */
  caps = gst_caps_new_simple ("audio/x-raw",
            "rate", G_TYPE_INT, 44100,
            "channels", G_TYPE_INT, 2,
            "format", G_TYPE_STRING, "S16LE",
            NULL);

#if DEBUG_GST > 0 && DEBUG > 0
  // TODO without the filter it does work but I'm not sure the audio format is correct
  if (!gst_element_link_filtered (conv2, id, caps)) {
#else
  if (!gst_element_link_filtered (conv2, sink, caps)) {
#endif
    g_warning ("Could not setup filtered gstreamer link");
    g_set_error (&(priv->error), XFBURN_ERROR, XFBURN_ERROR_GST_CREATION,
		    "%s",
                 _(errormsg_gst_setup));
    gst_caps_unref (caps);
    return;
  }
  gst_caps_unref (caps);
#if DEBUG_GST > 0 && DEBUG > 0
  gst_element_link (id, sink);
  g_signal_connect (id, "handoff", G_CALLBACK (cb_handoff), id);
#endif

  g_signal_connect (decoder, "pad-added", G_CALLBACK (on_pad_added), trans);
}

static void 
delete_pipeline (XfburnTranscoderGst *trans)
{
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (trans);

#if DEBUG_GST > 0
  DBG ("Deleting pipeline");
#endif
  if (gst_element_set_state (priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE)
    g_warning ("Failed to change state to null, deleting pipeline anyways");
  
  /* give gstreamer a chance to do something
   * this might not be necessary, but shouldn't hurt */
  g_thread_yield ();

  g_object_unref (priv->pipeline);
  priv->pipeline = NULL;
}

static void 
recreate_pipeline (XfburnTranscoderGst *trans)
{
  delete_pipeline (trans);
  create_pipeline (trans);
}

#if DEBUG_GST > 2
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
#endif

static gboolean
bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
  XfburnTranscoderGst *trans = XFBURN_TRANSCODER_GST (data);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (trans);

#if DEBUG_GST > 1
  DBG ("msg from %-20s: %s (%d) ", GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)), GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_TYPE (msg));
#endif

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS: {
      GError *error = NULL;
      XfburnAudioTrackGst *gtrack = XFBURN_AUDIO_TRACK_GET_GST (priv->curr_track);

#if DEBUG_GST > 0
  #if DEBUG > 0
      DBG ("End of stream, wrote %.0f bytes", (gfloat) total_size);
  #else
      g_message ("End of stream.");
  #endif
#endif

      close (gtrack->fd_in);

      if (gst_element_set_state (priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
        g_warning ("Gstreamer did not want to get ready after EOS!");
      }

      if (!transcode_next_track (trans, &error)) {
        g_warning ("Error while switching track: %s", error->message);
        g_error_free (error);
        /* FIXME: abort here? */
      }

      break;
    }

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      switch (priv->state) {
        case XFBURN_TRANSCODER_GST_STATE_IDLE:
#if DEBUG_GST > 0
          g_warning ("Ignoring gstreamer error while idling: %s", error->message);
#endif
          g_error_free (error);
          recreate_pipeline (trans);
          break;

        case XFBURN_TRANSCODER_GST_STATE_TRANSCODE_START:
        case XFBURN_TRANSCODER_GST_STATE_TRANSCODING:
          g_error ("Gstreamer error while transcoding: %s", error->message);
          g_error_free (error);
          break;
      }
      break;
    }

    case GST_MESSAGE_APPLICATION: {
      if (gst_element_set_state (priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
        DBG ("Failed to reset GST state.");
        recreate_pipeline (trans);
      }
      DBG("GST recognized an application, ignoring.");

      break;
    }

    case GST_MESSAGE_STATE_CHANGED: {
      GstState state, pending, old_state;

      gst_message_parse_state_changed (msg, &old_state, &state, &pending);

/* this is very verbose */
#if DEBUG_GST > 2
      if (pending != 0)
        DBG ("%-15s\tNew state is %s, old is %s and pending is %s", GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)), state_to_str(state), state_to_str(old_state), state_to_str(pending));
      else
        DBG ("%-15s\tNew state is %s, old is %s", GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)), state_to_str(state), state_to_str(old_state));
#endif

      switch (priv->state) {
        case XFBURN_TRANSCODER_GST_STATE_IDLE:
        case XFBURN_TRANSCODER_GST_STATE_TRANSCODING:
          break;

        case XFBURN_TRANSCODER_GST_STATE_TRANSCODE_START:
          if (state != GST_STATE_PLAYING)
            break;

          if (strcmp (GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)), "decoder") != 0)
            break;

          priv->gst_done = TRUE;
          g_cond_signal (&priv->gst_cond);
          break;
      } /* switch of priv->state */

      break;
    }

    case GST_MESSAGE_DURATION_CHANGED: {

      switch (priv->state) {
        case XFBURN_TRANSCODER_GST_STATE_IDLE:
        case XFBURN_TRANSCODER_GST_STATE_TRANSCODING:
        case XFBURN_TRANSCODER_GST_STATE_TRANSCODE_START:
          break;

      } /* switch of priv->state */

      break;
    }

    case GST_MESSAGE_ELEMENT: {
      /* This should not happen now that the discoverer API is used to identify songs.
       * Shouldn't hurt to keep around though. */
      if (gst_is_missing_plugin_message (msg)) {
          recreate_pipeline (trans);

          g_set_error (&(priv->error), XFBURN_ERROR, XFBURN_ERROR_MISSING_PLUGIN,
                       _(errormsg_missing_plugin),
                        gst_missing_plugin_message_get_description (msg)); 
      }
    }
    default:
#if DEBUG_GST == 1
      DBG ("msg from %-20s: %s (%d) ", GST_OBJECT_NAME (GST_MESSAGE_SRC (msg)), GST_MESSAGE_TYPE_NAME (msg), GST_MESSAGE_TYPE (msg));
#endif
      break;
  }

  return TRUE;
}

static void
on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
  XfburnTranscoderGst *trans = XFBURN_TRANSCODER_GST (data);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (trans);

  GstCaps *caps;
  GstStructure *str;
  GstPad *audiopad;
  GstElement *audio = (GstElement *) priv->conv1;

  // only link once
  audiopad = gst_element_get_static_pad (audio, "sink");
  if (GST_PAD_IS_LINKED (audiopad)) {
    DBG ("pads are already linked!");
    g_object_unref (audiopad);
    return;
  }

#if DEBUG_GST > 0
  DBG ("linking pads");
#endif

  // check disc type
  caps = gst_pad_query_caps (pad, NULL);
  str = gst_caps_get_structure (caps, 0);
  if (!g_strrstr (gst_structure_get_name (str), "audio")) {
    GstStructure *msg_struct;
    GstMessage *msg;
    GstBus *bus;

    const gchar *error_msg = N_("File content has a decoder but is not audio.");

    DBG ("%s", error_msg);
    
    gst_caps_unref (caps);
    gst_object_unref (audiopad);

    g_set_error (&(priv->error), XFBURN_ERROR, XFBURN_ERROR_GST_NO_AUDIO,
		    "%s",
                 _(error_msg));
    
    msg_struct = gst_structure_new_empty ("no-audio-content");

    msg = gst_message_new_application (GST_OBJECT (element), msg_struct);

    bus = gst_element_get_bus (element);
    if (!gst_bus_post (bus, msg)) {
      DBG ("Could not post the message on the gst bus!");
      g_object_unref (msg);
    }
    g_object_unref (bus);

    return;
  }
  gst_caps_unref (caps);

  // link'n'play
  gst_pad_link (pad, audiopad);
}


static const gchar * 
get_name (XfburnTranscoder *trans)
{
  /* Note to translators: you can probably keep this as gstreamer,
   * unless you have a good reason to call it by another name that
   * the user would understand better */
  return _("gstreamer");
}

static const gchar * 
get_description (XfburnTranscoder *trans)
{
  return _("The gstreamer transcoder uses the gstreamer\n"
           "library for creating audio compositions.\n"
           "\n"
           "Essentially all audio files should be supported\n"
           "given that the correct plugins are installed.\n"
           "If an audio file is not recognized, make sure\n"
           "that you have the 'good','bad', and 'ugly'\n"
           "gstreamer plugin packages installed.");
}

#if DEBUG_GST > 0 && DEBUG > 0

/* this function can inspect the data just before it is passed on
   to the fd for processing by libburn */
static void
cb_handoff (GstElement *element, GstBuffer *buffer, gpointer data)
{
  guint size = gst_buffer_get_size (buffer);
#if DEBUG_GST > 2
  static int i = 0;
  const int step = 30;
#endif

  total_size += size;
#if DEBUG_GST > 2
  if (++i % step == 0)
    DBG ("gstreamer just processed ~%6d bytes (%8.0f bytes total).", size*step, (float)total_size);
#endif
}

#endif

static gchar *
get_discoverer_required_plugins_message (GstDiscovererInfo *info)
{
  GString *str;
  gchar **plugins;
  gchar *plugins_str;

  plugins = (gchar **) gst_discoverer_info_get_missing_elements_installer_details (info);

  if (g_strv_length(plugins) == 0) {
    str = g_string_new (_("No information available on which plugin is required."));
  } else {
    str = g_string_new (_("Required plugins: "));
    plugins_str = g_strjoinv (", ", plugins);
    g_string_append (str, plugins_str);
    g_free (plugins_str);
  }

  return g_string_free (str, FALSE);
}

static gboolean
get_audio_track (XfburnTranscoder *trans, XfburnAudioTrack *atrack, GError **error)
{
  XfburnTranscoderGst *tgst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (tgst);

  XfburnAudioTrackGst *gtrack;
  off_t size;
  GstDiscovererInfo *info;
  GstDiscovererResult result;
  gchar *uri;

#if DEBUG_GST > 0
  DBG ("Querying GST about %s", atrack->inputfile);
#endif

  uri = g_strdup_printf("file://%s", atrack->inputfile);
  info = gst_discoverer_discover_uri(priv->discoverer, uri, error);
  g_free(uri);

  if (info == NULL) {
    if (error && *error == NULL) {
        DBG ("no info and no error");
        g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_GST_DISCOVERER,
                    _("An error occurred while identifying '%s' with gstreamer"), atrack->inputfile);
    }
    return FALSE;
  }

  result = gst_discoverer_info_get_result(info);
  switch (result) {
    case GST_DISCOVERER_MISSING_PLUGINS:
      g_clear_error (error);
      /*
      g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_GST_DISCOVERER,
                  _("%s: %s"), atrack->inputfile, get_discoverer_required_plugins_message (info));
      */
      // the message is pretty useless, so print it only on the console, and create our own instead.
      DBG ("%s", get_discoverer_required_plugins_message (info));
      g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_MISSING_PLUGIN,
                   _(errormsg_missing_plugin),
                   _("A plugin"));

      gst_discoverer_info_unref(info);
      return FALSE;
    case GST_DISCOVERER_OK:
      break;
    default:
      gst_discoverer_info_unref(info);
      DBG ("gst discoverer said %d", result);
      /* TODO: improve error messages */
      //recreate_pipeline (tgst);
      if (error && *error == NULL)
        g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_GST_DISCOVERER,
                     _("An error occurred while identifying '%s' with gstreamer"), atrack->inputfile);
      else
        g_prefix_error (error, "%s: ", atrack->inputfile);
      return FALSE;
  }

  priv->gst_done = FALSE;

  priv->duration = gst_discoverer_info_get_duration(info);
  atrack->length = priv->duration / 1000000000;

  size = (off_t) floorf (priv->duration * (PCM_BYTES_PER_SECS / (float) 1000000000));
  atrack->sectors = size / AUDIO_BYTES_PER_SECTOR;
  if (size % AUDIO_BYTES_PER_SECTOR > 0)
    atrack->sectors++;
  DBG ("Track length = %4d secs => size = %9.0f bytes => %5d sectors", atrack->length, (float) size, atrack->sectors);

  gtrack = g_new0 (XfburnAudioTrackGst, 1);
  atrack->data = (gpointer) gtrack;
  atrack->type = XFBURN_TYPE_AUDIO_TRACK_GST;

  gtrack->size = size;

  gst_discoverer_info_unref(info);

  return TRUE;
}


/* initialize the XfburnAudioTrack */
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
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_PIPE, "%s", g_strerror (errno));
    return NULL;
  }

  atrack->fd = pipe_fd[0];

  //DBG ("track %d fd = %d", atrack->pos, atrack->fd);

  atrack->src = burn_fd_source_new (atrack->fd, -1 , gtrack->size);
  if (atrack->src == NULL) {
    g_warning ("Could not create burn_source from %s.", atrack->inputfile);
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_BURN_SOURCE,
		 "%s",
                 _(errormsg_libburn_setup));
    XFBURN_AUDIO_TRACK_DELETE_DATA (atrack);
    atrack->fd = -1;
    close (pipe_fd[0]);  close (pipe_fd[1]);
    return NULL;
  }
  
  /* install fifo,
    * its size will be a bit bigger in audio mode but that shouldn't matter */
  src_fifo = burn_fifo_source_new (atrack->src, AUDIO_BYTES_PER_SECTOR, xfburn_settings_get_int ("fifo-size", FIFO_DEFAULT_SIZE) / 2, 0);
  burn_source_free (atrack->src);
  atrack->src = src_fifo;


  track = burn_track_create ();
  
  if (burn_track_set_source (track, atrack->src) != BURN_SOURCE_OK) {
    g_warning ("Could not add source to track %s.", atrack->inputfile);
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_BURN_SOURCE,
		 "%s",
                 _(errormsg_libburn_setup));
    XFBURN_AUDIO_TRACK_DELETE_DATA (atrack);
    burn_source_free (atrack->src);
    atrack->fd = -1;
    close (pipe_fd[0]);  close (pipe_fd[1]);
    return NULL;
  }

  gtrack->fd_in = pipe_fd[1];

  /* FIXME: I don't think this will be necessary with gstreamer, or will it be? */
  //burn_track_set_byte_swap (track, TRUE);

#ifdef DEBUG_NULL_DEVICE
  /* stdio:/dev/null only works with MODE1 */
  burn_track_define_data (track, 0, 0, 1, BURN_MODE1);
#else
  burn_track_define_data (track, 0, 0, 1, BURN_AUDIO);
#endif

  priv->tracks = g_slist_prepend (priv->tracks, atrack);

  return track;
}

static gboolean
prepare (XfburnTranscoder *trans, GError **error)
{
  XfburnTranscoderGst *gst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (gst);
  gboolean ret;
  gint64 end_time;

  g_mutex_lock(&priv->gst_mutex);

  priv->tracks = g_slist_reverse (priv->tracks);

  priv->state = XFBURN_TRANSCODER_GST_STATE_TRANSCODE_START;
  ret = transcode_next_track (gst, error);

  //DBG ("Waiting for start signal");
  end_time = g_get_monotonic_time () + SIGNAL_WAIT_TIMEOUT_MS * G_TIME_SPAN_MILLISECOND;
  while (!priv->gst_done)
    if (!g_cond_wait_until (&priv->gst_cond, &priv->gst_mutex, end_time)) {
      recreate_pipeline (gst);
      g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_GST_TIMEOUT,
                  _("Gstreamer did not want to start transcoding (timed out)"));
      return FALSE;
    }
  //DBG ("Got the start signal");

  priv->state = XFBURN_TRANSCODER_GST_STATE_TRANSCODING;

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
#if DEBUG_GST > 0
  DBG ("now transcoding %s -> %d", atrack->inputfile, gtrack->fd_in);
#endif

  if (gst_element_set_state (priv->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_warning ("Failed to change songs.");
    g_set_error (error, XFBURN_ERROR, XFBURN_ERROR_GST_STATE,
                 _("Failed to change songs while transcoding"));
    return FALSE;
  }

  priv->curr_track = (XfburnAudioTrack *) priv->tracks->data;
  priv->tracks = g_slist_next (priv->tracks);

  return TRUE;
}

static void 
finish (XfburnTranscoder *trans)
{
  XfburnTranscoderGst *gst = XFBURN_TRANSCODER_GST (trans);
  XfburnTranscoderGstPrivate *priv= XFBURN_TRANSCODER_GST_GET_PRIVATE (gst);

  /*
  GstState state;
  GstClock *clock;
  GstClockTime tv;
  */

#if DEBUG_GST > 0
  DBG ("Done transcoding");
#endif
  priv->state = XFBURN_TRANSCODER_GST_STATE_IDLE;

  priv->curr_track = NULL;

  /*
   * gstreamer doesn't even want to work with us again after getting back into
   * the ready state. Lame. So we just recreate the pipeline.
   * Which doesn't help either. Oh Well.
   * FIXME: how to get gstreamer to accept state changes again after an aborted burn run?
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
      (gst_element_set_state (priv->pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE)) {
    DBG ("Could not make pipeline ready, recreating it");
    recreate_pipeline (gst);
  }
  */
  recreate_pipeline (gst);

  g_mutex_unlock (&priv->gst_mutex);
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
xfburn_transcoder_gst_new (void)
{
  return g_object_new (XFBURN_TYPE_TRANSCODER_GST, NULL);
}
#endif /* HAVE_GST */
