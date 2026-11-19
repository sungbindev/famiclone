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
#include <cmath>
#include <vector>

#include "base/unicode.hpp"

namespace rujian::ui {

namespace {

/** 网页版与原生版的基线校准量（逻辑像素），与网页版截图对照后确定。 */
constexpr double kBaselineShift = 0.0;

/** 不限宽度时使用的排版宽度。 */
constexpr double kUnlimitedWidth = 100000.0;

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
  try {
    brand_ = std::make_unique<MemoryFontCollection>(factory_, embeddedFonts());
  } catch (const std::exception&) {
    // 嵌入字体加载不了时不影响其他文字，品牌字体改用正文字体
    brand_.reset();
  }
}

bool TextEngine::hasFamily(const wchar_t* name) const {
  UINT32 index = 0;
  BOOL exists = FALSE;
  return collection_ && SUCCEEDED(collection_->FindFamilyName(name, &index, &exists)) && exists != FALSE;
}

const std::wstring& TextEngine::familyFor(FontRole role) { return families_[static_cast<int>(role)]; }

TextEngine::VerticalMetrics TextEngine::verticalMetrics(const std::wstring& family, int weight, IDWriteFontCollection* collection) {
  const auto key = std::make_pair(family, weight);
  if (const auto found = metricsCache_.find(key); found != metricsCache_.end()) return found->second;

  VerticalMetrics result{0.9, 0.25, 0.0};
  UINT32 index = 0;
  BOOL exists = FALSE;
  Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;
  Microsoft::WRL::ComPtr<IDWriteFont> font;
  Microsoft::WRL::ComPtr<IDWriteFontFace> face;
  IDWriteFontCollection* source = collection != nullptr ? collection : collection_.Get();
  if (source && SUCCEEDED(source->FindFamilyName(family.c_str(), &index, &exists)) && exists != FALSE && SUCCEEDED(source->GetFontFamily(index, &fontFamily)) &&
      SUCCEEDED(fontFamily->GetFirstMatchingFont(static_cast<DWRITE_FONT_WEIGHT>(weight), DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font)) && SUCCEEDED(font->CreateFontFace(&face))) {
    DWRITE_FONT_METRICS metrics{};
    face->GetMetrics(&metrics);
    result = {static_cast<double>(metrics.ascent) / metrics.designUnitsPerEm, static_cast<double>(metrics.descent) / metrics.designUnitsPerEm, static_cast<double>(metrics.lineGap) / metrics.designUnitsPerEm};
  }
  metricsCache_[key] = result;
  return result;
}

Microsoft::WRL::ComPtr<IDWriteTextLayout> TextEngine::createLayout(std::string_view utf8, const TextStyle& style, double maxWidth, double maxHeight) {
  return createLayout(std::u16string_view(toUtf16(utf8)), style, maxWidth, maxHeight);
}

Microsoft::WRL::ComPtr<IDWriteTextLayout> TextEngine::createLayout(std::u16string_view utf16, const TextStyle& style, double maxWidth, double maxHeight) {
  const std::wstring text(reinterpret_cast<const wchar_t*>(utf16.data()), utf16.size());
  const bool brand = style.face == TextFace::Brand && brand_ != nullptr;
  const std::wstring& latin = brand ? brand_->familyName() : familyFor(FontRole::Latin);
  IDWriteFontCollection* const collection = brand ? brand_->collection() : nullptr;
  const auto weight = static_cast<DWRITE_FONT_WEIGHT>(style.fontWeight);

  Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
  if (FAILED(factory_->CreateTextFormat(latin.c_str(), collection, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, static_cast<FLOAT>(style.fontSize), L"zh-cn", &format))) return nullptr;

  format->SetWordWrapping(style.wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
  format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
  format->SetTextAlignment(style.align == TextAlign::Center ? DWRITE_TEXT_ALIGNMENT_CENTER : (style.align == TextAlign::End ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING));

  // 行高固定为“字号 × 倍数”，字体内容区（上升 + 下降）在行高里上下居中；
  // 行高为 normal 时，网页版把上升、下降、行距各自四舍五入到设备像素再相加
  const VerticalMetrics vertical = verticalMetrics(latin, style.fontWeight, collection);
  const auto toDevice = [this](double value) { return std::round(value * scale_) / scale_; };
  const bool normal = style.lineHeight <= 0;
  const double ascent = normal ? toDevice(vertical.ascent * style.fontSize) : vertical.ascent * style.fontSize;
  const double descent = normal ? toDevice(vertical.descent * style.fontSize) : vertical.descent * style.fontSize;
  const double lineGap = normal ? toDevice(vertical.lineGap * style.fontSize) : vertical.lineGap * style.fontSize;
  const double lineHeight = style.lineHeight > 0 ? style.fontSize * style.lineHeight : ascent + descent + lineGap;
  const double baseline = (lineHeight - (ascent + descent)) / 2.0 + ascent + kBaselineShift;
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
    // 品牌字体的排版对象用的是自定义字体集，其他字符要改回系统字体集
    if (brand) layout->SetFontCollection(collection_.Get(), DWRITE_TEXT_RANGE{static_cast<UINT32>(begin), static_cast<UINT32>(index - begin)});
  }

  if (style.underline && !text.empty()) layout->SetUnderline(TRUE, DWRITE_TEXT_RANGE{0, static_cast<UINT32>(text.size())});

  if (style.overflow == TextOverflow::Ellipsis && !style.wrap) {
    // 网页版的省略号取自 Rujian CJK Punctuation（宋体类字体），所以省略号标记也用标点字体建
    Microsoft::WRL::ComPtr<IDWriteTextFormat> signFormat;
    Microsoft::WRL::ComPtr<IDWriteInlineObject> sign;
    if (SUCCEEDED(factory_->CreateTextFormat(familyFor(FontRole::Punctuation).c_str(), nullptr, weight, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, static_cast<FLOAT>(style.fontSize), L"zh-cn", &signFormat)) &&
        SUCCEEDED(factory_->CreateEllipsisTrimmingSign(signFormat.Get(), &sign))) {
      const DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0};
      layout->SetTrimming(&trimming, sign.Get());
    }
  }
  return layout;
}

