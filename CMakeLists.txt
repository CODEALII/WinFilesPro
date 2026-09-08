cmake_minimum_required(VERSION 3.16)
project(WinFilesPro VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(WinFilesPro
    src/main.cpp
    src/mainwindow/mainwindow.cpp
    src/mainwindow/mainwindow.h
    src/mainwindow/iconprovider.cpp
    src/mainwindow/iconprovider.h
    src/mainwindow/style.cpp
    src/mainwindow/style.h
    src/mainwindow/i18n.cpp
    src/mainwindow/i18n.h
    src/mainwindow/settingsdialog.cpp
    src/mainwindow/settingsdialog.h
    src/mainwindow/settingsmanager.cpp
    src/mainwindow/settingsmanager.h
    assets/icons/resources.qrc
)

target_link_libraries(WinFilesPro PRIVATE Qt6::Widgets)

target_compile_definitions(WinFilesPro PRIVATE QT_NO_PLATFORM_THEME_INTEGRATION)