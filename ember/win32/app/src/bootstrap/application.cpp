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

#include <cmath>
#include <memory>
#include <optional>
#include <string>

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
#include "infra/dropped-files/memory-dropped-files-service.hpp"
#include "infra/help/unavailable-help-viewer.hpp"
#include "infra/last-directories/file-last-directories-service.hpp"
#include "infra/logging/file-error-logger-adapter.hpp"
#include "infra/recent-cartridges/file-recent-cartridges-service.hpp"
#include "infra/shell-state/file-shell-state-service.hpp"
#include "infra/updates/unsupported-app-updater.hpp"
#include "platform/dialogs/message-box.hpp"
#include "platform/dialogs/system-dialogs.hpp"
#include "platform/files/json-file-reader.hpp"
#include "platform/files/json-file-store.hpp"
#include "platform/files/known-folders.hpp"
#include "platform/logging/file-error-logger.hpp"
#include "platform/process/single-instance.hpp"
#include "platform/text/wide-text.hpp"
#include "platform/theme/system-theme.hpp"
#include "platform/threading/ui-thread.hpp"
#include "platform/window/frameless-frame.hpp"
#include "platform/window/window-placement.hpp"
#include "rujian/ui/window.hpp"
#include "shared/preferences/allow-multiple-instances.hpp"
#include "ui/shell/main-window-view.hpp"

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
    callbacks.activeChanged = [view](bool active) { view->setWindowActive(active); };
    // 开始的摆放不算用户移动窗口，显示之后的位置变化才保存
    callbacks.moved = [&shellState, &trackMoves](int x, int y) {
      if (trackMoves) shellState.updateWindow(shared::WindowPlacement{x, y});
    };
    callbacks.systemThemeChanged = [&] {
      if (themeSource == entry::ThemeSource::System) applyTheme();
    };
    callbacks.compositionChanged = [view](bool enabled) { view->setBorderVisible(!enabled); };
    platform::FramelessFrame frame(mainWindow, platform::FramelessOptions{kWindowWidth, kWindowHeight, [&window] { return window.root().scale(); }}, std::move(callbacks));
    window.setMessageHook([&frame](unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result) { return frame.handle(message, wParam, lParam, result); });
    frame.attach();

    const double scale = window.root().scale();
    platform::placeWindow(mainWindow, static_cast<int>(std::lround(kWindowWidth * scale)), static_cast<int>(std::lround(kWindowHeight * scale)), shellState.get().window);

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
    // 命令行带 --cartridge=路径（.nes）时，核心就绪后加载这张卡带；就绪之前发的请求排队
    if (launch.cartridgePath) {
      entry::loadAndRememberCartridge(core, recentCartridges, *launch.cartridgePath, [&logger](contracts::CoreReply reply) {
        if (!reply.ok) logger.write({std::nullopt, kMainErrorCode, "启动参数指定的卡带加载失败：" + reply.error.code, nlohmann::json{{"source", "launchCartridge"}}});
      });
    }

    rujian::ui::HostWindow::runMessageLoop();

    // 退出：保存界面状态，再停止核心
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
