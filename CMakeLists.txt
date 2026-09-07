cmake_minimum_required(VERSION 3.16)
project(WinFilesPro VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(WinFilesPro
    src/main.cpp
    src/mainwindow/mainwindow.cpp
    src/mainwindow/mainwindow.h
)

target_link_libraries(WinFilesPro PRIVATE Qt6::Widgets)