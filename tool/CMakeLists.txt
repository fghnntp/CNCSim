# 设置最低 CMake 版本
cmake_minimum_required(VERSION 3.5)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 收集当前目录下的所有源文件
file(GLOB TOOL_SOURCES
    "*.cpp"
    "*.c"
)

file(GLOB TOOL_HEADERS
    "*.h"
    "*.hpp"
)

# 如果你想手动指定文件，可以替换上面的 file(GLOB) 为：
# set(TOOL_SOURCES
#     tool.cpp
#     utils.cpp
#     # 添加其他源文件
# )

# 创建动态库
add_library(tool SHARED
    ${TOOL_SOURCES}
    ${TOOL_HEADERS}
)

# 设置库的输出名称（可选）
set_target_properties(tool PROPERTIES
    OUTPUT_NAME "tool"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# 如果需要链接其他库，在这里添加
# target_link_libraries(tool PRIVATE 
#     # 其他依赖库
# )

# 设置头文件包含目录
target_include_directories(tool PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 安装库文件
install(TARGETS tool
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# 安装头文件
install(FILES ${TOOL_HEADERS}
    DESTINATION include/tool
)
