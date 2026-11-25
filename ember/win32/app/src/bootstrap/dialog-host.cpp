/*
 * ------------------------------------------------------------------------
 * 名称：dialog-host.cpp
 * 说明：对话框的打开者的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/dialog-host.hpp"

#include <windows.h>

#include "platform/text/wide-text.hpp"
#include "rujian/ui/dialog-frame.hpp"

namespace ember::win32::bootstrap {

DialogHost::DialogHost(std::function<void*()> owner, std::function<void(std::function<void()>)> post, bool customFrame) : owner_(std::move(owner)), post_(std::move(post)), customFrame_(customFrame) {}

DialogHost::~DialogHost() { *alive_ = false; }

void DialogHost::setDefinitions(std::map<std::string, contracts::ShellDialogDefinition> definitions) { definitions_ = std::move(definitions); }

void DialogHost::setBody(const std::string& dialogId, BodyFactory factory, bool fills, bool flushVertical) {
  bodies_[dialogId] = std::move(factory);
  if (fills) fillingBodies_.insert(dialogId);
  else fillingBodies_.erase(dialogId);
  if (flushVertical) flushVerticalBodies_.insert(dialogId);
  else flushVerticalBodies_.erase(dialogId);
}

void DialogHost::setKeyCapture(const std::string& dialogId, std::function<bool(const std::string& code)> capture) { keyCaptures_[dialogId] = std::move(capture); }

void DialogHost::setShortcutCapture(const std::string& dialogId, std::function<bool(int virtualKey, unsigned modifiers)> capture) { shortcutCaptures_[dialogId] = std::move(capture); }

void DialogHost::setOpenHook(const std::string& dialogId, OpenHook hook) { openHooks_[dialogId] = std::move(hook); }

void DialogHost::setTheme(rujian::ui::ThemeName theme) {
  theme_ = theme;
  for (auto& entry : open_) entry.second->setTheme(theme);
}

void DialogHost::openDialog(const std::string& dialogId) { openWindow(dialogId, true); }

void DialogHost::openDialogInBackground(const std::string& dialogId) { openWindow(dialogId, false); }

void DialogHost::openWindow(const std::string& dialogId, bool activate) {
  const auto existing = open_.find(dialogId);
  if (existing != open_.end()) {
    if (activate) existing->second->activate();
    return;
  }
  const auto definition = definitions_.find(dialogId);
  const auto body = bodies_.find(dialogId);
  if (definition == definitions_.end() || body == bodies_.end()) return;

  auto frame = std::make_unique<rujian::ui::DialogFrame>();
  frame->setTitle(definition->second.title);
  frame->setMaximizable(true);
  frame->setContentFill(fillingBodies_.count(dialogId) > 0);
  // 铺满的正文自己负责滚动，滚动条要贴到对话框的右边
  frame->setContentFlushRight(true);
  frame->setContentFlushVertical(flushVerticalBodies_.count(dialogId) > 0);
  if (logo_) frame->setLogo(logo_());
  std::unique_ptr<rujian::ui::Widget> content = body->second();
  rujian::ui::Widget* contentPointer = content.get();
  frame->setContent(std::move(content));

  platform::DialogWindowOptions options;
  void* const ownerWindow = owner_();
  // 停靠的对话框跟着主窗口走，没有最小化与最大化按钮
  const bool docking = definition->second.dockLeft && definition->second.modeless && ownerWindow != nullptr;
  options.owner = ownerWindow;
  options.title = platform::widen(definition->second.title);
  options.width = definition->second.width;
  options.height = definition->second.height;
  options.minimumWidth = definition->second.minWidth;
  options.minimumHeight = definition->second.minHeight;
  options.fitMinimumWidth = definition->second.fitMinWidth;
  options.fitMinimumHeight = definition->second.fitMinHeight;
  options.maximizable = !docking;
  options.minimizable = !docking;
  options.modeless = definition->second.modeless;
  options.theme = theme_;
  options.customFrame = customFrame_;
  options.onCloseRequested = [this, dialogId] { closeDialog(dialogId); };
  if (const auto capture = keyCaptures_.find(dialogId); capture != keyCaptures_.end()) options.onPhysicalKey = capture->second;
  if (const auto capture = shortcutCaptures_.find(dialogId); capture != shortcutCaptures_.end()) options.onShortcutKey = capture->second;
  auto window = std::make_unique<platform::DialogWindow>(std::move(options), std::move(frame));
  platform::DialogWindow* raw = window.get();
  open_.emplace(dialogId, std::move(window));
  if (docking) {
    // 在窗口显示之前就贴到主窗口左边，不会先出现在别处
    auto dock = std::make_unique<platform::WindowDock>(ownerWindow, raw->handle(), [raw] { return raw->root().scale(); });
    platform::WindowDock* dockPointer = dock.get();
    dock->dockNow(true);
    raw->setMessageObserver([dockPointer](unsigned message, std::uintptr_t wParam, std::intptr_t lParam, std::intptr_t& result) { return dockPointer->followerMessage(message, wParam, lParam, result); });
    raw->setWidthResizer([dockPointer](int width) { return dockPointer->resizeFollowerWidth(width); });
    docks_[dialogId] = std::move(dock);
  }
  raw->show();
  // 后台打开：显示会激活新窗口，把焦点还给主窗口
  if (!activate && ownerWindow != nullptr) SetForegroundWindow(static_cast<HWND>(ownerWindow));
  if (const auto hook = openHooks_.find(dialogId); hook != openHooks_.end()) attachments_[dialogId] = hook->second(*raw, *contentPointer);
  if (definition->second.modeless && activate) {
    // 非模态对话框没有归属窗口，打开它的菜单关闭后主窗口会重新取得焦点并盖住它：等当前消息处理完再把它带到最前面
    const auto alive = alive_;
    post_([this, alive, dialogId] {
      if (!*alive) return;
      const auto found = open_.find(dialogId);
      if (found != open_.end()) found->second->activate();
    });
  }
  if (onOpenChanged_) onOpenChanged_(dialogId, true);
}

void DialogHost::closeDialog(const std::string& dialogId) {
  // 请求来自窗口自己的消息处理，延后到处理完再销毁窗口
  const auto alive = alive_;
  post_([this, alive, dialogId] {
    if (!*alive) return;
    // 窗口句柄上挂靠的资源先于窗口销毁
    attachments_.erase(dialogId);
    releaseDock(dialogId);
    const bool wasOpen = open_.erase(dialogId) != 0;
    if (wasOpen && onOpenChanged_) onOpenChanged_(dialogId, false);
  });
}

void DialogHost::releaseDock(const std::string& dialogId) {
  const auto dock = docks_.find(dialogId);
  if (dock == docks_.end()) return;
  // 窗口销毁期间还会收到消息，先清掉指向停靠对象的回调
  if (const auto window = open_.find(dialogId); window != open_.end()) {
    window->second->setMessageObserver({});
    window->second->setWidthResizer({});
  }
  docks_.erase(dock);
}

void DialogHost::ownerWindowChanged() {
  for (auto& entry : docks_) entry.second->leaderChanged();
}

void DialogHost::closeAll() {
  attachments_.clear();
  while (!docks_.empty()) releaseDock(docks_.begin()->first);
  open_.clear();
}

}  // namespace ember::win32::bootstrap
