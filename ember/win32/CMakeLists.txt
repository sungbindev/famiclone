# ------------------------------------------------------------------------
# 名称：CMakeLists.txt
# 说明：Win32 原生外壳的构建入口：用 MSVC v143 工具集构建核心静态库与原生外壳的各个目标，目标系统为 Windows 7 SP1 及以后的 64 位 Windows。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-19
# 备注：运行库静态链接，不要求用户安装 VC 运行库；核心源码仍在 ../core，这里只经 add_subdirectory 使用它，不改变 Electron 外壳用的构建；
#       用 CMakePresets.json 里的预设配置，生成器、工具集与平台都固定在预设里。
# ------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0091 NEW)

project(ember_win32 LANGUAGES C CXX)

if(NOT WIN32)
  message(FATAL_ERROR "Win32 原生外壳只能在 Windows 上构建")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 目标系统为 Windows 7：比它新的系统接口在编译时就不可见，要用只能运行时检测。
add_compile_definitions(_WIN32_WINNT=0x0601 WINVER=0x0601 NTDDI_VERSION=0x06010000)

set(EMBER_CORE_VERSION "0.0.0-dev" CACHE STRING "核心版本，构建脚本取自 package.json")

# 核心的测试登记在子目录里，顶层也要开启，ctest 才能从构建目录的顶层找到它们。
enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../core ${CMAKE_CURRENT_BINARY_DIR}/core)

add_subdirectory(probe)
add_subdirectory(tools/export-dump)
add_subdirectory(ui-kit)
add_subdirectory(app)
