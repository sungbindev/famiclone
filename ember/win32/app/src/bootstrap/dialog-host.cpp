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

#include "platform/text/wide-text.hpp"
#include "rujian/ui/dialog-frame.hpp"

namespace ember::win32::bootstrap {

DialogHost::DialogHost(std::function<void*()> owner, std::function<void(std::function<void()>)> post, bool customFrame) : owner_(std::move(owner)), post_(std::move(post)), customFrame_(customFrame) {}

DialogHost::~DialogHost() { *alive_ = false; }

void DialogHost::setDefinitions(std::map<std::string, contracts::ShellDialogDefinition> definitions) { definitions_ = std::move(definitions); }

void DialogHost::setBody(const std::string& dialogId, BodyFactory factory, bool fills) {
  bodies_[dialogId] = std::move(factory);
  if (fills) fillingBodies_.insert(dialogId);
  else fillingBodies_.erase(dialogId);
}

void DialogHost::setKeyCapture(const std::string& dialogId, std::function<bool(const std::string& code)> capture) { keyCaptures_[dialogId] = std::move(capture); }

void DialogHost::setShortcutCapture(const std::string& dialogId, std::function<bool(int virtualKey, unsigned modifiers)> capture) { shortcutCaptures_[dialogId] = std::move(capture); }

void DialogHost::setTheme(rujian::ui::ThemeName theme) {
  theme_ = theme;
  for (auto& entry : open_) entry.second->setTheme(theme);
}

void DialogHost::openDialog(const std::string& dialogId) {
  const auto existing = open_.find(dialogId);
  if (existing != open_.end()) {
    existing->second->activate();
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
  if (logo_) frame->setLogo(logo_());
  frame->setContent(body->second());

  platform::DialogWindowOptions options;
  options.owner = owner_();
  options.title = platform::widen(definition->second.title);
  options.width = definition->second.width;
  options.height = definition->second.height;
  options.minimumWidth = definition->second.minWidth;
  options.minimumHeight = definition->second.minHeight;
  options.fitMinimumWidth = definition->second.fitMinWidth;
  options.fitMinimumHeight = definition->second.fitMinHeight;
  options.maximizable = true;
  options.minimizable = true;
  options.theme = theme_;
  options.customFrame = customFrame_;
  options.onCloseRequested = [this, dialogId] { closeDialog(dialogId); };
  if (const auto capture = keyCaptures_.find(dialogId); capture != keyCaptures_.end()) options.onPhysicalKey = capture->second;
  if (const auto capture = shortcutCaptures_.find(dialogId); capture != shortcutCaptures_.end()) options.onShortcutKey = capture->second;
  auto window = std::make_unique<platform::DialogWindow>(std::move(options), std::move(frame));
  platform::DialogWindow* raw = window.get();
  open_.emplace(dialogId, std::move(window));
  raw->show();
}

void DialogHost::closeDialog(const std::string& dialogId) {
  // 请求来自窗口自己的消息处理，延后到处理完再销毁窗口
  const auto alive = alive_;
  post_([this, alive, dialogId] {
    if (*alive) open_.erase(dialogId);
  });
}

void DialogHost::closeAll() { open_.clear(); }

}  // namespace ember::win32::bootstrap
