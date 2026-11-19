/*
 * ------------------------------------------------------------------------
 * 名称：win32-ui-kit-tools.test.ts
 * 说明：Win32 控件库的 Node 工具测试：CSS 解析与求值、令牌与图标生成结果和已提交文件一致、PNG 解码、截图对照函数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：求值与解析既测正常写法，也测不支持的写法必须抛出异常（不跳过、不猜测）。
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { deflateSync } from 'node:zlib';
import { afterEach, describe, expect, it } from 'vitest';
import { compareLayout, comparePixels, contentBox, cropTo } from '../win32/ui-kit/tools/reference/compare-images.mjs';
import { decodePng } from '../win32/ui-kit/tools/reference/png.mjs';
import { CssValueError, Evaluator, parseRules, parseValue, splitTopLevel, stripComments } from '../win32/ui-kit/tools/lib/css.mjs';
import { DATA_FILE, generate as generateIcons, ID_FILE, RULES_FILE } from '../win32/ui-kit/tools/gen-icons.mjs';
import { generate as generateTokens, OUTPUT_FILE } from '../win32/ui-kit/tools/gen-tokens.mjs';

const directories: string[] = [];

afterEach(() => {
  for (const directory of directories.splice(0)) rmSync(directory, { recursive: true, force: true });
});

/** 用一组自定义属性建求值环境。 */
function evaluator(variables: Record<string, string>): InstanceType<typeof Evaluator> {
  return new Evaluator(new Map(Object.entries(variables)));
}

describe('CSS 文本处理', () => {
  it('去掉注释且保留其余内容', () => {
    expect(stripComments('a /* x */ b /* 多行\n注释 */ c')).toBe('a  b  c');
  });

  it('按顶层逗号拆分：括号与引号里的逗号不拆，空段丢弃', () => {
    expect(splitTopLevel("a, rgb(1, 2, 3), 'x,y', , b", ',')).toEqual(['a', 'rgb(1, 2, 3)', "'x,y'", 'b']);
  });

  it('解析规则、@media 里的规则与 @font-face', () => {
    const rules = parseRules(`
      :root { --a: 1px; --b: calc(var(--a) * 2); }
      @media (max-width: 10px) { .x { color: red; } }
      @font-face { font-family: 'F'; }
      @import 'x.css';
    `);
    expect(rules.map((rule) => rule.selector)).toEqual([':root', '.x', '@font-face']);
    expect(rules[0]?.declarations.get('--b')).toBe('calc(var(--a) * 2)');
    expect(rules[1]?.media).toBe('(max-width: 10px)');
  });

  it('同一属性后面的声明覆盖前面的', () => {
    const rules = parseRules('.a { color: red; color: blue; }');
    expect(rules[0]?.declarations.get('color')).toBe('blue');
  });

  it('解析取值：数字、单位、十六进制、字符串、函数与逗号', () => {
    const nodes = parseValue("12px #fff 'a b', var(--x, 3)");
    expect(nodes.map((node: { t: string }) => node.t)).toEqual(['num', 'hex', 'str', 'comma', 'fn']);
  });

  it('括号不配对与没有结束的字符串抛出异常', () => {
    expect(() => parseValue('calc(1 + 2')).toThrow();
    expect(() => parseValue("'abc")).toThrow();
  });
});

