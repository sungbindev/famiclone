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

#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/callout.hpp"
#include "rujian/ui/checkbox.hpp"
#include "rujian/ui/facts.hpp"
#include "rujian/ui/field.hpp"
#include "rujian/ui/app-frame.hpp"
#include "rujian/ui/dialog-frame.hpp"
#include "rujian/ui/label.hpp"
#include "rujian/ui/layout.hpp"
#include "rujian/ui/list.hpp"
#include "rujian/ui/menu.hpp"
#include "rujian/ui/panel.hpp"
#include "rujian/ui/radio-group.hpp"
#include "rujian/ui/row-list.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/select.hpp"
#include "rujian/ui/setting-row.hpp"
#include "rujian/ui/slider.hpp"
#include "rujian/ui/split-view.hpp"
#include "rujian/ui/state.hpp"
#include "rujian/ui/status-bar.hpp"
#include "rujian/ui/status-label.hpp"
#include "rujian/ui/table.hpp"
#include "rujian/ui/tabs.hpp"
#include "rujian/ui/title-bar.hpp"
#include "rujian/ui/text-input.hpp"
#include "rujian/ui/toolbar.hpp"
#include "rujian/ui/tree.hpp"

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

/** 菜单页的页面大小：一行三个菜单栏，弹层向下展开，高度取 4 的倍数。 */
constexpr ui::Size kMenuPageSize{480, 224};

/** 菜单栏里菜单名按钮的文字在左右各 8 像素的内边距之内、上下各 4 像素之内；这之外的底色与圆角才参与像素对照。 */
constexpr ui::Insets kMenuInset{4, 7, 4, 7};
/** 弹层与弹层里每一项：文字、勾选标记、快捷键都在四周 4 像素之内，只对照外圈。 */
constexpr ui::Insets kMenuPopupInset{4, 4, 4, 4};

ui::MenuItem makeMenuItem(const char* id, const char* label) {
  ui::MenuItem item;
  item.id = id;
  item.label = label;
  return item;
}

/** 往容器里加一个只有一个菜单的菜单栏并登记为样本。 */
void addMenuBar(Page& page, ui::Widget& parent, ui::MenuBarMenu menu) {
  auto bar = std::make_unique<ui::MenuBar>();
  ui::MenuBar* raw = bar.get();
  raw->setMenus({std::move(menu)});
  parent.addChild(std::move(bar));
  page.samples.push_back(raw);
}

/** 菜单页：普通项带图标与快捷键、禁用项、分隔线、助记符；勾选项与单选项、子菜单、提示文字。 */
Page buildMenuPage() {
  Page page;
  auto row = std::make_unique<ui::Row>();
  row->setPadding(ui::Insets::all(kPagePadding));

  ui::MenuItem create = makeMenuItem("new", "新建");
  create.icon = ui::IconId::Plus;
  create.shortcut = "Ctrl+N";
  ui::MenuItem open = makeMenuItem("open", "打开");
  open.icon = ui::IconId::FolderOpen;
  open.shortcut = "Ctrl+O";
  open.disabled = true;
  ui::MenuItem save = makeMenuItem("save", "保存");
  save.icon = ui::IconId::DeviceFloppy;
  save.shortcut = "Ctrl+S";
  ui::MenuItem exit = makeMenuItem("exit", "退出");
  exit.icon = ui::IconId::Power;
  exit.mnemonic = "x";
  exit.separatorBefore = true;
  addMenuBar(page, *row, ui::MenuBarMenu{.id = "file", .label = "文件", .mnemonic = "F", .items = {create, open, save, exit}});

  ui::MenuItem full = makeMenuItem("full", "全屏");
  full.checked = true;
  full.shortcut = "F11";
  ui::MenuItem dark = makeMenuItem("dark", "深色");
  dark.checked = true;
  dark.radio = true;
  ui::MenuItem light = makeMenuItem("light", "浅色");
  light.checked = false;
  light.radio = true;
  ui::MenuItem zoom = makeMenuItem("zoom", "缩放");
  zoom.separatorBefore = true;
  zoom.items = {makeMenuItem("zoom-in", "放大"), makeMenuItem("zoom-out", "缩小")};
  ui::MenuItem replay = makeMenuItem("replay", "回放");
  replay.hint = "暂未支持";
  replay.disabled = true;
  addMenuBar(page, *row, ui::MenuBarMenu{.id = "view", .label = "视图", .mnemonic = "V", .items = {full, dark, light, zoom, replay}});

  ui::MenuItem update = makeMenuItem("update", "检查更新");
  update.hint = "暂未支持";
  ui::MenuItem about = makeMenuItem("about", "关于");
  about.separatorBefore = true;
  addMenuBar(page, *row, ui::MenuBarMenu{.id = "help", .label = "帮助", .mnemonic = "H", .items = {makeMenuItem("manual", "手册"), update, about}});
  page.content = std::move(row);
  return page;
}

/** 标题栏页的页面大小：标题栏高 36，宽 480。 */
constexpr ui::Size kTitleBarPageSize{480, 36};

/** 标题栏（样本 0）里不参与像素对照的内部：图标与文字都在左右 6 像素、上下 4 像素之内，只对照底色与底部分割线。 */
constexpr ui::Insets kTitleBarInset{4, 4, 4, 6};
/** 折叠按钮里的图标（居中的 14 像素）在左右 14 像素、上下 8 像素之内。 */
constexpr ui::Insets kTitleBarToolInset{8, 14, 8, 14};

