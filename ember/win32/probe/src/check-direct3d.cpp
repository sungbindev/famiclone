/*
 * ------------------------------------------------------------------------
 * 名称：check-direct3d.cpp
 * 说明：Direct3D 11 检查：分别用硬件设备与软件设备（WARP）创建设备与 DXGI 1.0 交换链，上传一张 256x240 的动态纹理并呈现一帧。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：不请求 11_1 的功能级别（Windows 7 没有平台更新时请求它会失败）；硬件设备创建不出来记为失败，软件设备只是探测在没有 3D 加速的电脑（如 Hyper-V 里的 Windows 7）上能否作为回退，失败只记为提示。
 * ------------------------------------------------------------------------
 */

#include "probe-check.hpp"

#include <cstdio>
#include <cstring>

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace ember::win32::probe {

namespace {

using Microsoft::WRL::ComPtr;

/** 模拟器画面的宽度与高度（像素）。 */
constexpr UINT FRAME_WIDTH = 256;
constexpr UINT FRAME_HEIGHT = 240;
/** 微软基本渲染驱动（软件设备）的厂商编号。 */
constexpr UINT SOFTWARE_VENDOR_ID = 0x1414;

/** 功能级别的名称。 */
const char* featureLevelName(D3D_FEATURE_LEVEL level) {
  switch (level) {
    case D3D_FEATURE_LEVEL_11_0: return "11_0";
    case D3D_FEATURE_LEVEL_10_1: return "10_1";
    case D3D_FEATURE_LEVEL_10_0: return "10_0";
    case D3D_FEATURE_LEVEL_9_3: return "9_3";
    case D3D_FEATURE_LEVEL_9_2: return "9_2";
    case D3D_FEATURE_LEVEL_9_1: return "9_1";
    default: return "未知";
  }
}

/** 读取设备所在显卡的描述。 */
bool readAdapter(ID3D11Device& device, DXGI_ADAPTER_DESC& description) {
  ComPtr<IDXGIDevice> dxgiDevice;
  if (FAILED(device.QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf())))) return false;
  ComPtr<IDXGIAdapter> adapter;
  if (FAILED(dxgiDevice->GetAdapter(adapter.GetAddressOf()))) return false;
  return SUCCEEDED(adapter->GetDesc(&description));
}

/** 创建失败时的诊断：列出系统里的显卡，再逐个功能级别单独试一次硬件设备。 */
std::string describeFailure() {
  std::string text;
  ComPtr<IDXGIFactory> factory;
  if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(factory.GetAddressOf())))) {
    ComPtr<IDXGIAdapter> adapter;
    for (UINT index = 0; factory->EnumAdapters(index, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++index) {
      DXGI_ADAPTER_DESC description{};
      if (FAILED(adapter->GetDesc(&description))) continue;
      LARGE_INTEGER driver{};
      const bool hasDriver11 = SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driver));
      char line[200]{};
      std::snprintf(line, sizeof(line), "；显卡 %u：%s，厂商 0x%04X，驱动接口%s", index, toUtf8(description.Description).c_str(), description.VendorId, hasDriver11 ? "可用" : "不可用");
      text += line;
    }
  } else {
    text += "；读不到显卡列表";
  }
  const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1};
  for (const auto level : levels) {
    ComPtr<ID3D11Device> device;
    const HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &level, 1, D3D11_SDK_VERSION, device.GetAddressOf(), nullptr, nullptr);
    text += std::string("；硬件 ") + featureLevelName(level) + (SUCCEEDED(hr) ? " 可用" : " " + formatHresult(hr));
  }
  return text;
}

