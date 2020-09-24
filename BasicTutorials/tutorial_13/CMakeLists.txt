# CMakeList.txt : CMake project for CMakeProject2, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

pkg_check_modules(GIO REQUIRED gio-2.0)
if ( NOT (GIO_FOUND))
    message(FATAL_ERROR "Please Install Gstreamer Dev: CMake will Exit")
endif()
set(ENV{PKG_CONFIG_PATH})

# Add source to this project's executable.
add_executable (tutorial_13 "main.c" )

target_compile_options(tutorial_13 PUBLIC ${GIO_CFLAGS_OTHER})
target_include_directories(tutorial_13 PUBLIC "${GIO_INCLUDE_DIRS}")
target_link_libraries(tutorial_13 PUBLIC ${GIO_LIBRARIES})
target_link_directories(tutorial_13 PUBLIC ${GIO_LIBRARY_DIRS})

# TODO: Add tests and install targets if needed.