describe('CSS 求值', () => {
  it('长度、数字、时间、百分比与 em', () => {
    const e = evaluator({});
    expect(e.evaluateText('12px')).toEqual({ k: 'length', v: 12 });
    expect(e.evaluateText('1.5')).toEqual({ k: 'number', v: 1.5 });
    expect(e.evaluateText('0.15s')).toEqual({ k: 'time', v: 150 });
    expect(e.evaluateText('200ms')).toEqual({ k: 'time', v: 200 });
    expect(e.evaluateText('50%')).toEqual({ k: 'percent', v: 50 });
    expect(e.evaluateText('0.5em')).toEqual({ k: 'em', v: 0.5 });
  });

  it('var 取自定义属性，没有时用备用值', () => {
    const e = evaluator({ '--a': '4px' });
    expect(e.evaluateText('var(--a)')).toEqual({ k: 'length', v: 4 });
    expect(e.evaluateText('var(--missing, 7px)')).toEqual({ k: 'length', v: 7 });
    expect(e.evaluateText('var(--missing, var(--a))')).toEqual({ k: 'length', v: 4 });
  });

  it('calc 支持加减乘除、括号与负号', () => {
    const e = evaluator({ '--a': '10px' });
    expect(e.evaluateText('calc(var(--a) * 2 + 4px)')).toEqual({ k: 'length', v: 24 });
    expect(e.evaluateText('calc((1px + 3px) / 2)')).toEqual({ k: 'length', v: 2 });
    expect(e.evaluateText('calc(-1 * var(--a))')).toEqual({ k: 'length', v: -10 });
  });

  it('max、min 与 clamp', () => {
    const e = evaluator({});
    expect(e.evaluateText('max(2px, 5px)')).toEqual({ k: 'length', v: 5 });
    expect(e.evaluateText('min(2px, 5px)')).toEqual({ k: 'length', v: 2 });
    expect(e.evaluateText('clamp(1px, 9px, 4px)')).toEqual({ k: 'length', v: 4 });
    expect(e.evaluateText('clamp(3px, 1px, 4px)')).toEqual({ k: 'length', v: 3 });
  });

  it('颜色：十六进制、rgb、rgba、斜杠透明度与 transparent', () => {
    const e = evaluator({});
    expect(e.evaluateText('#abc')).toEqual({ k: 'color', r: 170, g: 187, b: 204, a: 1 });
    expect(e.evaluateText('#11223380')).toMatchObject({ k: 'color', r: 17, g: 34, b: 51 });
    expect(e.evaluateText('rgb(1, 2, 3)')).toEqual({ k: 'color', r: 1, g: 2, b: 3, a: 1 });
    expect(e.evaluateText('rgba(1, 2, 3, 0.5)')).toEqual({ k: 'color', r: 1, g: 2, b: 3, a: 0.5 });
    expect(e.evaluateText('rgb(1 2 3 / 25%)')).toEqual({ k: 'color', r: 1, g: 2, b: 3, a: 0.25 });
    expect(e.evaluateText('transparent')).toEqual({ k: 'color', r: 0, g: 0, b: 0, a: 0 });
  });

  it('color-mix：默认各一半、指定比例、比例由 var 给出', () => {
    const e = evaluator({ '--p': '25%' });
    expect(e.evaluateText('color-mix(in srgb, #000000, #ffffff)')).toMatchObject({ r: 127.5, g: 127.5, b: 127.5, a: 1 });
    expect(e.evaluateText('color-mix(in srgb, #000000 25%, #ffffff)')).toMatchObject({ r: 191.25 });
    expect(e.evaluateText('color-mix(in srgb, #000000 var(--p), #ffffff)')).toMatchObject({ r: 191.25 });
    expect(e.evaluateText('color-mix(in srgb, #000000 50%, transparent)')).toMatchObject({ r: 0, a: 0.5 });
  });

  it('逗号与空格组成的列表保留类型', () => {
    const value = evaluator({}).evaluateText("'A', 'B' 1px");
    expect(value).toMatchObject({ k: 'list', sep: ',' });
  });

  it('不支持的写法都抛出异常', () => {
    const e = evaluator({ '--loop': 'var(--loop)', '--a': 'var(--b)', '--b': 'var(--a)' });
    expect(() => e.evaluateText('1rem')).toThrow(CssValueError);
    expect(() => e.evaluateText('hsl(1, 2%, 3%)')).toThrow(CssValueError);
    expect(() => e.evaluateText('var(--none)')).toThrow(CssValueError);
    expect(() => e.evaluateText('var(foo)')).toThrow(CssValueError);
    expect(() => e.evaluateText('color-mix(in oklab, #000, #fff)')).toThrow(CssValueError);
    expect(() => e.evaluateText('max(1px, 2)')).toThrow(CssValueError);
    expect(() => e.evaluateText('clamp(1px, 2px)')).toThrow(CssValueError);
    expect(() => e.evaluateText('calc(1px +)')).toThrow(CssValueError);
    expect(() => e.evaluateText('#12')).toThrow(/不认识的颜色/);
    expect(() => e.resolve('--loop')).toThrow(/循环引用/);
    expect(() => e.resolve('--a')).toThrow(/循环引用/);
    expect(() => e.resolve('--nope')).toThrow(/没有定义/);
  });
});

