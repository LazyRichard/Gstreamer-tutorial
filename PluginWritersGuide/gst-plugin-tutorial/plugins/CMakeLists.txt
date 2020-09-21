add_library(helloworld SHARED gsthelloworld.c)

target_compile_options(helloworld PUBLIC ${GST_CFLAGS_OTHER})
