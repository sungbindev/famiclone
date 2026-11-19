/*
 * ------------------------------------------------------------------------
 * 名称：pages.cpp
 * 说明：预览程序的示例页的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：页的内边距与行间距与参考页的样式一致（page.css 的 reference-page：内边距 16px）。
 * ------------------------------------------------------------------------
 */

#include "pages.hpp"

#include <stdexcept>

#include "base/theme-tokens.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/checkbox.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/radio-group.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/select.hpp"
#include "rujian/ui/slider.hpp"
#include "rujian/ui/text-input.hpp"

namespace gallery {

namespace {

namespace ui = rujian::ui;

constexpr double kPagePadding = 16.0;

/** 按钮页的页面大小：三行按钮加内边距，高度取 4 的倍数。 */
constexpr ui::Size kButtonPageSize{440, 120};

/** 链接按钮整个区域都是文字，对照像素时连同四周的焦点环（偏差不足 0.4 像素，记入已知差异）一起不参与。 */
constexpr int kLinkSample = 12;
constexpr double kLinkMargin = -4.0;

/** 往容器里加一个按钮并登记为样本。 */
ui::Button* addButton(Page& page, ui::Widget& parent, const char* label) {
  auto button = std::make_unique<ui::Button>(label);
  ui::Button* raw = button.get();
  parent.addChild(std::move(button));
  page.samples.push_back(raw);
  return raw;
}

/** 按钮页：五种样式、自动图标、纯图标的无底色按钮、切换按钮、紧凑、图标在后、链接。 */
Page buildButtonPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  ui::Stack& root = *stack;

  auto firstRow = std::make_unique<ui::Row>();
  ui::Row& rowA = *firstRow;
  root.addChild(std::move(firstRow));
  addButton(page, rowA, "取消");
  addButton(page, rowA, "确定")->setVariant(ui::ButtonVariant::Primary);
  addButton(page, rowA, "保存");
  addButton(page, rowA, "删除")->setVariant(ui::ButtonVariant::Danger);
  ui::Button* disabled = addButton(page, rowA, "禁用");
  disabled->setAutoIcon(false);
  disabled->setEnabled(false);

  auto secondRow = std::make_unique<ui::Row>();
  ui::Row& rowB = *secondRow;
  root.addChild(std::move(secondRow));
  const auto ghostIcon = [&](const char* label, ui::IconId icon) {
    ui::Button* button = addButton(page, rowB, label);
    button->setVariant(ui::ButtonVariant::Ghost);
    button->setIconOnly(true);
    button->setCompact(true);
    button->setIcon(icon);
    return button;
  };
  ghostIcon("播放", ui::IconId::PlayerPlay);
  ghostIcon("停止", ui::IconId::PlayerStop)->setEnabled(false);
  ghostIcon("静音", ui::IconId::Volume)->setToggled(true);
  ui::Button* toggled = addButton(page, rowB, "静音");
  toggled->setAutoIcon(false);
  toggled->setToggled(true);

  auto thirdRow = std::make_unique<ui::Row>();
  ui::Row& rowC = *thirdRow;
  root.addChild(std::move(thirdRow));
  ui::Button* uninstall = addButton(page, rowC, "卸载");
  uninstall->setVariant(ui::ButtonVariant::Danger);
  uninstall->setCompact(true);
  uninstall->setIcon(ui::IconId::Trash);
  addButton(page, rowC, "下一步");
  ui::Button* more = addButton(page, rowC, "更多");
  more->setVariant(ui::ButtonVariant::Ghost);
  more->setAutoIcon(false);
  addButton(page, rowC, "选择目录")->setVariant(ui::ButtonVariant::Link);

  page.content = std::move(stack);
  return page;
}

/** 输入框页的页面大小：六个输入框纵向排列，高度取 4 的倍数。 */
constexpr ui::Size kInputPageSize{320, 296};

/** 往容器里加一个输入框并登记为样本。 */
ui::TextInput* addInput(Page& page, ui::Widget& parent) {
  auto input = std::make_unique<ui::TextInput>();
  ui::TextInput* raw = input.get();
  parent.addChild(std::move(input));
  page.samples.push_back(raw);
  return raw;
}

/** 输入框页：占位文字、有值、校验失败、禁用、搜索类型、超长的值。 */
Page buildInputPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  addInput(page, *stack)->setPlaceholder("请输入");
  addInput(page, *stack)->setValue("已填写的文字 Hello");
  ui::TextInput* invalid = addInput(page, *stack);
  invalid->setValue("不合法的值");
  invalid->setInvalid(true);
  ui::TextInput* disabled = addInput(page, *stack);
  disabled->setValue("禁用的输入框");
  disabled->setEnabled(false);
  ui::TextInput* disabledEmpty = addInput(page, *stack);
  disabledEmpty->setPlaceholder("禁用的占位文字");
  disabledEmpty->setEnabled(false);
  addInput(page, *stack)->setValue("这是一段很长很长很长很长很长很长很长很长很长很长很长的文字，超出输入框宽度会被裁掉");
  ui::TextInput* search = addInput(page, *stack);
  search->setSearch(true);
  search->setValue("搜索词");
  ui::TextInput* searchEmpty = addInput(page, *stack);
  searchEmpty->setSearch(true);
  searchEmpty->setPlaceholder("搜索");
  page.content = std::move(stack);
  return page;
}

