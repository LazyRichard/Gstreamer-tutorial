#include <gio/gio.h>
#include <gst/gst.h>
#include <stdio.h>
#include <string.h>

typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *video_sink;
  GMainLoop *loop;

  gboolean playing; // Playing or Paused
  gdouble rate;     // Current playback rate (can be negative)
} CustomData;

/* Send seek event to change rate */
static void send_seek_event(CustomData *);

/* Process keyboard input */
static gboolean handle_keyboard(GIOChannel *, GIOCondition, CustomData *);

int main(int argc, char *argv[], char *envp[]) {
  CustomData data;
  GIOChannel *io_stdin;
  gchar *file_path;
  GFile *file;
  char *file_name;
  gchar *uri;
  GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;

  /* Parsing from env */
  file_path = g_environ_getenv(envp, "FILE_PATH");
  if (file_path == NULL)
    g_error("FILE_PATH environment variable must be setted!");

  file = g_file_new_for_path(file_path);
  if (!g_file_query_exists(file, NULL))
    g_error("Given path '%s' is not exist!", file_path);

  file_name = g_file_get_uri(file);

  uri = g_strconcat("playbin uri=", file_name, NULL);

  g_object_unref(file);

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

  /* Build the pipeline */
  data.pipeline = gst_parse_launch(uri, NULL);

  /* Add a keyboard watch so we get notified of keystrokes */
#ifdef G_OS_WIN32
  io_stdin = g_io_channel_win32_new_fd(_fileno(stdin));
#else
  io_stdin = g_io_channel_unix_new(_fileno(stdin));
#endif
  g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, &data);

  /* Start playing */
  ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Unable to set the pipeline to the playing state.\n");
    gst_object_unref(data.pipeline);
    return -1;
  }
  data.playing = TRUE;
  data.rate = 1.0;

  /* Create a GLib Main Loop and set it to run */
  data.loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(data.loop);

  /* Free resources */
  g_main_loop_unref(data.loop);
  g_io_channel_unref(io_stdin);
  gst_element_set_state(data.pipeline, GST_STATE_NULL);

  if (data.video_sink != NULL)
    gst_object_unref(data.video_sink);
  gst_object_unref(data.pipeline);

  g_free(uri);

  return 0;
}

static void send_seek_event(CustomData *data) {
  gint64 position;
  GstEvent *seek_event;

  /* Obtain the current position, needed for the seek event */
  if (!gst_element_query_position(data->pipeline, GST_FORMAT_TIME, &position)) {
    g_printerr("Unable to retrieve current position.\n");
    return;
  }

  /* Create the seek event */
  if (data->rate > 0) {
    seek_event = gst_event_new_seek(data->rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                                    GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_END, 0);
  } else {
    seek_event = gst_event_new_seek(data->rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                                    GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, position);
  }

  if (data->video_sink == NULL) {
    /* If we have not done so, obtain the sink through which we will send the seek events */
    g_object_get(data->pipeline, "video-sink", &data->video_sink, NULL);
  }

  /* Send the event */
  gst_element_send_event(data->video_sink, seek_event);

  g_print("Current rate: %g\n", data->rate);
}

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, CustomData *data) {
  gchar *str = NULL;

  if (g_io_channel_read_line(source, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL) {
    return TRUE;
  }

  switch (g_ascii_tolower(str[0])) {
  case 'p':
    data->playing = !data->playing;
    gst_element_set_state(data->pipeline, data->playing ? GST_STATE_PLAYING : GST_STATE_PAUSED);
    g_print("Setting state to %s\n", data->playing ? "PLAYING" : "PAUSE");
    break;
  case 's':
    if (g_ascii_isupper(str[0])) {
      data->rate *= 2.0;
    } else {
      data->rate /= 2.0;
    }
    send_seek_event(data);
    break;
  case 'd':
    data->rate *= -1.0;
    send_seek_event(data);
    break;
  case 'n':
    if (data->video_sink == NULL) {
      /* If we have not done so, obtain the sink through which we will send the step events */
      g_object_get(data->pipeline, "video-sink", &data->video_sink, NULL);
    }

    gst_element_send_event(data->video_sink, gst_event_new_step(GST_FORMAT_BUFFERS, 1, ABS(data->rate), TRUE, FALSE));
    g_print("Stepping one frame\n");
    break;
  case 'q':
    g_main_loop_quit(data->loop);
    break;
  default:
    g_warning("Unsupported %c", g_ascii_tolower(str[0]));
    break;
  }

  g_free(str);

  return 1;
}