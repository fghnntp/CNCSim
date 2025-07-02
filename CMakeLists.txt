cmake_minimum_required(VERSION 3.5)

project(cnc VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt version (Qt5 or Qt6)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)

# Find Qt modules with the proper variable
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

find_package(OpenGL REQUIRED)

# 添加 tool 子目录
add_subdirectory(tool)
add_subdirectory(milltask)
add_subdirectory(rtapp)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        GCodeEdit.h
        GCodeEdit.cpp
        ToolTableDialog.h
        ToolTableDialog.cpp
        cnc_config.h
        live_plotter.cpp
        live_plotter.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(cnc
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET cnc APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(cnc SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(cnc
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(cnc PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/tool
   ${CMAKE_CURRENT_SOURCE_DIR}/milltask
   ${CMAKE_CURRENT_SOURCE_DIR}/rtapp
)
#target_link_libraries(milltask PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(cnc PUBLIC Qt${QT_VERSION_MAJOR}::Widgets OpenGL::GL
    tool
    milltask
    rtapp
)

set_target_properties(cnc PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS cnc
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(cnc)
endif()