describe('生成结果与已提交文件', () => {
  it('令牌头文件与生成结果一致', () => {
    expect(readFileSync(OUTPUT_FILE, 'utf8')).toBe(generateTokens());
  });

  it('图标编号与路径数据与生成结果一致', () => {
    const { id, data, rules, count } = generateIcons();
    expect(readFileSync(ID_FILE, 'utf8')).toBe(id);
    expect(readFileSync(DATA_FILE, 'utf8')).toBe(data);
    expect(readFileSync(RULES_FILE, 'utf8')).toBe(rules);
    expect(count).toBeGreaterThan(0);
  });

  it('按文字匹配图标的规则按书写顺序生成，且带位置标记', () => {
    const { rules } = generateIcons();
    expect(rules).toContain('{"^保存", IconId::DeviceFloppy, false},');
    expect(rules).toContain('{"^下一步", IconId::ArrowRight, true},');
    expect(rules.indexOf('"^取消生成"')).toBeLessThan(rules.indexOf('"^(取消|关闭)"'));
  });

  it('生成的令牌含按钮用到的尺寸与两个主题', () => {
    const text = generateTokens();
    expect(text).toContain('inline constexpr double btn_height = 22.0;');
    expect(text).toContain('inline constexpr ThemeTokens dark{');
    expect(text).toContain('inline constexpr ThemeTokens light{');
  });
});

/** 构造一个 PNG 数据块（CRC 填 0：解码器不校验）。 */
function chunk(type: string, body: Buffer): Buffer {
  const length = Buffer.alloc(4);
  length.writeUInt32BE(body.length);
  return Buffer.concat([length, Buffer.from(type, 'ascii'), body, Buffer.alloc(4)]);
}

/** 把 RGBA 像素编码成 PNG（每行用指定的滤波类型，CRC 填 0：解码器不校验）。 */
function encodePng(width: number, height: number, rgba: Uint8Array, filterOf: (y: number) => number, channels = 4): Buffer {
  const stride = width * channels;
  const rows: Buffer[] = [];
  for (let y = 0; y < height; y += 1) {
    const filter = filterOf(y);
    const row = Buffer.alloc(stride + 1);
    row[0] = filter;
    for (let x = 0; x < stride; x += 1) {
      const raw = (y * width + Math.floor(x / channels)) * 4 + (x % channels);
      const value = rgba[raw] ?? 0;
      const at = (dx: number, dy: number): number => {
        const column = x - dx * channels;
        const line = y - dy;
        if (column < 0 || line < 0) return 0;
        return rgba[(line * width + Math.floor(column / channels)) * 4 + (column % channels)] ?? 0;
      };
      const left = at(1, 0);
      const up = at(0, 1);
      const upLeft = at(1, 1);
      let predictor = 0;
      if (filter === 1) predictor = left;
      else if (filter === 2) predictor = up;
      else if (filter === 3) predictor = (left + up) >> 1;
      else if (filter === 4) {
        const estimate = left + up - upLeft;
        const dl = Math.abs(estimate - left);
        const du = Math.abs(estimate - up);
        const dul = Math.abs(estimate - upLeft);
        predictor = dl <= du && dl <= dul ? left : du <= dul ? up : upLeft;
      }
      row[x + 1] = (value - predictor) & 0xff;
    }
    rows.push(row);
  }
  const header = Buffer.alloc(13);
  header.writeUInt32BE(width, 0);
  header.writeUInt32BE(height, 4);
  header[8] = 8;
  header[9] = channels === 4 ? 6 : 2;
  return Buffer.concat([Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]), chunk('IHDR', header), chunk('IDAT', deflateSync(Buffer.concat(rows))), chunk('IEND', Buffer.alloc(0))]);
}

/** 生成测试用的像素：每个通道随位置变化。 */
function pattern(width: number, height: number): Uint8Array {
  const data = new Uint8Array(width * height * 4);
  for (let i = 0; i < width * height; i += 1) {
    data[i * 4] = (i * 7) & 0xff;
    data[i * 4 + 1] = (i * 13 + 5) & 0xff;
    data[i * 4 + 2] = (i * 29 + 11) & 0xff;
    data[i * 4 + 3] = 255 - ((i * 3) & 0x7f);
  }
  return data;
}

function writeTemp(name: string, content: Buffer | string): string {
  const directory = mkdtempSync(join(tmpdir(), 'ember-ui-tools-'));
  directories.push(directory);
  const file = join(directory, name);
  writeFileSync(file, content);
  return file;
}

