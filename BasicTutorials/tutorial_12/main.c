#include <gst/gst.h>

typedef struct _CustomData {
  gboolean is_live;
  GstElement *pipeline;
  GMainLoop *loop;
} CustomData;

static void cb_message(GstBus *, GstMessage *, CustomData *);

int main(int argc, char *argv[]) {
  GstElement *pipeline;
  GstBus *bus;
  CustomData data;
  gchar *uri = "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Initialize our data structure */
  memset(&data, 0, sizeof(data));

  /* Build the pipeline */
  pipeline = gst_parse_launch(uri, NULL);
  bus = gst_element_get_bus(pipeline);

  /* Start playing */
  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_error("Unable to set the pipeline to the playing state.\n");
    gst_object_unref(pipeline);
    return -1;
  } else if (ret == GST_STATE_CHANGE_NO_PREROLL) {
    data.is_live = TRUE;
  }

  data.loop = g_main_loop_new(NULL, FALSE);
  data.pipeline = pipeline;

  gst_bus_add_signal_watch(bus);
  g_signal_connect(bus, "message", G_CALLBACK(cb_message), &data);

  g_main_loop_run(data.loop);

  /* Free resource */
  g_main_loop_unref(data.loop);
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}

static void cb_message(GstBus *bus, GstMessage *msg, CustomData *data) {
  switch (GST_MESSAGE_TYPE(msg)) {
  case GST_MESSAGE_ERROR:
    GError *err;
    gchar *debug;

    gst_message_parse_error(msg, &err, &debug);
    g_error("Error: %s", err->message);
    g_error_free(err);
    g_free(debug);

    gst_element_set_state(data->pipeline, GST_STATE_READY);
    g_main_loop_quit(data->loop);

    break;
  case GST_MESSAGE_EOS:
    /* end-of-stream */
    gst_element_set_state(data->pipeline, GST_STATE_READY);
    g_main_loop_quit(data->loop);

    break;

  case GST_MESSAGE_BUFFERING:
    gint percent = 0;

    /* If the stream is live, we do not care about buffering */
    if (data->is_live)
      break;

    gst_message_parse_buffering(msg, &percent);
    g_message("Buffering (%3d%%)", percent);

    /* Wait until buffering is complete before start/resume playing */
    if (percent < 100)
      gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
    else
      gst_element_set_state(data->pipeline, GST_STATE_PLAYING);

    break;

  case GST_MESSAGE_CLOCK_LOST:
    /* Get a new clock */
    gst_element_set_state(data->pipeline, GST_STATE_PAUSED);
    gst_element_set_state(data->pipeline, GST_STATE_PLAYING);

    break;

  default:
    g_warning("Unexpected message type detected! %d %s", GST_MESSAGE_TYPE(msg),
              gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));

    break;
  }
}
