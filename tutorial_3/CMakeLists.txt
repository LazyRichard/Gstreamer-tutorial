# CMakeList.txt : CMake project for CMakeProject2, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (tutorial_3 "main.c" )

target_compile_options(tutorial_3 PUBLIC ${GST_CFLAGS_OTHER})

# TODO: Add tests and install targets if needed.