/** 应用图标的占位：主题色的圆角方块。 */
class LogoSquare final : public ui::Widget {
 public:
  void paint(ui::Canvas& canvas) override {
    canvas.drawBox(bounds(), ui::BoxStyle{.fill = ui::themeTokens(host()->theme()).accent, .radius = 4.0});
  }
};

/** 标题栏页：图标、应用名与副名、菜单栏、操作区的按钮、三个折叠按钮（展开、收起、禁用），不显示系统按钮。 */
Page buildTitleBarPage() {
  Page page;
  auto bar = std::make_unique<ui::TitleBar>();
  ui::TitleBar* raw = bar.get();
  raw->setLogo(std::make_unique<LogoSquare>());
  raw->setTitle("Ember");
  raw->setSubtitle("模拟器");
  auto menus = std::make_unique<ui::MenuBar>();
  ui::MenuBar* menuRaw = menus.get();
  menuRaw->setMenus({ui::MenuBarMenu{.id = "file", .label = "文件", .mnemonic = "F", .items = {makeMenuItem("open", "打开"), makeMenuItem("exit", "退出"), makeMenuItem("about", "关于")}}});
  raw->setMenuBar(std::move(menus));
  ui::Widget* update = raw->addAction(std::make_unique<ui::Button>("更新"));
  raw->setTools({ui::TitleBarTool{.id = "side", .label = "边栏", .icon = ui::IconId::Settings, .expanded = true},
                 ui::TitleBarTool{.id = "bottom", .label = "底部面板", .icon = ui::IconId::Eye},
                 ui::TitleBarTool{.id = "right", .label = "右边栏", .icon = ui::IconId::Bell, .disabled = true}});
  page.samples = {raw, menuRaw, update, raw->toolButton(0), raw->toolButton(1), raw->toolButton(2)};
  page.content = std::move(bar);
  return page;
}

/** 状态栏页的页面大小：普通 26 高，大尺寸 32 高。 */
constexpr ui::Size kStatusBarPageSize{480, 26};
constexpr ui::Size kStatusBarLargePageSize{480, 32};
/** 状态栏与状态项里不参与像素对照的内部：文字与图标都在 3 像素之内。 */
constexpr ui::Insets kStatusBarInset = ui::Insets::all(3);

/** 往状态栏加一个状态项并登记为样本。 */
ui::StatusBarItem* addStatusItem(Page& page, ui::StatusBar& bar, bool atEnd, const char* text) {
  auto item = std::make_unique<ui::StatusBarItem>(text);
  ui::StatusBarItem* raw = atEnd ? bar.addEnd(std::move(item)) : bar.addStart(std::move(item));
  page.samples.push_back(raw);
  return raw;
}

/**
 * 状态栏页：左侧依次是状态点、警告等级、成功等级的按钮、放不下时收缩的长文字；
 * 右侧依次是文字后的指示灯、画在图标里的指示灯、选中的纯图标按钮、未选中的纯图标按钮。
 */
Page buildStatusBarPage(bool large) {
  Page page;
  auto bar = std::make_unique<ui::StatusBar>();
  ui::StatusBar* raw = bar.get();
  raw->setLarge(large);
  page.samples.push_back(raw);
  addStatusItem(page, *raw, false, "运行中")->setDot(ui::StatusDot::Success);
  addStatusItem(page, *raw, false, "已暂停")->setTone(ui::StatusItemTone::Warning);
  ui::StatusBarItem* debug = addStatusItem(page, *raw, false, "调试模式");
  debug->setTone(ui::StatusItemTone::Success);
  debug->setOnClick([] {});
  addStatusItem(page, *raw, false, "这是一个很长很长很长很长很长很长的文件名称.nes");
  addStatusItem(page, *raw, true, "软驱")->setLamp(ui::StatusLamp{.lit = true, .label = "灯"});
  ui::StatusBarItem* disk = addStatusItem(page, *raw, true, "磁盘");
  disk->setIcon(ui::IconId::DeviceFloppy);
  disk->setLamp(ui::StatusLamp{.lit = true, .label = "读写灯", .inIcon = ui::Point{50, 60}});
  ui::StatusBarItem* selected = addStatusItem(page, *raw, true, "通知");
  selected->setIcon(ui::IconId::Bell);
  selected->setIconOnly(true);
  selected->setOnClick([] {});
  selected->setPressed(true);
  ui::StatusBarItem* plain = addStatusItem(page, *raw, true, "提示");
  plain->setIcon(ui::IconId::InfoCircle);
  plain->setIconOnly(true);
  plain->setOnClick([] {});
  page.content = std::move(bar);
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

/** 面板页的页面大小：五个面板纵向排列，高度取 4 的倍数。 */
constexpr ui::Size kPanelPageSize{320, 492};

/** 内容块：高 24、底色取强调色，没有文字。 */
class Block final : public ui::Widget {
 public:
  static constexpr double kBlockHeight = 24.0;

  ui::Size measure(ui::Size available) override { return ui::Size{available.width, kBlockHeight}; }

  void paint(ui::Canvas& canvas) override { canvas.fillRect(bounds(), ui::themeTokens(host()->theme()).accent); }
};

/** 面板的宽度：页宽减去两侧内边距。 */
constexpr double kPanelWidth = 288.0;
/** 标题文字所在的区域：对照像素时它不参与（水平从内边距之外起，垂直带上下各留 2 像素余量）。 */
constexpr double kTitleLeft = 8.0;

/** 标题区域的不参与边距：宽度为 titleRight 之前，垂直范围 [top, bottom]，面板总高 height。 */
ui::Insets titleInset(double top, double bottom, double titleRight, double height) { return ui::Insets{top, kPanelWidth - titleRight, height - bottom, kTitleLeft}; }

/** 往面板里加一个内容块。 */
void addBlock(ui::Panel& panel) { panel.addContent(std::make_unique<Block>()); }

/** 往容器里加一个面板并登记为样本。 */
ui::Panel* addPanel(Page& page, ui::Widget& parent, const char* title) {
  auto panel = std::make_unique<ui::Panel>();
  ui::Panel* raw = panel.get();
  raw->setTitle(title);
  parent.addChild(std::move(panel));
  page.samples.push_back(raw);
  return raw;
}

/** 面板页：标题加 aside 加两块内容、紧凑标题栏、平面、只有 aside、只有标题（内容区为空）。 */
Page buildPanelPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  ui::Panel* full = addPanel(page, *stack, "面板");
  auto progress = std::make_unique<ui::Slider>();
  progress->setValue(35);
  full->setAside(std::move(progress));
  addBlock(*full);
  addBlock(*full);
  ui::Panel* compact = addPanel(page, *stack, "紧凑标题");
  compact->setCompactHead(true);
  addBlock(*compact);
  ui::Panel* plain = addPanel(page, *stack, "平面");
  plain->setPlain(true);
  addBlock(*plain);
  ui::Panel* asideOnly = addPanel(page, *stack, "");
  auto asideSlider = std::make_unique<ui::Slider>();
  asideSlider->setValue(60);
  asideOnly->setAside(std::move(asideSlider));
  addBlock(*asideOnly);
  addPanel(page, *stack, "仅标题");
  page.content = std::move(stack);
  return page;
}

