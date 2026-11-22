/*
 * ------------------------------------------------------------------------
 * 名称：media-module.cpp
 * 说明：介质模块登记项的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/media/media-module.hpp"

#include <algorithm>

namespace ember::win32::logic {

namespace {

/** 软驱图标的方块就是读写灯，给面板灯的 inIcon。 */
constexpr bool kLampIsIconAccent = true;

/** 软盘名状态项的个数：核心的软驱编号最大到 3。 */
constexpr int kMaxFloppyDrives = 4;

/** 拖入窗口的软盘镜像载入软驱 A。 */
constexpr int kDropFloppyDrive = 0;

/** 可以拖进窗口的文件扩展名：软盘镜像；磁带的声音文件（ffmpeg 能解码的常见格式）。 */
const std::vector<std::string> kFloppyExtensions{"img", "ima", "vfd", "dsk", "flp"};
const std::vector<std::string> kSoundExtensions{"m4a", "mp3", "wav", "aac", "flac", "ogg", "opus", "mp4", "mka"};

/** 第 drive 个软驱的软盘名状态项。 */
contracts::ShellStatusItemDefinition floppyStatusItem(MediaController& controller, int drive) {
  contracts::ShellStatusItemDefinition item;
  item.id = "media.floppy." + std::to_string(drive);
  item.view = [&controller, drive]() -> std::optional<contracts::ShellStatusItemView> {
    const auto& floppies = controller.floppies();
    const auto found = std::find_if(floppies.begin(), floppies.end(), [drive](const FloppyState& floppy) { return floppy.drive == drive; });
    if (found == floppies.end() || !found->inserted) return std::nullopt;
    const std::string name = diskText(*found);
    return contracts::ShellStatusItemView{floppies.size() > 1 ? driveLabel(drive) + ": " + name : name, "软驱 " + driveLabel(drive) + " 的盘片", contracts::ShellTone::Normal};
  };
  return item;
}

}  // namespace

contracts::ShellModule createMediaModule(MediaController& controller) {
  contracts::ShellModule module;
  module.name = "media";

  contracts::ShellPanelDefinition panel;
  panel.id = kMediaPanelId;
  panel.title = "设备";
  panel.icon = contracts::ShellIcon::DeviceFloppy;
  panel.autoCollapse = true;
  panel.lamp = [&controller]() -> std::optional<contracts::ShellPanelLamp> {
    const auto& floppies = controller.floppies();
    if (floppies.empty()) return std::nullopt;
    const bool lit = std::any_of(floppies.begin(), floppies.end(), [](const FloppyState& floppy) { return floppy.motor; });
    return contracts::ShellPanelLamp{lit, "软驱读写灯", kLampIsIconAccent};
  };
  module.bottomPanels.push_back(std::move(panel));

  for (int drive = 0; drive < kMaxFloppyDrives; ++drive) module.statusItems.push_back(floppyStatusItem(controller, drive));

  module.fileHandlers.push_back({"media.floppyImage", kFloppyExtensions, [&controller](const contracts::DroppedFile& file) { controller.insertDroppedFloppy(kDropFloppyDrive, file.id); }});
  module.fileHandlers.push_back({"media.tapeSound", kSoundExtensions, [&controller](const contracts::DroppedFile& file) { controller.loadDroppedTape(file.id); }});
  module.attach = [&controller](contracts::ShellApi& shell) { return controller.attach(shell); };
  return module;
}

}  // namespace ember::win32::logic
