/*
 * ------------------------------------------------------------------------
 * 名称：dialog-confirmation.cpp
 * 说明：主题对话框确认询问的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-26
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/dialog-confirmation.hpp"

#include <windows.h>

#include "platform/text/wide-text.hpp"
#include "platform/window/dialog-window.hpp"
#include "rujian/ui/dialog-frame.hpp"
#include "rujian/ui/label.hpp"

namespace ember::win32::bootstrap {

namespace {

/** 用户可见的文字。 */
constexpr const char* kYes = "是";
constexpr const char* kNo = "否";
constexpr const char* kYesId = "yes";
constexpr const char* kNoId = "no";
/** 询问窗口的内容宽度与最小宽度（逻辑像素）。 */
constexpr double kWidth = 380.0;
constexpr double kMinimumWidth = 280.0;

}  // namespace

DialogConfirmation::DialogConfirmation(std::function<void*()> fallbackOwner) : fallbackOwner_(std::move(fallbackOwner)) {}

bool DialogConfirmation::ask(const std::string& title, const std::string& text) {
  bool done = false;
  bool answer = false;
  const auto finish = [&done, &answer](bool yes) {
    answer = yes;
    done = true;
  };

  auto frame = std::make_unique<rujian::ui::DialogFrame>();
  frame->setTitle(title);
  if (logo_) frame->setLogo(logo_());
  frame->setContent(std::make_unique<rujian::ui::Label>(text));
  frame->setButtons({rujian::ui::DialogButton{.id = kYesId, .text = kYes}, rujian::ui::DialogButton{.id = kNoId, .text = kNo, .isDefault = true, .isCancel = true}});
  frame->setOnButton([&finish](const std::string& id) { finish(id == kYesId); });

  platform::DialogWindowOptions options;
  HWND owner = GetActiveWindow();
  if (owner == nullptr && fallbackOwner_) owner = static_cast<HWND>(fallbackOwner_());
  options.owner = owner;
  options.title = platform::widen(title);
  options.width = kWidth;
  options.minimumWidth = kMinimumWidth;
  options.theme = theme_;
  options.customFrame = customFrame_;
  options.onCloseRequested = [&finish] { finish(false); };
  platform::DialogWindow window(std::move(options), std::move(frame));
  window.show();

  // 嵌套的消息循环：回答之前不返回；收到退出消息时把它重新投递给外层循环
  MSG message;
  while (!done) {
    const BOOL received = GetMessageW(&message, nullptr, 0, 0);
    if (received == 0) {
      PostQuitMessage(static_cast<int>(message.wParam));
      break;
    }
    if (received < 0) break;
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  return answer;
}

}  // namespace ember::win32::bootstrap
