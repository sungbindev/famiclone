/*
 * ------------------------------------------------------------------------
 * 名称：input.cpp
 * 说明：输入注入模块的登记：创建输入注入服务，登记 input.* 请求。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "bootstrap/modules/input.hpp"

#include <memory>
#include <string>
#include <variant>

#include "contracts/input/input-binding-service.hpp"
#include "contracts/input/input-service.hpp"
#include "contracts/input/input-settings.hpp"
#include "contracts/preferences/preference-service.hpp"
#include "contracts/runtime/runtime-service.hpp"
#include "contracts/runtime/runtime-settings.hpp"
#include "infra/input/input-binding-registry.hpp"
#include "logic/input/local-input-service.hpp"
#include "logic/input/pad-input-router.hpp"
#include "requests/input/input-requests.hpp"

namespace ember::bootstrap {

namespace {

void registerInputSettings(contracts::preferences::PreferenceService& service) {
  namespace setting = contracts::input::setting;
  service.addGroup({std::string(setting::SETTING_GROUP), "输入"});
  service.addSetting({.id = std::string(setting::SETTING_CAPTURE_MOUSE),
                      .group = std::string(setting::SETTING_GROUP),
                      .label = "点击画面捕获鼠标",
                      .description = "开启后点击画面会捕获鼠标：指针隐藏，鼠标移动以相对位移送给学习机，按 Esc 退出捕获；关闭时指针停在画面上。手柄的键位在“设置”菜单的“手柄设置”里设置。",
                      .kind = contracts::preferences::SettingKind::Switch,
                      .defaultValue = true});
  service.addSetting({.id = std::string(setting::SETTING_FAMILY_KEYBOARD),
                      .group = std::string(setting::SETTING_GROUP),
                      .label = "接入 Family BASIC 键盘",
                      .description = "给经典 FC 接上 Family BASIC 键盘：接上后电脑键盘的按键按它的键位送给机器，不再当作手柄按键；学习机用自己内置的键盘，不受影响。",
                      .kind = contracts::preferences::SettingKind::Switch,
                      .defaultValue = false});
}

}  // namespace

void registerInput(ModuleRegistry& registry) {
  auto preferences = registry.services.require<contracts::preferences::PreferenceService>();
  registerInputSettings(*preferences);
  auto runtime = registry.services.require<contracts::runtime::RuntimeService>();
  // Family BASIC 键盘：启动时按已保存的取值设置，之后跟随首选项的变化。
  const auto isOn = [](const contracts::preferences::SettingValue& value) { return std::get_if<bool>(&value) != nullptr && std::get<bool>(value); };
  runtime->setFamilyKeyboard(isOn(preferences->value(std::string(contracts::input::setting::SETTING_FAMILY_KEYBOARD))));
  preferences->subscribe([runtime, isOn](const std::string& id, const contracts::preferences::SettingValue& value) {
    if (id == contracts::input::setting::SETTING_FAMILY_KEYBOARD) runtime->setFamilyKeyboard(isOn(value));
  });
  // 开机与复位后遮黑最初的几帧：这里同时取用首选项与运行服务，运行模块不能反过来依赖首选项（会形成模块循环）。
  preferences->addSetting({.id = std::string(contracts::runtime::setting::SETTING_HIDE_STARTUP_FRAMES),
                           .group = "display",
                           .label = "开机与复位后隐藏最初的画面",
                           .description = "开启后，开机和复位后最初约 0.1 秒显示为黑色，避免闪过灰色、绿色或花屏；只影响显示，不影响模拟和调试截图。",
                           .kind = contracts::preferences::SettingKind::Switch,
                           .defaultValue = false});
  runtime->setHideStartupFrames(isOn(preferences->value(std::string(contracts::runtime::setting::SETTING_HIDE_STARTUP_FRAMES))));
  preferences->subscribe([runtime, isOn](const std::string& id, const contracts::preferences::SettingValue& value) {
    if (id == contracts::runtime::setting::SETTING_HIDE_STARTUP_FRAMES) runtime->setHideStartupFrames(isOn(value));
  });
  // 手柄绑定：服务保存绑定，路由器作为运行服务的输入钩子，把手柄与键盘按键换成手柄口的按键。
  infra::input::InputBindingRegistryOptions bindingOptions;
  bindingOptions.file = registry.userDataPath / "config" / "input-bindings.json";
  bindingOptions.events = registry.events;
  bindingOptions.errors = registry.errors;
  auto bindings = std::make_shared<infra::input::InputBindingRegistry>(std::move(bindingOptions));
  registry.services.provide<contracts::input::InputBindingService>(bindings);
  runtime->setInputHook(std::make_shared<logic::input::PadInputRouter>(bindings));
  auto service = std::make_shared<logic::input::LocalInputService>(std::move(runtime));
  registry.services.provide<contracts::input::InputService>(service);
  requests::input::addInputRequests(registry.requests, service, bindings);
}

}  // namespace ember::bootstrap
