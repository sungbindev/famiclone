/*
 * ------------------------------------------------------------------------
 * 名称：main.cpp
 * 说明：控件库预览程序：显示各控件的示例页；带 --render 参数时改为离屏绘制成 PNG 并输出样本区域，供和网页版截图对照与自动测试。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用法：
 *       rujian-ui-gallery.exe [--page 页名] [--theme dark|light]      打开预览窗口
 *       rujian-ui-gallery.exe --info                                  输出全部示例页的说明（JSON）
 *       rujian-ui-gallery.exe --render 输出.png [选项]                 离屏绘制
 *       选项：--page 页名  --scene normal|hover|pressed|focus  --target 样本序号  --theme dark|light  --scale 缩放比例  --rects 输出.json
 *       场景：hover 把鼠标移到目标样本；pressed 再按下左键；focus 按 Tab 键直到焦点落在目标样本上。
 *       离屏绘制时文字光标一直隐藏，网页版参考页的光标颜色也设为透明。
 * ------------------------------------------------------------------------
 */

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "base/theme-tokens.hpp"
#include "pages.hpp"
#include "rujian/ui/offscreen.hpp"
#include "rujian/ui/root.hpp"
#include "rujian/ui/scroll-area.hpp"
#include "rujian/ui/select.hpp"
#include "rujian/ui/window.hpp"

namespace ui = rujian::ui;

namespace {

constexpr int kVirtualKeyTab = 0x09;
constexpr int kVirtualKeyDown = 0x28;
/** openhover 场景里鼠标悬停的选项序号。 */
constexpr std::size_t kHoverOption = 2;
/** drag 场景里滑块向下拖的距离。 */
constexpr double kDragDistance = 30.0;

/** 命令行选项。 */
struct Options {
  std::filesystem::path render;
  std::filesystem::path rects;
  std::string page{"button"};
  std::string scene{"normal"};
  int target{0};
  ui::ThemeName theme{ui::ThemeName::Dark};
  double scale{1.0};
  bool info{false};
};

/** UTF-8 命令行参数转路径。 */
std::filesystem::path pathFromUtf8(const char* text) { return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text))); }

/** 解析命令行；参数不合法时返回 false 并输出说明。 */
bool parseArguments(int argc, char** argv, Options& options) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto value = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };
    const char* text = nullptr;
    if (arg == "--info") options.info = true;
    else if (arg == "--render" && (text = value()) != nullptr) options.render = pathFromUtf8(text);
    else if (arg == "--rects" && (text = value()) != nullptr) options.rects = pathFromUtf8(text);
    else if (arg == "--page" && (text = value()) != nullptr) options.page = text;
    else if (arg == "--scene" && (text = value()) != nullptr) options.scene = text;
    else if (arg == "--target" && (text = value()) != nullptr) options.target = std::atoi(text);
    else if (arg == "--theme" && (text = value()) != nullptr && (std::strcmp(text, "dark") == 0 || std::strcmp(text, "light") == 0)) options.theme = std::strcmp(text, "light") == 0 ? ui::ThemeName::Light : ui::ThemeName::Dark;
    else if (arg == "--scale" && (text = value()) != nullptr && std::atof(text) > 0) options.scale = std::atof(text);
    else {
      std::fprintf(stderr, "参数不正确：%s\n", arg.c_str());
      return false;
    }
  }
  return true;
}

/** 找页的说明；没有时返回空指针。 */
const gallery::PageInfo* findInfo(const std::string& name) {
  for (const gallery::PageInfo& info : gallery::pageInfos()) {
    if (info.name == name) return &info;
  }
  return nullptr;
}

/** 边距的 JSON 写法。 */
std::string insetJson(const ui::Insets& inset) {
  char text[128];
  std::snprintf(text, sizeof(text), "{\"left\": %g, \"top\": %g, \"right\": %g, \"bottom\": %g}", inset.left, inset.top, inset.right, inset.bottom);
  return text;
}

/** 输出全部示例页的说明。 */
int printInfo() {
  std::printf("[\n");
  const auto& infos = gallery::pageInfos();
  for (std::size_t i = 0; i < infos.size(); ++i) {
    const gallery::PageInfo& info = infos[i];
    std::printf("  {\"name\": \"%s\", \"width\": %g, \"height\": %g, \"inset\": %s, \"overrides\": [", info.name.c_str(), info.size.width, info.size.height, insetJson(info.inset).c_str());
    for (std::size_t o = 0; o < info.insetOverrides.size(); ++o) {
      const auto& [index, inset] = info.insetOverrides[o];
      std::printf("%s{\"index\": %d, \"inset\": %s}", o > 0 ? ", " : "", index, insetJson(inset).c_str());
    }
    std::printf("], \"targets\": [");
    for (std::size_t t = 0; t < info.targets.size(); ++t) std::printf("%s%d", t > 0 ? ", " : "", info.targets[t]);
    std::printf("], \"scenes\": [");
    for (std::size_t s = 0; s < info.scenes.size(); ++s) std::printf("%s\"%s\"", s > 0 ? ", " : "", info.scenes[s].c_str());
    std::printf("], \"sampleCount\": %zu, \"extraInset\": %s}%s\n", gallery::buildPage(info.name).samples.size(), insetJson(info.extraInset).c_str(), i + 1 < infos.size() ? "," : "");
  }
  std::printf("]\n");
  return 0;
}

