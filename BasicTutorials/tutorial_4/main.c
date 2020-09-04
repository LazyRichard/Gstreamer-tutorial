#include <gst/gst.h>

/* Structure to contain all out information, so we can pass it around */
typedef struct _CustomData
{
    GstElement *playbin;   // Out one and only element
    gboolean playing;      // Are we in the PLAYING state
    gboolean terminate;    // Sould we terminate execution?
    gboolean seek_enabled; // Is seeking enabled for this media?
    gboolean seek_done;    // Have we performed the seek already?
    gint64 duration;       // How long does this media last, in nanoseconds
} CustomData;

/* Forward definition of the message processing function */
static void handle_message(CustomData *, GstMessage *);

int main(int argc, char *argv[])
{
    CustomData data = {
        .playbin = NULL,
        .playing = FALSE,
        .terminate = FALSE,
        .seek_enabled = FALSE,
        .duration = GST_CLOCK_TIME_NONE};
    GstBus *bus = NULL;
    GstMessage *msg = NULL;
    GstStateChangeReturn ret = GST_STATE_CHANGE_FAILURE;
    const gchar *uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    data.playbin = gst_element_factory_make("playbin", "playbin");
    if (!data.playbin)
    {
        g_error("Not all elements could be created.");

        return -1;
    }

    /* Set the URI to play */
    g_object_set(data.playbin, "uri", uri, NULL);

    /* Start playing */
    ret = gst_element_set_state(data.playbin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_error("Unable to set the pipeline to the playing state.");
        gst_object_unref(data.playbin);

        return -1;
    }

    /* Listen to the bus */
    bus = gst_element_get_bus(data.playbin);

    do
    {
        msg = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND,
                                         GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION);

        /* Parse message */
        if (msg != NULL)
        {
            handle_message(&data, msg);
        }
        else
        {
            /* We got no message, this means the timeout expired */
            if (data.playing)
            {
                guint64 current = -1;

                /* Query the current position of the stream */
                if (!gst_element_query_position(data.playbin, GST_FORMAT_TIME, &current))
                {
                    g_error("Could not query current position.");
                }

                /* If we didn't know it yet, query the stream duration */
                if (!GST_CLOCK_TIME_IS_VALID(data.duration))
                {
                    if (!gst_element_query_duration(data.playbin, GST_FORMAT_TIME, &data.duration))
                    {
                        g_error("Coult not query current duration.");
                    }
                }

                /* Print current position and total duration */
                g_message("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
                          GST_TIME_ARGS(current), GST_TIME_ARGS(data.duration));

                /* If seeking is enabled, we have not done yet, and the time is right, seek */
                if (data.seek_enabled && !data.seek_done && current > 10 * GST_SECOND)
                {
                    g_message("\nReaced 10s, performing seek...");
                    gst_element_seek_simple(data.playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND);
                    data.seek_done = TRUE;
                }
            }
        }
    } while (!data.terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(data.playbin, GST_STATE_NULL);
    gst_object_unref(data.playbin);
}

static void handle_message(CustomData *data, GstMessage *msg)
{
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        g_error("Error received from element %s: %s", GST_OBJECT_NAME(msg->src), err->message);
        g_error("Debugging information: %s", debug_info ? debug_info : "none");

        g_clear_error(&err);
        g_free(debug_info);
        data->terminate = TRUE;

        break;

    case GST_MESSAGE_EOS:
        g_message("EOS reached");
        data->terminate = TRUE;

        break;

    case GST_MESSAGE_DURATION:
        /* The duration has changed, mark the current one as invalid */
        data->duration = GST_CLOCK_TIME_NONE;
        break;

    case GST_MESSAGE_STATE_CHANGED:
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data->playbin))
        {
            g_message("Pipeline state changed from %s to %s:", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));

            /* Remember whether we are in the PLAYING state of not */
            data->playing = (new_state == GST_STATE_PLAYING);

            if (data->playing)
            {
                /* We just moved to PLAYING. Check if seeking is possible */
                GstQuery *query;
                gint64 start, end;

                query = gst_query_new_seeking(GST_FORMAT_TIME);
                if (gst_element_query(data->playbin, query))
                {
                    gst_query_parse_seeking(query, NULL, &data->seek_enabled, &start, &end);
                    if (data->seek_enabled)
                    {
                        g_message("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT, GST_TIME_ARGS(start), GST_TIME_ARGS(end));
                    }
                    else
                    {
                        g_message("Seeking is DIABLED for this stream.");
                    }
                }
                else
                {
                    g_error("Seeking query failed.");
                }

                gst_query_unref(query);
            }
        }

        break;

    default:
        /* We should not reach here */
        g_error("Unexpected message received. '%s'", GST_MESSAGE_TYPE_NAME(msg));

        break;
    }

    gst_message_unref(msg);
}