/*
 * ------------------------------------------------------------------------
 * 名称：memory-font-collection.hpp
 * 说明：内存字体集：把嵌入程序的 TrueType 字节建成 DirectWrite 的自定义字体集，不依赖系统是否装了这些字体。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用 DirectWrite 1.0 的自定义字体文件加载器与字体集加载器实现（Windows 7 可用）；字体字节必须在字体集的整个生命周期里有效（嵌入的静态数据）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <dwrite.h>
#include <wrl/client.h>

#include <string>
#include <vector>

#include "base/font-data.hpp"

namespace rujian::ui {

/** 由一组内存字体建成的字体集。 */
class MemoryFontCollection {
 public:
  /**
   * @param factory DirectWrite 工厂，必须比本对象活得久。
   * @param fonts 字体字节，必须一直有效。
   * @throws std::runtime_error 建字体集失败时。
   */
  MemoryFontCollection(IDWriteFactory* factory, const std::vector<EmbeddedFont>& fonts);
  ~MemoryFontCollection();
  MemoryFontCollection(const MemoryFontCollection&) = delete;
  MemoryFontCollection& operator=(const MemoryFontCollection&) = delete;

  [[nodiscard]] IDWriteFontCollection* collection() const { return collection_.Get(); }
  /** 字体集里第一个字体族的名称（字体里没有英文名时取第一个名称）。 */
  [[nodiscard]] const std::wstring& familyName() const { return familyName_; }

 private:
  IDWriteFactory* factory_;
  Microsoft::WRL::ComPtr<IDWriteFontFileLoader> fileLoader_;
  Microsoft::WRL::ComPtr<IDWriteFontCollectionLoader> collectionLoader_;
  Microsoft::WRL::ComPtr<IDWriteFontCollection> collection_;
  std::wstring familyName_;
};

}  // namespace rujian::ui
