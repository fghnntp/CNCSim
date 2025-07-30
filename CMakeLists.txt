cmake_minimum_required(VERSION 3.5)

include(version_info.cmake)

project(CNCSimu
        VERSION ${CNCSIMU_VERSION}
        DESCRIPTION "CNC Simulation Software base linuxcnc${CNCSIMU_VERSION}${BUILD_VERSION_SUFFIX}"
        LANGUAGES CXX C)

# Cpackge configuration
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "jdzb")
set(CPACK_PACKAGE_CONTACT "fghnntp@hotmail.com")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/ReadMe.md")

set(CPACK_PACKAGE_VERSION_MAJOR ${CNCSIMU_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${CNCSIMU_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${CNCSIMU_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${CNCSIMU_VERSION}")
# Store defualt version
set(CPACK_PACKAGE_VERSION "${CNCSIMU_VERSION}")
if(DEFINED GIT_COMMIT_HASH)
    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}.${GIT_COMMIT_HASH}")
endif()

set(CPACK_PACKAGE_FILE_NAME 
    "${PROJECT_NAME}-${CNCSIMU_VERSION}_${BUILD_TIMESTAMP}")

if(DEFINED GIT_COMMIT_HASH)
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.${GIT_COMMIT_HASH}")
endif()

set(CPACK_COMPONENT_MAIN_DEV_DISPLAY_NAME "CNCSIMU Development Files")
set(CPACK_COMPONENT_MAIN_DEV_DESCRIPTION "Headers and development files for CNCSIMU")

include(CPack)

# 设置生成头文件输出目录
set(VERSION_HEADER_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${VERSION_HEADER_DIR})

# 配置版本头文件
configure_file(
    ${CMAKE_SOURCE_DIR}/config/Version.h.in
    ${VERSION_HEADER_DIR}/Version.h
    @ONLY
)


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
        wins/main.cpp
        wins/mainwindow.cpp
        wins/mainwindow.h
        wins/GCodeEdit.h
        wins/GCodeEdit.cpp
        wins/ToolTableDialog.h
        wins/ToolTableDialog.cpp
        wins/cnc_config.h
        wins/live_plotter.cpp
        wins/live_plotter.h
        wins/live_plotter_motion.cpp
        wins/live_plotter.h
        wins/CmdTextEdit.cpp
        wins/CmdTextEdit.h
        wins/LogDisplayWidget.cpp
        wins/LogDisplayWidget.h
        wins/KeyInfoDisplayWidget.cpp
        wins/KeyInfoDisplayWidget.h
#        paras_config/BaseParamsWidget.h
#        paras_config/ParamsWidgetFactory.h
#        paras_config/TrajParamsWidget.h
#        paras_config/KinematicParamsWidget.h
#        paras_config/AxisParamsWidget.h
#        paras_config/CNCParamsWidgetFactory.h
#        paras_config/ParameterConfigDialog.h
#        paras_config/BaseParamsWidget.cpp
#        paras_config/TrajParamsWidget.cpp
#        paras_config/CNCParamsWidgetFactory.cpp
#        paras_config/ParameterConfigDialog.cpp
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
    ${VERSION_HEADER_DIR}
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

set(README_FILE "${CMAKE_SOURCE_DIR}/ReadMe.md")

# 验证文件存在
if(NOT EXISTS "${README_FILE}")
    message(FATAL_ERROR "ReadMe.txt not found at ${README_FILE}")
endif()

install(TARGETS CNCSimu
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${README_FILE} DESTINATION .
)

install(FILES ${VERSION_HEADER_DIR}/Version.h DESTINATION .)

# Automatically find and install all runtime dependencies (shared libs)
install(IMPORTED_RUNTIME_ARTIFACTS
    ${PROJECT_NAME}
    DESTINATION lib
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(CNCSimu)
endif()
