# ------------------------------------------------------------------------
# 名称：CMakeLists.txt
# 说明：C++ 核心的构建：核心库（不依赖 Node）、Node-API 原生模块（由 cmake-js 编译时才构建）与测试（EMBER_BUILD_TESTS）。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-13
# 备注：第三方库源码放在 core/third_party（清单见 docs/01-软件架构设计.md 第 10 节），编译时不联网；Windows 静态链接 C++ 运行库；
#       生成的代码（src/contracts/*/generated）由 npm run schema 产生，配置前须先生成；
#       SQLite 先登记目标，核心库用到时再链接（EXCLUDE_FROM_ALL，没人链接就不编译）；QuickJS-ng、miniz 与 miniaudio 已被核心库链接。
# ------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0091 NEW)

project(ember_core LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(EMBER_BUILD_TESTS "构建 C++ 测试" OFF)
set(EMBER_CORE_VERSION "0.0.0-dev" CACHE STRING "核心版本，构建脚本取自 package.json")
set(EMBER_THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# 测试在 Debug 配置下构建，但 ROM 测试要模拟上万帧画面：对 MSVC 改用 /O2，去掉与它冲突的运行时检查，并关闭标准库的迭代器检查。
if(EMBER_BUILD_TESTS AND MSVC)
  string(REGEX REPLACE "/RTC[1csu]+" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  string(REPLACE "/Od" "/O2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  string(REPLACE "/Ob0" "/Ob2" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
endif()

# ---------------------------------------------------------------- 第三方库

add_library(ember_json INTERFACE)
target_include_directories(ember_json SYSTEM INTERFACE ${EMBER_THIRD_PARTY}/nlohmann-json/include)

add_library(ember_sqlite STATIC EXCLUDE_FROM_ALL ${EMBER_THIRD_PARTY}/sqlite/sqlite3.c)
target_include_directories(ember_sqlite PUBLIC ${EMBER_THIRD_PARTY}/sqlite)
target_compile_definitions(ember_sqlite PRIVATE SQLITE_DQS=0 SQLITE_DEFAULT_FOREIGN_KEYS=1 SQLITE_OMIT_LOAD_EXTENSION=1 SQLITE_THREADSAFE=1)

# mapper 插件的沙箱：只编译引擎核心，不编译提供文件与进程访问的 quickjs-libc。
add_library(ember_quickjs STATIC
  ${EMBER_THIRD_PARTY}/quickjs-ng/quickjs.c
  ${EMBER_THIRD_PARTY}/quickjs-ng/libregexp.c
  ${EMBER_THIRD_PARTY}/quickjs-ng/libunicode.c
  ${EMBER_THIRD_PARTY}/quickjs-ng/dtoa.c)
target_include_directories(ember_quickjs SYSTEM PUBLIC ${EMBER_THIRD_PARTY}/quickjs-ng)
set_target_properties(ember_quickjs PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(MSVC)
  target_compile_options(ember_quickjs PRIVATE /W0 /experimental:c11atomics)
else()
  target_compile_definitions(ember_quickjs PRIVATE _GNU_SOURCE)
  target_compile_options(ember_quickjs PRIVATE -w -funsigned-char)
endif()

add_library(ember_miniz STATIC ${EMBER_THIRD_PARTY}/miniz/miniz.c)
target_include_directories(ember_miniz SYSTEM PUBLIC ${EMBER_THIRD_PARTY}/miniz)
target_compile_definitions(ember_miniz PUBLIC MINIZ_NO_ZLIB_COMPATIBLE_NAMES)

# 系统声音输出：miniaudio 的实现单独一个编译单元，只编译播放设备部分；第三方代码关闭警告。
# 这些宏要对头文件的使用者也生效（PUBLIC），与 third_party/miniaudio/miniaudio-impl.c 里的定义一致。
add_library(ember_miniaudio STATIC ${EMBER_THIRD_PARTY}/miniaudio/miniaudio-impl.c)
target_include_directories(ember_miniaudio SYSTEM PUBLIC ${EMBER_THIRD_PARTY}/miniaudio)
target_compile_definitions(ember_miniaudio PUBLIC MA_NO_ENCODING MA_NO_DECODING MA_NO_GENERATION MA_NO_ENGINE MA_NO_NODE_GRAPH MA_NO_RESOURCE_MANAGER)
set_target_properties(ember_miniaudio PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(MSVC)
  target_compile_options(ember_miniaudio PRIVATE /W0)
else()
  target_compile_options(ember_miniaudio PRIVATE -w)
endif()
if(APPLE)
  target_link_libraries(ember_miniaudio PUBLIC "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
elseif(UNIX)
  target_link_libraries(ember_miniaudio PUBLIC ${CMAKE_DL_LIBS} pthread m)
endif()

# ---------------------------------------------------------------- 核心库

file(GLOB_RECURSE EMBER_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(FILTER EMBER_CORE_SOURCES EXCLUDE REGEX "/src/binding/")
# 按平台分开的实现：*-win.cpp 只在 Windows 编译，*-mac.cpp 只在 macOS 编译。
if(NOT WIN32)
  list(FILTER EMBER_CORE_SOURCES EXCLUDE REGEX "-win\\.cpp$")
endif()
if(NOT APPLE)
  list(FILTER EMBER_CORE_SOURCES EXCLUDE REGEX "-mac\\.cpp$")
endif()

add_library(ember_core STATIC ${EMBER_CORE_SOURCES})
target_include_directories(ember_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# miniz 的头文件对使用者可见（测试要写插件包）；QuickJS 与 miniaudio 只在核心库内部使用，头文件不外露。
target_link_libraries(ember_core PUBLIC ember_json ember_miniz PRIVATE ember_quickjs ember_miniaudio)
target_compile_definitions(ember_core PUBLIC EMBER_CORE_VERSION="${EMBER_CORE_VERSION}")

# 调试服务的 TCP 监听（platform/network）：Windows 用 Winsock。
if(WIN32)
  target_link_libraries(ember_core PUBLIC ws2_32)
endif()

if(MSVC)
  target_compile_options(ember_core PUBLIC /utf-8 /W4 /permissive- /EHsc)
else()
  target_compile_options(ember_core PUBLIC -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------- 原生模块（cmake-js）

if(CMAKE_JS_INC)
  execute_process(
    COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

  add_library(ember_binding SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/binding/binding.cpp ${CMAKE_JS_SRC})
  set_target_properties(ember_binding PROPERTIES PREFIX "" SUFFIX ".node")
  target_include_directories(ember_binding PRIVATE ${CMAKE_JS_INC} ${NODE_ADDON_API_DIR})
  target_compile_definitions(ember_binding PRIVATE NAPI_VERSION=8 NAPI_CPP_EXCEPTIONS NODE_ADDON_API_DISABLE_DEPRECATED)
  target_link_libraries(ember_binding PRIVATE ember_core ${CMAKE_JS_LIB})

  # 发布构建的调试符号单独保存为 ember_binding.pdb，不进入 .node 文件。
  if(MSVC)
    target_compile_options(ember_binding PRIVATE $<$<CONFIG:Release>:/Zi>)
    target_compile_options(ember_core PRIVATE $<$<CONFIG:Release>:/Zi>)
    target_link_options(ember_binding PRIVATE "$<$<CONFIG:Release>:/DEBUG;/OPT:REF;/OPT:ICF>")
  endif()

  if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
    execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
  endif()

  # Windows 键盘独占模块：独立的小模块，不依赖核心库，只由主进程加载。
  if(WIN32)
    add_library(ember_keyboard SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/binding/keyboard-capture-win.cpp ${CMAKE_JS_SRC})
    set_target_properties(ember_keyboard PROPERTIES PREFIX "" SUFFIX ".node")
    target_include_directories(ember_keyboard PRIVATE ${CMAKE_JS_INC} ${NODE_ADDON_API_DIR})
    target_compile_definitions(ember_keyboard PRIVATE NAPI_VERSION=8 NAPI_CPP_EXCEPTIONS NODE_ADDON_API_DISABLE_DEPRECATED)
    target_compile_options(ember_keyboard PRIVATE /utf-8 /W4 /permissive- /EHsc)
    target_link_libraries(ember_keyboard PRIVATE user32 comctl32 ${CMAKE_JS_LIB})
  endif()
endif()

# ---------------------------------------------------------------- 测试

if(EMBER_BUILD_TESTS)
  set(INSTALL_GTEST OFF CACHE INTERNAL "")
  set(gtest_force_shared_crt OFF CACHE INTERNAL "")
  add_subdirectory(${EMBER_THIRD_PARTY}/googletest ${CMAKE_CURRENT_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)

  enable_testing()
  file(GLOB_RECURSE EMBER_CORE_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp)
  add_executable(ember_core_tests ${EMBER_CORE_TESTS})
  target_link_libraries(ember_core_tests PRIVATE ember_core GTest::gtest_main)
  target_compile_definitions(ember_core_tests PRIVATE EMBER_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/data" EMBER_PLUGIN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../build/ember/plugins")
  # 磁带解码的端到端用例用随应用附带的 ffmpeg；当前平台没有编译好的程序时用例跳过。
  if(WIN32)
    target_compile_definitions(ember_core_tests PRIVATE EMBER_FFMPEG_PROGRAM="${CMAKE_CURRENT_SOURCE_DIR}/third_party/ffmpeg/bin/win32-x64/ffmpeg.exe")
  elseif(APPLE)
    target_compile_definitions(ember_core_tests PRIVATE EMBER_FFMPEG_PROGRAM="${CMAKE_CURRENT_SOURCE_DIR}/third_party/ffmpeg/bin/darwin-arm64/ffmpeg")
  endif()
  include(GoogleTest)
  gtest_discover_tests(ember_core_tests DISCOVERY_MODE PRE_TEST)
endif()
