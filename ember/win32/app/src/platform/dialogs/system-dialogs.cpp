/*
 * ------------------------------------------------------------------------
 * 名称：system-dialogs.cpp
 * 说明：系统文件选择窗口的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：每次调用都初始化一次 COM（单线程套间）并在返回前反初始化；线程已用别的模式初始化 COM 时沿用。
 * ------------------------------------------------------------------------
 */

#include "platform/dialogs/system-dialogs.hpp"

#include <windows.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include "platform/text/wide-text.hpp"

namespace ember::win32::platform {

namespace {

using Microsoft::WRL::ComPtr;

/** COM 的初始化范围。 */
class ComScope {
 public:
  ComScope() : result_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
  ~ComScope() {
    if (SUCCEEDED(result_)) CoUninitialize();
  }
  ComScope(const ComScope&) = delete;
  ComScope& operator=(const ComScope&) = delete;

  /** COM 能用：初始化成功，或线程已经用别的模式初始化过。 */
  [[nodiscard]] bool usable() const { return SUCCEEDED(result_) || result_ == RPC_E_CHANGED_MODE; }

 private:
  HRESULT result_;
};

/** 文件类型过滤的窗口文字：每个过滤一条，扩展名用分号连接。 */
struct FilterTexts {
  std::vector<std::wstring> names;
  std::vector<std::wstring> specs;
  std::vector<COMDLG_FILTERSPEC> entries;

  explicit FilterTexts(const std::vector<DialogFilter>& filters) {
    names.reserve(filters.size());
    specs.reserve(filters.size());
    for (const DialogFilter& filter : filters) {
      std::wstring spec;
      for (const std::string& extension : filter.extensions) {
        if (!spec.empty()) spec += L';';
        spec += extension == "*" ? std::wstring(L"*.*") : L"*." + widen(extension);
      }
      names.push_back(widen(filter.name));
      specs.push_back(std::move(spec));
    }
    for (std::size_t i = 0; i < names.size(); ++i) entries.push_back({names[i].c_str(), specs[i].c_str()});
  }
};

/** 设置窗口打开时所在的目录；目录不存在时不设置。 */
void setStartDirectory(IFileDialog* dialog, const std::optional<std::string>& directory) {
  if (!directory) return;
  ComPtr<IShellItem> folder;
  if (SUCCEEDED(SHCreateItemFromParsingName(widen(*directory).c_str(), nullptr, IID_PPV_ARGS(&folder)))) dialog->SetFolder(folder.Get());
}

/** 取用户选定的路径。 */
std::optional<std::string> selectedPath(IFileDialog* dialog) {
  ComPtr<IShellItem> item;
  if (FAILED(dialog->GetResult(&item))) return std::nullopt;
  PWSTR raw = nullptr;
  if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw))) return std::nullopt;
  std::string path = narrow(raw);
  CoTaskMemFree(raw);
  if (path.empty()) return std::nullopt;
  return path;
}

/** 显示窗口并取结果。 */
std::optional<std::string> show(IFileDialog* dialog, void* parent) {
  if (FAILED(dialog->Show(static_cast<HWND>(parent)))) return std::nullopt;
  return selectedPath(dialog);
}

}  // namespace

SystemDialogs::SystemDialogs(ParentProvider parent) : parent_(std::move(parent)) {}

std::optional<std::string> SystemDialogs::pickFile(const FileDialogRequest& request) const {
  ComScope com;
  if (!com.usable()) return std::nullopt;
  ComPtr<IFileOpenDialog> dialog;
  if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) return std::nullopt;
  FILEOPENDIALOGOPTIONS options = 0;
  dialog->GetOptions(&options);
  dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
  dialog->SetTitle(widen(request.title).c_str());
  const FilterTexts filters(request.filters);
  if (!filters.entries.empty()) dialog->SetFileTypes(static_cast<UINT>(filters.entries.size()), filters.entries.data());
  setStartDirectory(dialog.Get(), request.startDirectory);
  return show(dialog.Get(), parent_ ? parent_() : nullptr);
}

std::optional<std::string> SystemDialogs::pickDirectory(const DirectoryDialogRequest& request) const {
  ComScope com;
  if (!com.usable()) return std::nullopt;
  ComPtr<IFileOpenDialog> dialog;
  if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) return std::nullopt;
  FILEOPENDIALOGOPTIONS options = 0;
  dialog->GetOptions(&options);
  dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
  dialog->SetTitle(widen(request.title).c_str());
  setStartDirectory(dialog.Get(), request.startDirectory);
  return show(dialog.Get(), parent_ ? parent_() : nullptr);
}

std::optional<std::string> SystemDialogs::pickSaveFile(const FileDialogRequest& request) const {
  ComScope com;
  if (!com.usable()) return std::nullopt;
  ComPtr<IFileSaveDialog> dialog;
  if (FAILED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) return std::nullopt;
  FILEOPENDIALOGOPTIONS options = 0;
  dialog->GetOptions(&options);
  dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT);
  dialog->SetTitle(widen(request.title).c_str());
  const FilterTexts filters(request.filters);
  if (!filters.entries.empty()) {
    dialog->SetFileTypes(static_cast<UINT>(filters.entries.size()), filters.entries.data());
    // 第一个过滤的第一个扩展名作为没写扩展名时补上的扩展名
    if (!request.filters[0].extensions.empty() && request.filters[0].extensions[0] != "*") dialog->SetDefaultExtension(widen(request.filters[0].extensions[0]).c_str());
  }
  setStartDirectory(dialog.Get(), request.startDirectory);
  return show(dialog.Get(), parent_ ? parent_() : nullptr);
}

}  // namespace ember::win32::platform
