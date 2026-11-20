/*
 * ------------------------------------------------------------------------
 * 名称：screen-window.cpp
 * 说明：运行画面子窗口的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：设备丢失（显卡驱动重置、拔掉显卡）时下一次 render 重新创建设备与全部资源，帧纹理由下一帧补上。
 * ------------------------------------------------------------------------
 */

#include "platform/display/screen-window.hpp"

#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>

#include "platform/display/screen-shaders.hpp"

namespace ember::win32::platform {

namespace {

using Microsoft::WRL::ComPtr;

constexpr wchar_t kWindowClass[] = L"EmberScreenWindow";

/** 毛玻璃模糊的内圈半径（设备像素），外圈是它的两倍。 */
constexpr float kGlassBlurRadius = 5.0f;

/** 顶点：位置（标准化设备坐标）与纹理坐标。 */
struct Vertex {
  float x;
  float y;
  float u;
  float v;
};

}  // namespace

struct ScreenWindow::Impl {
  HWND hwnd{nullptr};
  HWND parent{nullptr};
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;
  ComPtr<IDXGISwapChain> swapChain;
  ComPtr<ID3D11RenderTargetView> renderTarget;
  ComPtr<ID3D11VertexShader> vertexShader;
  ComPtr<ID3D11PixelShader> pixelShader;
  ComPtr<ID3D11PixelShader> glassShader;
  ComPtr<ID3D11Buffer> glassConstants;
  ComPtr<ID3D11InputLayout> inputLayout;
  ComPtr<ID3D11Buffer> vertexBuffer;
  ComPtr<ID3D11SamplerState> pointSampler;
  ComPtr<ID3D11SamplerState> linearSampler;
  ComPtr<ID3D11BlendState> opaqueBlend;
  ComPtr<ID3D11BlendState> premultipliedBlend;
  ComPtr<ID3D11RasterizerState> rasterizer;
  ComPtr<ID3D11Texture2D> frameTexture;
  ComPtr<ID3D11ShaderResourceView> frameView;
  std::uint32_t frameWidth{0};
  std::uint32_t frameHeight{0};
  ComPtr<ID3D11ShaderResourceView> overlayView;
  /** 提示框的矩形（设备像素，相对子窗口）：这块区域里的画面先模糊再叠上提示位图。 */
  shared::ScreenRect glass;
  bool frameVisible{false};
  shared::ScreenAspect aspect{shared::kDefaultScreenAspect};
  shared::ScreenScaling scaling{shared::kDefaultScreenScaling};
  int cropLines{0};
  shared::ScreenRect rect;
  std::vector<shared::ScreenRect> holes;
  UINT swapWidth{0};
  UINT swapHeight{0};

  Impl() = default;
  ~Impl() {
    if (hwnd != nullptr) DestroyWindow(hwnd);
  }

  /** 窗口过程：对鼠标透明，鼠标与键盘都由主窗口处理；设置区域等露出子窗口内容时重画。 */
  static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
      case WM_NCHITTEST:
        return HTTRANSPARENT;
      case WM_ERASEBKGND:
        return 1;
      case WM_PAINT: {
        PAINTSTRUCT paint;
        BeginPaint(window, &paint);
        EndPaint(window, &paint);
        if (auto* self = reinterpret_cast<Impl*>(GetWindowLongPtrW(window, GWLP_USERDATA))) self->render();
        return 0;
      }
      default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
  }

