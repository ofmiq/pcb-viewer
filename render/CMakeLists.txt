find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

file(GLOB RENDER_SOURCES CONFIGURE_DEPENDS
    *.c
)

add_executable(pcb_render
    ${RENDER_SOURCES}
)

target_include_directories(pcb_render PRIVATE
    include
)

target_link_libraries(pcb_render PRIVATE
    PkgConfig::CAIRO
)

target_compile_options(pcb_render PRIVATE
    -Wall
    -Wextra
)

set_target_properties(pcb_render PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(render_build DEPENDS
    pcb_render
)
