find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(pcb-viewer_penis gui.c)

target_include_directories(pcb-viewer_penis PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/third_party/gtk4
)

target_link_libraries(pcb-viewer_penis PRIVATE PkgConfig::GTK4)
