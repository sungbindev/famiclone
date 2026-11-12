/*
 * ------------------------------------------------------------------------
 * 名称：DebugServer.h
 * 说明：调试服务的对外接口。模拟器核心经这里的钩子上报指令与内存访问，
 *       模拟线程经这里的帧门控接受外部授权，外部分析工具经本机 TCP 驱动模拟器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-09
 * 备注：只有设置了环境变量 FCDEV_DEBUG_PORT 时才启用，未启用时各钩子只多一次变量判断。
 * ------------------------------------------------------------------------
 */
#ifndef __DEBUGSERVER_INCLUDED__
#define __DEBUGSERVER_INCLUDED__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "typedef.h"
#include "cpu.h"

// 调试模式开关：1 表示模拟线程只在服务授权时推进帧
extern volatile LONG g_dbgMode;

// 钩子总开关：任一钩子（断点、跟踪、代码数据标记、监视）有效时为 1，CPU 核心只检查它
extern volatile LONG g_dbgActive;

// 外部注入的按键状态，下标为 DirectInput 扫描码，值非 0 表示按下
extern BYTE g_dbgKeys[256];

// 外部注入的手柄按键，下标为手柄编号（位顺序：A、B、Select、Start、上、下、左、右）
extern BYTE g_dbgPad[4];

// 启动调试服务；未设置 FCDEV_DEBUG_PORT 时什么也不做
void DebugServer_Start();

// 模拟线程每帧开始前调用，返回 FALSE 表示当前没有被授权的帧
BOOL DebugServer_BeginFrame();

// 模拟线程每帧结束后调用
void DebugServer_EndFrame();

// 本帧结束后是否需要刷新窗口画面（只在一批帧的最后一帧刷新）
BOOL DebugServer_WantBlt();

// CPU 取指后、执行前调用；返回 TRUE 表示要求停止，CPU 核心应回退 PC 并退出本次执行
BOOL DebugHook_OnInstr(WORD pc, BYTE opcode, const R6502& r);

// CPU 数据读写时调用
void DebugHook_OnRead(WORD addr, BYTE data);
void DebugHook_OnWrite(WORD addr, BYTE data);

// 中断入口时调用，type：0 为 NMI，1 为 IRQ
void DebugHook_OnInterrupt(int type, WORD vectorTarget);

// 软驱读写一个扇区时调用，lba 为逻辑扇区号，write 为 1 表示写
void DebugHook_OnDisk(int lba, int write);

#endif // !__DEBUGSERVER_INCLUDED__
