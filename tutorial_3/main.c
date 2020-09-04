#include <gst/gst.h>

/* Structure to contain all out information, so we can pass it to callbacks */
typedef struct _CustomDaat
{
    GstElement *pipeline;
    GstElement *source;
    GstElement *convert;
    GstElement *resample;
    GstElement *sink;
} CustomData;

/* Handler for the pad-added signal */
static void pad_added_handler(GstElement *, GstPad *, CustomData *);

int main(int argc, char *argv[])
{
    CustomData data;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;
    const gchar *uri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm";

    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    data.source = gst_element_factory_make("uridecodebin", "source");
    data.convert = gst_element_factory_make("audioconvert", "convert");
    data.resample = gst_element_factory_make("audioresample", "resample");
    data.sink = gst_element_factory_make("autoaudiosink", "sink");

    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new("test-pipeline");

    if (!data.pipeline || !data.source || !data.convert || !data.resample || !data.sink)
    {
        g_error("Not all elements could be created.");

        return -1;
    }

    /* Build the pipeline. Note that we are NOT linking the source at this
     * point. We will do it later. */
    gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.convert, data.resample, data.sink, NULL);
    if (!gst_element_link_many(data.convert, data.resample, data.sink, NULL))
    {
        g_error("elements could not be linked.");
        gst_object_unref(data.pipeline);

        return -1;
    }

    /* Set the URI to play */
    g_object_set(data.source, "uri", uri, NULL);

    /* Connect to the pad-added signal */
    g_signal_connect(data.source, "pad-added", G_CALLBACK(pad_added_handler), &data);

    /* Start playing */
    ret = gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
    {
        g_error("Unable to set the pipeline to the playing state.");

        gst_object_unref(data.pipeline);
        return -1;
    }

    /* Listen to the bus */
    bus = gst_element_get_bus(data.pipeline);
    do
    {
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                         GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

        /* Parse message */
        if (msg != NULL)
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

                terminate = TRUE;
                break;

            case GST_MESSAGE_EOS:
                g_message("EOS reached.");
                terminate = TRUE;

                break;
            case GST_MESSAGE_STATE_CHANGED:
                /* We are only interested in state-changd messages from the pipeline */
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline))
                {
                    GstState old_state, new_state, pending_state;

                    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                    g_message("Pipeline state changed from %s to %s:",
                              gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
                }

                break;

            default:
                /* We should not reach here */
                g_error("Unexpected message received.");

                break;
            }
        }
    } while (!terminate);

    /* Free resources */
    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}

/* This function will be called by the pad-added signal */
static void pad_added_handler(GstElement *src, GstPad *new_pad, CustomData *data)
{
    GstPad *sink_pad = gst_element_get_static_pad(data->convert, "sink");
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;

    g_message("Received new pad '%s' from '%s': ", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

    /* If out converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked(sink_pad))
    {
        g_message("We are already linked. Ignoring.");

        goto exit;
    }

    /* Check the new pad's type */
    new_pad_caps = gst_pad_get_current_caps(new_pad);
    new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    new_pad_type = gst_structure_get_name(new_pad_struct);
    if (!g_str_has_prefix(new_pad_type, "audio/x-raw"))
    {
        g_message("It has type '%s' which is not raw audio. Ignoring.", new_pad_type);

        goto exit;
    }

    /* Attempt the link */
    ret = gst_pad_link(new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED(ret))
    {
        g_message("Type is '%' but link failed.", new_pad_type);
    }
    else
    {
        g_message("Link succeeded (type '%s').", new_pad_type);
    }

exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
    {
        gst_caps_unref(new_pad_caps);
    }

    /* Unreference the sink pad */
    gst_object_unref(sink_pad);
}
