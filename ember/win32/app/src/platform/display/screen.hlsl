// ------------------------------------------------------------------------
// 名称：screen.hlsl
// 说明：运行画面的着色器：顶点着色器把已经算好的位置与纹理坐标原样交给光栅化，像素着色器按采样器取纹理的颜色；毛玻璃的像素着色器对画面纹理做模糊。
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

// 毛玻璃：对画面纹理做 17 点采样的模糊（中心、内圈 8 点、外圈 8 点，权重 4:2:1），圆角矩形之外的像素丢弃（保持清晰）。
// glassParams[0].xy 是内圈半径对应的纹理坐标增量，外圈是它的两倍；共 17 次采样，在 level_9_1 的纹理指令上限之内；
// level_9_1 的像素着色器拿不到屏幕位置，所以由纹理坐标换算：像素位置 = glassParams[0].zw + uv * glassParams[1].xy；
// glassParams[1].zw 是圆角框的中心，glassParams[2].xy 是框的半宽半高减去圆角半径，glassParams[2].z 是圆角半径（像素，相对子窗口）。
cbuffer GlassConstants : register(b0) {
  float4 glassParams[3];
}

float4 glassMain(PixelInput input) : SV_Target {
  const float2 pixel = glassParams[0].zw + input.uv * glassParams[1].xy;
  const float2 corner = abs(pixel - glassParams[1].zw) - glassParams[2].xy;
  clip(glassParams[2].z - length(max(corner, 0.0)));
  const float2 inner = glassParams[0].xy;
  const float2 outer = glassParams[0].xy * 2.0;
  const float2 diagonalInner = inner * 0.7071;
  const float2 diagonalOuter = outer * 0.7071;
  float4 sum = screenTexture.Sample(screenSampler, input.uv) * 4.0;
  sum += (screenTexture.Sample(screenSampler, input.uv + float2(inner.x, 0.0)) + screenTexture.Sample(screenSampler, input.uv - float2(inner.x, 0.0)) +
          screenTexture.Sample(screenSampler, input.uv + float2(0.0, inner.y)) + screenTexture.Sample(screenSampler, input.uv - float2(0.0, inner.y)) +
          screenTexture.Sample(screenSampler, input.uv + diagonalInner) + screenTexture.Sample(screenSampler, input.uv - diagonalInner) +
          screenTexture.Sample(screenSampler, input.uv + float2(diagonalInner.x, -diagonalInner.y)) + screenTexture.Sample(screenSampler, input.uv + float2(-diagonalInner.x, diagonalInner.y))) * 2.0;
  sum += screenTexture.Sample(screenSampler, input.uv + float2(outer.x, 0.0)) + screenTexture.Sample(screenSampler, input.uv - float2(outer.x, 0.0)) +
         screenTexture.Sample(screenSampler, input.uv + float2(0.0, outer.y)) + screenTexture.Sample(screenSampler, input.uv - float2(0.0, outer.y)) +
         screenTexture.Sample(screenSampler, input.uv + diagonalOuter) + screenTexture.Sample(screenSampler, input.uv - diagonalOuter) +
         screenTexture.Sample(screenSampler, input.uv + float2(diagonalOuter.x, -diagonalOuter.y)) + screenTexture.Sample(screenSampler, input.uv + float2(-diagonalOuter.x, diagonalOuter.y));
  return float4((sum / 28.0).rgb, 1.0);
}
