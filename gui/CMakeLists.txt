find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(pcb-viewer gui.c)

target_link_libraries(pcb-viewer PRIVATE PkgConfig::GTK4)
