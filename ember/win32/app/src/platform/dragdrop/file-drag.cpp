/*
 * ------------------------------------------------------------------------
 * 名称：file-drag.cpp
 * 说明：文件拖放的实现：虚拟文件的数据对象（文件描述加文件内容，另带窗口内部拖动的标记）、拖动源、拖放目标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：文件内容用内存流（或全局内存）提供：资源管理器在放下之后逐个文件向数据对象要内容，所以数据对象在整个拖放期间保存全部内容。
 * ------------------------------------------------------------------------
 */

#include "platform/dragdrop/file-drag.hpp"

#include <windows.h>
#include <ole2.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <algorithm>
#include <cstring>
#include <optional>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

namespace {

/** 窗口内部拖动用的自定义剪贴板格式的名称。 */
constexpr const wchar_t* kInternalFormatName = L"EmberDiskImageDrag";

/** OLE 要先初始化才能拖放；窗口所在线程已经是单线程套间，这里只是补上 OLE 的部分。 */
void ensureOle() {
  static const HRESULT initialized = OleInitialize(nullptr);
  static_cast<void>(initialized);
}

UINT descriptorFormat() {
  static const UINT format = RegisterClipboardFormatW(L"FileGroupDescriptorW");
  return format;
}

UINT contentsFormat() {
  static const UINT format = RegisterClipboardFormatW(L"FileContents");
  return format;
}

UINT internalFormat() {
  static const UINT format = RegisterClipboardFormatW(kInternalFormatName);
  return format;
}

FORMATETC makeFormat(UINT format, DWORD medium, LONG index = -1) {
  FORMATETC result{};
  result.cfFormat = static_cast<CLIPFORMAT>(format);
  result.ptd = nullptr;
  result.dwAspect = DVASPECT_CONTENT;
  result.lindex = index;
  result.tymed = medium;
  return result;
}

/** 把字节放进一块全局内存。 */
HGLOBAL makeGlobal(const void* data, std::size_t size) {
  HGLOBAL global = GlobalAlloc(GMEM_MOVEABLE, size);
  if (global == nullptr || size == 0) return global;
  void* target = GlobalLock(global);
  if (target == nullptr) {
    GlobalFree(global);
    return nullptr;
  }
  std::memcpy(target, data, size);
  GlobalUnlock(global);
  return global;
}

/** 窗口内部拖动用的数据对象：虚拟文件加内部标记。 */
class DragDataObject final : public IDataObject {
 public:
  DragDataObject(std::vector<DragItem> items, std::uint64_t token) : items_(std::move(items)), token_(token) {
    if (!items_.empty()) {
      formats_.push_back(makeFormat(descriptorFormat(), TYMED_HGLOBAL));
      formats_.push_back(makeFormat(contentsFormat(), TYMED_HGLOBAL | TYMED_ISTREAM));
    }
    formats_.push_back(makeFormat(internalFormat(), TYMED_HGLOBAL));
  }

