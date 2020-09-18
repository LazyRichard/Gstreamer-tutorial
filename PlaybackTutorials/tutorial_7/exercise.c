#include <gst/gst.h>

typedef struct _CustomAudioSinkBin {
  GstElement *bin;
  GstElement *equalizer;
  GstElement *convert;
  GstElement *sink;
  GstPad *ghost_pad;
} CustomAudioSinkBin;

typedef struct _CustomVideoSinkBin {
  GstElement *bin;
  GstElement *filter;
  GstElement *convert;
  GstElement *sink;
  GstPad *ghost_pad;
} CustomVideoSinkBin;

static void cb_message(GstBus *, GstMessage *, GstElement *);

int main(int argc, char *argv[]) {
  GstElement *pipeline;
  CustomAudioSinkBin audio_bin;
  CustomVideoSinkBin video_bin;
  GstPad *audio_pad;
  GstPad *video_pad;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gchar *uri = "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Build the pipeline */
  pipeline = gst_parse_launch(uri, NULL);

  /* Create elements for audio sink bin */
  audio_bin.bin = gst_bin_new("audio_sink_bin");
  audio_bin.equalizer = gst_element_factory_make("equalizer-3bands", "equalizer");
  audio_bin.convert = gst_element_factory_make("audioconvert", "audio_convert");
  audio_bin.sink = gst_element_factory_make("autoaudiosink", "audio_sink");

  if (!audio_bin.equalizer || !audio_bin.convert || !audio_bin.sink) {
    g_error("Not all audio_bin's elements could be created.");
  }

  /* Create elements for video sink bin */
  video_bin.bin = gst_bin_new("video_sink_bin");
  video_bin.filter = gst_element_factory_make("vertigotv", "video_filter");
  video_bin.convert = gst_element_factory_make("videoconvert", "video_convert");
  video_bin.sink = gst_element_factory_make("autovideosink", "video_sink");

  if (!video_bin.convert || !video_bin.sink) {
    g_error("Not all video_bin's elements could be created.");
  }

  /* Add the elements to audio_bin and link them */
  gst_bin_add_many(GST_BIN(audio_bin.bin), audio_bin.equalizer, audio_bin.convert, audio_bin.sink, NULL);
  gst_element_link_many(audio_bin.equalizer, audio_bin.convert, audio_bin.sink, NULL);
  audio_pad = gst_element_get_static_pad(audio_bin.equalizer, "sink");
  audio_bin.ghost_pad = gst_ghost_pad_new("sink", audio_pad);
  gst_pad_set_active(audio_bin.ghost_pad, TRUE);
  gst_element_add_pad(audio_bin.bin, audio_bin.ghost_pad);
  gst_object_unref(audio_pad);

  /* Configure the equalizer */
  g_object_set(G_OBJECT(audio_bin.equalizer), "band1", (gdouble)-24.0, NULL);
  g_object_set(G_OBJECT(audio_bin.equalizer), "band2", (gdouble)-24.0, NULL);

  /* Add the elements to video_bin and link them */
  gst_bin_add_many(GST_BIN(video_bin.bin), video_bin.filter, video_bin.convert, video_bin.sink, NULL);
  gst_element_link_many(video_bin.filter, video_bin.convert, video_bin.sink, NULL);
  video_pad = gst_element_get_static_pad(video_bin.filter, "sink");
  video_bin.ghost_pad = gst_ghost_pad_new("sink", video_pad);
  gst_pad_set_active(video_bin.ghost_pad, TRUE);
  gst_element_add_pad(video_bin.bin, video_bin.ghost_pad);
  gst_object_unref(video_pad);

  /* Set playbin's audio sink to be our sink bin */
  g_object_set(GST_OBJECT(pipeline), "audio-sink", audio_bin.bin, NULL);
  g_object_set(GST_OBJECT(pipeline), "video-sink", video_bin.bin, NULL);

  /* Start playing */
  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Cannot play");

    gst_object_unref(pipeline);

    return -1;
  }

  /* Wait until error or EOS */
  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  cb_message(bus, msg, pipeline);

  /* Free resource */
  if (msg != NULL)
    gst_message_unref(msg);

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}

static void cb_message(GstBus *bus, GstMessage *msg, GstElement *pipeline) {
  switch (GST_MESSAGE_TYPE(msg)) {
  case GST_MESSAGE_ERROR:
    GError *err;
    gchar *debug;

    gst_message_parse_error(msg, &err, &debug);
    g_printerr("Error: %s\n", err->message);
    g_error_free(err);
    g_free(debug);

    gst_element_set_state(pipeline, GST_STATE_READY);
    break;
  case GST_MESSAGE_EOS:
    g_message("End=Of-Stream reached.");
    break;

  default:
    g_warn_if_reached();
    break;
  }
}