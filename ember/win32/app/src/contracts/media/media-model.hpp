/*
 * ------------------------------------------------------------------------
 * 名称：media-model.hpp
 * 说明：介质模块给界面层的模型：设备面板每一行要显示的文字与各按钮是否可用（软驱、磁带机、打印机），以及用户的操作（载入镜像、取出、写保护、磁带的取出、播放、录音、停止）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文字与可用状态由逻辑层算好，界面只显示；规则见 docs/05-界面.md 第 4.3 节。
 *       软驱的读写状态与磁道、磁带的位置、打印机的状态随机器运行变化，核心不推送，所以设备面板展开期间由界面定时调用 refresh 读取，收起后不再调用。
 *       磁带的时间轴（声音大小与空白段）不放进 MediaView（它会被频繁比较），由 timelineView 单独给出，只在时间轴弹层打开时读取。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 设备面板里软驱的一行。 */
struct MediaFloppyRow {
  /** 驱动器编号，从 0 开始。 */
  int drive{0};
  /** 行首的名称，如“软驱 A”。 */
  std::string label;
  /** 盘片：镜像文件名，有未保存的修改时带注明；没有盘片时是“空”。 */
  std::string disk;
  /** 状态文字：无盘片、空闲、读写中（带磁道）。 */
  std::string state;
  /** 盘片是否写保护（写保护按钮的按下态）。 */
  bool writeProtected{false};
  bool canWriteProtect{false};
  bool canInsert{false};
  bool canEject{false};
  bool operator==(const MediaFloppyRow&) const = default;
};

/** 设备面板里磁带机的一行。 */
struct MediaTapeRow {
  /** 磁带：声音文件名；没有磁带时是“空”。 */
  std::string disk;
  /** 位置：“当前 / 总时长”，总时长不知道时只写当前位置，没有磁带时是破折号。 */
  std::string position;
  /** 状态文字：无磁带、已停止、播放中、录音中。 */
  std::string state;
  bool canEject{false};
  bool canPlay{false};
  bool canRecord{false};
  bool canStop{false};
  /** 位置列是否可点击打开时间轴：里面有磁带且不在录音（录音时不能定位）。 */
  bool canOpenTimeline{false};
  bool operator==(const MediaTapeRow&) const = default;
};

/** 时间轴上的一段空白（秒）。 */
struct MediaTimelineBlank {
  double start{0};
  double end{0};
  bool operator==(const MediaTimelineBlank&) const = default;
};

/** 磁带时间轴弹层此刻要显示的内容。 */
struct MediaTimelineView {
  /** 有磁带且可以定位：为 false 时弹层应关闭。 */
  bool available{false};
  /** 总时长与当前位置（秒）。 */
  double duration{0};
  double position{0};
  /** 轨道中间的说明：分析中带进度、分析失败；分析完成时为空。 */
  std::string notice;
  /** 分析完成时为 true；包络与空白段只在这时有效。 */
  bool ready{false};
  /** 分析的磁带序号：变了说明换了磁带，弹层要重新设置包络。 */
  std::int64_t serial{0};
  /** 包络：每 step 秒一项，0 到 100。 */
  std::vector<std::uint8_t> peaks;
  double step{0};
  std::vector<MediaTimelineBlank> blanks;
};

/** 设备面板此刻要显示的内容；软驱一直至少有一行，磁带机与打印机一直各有一行。 */
struct MediaView {
  std::vector<MediaFloppyRow> floppies;
  MediaTapeRow tape;
  /** 打印机的状态文字：空闲、打印中、打印完成。 */
  std::string printer;
  bool operator==(const MediaView&) const = default;
};

/** 介质模块的模型。 */
class MediaModel {
 public:
  virtual ~MediaModel() = default;

  [[nodiscard]] virtual MediaView view() const = 0;

  /** 弹出系统文件选择窗口，把选定的镜像载入软驱；用户取消时什么都不发生。 */
  virtual void insertFloppy(int drive) = 0;
  /** 取出软驱里的盘片；有尚未写回的修改时核心先写回镜像文件，写回失败则不取出。 */
  virtual void ejectFloppy(int drive) = 0;
  /** 设置或取消盘片的写保护。 */
  virtual void setWriteProtected(int drive, bool writeProtected) = 0;
  /** 取出磁带；播放或录音时先停止。 */
  virtual void ejectTape() = 0;
  /** 播放：里面有磁带就直接播放；没有磁带时先弹出系统文件选择窗口选磁带，载入后磁带机已接在通电的主机上就立即开始播放，否则只载入。 */
  virtual void playTape() = 0;
  /** 弹出系统的保存文件窗口，选定文件后开始录音；用户取消时什么都不发生。 */
  virtual void recordTape() = 0;
  /** 停止播放或录音。 */
  virtual void stopTape() = 0;
  /** 把磁带定位到某个位置（秒）；停止与播放时都可以，录音时不能。 */
  virtual void seekTape(double seconds) = 0;
  /** 磁带时间轴弹层此刻要显示的内容。 */
  [[nodiscard]] virtual MediaTimelineView timelineView() const = 0;

  /** 重新读取软驱、磁带机与打印机的状态（设备面板展开期间定时调用）。 */
  virtual void refresh() = 0;
  /** 订阅内容变化；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void()> handler) = 0;
};

}  // namespace ember::win32::contracts
