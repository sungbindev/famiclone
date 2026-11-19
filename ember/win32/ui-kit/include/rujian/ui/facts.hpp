/*
 * ------------------------------------------------------------------------
 * 名称：facts.hpp
 * 说明：键值清单：一行一条，左边是名称，右边是内容；窗口很窄时名称与内容上下排列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：对应组件库的 Facts。内容可以是文字（换行符分行），也可以是任意控件（如链接按钮）。
 *       窄窗口的判定与网页版的媒体查询一致：窗口宽度不超过 420 逻辑像素。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rujian/ui/widget.hpp"

namespace rujian::ui {

/** 键值清单。 */
class Facts : public Widget {
 public:
  /** 加一条，内容是文字。 */
  void addFact(std::string label, std::string value);
  /** 加一条，内容是控件。 */
  void addFact(std::string label, std::unique_ptr<Widget> value);

  [[nodiscard]] Size measure(Size available) override;
  void arrange(const Rect& bounds) override;

 private:
  /** 一条的名称与内容控件。 */
  struct Fact {
    Widget* label;
    Widget* value;
  };

  /** 按可用宽度算出的布局：每条的名称高、内容高。 */
  struct Layout {
    bool narrow{false};
    double valueWidth{0};
    std::vector<double> labelHeights;
    std::vector<double> valueHeights;
    double height{0};
  };

  [[nodiscard]] Layout layout(double width);
  [[nodiscard]] bool narrow() const;

  std::vector<Fact> facts_;
};

}  // namespace rujian::ui
