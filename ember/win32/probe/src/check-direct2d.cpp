/*
 * ------------------------------------------------------------------------
 * 名称：check-direct2d.cpp
 * 说明：Direct2D 与 DirectWrite 检查：创建 Direct2D 1.0 的窗口渲染目标并绘制一帧，创建 DirectWrite 1.0 工厂并查找界面用到的字体。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只用 Windows 7 自带的接口版本；没有显卡时 Direct2D 自动改用软件渲染，所以这里不要求硬件加速。
 * ------------------------------------------------------------------------
 */

#include "probe-check.hpp"

#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace ember::win32::probe {

namespace {

using Microsoft::WRL::ComPtr;

/** 检查 Direct2D 能否在窗口上绘制。 */
void checkRenderTarget(CheckResults& results, HWND window) {
  ComPtr<ID2D1Factory> factory;
  HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), nullptr, reinterpret_cast<void**>(factory.GetAddressOf()));
  if (FAILED(hr)) {
    results.push_back({"Direct2D 1.0", CheckLevel::Fail, "创建工厂失败 " + formatHresult(hr)});
    return;
  }
  RECT client{};
  GetClientRect(window, &client);
  const D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(client.right - client.left), static_cast<UINT32>(client.bottom - client.top));
  ComPtr<ID2D1HwndRenderTarget> target;
  hr = factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(window, size), target.GetAddressOf());
  if (FAILED(hr)) {
    results.push_back({"Direct2D 1.0", CheckLevel::Fail, "创建窗口渲染目标失败 " + formatHresult(hr)});
    return;
  }
  target->BeginDraw();
  target->Clear(D2D1::ColorF(D2D1::ColorF::Black));
  hr = target->EndDraw();
  results.push_back({"Direct2D 1.0", FAILED(hr) ? CheckLevel::Fail : CheckLevel::Pass, FAILED(hr) ? "绘制失败 " + formatHresult(hr) : "窗口渲染目标可以绘制"});
}

/** 检查 DirectWrite 工厂，并查找一个字体族是否存在。 */
bool hasFont(IDWriteFontCollection& fonts, const wchar_t* family) {
  UINT32 index = 0;
  BOOL exists = FALSE;
  return SUCCEEDED(fonts.FindFamilyName(family, &index, &exists)) && exists != FALSE;
}

/** 检查 DirectWrite 与界面用到的字体。 */
void checkFonts(CheckResults& results) {
  ComPtr<IDWriteFactory> factory;
  HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf()));
  if (FAILED(hr)) {
    results.push_back({"DirectWrite 1.0", CheckLevel::Fail, "创建工厂失败 " + formatHresult(hr)});
    return;
  }
  ComPtr<IDWriteFontCollection> fonts;
  hr = factory->GetSystemFontCollection(fonts.GetAddressOf(), FALSE);
  if (FAILED(hr)) {
    results.push_back({"DirectWrite 1.0", CheckLevel::Fail, "读取系统字体失败 " + formatHresult(hr)});
    return;
  }
  const bool segoe = hasFont(*fonts.Get(), L"Segoe UI");
  const bool yahei = hasFont(*fonts.Get(), L"Microsoft YaHei");
  const bool simsun = hasFont(*fonts.Get(), L"SimSun");
  results.push_back({"DirectWrite 1.0", CheckLevel::Pass, "工厂可用"});
  results.push_back({"字体 Segoe UI", segoe ? CheckLevel::Pass : CheckLevel::Fail, segoe ? "存在" : "不存在"});
  results.push_back({"字体 Microsoft YaHei", yahei ? CheckLevel::Pass : CheckLevel::Fail, yahei ? "存在" : "不存在"});
  results.push_back({"字体 SimSun", simsun ? CheckLevel::Pass : CheckLevel::Info, simsun ? "存在" : "不存在，弯引号等标点改用微软雅黑"});
}

}  // namespace

void checkDirect2D(CheckResults& results, HWND window) {
  checkRenderTarget(results, window);
  checkFonts(results);
}

}  // namespace ember::win32::probe