  STDMETHODIMP QueryInterface(REFIID id, void** object) override {
    if (object == nullptr) return E_POINTER;
    if (id == IID_IUnknown || id == IID_IDataObject) {
      *object = static_cast<IDataObject*>(this);
      AddRef();
      return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
  }
  STDMETHODIMP_(ULONG) AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references_)); }
  STDMETHODIMP_(ULONG) Release() override {
    const LONG count = InterlockedDecrement(&references_);
    if (count == 0) delete this;
    return static_cast<ULONG>(count);
  }

  STDMETHODIMP GetData(FORMATETC* format, STGMEDIUM* medium) override {
    if (format == nullptr || medium == nullptr) return E_INVALIDARG;
    ZeroMemory(medium, sizeof(*medium));
    if ((format->dwAspect & DVASPECT_CONTENT) == 0) return DV_E_DVASPECT;
    if (format->cfFormat == descriptorFormat() && !items_.empty()) {
      if ((format->tymed & TYMED_HGLOBAL) == 0) return DV_E_TYMED;
      return makeDescriptor(medium);
    }
    if (format->cfFormat == contentsFormat() && !items_.empty()) {
      if (format->lindex < 0 || static_cast<std::size_t>(format->lindex) >= items_.size() || items_[static_cast<std::size_t>(format->lindex)].directory) return DV_E_LINDEX;
      const std::vector<std::uint8_t>& data = items_[static_cast<std::size_t>(format->lindex)].content;
      if ((format->tymed & TYMED_ISTREAM) != 0) return makeStream(data, medium);
      if ((format->tymed & TYMED_HGLOBAL) != 0) {
        HGLOBAL global = makeGlobal(data.data(), data.size());
        if (global == nullptr) return E_OUTOFMEMORY;
        medium->tymed = TYMED_HGLOBAL;
        medium->hGlobal = global;
        return S_OK;
      }
      return DV_E_TYMED;
    }
    if (format->cfFormat == internalFormat()) {
      if ((format->tymed & TYMED_HGLOBAL) == 0) return DV_E_TYMED;
      HGLOBAL global = makeGlobal(&token_, sizeof(token_));
      if (global == nullptr) return E_OUTOFMEMORY;
      medium->tymed = TYMED_HGLOBAL;
      medium->hGlobal = global;
      return S_OK;
    }
    return DV_E_FORMATETC;
  }

  STDMETHODIMP GetDataHere(FORMATETC* /*format*/, STGMEDIUM* /*medium*/) override { return E_NOTIMPL; }

  STDMETHODIMP QueryGetData(FORMATETC* format) override {
    if (format == nullptr) return E_INVALIDARG;
    if ((format->dwAspect & DVASPECT_CONTENT) == 0) return DV_E_DVASPECT;
    for (const FORMATETC& supported : formats_) {
      if (supported.cfFormat == format->cfFormat) return (supported.tymed & format->tymed) != 0 ? S_OK : DV_E_TYMED;
    }
    return DV_E_FORMATETC;
  }

  STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* /*format*/, FORMATETC* canonical) override {
    if (canonical != nullptr) canonical->ptd = nullptr;
    return DATA_S_SAMEFORMATETC;
  }

  STDMETHODIMP SetData(FORMATETC* /*format*/, STGMEDIUM* /*medium*/, BOOL /*release*/) override { return E_NOTIMPL; }

  STDMETHODIMP EnumFormatEtc(DWORD direction, IEnumFORMATETC** enumerator) override {
    if (direction != DATADIR_GET) return E_NOTIMPL;
    return SHCreateStdEnumFmtEtc(static_cast<UINT>(formats_.size()), formats_.data(), enumerator);
  }

  STDMETHODIMP DAdvise(FORMATETC* /*format*/, DWORD /*flags*/, IAdviseSink* /*sink*/, DWORD* /*connection*/) override { return OLE_E_ADVISENOTSUPPORTED; }
  STDMETHODIMP DUnadvise(DWORD /*connection*/) override { return OLE_E_ADVISENOTSUPPORTED; }
  STDMETHODIMP EnumDAdvise(IEnumSTATDATA** /*enumerator*/) override { return OLE_E_ADVISENOTSUPPORTED; }

 private:
  ~DragDataObject() = default;

  /** 文件组描述：每个文件或文件夹的名称（相对路径，用反斜杠）、属性与大小。 */
  HRESULT makeDescriptor(STGMEDIUM* medium) const {
    const std::size_t count = items_.size();
    const std::size_t bytes = sizeof(FILEGROUPDESCRIPTORW) + (count - 1) * sizeof(FILEDESCRIPTORW);
    HGLOBAL global = GlobalAlloc(GHND, bytes);
    if (global == nullptr) return E_OUTOFMEMORY;
    auto* group = static_cast<FILEGROUPDESCRIPTORW*>(GlobalLock(global));
    if (group == nullptr) {
      GlobalFree(global);
      return E_OUTOFMEMORY;
    }
    group->cItems = static_cast<UINT>(count);
    for (std::size_t i = 0; i < count; ++i) {
      const DragItem& item = items_[i];
      FILEDESCRIPTORW& descriptor = group->fgd[i];
      descriptor.dwFlags = FD_ATTRIBUTES | FD_FILESIZE | FD_PROGRESSUI;
      descriptor.dwFileAttributes = item.directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
      const ULONGLONG size = item.content.size();
      descriptor.nFileSizeHigh = static_cast<DWORD>(size >> 32);
      descriptor.nFileSizeLow = static_cast<DWORD>(size & 0xFFFFFFFFULL);
      std::wstring name = widen(item.path);
      std::replace(name.begin(), name.end(), L'/', L'\\');
      wcsncpy_s(descriptor.cFileName, MAX_PATH, name.c_str(), _TRUNCATE);
    }
    GlobalUnlock(global);
    medium->tymed = TYMED_HGLOBAL;
    medium->hGlobal = global;
    return S_OK;
  }

  /** 一个文件的内容：内存流，大小精确等于文件大小。 */
  static HRESULT makeStream(const std::vector<std::uint8_t>& data, STGMEDIUM* medium) {
    IStream* stream = SHCreateMemStream(data.empty() ? nullptr : data.data(), static_cast<UINT>(data.size()));
    if (stream == nullptr) return E_OUTOFMEMORY;
    medium->tymed = TYMED_ISTREAM;
    medium->pstm = stream;
    return S_OK;
  }

  LONG references_{1};
  std::vector<DragItem> items_;
  std::uint64_t token_;
  std::vector<FORMATETC> formats_;
};

