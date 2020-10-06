#include <gst/check/gstcheck.h>
#include <gst/gst.h>

static GstElement * setup_myfilter(void) {
    GstElement *myfilter;

    GST_DEBUG("setup myfilter");

    myfilter = gst_check_setup_element("myfilter");

    return myfilter;
}

static void cleanup_myfilter(GstElement* myfilter){
    GST_DEBUG("cleanup myfilter");

    gst_check_teardown_element(myfilter);
}


GST_START_TEST (test_myfilter)
{
    GstElement *myfilter;

    /* Setup */
    myfilter = setup_myfilter();

    /* Test */
    fail_unless(myfilter != NULL, "Could not create element");

    /* Teardown */
    cleanup_myfilter(myfilter);
}
GST_END_TEST;

static Suite* myfilter_suite(void) {
    Suite *s = suite_create("myfilter");
    TCase *tc_chain = tcase_create("general");

    suite_add_tcase(s, tc_chain);
    tcase_add_test(tc_chain, test_myfilter);

    return s;
}

GST_CHECK_MAIN(myfilter);