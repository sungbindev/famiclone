/*
 * ------------------------------------------------------------------------
 * 名称：pad-layout.cpp
 * 说明：手柄图版面的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/input/pad-layout.hpp"

namespace ember::win32::ui {

rujian::ui::PadDiagramLayout padLayout() {
  rujian::ui::PadDiagramLayout layout;
  layout.canvasWidth = 560;
  layout.canvasHeight = 220;
  layout.body = rujian::ui::Rect{4, 4, 552, 212};
  layout.bodyRadius = 36;
  layout.crossCenter = rujian::ui::Rect{80, 88, 60, 44};
  layout.keys = {
      {"up", false, 110, 66, 60, 44},     {"down", false, 110, 154, 60, 44},  {"left", false, 50, 110, 60, 44},   {"right", false, 170, 110, 60, 44}, {"select", false, 232, 168, 84, 34},
      {"start", false, 328, 168, 84, 34}, {"turboB", true, 420, 60, 64, 64},  {"turboA", true, 500, 60, 64, 64},  {"b", true, 420, 140, 64, 64},      {"a", true, 500, 140, 64, 64}};
  return layout;
}

}  // namespace ember::win32::ui
