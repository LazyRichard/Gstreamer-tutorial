#include <gst/gst.h>

int main(int argc, char *argv[])
{
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;
    gboolean terminate = FALSE;

    const gchar *pipeline_desc = "playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";

    gst_init(&argc, &argv);

    pipeline = gst_parse_launch(pipeline_desc, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    bus = gst_element_get_bus(pipeline);

    do
    {
        msg = gst_bus_timed_pop_filtered(
            bus,
            GST_CLOCK_TIME_NONE,
            GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

        if (msg != NULL)
        {
            GError *err;
            gchar *debug_info;

            switch (GST_MESSAGE_TYPE(msg))
            {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_error("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_error("Debugging information: %s\n", debug_info ? debug_info : "none");

                g_clear_error(&err);
                g_free(debug_info);

                terminate = TRUE;
                break;
            case GST_MESSAGE_EOS:
                g_message("End-Of-Stream reached.\n");

                terminate = TRUE;
                break;

            case GST_MESSAGE_STATE_CHANGED:
                /* We are only interested in state-changed message from the pipeline */
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline))
                {
                    GstState old_state, new_state, pending_state;

                    gst_message_parse_state_changed(
                        msg, &old_state, &new_state, &pending_state);
                    g_message("Pipeline state change from %s to %s:\n",
                              gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));

                    /* Create graph per state change */
                    GString *base_fname = g_string_new("");
                    g_string_printf(base_fname, "pipeline-%s-%s", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));

                    GstBin *bin = GST_BIN(pipeline);
                    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(bin, GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE, base_fname->str);

                    g_string_free(base_fname, TRUE);
                }

                break;
            default:
                /* We should not reach here */
                g_error("Unexpected message received.\n");

                break;
            }

            gst_message_unref(msg);
        }
    } while (!terminate);

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}