/** 拖动源：按住左键拖，松开左键放下，Esc 取消，光标用系统默认的。 */
class DragSource final : public IDropSource {
 public:
  STDMETHODIMP QueryInterface(REFIID id, void** object) override {
    if (object == nullptr) return E_POINTER;
    if (id == IID_IUnknown || id == IID_IDropSource) {
      *object = static_cast<IDropSource*>(this);
      AddRef();
      return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
  }
  STDMETHODIMP_(ULONG) AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references_)); }
  STDMETHODIMP_(ULONG) Release() override {
    const LONG count = InterlockedDecrement(&references_);
    if (count == 0) delete this;
    return static_cast<ULONG>(count);
  }

  STDMETHODIMP QueryContinueDrag(BOOL escapePressed, DWORD keyState) override {
    if (escapePressed != FALSE) return DRAGDROP_S_CANCEL;
    if ((keyState & MK_LBUTTON) == 0) return DRAGDROP_S_DROP;
    return S_OK;
  }

  STDMETHODIMP GiveFeedback(DWORD /*effect*/) override { return DRAGDROP_S_USEDEFAULTCURSORS; }

 private:
  ~DragSource() = default;

  LONG references_{1};
};

/** 窗口的拖放目标。 */
class DropTargetImpl final : public IDropTarget {
 public:
  explicit DropTargetImpl(DropCallbacks callbacks) : callbacks_(std::move(callbacks)) {}

  /** 使用方销毁之后不再回调。 */
  void detach() { callbacks_ = {}; }

