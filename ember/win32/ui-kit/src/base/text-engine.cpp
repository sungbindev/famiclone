/*
 * ------------------------------------------------------------------------
 * 名称：text-engine.cpp
 * 说明：文字排版引擎的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：Windows 7 的 DirectWrite 没有自动换字体，所以按 fontRoleOf 把文字切成段，每段单独指定字体族。
 * ------------------------------------------------------------------------
 */

#include "base/text-engine.hpp"

#include <windows.h>

#include <algorithm>
#include <vector>

namespace rujian::ui {

namespace {

/** 网页版与原生版的基线校准量（逻辑像素），与网页版截图对照后确定。 */
constexpr double kBaselineShift = 0.0;

/** 不限宽度时使用的排版宽度。 */
constexpr double kUnlimitedWidth = 100000.0;

/** UTF-8 转 UTF-16；无效序列按替换字符处理。 */
std::wstring widen(std::string_view utf8) {
  if (utf8.empty()) return {};
  const int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  std::wstring result(static_cast<std::size_t>(length), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), result.data(), length);
  return result;
}

}  // namespace

TextEngine::TextEngine(IDWriteFactory* factory) : factory_(factory) {
  factory_->GetSystemFontCollection(&collection_, FALSE);
  const auto pick = [this](std::initializer_list<const wchar_t*> candidates) {
    for (const wchar_t* name : candidates) {
      if (hasFamily(name)) return std::wstring(name);
    }
    return std::wstring(L"Arial");
  };
  families_[static_cast<int>(FontRole::Latin)] = pick({L"Segoe UI", L"Microsoft YaHei", L"Arial"});
  families_[static_cast<int>(FontRole::Cjk)] = pick({L"Microsoft YaHei", L"SimSun", L"Segoe UI"});
  families_[static_cast<int>(FontRole::Punctuation)] = pick({L"SimSun", L"NSimSun", L"Microsoft YaHei"});
}

bool TextEngine::hasFamily(const wchar_t* name) const {
  UINT32 index = 0;
  BOOL exists = FALSE;
  return collection_ && SUCCEEDED(collection_->FindFamilyName(name, &index, &exists)) && exists != FALSE;
}

const std::wstring& TextEngine::familyFor(FontRole role) { return families_[static_cast<int>(role)]; }

TextEngine::VerticalMetrics TextEngine::verticalMetrics(const std::wstring& family, int weight) {
  const auto key = std::make_pair(family, weight);
  if (const auto found = metricsCache_.find(key); found != metricsCache_.end()) return found->second;

  VerticalMetrics result{0.9, 0.25, 0.0};
  UINT32 index = 0;
  BOOL exists = FALSE;
  Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;
  Microsoft::WRL::ComPtr<IDWriteFont> font;
  Microsoft::WRL::ComPtr<IDWriteFontFace> face;
  if (collection_ && SUCCEEDED(collection_->FindFamilyName(family.c_str(), &index, &exists)) && exists != FALSE && SUCCEEDED(collection_->GetFontFamily(index, &fontFamily)) &&
      SUCCEEDED(fontFamily->GetFirstMatchingFont(static_cast<DWRITE_FONT_WEIGHT>(weight), DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font)) && SUCCEEDED(font->CreateFontFace(&face))) {
    DWRITE_FONT_METRICS metrics{};
    face->GetMetrics(&metrics);
    result = {static_cast<double>(metrics.ascent) / metrics.designUnitsPerEm, static_cast<double>(metrics.descent) / metrics.designUnitsPerEm, static_cast<double>(metrics.lineGap) / metrics.designUnitsPerEm};
  }
  metricsCache_[key] = result;
  return result;
}

Microsoft::WRL::ComPtr<IDWriteTextLayout> TextEngine::createLayout(std::string_view utf8, const TextStyle& style, double maxWidth, double maxHeight) {
  const std::wstring text = widen(utf8);
  const std::wstring& latin = familyFor(FontRole::Latin);
  const auto weight = static_cast<DWRITE_FONT_WEIGHT>(style.fontWeight);

  Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
  if (FAILED(factory_->CreateTextFormat(latin.c_str(), nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, static_cast<FLOAT>(style.fontSize), L"zh-cn", &format))) return nullptr;

  format->SetWordWrapping(style.wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
  format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
  format->SetTextAlignment(style.align == TextAlign::Center ? DWRITE_TEXT_ALIGNMENT_CENTER : (style.align == TextAlign::End ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING));

  // 行高固定为“字号 × 倍数”，字体内容区（上升 + 下降）在行高里上下居中
  const VerticalMetrics vertical = verticalMetrics(latin, style.fontWeight);
  const double lineHeight = style.lineHeight > 0 ? style.fontSize * style.lineHeight : (vertical.ascent + vertical.descent + vertical.lineGap) * style.fontSize;
  const double baseline = (lineHeight - (vertical.ascent + vertical.descent) * style.fontSize) / 2.0 + vertical.ascent * style.fontSize + kBaselineShift;
  format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, static_cast<FLOAT>(lineHeight), static_cast<FLOAT>(baseline));

  Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
  const double width = maxWidth > 0 ? maxWidth : kUnlimitedWidth;
  if (FAILED(factory_->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.size()), format.Get(), static_cast<FLOAT>(width), static_cast<FLOAT>(maxHeight > 0 ? maxHeight : kUnlimitedWidth), &layout))) return nullptr;

  // 按字符分段指定字体：连续同类的字符合并成一段
  std::size_t index = 0;
  while (index < text.size()) {
    const std::size_t begin = index;
    char32_t codePoint = text[index];
    if (codePoint >= 0xD800 && codePoint <= 0xDBFF && index + 1 < text.size()) {
      codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (static_cast<char32_t>(text[index + 1]) - 0xDC00);
      ++index;
    }
    ++index;
    const FontRole role = fontRoleOf(codePoint);
    if (role == FontRole::Latin) continue;
    // 向后合并同类字符
    while (index < text.size()) {
      char32_t next = text[index];
      std::size_t step = 1;
      if (next >= 0xD800 && next <= 0xDBFF && index + 1 < text.size()) {
        next = 0x10000 + ((next - 0xD800) << 10) + (static_cast<char32_t>(text[index + 1]) - 0xDC00);
        step = 2;
      }
      if (fontRoleOf(next) != role) break;
      index += step;
    }
    layout->SetFontFamilyName(familyFor(role).c_str(), DWRITE_TEXT_RANGE{static_cast<UINT32>(begin), static_cast<UINT32>(index - begin)});
  }

  if (style.overflow == TextOverflow::Ellipsis && !style.wrap) {
    Microsoft::WRL::ComPtr<IDWriteInlineObject> sign;
    if (SUCCEEDED(factory_->CreateEllipsisTrimmingSign(layout.Get(), &sign))) {
      const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
      layout->SetTrimming(&trimming, sign.Get());
    }
  }
  return layout;
}

TextMetrics TextEngine::measure(std::string_view utf8, const TextStyle& style, double maxWidth) {
  const auto layout = createLayout(utf8, style, maxWidth, 0);
  if (!layout) return {};
  DWRITE_TEXT_METRICS metrics{};
  if (FAILED(layout->GetMetrics(&metrics))) return {};
  return TextMetrics{static_cast<double>(metrics.width), static_cast<double>(metrics.height)};
}

}  // namespace rujian::ui
