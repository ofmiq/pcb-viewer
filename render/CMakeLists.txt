# Убедитесь, что PkgConfig найден (если он уже подключен в родительском CMakeLists.txt, эту строчку можно пропустить)
find_package(PkgConfig REQUIRED)

# Находим систему Cairo через pkg-config (она автоматически подтянет правильные пути include и libs)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

# Объявляем библиотеку
add_library(pcbviewer STATIC
    hello.c
)

# Подключаем локальные инклюды (если папка include реально есть в проекте)
target_include_directories(pcbviewer PUBLIC
    include
)

# ПРИВЯЗЫВАЕМ CAIRO К БИБЛИОТЕКЕ ЧЕРЕЗ PkgConfig::CAIRO
target_link_libraries(pcbviewer PRIVATE PkgConfig::CAIRO)

# Создаем исполняемый файл
add_executable(hello hello.c)
target_link_libraries(hello PRIVATE pcbviewer PkgConfig::CAIRO)
set_target_properties(hello PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Настройка варнингов (убедитесь, что таргет test_batch существует в этом файле, иначе CMake выдаст ошибку)
if(TARGET test_batch)
    target_compile_options(test_batch PRIVATE
        -Wall
        -Wextra
    )
endif()