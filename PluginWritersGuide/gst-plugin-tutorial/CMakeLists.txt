include_directories("${PROJECT_SOURCE_DIR}/plugins/helloworld")

add_subdirectory("plugins")
add_executable(plugin-test testapp.c)
target_link_libraries(plugin-test myfilter)

