#include <gst/gst.h>
#include <stdio.h>

int main(int argc, char *argv[], char *envp[]) {
  GstElement *pipeline;
  GstElement *filesrc;
  GstElement *filter;

  /* Initialize pipeline */
  gst_init(&argc, &argv);

  pipeline = gst_pipeline_new("test_pipeline");

  for (char **env = envp; *env != 0; env++) {
    char *thisEnv = *env;
    printf("%s\n", thisEnv);
  }

  filesrc = gst_element_factory_make("filesrc", "my_filesource");
  filter = gst_element_factory_make("helloworld", "my_filter");

  if (!filesrc || !filter) {
    g_printerr("Failed create element.");

    return -1;
  }

  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}