  void createDevice() {
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1};
    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferDesc.Width = std::max<UINT>(1, static_cast<UINT>(rect.width));
    description.BufferDesc.Height = std::max<UINT>(1, static_cast<UINT>(rect.height));
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 1;
    description.OutputWindow = hwnd;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL achieved{};
    // 只用硬件设备：不用 WARP 等软件设备
    const HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION, &description, &swapChain, &device,
                                                          &achieved, &context);
    if (FAILED(result)) throw std::runtime_error("无法初始化画面渲染：创建不出硬件的 Direct3D 11 设备");
    swapWidth = description.BufferDesc.Width;
    swapHeight = description.BufferDesc.Height;
    // 不让系统用 Alt+Enter 把画面切成独占全屏
    ComPtr<IDXGIFactory> factory;
    if (SUCCEEDED(swapChain->GetParent(IID_PPV_ARGS(&factory)))) factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES);
    createResources();
    createRenderTarget();
  }

  void createResources() {
    check(device->CreateVertexShader(kScreenVertexShader, kScreenVertexShaderSize, nullptr, &vertexShader), "顶点着色器");
    check(device->CreatePixelShader(kScreenPixelShader, kScreenPixelShaderSize, nullptr, &pixelShader), "像素着色器");
    check(device->CreatePixelShader(kScreenGlassShader, kScreenGlassShaderSize, nullptr, &glassShader), "毛玻璃着色器");
    D3D11_BUFFER_DESC constantDescription{};
    constantDescription.ByteWidth = sizeof(float) * 4;
    constantDescription.Usage = D3D11_USAGE_DYNAMIC;
    constantDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    check(device->CreateBuffer(&constantDescription, nullptr, &glassConstants), "毛玻璃常量缓冲");
    const D3D11_INPUT_ELEMENT_DESC elements[] = {{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0}};
    check(device->CreateInputLayout(elements, static_cast<UINT>(std::size(elements)), kScreenVertexShader, kScreenVertexShaderSize, &inputLayout), "输入布局");
    D3D11_BUFFER_DESC bufferDescription{};
    bufferDescription.ByteWidth = sizeof(Vertex) * 4;
    bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
    bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    check(device->CreateBuffer(&bufferDescription, nullptr, &vertexBuffer), "顶点缓冲");
    D3D11_SAMPLER_DESC samplerDescription{};
    samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDescription.AddressU = samplerDescription.AddressV = samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;
    samplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
    check(device->CreateSamplerState(&samplerDescription, &pointSampler), "最近邻采样器");
    samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    check(device->CreateSamplerState(&samplerDescription, &linearSampler), "双线性采样器");
    D3D11_BLEND_DESC blendDescription{};
    blendDescription.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    check(device->CreateBlendState(&blendDescription, &opaqueBlend), "不透明混合");
    blendDescription.RenderTarget[0].BlendEnable = TRUE;
    blendDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    check(device->CreateBlendState(&blendDescription, &premultipliedBlend), "预乘透明度混合");
    D3D11_RASTERIZER_DESC rasterizerDescription{};
    rasterizerDescription.FillMode = D3D11_FILL_SOLID;
    rasterizerDescription.CullMode = D3D11_CULL_NONE;
    rasterizerDescription.DepthClipEnable = TRUE;
    check(device->CreateRasterizerState(&rasterizerDescription, &rasterizer), "光栅化状态");
  }

  static void check(HRESULT result, const char* what) {
    if (FAILED(result)) throw std::runtime_error(std::string("无法初始化画面渲染：创建") + what + "失败");
  }

  void createRenderTarget() {
    ComPtr<ID3D11Texture2D> backBuffer;
    check(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)), "后台缓冲");
    check(device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTarget), "渲染目标");
  }

  /** 让交换链的缓冲与子窗口一样大。 */
  void resizeSwapChain() {
    const UINT width = std::max<UINT>(1, static_cast<UINT>(rect.width));
    const UINT height = std::max<UINT>(1, static_cast<UINT>(rect.height));
    if (width == swapWidth && height == swapHeight) return;
    context->OMSetRenderTargets(0, nullptr, nullptr);
    renderTarget.Reset();
    if (FAILED(swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))) return;
    swapWidth = width;
    swapHeight = height;
    createRenderTarget();
  }

  /** 设备丢失后重新创建设备与全部资源；帧纹理由下一帧补上。 */
  void recoverDevice() {
    context.Reset();
    renderTarget.Reset();
    swapChain.Reset();
    device.Reset();
    frameTexture.Reset();
    frameView.Reset();
    overlayView.Reset();
    frameWidth = frameHeight = 0;
    createDevice();
  }

  void applyRegion() {
    if (hwnd == nullptr || rect.width <= 0 || rect.height <= 0) return;
    if (holes.empty()) {
      SetWindowRgn(hwnd, nullptr, TRUE);
      return;
    }
    HRGN region = CreateRectRgn(0, 0, rect.width, rect.height);
    for (const auto& hole : holes) {
      HRGN cut = CreateRectRgn(hole.x - rect.x, hole.y - rect.y, hole.x - rect.x + hole.width, hole.y - rect.y + hole.height);
      CombineRgn(region, region, cut, RGN_DIFF);
      DeleteObject(cut);
    }
    // 系统接管 region，不用再删除
    SetWindowRgn(hwnd, region, TRUE);
  }

  void writeQuad(float left, float top, float right, float bottom, float u0, float u1, float v0, float v1) {
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
    auto* vertices = static_cast<Vertex*>(mapped.pData);
    // 三角形带：左上、右上、左下、右下
    vertices[0] = Vertex{left, top, u0, v0};
    vertices[1] = Vertex{right, top, u1, v0};
    vertices[2] = Vertex{left, bottom, u0, v1};
    vertices[3] = Vertex{right, bottom, u1, v1};
    context->Unmap(vertexBuffer.Get(), 0);
  }

  /** 把纹理的 [u0, u1] x [v0, v1] 这一块画满 viewport。 */
  void draw(ID3D11ShaderResourceView* view, ID3D11SamplerState* sampler, ID3D11BlendState* blend, const shared::ScreenRect& viewport, float u0, float u1, float v0, float v1) {
    writeQuad(-1.0f, 1.0f, 1.0f, -1.0f, u0, u1, v0, v1);
    const D3D11_VIEWPORT area{static_cast<float>(viewport.x), static_cast<float>(viewport.y), static_cast<float>(viewport.width), static_cast<float>(viewport.height), 0.0f, 1.0f};
    context->RSSetViewports(1, &area);
    context->OMSetBlendState(blend, nullptr, 0xffffffff);
    context->PSSetShaderResources(0, 1, &view);
    ID3D11SamplerState* samplers[] = {sampler};
    context->PSSetSamplers(0, 1, samplers);
    context->Draw(4, 0);
  }

  /**
   * 毛玻璃：把提示框与画面相交的区域里的画面换成模糊后的，再由提示位图叠上底色、边框与文字；画面之外是黑底，不需要模糊。
   * video 是画面在子窗口里的矩形，v0、v1 是它对应的纹理纵坐标范围。
   */
  void drawGlass(const shared::ScreenRect& video, float v0, float v1) {
    if (glass.width <= 0 || glass.height <= 0 || video.width <= 0 || video.height <= 0) return;
    const int left = std::max(glass.x, video.x);
    const int top = std::max(glass.y, video.y);
    const int right = std::min(glass.x + glass.width, video.x + video.width);
    const int bottom = std::min(glass.y + glass.height, video.y + video.height);
    if (right <= left || bottom <= top) return;
    const float width = static_cast<float>(video.width);
    const float height = static_cast<float>(video.height);
    const float u0 = static_cast<float>(left - video.x) / width;
    const float u1 = static_cast<float>(right - video.x) / width;
    const float t0 = v0 + static_cast<float>(top - video.y) / height * (v1 - v0);
    const float t1 = v0 + static_cast<float>(bottom - video.y) / height * (v1 - v0);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(context->Map(glassConstants.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
    auto* step = static_cast<float*>(mapped.pData);
    step[0] = kGlassBlurRadius / width;
    step[1] = kGlassBlurRadius / height * (v1 - v0);
    step[2] = 0.0f;
    step[3] = 0.0f;
    context->Unmap(glassConstants.Get(), 0);
    ID3D11Buffer* constants[] = {glassConstants.Get()};
    context->PSSetConstantBuffers(0, 1, constants);
    context->PSSetShader(glassShader.Get(), nullptr, 0);
    draw(frameView.Get(), linearSampler.Get(), opaqueBlend.Get(), shared::ScreenRect{left, top, right - left, bottom - top}, u0, u1, t0, t1);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
  }

  void render() {
    if (rect.width <= 0 || rect.height <= 0) return;
    if (!device) recoverDevice();
    resizeSwapChain();
    if (!renderTarget) return;
    ID3D11RenderTargetView* targets[] = {renderTarget.Get()};
    context->OMSetRenderTargets(1, targets, nullptr);
    const float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    context->ClearRenderTargetView(renderTarget.Get(), black);
    context->IASetInputLayout(inputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    const UINT stride = sizeof(Vertex);
    const UINT offset = 0;
    ID3D11Buffer* buffers[] = {vertexBuffer.Get()};
    context->IASetVertexBuffers(0, 1, buffers, &stride, &offset);
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->RSSetState(rasterizer.Get());
    if (frameVisible && frameView && frameWidth > 0 && frameHeight > 0 && static_cast<int>(frameHeight) > cropLines * 2) {
      const int visibleHeight = static_cast<int>(frameHeight) - cropLines * 2;
      const shared::ScreenRect video = shared::computeScreenRect(rect.width, rect.height, frameWidth, visibleHeight, aspect, scaling);
      if (video.width > 0 && video.height > 0) {
        const float top = static_cast<float>(cropLines) / static_cast<float>(frameHeight);
        const float bottom = static_cast<float>(static_cast<int>(frameHeight) - cropLines) / static_cast<float>(frameHeight);
        draw(frameView.Get(), scaling == shared::ScreenScaling::Integer ? pointSampler.Get() : linearSampler.Get(), opaqueBlend.Get(), video, 0.0f, 1.0f, top, bottom);
        if (overlayView) drawGlass(video, top, bottom);
      }
    }
    if (overlayView) draw(overlayView.Get(), linearSampler.Get(), premultipliedBlend.Get(), shared::ScreenRect{0, 0, rect.width, rect.height}, 0.0f, 1.0f, 0.0f, 1.0f);
    // 不等垂直同步：界面线程不能被显示器的刷新卡住，窗口合成由系统负责，不会撕裂
    const HRESULT presented = swapChain->Present(0, 0);
    if (presented == DXGI_ERROR_DEVICE_REMOVED || presented == DXGI_ERROR_DEVICE_RESET) {
      device.Reset();
      context.Reset();
      swapChain.Reset();
      renderTarget.Reset();
    }
  }
};

ScreenWindow::ScreenWindow(void* parentWindow) : impl_(std::make_unique<Impl>()) {
  static const ATOM registered = [] {
    WNDCLASSEXW windowClass{sizeof(windowClass)};
    windowClass.lpfnWndProc = &Impl::procedure;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));  // 32512 即 IDC_ARROW
    windowClass.lpszClassName = kWindowClass;
    return RegisterClassExW(&windowClass);
  }();
  if (registered == 0) throw std::runtime_error("注册画面窗口类失败");
  impl_->parent = static_cast<HWND>(parentWindow);
  impl_->hwnd = CreateWindowExW(0, kWindowClass, L"", WS_CHILD | WS_CLIPSIBLINGS, 0, 0, 1, 1, impl_->parent, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (impl_->hwnd == nullptr) throw std::runtime_error("创建画面窗口失败");
  SetWindowLongPtrW(impl_->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl_.get()));
  impl_->createDevice();
}

