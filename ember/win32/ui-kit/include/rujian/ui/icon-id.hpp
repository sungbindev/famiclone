/*
 * ------------------------------------------------------------------------
 * 名称：icon-id.hpp
 * 说明：图标编号：界面与已登记组件用到的全部图标，名称与组件库里的 icon<名称> 对应。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：由 tools/gen-icons.mjs 生成，不要手改；图标图形的来源与许可见 third_party。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>

namespace rujian::ui {

/** 图标编号。 */
enum class IconId : std::uint16_t {
  Adjustments,
  AlertCircle,
  AlertTriangle,
  AppWindow,
  Apple,
  ArrowBackUp,
  ArrowDown,
  ArrowLeft,
  ArrowRight,
  ArrowUp,
  ArrowsDiff,
  Bell,
  Binary,
  Category,
  Check,
  ChevronDown,
  ChevronRight,
  ChevronUp,
  CircleCheck,
  CircleX,
  Clipboard,
  Clock,
  Copy,
  Cut,
  DeviceAudioTape,
  DeviceFloppy,
  Eraser,
  Eye,
  EyeOff,
  File,
  FileCode,
  FileExport,
  FileText,
  FileUnknown,
  FileUpload,
  FileZip,
  Filter,
  Folder,
  FolderOpen,
  FolderPlus,
  HandClick,
  HardDiskOne,
  InfoCircle,
  LayoutBottombarCollapse,
  Lock,
  LockOpen,
  Pause,
  Pencil,
  Photo,
  PlayOne,
  PlayerEject,
  PlayerPause,
  PlayerPlay,
  PlayerRecord,
  PlayerStop,
  Plus,
  Power,
  Printer,
  Refresh,
  Reload,
  Replace,
  Rotate,
  SelectAll,
  Send,
  Settings,
  Sparkles,
  Square,
  Trash,
  Unlink,
  Upload,
  Volume,
  VolumeOff,
  X,
  Count
};

}  // namespace rujian::ui
