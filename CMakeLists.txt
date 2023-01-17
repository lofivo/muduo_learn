# 指定使用 CMake 的最低版本号
cmake_minimum_required(VERSION 3.0)

# 设置项目名称
project(mymuduo C CXX)

# 添加搜索路径
include_directories(${PROJECT_SOURCE_DIR})

# 包含这些目录下的.cc文件    
aux_source_directory(${PROJECT_SOURCE_DIR}/src/base SRC_BASE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/logger SRC_LOG)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net SRC_NET)

# 设置编译选项
set(CXX_FLAGS
    -g
    -Wall
    -std=c++11
    )

# 设置输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# 生成动态库 mymuduo
add_library(mymuduo SHARED 
${SRC_BASE}
${SRC_LOG}
${SRC_NET}
)

# 目标动态库所需连接的库（这里需要连接libpthread.so）
target_link_libraries(mymuduo pthread)

# 设置生成动态库的路径，放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(src/logger/test)