ScreenWindow::~ScreenWindow() {
  if (impl_ != nullptr && impl_->hwnd != nullptr) SetWindowLongPtrW(impl_->hwnd, GWLP_USERDATA, 0);
}

void* ScreenWindow::handle() const { return impl_->hwnd; }

void ScreenWindow::setBounds(const shared::ScreenRect& rect) {
  if (rect == impl_->rect) return;
  impl_->rect = rect;
  if (rect.width <= 0 || rect.height <= 0) {
    ShowWindow(impl_->hwnd, SW_HIDE);
    return;
  }
  SetWindowPos(impl_->hwnd, HWND_TOP, rect.x, rect.y, rect.width, rect.height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  impl_->applyRegion();
  impl_->render();
}

shared::ScreenRect ScreenWindow::bounds() const { return impl_->rect; }

shared::ScreenRect ScreenWindow::screenBounds() const {
  RECT area{};
  if (impl_->rect.width <= 0 || impl_->rect.height <= 0 || GetWindowRect(impl_->hwnd, &area) == FALSE) return shared::ScreenRect{};
  return shared::ScreenRect{area.left, area.top, area.right - area.left, area.bottom - area.top};
}

void ScreenWindow::setView(shared::ScreenAspect aspect, shared::ScreenScaling scaling, int cropLines) {
  if (impl_->aspect == aspect && impl_->scaling == scaling && impl_->cropLines == cropLines) return;
  impl_->aspect = aspect;
  impl_->scaling = scaling;
  impl_->cropLines = cropLines;
  impl_->render();
}

void ScreenWindow::setHoles(const std::vector<shared::ScreenRect>& holes) {
  if (holes == impl_->holes) return;
  impl_->holes = holes;
  impl_->applyRegion();
}

void ScreenWindow::setFrame(const std::uint32_t* pixels, std::uint32_t width, std::uint32_t height) {
  Impl& self = *impl_;
  if (width == 0 || height == 0 || pixels == nullptr) return;
  if (!self.device) {
    if (self.rect.width <= 0) return;
    self.recoverDevice();
  }
  if (!self.frameTexture || self.frameWidth != width || self.frameHeight != height) {
    self.frameTexture.Reset();
    self.frameView.Reset();
    D3D11_TEXTURE2D_DESC description{};
    description.Width = width;
    description.Height = height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DYNAMIC;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(self.device->CreateTexture2D(&description, nullptr, &self.frameTexture))) return;
    if (FAILED(self.device->CreateShaderResourceView(self.frameTexture.Get(), nullptr, &self.frameView))) {
      self.frameTexture.Reset();
      return;
    }
    self.frameWidth = width;
    self.frameHeight = height;
  }
  D3D11_MAPPED_SUBRESOURCE mapped{};
  if (FAILED(self.context->Map(self.frameTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
  const std::size_t rowBytes = static_cast<std::size_t>(width) * sizeof(std::uint32_t);
  auto* destination = static_cast<std::uint8_t*>(mapped.pData);
  for (std::uint32_t row = 0; row < height; ++row) std::memcpy(destination + static_cast<std::size_t>(row) * mapped.RowPitch, pixels + static_cast<std::size_t>(row) * width, rowBytes);
  self.context->Unmap(self.frameTexture.Get(), 0);
}

void ScreenWindow::setFrameVisible(bool visible) {
  if (impl_->frameVisible == visible) return;
  impl_->frameVisible = visible;
  impl_->render();
}

void ScreenWindow::setOverlay(const std::uint8_t* pixels, int width, int height, const shared::ScreenRect& glass) {
  Impl& self = *impl_;
  self.overlayView.Reset();
  self.glass = pixels != nullptr ? glass : shared::ScreenRect{};
  if (pixels != nullptr && width > 0 && height > 0 && self.device) {
    D3D11_TEXTURE2D_DESC description{};
    description.Width = static_cast<UINT>(width);
    description.Height = static_cast<UINT>(height);
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_IMMUTABLE;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    const D3D11_SUBRESOURCE_DATA data{pixels, static_cast<UINT>(width) * 4, 0};
    ComPtr<ID3D11Texture2D> texture;
    if (SUCCEEDED(self.device->CreateTexture2D(&description, &data, &texture))) self.device->CreateShaderResourceView(texture.Get(), nullptr, &self.overlayView);
  }
  self.render();
}

void ScreenWindow::render() { impl_->render(); }

}  // namespace ember::win32::platform
