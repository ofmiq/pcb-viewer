add_library(pcbcore STATIC
    src/image.c
    src/stb_impl.c
)

target_include_directories(pcbcore PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/third_party/stb
)

target_link_libraries(pcbcore PUBLIC m)

add_executable(test_core tests/test_core.c)
target_link_libraries(test_core PRIVATE pcbcore)

add_test(NAME core_image_io COMMAND test_core)
set_tests_properties(core_image_io PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
