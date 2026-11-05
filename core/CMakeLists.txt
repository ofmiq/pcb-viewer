file(GLOB CORE_SOURCES CONFIGURE_DEPENDS
    src/*.c
)

add_library(pcb_core_lib STATIC
    ${CORE_SOURCES}
)

target_include_directories(pcb_core_lib PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/third_party/stb
)

target_link_libraries(pcb_core_lib PUBLIC
    m
)

target_compile_options(pcb_core_lib PRIVATE
    -Wall
    -Wextra
)

add_executable(test_batch
    tests/test_batch.c
)

add_executable(test_pipeline
    tests/test_pipeline.c
)

add_executable(test_orb
    tests/test_orb.c
)

target_link_libraries(test_batch PRIVATE
    pcb_core_lib
)

target_link_libraries(test_pipeline PRIVATE
    pcb_core_lib
)

target_link_libraries(test_orb PRIVATE
    pcb_core_lib
)

foreach(target test_batch test_pipeline test_orb)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
    )

    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endforeach()

add_custom_target(core_build DEPENDS
    test_batch
    test_pipeline
    test_orb
)
