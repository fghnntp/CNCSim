cmake_minimum_required(VERSION 3.5)

project(CNCSimu VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt version (Qt5 or Qt6)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts)

# Find Qt modules with the proper variable
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED Charts)

find_package(OpenGL REQUIRED)

# 添加子目录
add_subdirectory(tool)
add_subdirectory(milltask)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        GCodeEdit.h
        GCodeEdit.cpp
        ToolTableDialog.h
        ToolTableDialog.cpp
        cnc_config.h
        live_plotter.cpp
        live_plotter.h
        live_plotter_motion.cpp
        live_plotter.h
        CmdTextEdit.cpp
        CmdTextEdit.h
        LogDisplayWidget.cpp
        LogDisplayWidget.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(CNCSimu
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET cnc APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(CNCSimu SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(CNCSimu
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(CNCSimu PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}/tool
   ${CMAKE_CURRENT_SOURCE_DIR}/milltask
)

# Link Charts if found
if(TARGET Qt${QT_VERSION_MAJOR}::Charts)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Charts)
    add_definitions(-DHAVE_QT_CHARTS)
else()
    message(WARNING "Qt Charts module not found - some features will be disabled")
endif()

#target_link_libraries(milltask PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(CNCSimu PUBLIC Qt${QT_VERSION_MAJOR}::Widgets OpenGL::GL
    tool
    milltask
)

set_target_properties(CNCSimu PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS CNCSimu
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(CNCSimu)
endif()
