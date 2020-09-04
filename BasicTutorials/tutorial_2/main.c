#include <gst/gst.h>

int main(int argc, char *argv[]) {
  GstElement *pipeline;
  GstElement *source;
  GstElement *sink;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Create the elements */
  source = gst_element_factory_make("videotestsrc", "source");
  sink = gst_element_factory_make("autovideosink", "sink");

  /* Create the empty pipeline */
  pipeline = gst_pipeline_new("test-pipeline");

  if (!pipeline || !source || !sink) {
    g_error("Not all elements could be created.");

    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
  if (gst_element_link(source, sink) != TRUE) {
    g_error("Elements could not be linked.");
    gst_object_unref("pipeline");

    return -1;
  }

  /* Modify the source's properties */
  g_object_set(source, "pattern", 0, NULL);

  /* Start playing */
  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_error("Unable to set the pipeline to the playing state.");
    gst_object_unref(pipeline);

    return -1;
  }

  /* Wait until error or EOS */
  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Parsing message */
  if (msg != NULL) {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error(msg, &err, &debug_info);

      g_error("Error received from element %s: %s", GST_OBJECT_NAME(msg->src),
              err->message);
      g_error("Debugging information: %s", debug_info ? debug_info : "none");

      g_clear_error(&err);
      g_free(debug_info);

      break;

    case GST_MESSAGE_EOS:
      g_message("End-Of-Stream rreached.");

      break;

    default:
      /* We should not reach here because we only asked for ERRORs and EOS */
      g_error("Unexpected message received message type id: %d",
              GST_MESSAGE_TYPE(msg));

      break;
    }

    gst_message_unref(msg);
  }

  /* Free resources */
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}