/** 用指定的驱动类型创建设备与交换链，上传纹理并呈现一帧；软件设备（WARP）只是探测，失败记为提示。 */
void checkDevice(CheckResults& results, HWND window, D3D_DRIVER_TYPE driverType) {
  const bool hardware = driverType == D3D_DRIVER_TYPE_HARDWARE;
  const std::string deviceName = hardware ? "Direct3D 11 硬件设备" : "Direct3D 11 软件设备（WARP）";
  const std::string suffix = hardware ? "" : "（WARP）";
  const CheckLevel failLevel = hardware ? CheckLevel::Fail : CheckLevel::Info;
  DXGI_SWAP_CHAIN_DESC swapChainDesc{};
  swapChainDesc.BufferDesc.Width = FRAME_WIDTH;
  swapChainDesc.BufferDesc.Height = FRAME_HEIGHT;
  swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
  swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.BufferCount = 1;
  swapChainDesc.OutputWindow = window;
  swapChainDesc.Windowed = TRUE;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1};
  ComPtr<IDXGISwapChain> swapChain;
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;
  D3D_FEATURE_LEVEL achieved{};
  HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr, 0, levels, static_cast<UINT>(sizeof(levels) / sizeof(levels[0])), D3D11_SDK_VERSION, &swapChainDesc, swapChain.GetAddressOf(), device.GetAddressOf(), &achieved, context.GetAddressOf());
  if (FAILED(hr)) {
    results.push_back({deviceName, failLevel, "创建失败 " + formatHresult(hr) + (hardware ? "，没有显卡、没有驱动或虚拟机没有 3D 加速" + describeFailure() : std::string())});
    return;
  }

  DXGI_ADAPTER_DESC adapter{};
  std::string adapterName = "读不到显卡信息";
  bool software = false;
  if (readAdapter(*device.Get(), adapter)) {
    adapterName = toUtf8(adapter.Description);
    software = adapter.VendorId == SOFTWARE_VENDOR_ID;
  }
  char text[256]{};
  std::snprintf(text, sizeof(text), "功能级别 %s，%s", featureLevelName(achieved), adapterName.c_str());
  const bool wrongKind = hardware && software;
  results.push_back({deviceName, wrongKind ? CheckLevel::Fail : CheckLevel::Pass, wrongKind ? std::string("得到的是软件设备：") + text : std::string(text)});

  D3D11_TEXTURE2D_DESC textureDesc{};
  textureDesc.Width = FRAME_WIDTH;
  textureDesc.Height = FRAME_HEIGHT;
  textureDesc.MipLevels = 1;
  textureDesc.ArraySize = 1;
  textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.Usage = D3D11_USAGE_DYNAMIC;
  textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  ComPtr<ID3D11Texture2D> texture;
  hr = device->CreateTexture2D(&textureDesc, nullptr, texture.GetAddressOf());
  if (FAILED(hr)) {
    results.push_back({"动态纹理上传" + suffix, failLevel, "创建纹理失败 " + formatHresult(hr)});
    return;
  }
  D3D11_MAPPED_SUBRESOURCE mapped{};
  hr = context->Map(texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) {
    results.push_back({"动态纹理上传" + suffix, failLevel, "映射纹理失败 " + formatHresult(hr)});
    return;
  }
  for (UINT row = 0; row < FRAME_HEIGHT; ++row) std::memset(static_cast<unsigned char*>(mapped.pData) + static_cast<std::size_t>(row) * mapped.RowPitch, 0x80, static_cast<std::size_t>(FRAME_WIDTH) * 4);
  context->Unmap(texture.Get(), 0);
  results.push_back({"动态纹理上传" + suffix, CheckLevel::Pass, "256x240 的 RGBA 纹理可以写入"});

  ComPtr<ID3D11Texture2D> backBuffer;
  hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
  ComPtr<ID3D11RenderTargetView> view;
  if (SUCCEEDED(hr)) hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr, view.GetAddressOf());
  if (SUCCEEDED(hr)) {
    const float black[4] = {0.0F, 0.0F, 0.0F, 1.0F};
    context->ClearRenderTargetView(view.Get(), black);
    hr = swapChain->Present(0, 0);
  }
  results.push_back({"DXGI 1.0 交换链" + suffix, FAILED(hr) ? failLevel : CheckLevel::Pass, FAILED(hr) ? "呈现失败 " + formatHresult(hr) : "位块传送模式可以呈现"});
}

}  // namespace

void checkDirect3D(CheckResults& results, HWND window) {
  checkDevice(results, window, D3D_DRIVER_TYPE_HARDWARE);
  checkDevice(results, window, D3D_DRIVER_TYPE_WARP);
}

}  // namespace ember::win32::probe
