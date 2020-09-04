#include <gst/gst.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
  GstElement *pipeline, *source, *sink, *filter, *convert;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  gst_init(&argc, &argv);

  source = gst_element_factory_make("videotestsrc", "source");
  filter = gst_element_factory_make("vertigotv", "filter");
  convert = gst_element_factory_make("videoconvert", "videoconvert");
  sink = gst_element_factory_make("autovideosink", "sink");

  pipeline = gst_pipeline_new("test-pipeline");

  if (!pipeline || !source || !filter || !convert || !sink) {
    g_error("Not all elements could be created.");

    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many(GST_BIN(pipeline), source, filter, convert, sink, NULL);

  if (!gst_element_link_many(source, filter, convert, sink, NULL)) {
    g_error("Elements could not be linked.");
    gst_object_unref(pipeline);

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

  do {
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                     GST_MESSAGE_STATE_CHANGED |
                                         GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

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

        terminate = TRUE;
        break;
      case GST_MESSAGE_EOS:
        g_message("End-Of-Stream reached.");

        terminate = TRUE;
        break;

      case GST_MESSAGE_STATE_CHANGED:
        /* We are only interested in state-changed message from the pipeline */
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
          GstState old_state, new_state, pending_state;

          gst_message_parse_state_changed(msg, &old_state, &new_state,
                                          &pending_state);
          g_message("Pipeline state change from %s to %s:",
                    gst_element_state_get_name(old_state),
                    gst_element_state_get_name(new_state));

          /* Create graph per state change */
          GString *base_fname = g_string_new("");
          g_string_printf(base_fname, "pipeline-%s-%s",
                          gst_element_state_get_name(old_state),
                          gst_element_state_get_name(new_state));

          GstBin *bin = GST_BIN(pipeline);
          GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(
              bin, GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE, base_fname->str);

          g_string_free(base_fname, TRUE);
        }

        break;
      default:
        /* We should not reach here */
        g_error("Unexpected message received.");

        break;
      }

      gst_message_unref(msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}