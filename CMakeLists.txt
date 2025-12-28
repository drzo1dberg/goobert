cmake_minimum_required(VERSION 3.16)
project(goobert VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Network)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)

# Sources
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mpvwidget.cpp
    src/mpvcontroller.cpp
    src/gridcell.cpp
    src/controlpanel.cpp
    src/filescanner.cpp
    src/config.cpp
    src/keymap.cpp
)

set(HEADERS
    src/mainwindow.h
    src/mpvwidget.h
    src/mpvcontroller.h
    src/gridcell.h
    src/controlpanel.h
    src/filescanner.h
    src/config.h
    src/keymap.h
)

# Executable
add_executable(goobert ${SOURCES} ${HEADERS})

target_include_directories(goobert PRIVATE
    ${MPV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(goobert PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Network
    ${MPV_LIBRARIES}
)

target_compile_definitions(goobert PRIVATE
    GOOBERT_VERSION="${PROJECT_VERSION}"
)

# Install
install(TARGETS goobert RUNTIME DESTINATION bin)