/** 选择控件页的页面大小：四个复选框、四个分段单选组纵向排列，高度取 4 的倍数。 */
constexpr ui::Size kChoicePageSize{320, 336};

/** 复选框里方框加间距占的宽度：对照像素时这段以右是文字，不参与。 */
constexpr double kCheckboxMarkWidth = 20.0;
/** 单选组里文字所在的水平带：上下各空出 8 像素参与像素对照（底色、分隔线、强调线、边框），中间的文字不参与。 */
constexpr double kGroupTextBand = 8.0;
constexpr ui::Insets kGroupInset{kGroupTextBand, 0, kGroupTextBand, 0};
/** 大于样本尺寸的边距：样本内部没有任何区域被排除，全部参与像素对照（没有文字的控件用）。 */
constexpr double kCompareAll = 1000.0;

/** 往容器里加一个复选框并登记为样本。 */
ui::Checkbox* addCheckbox(Page& page, ui::Widget& parent, const char* label) {
  auto checkbox = std::make_unique<ui::Checkbox>(label);
  ui::Checkbox* raw = checkbox.get();
  parent.addChild(std::move(checkbox));
  page.samples.push_back(raw);
  return raw;
}

/** 往容器里加一个三选项的分段单选组并登记为样本：样本中心正好落在中间那一项上。 */
ui::RadioGroup* addGroup(Page& page, ui::Widget& parent, const char* second, bool secondDisabled, const char* value) {
  auto group = std::make_unique<ui::RadioGroup>();
  ui::RadioGroup* raw = group.get();
  raw->setOptions({{"all", "全部", false}, {"second", second, secondDisabled}, {"third", "收藏", false}});
  raw->setValue(value);
  parent.addChild(std::move(group));
  page.samples.push_back(raw);
  return raw;
}

/** 选择控件页：复选框的未勾选、已勾选、禁用；分段单选组选中中间项、选中首项、中间项禁用、整组禁用。 */
Page buildChoicePage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  addCheckbox(page, *stack, "启用自动保存");
  addCheckbox(page, *stack, "已勾选的选项")->setChecked(true);
  addCheckbox(page, *stack, "禁用的选项")->setEnabled(false);
  ui::Checkbox* disabledChecked = addCheckbox(page, *stack, "禁用且已勾选");
  disabledChecked->setChecked(true);
  disabledChecked->setEnabled(false);
  addGroup(page, *stack, "最近", false, "second");
  addGroup(page, *stack, "最近", false, "all");
  addGroup(page, *stack, "已停用", true, "all");
  addGroup(page, *stack, "最近", false, "second")->setEnabled(false);
  page.content = std::move(stack);
  return page;
}

/** 下拉框页的页面大小：一行三个下拉框，下面留出弹层的位置，高度取 4 的倍数。 */
constexpr ui::Size kSelectPageSize{320, 160};

/** 下拉框里文字所在的区域：左边的内边距、右边的箭头列、上下各 4 像素之内不参与像素对照。 */
constexpr ui::Insets kSelectInset{4, 24, 4, 8};
/** 弹层里每个选项的文字从左边缘内 8 像素起（有已选项时还要加对勾列），这段之后不参与。 */
constexpr ui::Insets kOptionInset{0, 0, 0, 8};

/** 往容器里加一个四选项的下拉框并登记为样本。 */
ui::Select* addSelect(Page& page, ui::Widget& parent, const char* value) {
  auto select = std::make_unique<ui::Select>();
  ui::Select* raw = select.get();
  raw->setOptions({{"a", "选项一"}, {"b", "选项二"}, {"c", "选项三"}, {"d", "选项四"}});
  raw->setAllowEmpty(false);
  raw->setValue(value);
  parent.addChild(std::move(select));
  page.samples.push_back(raw);
  return raw;
}

