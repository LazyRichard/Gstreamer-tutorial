pkg_check_modules(CHECK REQUIRED gstreamer-check-1.0)
if ( NOT (CHECK_FOUND))
    message(FATAL_ERROR "Please Install libcheck: CMake will Exit")
endif()
set(ENV{PKG_CONFIG_PATH})

add_executable(test-gstmyfilter test_gstmyfilter.c)

target_compile_options(test-gstmyfilter PUBLIC ${CHECK_CFLAGS_OTHER})
target_include_directories(test-gstmyfilter PUBLIC ${CHECK_INCLUDE_DIRS})
target_link_libraries(test-gstmyfilter PUBLIC ${CHECK_LIBRARIES})
target_link_directories(test-gstmyfilter PUBLIC ${CHECK_LIBRARY_DIRS})
