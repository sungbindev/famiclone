/*
 * ------------------------------------------------------------------------
 * 名称：screen-overlay-view.cpp
 * 说明：画面提示的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：尺寸取 rujian-ui-kit 的令牌：图标 32、间距 8（space-md）、标题 16（font-size-display）加粗 600、说明 11（font-size-caption）。
 * ------------------------------------------------------------------------
 */

#include "ui/runtime/screen-overlay-view.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "rujian/ui/canvas.hpp"

namespace ember::win32::ui {

namespace {

using rujian::ui::Color;
using rujian::ui::Rect;
using rujian::ui::TextStyle;
using rujian::ui::VerticalAlign;

constexpr double kIconSize = 32;
constexpr double kGapMd = 8;
constexpr double kGapSm = 6;
constexpr double kGapXs = 4;
constexpr double kDisplayFontSize = 16;
constexpr double kCaptionFontSize = 11;
constexpr int kStrongWeight = 600;
constexpr double kIconStroke = 1.0;
/** 提示框的内边距：水平 space-md，垂直 space-sm。 */
constexpr double kPanelPaddingX = kGapMd;
constexpr double kPanelPaddingY = kGapSm;
constexpr double kGlassBorderWidth = 1;

const Color kText = Color::rgba(235, 235, 235);
const Color kBlack = Color::rgba(0, 0, 0);
/** 暂停时的半透明底色：黑色，不透明度 0.55。 */
const Color kPausedFill = Color::rgba(0, 0, 0, 140);
/** 提示框的底色：黑色，不透明度 0.5，下面是模糊后的画面（毛玻璃）。 */
const Color kGlassFill = Color::rgba(0, 0, 0, 128);
/** 提示框的边框：白色，不透明度 0.35，和画面有清晰的界限。 */
const Color kGlassBorder = Color::rgba(255, 255, 255, 90);

constexpr const char* kKeyboardHint = "键盘已被机器独占，快捷键无效，请用鼠标操作菜单";
constexpr const char* kMouseHint = "点击画面捕获鼠标，双击右键释放";

TextStyle textStyle(double size, int weight) {
  TextStyle style;
  style.fontSize = size;
  style.fontWeight = weight;
  style.color = kText;
  style.lineHeight = 0;
  style.wrap = false;
  return style;
}

/** “未开机”的步骤：每一行是加粗的机型名加一句说明。 */
struct Step {
  const char* label;
  const char* text;
};
constexpr Step kSteps[] = {{"学习机", "文件→选择机型，选好即开机"}, {"经典 FC", "文件→打开卡带，选好即开机"}};

}  // namespace

void ScreenOverlayView::paint(rujian::ui::Canvas& canvas) {
  if (state_ != OverlayState::Hidden) paintState(canvas);
  paintPanel(canvas);
}

void ScreenOverlayView::paintState(rujian::ui::Canvas& canvas) const {
  const Rect area = bounds();
  const bool blank = state_ != OverlayState::Paused;
  canvas.fillRect(area, blank ? kBlack : kPausedFill);

  const char* title = state_ == OverlayState::NotPoweredOn ? "未开机" : (state_ == OverlayState::PoweredOff ? "已断电" : "已暂停");
  const auto icon = state_ == OverlayState::Paused ? rujian::ui::IconId::PlayerPause : rujian::ui::IconId::Power;
  const TextStyle titleStyle = textStyle(kDisplayFontSize, kStrongWeight);
  const TextStyle captionStyle = textStyle(kCaptionFontSize, 400);
  const TextStyle labelStyle = textStyle(kCaptionFontSize, kStrongWeight);
  const auto measure = [this](const std::string& text, const TextStyle& style) { return host()->measureText(text, style, 0); };

  const auto titleSize = measure(title, titleStyle);
  double contentHeight = kIconSize + kGapMd + titleSize.height;
  std::string caption;
  if (state_ == OverlayState::PoweredOff) caption = "点系统栏的“运行”，按“开机”，或按 F9";
  double captionHeight = 0;
  if (!caption.empty()) {
    captionHeight = measure(caption, captionStyle).height;
    contentHeight += kGapMd + captionHeight;
  }
  // 步骤：两列对齐（机型名一列，说明一列），整块居中、文字靠左
  double labelWidth = 0;
  double stepWidth = 0;
  std::vector<double> rowHeights;
  if (state_ == OverlayState::NotPoweredOn) {
    for (const Step& step : kSteps) {
      const auto label = measure(step.label, labelStyle);
      const auto text = measure(step.text, captionStyle);
      labelWidth = std::max(labelWidth, label.width);
      stepWidth = std::max(stepWidth, text.width);
      rowHeights.push_back(std::max(label.height, text.height));
    }
    contentHeight += kGapMd;
    for (const double height : rowHeights) contentHeight += height;
    contentHeight += kGapSm * static_cast<double>(rowHeights.size() - 1);
  }

  double y = area.y + (area.height - contentHeight) / 2;
  canvas.drawIcon(icon, Rect{area.x + (area.width - kIconSize) / 2, y, kIconSize, kIconSize}, kText, kIconStroke);
  y += kIconSize + kGapMd;
  canvas.drawText(title, Rect{area.x, y, area.width, titleSize.height}, [&] {
    TextStyle centered = titleStyle;
    centered.align = rujian::ui::TextAlign::Center;
    return centered;
  }(), VerticalAlign::Top);
  y += titleSize.height;
  if (!caption.empty()) {
    y += kGapMd;
    TextStyle centered = captionStyle;
    centered.align = rujian::ui::TextAlign::Center;
    canvas.drawText(caption, Rect{area.x, y, area.width, captionHeight}, centered, VerticalAlign::Top);
  }
  if (state_ == OverlayState::NotPoweredOn) {
    y += kGapMd;
    const double blockWidth = labelWidth + kGapMd + stepWidth;
    const double left = area.x + (area.width - blockWidth) / 2;
    std::size_t index = 0;
    for (const Step& step : kSteps) {
      const double rowHeight = rowHeights[index++];
      canvas.drawText(step.label, Rect{left, y, labelWidth + 1, rowHeight}, labelStyle, VerticalAlign::Top);
      canvas.drawText(step.text, Rect{left + labelWidth + kGapMd, y, stepWidth + 1, rowHeight}, captionStyle, VerticalAlign::Top);
      y += rowHeight + kGapSm;
    }
  }
}

std::optional<ScreenOverlayView::Panel> ScreenOverlayView::layoutPanel() const {
  if (!keyboardHint_ && !mouseHint_) return std::nullopt;
  const Rect area = bounds();
  TextStyle style = textStyle(kDisplayFontSize, 400);
  style.wrap = true;
  // 窗口很小时文字换行，不超出画面
  const double maxTextWidth = std::max(0.0, area.width - 2 * (kGapMd + kPanelPaddingX));
  Panel panel;
  if (keyboardHint_) panel.lines.push_back({kKeyboardHint, host()->measureText(kKeyboardHint, style, maxTextWidth)});
  if (mouseHint_) panel.lines.push_back({kMouseHint, host()->measureText(kMouseHint, style, maxTextWidth)});
  double textWidth = 0;
  double textHeight = 0;
  for (const PanelLine& line : panel.lines) {
    textWidth = std::max(textWidth, line.size.width);
    textHeight += line.size.height;
  }
  textHeight += kGapXs * static_cast<double>(panel.lines.size() - 1);
  const double width = textWidth + 2 * kPanelPaddingX;
  const double height = textHeight + 2 * kPanelPaddingY;
  panel.box = Rect{area.right() - kGapMd - width, area.bottom() - kGapMd - height, width, height};
  return panel;
}

std::optional<Rect> ScreenOverlayView::glassRect() const {
  const auto panel = layoutPanel();
  if (!panel) return std::nullopt;
  return panel->box;
}

void ScreenOverlayView::paintPanel(rujian::ui::Canvas& canvas) const {
  const auto panel = layoutPanel();
  if (!panel) return;
  TextStyle style = textStyle(kDisplayFontSize, 400);
  style.wrap = true;
  canvas.drawBox(panel->box, rujian::ui::BoxStyle{.fill = kGlassFill, .borderWidth = kGlassBorderWidth, .borderColor = kGlassBorder});
  double y = panel->box.y + kPanelPaddingY;
  for (const PanelLine& line : panel->lines) {
    canvas.drawText(line.text, Rect{panel->box.x + kPanelPaddingX, y, line.size.width + 1, line.size.height}, style, VerticalAlign::Top);
    y += line.size.height + kGapXs;
  }
}

}  // namespace ember::win32::ui
