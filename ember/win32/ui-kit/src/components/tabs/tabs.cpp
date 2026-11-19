/*
 * ------------------------------------------------------------------------
 * 名称：tabs.cpp
 * 说明：页签的实现：页签栏控件（TabBar）自己画各页签的文字、悬停底色、选中页签的下划线与焦点框并处理鼠标与键盘，Tabs 负责内容的显示与摆放。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：页签高 32、左右内边距 10；选中页签的文字加粗并变宽；下划线高 2、顶部圆角 2；整条页签栏下方有一条分割线。
 *       悬停底色只有上面两个圆角（用“向下多出一个圆角高度的圆角裁剪区”实现），下划线同理。
 * ------------------------------------------------------------------------
 */

#include "rujian/ui/tabs.hpp"

#include <algorithm>

#include "base/theme-tokens.hpp"
#include "rujian/ui/geometry.hpp"

namespace rujian::ui {

namespace {

constexpr double kBorderWidth = 1.0;
constexpr double kFocusOffset = -2.0;
constexpr double kFocusWidth = 1.0;
constexpr int kVirtualKeyEnd = 0x23;
constexpr int kVirtualKeyHome = 0x24;
constexpr int kVirtualKeyLeft = 0x25;
constexpr int kVirtualKeyRight = 0x27;

}  // namespace

/** 页签栏：只管页签栏自己的绘制与输入，选中的结果交给 Tabs。 */
class TabBar final : public Widget {
 public:
  explicit TabBar(Tabs& owner) : owner_(owner) {}

  [[nodiscard]] bool focusable() const override { return true; }
  [[nodiscard]] CursorKind cursor() const override { return hoverIndex_ >= 0 ? CursorKind::Hand : CursorKind::Arrow; }

  [[nodiscard]] Size measure(Size /*available*/) override {
    double width = 0.0;
    for (std::size_t i = 0; i < owner_.tabs_.size(); ++i) width += tabWidth(i);
    return Size{width, tokens::tab_height + borderWidth()};
  }

  [[nodiscard]] bool stretchesHeight() const override { return false; }

  bool onMouseDown(const MouseEvent& event) override {
    if (event.button != MouseButton::Left) return false;
    pressedIndex_ = indexAt(event.position);
    return true;
  }

  bool onMouseUp(const MouseEvent& event) override {
    if (event.button != MouseButton::Left) return false;
    const int index = indexAt(event.position);
    const int pressed = pressedIndex_;
    pressedIndex_ = -1;
    if (index >= 0 && index == pressed) owner_.choose(static_cast<std::size_t>(index));
    return true;
  }

  bool onMouseMove(const MouseEvent& event) override {
    const int index = indexAt(event.position);
    if (index != hoverIndex_) {
      hoverIndex_ = index;
      invalidate();
    }
    return false;
  }

  void onStateChanged() override {
    Widget::onStateChanged();
    if (!hovered() && hoverIndex_ != -1) hoverIndex_ = -1;
  }

  bool onKeyDown(const KeyEvent& event) override {
    const std::size_t count = owner_.tabs_.size();
    if (count == 0) return false;
    std::size_t target = owner_.selected_;
    switch (event.virtualKey) {
      case kVirtualKeyLeft: target = (owner_.selected_ + count - 1) % count; break;
      case kVirtualKeyRight: target = (owner_.selected_ + 1) % count; break;
      case kVirtualKeyHome: target = 0; break;
      case kVirtualKeyEnd: target = count - 1; break;
      default: return false;
    }
    owner_.choose(target);
    return true;
  }

  void paint(Canvas& canvas) override {
    const auto& colors = themeTokens(host()->theme());
    const double border = borderWidth();
    // 页签栏下方的分割线
    canvas.fillRect(Rect{bounds().x, bounds().bottom() - border, bounds().width, border}, colors.divider);
    for (std::size_t i = 0; i < owner_.tabs_.size(); ++i) {
      const Rect box = tabRect(i);
      const bool selected = i == owner_.selected_;
      if (static_cast<int>(i) == hoverIndex_) {
        // 只有上面两个圆角：圆角裁剪区向下多出一个圆角的高度，填充只铺到页签底边
        canvas.pushClip(Rect{box.x, box.y, box.width, box.height + tokens::radius_md}, tokens::radius_md);
        canvas.fillRect(box, colors.option_hover_bg);
        canvas.popClip();
      }
      canvas.drawText(owner_.tabs_[i].label, box.inset(Insets{0, tokens::tab_padding_x, 0, tokens::tab_padding_x}), style(selected, selected ? colors.tab_selected_color : colors.tab_color), VerticalAlign::Center);
      if (selected) {
        // 下划线高 2、顶部圆角 2（圆角裁剪区向下多出一个圆角高度，填充只铺到页签底边）
        const Rect indicator{box.x, box.bottom() - tokens::tab_indicator_height, box.width, tokens::tab_indicator_height};
        canvas.pushClip(Rect{indicator.x, indicator.y, indicator.width, indicator.height + tokens::tab_indicator_height}, tokens::tab_indicator_height);
        canvas.fillRect(indicator, colors.accent);
        canvas.popClip();
        if (focusVisible()) canvas.strokeOutline(box, 0.0, kFocusOffset, kFocusWidth, colors.focus_border);
      }
    }
  }

