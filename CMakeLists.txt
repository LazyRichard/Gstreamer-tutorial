# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("GStreamerTutorials")

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
find_package(PkgConfig REQUIRED)
if ( NOT (PKGCONFIG_FOUND))
    message(FATAL_ERROR "Please Install PkgConfig: CMake will Exit")
endif()

pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.8)
if ( NOT (GST_FOUND))
    message(FATAL_ERROR "Please Install Gstreamer Dev: CMake will Exit")
endif()
set(ENV{PKG_CONFIG_PATH})

include_directories("${GST_INCLUDE_DIRS}")
link_libraries(${GST_LIBRARIES})
link_directories(${GST_LIBRARY_DIRS})
include_directories(${GST_INCLUDE_DIRS})


# Include sub-projects.
add_subdirectory ("tutorial_1")
add_subdirectory ("tutorial_2")
add_subdirectory ("tutorial_3")
add_subdirectory ("tutorial_4")