/** 用输入事件把目标样本带到指定场景；场景不认识时返回 false。 */
bool applyScene(ui::Root& root, const std::vector<ui::Widget*>& samples, const std::string& scene, int target) {
  if (scene == "normal") return true;
  if (target < 0 || static_cast<std::size_t>(target) >= samples.size()) {
    std::fprintf(stderr, "目标样本不存在：%d\n", target);
    return false;
  }
  const ui::Rect box = samples[static_cast<std::size_t>(target)]->bounds();
  const ui::Point center{box.x + box.width / 2, box.y + box.height / 2};
  if (scene == "hover") {
    root.mouseMove(center, 0);
  } else if (scene == "pressed") {
    root.mouseMove(center, 0);
    root.mouseDown(center, ui::MouseButton::Left, 0);
  } else if (scene == "focus") {
    // 焦点顺序就是样本顺序：数出目标之前（含目标）有多少个可获得焦点的样本
    int presses = 0;
    for (int i = 0; i <= target; ++i) {
      const ui::Widget* sample = samples[static_cast<std::size_t>(i)];
      if (sample->enabled() && sample->focusable()) ++presses;
    }
    for (int i = 0; i < presses; ++i) root.keyDown(kVirtualKeyTab, 0, false);
  } else if (scene == "open" || scene == "openhover") {
    // 在下拉框上点一下展开弹层；openhover 再把鼠标移到弹层里的第三个选项上
    root.mouseMove(center, 0);
    root.mouseDown(center, ui::MouseButton::Left, 0);
    root.mouseUp(center, ui::MouseButton::Left, 0);
    if (scene == "openhover") {
      const auto* select = dynamic_cast<const ui::Select*>(samples[static_cast<std::size_t>(target)]);
      const auto row = select != nullptr ? select->optionRect(kHoverOption) : std::nullopt;
      // 禁用的下拉框展开不了，没有选项可悬停
      if (row) root.mouseMove(ui::Point{row->x + row->width / 2, row->y + row->height / 2}, 0);
    }
  } else if (scene == "wheel") {
    // 鼠标移到样本上再向下滚动一格
    root.mouseMove(center, 0);
    root.mouseWheel(center, -1.0, 0);
  } else if (scene == "drag") {
    // 鼠标移到样本上，在滚动条滑块上按下并向下拖 30 像素
    root.mouseMove(center, 0);
    const auto* area = dynamic_cast<const ui::ScrollArea*>(samples[static_cast<std::size_t>(target)]);
    if (area == nullptr) return false;
    const ui::Rect slider = area->sliderRect();
    const ui::Point grab{slider.x + slider.width / 2, slider.y + slider.height / 2};
    root.mouseMove(grab, 0);
    root.mouseDown(grab, ui::MouseButton::Left, 0);
    root.mouseMove(ui::Point{grab.x, grab.y + kDragDistance}, 0);
  } else if (scene == "openkey") {
    // 用 Tab 把焦点带到下拉框上，再按“向下”键展开弹层
    for (int i = 0; i <= target; ++i) {
      const ui::Widget* sample = samples[static_cast<std::size_t>(i)];
      if (sample->enabled() && sample->focusable()) root.keyDown(kVirtualKeyTab, 0, false);
    }
    root.keyDown(kVirtualKeyDown, 0, false);
  } else {
    std::fprintf(stderr, "未知的场景：%s\n", scene.c_str());
    return false;
  }
  return true;
}

/** 离屏绘制，成功返回 0。 */
int renderOffscreen(const Options& options) {
  const gallery::PageInfo* info = findInfo(options.page);
  if (info == nullptr) {
    std::fprintf(stderr, "未知的示例页：%s\n", options.page.c_str());
    return 2;
  }
  ui::Root root(options.theme, options.scale);
  gallery::Page page = gallery::buildPage(options.page);
  const std::vector<ui::Widget*> samples = page.samples;
  root.setContent(std::move(page.content));
  root.setCaretHidden(true);
  root.setAnimationsEnabled(false);
  root.layout(info->size);
  if (!applyScene(root, samples, options.scene, options.target)) return 2;

  ui::OffscreenSurface surface(info->size, options.scale);
  if (!surface.render(root, ui::themeTokens(options.theme).content_bg) || !surface.savePng(options.render)) {
    std::fprintf(stderr, "绘制或保存失败\n");
    return 1;
  }
  if (!options.rects.empty()) {
    // 样本的位置，之后是展开的下拉框弹层里各个选项的位置（与参考页里 .ui-select__option 的顺序一致）
    std::vector<ui::Rect> rects;
    for (const ui::Widget* sample : samples) rects.push_back(sample->bounds());
    for (const ui::Widget* sample : samples) {
      const auto* select = dynamic_cast<const ui::Select*>(sample);
      if (select == nullptr) continue;
      for (std::size_t row = 0; const auto rect = select->optionRect(row); ++row) rects.push_back(*rect);
    }
    std::ofstream out(options.rects, std::ios::binary);
    out << "[\n";
    for (std::size_t i = 0; i < rects.size(); ++i) {
      const ui::Rect& r = rects[i];
      out << "  {\"index\": " << i << ", \"x\": " << r.x << ", \"y\": " << r.y << ", \"width\": " << r.width << ", \"height\": " << r.height << "}" << (i + 1 < rects.size() ? "," : "") << "\n";
    }
    out << "]\n";
  }
  return 0;
}

/** 打开预览窗口。 */
int runWindow(const Options& options) {
  const gallery::PageInfo* info = findInfo(options.page);
  if (info == nullptr) {
    std::fprintf(stderr, "未知的示例页：%s\n", options.page.c_str());
    return 2;
  }
  ui::HostWindow window(ui::HostWindow::Options{.title = L"如见控件库预览", .clientSize = info->size, .theme = options.theme});
  window.root().setContent(gallery::buildPage(options.page).content);
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
    if (options.info) return printInfo();
    return options.render.empty() ? runWindow(options) : renderOffscreen(options);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "出错：%s\n", error.what());
    return 1;
  }
}
