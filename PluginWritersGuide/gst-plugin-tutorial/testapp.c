#include <gst/gst.h>
#include <stdio.h>

typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *filter;
  GstElement *sink;
} CustomData;

int main(int argc, char *argv[], char *envp[]) {
  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;

  /* Initialize pipeline */
  gst_init(&argc, &argv);

  data.pipeline = gst_pipeline_new("test_pipeline");

  data.source = gst_element_factory_make("videotestsrc", "source");
  data.sink = gst_element_factory_make("autovideosink", "sink");
  data.filter = gst_element_factory_make("myfilter", "my_filter");

  if (!data.source || !data.sink) {
    g_printerr("Failed create element.");

    return -1;
  } else if (!data.filter) {
    g_printerr("Could not load custom filter element. please check plugin properly installed.");

    /* Print all environment variables for debug purposes */
    for (char **env = envp; *env != 0; env++) {
      char *thisEnv = *env;
      g_print("%s\n", thisEnv);
    }

    return -1;
  }

  gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.filter, data.sink, NULL);

  if (!gst_element_link_many(data.source, data.filter, data.sink, NULL)) {
    g_printerr("Failed to link one or more elements!\n");

    return -1;
  }

  ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Failed to start up pipeline!\n");

    return -1;
  }

  bus = gst_element_get_bus(data.pipeline);
  if (bus == NULL) {
    g_printerr("Failed to get bus from pipeline!\n");

    return -1;
  }

  /* Wait until error or EOS */
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Free resources */
  if (msg != NULL)
    gst_message_unref(msg);

  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);

  return 0;
}