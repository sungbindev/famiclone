/*
 * ------------------------------------------------------------------------
 * 名称：application.cpp
 * 说明：组装根的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：局部对象按“被依赖的先创建”的顺序声明，退出时按相反顺序销毁：窗口与界面线程最后销毁，核心与请求路由在界面状态服务之前销毁；
 *       主窗口等主题同步完成（最多等 2 秒）再显示，启动时不会先闪一下错误的颜色。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/application.hpp"

#include <windows.h>
#include <windowsx.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "bootstrap/resource-locations.hpp"
#include "entry/cartridges/cartridge-handlers.hpp"
#include "entry/machines/machine-handlers.hpp"
#include "entry/main-requests/main-request-router.hpp"
#include "entry/mappers/mapper-handlers.hpp"
#include "entry/media/media-handlers.hpp"
#include "entry/preferences/preference-handlers.hpp"
#include "entry/preferences/theme-sync.hpp"
#include "entry/system/system-handlers.hpp"
#include "infra/core-client/in-process-core-gateway.hpp"
#include "infra/core-client/latest-frame-slot.hpp"
#include "infra/dialogs/remembering-file-dialog.hpp"
#include "infra/directories/explorer-directory.hpp"
#include "infra/display/screen-host-adapter.hpp"
#include "infra/dropped-files/memory-dropped-files-service.hpp"
#include "infra/help/unavailable-help-viewer.hpp"
#include "infra/last-directories/file-last-directories-service.hpp"
#include "infra/logging/file-error-logger-adapter.hpp"
#include "infra/recent-cartridges/file-recent-cartridges-service.hpp"
#include "infra/shell-state/file-shell-state-service.hpp"
#include "infra/updates/unsupported-app-updater.hpp"
#include "logic/input/input-controller.hpp"
#include "logic/runtime/runtime-controller.hpp"
#include "logic/runtime/runtime-module.hpp"
#include "logic/shell/shell-controller.hpp"
#include "logic/shell/shell-module.hpp"
#include "platform/dialogs/message-box.hpp"
#include "platform/dialogs/system-dialogs.hpp"
#include "platform/files/json-file-reader.hpp"
#include "platform/files/json-file-store.hpp"
#include "platform/files/known-folders.hpp"
#include "platform/logging/file-error-logger.hpp"
#include "platform/process/environment.hpp"
#include "platform/process/single-instance.hpp"
#include "platform/text/wide-text.hpp"
#include "platform/theme/system-theme.hpp"
#include "platform/threading/ui-thread.hpp"
#include "platform/window/frameless-frame.hpp"
#include "platform/window/window-modes.hpp"
#include "platform/window/window-placement.hpp"
#include "rujian/ui/window.hpp"
#include "shared/preferences/allow-multiple-instances.hpp"
#include "shared/shortcut/shortcut.hpp"
#include "ui/runtime/runtime-panel-content.hpp"
#include "ui/runtime/screen-widget.hpp"
#include "ui/shell/main-window-view.hpp"
#include "ui/shell/shell-view.hpp"

namespace ember::win32::bootstrap {

namespace {

/** 应用名：标题栏上显示的名字。 */
constexpr const char* kAppName = "EMBER";
/** 窗口标题（任务栏与系统菜单里的名字）。 */
constexpr const wchar_t* kWindowTitle = L"Ember";
/** 窗口的大小与最小尺寸（逻辑像素），见 docs/05-界面.md 第 2 节。 */
constexpr double kWindowWidth = 524;
constexpr double kWindowHeight = 500;
/** 显示窗口前最多等保存的主题多久（毫秒），核心起不来时不能一直不显示。 */
constexpr unsigned kThemeWaitMilliseconds = 2000;
/** 主进程错误的错误码，同 Electron 外壳。 */
constexpr const char* kMainErrorCode = "shell.mainError";

/** 主题来源转成要显示的主题；跟随系统时读系统设置。 */
rujian::ui::ThemeName resolveTheme(entry::ThemeSource source) {
  switch (source) {
    case entry::ThemeSource::Dark:
      return rujian::ui::ThemeName::Dark;
    case entry::ThemeSource::Light:
      return rujian::ui::ThemeName::Light;
    case entry::ThemeSource::System:
      break;
  }
  return platform::systemUsesLightTheme() ? rujian::ui::ThemeName::Light : rujian::ui::ThemeName::Dark;
}

