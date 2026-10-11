cmake_minimum_required(VERSION 3.19)

project(ScreenMacro VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(ScreenMacro WIN32
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/pointpicker.h
    src/pointpicker.cpp
    src/macroplayer.h
    src/macroplayer.cpp
)

target_link_libraries(ScreenMacro PRIVATE Qt6::Widgets user32)

set_target_properties(ScreenMacro PROPERTIES
    WIN32_EXECUTABLE TRUE
)