/** 键值清单页的页面大小：宽窗口（大于 420）两列，窄窗口上下排列；高度取 4 的倍数。 */
constexpr ui::Size kFactsPageSize{440, 108};
constexpr ui::Size kFactsNarrowPageSize{320, 160};

/** 键值清单页：三条，最后一条的内容有两行；全部是文字，对照文字外框与样本位置。 */
Page buildFactsPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  auto facts = std::make_unique<ui::Facts>();
  facts->addFact("构建时间", "2026-11-19 10:00");
  facts->addFact("作者", "某某");
  facts->addFact("授权", "第一行\n第二行");
  page.samples.push_back(facts.get());
  stack->addChild(std::move(facts));
  page.content = std::move(stack);
  return page;
}

/** 字段页的页面大小：两个字段（有标签与没有标签）纵向排列，高度取 4 的倍数。 */
constexpr ui::Size kFieldPageSize{320, 160};

/** 往容器里加一个字段并登记为样本；控件是没有文字的滑块。 */
ui::Field* addField(Page& page, ui::Widget& parent, const char* label, const char* description) {
  auto field = std::make_unique<ui::Field>();
  ui::Field* raw = field.get();
  raw->setLabel(label);
  raw->setDescription(description);
  auto slider = std::make_unique<ui::Slider>();
  slider->setValue(35);
  raw->setControl(std::move(slider));
  parent.addChild(std::move(field));
  page.samples.push_back(raw);
  return raw;
}

/** 字段页：标签加说明加控件、没有标签时控件加说明。 */
Page buildFieldPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  addField(page, *stack, "名称", "说明文字");
  addField(page, *stack, "", "说明文字");
  page.content = std::move(stack);
  return page;
}

/** 设置行页的页面大小：宽窗口（大于 720）左右两列，窄窗口上下排列；高度取 4 的倍数。 */
constexpr ui::Size kSettingRowPageSize{760, 200};
constexpr ui::Size kSettingRowNarrowPageSize{600, 276};

/** 往容器里加一个设置行并登记为样本；控件是没有文字的滑块。 */
ui::SettingRow* addSettingRow(Page& page, ui::Widget& parent, const char* description, const char* error) {
  auto row = std::make_unique<ui::SettingRow>();
  ui::SettingRow* raw = row.get();
  raw->setLabel("名称");
  raw->setDescription(description);
  raw->setError(error);
  auto slider = std::make_unique<ui::Slider>();
  slider->setValue(60);
  raw->setControl(std::move(slider));
  parent.addChild(std::move(row));
  page.samples.push_back(raw);
  return raw;
}

/** 设置行页：名称加说明、只有名称、控件下方带错误文字。 */
Page buildSettingRowPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  addSettingRow(page, *stack, "说明文字", "");
  addSettingRow(page, *stack, "", "");
  addSettingRow(page, *stack, "", "错误文字");
  page.content = std::move(stack);
  return page;
}

/** 反馈控件页的页面大小：两个提示块、两个说明块、一行状态标签，高度取 4 的倍数。 */
constexpr ui::Size kFeedbackPageSize{360, 240};

/** 提示块样本的内边距：边框 1 加内边距 6（上下）之内、边框 1 加内边距 10（左右，少留 1 像素给链接按钮下划线的半像素边缘）之内是文字与链接按钮，不参与像素对照。 */
constexpr ui::Insets kCalloutInset{7, 10, 7, 10};

