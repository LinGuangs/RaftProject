# 指定最低 Cmake 版本
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和语言
project(RaftProject CXX)

# 指定C++标准，这里使用 C++17
set(CMAKE_CXX_STANDARD 17)

# 收集 src 目录下的所有 .cpp 文件
file(GLOB_RECURSE RAFT_SOURCES "src/*.cpp")

# 创建一个可执行文件目标，名称为 raft_node，使用上面收集的所有源文件
add_executable(raft_node ${RAFT_SOURCES})

# 指定头文件检索路径，将 include 目录添加为公共头文件目录
target_include_directories(raft_node PUBLIC include)

# Google Test
# 指示 CMake 查找已安装的 Geoogle Test 库
find_package(GTest REQUIRED)
# 将 Google Test 的头文件目录添加到项目的头文件搜索路径中
include_directories(${GTEST_INCLUDE_DIRS})
# 创建一个名为 runTests 的可执行文件，该文件由 test.cpp 源文件编译而成。
add_executable(runTests tests/test_raft.cpp)
# 指定 runTests 目标链接 Google Test 库，这样编译器在生成可执行文件时就会把 Google Test 的实现代码合并进去
target_link_libraries(runTests ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)