rujian::ui::TitleBarHit toTitleBarHit(platform::FrameHit hit) {
  switch (hit) {
    case platform::FrameHit::Caption:
      return rujian::ui::TitleBarHit::Caption;
    case platform::FrameHit::Minimize:
      return rujian::ui::TitleBarHit::Minimize;
    case platform::FrameHit::Maximize:
      return rujian::ui::TitleBarHit::Maximize;
    case platform::FrameHit::Close:
      return rujian::ui::TitleBarHit::Close;
    case platform::FrameHit::Client:
      break;
  }
  return rujian::ui::TitleBarHit::Client;
}

platform::FrameHit toFrameHit(rujian::ui::TitleBarHit hit) {
  switch (hit) {
    case rujian::ui::TitleBarHit::Caption:
      return platform::FrameHit::Caption;
    case rujian::ui::TitleBarHit::Minimize:
      return platform::FrameHit::Minimize;
    case rujian::ui::TitleBarHit::Maximize:
      return platform::FrameHit::Maximize;
    case rujian::ui::TitleBarHit::Close:
      return platform::FrameHit::Close;
    case rujian::ui::TitleBarHit::Client:
      break;
  }
  return platform::FrameHit::Client;
}

/** 把窗口提到前面：最小化时先还原。 */
void activateWindow(HWND window) {
  if (window == nullptr) return;
  if (IsIconic(window) != FALSE) ShowWindow(window, SW_RESTORE);
  SetForegroundWindow(window);
}

/** 外壳的三个端口先于主窗口创建，它们的实现（窗口模式、通知）要等窗口与界面创建之后才有，所以用转发器先占位。 */
class ForwardingWindowControl final : public contracts::ShellWindowControl {
 public:
  platform::WindowModes* target{nullptr};
  std::function<void()> onQuit;
  void toggleFullScreen() override {
    if (target != nullptr) target->toggleFullScreen();
  }
  void toggleAlwaysOnTop() override {
    if (target != nullptr) target->toggleAlwaysOnTop();
  }
  void quit() override {
    if (onQuit) onQuit();
  }
};

class ForwardingNotifier final : public contracts::ShellNotifier {
 public:
  contracts::ShellNotifier* target{nullptr};
  void notify(const std::string& text, contracts::NotificationLevel level) override {
    if (target != nullptr) target->notify(text, level);
  }
};

/** 对话框：阶段 7 没有登记任何对话框，对话框窗口随后续阶段加入。 */
class NoDialogs final : public contracts::ShellDialogOpener {
 public:
  void openDialog(const std::string& /*dialogId*/) override {}
};

/** 当前的修饰键（Ctrl、Shift、Alt）。 */
unsigned currentModifiers() {
  unsigned modifiers = 0;
  if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) modifiers |= shared::kShortcutShift;
  if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) modifiers |= shared::kShortcutCtrl;
  if ((GetKeyState(VK_MENU) & 0x8000) != 0) modifiers |= shared::kShortcutAlt;
  return modifiers;
}

}  // namespace

