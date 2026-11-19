/*
 * ------------------------------------------------------------------------
 * 名称：text-engine.hpp
 * 说明：文字排版引擎：用 DirectWrite 把文字按字符分段选字体，统一行高，生成排版对象与度量。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：行高与 CSS 一致：行高 = 字号 × 倍数，字体的上升与下降之和在行高里上下居中（半行距）；
 *       基线位置按主字体（Segoe UI）计算，与网页版对齐的校准量见 kBaselineShift。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <dwrite.h>
#include <wrl/client.h>

#include <map>
#include <string>
#include <string_view>
#include <utility>

#include "base/font-roles.hpp"
#include "rujian/ui/text.hpp"

namespace rujian::ui {

/** 文字排版引擎，单线程使用。 */
class TextEngine {
 public:
  /** @param factory DirectWrite 工厂，必须在引擎的生命周期内有效。 */
  explicit TextEngine(IDWriteFactory* factory);

  /**
   * 创建排版对象。
   * @param utf8 UTF-8 文字。
   * @param style 样式。
   * @param maxWidth 排版宽度（逻辑像素，不限宽度传一个很大的值）。
   * @param maxHeight 排版高度。
   * @return 排版对象；失败时为空。
   */
  [[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextLayout> createLayout(std::string_view utf8, const TextStyle& style, double maxWidth, double maxHeight);

  /** 度量文字；maxWidth 为 0 表示不限宽度。 */
  [[nodiscard]] TextMetrics measure(std::string_view utf8, const TextStyle& style, double maxWidth);

 private:
  /** 字体的纵向度量，都是占字号的比例。 */
  struct VerticalMetrics {
    double ascent;
    double descent;
    double lineGap;
  };

  /** 字体类别对应的实际字体族名。 */
  [[nodiscard]] const std::wstring& familyFor(FontRole role);
  /** 主字体的纵向度量。 */
  [[nodiscard]] VerticalMetrics verticalMetrics(const std::wstring& family, int weight);
  [[nodiscard]] bool hasFamily(const wchar_t* name) const;

  IDWriteFactory* factory_;
  Microsoft::WRL::ComPtr<IDWriteFontCollection> collection_;
  std::wstring families_[3];
  std::map<std::pair<std::wstring, int>, VerticalMetrics> metricsCache_;
};

}  // namespace rujian::ui
