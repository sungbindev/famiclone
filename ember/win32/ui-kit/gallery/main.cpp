/*
 * ------------------------------------------------------------------------
 * 名称：main.cpp
 * 说明：控件库预览程序：显示各控件的示例页；带 --render 参数时改为离屏绘制成 PNG 并输出控件区域，供和网页版截图对照与自动测试。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用法：
 *       rujian-ui-gallery.exe                                  打开预览窗口
 *       rujian-ui-gallery.exe --render 输出.png [选项]         离屏绘制
 *       选项：--scene 场景名（normal、hover、pressed、focus）  --theme dark|light  --scale 缩放比例  --rects 输出.json
 *       示例页是一排按钮：取消、确定、带图标的保存、禁用；hover、pressed 针对第一个按钮，focus 用 Tab 键移到第一个按钮。
 * ------------------------------------------------------------------------
 */

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "base/theme-tokens.hpp"
#include "rujian/ui/button.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/window.hpp"

namespace ui = rujian::ui;

namespace {

/** 示例页的内边距与按钮间距。 */
constexpr double kPagePadding = 16.0;
constexpr double kButtonGap = 8.0;
constexpr ui::Size kPageSize{440, 60};

/** 示例页：一排按钮，按钮按各自想要的宽度从左到右摆放。 */
class SamplePage final : public ui::Widget {
 public:
  void arrange(const ui::Rect& bounds) override {
    Widget::arrange(bounds);
    double x = bounds.x + kPagePadding;
    for (const auto& child : children()) {
      const ui::Size size = child->measure({});
      child->arrange(ui::Rect{x, bounds.y + kPagePadding, size.width, size.height});
      x += size.width + kButtonGap;
    }
  }
};

/** 建好示例页，并把四个按钮的指针按顺序放进 buttons。 */
std::unique_ptr<SamplePage> buildPage(std::vector<ui::Button*>& buttons) {
  auto page = std::make_unique<SamplePage>();
  const auto add = [&](const char* label) {
    auto button = std::make_unique<ui::Button>(label);
    buttons.push_back(button.get());
    page->addChild(std::move(button));
  };
  add("取消");
  add("确定");
  add("保存");
  add("禁用");
  buttons[1]->setVariant(ui::ButtonVariant::Primary);
  buttons[2]->setIcon(ui::IconId::DeviceFloppy);
  buttons[3]->setEnabled(false);
  return page;
}

/** 命令行选项。 */
struct Options {
  std::filesystem::path render;
  std::filesystem::path rects;
  std::string scene{"normal"};
  ui::ThemeName theme{ui::ThemeName::Dark};
  double scale{1.0};
};

/** UTF-8 命令行参数转路径。 */
std::filesystem::path pathFromUtf8(const char* text) { return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text))); }

/** 解析命令行；参数不合法时返回 false 并输出说明。 */
bool parseArguments(int argc, char** argv, Options& options) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto value = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };
    const char* text = nullptr;
    if (arg == "--render" && (text = value()) != nullptr) options.render = pathFromUtf8(text);
    else if (arg == "--rects" && (text = value()) != nullptr) options.rects = pathFromUtf8(text);
    else if (arg == "--scene" && (text = value()) != nullptr) options.scene = text;
    else if (arg == "--theme" && (text = value()) != nullptr && (std::strcmp(text, "dark") == 0 || std::strcmp(text, "light") == 0)) options.theme = std::strcmp(text, "light") == 0 ? ui::ThemeName::Light : ui::ThemeName::Dark;
    else if (arg == "--scale" && (text = value()) != nullptr && std::atof(text) > 0) options.scale = std::atof(text);
    else {
      std::fprintf(stderr, "参数不正确：%s\n", arg.c_str());
      return false;
    }
  }
  return true;
}

/** 离屏绘制，成功返回 0。 */
int renderOffscreen(const Options& options) {
  ui::Root root(options.theme, options.scale);
  std::vector<ui::Button*> buttons;
  root.setContent(buildPage(buttons));
  root.layout(kPageSize);

  // 场景：用输入事件把第一个按钮带到指定状态
  const ui::Rect first = buttons[0]->bounds();
  const ui::Point center{first.x + first.width / 2, first.y + first.height / 2};
  if (options.scene == "hover") {
    root.mouseMove(center, 0);
  } else if (options.scene == "pressed") {
    root.mouseMove(center, 0);
    root.mouseDown(center, ui::MouseButton::Left, 0);
  } else if (options.scene == "focus") {
    root.keyDown(0x09, 0, false);
  } else if (options.scene != "normal") {
    std::fprintf(stderr, "未知的场景：%s\n", options.scene.c_str());
    return 2;
  }

  ui::OffscreenSurface surface(kPageSize, options.scale);
  if (!surface.render(root, ui::themeTokens(options.theme).content_bg) || !surface.savePng(options.render)) {
    std::fprintf(stderr, "绘制或保存失败\n");
    return 1;
  }
  if (!options.rects.empty()) {
    std::ofstream out(options.rects, std::ios::binary);
    out << "[\n";
    for (std::size_t i = 0; i < buttons.size(); ++i) {
      const ui::Rect r = buttons[i]->bounds();
      out << "  {\"index\": " << i << ", \"x\": " << r.x << ", \"y\": " << r.y << ", \"width\": " << r.width << ", \"height\": " << r.height << "}" << (i + 1 < buttons.size() ? "," : "") << "\n";
    }
    out << "]\n";
  }
  return 0;
}

/** 打开预览窗口。 */
int runWindow() {
  ui::HostWindow window(ui::HostWindow::Options{.title = L"如见控件库预览", .clientSize = kPageSize, .theme = ui::ThemeName::Dark});
  std::vector<ui::Button*> buttons;
  window.root().setContent(buildPage(buttons));
  // “取消”按钮用来切换主题，方便目测两个主题
  buttons[0]->setOnClick([&window] { window.root().setTheme(window.root().theme() == ui::ThemeName::Dark ? ui::ThemeName::Light : ui::ThemeName::Dark); });
  window.root().layout(window.root().size());
  window.show();
  return ui::HostWindow::runMessageLoop();
}

}  // namespace

int main(int argc, char** argv) {
  SetConsoleOutputCP(CP_UTF8);
  Options options;
  if (!parseArguments(argc, argv, options)) return 2;
  try {
    return options.render.empty() ? runWindow() : renderOffscreen(options);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "出错：%s\n", error.what());
    return 1;
  }
}
