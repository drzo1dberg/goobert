cmake_minimum_required(VERSION 3.16)
project(goobert VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# macOS: Add Homebrew paths for Qt6 and dependencies
if(APPLE)
    # Apple Silicon (M1/M2/M3)
    if(EXISTS "/opt/homebrew")
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@6")
        set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    # Intel Mac
    elseif(EXISTS "/usr/local/opt/qt@6")
        list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt@6")
        set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
endif()

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Network Sql)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)

# Sources
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mpvwidget.cpp
    src/gridcell.cpp
    src/toolbar.cpp
    src/sidepanel.cpp
    src/monitorwidget.cpp
    src/playlistwidget.cpp
    src/playlistpicker.cpp
    src/filescanner.cpp
    src/config.cpp
    src/keymap.cpp
    src/statsmanager.cpp
    src/settingsdialog.cpp
)

set(HEADERS
    src/mainwindow.h
    src/mpvwidget.h
    src/gridcell.h
    src/toolbar.h
    src/sidepanel.h
    src/monitorwidget.h
    src/playlistwidget.h
    src/playlistpicker.h
    src/filescanner.h
    src/config.h
    src/keymap.h
    src/theme.h
    src/statsmanager.h
    src/settingsdialog.h
)

# Executable
if(APPLE)
    # Create macOS app bundle
    set(MACOSX_BUNDLE_BUNDLE_NAME "Goobert")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.goobert.app")
    add_executable(goobert MACOSX_BUNDLE ${SOURCES} ${HEADERS})
else()
    add_executable(goobert ${SOURCES} ${HEADERS})
endif()

target_include_directories(goobert PRIVATE
    ${MPV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(goobert PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Network
    Qt6::Sql
    ${MPV_LIBRARIES}
)

# macOS: Link OpenGL framework
if(APPLE)
    target_link_libraries(goobert PRIVATE "-framework OpenGL")
endif()

target_compile_definitions(goobert PRIVATE
    GOOBERT_VERSION="${PROJECT_VERSION}"
)

# Install
if(APPLE)
    install(TARGETS goobert BUNDLE DESTINATION .)
else()
    install(TARGETS goobert RUNTIME DESTINATION bin)
endif()