  STDMETHODIMP QueryInterface(REFIID id, void** object) override {
    if (object == nullptr) return E_POINTER;
    if (id == IID_IUnknown || id == IID_IDropTarget) {
      *object = static_cast<IDropTarget*>(this);
      AddRef();
      return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
  }
  STDMETHODIMP_(ULONG) AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references_)); }
  STDMETHODIMP_(ULONG) Release() override {
    const LONG count = InterlockedDecrement(&references_);
    if (count == 0) delete this;
    return static_cast<ULONG>(count);
  }

  STDMETHODIMP DragEnter(IDataObject* data, DWORD /*keyState*/, POINTL point, DWORD* effect) override {
    kind_ = data != nullptr ? classify(data) : std::nullopt;
    return update(point, effect);
  }

  STDMETHODIMP DragOver(DWORD /*keyState*/, POINTL point, DWORD* effect) override { return update(point, effect); }

  STDMETHODIMP DragLeave() override {
    kind_.reset();
    if (callbacks_.leave) callbacks_.leave();
    return S_OK;
  }

  STDMETHODIMP Drop(IDataObject* data, DWORD /*keyState*/, POINTL point, DWORD* effect) override {
    const std::optional<DropKind> kind = data != nullptr ? classify(data) : std::nullopt;
    const DWORD allowed = effect != nullptr ? *effect : DROPEFFECT_NONE;
    if (effect != nullptr) *effect = DROPEFFECT_NONE;
    if (kind && callbacks_.drop) {
      std::vector<std::string> paths;
      std::uint64_t token = 0;
      if (*kind == DropKind::WindowsFiles) paths = readPaths(data);
      else token = readToken(data);
      const DWORD wanted = *kind == DropKind::Internal ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
      callbacks_.drop(*kind, point.x, point.y, paths, token);
      if (effect != nullptr && (allowed & wanted) != 0) *effect = wanted;
    }
    kind_.reset();
    if (callbacks_.leave) callbacks_.leave();
    return S_OK;
  }

 private:
  ~DropTargetImpl() = default;

  static std::optional<DropKind> classify(IDataObject* data) {
    FORMATETC internal = makeFormat(internalFormat(), TYMED_HGLOBAL);
    if (data->QueryGetData(&internal) == S_OK) return DropKind::Internal;
    FORMATETC files = makeFormat(CF_HDROP, TYMED_HGLOBAL);
    if (data->QueryGetData(&files) == S_OK) return DropKind::WindowsFiles;
    return std::nullopt;
  }

  HRESULT update(POINTL point, DWORD* effect) {
    if (effect == nullptr) return E_INVALIDARG;
    const DWORD allowed = *effect;
    *effect = DROPEFFECT_NONE;
    if (!kind_ || !callbacks_.over || !callbacks_.over(*kind_, point.x, point.y)) return S_OK;
    const DWORD wanted = *kind_ == DropKind::Internal ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
    if ((allowed & wanted) != 0) *effect = wanted;
    return S_OK;
  }

  static std::vector<std::string> readPaths(IDataObject* data) {
    std::vector<std::string> paths;
    FORMATETC format = makeFormat(CF_HDROP, TYMED_HGLOBAL);
    STGMEDIUM medium{};
    if (FAILED(data->GetData(&format, &medium))) return paths;
    if (medium.tymed == TYMED_HGLOBAL) {
      HDROP drop = static_cast<HDROP>(GlobalLock(medium.hGlobal));
      if (drop != nullptr) {
        const UINT count = DragQueryFileW(drop, 0xFFFFFFFFU, nullptr, 0);
        for (UINT i = 0; i < count; ++i) {
          const UINT length = DragQueryFileW(drop, i, nullptr, 0);
          std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
          DragQueryFileW(drop, i, text.data(), length + 1);
          text.resize(length);
          paths.push_back(narrow(text));
        }
        GlobalUnlock(medium.hGlobal);
      }
    }
    ReleaseStgMedium(&medium);
    return paths;
  }

  static std::uint64_t readToken(IDataObject* data) {
    std::uint64_t token = 0;
    FORMATETC format = makeFormat(internalFormat(), TYMED_HGLOBAL);
    STGMEDIUM medium{};
    if (FAILED(data->GetData(&format, &medium))) return token;
    if (medium.tymed == TYMED_HGLOBAL && GlobalSize(medium.hGlobal) >= sizeof(token)) {
      if (const void* source = GlobalLock(medium.hGlobal)) {
        std::memcpy(&token, source, sizeof(token));
        GlobalUnlock(medium.hGlobal);
      }
    }
    ReleaseStgMedium(&medium);
    return token;
  }

  LONG references_{1};
  DropCallbacks callbacks_;
  std::optional<DropKind> kind_;
};

}  // namespace

void startFileDrag(const std::vector<DragItem>& items, std::uint64_t internalToken) {
  ensureOle();
  // 内容是异步读出的，读出来之前用户已经松开了鼠标主键的话，系统的拖放会马上结束，这里就不开始
  const int primary = GetSystemMetrics(SM_SWAPBUTTON) != 0 ? VK_RBUTTON : VK_LBUTTON;
  if ((GetAsyncKeyState(primary) & 0x8000) == 0) return;
  IDataObject* data = new DragDataObject(items, internalToken);
  IDropSource* source = new DragSource();
  DWORD effect = DROPEFFECT_NONE;
  DoDragDrop(data, source, DROPEFFECT_COPY | DROPEFFECT_MOVE, &effect);
  data->Release();
  source->Release();
}

class FileDropTarget::Impl {
 public:
  Impl(HWND window, DropCallbacks callbacks) : window_(window), target_(new DropTargetImpl(std::move(callbacks))) {
    RegisterDragDrop(window_, target_);
  }

  ~Impl() {
    if (IsWindow(window_) != FALSE) RevokeDragDrop(window_);
    target_->detach();
    target_->Release();
  }

 private:
  HWND window_;
  DropTargetImpl* target_;
};

FileDropTarget::FileDropTarget(void* window, DropCallbacks callbacks) {
  ensureOle();
  impl_ = std::make_unique<Impl>(static_cast<HWND>(window), std::move(callbacks));
}

FileDropTarget::~FileDropTarget() = default;

}  // namespace ember::win32::platform
