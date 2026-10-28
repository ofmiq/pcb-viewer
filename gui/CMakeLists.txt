find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

add_executable(pcb-viewer gui.c)

target_include_directories(pcb-viewer PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_directories(pcb-viewer PRIVATE ${GTK4_LIBRARY_DIRS})
target_compile_options(pcb-viewer PRIVATE ${GTK4_CFLAGS_OTHER})
target_link_libraries(pcb-viewer PRIVATE ${GTK4_LIBRARIES})
