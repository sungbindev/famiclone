/*
 * ------------------------------------------------------------------------
 * 名称：file-drag.hpp
 * 说明：窗口里文件的拖放：向别的程序拖出内存里的文件（系统的“虚拟文件”拖放，资源管理器收到后按名称与内容创建文件与文件夹），以及让窗口接收从系统文件管理器拖入的文件，和窗口内部拖动条目的识别。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：基于 OLE 拖放（IDataObject、IDropSource、IDropTarget），需要在窗口所在的（单线程套间的）线程上使用；
 *       拖出：startFileDrag 在系统的拖放消息循环里阻塞，到拖放结束才返回；数据对象同时带一个窗口内部用的标记（进程内的数字，不含任何内容），本窗口的拖放目标据此认出这是自己人拖的；
 *       拖入：FileDropTarget 把窗口登记成拖放目标，按拖动位置向使用方询问能不能放、放下时交出路径或内部标记；窗口销毁前要先销毁它。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ember::win32::platform {

/** 拖出到别的程序里的一个文件或文件夹。 */
struct DragItem {
  /** 相对路径，用 / 分隔，第一级是拖出的条目的名称。 */
  std::string path;
  bool directory{false};
  /** 文件的内容；文件夹为空。 */
  std::vector<std::uint8_t> content;
};

/**
 * 开始一次拖放，到拖放结束才返回。
 * @param items 拖出到别的程序里的内容；为空时只有窗口内部的拖动。
 * @param internalToken 窗口内部拖动的标记，不为 0。
 */
void startFileDrag(const std::vector<DragItem>& items, std::uint64_t internalToken);

/** 拖进窗口的东西的来源。 */
enum class DropKind {
  /** 从系统文件管理器等别的程序拖进来的文件。 */
  WindowsFiles,
  /** 本程序用 startFileDrag 拖出来的内容。 */
  Internal
};

/** 拖放目标的回调。 */
struct DropCallbacks {
  /** 拖到屏幕坐标（设备像素）上：返回这里能不能放。 */
  std::function<bool(DropKind kind, int screenX, int screenY)> over;
  /** 在屏幕坐标上放下：Windows 文件给出路径（UTF-8），内部拖动给出标记。 */
  std::function<void(DropKind kind, int screenX, int screenY, const std::vector<std::string>& paths, std::uint64_t internalToken)> drop;
  /** 拖动离开了窗口，或放下之后。 */
  std::function<void()> leave;
};

/** 窗口上的拖放目标。 */
class FileDropTarget {
 public:
  /**
   * @param window 窗口句柄（HWND）。
   * @param callbacks 回调。
   */
  FileDropTarget(void* window, DropCallbacks callbacks);
  ~FileDropTarget();
  FileDropTarget(const FileDropTarget&) = delete;
  FileDropTarget& operator=(const FileDropTarget&) = delete;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace ember::win32::platform