describe('PNG 解码', () => {
  it.each([0, 1, 2, 3, 4])('滤波类型 %i 解码后与原像素一致', (filter) => {
    const source = pattern(9, 6);
    const file = writeTemp('a.png', encodePng(9, 6, source, () => filter));
    const image = decodePng(file);
    expect(image.width).toBe(9);
    expect(image.height).toBe(6);
    expect([...image.data]).toEqual([...source]);
  });

  it('每行用不同的滤波类型也能解码', () => {
    const source = pattern(5, 5);
    const image = decodePng(writeTemp('b.png', encodePng(5, 5, source, (y) => y % 5)));
    expect([...image.data]).toEqual([...source]);
  });

  it('RGB（无透明通道）补成不透明', () => {
    const source = pattern(4, 3);
    const image = decodePng(writeTemp('c.png', encodePng(4, 3, source, () => 1, 3)));
    expect(image.data[3]).toBe(255);
    expect(image.data[0]).toBe(source[0]);
    expect(image.data[4]).toBe(source[4]);
  });

  it('不是 PNG 或格式不支持时抛出异常', () => {
    expect(() => decodePng(writeTemp('d.png', 'not a png'))).toThrow(/不是 PNG/);
    const bad = encodePng(2, 2, pattern(2, 2), () => 0);
    bad[8 + 8 + 8] = 16;
    expect(() => decodePng(writeTemp('e.png', bad))).toThrow(/不受支持/);
  });
});

/** 构造纯色图。 */
function solid(width: number, height: number, color: [number, number, number]) {
  const data = new Uint8Array(width * height * 4);
  for (let i = 0; i < width * height; i += 1) data.set([...color, 255], i * 4);
  return { width, height, data };
}

describe('截图对照函数', () => {
  it('位置对照给出每个控件的偏差与最大值', () => {
    const result = compareLayout([{ x: 0, y: 0, width: 10, height: 10 }], [{ x: 0.5, y: -1, width: 10, height: 11 }]);
    expect(result.maxDelta).toBe(1);
    expect(result.deltas[0]).toEqual({ index: 0, x: 0.5, y: -1, width: 0, height: 1 });
  });

  it('控件个数不同时抛出异常', () => {
    expect(() => compareLayout([{ x: 0, y: 0, width: 1, height: 1 }], [])).toThrow(/个数不同/);
  });

  it('像素对照：相同的图没有差异', () => {
    const image = solid(4, 4, [10, 20, 30]);
    expect(comparePixels(image, image, [], 0)).toMatchObject({ total: 16, mismatched: 0, maxDiff: 0, worst: null });
  });

  it('像素对照：统计超过容差的像素，记录最大差与位置，排除区域不参与', () => {
    const a = solid(4, 4, [10, 20, 30]);
    const b = solid(4, 4, [10, 20, 30]);
    b.data.set([10, 20, 40, 255], (1 * 4 + 2) * 4);
    b.data.set([10, 20, 33, 255], (3 * 4 + 3) * 4);
    const full = comparePixels(a, b, [], 5);
    expect(full).toMatchObject({ total: 16, mismatched: 1, maxDiff: 10, worst: { x: 2, y: 1 } });
    const masked = comparePixels(a, b, [{ left: 2, top: 1, right: 3, bottom: 2 }], 5);
    expect(masked).toMatchObject({ total: 15, mismatched: 0, maxDiff: 3 });
  });

  it('像素对照：两张图大小不同时抛出异常', () => {
    expect(() => comparePixels(solid(2, 2, [0, 0, 0]), solid(3, 2, [0, 0, 0]), [], 0)).toThrow(/大小不同/);
  });

  it('内容外框：只框住与背景不同的像素；没有内容时为空', () => {
    const image = solid(10, 10, [0, 0, 0]);
    image.data.set([255, 255, 255, 255], (3 * 10 + 4) * 4);
    image.data.set([255, 255, 255, 255], (6 * 10 + 7) * 4);
    expect(contentBox(image, { left: 0, top: 0, right: 10, bottom: 10 }, [0, 0, 0], 40)).toEqual({ left: 4, top: 3, right: 8, bottom: 7 });
    expect(contentBox(image, { left: 0, top: 0, right: 3, bottom: 3 }, [0, 0, 0], 40)).toBeNull();
    expect(contentBox(image, { left: 0, top: 0, right: 10, bottom: 10 }, [0, 0, 0], 800)).toBeNull();
  });

  it('裁剪保留左上角；目标比原图大时抛出异常', () => {
    const source = { width: 3, height: 2, data: new Uint8Array(pattern(3, 2)) };
    const cropped = cropTo(source, 2, 1);
    expect(cropped.width).toBe(2);
    expect(cropped.height).toBe(1);
    expect([...cropped.data]).toEqual([...source.data.subarray(0, 8)]);
    expect(() => cropTo(source, 4, 2)).toThrow(/无法/);
  });
});
