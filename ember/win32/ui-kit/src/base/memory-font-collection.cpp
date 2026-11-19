/*
 * ------------------------------------------------------------------------
 * 名称：memory-font-collection.cpp
 * 说明：内存字体集的实现：字体文件流、文件加载器、字体集加载器与枚举器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：字体文件的键是它在字体数组里的序号（32 位整数）；文件流直接指向嵌入的静态数据，不复制。
 * ------------------------------------------------------------------------
 */

#include "base/memory-font-collection.hpp"

#include <wrl/implements.h>

#include <stdexcept>

namespace rujian::ui {

namespace {

using Microsoft::WRL::ClassicCom;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Make;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;

/** 一个字体文件的读取流。 */
class FontFileStream final : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IDWriteFontFileStream> {
 public:
  explicit FontFileStream(const EmbeddedFont& font) : font_(font) {}

  IFACEMETHODIMP ReadFileFragment(void const** fragmentStart, UINT64 fileOffset, UINT64 fragmentSize, void** fragmentContext) override {
    *fragmentContext = nullptr;
    if (fileOffset > font_.size || fragmentSize > font_.size - fileOffset) {
      *fragmentStart = nullptr;
      return E_FAIL;
    }
    *fragmentStart = font_.data + fileOffset;
    return S_OK;
  }

  IFACEMETHODIMP_(void) ReleaseFileFragment(void* /*fragmentContext*/) override {}

  IFACEMETHODIMP GetFileSize(UINT64* fileSize) override {
    *fileSize = font_.size;
    return S_OK;
  }

  IFACEMETHODIMP GetLastWriteTime(UINT64* lastWriteTime) override {
    *lastWriteTime = 0;
    return E_NOTIMPL;
  }

 private:
  EmbeddedFont font_;
};

/** 字体文件加载器：按键（序号）找到嵌入的字体并建流。 */
class FontFileLoader final : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IDWriteFontFileLoader> {
 public:
  explicit FontFileLoader(const std::vector<EmbeddedFont>& fonts) : fonts_(fonts) {}

  IFACEMETHODIMP CreateStreamFromKey(void const* fontFileReferenceKey, UINT32 fontFileReferenceKeySize, IDWriteFontFileStream** fontFileStream) override {
    *fontFileStream = nullptr;
    if (fontFileReferenceKeySize != sizeof(UINT32)) return E_INVALIDARG;
    const UINT32 index = *static_cast<const UINT32*>(fontFileReferenceKey);
    if (index >= fonts_.size()) return E_INVALIDARG;
    *fontFileStream = Make<FontFileStream>(fonts_[index]).Detach();
    return S_OK;
  }

 private:
  const std::vector<EmbeddedFont>& fonts_;
};

/** 字体文件枚举器：依次给出全部嵌入字体。 */
class FontFileEnumerator final : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IDWriteFontFileEnumerator> {
 public:
  FontFileEnumerator(IDWriteFactory* factory, IDWriteFontFileLoader* loader, UINT32 count) : factory_(factory), loader_(loader), count_(count) {}

  IFACEMETHODIMP MoveNext(BOOL* hasCurrentFile) override {
    ++current_;
    *hasCurrentFile = current_ < count_ ? TRUE : FALSE;
    return S_OK;
  }

  IFACEMETHODIMP GetCurrentFontFile(IDWriteFontFile** fontFile) override {
    *fontFile = nullptr;
    if (current_ >= count_) return E_FAIL;
    const UINT32 key = current_;
    return factory_->CreateCustomFontFileReference(&key, sizeof(key), loader_.Get(), fontFile);
  }

 private:
  ComPtr<IDWriteFactory> factory_;
  ComPtr<IDWriteFontFileLoader> loader_;
  UINT32 count_;
  UINT32 current_{UINT32_MAX};
};

/** 字体集加载器：字体集就是全部嵌入字体。 */
class FontCollectionLoader final : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IDWriteFontCollectionLoader> {
 public:
  FontCollectionLoader(IDWriteFontFileLoader* fileLoader, UINT32 count) : fileLoader_(fileLoader), count_(count) {}

  IFACEMETHODIMP CreateEnumeratorFromKey(IDWriteFactory* factory, void const* /*collectionKey*/, UINT32 /*collectionKeySize*/, IDWriteFontFileEnumerator** fontFileEnumerator) override {
    *fontFileEnumerator = Make<FontFileEnumerator>(factory, fileLoader_.Get(), count_).Detach();
    return S_OK;
  }

 private:
  ComPtr<IDWriteFontFileLoader> fileLoader_;
  UINT32 count_;
};

/** 取字体族的名称：优先英文名。 */
std::wstring familyNameOf(IDWriteFontFamily* family) {
  ComPtr<IDWriteLocalizedStrings> names;
  if (FAILED(family->GetFamilyNames(&names))) return {};
  UINT32 index = 0;
  BOOL exists = FALSE;
  if (FAILED(names->FindLocaleName(L"en-us", &index, &exists)) || exists == FALSE) index = 0;
  UINT32 length = 0;
  if (FAILED(names->GetStringLength(index, &length))) return {};
  std::wstring name(length + 1, L'\0');
  if (FAILED(names->GetString(index, name.data(), length + 1))) return {};
  name.resize(length);
  return name;
}

}  // namespace

MemoryFontCollection::MemoryFontCollection(IDWriteFactory* factory, const std::vector<EmbeddedFont>& fonts) : factory_(factory) {
  const UINT32 count = static_cast<UINT32>(fonts.size());
  fileLoader_ = Make<FontFileLoader>(fonts);
  collectionLoader_ = Make<FontCollectionLoader>(fileLoader_.Get(), count);
  if (FAILED(factory_->RegisterFontFileLoader(fileLoader_.Get()))) throw std::runtime_error("注册字体文件加载器失败");
  if (FAILED(factory_->RegisterFontCollectionLoader(collectionLoader_.Get()))) {
    factory_->UnregisterFontFileLoader(fileLoader_.Get());
    throw std::runtime_error("注册字体集加载器失败");
  }
  const UINT32 key = 0;
  if (FAILED(factory_->CreateCustomFontCollection(collectionLoader_.Get(), &key, sizeof(key), &collection_)) || collection_->GetFontFamilyCount() == 0) {
    collection_.Reset();
    factory_->UnregisterFontCollectionLoader(collectionLoader_.Get());
    factory_->UnregisterFontFileLoader(fileLoader_.Get());
    throw std::runtime_error("建自定义字体集失败");
  }
  ComPtr<IDWriteFontFamily> family;
  if (SUCCEEDED(collection_->GetFontFamily(0, &family))) familyName_ = familyNameOf(family.Get());
}

MemoryFontCollection::~MemoryFontCollection() {
  collection_.Reset();
  factory_->UnregisterFontCollectionLoader(collectionLoader_.Get());
  factory_->UnregisterFontFileLoader(fileLoader_.Get());
}

}  // namespace rujian::ui
