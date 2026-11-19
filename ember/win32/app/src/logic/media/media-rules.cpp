/*
 * ------------------------------------------------------------------------
 * 名称：media-rules.cpp
 * 说明：介质模块纯规则的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "logic/media/media-rules.hpp"

#include <algorithm>
#include <cmath>

namespace ember::win32::logic {

namespace {

/** 面板里至少列出的软驱数；软驱 A 一直存在，还没读到状态时也先列出，按钮置灰。 */
constexpr int kMinFloppyDrives = 1;

std::string stringOf(const nlohmann::json& object, const char* key) {
  const auto found = object.find(key);
  return found != object.end() && found->is_string() ? found->get<std::string>() : std::string();
}

bool boolOf(const nlohmann::json& object, const char* key) {
  const auto found = object.find(key);
  return found != object.end() && found->is_boolean() && found->get<bool>();
}

double numberOf(const nlohmann::json& object, const char* key) {
  const auto found = object.find(key);
  return found != object.end() && found->is_number() ? found->get<double>() : 0.0;
}

/** 把秒数写成“分:秒”，超过 1 小时写成“时:分:秒”。 */
std::string clockText(double seconds) {
  const long total = static_cast<long>(std::floor(std::max(0.0, seconds)));
  const long hours = total / 3600;
  const long minutes = (total % 3600) / 60;
  const std::string rest = (total % 60 < 10 ? "0" : "") + std::to_string(total % 60);
  if (hours > 0) return std::to_string(hours) + ":" + (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" + rest;
  return std::to_string(minutes) + ":" + rest;
}

}  // namespace

std::vector<FloppyState> parseFloppies(const nlohmann::json& result) {
  std::vector<FloppyState> floppies;
  const auto list = result.is_object() ? result.find("drives") : result.end();
  if (!result.is_object() || list == result.end() || !list->is_array()) return floppies;
  for (const auto& item : *list) {
    if (!item.is_object() || !item.contains("drive") || !item["drive"].is_number_integer()) continue;
    FloppyState floppy;
    floppy.drive = item["drive"].get<int>();
    floppy.inserted = boolOf(item, "inserted");
    floppy.fileName = stringOf(item, "fileName");
    floppy.writeProtected = boolOf(item, "writeProtected");
    floppy.modified = boolOf(item, "modified");
    floppy.cylinder = static_cast<int>(numberOf(item, "cylinder"));
    floppy.motor = boolOf(item, "motor");
    floppies.push_back(std::move(floppy));
  }
  std::sort(floppies.begin(), floppies.end(), [](const FloppyState& a, const FloppyState& b) { return a.drive < b.drive; });
  return floppies;
}

std::optional<TapeState> parseTape(const nlohmann::json& status) {
  if (!status.is_object() || !status.contains("mode")) return std::nullopt;
  TapeState tape;
  tape.connected = boolOf(status, "connected");
  tape.loaded = boolOf(status, "loaded");
  tape.fileName = stringOf(status, "fileName");
  const std::string mode = stringOf(status, "mode");
  tape.mode = mode == "playing" ? TapeMode::Playing : mode == "recording" ? TapeMode::Recording : TapeMode::Stopped;
  tape.position = numberOf(status, "position");
  tape.duration = numberOf(status, "duration");
  return tape;
}

std::optional<PrinterMode> parsePrinter(const nlohmann::json& result) {
  const auto status = result.is_object() ? result.find("status") : result.end();
  if (!result.is_object() || status == result.end() || !status->is_object()) return std::nullopt;
  const std::string state = stringOf(*status, "state");
  if (state == "printing") return PrinterMode::Printing;
  if (state == "done") return PrinterMode::Done;
  return PrinterMode::Idle;
}

std::string driveLabel(int drive) { return std::string(1, static_cast<char>('A' + drive)); }

std::string diskText(const std::optional<FloppyState>& floppy) {
  if (!floppy || !floppy->inserted) return "空";
  return floppy->fileName + (floppy->modified ? "（有未保存的修改）" : "");
}

std::string floppyStateText(const std::optional<FloppyState>& floppy) {
  if (!floppy || !floppy->inserted) return "无盘片";
  return floppy->motor ? "读写中（磁道 " + std::to_string(floppy->cylinder) + "）" : "空闲";
}

std::string printerStateText(const std::optional<PrinterMode>& printer) {
  switch (printer.value_or(PrinterMode::Idle)) {
    case PrinterMode::Printing: return "打印中";
    case PrinterMode::Done: return "打印完成";
    case PrinterMode::Idle: break;
  }
  return "空闲";
}

std::string tapeNameText(const std::optional<TapeState>& tape) { return tape && tape->loaded ? tape->fileName : std::string("空"); }

std::string tapeStateText(const std::optional<TapeState>& tape) {
  if (!tape || !tape->loaded) return "无磁带";
  switch (tape->mode) {
    case TapeMode::Playing: return "播放中";
    case TapeMode::Recording: return "录音中";
    case TapeMode::Stopped: break;
  }
  return "已停止";
}

std::string tapePositionText(const std::optional<TapeState>& tape) {
  if (!tape || !tape->loaded) return "—";
  return tape->duration > 0 ? clockText(tape->position) + " / " + clockText(tape->duration) : clockText(tape->position);
}

FloppyAvailability floppyAvailability(const std::optional<FloppyState>& floppy) {
  if (!floppy) return {};
  return {true, floppy->inserted, floppy->inserted};
}

TapeAvailability tapeAvailability(const std::optional<TapeState>& tape) {
  if (!tape) return {};
  const bool stopped = tape->mode == TapeMode::Stopped;
  return {tape->loaded, !tape->loaded || (tape->connected && stopped), tape->connected && stopped, !stopped};
}

contracts::MediaView buildMediaView(const std::vector<FloppyState>& floppies, const std::optional<TapeState>& tape, const std::optional<PrinterMode>& printer) {
  contracts::MediaView view;
  const int count = std::max(kMinFloppyDrives, floppies.empty() ? 0 : floppies.back().drive + 1);
  for (int drive = 0; drive < count; ++drive) {
    const auto found = std::find_if(floppies.begin(), floppies.end(), [drive](const FloppyState& item) { return item.drive == drive; });
    const std::optional<FloppyState> floppy = found == floppies.end() ? std::nullopt : std::optional<FloppyState>(*found);
    const FloppyAvailability available = floppyAvailability(floppy);
    contracts::MediaFloppyRow row;
    row.drive = drive;
    row.label = "软驱 " + driveLabel(drive);
    row.disk = diskText(floppy);
    row.state = floppyStateText(floppy);
    row.writeProtected = floppy && floppy->writeProtected;
    row.canWriteProtect = available.writeProtect;
    row.canInsert = available.insert;
    row.canEject = available.eject;
    view.floppies.push_back(std::move(row));
  }
  const TapeAvailability available = tapeAvailability(tape);
  view.tape = {tapeNameText(tape), tapePositionText(tape), tapeStateText(tape), available.eject, available.play, available.record, available.stop};
  view.printer = printerStateText(printer);
  return view;
}

}  // namespace ember::win32::logic
