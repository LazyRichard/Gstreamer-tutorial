add_library(myfilter SHARED gstmyfilter.c)

target_compile_options(myfilter PUBLIC ${GST_CFLAGS_OTHER})
