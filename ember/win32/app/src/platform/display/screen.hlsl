// ------------------------------------------------------------------------
// 名称：screen.hlsl
// 说明：运行画面的着色器：顶点着色器把已经算好的位置与纹理坐标原样交给光栅化，像素着色器按采样器取纹理的颜色。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-19
// 备注：用 level_9_1 的档次编译，在 Direct3D 11 的全部功能级别（9_1 到 11_0）上都能运行；
//       干净的 Windows 7 没有 d3dcompiler_47.dll，所以不在运行时编译：由 ember/tools/build/gen-screen-shaders.mjs 用 fxc 编译成字节码，
//       写进 screen-shaders.generated.cpp（提交进仓库）。改了这个文件要重新运行那个脚本。
// ------------------------------------------------------------------------

struct VertexInput {
  float2 position : POSITION;
  float2 uv : TEXCOORD0;
};

struct PixelInput {
  float4 position : SV_Position;
  float2 uv : TEXCOORD0;
};

Texture2D screenTexture : register(t0);
SamplerState screenSampler : register(s0);

PixelInput vertexMain(VertexInput input) {
  PixelInput output;
  output.position = float4(input.position, 0.0, 1.0);
  output.uv = input.uv;
  return output;
}

float4 pixelMain(PixelInput input) : SV_Target {
  return screenTexture.Sample(screenSampler, input.uv);
}
