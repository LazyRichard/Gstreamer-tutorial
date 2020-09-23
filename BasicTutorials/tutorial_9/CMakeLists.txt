# CMakeList.txt : CMake project for CMakeProject2, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

pkg_check_modules(GST_AUDIO REQUIRED gstreamer-pbutils-1.0)
if ( NOT (GST_AUDIO_FOUND))
    message(FATAL_ERROR "Please Install Gstreamer Dev: CMake will Exit")
endif()
set(ENV{PKG_CONFIG_PATH})

# Add source to this project's executable.
add_executable (tutorial_9 "main.c" )

target_compile_options(tutorial_9 PUBLIC ${GST_AUDIO_CFLAGS_OTHER})
target_include_directories(tutorial_9 PUBLIC "${GST_AUDIO_INCLUDE_DIRS}")
target_link_libraries(tutorial_9 ${GST_AUDIO_LIBRARIES})
target_link_directories(tutorial_9 PUBLIC ${GST_AUDIO_LIBRARY_DIRS})

# TODO: Add tests and install targets if needed.