TextMetrics TextEngine::measure(std::string_view utf8, const TextStyle& style, double maxWidth) { return measure(std::u16string_view(toUtf16(utf8)), style, maxWidth); }

TextMetrics TextEngine::measure(std::u16string_view utf16, const TextStyle& style, double maxWidth) {
  const auto layout = createLayout(utf16, style, maxWidth, 0);
  if (!layout) return {};
  DWRITE_TEXT_METRICS metrics{};
  if (FAILED(layout->GetMetrics(&metrics))) return {};
  double width = metrics.width;
  // 字间距：每个字符簇后面加一份（包括最后一个，与 CSS 一致）
  if (style.letterSpacing != 0) width += static_cast<double>(clusterMetrics(layout.Get()).size()) * style.letterSpacing * style.fontSize;
  return TextMetrics{width, static_cast<double>(metrics.height)};
}

std::vector<DWRITE_CLUSTER_METRICS> TextEngine::clusterMetrics(IDWriteTextLayout* layout) {
  UINT32 count = 0;
  layout->GetClusterMetrics(nullptr, 0, &count);
  std::vector<DWRITE_CLUSTER_METRICS> clusters(count);
  if (count > 0 && FAILED(layout->GetClusterMetrics(clusters.data(), count, &count))) return {};
  clusters.resize(count);
  return clusters;
}

double TextEngine::caretOffset(std::u16string_view text, const TextStyle& style, std::size_t index) {
  const auto layout = createLayout(text, style, 0, 0);
  if (!layout) return 0;
  FLOAT x = 0;
  FLOAT y = 0;
  DWRITE_HIT_TEST_METRICS metrics{};
  if (FAILED(layout->HitTestTextPosition(static_cast<UINT32>(std::min(index, text.size())), FALSE, &x, &y, &metrics))) return 0;
  return static_cast<double>(x);
}

std::size_t TextEngine::caretIndexAt(std::u16string_view text, const TextStyle& style, double x) {
  if (text.empty()) return 0;
  const auto layout = createLayout(text, style, 0, 0);
  if (!layout) return 0;
  BOOL trailing = FALSE;
  BOOL inside = FALSE;
  DWRITE_HIT_TEST_METRICS metrics{};
  if (FAILED(layout->HitTestPoint(static_cast<FLOAT>(x), 0.0f, &trailing, &inside, &metrics))) return 0;
  return static_cast<std::size_t>(metrics.textPosition) + (trailing != FALSE ? metrics.length : 0);
}

}  // namespace rujian::ui
