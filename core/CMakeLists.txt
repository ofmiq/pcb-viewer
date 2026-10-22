add_library(pcbcore STATIC
    src/image.c
    src/stb_impl.c
    src/image_utils.c
    src/orb.c
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

add_executable(test_orb tests/test_orb.c)
target_link_libraries(test_orb PRIVATE pcbcore)
set_target_properties(test_orb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_compile_options(test_batch PRIVATE
    -Wall
    -Wextra
)

target_compile_options(test_pipeline PRIVATE
    -Wall
    -Wextra
)

target_compile_options(test_orb PRIVATE
    -Wall
    -Wextra
    -fsanitize=address,leak,undefined
    -g
    -fno-omit-frame-pointer
)

target_link_options(test_orb PRIVATE
    -fsanitize=address,leak,undefined
    -g
    -fno-omit-frame-pointer
)