/** 反馈控件页：提示块（带链接按钮、错误样式）、说明块（普通、出错）、状态标签（成功、已停止）。 */
Page buildFeedbackPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));

  auto info = std::make_unique<ui::Callout>();
  info->setText("这是一段提示说明文字");
  auto link = std::make_unique<ui::Button>("选择目录");
  link->setVariant(ui::ButtonVariant::Link);
  info->addContent(std::move(link));
  page.samples.push_back(info.get());
  stack->addChild(std::move(info));

  auto error = std::make_unique<ui::Callout>();
  error->setText("这是一段错误提示文字");
  error->setError(true);
  page.samples.push_back(error.get());
  stack->addChild(std::move(error));

  auto empty = std::make_unique<ui::State>();
  empty->setText("没有符合条件的插件");
  page.samples.push_back(empty.get());
  stack->addChild(std::move(empty));

  auto failed = std::make_unique<ui::State>();
  failed->setText("读取数据失败");
  failed->setError(true);
  page.samples.push_back(failed.get());
  stack->addChild(std::move(failed));

  auto labels = std::make_unique<ui::Row>();
  for (const bool enabled : {true, false}) {
    auto label = std::make_unique<ui::StatusLabel>();
    label->setTone(enabled ? ui::StatusTone::Success : ui::StatusTone::Stopped);
    label->setText(enabled ? "已启用" : "已停用");
    page.samples.push_back(label.get());
    labels->addChild(std::move(label));
  }
  stack->addChild(std::move(labels));
  page.content = std::move(stack);
  return page;
}

/** 分栏页的页面大小：高 160 的分栏视图加内边距，高度取 4 的倍数。 */
constexpr ui::Size kSplitPageSize{440, 192};

/** 高度固定的外框：分栏视图的高度取摩登所在区域，这里用它给出固定的 160。 */
class FixedHeight final : public ui::Widget {
 public:
  static constexpr double kFrameHeight = 160.0;

  ui::Size measure(ui::Size available) override { return ui::Size{available.width, kFrameHeight}; }
};

/** 铺满所在区域的底色块，没有文字。 */
class Solid final : public ui::Widget {
 public:
  ui::Size measure(ui::Size available) override { return ui::Size{available.width, 0}; }

  void paint(ui::Canvas& canvas) override { canvas.fillRect(bounds(), ui::themeTokens(host()->theme()).input_bg); }
};

/** 分栏页：左栏是会滚动的色带，主区是铺满的底色块；样本是分栏视图与它的分隔条。 */
Page buildSplitPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  auto frame = std::make_unique<FixedHeight>();
  auto view = std::make_unique<ui::SplitView>();
  view->setSideWidthRange(100, 200);
  view->setSideWidth(140);
  view->setSide(std::make_unique<Bands>());
  view->setMain(std::make_unique<Solid>());
  page.samples.push_back(view.get());
  page.samples.push_back(view->splitter());
  frame->addChild(std::move(view));
  stack->addChild(std::move(frame));
  page.content = std::move(stack);
  return page;
}

/** 页签页的页面大小：一行里的一条页签栏，下面是带内容的一组页签，高度取 4 的倍数。 */
constexpr ui::Size kTabsPageSize{360, 160};

/** 页签栏样本里除了顶部 6 像素（悬停底色）与底部 1 像素（分割线）之外都不参与像素对照：中间有文字，下划线两端在各缩放比例下两种光栅器的圆角抗锯齿不同（只对照它的外框）。 */
constexpr ui::Insets kTabBarInset{6, 0, 1, 0};

/** 页签页：一条只有页签的页签栏（在一行里，宽度是内容宽），以及纵向堆叠里带内容的页签（栏铺满宽度，下面留 10 像素再接内容）。 */
Page buildTabsPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));

  auto row = std::make_unique<ui::Row>();
  auto plain = std::make_unique<ui::Tabs>();
  plain->addTab("a", "第一页");
  plain->addTab("b", "第二页");
  plain->addTab("c", "第三页");
  page.samples.push_back(plain->tabBar());
  row->addChild(std::move(plain));
  stack->addChild(std::move(row));

  auto withContent = std::make_unique<ui::Tabs>();
  page.samples.push_back(withContent->tabBar());
  withContent->addTab("a", "第一页", std::make_unique<Block>());
  withContent->addTab("b", "第二页");
  page.samples.push_back(withContent->children()[1].get());
  stack->addChild(std::move(withContent));
  page.content = std::move(stack);
  return page;
}
/** 列表页的页面大小：一个面板，里面是三个设置行的分隔线列表，高度取 4 的倍数。 */
constexpr ui::Size kListPageSize{760, 160};
/** 列表样本：设置行左边 420 像素是名称，面板内边距 10 加边框 1 之内的列表宽 706。 */
constexpr double kListWidth = 706.0;

/** 列表页：面板内容区里的分隔线列表（第一行与最后一行贴着面板边、去掉内边距）。 */
Page buildListPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  auto panel = std::make_unique<ui::Panel>();
  page.samples.push_back(panel.get());
  auto list = std::make_unique<ui::List>();
  for (int i = 0; i < 3; ++i) {
    auto row = std::make_unique<ui::SettingRow>();
    row->setLabel("名称");
    auto slider = std::make_unique<ui::Slider>();
    slider->setValue(20 + 30 * i);
    row->setControl(std::move(slider));
    list->addRow(std::move(row));
  }
  page.samples.push_back(list.get());
  panel->addContent(std::move(list));
  stack->addChild(std::move(panel));
  page.content = std::move(stack);
  return page;
}

