/*
 * ------------------------------------------------------------------------
 * 名称：miniaudio-impl.c
 * 说明：miniaudio 的实现编译单元：展开 miniaudio.h 里的实现部分。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只编译播放设备部分：MA_NO_ENCODING、MA_NO_DECODING、MA_NO_GENERATION、MA_NO_ENGINE、MA_NO_NODE_GRAPH、MA_NO_RESOURCE_MANAGER
 *       由 core/CMakeLists.txt 里的 ember_miniaudio 目标公开定义，头文件的使用者也要用同样的宏，否则结构体布局可能不同；
 *       第三方代码不在项目的警告范围内，CMake 里对本目标关闭警告。
 * ------------------------------------------------------------------------
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
