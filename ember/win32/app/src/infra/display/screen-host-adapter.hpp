/*
 * ------------------------------------------------------------------------
 * 名称：screen-host-adapter.hpp
 * 说明：运行画面宿主的实现：持有平台层的画面子窗口、原始输入采集与手柄读取，把核心送来的帧交给画面子窗口显示，把独占键盘的按键、捕获鼠标的位移与手柄的状态转成输入端口的调用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：规则见 docs/08-Win32原生外壳.md 第 5.5 节与 docs/05-界面.md 第 7 节：帧到界面线程后只取最新一帧上传并显示，旧帧丢弃；
 *       键盘独占期间的物理按键经扫描码对照表转成键位名（不独占的“共享”采集时 F 键、Tab、Esc 与带 Ctrl、Alt、Meta 的组合留给外壳，不上报）；捕获鼠标期间指针的相对位移按画面的显示比例换算成标准帧的位移，双击右键（400 毫秒内两次右键按下，第二次不送给机器）释放；
 *       手柄每个画面帧（约 16 毫秒）读一次，状态变化时上报；窗口消息经 handleMessage 交给原始输入采集（WM_INPUT）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "contracts/core-gateway/frame-source.hpp"
#include "contracts/input/screen-input.hpp"
#include "contracts/runtime/screen-host.hpp"

namespace ember::win32::platform {
class RawInputCapture;
class ScreenWindow;
class UiThread;
class XInputGamepads;
}  // namespace ember::win32::platform

namespace ember::win32::infra {

/** 画面延迟的摘要（毫秒）：核心发布一帧到它被画出。 */
struct DisplayLatency {
  /** 记录到的帧数。 */
  std::uint64_t count{0};
  double average{0};
  double p95{0};
  double max{0};
};

/** 运行画面宿主。 */
class ScreenHostAdapter final : public contracts::ScreenHost {
 public:
  /**
   * @param parentWindow 主窗口句柄（HWND）。
   * @param frames 核心的画面帧来源。
   * @param input 输入端口，独占键盘、捕获鼠标与手柄的事件都交给它。
   * @param ui 界面线程，手柄的定时读取用它。
   * @param onError 出错的记录（开启键盘独占失败等）。
   * @throws std::runtime_error 创建不出 Direct3D 11 设备（硬件与 WARP 都不行）时。
   */
  ScreenHostAdapter(void* parentWindow, contracts::FrameSource& frames, contracts::ScreenInput& input, platform::UiThread& ui, std::function<void(const std::string&)> onError);
  ~ScreenHostAdapter() override;
  ScreenHostAdapter(const ScreenHostAdapter&) = delete;
  ScreenHostAdapter& operator=(const ScreenHostAdapter&) = delete;

  /** 弹层盖到画面上的区域变了（设备像素，相对主窗口客户区）：把画面子窗口对应的区域挖掉。 */
  void setHoles(const std::vector<shared::ScreenRect>& holes);

  /** 处理窗口消息，作为窗口宿主的消息钩子之一使用；只处理 WM_INPUT。 */
  bool handleMessage(unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result);

  /** 开始记录每一帧从核心发布到画出的延迟（仅开发期的诊断用）。 */
  void enableLatencyRecording() { recordLatency_ = true; }
  /** 到现在记录到的延迟摘要；没有记录时全是 0。 */
  [[nodiscard]] DisplayLatency latency() const;

  // ---- ScreenHost ----
  void setBounds(const shared::ScreenRect& rect) override;
  void setView(shared::ScreenAspect aspect, shared::ScreenScaling scaling, bool showAllLines) override;
  [[nodiscard]] double nominalRatio() const override;
  void setFrameVisible(bool visible) override;
  void setOverlay(const std::uint8_t* pixels, int width, int height, const std::vector<shared::ScreenRect>& glass, int glassRadius) override;
  [[nodiscard]] std::optional<shared::FramePoint> toFramePoint(double x, double y) const override;
  void setKeyboardMode(contracts::KeyboardMode mode) override;
  bool setMouseCaptured(bool captured) override;
  void setOnMouseReleased(std::function<void()> handler) override { onMouseReleased_ = std::move(handler); }

 private:
  void showLatestFrame();
  void pollGamepads();
  void schedulePoll();
  void handleRawKey(std::uint16_t make, bool extended, bool prefixE1, bool up);
  void handleRawMouse(int dx, int dy, int buttons, int pressed, int released, double wheel);

  contracts::FrameSource& frames_;
  contracts::ScreenInput& input_;
  platform::UiThread& ui_;
  std::function<void(const std::string&)> onError_;
  std::unique_ptr<platform::ScreenWindow> window_;
  std::unique_ptr<platform::RawInputCapture> rawInput_;
  std::unique_ptr<platform::XInputGamepads> gamepads_;
  contracts::FrameImage frame_;
  shared::ScreenAspect aspect_{shared::kDefaultScreenAspect};
  shared::ScreenScaling scaling_{shared::kDefaultScreenScaling};
  int cropLines_{0};
  /** 不独占键盘时跟踪的 Ctrl、Alt、Meta 按键状态（commandModifierBit 的位或）。 */
  unsigned commandModifiers_{0};
  std::function<void()> onMouseReleased_;
  /** 捕获期间上一次右键按下的时刻，用来判断双击右键。 */
  std::chrono::steady_clock::time_point lastRightDown_{};
  bool haveRightDown_{false};
  unsigned pollTimer_{0};
  bool destroyed_{false};
  bool recordLatency_{false};
  std::vector<double> latencies_;
};

}  // namespace ember::win32::infra
