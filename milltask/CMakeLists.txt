# 设置最低 CMake 版本
cmake_minimum_required(VERSION 3.5)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(LINUX_CNC /home/user/srcCode/linuxcnc-master)
set(LINUX_CNC_INC ${LINUX_CNC}/include)
set(LINUX_CNC_LIB ${LINUX_CNC}/lib)
set(LINUX_CNC_SRC ${LINUX_CNC}/src)

# 收集当前目录下的所有源文件
file(GLOB MILLTASK_SOURCES
    "*.cpp"
    "*.c"
    "*.cc"
)

file(GLOB MILLTASK_HEADERS
    "*.h"
)

# 创建动态库
# All real milltask should be compiler
# And Attention milltask usrmotintf and taskintf
# should add simulation mode, this allaw this project
# can use linuxcnc srccode as more as possiabley
add_library(milltask SHARED
    ${MILLTASK_SOURCES}
    ${MILLTASK_HEADERS}
    ${LINUX_CNC_SRC}/emc/motion/emcmotglb.c
    ${LINUX_CNC_SRC}/emc/task/emctask.cc
    ${LINUX_CNC_SRC}/emc/task/emccanon.cc
    ${LINUX_CNC_SRC}/emc/task/emctaskmain.cc
    ${LINUX_CNC_SRC}/emc/motion/usrmotintf.cc
    ${LINUX_CNC_SRC}/emc/motion/emcmotutil.c
    ${LINUX_CNC_SRC}/emc/task/taskintf.cc
    ${LINUX_CNC_SRC}/emc/motion/dbuf.c
    ${LINUX_CNC_SRC}/emc/motion/stashf.c
    ${LINUX_CNC_SRC}/emc/task/taskclass.cc
    ${LINUX_CNC_SRC}/emc/task/backtrace.cc
)

# 设置库的输出名称（可选）
set_target_properties(milltask PROPERTIES
    OUTPUT_NAME "milltask"
    VERSION ${MILL_VERSION}
    SOVERSION ${MILL_VERSION_MAJOR}.${MILL_VERSION_MINOR}
)

#link_directories(${LINUX_CNC_LIB})
#We should link all libs real milltask willbe link
target_link_libraries(milltask PRIVATE
    ${LINUX_CNC_LIB}/librs274.so
    ${LINUX_CNC_LIB}/liblinuxcnc.a
    ${LINUX_CNC_LIB}/libnml.so
    ${LINUX_CNC_LIB}/liblinuxcncini.so
    ${LINUX_CNC_LIB}/libposemath.so
    ${LINUX_CNC_LIB}/libpyplugin.so
    ${LINUX_CNC_LIB}/liblinuxcnchal.so
    ${LINUX_CNC_LIB}/libtooldata.so
)


# 设置头文件包含目录
target_include_directories(milltask PUBLIC
    ${VERSION_HEADER_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LINUX_CNC_INC}
    ${LINUX_CNC_INC}/linuxcnc
    ${LINUX_CNC_SRC}
    ${LINUX_CNC_SRC}/hal
    ${LINUX_CNC_SRC}/emc/pythonplugin
    /usr/include/python3.11
    ${LINUX_CNC_SRC}/emc/rs274ngc
    ${LINUX_CNC_SRC}/emc/motion
    ${LINUX_CNC_SRC}/emc/tp
#    ${LINUX_CNC_SRC}/emc/nml_intf
#    ${LINUX_CNC_SRC}/emc/motion
#    ${LINUX_CNC_SRC}/libnml/nml
#    ${LINUX_CNC_SRC}/emc/rs274ngc
#    ${LINUX_CNC_SRC}/emc
#    ${LINUX_CNC_SRC}/libnml/rcs
#    ${LINUX_CNC_SRC}/libnml/cms
#    ${LINUX_CNC_SRC}/libnml/postmath
)

target_compile_definitions(milltask PRIVATE
    ULAPI
)

# 安装库文件
install(TARGETS milltask
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

file(GLOB LINUXCNC_LIBS "${LINUX_CNC_LIB}/*.so*")  # Matches .so, .so.1, .so.1.0, etc.
install(FILES ${LINUXCNC_LIBS} DESTINATION lib)

# install(FILES
#     ${LINUX_CNC_LIB}/librs274.so
#     ${LINUX_CNC_LIB}/liblinuxcnc.a
#     ${LINUX_CNC_LIB}/libnml.so
#     ${LINUX_CNC_LIB}/liblinuxcncini.so
#     ${LINUX_CNC_LIB}/libposemath.so
#     ${LINUX_CNC_LIB}/libpyplugin.so
#     ${LINUX_CNC_LIB}/liblinuxcnchal.so
#     ${LINUX_CNC_LIB}/libtooldata.so
#     DESTINATION lib
# )

set(MILL_FILE_IF "${CMAKE_CURRENT_SOURCE_DIR}/mill_task_interface.h")
# 安装头文件
install(FILES ${MILL_FILE_IF}
    DESTINATION include/milltask
)

# Automatically find and install all runtime dependencies (shared libs)
install(IMPORTED_RUNTIME_ARTIFACTS
    milltask
    DESTINATION lib
)
