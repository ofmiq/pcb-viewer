find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

file(GLOB GUI_SOURCES CONFIGURE_DEPENDS
    *.c
)

add_executable(pcb_gui
    ${GUI_SOURCES}
)

target_link_libraries(pcb_gui PRIVATE
    PkgConfig::GTK4
)

target_compile_options(pcb_gui PRIVATE
    -Wall
    -Wextra
)

set_target_properties(pcb_gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(gui_build DEPENDS
    pcb_gui
)
