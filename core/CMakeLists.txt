add_library(pcbcore STATIC
    src/image.c
    src/stb_impl.c
)

target_include_directories(pcbcore PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/third_party/stb
)

target_link_libraries(pcbcore PUBLIC m)

add_executable(test_batch tests/test_batch.c)
target_link_libraries(test_batch PRIVATE pcbcore)
set_target_properties(test_batch PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(test_pipeline tests/test_pipeline.c)
target_link_libraries(test_pipeline PRIVATE pcbcore)
set_target_properties(test_pipeline PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_compile_options(test_batch PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_compile_options(test_pipeline PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