int runApplication(const shared::LaunchArguments& launch) {
  std::filesystem::path userData = launch.userDataDirectory ? platform::pathFromUtf8(*launch.userDataDirectory) : platform::defaultUserDataDirectory();
  if (userData.empty()) {
    platform::showErrorMessage("Ember", "无法启动：找不到用户数据目录。");
    return 1;
  }
  std::error_code ignored;
  if (const auto absolute = std::filesystem::absolute(userData, ignored); !ignored) userData = absolute;

  platform::UiThread ui;
  /** 主窗口句柄：单实例的通知与文件选择窗口的父窗口都用它，窗口创建后才有值。 */
  HWND mainWindow = nullptr;

  // 单实例：首选项里允许同时运行多个实例时不取锁；这时核心还没启动，所以直接读首选项文件
  const auto preferences = platform::readJsonFile(userData / "config" / "preferences.json");
  std::optional<platform::SingleInstance> singleInstance;
  if (!(preferences && shared::readAllowMultipleInstances(*preferences))) {
    singleInstance.emplace(userData, [&mainWindow] { activateWindow(mainWindow); });
    switch (singleInstance->acquire()) {
      case platform::SingleInstanceResult::Acquired:
        break;
      case platform::SingleInstanceResult::NotifiedExisting:
        return 0;
      case platform::SingleInstanceResult::InUse:
        platform::showErrorMessage("Ember", "无法启动：这份用户数据正在被另一个 Ember 使用，但它没有响应。请先结束它再启动。");
        return 1;
    }
  }

  platform::FileErrorLogger fileLogger(userData / "logs");
  infra::FileErrorLoggerAdapter logger(fileLogger);
  /** 文件存储读写出错时写日志。 */
  const auto reportTo = [&logger](const char* source) {
    return [&logger, source](const std::string& message) { logger.write({std::nullopt, kMainErrorCode, message, nlohmann::json{{"source", source}}}); };
  };

  platform::JsonFileStore shellStateStore(userData / "config" / "shell-state.json", ui, reportTo("shellState"));
  infra::FileShellStateService shellState(shellStateStore);
  platform::JsonFileStore recentStore(userData / "config" / "recent-cartridges.json", ui, reportTo("recentCartridges"));
  infra::FileRecentCartridgesService recentCartridges(recentStore);
  platform::JsonFileStore lastDirectoriesStore(userData / "config" / "last-directories.json", ui, reportTo("lastDirectories"));
  infra::FileLastDirectoriesService lastDirectories(lastDirectoriesStore);
  infra::MemoryDroppedFilesService droppedFiles;

  const platform::SystemDialogs systemDialogs([&mainWindow]() -> void* { return mainWindow; });
  infra::RememberingFileDialog fileDialog(systemDialogs, lastDirectories);
  infra::ExplorerDirectory logDirectory(userData / "logs");
  infra::ExplorerDirectory biosDirectory(userData / "bios");
  infra::UnsupportedAppUpdater updater;
  infra::UnavailableHelpViewer help;

  const ResourceLocations resources = locateResources(platform::executableDirectory());
  infra::LatestFrameSlot frames(ui);
  infra::InProcessCoreGateway core(infra::InProcessCoreOptions{platform::pathToUtf8(userData), EMBER_APP_VERSION, resources.plugins, resources.bios, resources.ffmpeg, launch.debugPort, launch.debugToken}, ui, frames, logger);

  entry::MainRequestRouter router(core, logger);
  router.add(entry::createCartridgeHandlers({core, fileDialog, droppedFiles, recentCartridges}));
  router.add(entry::createMachineHandlers({core, fileDialog, biosDirectory}));
  router.add(entry::createMapperHandlers({core, fileDialog}));
  router.add(entry::createMediaHandlers({core, fileDialog, droppedFiles}));
  router.add(entry::createPreferenceHandlers({fileDialog}));
  router.add(entry::createSystemHandlers({logDirectory, help, updater}));

  try {
    // 运行与外壳模块：先于主窗口创建，主窗口里的控件销毁时它们还在；画面宿主需要窗口句柄，窗口创建后再创建
    logic::InputController input(core);
    logic::RuntimeController runtime(router, core, shellState);
    ForwardingWindowControl windowControl;
    ForwardingNotifier notifier;
    NoDialogs dialogs;
    logic::ShellController shell(logic::buildShellRegistry({logic::createShellModule(), logic::createRuntimeModule(runtime)}), windowControl, dialogs, notifier, shellState, droppedFiles);
    std::optional<infra::ScreenHostAdapter> screenHost;
    platform::WindowModes* modes = nullptr;
    ui::ShellView* shellView = nullptr;
    ui::ScreenWidget* screenWidget = nullptr;

    // 主窗口：按系统主题创建，读到首选项里的主题后再切换
    entry::ThemeSource themeSource = entry::ThemeSource::System;
    rujian::ui::HostWindow window(rujian::ui::HostWindow::Options{kWindowTitle, rujian::ui::Size{kWindowWidth, kWindowHeight}, resolveTheme(themeSource)});
    mainWindow = static_cast<HWND>(window.nativeHandle());

    auto content = std::make_unique<ui::MainWindowView>(kAppName);
    ui::MainWindowView* view = content.get();
    window.root().setContent(std::move(content));
    view->setBorderVisible(!platform::FramelessFrame::compositionEnabled());

    const auto applyTheme = [&] { window.root().setTheme(resolveTheme(themeSource)); };

    bool trackMoves = false;
    platform::FramelessCallbacks callbacks;
    callbacks.hitTest = [view](double x, double y) { return toFrameHit(view->hitTestWindow(rujian::ui::Point{x, y})); };
    callbacks.systemButtonState = [view](platform::FrameHit hover, platform::FrameHit pressed) { view->setSystemButtonState(toTitleBarHit(hover), toTitleBarHit(pressed)); };
    callbacks.maximizedChanged = [view](bool maximized) { view->setMaximized(maximized); };
    callbacks.activeChanged = [view, &screenWidget](bool active) {
      view->setWindowActive(active);
      if (screenWidget != nullptr) screenWidget->setWindowActive(active);
    };
    // 开始的摆放不算用户移动窗口，显示之后的位置变化才保存
    callbacks.moved = [&shellState, &trackMoves](int x, int y) {
      if (trackMoves) shellState.updateWindow(shared::WindowPlacement{x, y});
    };
    callbacks.systemThemeChanged = [&] {
      if (themeSource == entry::ThemeSource::System) applyTheme();
    };
    callbacks.compositionChanged = [view](bool enabled) { view->setBorderVisible(!enabled); };
    platform::FramelessFrame frame(mainWindow, platform::FramelessOptions{kWindowWidth, kWindowHeight, [&window] { return window.root().scale(); }}, std::move(callbacks));
    window.setMessageHook([&](unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result) {
      if (frame.handle(message, wParam, lParam, result)) return true;
      if (modes != nullptr && modes->handle(message, wParam, lParam, result)) return true;
      if (screenHost && screenHost->handleMessage(message, wParam, lParam, result)) return true;
      if (shellView == nullptr) return false;
      switch (message) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
          // 快捷键与 Alt+助记字母；已处理的按键不再交给控件
          if (shellView->handleKeyDown(static_cast<int>(wParam), currentModifiers(), (lParam & 0x40000000) != 0)) {
            result = 0;
            return true;
          }
          return false;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: {
          const double pointScale = window.root().scale();
          shellView->handleMouseDown(rujian::ui::Point{GET_X_LPARAM(lParam) / pointScale, GET_Y_LPARAM(lParam) / pointScale});
          return false;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
          shellView->handleMouseUp();
          return false;
        default:
          return false;
      }
    });
    frame.attach();

    const double scale = window.root().scale();
    platform::placeWindow(mainWindow, static_cast<int>(std::lround(kWindowWidth * scale)), static_cast<int>(std::lround(kWindowHeight * scale)), shellState.get().window);

    // 窗口模式（全屏、置顶、拖放）与运行画面宿主
    windowControl.onQuit = [window = mainWindow] { PostMessageW(window, WM_CLOSE, 0, 0); };
    std::optional<platform::WindowModes> windowModes;
    windowModes.emplace(
        mainWindow, frame,
        [&] {
          shell.setWindowState(windowModes->fullScreen(), windowModes->alwaysOnTop());
          if (shellView != nullptr) shellView->setFullScreen(windowModes->fullScreen());
        },
        [&shell](std::vector<std::string> paths) { shell.dropFiles(paths); });
    modes = &*windowModes;
    windowControl.target = modes;
    try {
      screenHost.emplace(mainWindow, frames, input, ui, [&logger](const std::string& message) { logger.write({std::nullopt, kMainErrorCode, message, nlohmann::json{{"source", "screenHost"}}}); });
    } catch (const std::runtime_error& error) {
      logger.write({std::nullopt, kMainErrorCode, error.what(), nlohmann::json{{"source", "screenHost"}}});
      platform::showErrorMessage("Ember", "无法初始化画面渲染，当前电脑不支持。\n需要支持 Direct3D 11 的显卡与驱动；没有显卡、没有驱动或没有 3D 加速的虚拟机不在支持范围内。");
      core.stop();
      return 1;
    }

    // 外壳界面：菜单、系统栏、底部面板与通知，显示区是运行画面，面板内容由这里按标识注入
    std::optional<ui::ShellView> shellViewStorage;
    shellViewStorage.emplace(window.root(), *view, shell, [&ui](std::function<void()> task) { ui.post(std::move(task)); });
    shellView = &*shellViewStorage;
    notifier.target = shellView;
    auto screen = std::make_unique<ui::ScreenWidget>(runtime, *screenHost, input);
    screenWidget = screen.get();
    shellView->setScreen(std::move(screen));
    shellView->setPanelContent(logic::kRuntimePanelId, [&runtime] { return std::make_unique<ui::RuntimePanelContent>(runtime); });
    shellView->setFocusScreen([screenWidget] { screenWidget->focusScreen(); });
    // 弹层（菜单、通知、提示）盖到画面上时把画面子窗口对应的区域挖掉
    window.root().setLayerRectsHandler([&](const std::vector<rujian::ui::Rect>& rects) {
      const double layerScale = window.root().scale();
      std::vector<shared::ScreenRect> holes;
      for (const auto& rect : rects) {
        const int left = static_cast<int>(std::floor(rect.x * layerScale));
        const int top = static_cast<int>(std::floor(rect.y * layerScale));
        holes.push_back(shared::ScreenRect{left, top, static_cast<int>(std::ceil(rect.right() * layerScale)) - left, static_cast<int>(std::ceil(rect.bottom() * layerScale)) - top});
      }
      screenHost->setHoles(holes);
      shellView->setLayerRects(rects);
    });
    shell.setWindowState(false, windowModes->alwaysOnTop());
    shell.attachModules();
    shellView->refresh();

    bool shown = false;
    unsigned showTimer = 0;
    const auto showWindow = [&] {
      if (shown) return;
      shown = true;
      if (showTimer != 0) ui.cancelDelayed(showTimer);
      window.show();
      trackMoves = true;
    };
    showTimer = ui.postDelayed(kThemeWaitMilliseconds, [&] {
      showTimer = 0;
      showWindow();
    });

    entry::ThemeSync themeSync(
        core,
        [&](entry::ThemeSource source) {
          themeSource = source;
          applyTheme();
        },
        [&logger](const std::string& message) { logger.write({std::nullopt, kMainErrorCode, message, nlohmann::json{{"source", "themeSync"}}}); }, showWindow);

    core.start();
    const auto diagnosticsPath = platform::environmentVariable("EMBER_WIN_DIAGNOSTICS");
    const auto diagnosticsStart = std::chrono::steady_clock::now();
    if (diagnosticsPath) screenHost->enableLatencyRecording();
    // 命令行带 --cartridge=路径（.nes）时，核心就绪后加载这张卡带；就绪之前发的请求排队
    if (launch.cartridgePath) {
      entry::loadAndRememberCartridge(core, recentCartridges, *launch.cartridgePath, [&logger](contracts::CoreReply reply) {
        if (!reply.ok) logger.write({std::nullopt, kMainErrorCode, "启动参数指定的卡带加载失败：" + reply.error.code, nlohmann::json{{"source", "launchCartridge"}}});
      });
    }

    rujian::ui::HostWindow::runMessageLoop();

    // 开发期诊断：环境变量 EMBER_WIN_DIAGNOSTICS 给出文件路径时，退出时写出帧计数与画面延迟
    if (diagnosticsPath) {
      const auto stats = frames.stats();
      const auto latency = screenHost->latency();
      const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - diagnosticsStart).count();
      const nlohmann::json report{{"seconds", seconds}, {"published", stats.published}, {"consumed", stats.consumed}, {"dropped", stats.dropped}, {"latency", {{"count", latency.count}, {"averageMs", latency.average}, {"p95Ms", latency.p95}, {"maxMs", latency.max}}}};
      std::ofstream(platform::pathFromUtf8(*diagnosticsPath), std::ios::binary | std::ios::trunc) << report.dump(2);
    }
    // 退出：先断开各模块的常驻部分，再保存界面状态、停止核心
    shell.detachModules();
    shellState.flush();
    recentCartridges.flush();
    lastDirectories.flush();
    core.stop();
    mainWindow = nullptr;
  } catch (const std::exception& error) {
    logger.write({std::nullopt, kMainErrorCode, std::string("主窗口出错：") + error.what(), nlohmann::json{{"source", "mainWindow"}}});
    platform::showErrorMessage("Ember", std::string("无法启动：") + error.what());
    core.stop();
    return 1;
  }
  return 0;
}

}  // namespace ember::win32::bootstrap