/** 无底色、紧凑、纯图标的按钮（行尾操作、工具栏操作用）。 */
std::unique_ptr<ui::Button> makeGhostIcon(const char* label, ui::IconId icon) {
  auto button = std::make_unique<ui::Button>(label);
  button->setVariant(ui::ButtonVariant::Ghost);
  button->setIconOnly(true);
  button->setCompact(true);
  button->setIcon(icon);
  return button;
}

/** 一组操作按钮。 */
std::unique_ptr<ui::Actions> makeActions(std::initializer_list<std::pair<const char*, ui::IconId>> buttons) {
  auto actions = std::make_unique<ui::Actions>();
  for (const auto& [label, icon] : buttons) actions->addChild(makeGhostIcon(label, icon));
  return actions;
}

/** 信息行页的页面大小：三行信息行加内边距，高度取 4 的倍数。 */
constexpr ui::Size kRowListPageSize{480, 112};
/** 信息行样本右边 92 像素（行尾操作加右内边距）参与像素对照，左边的文字不参与。 */
constexpr ui::Insets kRowListInset{0, 92, 0, 0};

/** 信息行页：带前导、信息列与三个操作的行，带两列（定宽与自然宽）与两个操作的行，仅前导与标题的行。 */
Page buildRowListPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  auto list = std::make_unique<ui::RowList>();

  auto floppy = std::make_unique<ui::RowListItem>();
  floppy->setLeading("软驱 A");
  floppy->setTitle("无软盘");
  floppy->addCell({"未就绪", 0, true});
  floppy->setActions(makeActions({{"写保护", ui::IconId::Lock}, {"插入", ui::IconId::FolderOpen}, {"弹出", ui::IconId::PlayerEject}}));
  page.samples.push_back(floppy.get());
  list->addChild(std::move(floppy));

  auto tape = std::make_unique<ui::RowListItem>();
  tape->setLeading("磁带");
  tape->setTitle("无磁带");
  tape->addCell({"00:00", 64, true});
  tape->addCell({"已停止", 0, true});
  tape->setActions(makeActions({{"播放", ui::IconId::PlayerPlay}, {"停止", ui::IconId::PlayerStop}}));
  page.samples.push_back(tape.get());
  list->addChild(std::move(tape));

  auto printer = std::make_unique<ui::RowListItem>();
  printer->setLeading("打印机");
  printer->setTitle("空闲");
  page.samples.push_back(printer.get());
  list->addChild(std::move(printer));

  stack->addChild(std::move(list));
  page.content = std::move(stack);
  return page;
}

/** 工具栏页的页面大小：宽页（操作在右边）与窄页（操作换到下一行）。 */
constexpr ui::Size kToolbarPageSize{560, 64};
constexpr ui::Size kToolbarNarrowPageSize{200, 92};

/** 工具栏页：前导是滑块，说明文字跟在后面，右边三个操作；宽、窄两个页面共用。 */
Page buildToolbarPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  auto toolbar = std::make_unique<ui::Toolbar>();
  auto slider = std::make_unique<ui::Slider>();
  slider->setValue(40);
  toolbar->setLeading(std::move(slider));
  toolbar->setDescription("共 3 项");
  toolbar->setActions(makeActions({{"播放", ui::IconId::PlayerPlay}, {"停止", ui::IconId::PlayerStop}, {"弹出", ui::IconId::PlayerEject}}));
  page.samples.push_back(toolbar.get());
  stack->addChild(std::move(toolbar));
  page.content = std::move(stack);
  return page;
}

/** 树页的页面大小：五行的树加内边距，高度取 4 的倍数。 */
constexpr ui::Size kTreePageSize{232, 144};
/** 树样本里名称所在的区域：左边 22 像素（内边距、展开箭头位置与间距）、右边 4 像素参与像素对照，中间不参与。 */
constexpr ui::Insets kTreeInset{0, 4, 0, 22};

/** 树页：五个扁平条目，选中第二项。 */
Page buildTreePage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  auto tree = std::make_unique<ui::Tree>();
  tree->setItems({{"general", "通用"}, {"appearance", "外观"}, {"machines", "机型与 BIOS"}, {"input", "输入"}, {"paths", "路径"}});
  tree->setSelectedId("appearance");
  page.samples.push_back(tree.get());
  stack->addChild(std::move(tree));
  page.content = std::move(stack);
  return page;
}

/** 表格页的页面大小：表头加三行（第二行选中并带详情），高度取 4 的倍数。 */
constexpr ui::Size kTablePageSize{560, 232};
/** 表格样本里左右各 9 像素（单元格左右内边距 10 少留 1，给文字边缘的半像素）参与像素对照（表头底色、分割线、悬停与选中底色），中间的文字不参与。 */
constexpr ui::Insets kTableInset{0, 9, 0, 9};

/** 表格页的详情块：高 24、底色取强调色，没有文字。 */
class DetailBlock final : public ui::Widget {
 public:
  ui::Size measure(ui::Size available) override { return ui::Size{available.width, 24.0}; }

  void paint(ui::Canvas& canvas) override { canvas.fillRect(bounds(), ui::themeTokens(host()->theme()).accent); }
};

