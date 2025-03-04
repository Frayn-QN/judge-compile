cmake_minimum_required(VERSION 3.10)
project(JudgeCompile)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# 第三方库配置
find_package(Boost 1.74.0 COMPONENTS system filesystem chrono REQUIRED)
# 查找 SimpleAmqpClient 库
find_library(SimpleAmqpClient_LIBRARIES NAMES SimpleAmqpClient PATHS /usr/local/lib NO_DEFAULT_PATH)
find_path(SimpleAmqpClient_INCLUDE_DIRS NAMES SimpleAmqpClient/SimpleAmqpClient.h PATHS /usr/local/include NO_DEFAULT_PATH)
# 如果找不到库或头文件，则停止处理
if(NOT SimpleAmqpClient_LIBRARIES OR NOT SimpleAmqpClient_INCLUDE_DIRS)
    message(FATAL_ERROR "SimpleAmqpClient library or headers not found")
endif()

# 可执行文件配置
add_executable(${PROJECT_NAME} 
    src/main.cpp
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBS_DIR}
    ${Boost_INCLUDE_DIRS}
    ${SimpleAmqpClient_INCLUDE_DIRS}
    ${workspace_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${SimpleAmqpClient_LIBRARIES}
    ${workspace_LIBRARIES}
)