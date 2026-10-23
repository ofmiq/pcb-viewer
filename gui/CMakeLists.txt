# Шукаємо системний GTK4
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

# Створюємо програму (переконайтеся, що main.c та pixbufpaintable.c лежать у папці gui)
add_executable(pcb-viewer gui.c pixbufpaintable.c)

# Передаємо налаштування GTK4 до нашої програми
target_include_directories(pcb-viewer PRIVATE ${GTK4_INCLUDE_DIRS})
target_link_directories(pcb-viewer PRIVATE ${GTK4_LIBRARY_DIRS})
target_compile_options(pcb-viewer PRIVATE ${GTK4_CFLAGS_OTHER})
target_link_libraries(pcb-viewer PRIVATE ${GTK4_LIBRARIES})