/** 往表格里加一行：名称（可带说明）、版本、匹配、状态。 */
std::size_t addPluginRow(ui::Table& table, const char* name, const char* description, const char* version, const char* match, bool enabled) {
  std::vector<std::unique_ptr<ui::Widget>> cells;
  auto main = std::make_unique<ui::TableMainCell>();
  main->setText(name);
  main->setDescription(description);
  cells.push_back(std::move(main));
  cells.push_back(ui::Table::makeText(version));
  cells.push_back(ui::Table::makeText(match));
  auto state = std::make_unique<ui::StatusLabel>();
  state->setTone(enabled ? ui::StatusTone::Success : ui::StatusTone::Stopped);
  state->setText(enabled ? "已启用" : "已停用");
  cells.push_back(std::move(state));
  return table.addRow(std::move(cells));
}

/** 表格页：四列（主副文字、版本、匹配、状态）三行，第二行选中并带详情。 */
Page buildTablePage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  stack->setPadding(ui::Insets::all(kPagePadding));
  auto table = std::make_unique<ui::Table>();
  table->addColumn({"名称", 140, false, false});
  table->addColumn({"版本", 0, true, true});
  table->addColumn({"匹配", 120, false, true});
  table->addColumn({"状态", 0, true, false});
  addPluginRow(*table, "插件甲", "第一段说明", "1.0", "NES 卡带", true);
  const std::size_t selected = addPluginRow(*table, "插件乙", "说明乙", "2.1.3", "SB2000 软盘", false);
  addPluginRow(*table, "插件丙", "", "0.9", "全部", true);
  table->setRowDetail(selected, std::make_unique<DetailBlock>());
  table->setOnRowSelect([](std::size_t) {});
  table->setSelectedRow(static_cast<int>(selected));
  page.samples.push_back(table.get());
  stack->addChild(std::move(table));
  page.content = std::move(stack);
  return page;
}

/** 悬停提示页的页面大小：三个按钮一排，提示在按钮右侧或左侧。 */
constexpr ui::Size kTooltipPageSize{480, 160};
/** 提示里文字与图标都在四周 4 像素之内，只对照外圈。 */
constexpr ui::Insets kTooltipInset{4, 4, 4, 4};

/** 悬停提示页：三个按钮依次带短文字提示、会换行的长文字提示、版式提示。 */
Page buildTooltipPage() {
  Page page;
  auto row = std::make_unique<ui::Row>();
  row->setPadding(ui::Insets::all(kPagePadding));
  const auto add = [&](const char* label) {
    auto button = std::make_unique<ui::Button>(label);
    button->setAutoIcon(false);
    ui::Button* raw = button.get();
    row->addChild(std::move(button));
    page.samples.push_back(raw);
    return raw;
  };
  add("提示一")->setTooltip("这是一条短提示");
  add("提示二")->setTooltip("这是一段比较长的提示文字，用来验证提示在最大宽度处换行并保持角标位置。这是一段比较长的提示文字，用来验证提示在最大宽度处换行并保持角标位置。");
  add("提示三")->setRichTooltip(ui::TooltipContent{.title = "标题", .icon = ui::IconId::InfoCircle, .items = {ui::TooltipItem{.icon = ui::IconId::Eye, .title = "小标题", .text = "正文说明文字，稍微长一点用来换行验证宽度"}, ui::TooltipItem{.text = "第二条说明"}}});
  page.content = std::move(row);
  return page;
}

/** 通知页的页面大小：三条弹出的通知堆叠在右下角。 */
constexpr ui::Size kNotificationPageSize{480, 240};
/** 通知面板页的页面大小：顶部一条状态栏（带通知按钮），右下角展开的通知面板。 */
constexpr ui::Size kNotificationCenterPageSize{480, 360};
/** 通知与面板里的图标、文字都在四周 4 像素之内，只对照外圈。 */
constexpr ui::Insets kNoticeInset{4, 4, 4, 4};

/** 发三条通知：短消息、会换行的警告、超过三行被截断的错误。 */
void sendSampleNotices(ui::NotificationCenter& center) {
  center.notify("已保存设置。", ui::NoticeLevel::Info);
  center.notify("这是一条较长的警告消息，需要换行显示：卡带的存档目录无法写入，请检查目录权限后重试。", ui::NoticeLevel::Warning);
  center.notify("保存失败\n原因：磁盘已满。请释放空间后重试。详情详情详情详情详情详情详情详情详情详情详情详情详情详情详情详情详情详情详情详情", ui::NoticeLevel::Error);
}

/** 通知页：页面是空的，通知在布局后发出，弹出的通知就是样本。 */
Page buildNotificationPage() {
  Page page;
  page.content = std::make_unique<ui::Row>();
  page.setup = [](ui::NotificationCenter& center, std::vector<ui::Widget*>& samples) {
    sendSampleNotices(center);
    samples = center.toastWidgets();
  };
  return page;
}

/** 通知面板页：状态栏右侧是通知按钮；发三条通知后打开面板，面板自己与各条历史是额外的矩形。 */
Page buildNotificationCenterPage() {
  Page page;
  auto stack = std::make_unique<ui::Stack>();
  auto bar = std::make_unique<ui::StatusBar>();
  ui::StatusBar* raw = bar.get();
  stack->addChild(std::move(bar));
  page.samples.push_back(raw);
  page.content = std::move(stack);
  page.setup = [raw](ui::NotificationCenter& center, std::vector<ui::Widget*>& samples) {
    samples.push_back(raw->addEndWidget(std::make_unique<ui::NotificationButton>(center)));
    sendSampleNotices(center);
    center.setOpen(true);
  };
  return page;
}

/** 对话框页的页面大小：对话框的内容铺满窗口。 */
constexpr ui::Size kDialogPageSize{420, 160};
/** 标题栏里 24×24 的小方块按钮：图标在四周 4 像素之内。 */
constexpr ui::Insets kDialogControlInset{4, 4, 4, 4};

