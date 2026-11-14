/*
 * ------------------------------------------------------------------------
 * 名称：webgl-renderer.ts
 * 说明：WebGL 渲染器：在 OffscreenCanvas 上用一张 RGBA8 纹理显示帧，每帧用 texSubImage2D 上传，着色器做缩放，最近邻采样，画面之外是黑底。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：只在渲染 Worker 里使用。帧像素的第 0 行在画面最上面，纹理坐标的 v 随行号增加，所以着色器里把 y 翻转，不用 UNPACK_FLIP_Y。
 *       WebGL 上下文丢失时画面暂停，恢复后重建资源，下一帧起继续。
 * ------------------------------------------------------------------------
 */

import { BYTES_PER_PIXEL } from './frame-receiver';
import type { ScreenRect } from './screen-layout';

/** 顶点着色器：画面矩形由视口决定，这里只铺满视口。 */
const VERTEX_SHADER = `
attribute vec2 a_position;
varying vec2 v_uv;
void main() {
  gl_Position = vec4(a_position * 2.0 - 1.0, 0.0, 1.0);
  v_uv = vec2(a_position.x, 1.0 - a_position.y);
}`;

/** 片元着色器：最近邻采样；u_crop 是上下各裁掉的比例（占整帧高度）。 */
const FRAGMENT_SHADER = `
precision mediump float;
uniform sampler2D u_texture;
uniform vec2 u_crop;
varying vec2 v_uv;
void main() {
  vec2 uv = vec2(v_uv.x, u_crop.x + v_uv.y * (1.0 - u_crop.x - u_crop.y));
  gl_FragColor = vec4(texture2D(u_texture, uv).rgb, 1.0);
}`;

/** 要画的一帧：尺寸与像素。 */
export interface DrawableFrame {
  readonly width: number;
  readonly height: number;
  readonly pixels: ArrayBuffer;
}

/** 编译一个着色器；失败时抛出带日志的异常。 */
function compileShader(gl: WebGLRenderingContext, type: number, source: string): WebGLShader {
  const shader = gl.createShader(type);
  if (shader === null) throw new Error('无法创建着色器');
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  if (gl.getShaderParameter(shader, gl.COMPILE_STATUS) !== true) throw new Error(`着色器编译失败：${gl.getShaderInfoLog(shader) ?? ''}`);
  return shader;
}

/** WebGL 渲染器。 */
export class WebGlFrameRenderer {
  private readonly gl: WebGLRenderingContext;
  private program: WebGLProgram | null = null;
  private cropLocation: WebGLUniformLocation | null = null;
  private texture: WebGLTexture | null = null;
  private buffer: WebGLBuffer | null = null;
  private textureWidth = 0;
  private textureHeight = 0;
  private lost = false;

  /**
   * @param canvas 离屏画布。
   * @throws 浏览器不支持 WebGL 或着色器编译失败时抛出异常。
   */
  constructor(private readonly canvas: OffscreenCanvas) {
    const gl = canvas.getContext('webgl', { alpha: false, antialias: false, depth: false, stencil: false, preserveDrawingBuffer: false, powerPreference: 'high-performance' });
    if (gl === null) throw new Error('浏览器不支持 WebGL');
    this.gl = gl;
    canvas.addEventListener('webglcontextlost', (event) => {
      event.preventDefault();
      this.lost = true;
    });
    canvas.addEventListener('webglcontextrestored', () => {
      this.lost = false;
      this.createResources();
    });
    this.createResources();
  }

  /** 画布的设备像素尺寸。 */
  get size(): { readonly width: number; readonly height: number } {
    return { width: this.canvas.width, height: this.canvas.height };
  }

  /** 调整画布尺寸；改变尺寸会清空画布，调用方要重画。 */
  resize(width: number, height: number): void {
    if (this.canvas.width !== width) this.canvas.width = width;
    if (this.canvas.height !== height) this.canvas.height = height;
  }

  /**
   * 画一帧：上传纹理，清成黑底，再把画面画在矩形里。
   * @param frame 要上传的新帧；传 undefined 时重画纹理里已有的上一帧（版面变化时用，不重新上传）。
   * @param rect 画面矩形（设备像素，左下角为原点）。
   * @param cropLines 帧上下各裁掉的行数，默认 0（显示全部行）。
   */
  draw(frame: DrawableFrame | undefined, rect: ScreenRect, cropLines = 0): void {
    if (this.lost || this.program === null) return;
    const { gl } = this;
    if (frame !== undefined) this.upload(frame);
    gl.viewport(0, 0, this.canvas.width, this.canvas.height);
    gl.clearColor(0, 0, 0, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
    if (this.textureWidth === 0 || rect.width <= 0 || rect.height <= 0) return;
    const crop = cropLines / this.textureHeight;
    gl.uniform2f(this.cropLocation, crop, crop);
    gl.viewport(rect.x, rect.y, rect.width, rect.height);
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
  }

  /** 创建程序、顶点缓冲与纹理；上下文恢复后也调用。 */
  private createResources(): void {
    const { gl } = this;
    const program = gl.createProgram();
    if (program === null) throw new Error('无法创建着色器程序');
    gl.attachShader(program, compileShader(gl, gl.VERTEX_SHADER, VERTEX_SHADER));
    gl.attachShader(program, compileShader(gl, gl.FRAGMENT_SHADER, FRAGMENT_SHADER));
    gl.bindAttribLocation(program, 0, 'a_position');
    gl.linkProgram(program);
    if (gl.getProgramParameter(program, gl.LINK_STATUS) !== true) throw new Error(`着色器程序链接失败：${gl.getProgramInfoLog(program) ?? ''}`);
    gl.useProgram(program);
    gl.uniform1i(gl.getUniformLocation(program, 'u_texture'), 0);
    this.cropLocation = gl.getUniformLocation(program, 'u_crop');

    this.buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, this.buffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([0, 0, 1, 0, 0, 1, 1, 1]), gl.STATIC_DRAW);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);

    this.texture = gl.createTexture();
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, this.texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    this.textureWidth = 0;
    this.textureHeight = 0;
    this.program = program;
  }

  /** 把帧上传到纹理：尺寸变了先重新分配，之后每帧只用 texSubImage2D 更新。 */
  private upload(frame: DrawableFrame): void {
    const { gl } = this;
    if (frame.width !== this.textureWidth || frame.height !== this.textureHeight) {
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, frame.width, frame.height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
      this.textureWidth = frame.width;
      this.textureHeight = frame.height;
    }
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, frame.width, frame.height, gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array(frame.pixels, 0, frame.width * frame.height * BYTES_PER_PIXEL));
  }
}