 private:
  [[nodiscard]] double borderWidth() const { return snapBorderWidth(kBorderWidth, host() != nullptr ? host()->scale() : 1.0); }

  [[nodiscard]] static TextStyle style(bool selected, Color color) {
    TextStyle result;
    result.fontSize = tokens::tab_font_size;
    result.fontWeight = static_cast<int>(selected ? tokens::tab_selected_font_weight : 400.0);
    result.lineHeight = 0;
    result.color = color;
    return result;
  }

  /** 第 index 个页签的宽度：文字宽度（选中的加粗）加左右内边距。 */
  [[nodiscard]] double tabWidth(std::size_t index) const {
    const bool selected = index == owner_.selected_;
    return host()->measureText(owner_.tabs_[index].label, style(selected, kTransparent), 0).width + 2 * tokens::tab_padding_x;
  }

  [[nodiscard]] Rect tabRect(std::size_t index) const {
    double x = bounds().x;
    for (std::size_t i = 0; i < index; ++i) x += tabWidth(i);
    return Rect{x, bounds().y, tabWidth(index), tokens::tab_height};
  }

  [[nodiscard]] int indexAt(Point point) const {
    for (std::size_t i = 0; i < owner_.tabs_.size(); ++i) {
      if (tabRect(i).contains(point)) return static_cast<int>(i);
    }
    return -1;
  }

  Tabs& owner_;
  int hoverIndex_{-1};
  int pressedIndex_{-1};
};

Tabs::Tabs() { bar_ = static_cast<TabBar*>(addChild(std::make_unique<TabBar>(*this))); }

Widget* Tabs::addTab(std::string id, std::string label, std::unique_ptr<Widget> content) {
  Widget* added = content ? addChild(std::move(content)) : nullptr;
  tabs_.push_back(Tab{std::move(id), std::move(label), added});
  showSelectedContent();
  invalidate();
  return added;
}

void Tabs::select(const std::string& id) {
  for (std::size_t i = 0; i < tabs_.size(); ++i) {
    if (tabs_[i].id != id) continue;
    selected_ = i;
    showSelectedContent();
    if (host() != nullptr) arrange(bounds());
    invalidate();
    return;
  }
}

std::string Tabs::selectedId() const { return tabs_.empty() ? std::string() : tabs_[selected_].id; }

void Tabs::setOnSelect(SelectHandler handler) { onSelect_ = std::move(handler); }

Widget* Tabs::tabBar() const { return bar_; }

void Tabs::choose(std::size_t index) {
  if (index == selected_ || index >= tabs_.size()) return;
  selected_ = index;
  showSelectedContent();
  arrange(bounds());
  invalidate();
  if (onSelect_) onSelect_(tabs_[index].id);
}

void Tabs::showSelectedContent() {
  for (std::size_t i = 0; i < tabs_.size(); ++i) {
    if (tabs_[i].content != nullptr) tabs_[i].content->setVisible(i == selected_);
  }
}

Size Tabs::measure(Size available) {
  const Size bar = bar_->measure(available);
  double width = bar.width;
  double height = bar.height;
  if (!tabs_.empty() && tabs_[selected_].content != nullptr) {
    const Size content = tabs_[selected_].content->measure(Size{available.width, 0});
    width = std::max(width, content.width);
    height += tokens::space_page + content.height;
  }
  return Size{available.width > 0 ? available.width : width, height};
}

void Tabs::arrange(const Rect& bounds) {
  Widget::arrange(bounds);
  const Size bar = bar_->measure(Size{bounds.width, 0});
  bar_->arrange(Rect{bounds.x, bounds.y, bounds.width, bar.height});
  if (!tabs_.empty() && tabs_[selected_].content != nullptr) {
    const double top = bounds.y + bar.height + tokens::space_page;
    Widget* content = tabs_[selected_].content;
    content->arrange(Rect{bounds.x, top, bounds.width, content->measure(Size{bounds.width, 0}).height});
  }
}

}  // namespace rujian::ui