/** 对话框页：标题栏带最大化与关闭按钮，一段正文，按钮区有“取消”与默认的“确定”。 */
Page buildDialogPage() {
  Page page;
  auto dialog = std::make_unique<ui::DialogFrame>();
  ui::DialogFrame* raw = dialog.get();
  raw->setTitle("对话框标题");
  raw->setMaximizable(true);
  raw->setContent(std::make_unique<ui::Label>("这是对话框的正文内容。"));
  raw->setButtons({ui::DialogButton{.id = "cancel", .text = "取消", .isCancel = true}, ui::DialogButton{.id = "ok", .text = "确定", .variant = ui::ButtonVariant::Primary, .isDefault = true}});
  page.samples = {raw->maximizeControl(), raw->closeControl(), raw->button("cancel"), raw->button("ok")};
  page.content = std::move(dialog);
  return page;
}

/** 窗口框架页的页面大小：框架铺满窗口。 */
constexpr ui::Size kAppFramePageSize{480, 240};

/** 窗口框架页：标题栏、空的页面区、高 60 的底部面板、大尺寸状态栏（左右各一个状态项）。 */
Page buildAppFramePage() {
  Page page;
  auto frame = std::make_unique<ui::AppFrame>();
  ui::AppFrame* raw = frame.get();
  ui::TitleBar* titleBar = raw->setTitleBar(std::make_unique<ui::TitleBar>());
  titleBar->setTitle("Ember");
  titleBar->setSubtitle("模拟器");
  raw->setStatusBarLarge(true);
  ui::StatusBar* statusBar = raw->setStatusBar(std::make_unique<ui::StatusBar>());
  ui::StatusBarItem* start = statusBar->addStart(std::make_unique<ui::StatusBarItem>("就绪"));
  ui::StatusBarItem* end = statusBar->addEnd(std::make_unique<ui::StatusBarItem>("60 fps"));
  raw->setPage(std::make_unique<ui::Widget>());
  raw->setBottomPanel(std::make_unique<ui::Widget>());
  raw->setBottomPanelMinHeight(60);
  raw->setBottomPanelHeight(60);
  raw->setBottomPanelOpen(true);
  page.samples = {raw, titleBar, raw->pageRegion(), raw->bottomRegion(), statusBar, start, end};
  page.content = std::move(frame);
  return page;
}

}  // namespace

