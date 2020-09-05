#include <gst/gst.h>

/* Functions below pint the Capabilities in a human-friendly format */
static gboolean print_field(GQuark, const GValue *, gpointer);
static void print_caps(const GstCaps *, const gchar *);

/* Prints information about a Pad Template, including its Capabilities */
static void print_pad_templates_information(GstElementFactory *);

/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities(GstElement *, gchar *);

int main(int argc, char *argv[]) {
  GstElement *pipeline, *source, *sink;
  GstElementFactory *source_factory, *sink_factory;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  /* Initialize GStreamer */
  gst_init(&argc, &argv);

  /* Create the element factories */
  source_factory = gst_element_factory_find("audiotestsrc");
  sink_factory = gst_element_factory_find("autoaudiosink");
  if (!source_factory || !sink_factory) {
    g_error("Not all element factories could be created.\n");
    return -1;
  }

  /* Print information about the pad templates of these factories */
  print_pad_templates_information(source_factory);
  print_pad_templates_information(sink_factory);

  /* Ask the factories to instantiate actual elements */
  source = gst_element_factory_create(source_factory, "source");
  sink = gst_element_factory_create(sink_factory, "sink");

  /* Create the empty pipeline */
  pipeline = gst_pipeline_new("test-pipeline");

  if (!pipeline || !source || !sink) {
    g_error("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline */
  gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
  if (gst_element_link(source, sink) != TRUE) {
    g_error("Elements could not be linked.\n");
    gst_object_unref(pipeline);
    return -1;
  }

  /* Print initial negotiated caps (in NULL state) */
  g_message("In NULL state:\n");
  print_pad_capabilities(sink, "sink");

  /* Start playing */
  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_error("Unable to set the pipeline to the playing state (check the bus "
            "for error messages).\n");
  }

  /* Wait until error, EOS or State Change */
  bus = gst_element_get_bus(pipeline);
  do {
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                     GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED);

    /* Parse message */
    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE(msg)) {
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
        /* We are only interested in state-changed messages from the pipeline */
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
          GstState old_state, new_state, pending_state;
          gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
          g_message("Pipeline state changed from %s to %s:", gst_element_state_get_name(old_state),
                    gst_element_state_get_name(new_state));
          /* Print the current capabilities of the sink element */
          print_pad_capabilities(sink, "sink");
        }
        break;
      default:
        /* We should not reach here because we only asked for ERRORs, EOS and
         * STATE_CHANGED */
        g_error("Unexpected message received.\n");
        break;
      }
      gst_message_unref(msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  gst_object_unref(source_factory);
  gst_object_unref(sink_factory);
  return 0;
}

static gboolean print_field(GQuark field, const GValue *value, gpointer pfx) {
  gchar *str = gst_value_serialize(value);

  // g_print("%s %15s: %s\n", (gchar *)pfx, g_quark_to_string(field), str);
  g_print("%15s: %s\n", g_quark_to_string(field), str);
  g_free(str);

  return TRUE;
}

static void print_caps(const GstCaps *caps, const gchar *pfx) {
  g_return_if_fail(caps != NULL);

  if (gst_caps_is_any(caps)) {
    g_print("%sANY\n", pfx);

    return;
  }

  if (gst_caps_is_empty(caps)) {
    g_print("%sEMPTY\n", pfx);

    return;
  }

  for (guint i = 0; i < gst_caps_get_size(caps); i++) {
    GstStructure *structure = gst_caps_get_structure(caps, i);

    g_print("%s%s\n", pfx, gst_structure_get_name(structure));
    gst_structure_foreach(structure, print_field, (gpointer)pfx);
  }
}

static void print_pad_templates_information(GstElementFactory *factory) {
  const GList *pads;
  GstStaticPadTemplate *pad_template;

  g_print("Pad Templates for %s:\n", gst_element_factory_get_longname(factory));
  if (!gst_element_factory_get_num_pad_templates(factory)) {
    g_print("  none\n");

    return;
  }

  pads = gst_element_factory_get_static_pad_templates(factory);
  while (pads) {
    pad_template = pads->data;
    pads = g_list_next(pads);

    switch (pad_template->direction) {
    case GST_PAD_SRC:
      g_print("  SRC template: '%s'\n", pad_template->name_template);
      break;
    case GST_PAD_SINK:
      g_print("   SINK template: '%s'\n", pad_template->name_template);
      break;
    default:
      g_warning("   UNKNOWN!!! template: '%s'\n", pad_template->name_template);
      break;
    }

    switch (pad_template->presence) {
    case GST_PAD_ALWAYS:
      g_print("    Availability: Always\n");
      break;
    case GST_PAD_SOMETIMES:
      g_print("    Availability: Sometimes\n");
      break;
    case GST_PAD_REQUEST:
      g_print("    Availability: On request\n");
      break;
    default:
      g_warning("    Availability: UNKNOWN!!!\n");
      break;
    }

    if (pad_template->static_caps.string) {
      GstCaps *caps;
      g_print("    Capabilities:\n");
      caps = gst_static_caps_get(&pad_template->static_caps);

      print_caps(caps, "     ");
      gst_caps_unref(caps);
    }

    g_print("\n");
  }
}

static void print_pad_capabilities(GstElement *element, gchar *pad_name) {
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  /* Retrieve pad */
  pad = gst_element_get_static_pad(element, pad_name);
  if (!pad) {
    g_error("Could not retrieve pad '%s'", pad_name);

    return;
  }

  /* Retrieve negotiated caps (or acceptable caps if negotiation if not finished
   * yet) */
  caps = gst_pad_get_current_caps(pad);
  if (!caps) {
    caps = gst_pad_query_caps(pad, NULL);
  }

  /* Print and free */
  g_print("Caps for the %s pad:\n", pad_name);
  print_caps(caps, "      ");
  gst_caps_unref(caps);
  gst_object_unref(pad);
}