/** 下拉框页：已选、未选（显示占位文字）、禁用。 */
Page buildSelectPage() {
  Page page;
  auto row = std::make_unique<ui::Row>();
  row->setPadding(ui::Insets::all(kPagePadding));
  addSelect(page, *row, "b");
  addSelect(page, *row, "");
  addSelect(page, *row, "c")->setEnabled(false);
  page.content = std::move(row);
  return page;
}

/** 滑块页的页面大小：一行四个滑块，高度取 4 的倍数。 */
constexpr ui::Size kSliderPageSize{448, 56};

/** 往容器里加一个滑块并登记为样本。 */
ui::Slider* addSlider(Page& page, ui::Widget& parent, double value) {
  auto slider = std::make_unique<ui::Slider>();
  ui::Slider* raw = slider.get();
  raw->setValue(value);
  parent.addChild(std::move(slider));
  page.samples.push_back(raw);
  return raw;
}

/** 滑块页：最左、靠左、最右、禁用。 */
Page buildSliderPage() {
  Page page;
  auto row = std::make_unique<ui::Row>();
  row->setPadding(ui::Insets::all(kPagePadding));
  addSlider(page, *row, 0);
  addSlider(page, *row, 35);
  addSlider(page, *row, 100);
  addSlider(page, *row, 60)->setEnabled(false);
  page.content = std::move(row);
  return page;
}

/** 滚动页的页面大小：一个 200×120 的滚动区域加内边距，高度取 4 的倍数。 */
constexpr ui::Size kScrollPageSize{232, 152};

/** 一摞交替颜色的色带，没有文字，用来看滚动位置：十二条，每条 24 像素高。 */
class Bands final : public ui::Widget {
 public:
  static constexpr double kBandHeight = 24.0;
  static constexpr int kBandCount = 12;

  ui::Size measure(ui::Size available) override { return ui::Size{available.width, kBandHeight * kBandCount}; }

  void paint(ui::Canvas& canvas) override {
    const auto& colors = ui::themeTokens(host()->theme());
    for (int i = 0; i < kBandCount; ++i) {
      canvas.fillRect(ui::Rect{bounds().x, bounds().y + i * kBandHeight, bounds().width, kBandHeight}, i % 2 == 0 ? colors.accent : colors.input_bg);
    }
  }
};

/** 滚动页：高度受限的滚动区域里放比它高的内容。 */
Page buildScrollPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  auto area = std::make_unique<ui::ScrollArea>();
  ui::ScrollArea* raw = area.get();
  raw->setMaxHeight(120);
  raw->setContent(std::make_unique<Bands>());
  stack->addChild(std::move(area));
  page.samples.push_back(raw);
  page.content = std::move(stack);
  return page;
}

}  // namespace

const std::vector<PageInfo>& pageInfos() {
  static const std::vector<PageInfo> infos = {
      {"button", kButtonPageSize, ui::Insets::all(3), {{kLinkSample, ui::Insets::all(kLinkMargin)}}, {}, {0, 1, 3, 5, 7, kLinkSample}, {"hover", "pressed", "focus"}},
      {"input", kInputPageSize, ui::Insets::symmetric(4, 3), {}, {}, {0, 1, 2, 3, 6, 7}, {"hover", "pressed", "focus"}},
      {"choice", kChoicePageSize, ui::Insets{0, 0, 0, kCheckboxMarkWidth}, {{4, kGroupInset}, {5, kGroupInset}, {6, kGroupInset}, {7, kGroupInset}}, {}, {0, 1, 2, 3, 4, 5, 6, 7}, {"hover", "pressed", "focus"}},
      {"select", kSelectPageSize, kSelectInset, {}, kOptionInset, {0, 1, 2}, {"hover", "pressed", "focus", "open", "openkey", "openhover"}},
      {"slider", kSliderPageSize, ui::Insets::all(kCompareAll), {}, {}, {0, 1, 2, 3}, {"hover", "pressed", "focus"}},
      {"scroll", kScrollPageSize, ui::Insets::all(kCompareAll), {}, {}, {0}, {"hover", "wheel", "drag"}},
  };
  return infos;
}

Page buildPage(const std::string& name) {
  if (name == "button") return buildButtonPage();
  if (name == "input") return buildInputPage();
  if (name == "choice") return buildChoicePage();
  if (name == "select") return buildSelectPage();
  if (name == "slider") return buildSliderPage();
  if (name == "scroll") return buildScrollPage();
  throw std::runtime_error("未知的示例页：" + name);
}

}  // namespace gallery
