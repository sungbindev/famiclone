/*
 * ------------------------------------------------------------------------
 * 名称：process-control.hpp
 * 说明：进程操作：取当前进程号、等待另一个进程退出、启动软件自己（可带命令行参数）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：参数都是 UTF-8 文字，启动时按 Windows 命令行规则加引号。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ember::win32::platform {

/** 当前进程号。 */
[[nodiscard]] std::uint32_t currentProcessId();

/**
 * 等待进程退出。
 * @param processId 进程号。
 * @param timeoutMilliseconds 最多等多久（毫秒）。
 * @returns 进程已经退出（或根本不存在）时为 true；超时仍在运行为 false。
 */
[[nodiscard]] bool waitForProcessExit(std::uint32_t processId, unsigned timeoutMilliseconds);

/**
 * 启动一个程序，不等它结束。
 * @param executable 程序路径。
 * @param arguments 命令行参数（UTF-8）。
 * @returns 启动成功时为 true。
 */
[[nodiscard]] bool startProcess(const std::filesystem::path& executable, const std::vector<std::string>& arguments);

}  // namespace ember::win32::platform
