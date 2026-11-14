/*
 * ------------------------------------------------------------------------
 * 名称：input-requests.cpp
 * 说明：input 模块的请求登记实现：校验参数，转成硬件契约的输入事件，交给输入注入服务。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include "requests/input/input-requests.hpp"

#include "contracts/hardware/input-event.hpp"
#include "contracts/input/generated/input.hpp"

namespace ember::requests::input {

namespace contract = ::ember::contracts::input;
namespace hw = ::ember::contracts::hardware;

namespace {

hw::MouseKind mouseKindOf(const std::string& name) {
  if (name == "down") return hw::MouseKind::Down;
  if (name == "up") return hw::MouseKind::Up;
  if (name == "wheel") return hw::MouseKind::Wheel;
  if (name == "leave") return hw::MouseKind::Leave;
  return hw::MouseKind::Move;
}

}  // namespace

void addInputRequests(platform::RequestRouter& router, std::shared_ptr<contract::InputService> service) {
  router.add("input", contract::INPUT_KEY_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInputKeyParams(params, "params");
    service->key(parsed.code, parsed.down);
    return nlohmann::json(contract::InputKeyResult{});
  });
  router.add("input", contract::INPUT_MOUSE_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInputMouseParams(params, "params");
    hw::MouseInput event;
    event.kind = mouseKindOf(parsed.kind);
    event.x = parsed.x.value_or(0);
    event.y = parsed.y.value_or(0);
    event.dx = parsed.dx.value_or(0);
    event.dy = parsed.dy.value_or(0);
    event.buttons = static_cast<std::uint8_t>(parsed.buttons.value_or(0));
    event.wheel = parsed.wheel.value_or(0);
    service->mouse(event);
    return nlohmann::json(contract::InputMouseResult{});
  });
  router.add("input", contract::INPUT_GAMEPAD_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInputGamepadParams(params, "params");
    hw::GamepadInput event;
    event.index = static_cast<std::uint8_t>(parsed.index);
    event.connected = parsed.connected;
    if (parsed.connected) {
      event.buttons = parsed.buttons.value_or(std::vector<bool>{});
      event.axes = parsed.axes.value_or(std::vector<double>{});
    }
    service->gamepad(event);
    return nlohmann::json(contract::InputGamepadResult{});
  });
  router.add("input", contract::INPUT_PAD_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    const auto parsed = contract::parseInputPadParams(params, "params");
    service->pad(static_cast<unsigned>(parsed.port), static_cast<std::uint8_t>(parsed.buttons));
    return nlohmann::json(contract::InputPadResult{});
  });
  router.add("input", contract::INPUT_RELEASE_ALL_REQUEST, [service](const nlohmann::json& params, const platform::CancellationToken&) {
    static_cast<void>(contract::parseInputReleaseAllParams(params, "params"));
    service->releaseAll();
    return nlohmann::json(contract::InputReleaseAllResult{});
  });
}

}  // namespace ember::requests::input