const std::vector<PageInfo>& pageInfos() {
  static const std::vector<PageInfo> infos = {
      {"button", kButtonPageSize, ui::Insets::all(3), {{kLinkSample, ui::Insets::all(kLinkMargin)}}, {}, {0, 1, 3, 5, 7, kLinkSample}, {"hover", "pressed", "focus"}},
      {"input", kInputPageSize, ui::Insets::symmetric(4, 3), {}, {}, {0, 1, 2, 3, 6, 7}, {"hover", "pressed", "focus"}},
      {"choice", kChoicePageSize, ui::Insets{0, 0, 0, kCheckboxMarkWidth}, {{4, kGroupInset}, {5, kGroupInset}, {6, kGroupInset}, {7, kGroupInset}}, {}, {0, 1, 2, 3, 4, 5, 6, 7}, {"hover", "pressed", "focus"}},
      {"select", kSelectPageSize, kSelectInset, {}, kOptionInset, {0, 1, 2}, {"hover", "pressed", "focus", "open", "openkey", "openhover"}},
      {"menu", kMenuPageSize, kMenuInset, {}, kMenuPopupInset, {0, 1, 2}, {"hover", "pressed", "focus", "open", "openkey", "openhover"}},
      {"title-bar", kTitleBarPageSize, kTitleBarInset, {{1, kMenuInset}, {2, ui::Insets::all(3)}, {3, kTitleBarToolInset}, {4, kTitleBarToolInset}, {5, kTitleBarToolInset}}, {}, {2, 3, 4, 5}, {"hover", "pressed"}},
      {"status-bar", kStatusBarPageSize, kStatusBarInset, {}, {}, {3, 7, 8}, {"hover", "pressed"}},
      {"tooltip", kTooltipPageSize, ui::Insets::all(3), {}, kTooltipInset, {0, 1, 2}, {"tip"}},
      {"notification", kNotificationPageSize, kNoticeInset, {}, {}, {}, {}, 3},
      {"dialog", kDialogPageSize, kDialogControlInset, {{2, ui::Insets::all(3)}, {3, ui::Insets::all(3)}}, {}, {0, 1, 2, 3}, {"hover", "pressed"}},
      {"app-frame", kAppFramePageSize, ui::Insets::all(4), {{1, kTitleBarInset}, {4, kStatusBarInset}, {5, ui::Insets::all(3)}, {6, ui::Insets::all(3)}}, {}, {}, {}},
      {"notification-center", kNotificationCenterPageSize, ui::Insets::all(3), {}, kNoticeInset, {}, {}, 1},
      {"status-bar-large", kStatusBarLargePageSize, kStatusBarInset, {}, {}, {3, 7, 8}, {"hover", "pressed"}},
      {"slider", kSliderPageSize, ui::Insets::all(kCompareAll), {}, {}, {0, 1, 2, 3}, {"hover", "pressed", "focus"}},
      {"scroll", kScrollPageSize, ui::Insets::all(kCompareAll), {}, {}, {0}, {"hover", "wheel", "drag"}},
      // 面板样本高度：122、74、88、90、46（边框、标题行、44 或 28、内容区内边距与内容块）
      {"panel", kPanelPageSize, titleInset(12, 34, 64, 122), {{1, titleInset(4, 25, 72, 74)}, {2, titleInset(11, 33, 50, 88)}, {3, ui::Insets::all(kCompareAll)}, {4, titleInset(12, 34, 72, 46)}}, {}, {}, {}},
      // 键值清单全是文字，样本内部整个不参与像素对照
      {"facts", kFactsPageSize, ui::Insets::all(0), {}, {}, {}, {}},
      {"facts-narrow", kFactsNarrowPageSize, ui::Insets::all(0), {}, {}, {}, {}},
      // 字段：有标签的上半部分是文字（底部 22 像素是滑块），没有标签的底部是文字（顶部 22 像素是滑块）
      {"field", kFieldPageSize, ui::Insets{0, 0, ui::tokens::slider_height, 0}, {{1, ui::Insets{ui::tokens::slider_height, 0, 0, 0}}}, {}, {}, {}},
      // 设置行：宽窗口左边 420 像素是名称与说明；带错误文字的行整个不参与
      {"setting-row", kSettingRowPageSize, ui::Insets{0, 728 - ui::tokens::setting_row_text_width, 0, 0}, {{2, ui::Insets::all(0)}}, {}, {}, {}},
      {"setting-row-narrow", kSettingRowNarrowPageSize, ui::Insets{6, 0, 37, 0}, {{2, ui::Insets::all(0)}}, {}, {}, {}},
      // 反馈：提示块文字与按钮在内边距之内；说明块与状态标签整个只对照文字与图标的外框
      {"feedback", kFeedbackPageSize, ui::Insets::all(0), {{0, kCalloutInset}, {1, kCalloutInset}}, {}, {}, {}},
      // 分栏：整个视图与分隔条都参与对照；悬停、按下、焦点的目标是分隔条（样本 1）
      {"split", kSplitPageSize, ui::Insets::all(kCompareAll), {}, {}, {1}, {"hover", "pressed", "focus"}},
      // 列表：面板样本只对照外框位置；列表样本左边 420 像素（名称列）不参与像素对照
      {"list", kListPageSize, ui::Insets{0, kListWidth - ui::tokens::setting_row_text_width, 0, 0}, {{0, ui::Insets::all(0)}}, {}, {}, {}},
      // 信息行：前两行右边的操作参与对照，第三行只有文字；悬停与按下的目标是第一行
      {"row-list", kRowListPageSize, kRowListInset, {{1, ui::Insets{0, 60, 0, 0}}, {2, ui::Insets::all(0)}}, {}, {0}, {"hover", "pressed"}},
      // 工具栏：中间的说明文字（约 100 到 170）不参与，左边的滑块与右边的操作参与
      {"toolbar", kToolbarPageSize, ui::Insets{0, 528 - 170, 0, 100}, {}, {}, {}, {}},
      {"toolbar-narrow", kToolbarNarrowPageSize, ui::Insets{0, 8, 0, 100}, {}, {}, {}, {}},
      // 树：悬停、按下的目标是整棵树的中心（第三行）；焦点落在选中行上
      {"tree", kTreePageSize, kTreeInset, {}, {}, {0}, {"hover", "pressed", "focus"}},
      // 表格：悬停与按下的目标是整张表的中心所在的行
      {"table", kTablePageSize, kTableInset, {}, {}, {0}, {"hover", "pressed"}},
      // 页签：两条页签栏（文字带不参与）与第二条下面的内容块（全部参与）；悬停、按下、焦点的目标是第一条
      {"tabs", kTabsPageSize, kTabBarInset, {{2, ui::Insets::all(kCompareAll)}}, {}, {0}, {"hover", "pressed", "focus"}},
  };
  return infos;
}

Page buildPage(const std::string& name) {
  if (name == "button") return buildButtonPage();
  if (name == "input") return buildInputPage();
  if (name == "choice") return buildChoicePage();
  if (name == "select") return buildSelectPage();
  if (name == "menu") return buildMenuPage();
  if (name == "title-bar") return buildTitleBarPage();
  if (name == "status-bar") return buildStatusBarPage(false);
  if (name == "tooltip") return buildTooltipPage();
  if (name == "notification") return buildNotificationPage();
  if (name == "dialog") return buildDialogPage();
  if (name == "app-frame") return buildAppFramePage();
  if (name == "notification-center") return buildNotificationCenterPage();
  if (name == "status-bar-large") return buildStatusBarPage(true);
  if (name == "slider") return buildSliderPage();
  if (name == "scroll") return buildScrollPage();
  if (name == "panel") return buildPanelPage();
  if (name == "facts" || name == "facts-narrow") return buildFactsPage();
  if (name == "field") return buildFieldPage();
  if (name == "setting-row" || name == "setting-row-narrow") return buildSettingRowPage();
  if (name == "feedback") return buildFeedbackPage();
  if (name == "split") return buildSplitPage();
  if (name == "list") return buildListPage();
  if (name == "row-list") return buildRowListPage();
  if (name == "toolbar" || name == "toolbar-narrow") return buildToolbarPage();
  if (name == "tree") return buildTreePage();
  if (name == "table") return buildTablePage();
  if (name == "tabs") return buildTabsPage();
  throw std::runtime_error("未知的示例页：" + name);
}

}  // namespace gallery
