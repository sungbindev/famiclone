/*
 * ------------------------------------------------------------------------
 * 名称：media-rules.hpp
 * 说明：介质模块用到的纯规则：从核心回复读出软驱、磁带机与打印机的状态，得出设备面板每一行的文字与各按钮是否可用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则同 Electron 外壳的 media-status.ts（见 docs/05-界面.md 第 4.3 节）：没有盘片时取出与写保护置灰；软驱的状态分无盘片、空闲、读写中（带磁道）；打印机的状态分空闲、打印中、打印完成；
 *       磁带机的状态分无磁带、已停止、播放中、录音中；磁带机是独立设备，载入与取出随时可用：有磁带就能取出（播放或录音时先停止再取出），没有磁带时“播放”可点（用来选磁带），
 *       有磁带时只有磁带机接在通电的主机上才能播放，录音也要接上主机，播放或录音时只有停止可用。状态还没读到（没有值）时对应的按钮全部置灰。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "contracts/media/media-model.hpp"

namespace ember::win32::logic {

/** 一个软驱的状态（media.floppyStatus 的一项）。 */
struct FloppyState {
  int drive{0};
  bool inserted{false};
  std::string fileName;
  bool writeProtected{false};
  bool modified{false};
  int cylinder{0};
  bool motor{false};
  bool operator==(const FloppyState&) const = default;
};

/** 磁带机的工作状态。 */
enum class TapeMode { Stopped, Playing, Recording };

/** 磁带机的状态（media.tapeStatus 的结果）。 */
struct TapeState {
  /** 是否接在通电的主机上。 */
  bool connected{false};
  bool loaded{false};
  std::string fileName;
  TapeMode mode{TapeMode::Stopped};
  double position{0};
  double duration{0};
  bool operator==(const TapeState&) const = default;
};

/** 打印机的状态。 */
enum class PrinterMode { Idle, Printing, Done };

/** 从 media.floppyStatus 的结果读出各软驱，按编号排列；格式不对时为空。 */
[[nodiscard]] std::vector<FloppyState> parseFloppies(const nlohmann::json& result);
/** 从磁带机状态（media.tapeStatus 结果的 status，或 openTape 等结果里的 status）读出；格式不对时没有值。 */
[[nodiscard]] std::optional<TapeState> parseTape(const nlohmann::json& status);
/** 从 media.printerStatus 的结果读出打印机状态；格式不对时没有值。 */
[[nodiscard]] std::optional<PrinterMode> parsePrinter(const nlohmann::json& result);

/** 驱动器的显示名：0 为 A，1 为 B。 */
[[nodiscard]] std::string driveLabel(int drive);
/** 盘片的显示文字：镜像文件名，有未保存的修改时在后面注明；没有盘片或软驱没有连接时为“空”。 */
[[nodiscard]] std::string diskText(const std::optional<FloppyState>& floppy);
/** 软驱的状态文字：无盘片、空闲、读写中（带磁道）；软驱没有连接时按无盘片。 */
[[nodiscard]] std::string floppyStateText(const std::optional<FloppyState>& floppy);
/** 打印机的状态文字；还没读到时按空闲显示。 */
[[nodiscard]] std::string printerStateText(const std::optional<PrinterMode>& printer);
/** 磁带的显示文字：声音文件名；没有磁带或磁带机没有连接时为“空”。 */
[[nodiscard]] std::string tapeNameText(const std::optional<TapeState>& tape);
/** 磁带机的状态文字：无磁带、已停止、播放中、录音中；磁带机没有连接时按无磁带。 */
[[nodiscard]] std::string tapeStateText(const std::optional<TapeState>& tape);
/** 磁带的位置文字：“当前 / 总时长”，总时长不知道时只写当前位置；没有磁带时为破折号。 */
[[nodiscard]] std::string tapePositionText(const std::optional<TapeState>& tape);

/** 软驱各按钮是否可用；软驱没有连接（还没读到状态）时全部不可用。 */
struct FloppyAvailability {
  bool insert{false};
  bool eject{false};
  bool writeProtect{false};
};
[[nodiscard]] FloppyAvailability floppyAvailability(const std::optional<FloppyState>& floppy);

/** 磁带机各按钮是否可用；状态还没读到时全部不可用。 */
struct TapeAvailability {
  bool eject{false};
  bool play{false};
  bool record{false};
  bool stop{false};
};
[[nodiscard]] TapeAvailability tapeAvailability(const std::optional<TapeState>& tape);

/** 设备面板此刻的内容：软驱至少列出软驱 A，还没读到状态时按钮置灰。 */
[[nodiscard]] contracts::MediaView buildMediaView(const std::vector<FloppyState>& floppies, const std::optional<TapeState>& tape, const std::optional<PrinterMode>& printer);

}  // namespace ember::win32::logic
