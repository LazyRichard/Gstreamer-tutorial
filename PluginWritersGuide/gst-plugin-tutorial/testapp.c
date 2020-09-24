#include <gst/gst.h>
#include <stdio.h>

typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *source;
  GstElement *filter;
  GstElement *sink;

  gboolean playing;
  gboolean terminate;

  GMainLoop *main_loop;
} CustomData;

static gboolean handle_keyboard(GIOChannel *, GIOCondition, CustomData *);

static void error_cb(GstBus *bus, GstMessage *msg, CustomData *data);
static void eos_cb(GstBus *bus, GstMessage *msg, CustomData *data);
static void state_changed_cb(GstBus *bus, GstMessage *msg, CustomData *data);

int main(int argc, char *argv[], char *envp[]) {
  CustomData data = {
      .pipeline = NULL, .source = NULL, .filter = NULL, .sink = NULL, .terminate = FALSE, .main_loop = NULL};
  GIOChannel *io_stdin;
  GstBus *bus;
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Initialize our data structure */
  memset(&data, 0, sizeof(data));

  /* Print usage map */
  g_print("USAGE: Choose one of the following options, then press enter:\n"
          " 'P' to toggle between PAUSE and PLAY\n"
          " 'S' to increase playback speed, 's' to decrease playback speed\n"
          " 'D' to toggle playback direction\n"
          " 'N' to move to next frame (in the current direction, better in PAUSE)\n"
          " 'Q' to quit\n");

  /* Initialize gstreamer */
  gst_init(&argc, &argv);

  /* Add a keyboard watch so we get notified of keystrokes */
#ifdef G_OS_WIN32
  io_stdin = g_io_channel_win32_new_fd(_fileno(stdin));
#else
  io_stdin = g_io_channel_unix_new(_fileno(stdin));
#endif
  g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);

  /* Initialize pipeline */
  data.pipeline = gst_pipeline_new("pipeline");

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

  /* Watching bus and connect signals */
  bus = gst_element_get_bus(data.pipeline);
  gst_bus_add_signal_watch(bus);
  g_signal_connect(G_OBJECT(bus), "message::error", (GCallback)error_cb, &data);
  g_signal_connect(G_OBJECT(bus), "message::eos", (GCallback)eos_cb, &data);
  g_signal_connect(G_OBJECT(bus), "message::state-changed", (GCallback)state_changed_cb, &data);

  /* Start playing the pipeline */
  ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Failed to start up pipeline!\n");

    return -1;
  }

  data.main_loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(data.main_loop);

  /* Free resources */
  gst_element_set_state(data.pipeline, GST_STATE_NULL);
  gst_object_unref(data.pipeline);
  g_io_channel_unref(io_stdin);

  return 0;
}

/* This function is called hen an error message is posted on the bus */
static void error_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
  GError *err;
  gchar *debug_info;

  /* Print error details on the console */
  gst_message_parse_error(msg, &err, &debug_info);

  g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
  g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
  g_clear_error(&err);
  g_free(debug_info);

  /* Set the pipeline to READY (which stops playback) */
  gst_element_set_state(data->pipeline, GST_STATE_READY);
  data->terminate = TRUE;
}

/* This function is called when an End-Of-Stream message is posted on the bus.
 * We just set the pipeline to READY (which stops playback) */
static void eos_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
  g_message("End-Of-Stream reached.\n");
  gst_element_set_state(data->pipeline, GST_STATE_READY);
  data->terminate = TRUE;
}

/* This function is called when the pipeline changes states. We use it to
 * keep track of the current state. */
static void state_changed_cb(GstBus *bus, GstMessage *msg, CustomData *data) {
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
  if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->pipeline)) {
    g_message("State set to %s\n", gst_element_state_get_name(new_state));
    if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED) {
      /* For extra responsiveness, we refresh the GUI as soon as we reach the
       * PAUSED state */
    }
  }
}

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, CustomData *data) { return TRUE; }
