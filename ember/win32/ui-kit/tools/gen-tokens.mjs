/*
 * ------------------------------------------------------------------------
 * 名称：gen-tokens.mjs
 * 说明：令牌生成器：读取 rujian-ui-kit 的令牌与主题，把已登记的组件引用到的全部令牌求成最终取值，生成 src/generated/tokens.generated.hpp。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：长度、数字、时间等不随主题变化的取值生成成命名空间里的常量；颜色与阴影按主题生成成 ThemeTokens 的字段；
 *       不认识的写法与随主题变化的非颜色取值直接报错；有意不生成的令牌登记在 tools/tokens-ignore.json，并写明原因。
 *       用法：node tools/gen-tokens.mjs 写文件，node tools/gen-tokens.mjs --check 只检查已提交的文件与生成结果一致。
 * ------------------------------------------------------------------------
 */

import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { Evaluator, parseRules } from './lib/css.mjs';
import { findWebKit, KIT_ROOT, readComponentStyles, readStyleVariables, referencedVariables, TOOLS_ROOT } from './lib/kit-source.mjs';

/** 生成文件的位置。 */
export const OUTPUT_FILE = join(KIT_ROOT, 'src', 'generated', 'tokens.generated.hpp');

/**
 * 读取忽略清单。
 * @returns {{ names: Map<string, string>, patterns: { regex: RegExp, reason: string }[] }} 按名称与按模式登记的忽略项。
 */
function readIgnoreList() {
  const raw = JSON.parse(readFileSync(join(TOOLS_ROOT, 'tokens-ignore.json'), 'utf8'));
  const names = new Map();
  const patterns = [];
  for (const item of raw.ignored) {
    if (item.name) names.set(item.name, item.reason);
    else patterns.push({ regex: new RegExp(item.pattern), reason: item.reason });
  }
  return { names, patterns };
}

/**
 * 令牌名转成 C++ 标识符：去掉前面的 --，连字符换成下划线。
 * @param {string} name 令牌名。
 * @returns {string} 标识符。
 */
function identifier(name) {
  return name.replace(/^--/, '').replaceAll('-', '_');
}

/**
 * 数字转成 C++ 浮点字面量。
 * @param {number} value 数值。
 * @returns {string} 字面量。
 */
function number(value) {
  const rounded = Math.round(value * 1e6) / 1e6;
  return Number.isInteger(rounded) ? `${rounded}.0` : String(rounded);
}

/**
 * 把颜色分量限制在 0 到 255 并取整。
 * @param {number} value 分量。
 * @returns {number} 整数分量。
 */
function clampChannel(value) {
  return Math.max(0, Math.min(255, Math.round(value)));
}

/**
 * 颜色值转成 C++ 的 Color 字面量（0xAARRGGBB）。
 * @param {{ r: number, g: number, b: number, a: number }} color 颜色。
 * @returns {string} 表达式。
 */
function colorLiteral(color) {
  const argb = ((clampChannel(color.a * 255) << 24) | (clampChannel(color.r) << 16) | (clampChannel(color.g) << 8) | clampChannel(color.b)) >>> 0;
  return `Color{0x${argb.toString(16).toUpperCase().padStart(8, '0')}u}`;
}

/**
 * 判断一个列表是否是一条阴影（偏移、模糊、可选扩展、颜色）。
 * @param {object} value 值。
 * @returns {boolean} 是否是阴影。
 */
function isShadow(value) {
  if (value.k !== 'list' || value.sep !== ' ') return false;
  const items = value.items;
  const last = items[items.length - 1];
  const leading = items.filter((item) => item.k === 'ident');
  const scalars = items.filter((item) => item.k === 'length' || item.k === 'number');
  return last.k === 'color' && scalars.length >= 2 && scalars.length <= 4 && leading.every((item) => item.v === 'inset') && scalars.length + leading.length + 1 === items.length;
}

/**
 * 一条阴影转成 C++ 表达式。
 * @param {object} value 阴影值。
 * @returns {string} 表达式。
 */
function shadowLiteral(value) {
  const scalars = value.items.filter((item) => item.k === 'length' || item.k === 'number').map((item) => item.v);
  const inset = value.items.some((item) => item.k === 'ident');
  const color = value.items[value.items.length - 1];
  return `Shadow{${number(scalars[0])}, ${number(scalars[1])}, ${number(scalars[2] ?? 0)}, ${number(scalars[3] ?? 0)}, ${colorLiteral(color)}, ${inset}}`;
}

/**
 * 把一个令牌在各主题下的取值归类，决定输出形式。
 * @param {string} name 令牌名。
 * @param {Map<string, object>} perTheme 主题名到值。
 * @returns {{ kind: string, values: Map<string, object> }} 类别：scalar、color、shadows、numbers、fonts。
 */
function classify(name, perTheme) {
  const values = [...perTheme.values()];
  const first = values[0];
  if (['length', 'number', 'time', 'percent', 'em'].includes(first.k)) {
    if (values.some((value) => value.k !== first.k || value.v !== first.v)) throw new Error(`${name} 的取值随主题变化，但不是颜色或阴影`);
    return { kind: 'scalar', values: perTheme };
  }
  if (values.every((value) => value.k === 'color' || (value.k === 'ident' && value.v === 'currentColor'))) return { kind: 'color', values: perTheme };
  if (values.every((value) => isShadow(value) || (value.k === 'ident' && value.v === 'none') || (value.k === 'list' && value.sep === ',' && value.items.every(isShadow))) && values.some((value) => value.k === 'list')) return { kind: 'shadows', values: perTheme };
  if (values.every((value) => value.k === 'list' && value.sep === ' ' && value.items.every((item) => item.k === 'length' || item.k === 'number'))) return { kind: 'numbers', values: perTheme };
  if (values.every((value) => value.k === 'list' && value.sep === ',')) return { kind: 'fonts', values: perTheme };
  throw new Error(`${name} 的取值类型不支持：${JSON.stringify(first).slice(0, 120)}`);
}

/**
 * 把字体列表展开成字体名数组（嵌套列表展开，去掉重复）。
 * @param {object} value 列表值。
 * @returns {string[]} 字体名，原样保留关键字。
 */
function flattenFonts(value) {
  const names = [];
  const visit = (node) => {
    if (node.k === 'list') node.items.forEach(visit);
    else if (node.k === 'string' || node.k === 'ident') names.push(node.v);
    else throw new Error('字体列表里只能是字体名');
  };
  visit(value);
  return [...new Set(names)];
}

/**
 * 阴影值转成列表。
 * @param {object} value 阴影、none 或阴影列表。
 * @returns {object[]} 阴影值数组。
 */
function shadowItems(value) {
  if (value.k === 'ident') return [];
  return isShadow(value) ? [value] : value.items;
}

/** 一个令牌的阴影最多条数，与 ShadowList 的容量一致。 */
const MAX_SHADOWS = 3;

/**
 * 生成头文件的文本。
 * @returns {string} 文本。
 */
export function generate() {
  const webKit = findWebKit();
  const { base, themes } = readStyleVariables(webKit);
  const ignore = readIgnoreList();
  const referenced = new Set();
  const componentLocal = new Set();
  for (const { text } of readComponentStyles(webKit)) {
    for (const name of referencedVariables(text)) referenced.add(name);
    for (const rule of parseRules(text)) for (const name of rule.declarations.keys()) if (name.startsWith('--')) componentLocal.add(name);
  }

  const used = [...referenced].filter((name) => base.has(name) || [...themes.values()].some((theme) => theme.has(name))).toSorted();
  const missing = [...referenced].filter((name) => !used.includes(name) && !componentLocal.has(name) && !ignore.names.has(name) && !ignore.patterns.some((pattern) => pattern.regex.test(name)));
  if (missing.length > 0) throw new Error(`这些令牌被组件引用但没有定义，也没有登记到 tokens-ignore.json：${missing.join('、')}`);

  const themeNames = [...themes.keys()].toSorted();
  const evaluators = new Map(themeNames.map((theme) => [theme, new Evaluator(new Map([...base, ...themes.get(theme)]))]));
  const scalars = [];
  const themed = [];
  const fonts = [];
  const numberLists = [];
  for (const name of used) {
    if (ignore.names.has(name) || ignore.patterns.some((pattern) => pattern.regex.test(name))) continue;
    const perTheme = new Map(themeNames.map((theme) => [theme, evaluators.get(theme).resolve(name)]));
    const { kind } = classify(name, perTheme);
    const id = identifier(name);
    const sample = perTheme.get(themeNames[0]);
    if (kind === 'scalar') scalars.push(`inline constexpr double ${id}${sample.k === 'time' ? '_ms' : sample.k === 'em' ? '_em' : sample.k === 'percent' ? '_percent' : ''} = ${number(sample.v)};`);
    else if (kind === 'numbers') numberLists.push({ id, values: sample.items.map((item) => item.v) });
    else if (kind === 'fonts') fonts.push({ id, names: flattenFonts(sample) });
    else themed.push({ id, kind, perTheme });
  }

  const lines = [];
  lines.push(`/*
 * ------------------------------------------------------------------------
 * 名称：tokens.generated.hpp
 * 说明：设计令牌的最终取值，由 tools/gen-tokens.mjs 从 rujian-ui-kit 的令牌与主题生成：长度、数字、时间等是常量，颜色与阴影按主题放在 ThemeTokens 里。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：不要手改；令牌或组件引用变化后重新运行生成器，npm run win32:check 会检查这份文件与生成结果一致。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <string_view>

#include "rujian/ui/token-types.hpp"

namespace rujian::ui::tokens {
`);
  lines.push('// 长度（逻辑像素）、数字、时间（毫秒）、百分比与 em 倍数。');
  lines.push(...scalars, '');
  lines.push('// 数字列表（如简写的内边距，按书写顺序）。');
  for (const item of numberLists) lines.push(`inline constexpr std::array<double, ${item.values.length}> ${item.id}{${item.values.map(number).join(', ')}};`);
  lines.push('', '// 字体列表（按书写顺序，含通用字体族关键字）。');
  for (const item of fonts) lines.push(`inline constexpr std::array<std::string_view, ${item.names.length}> ${item.id}{${item.names.map((fontName) => `"${fontName}"`).join(', ')}};`);
  lines.push('', '/** 随主题变化的令牌：颜色与阴影。 */', 'struct ThemeTokens {');
  for (const item of themed) lines.push(`  ${item.kind === 'color' ? 'Color' : 'ShadowList'} ${item.id};`);
  lines.push('};', '');
  for (const theme of themeNames) {
    lines.push(`inline constexpr ThemeTokens ${theme}{`);
    for (const item of themed) {
      const value = item.perTheme.get(theme);
      if (item.kind === 'color') lines.push(`    ${value.k === 'ident' ? 'kCurrentColor' : colorLiteral(value)},`);
      else {
        const shadows = shadowItems(value);
        if (shadows.length > MAX_SHADOWS) throw new Error(`${item.id} 的阴影超过 ${MAX_SHADOWS} 条`);
        lines.push(`    ShadowList{${shadows.length}, {${shadows.map(shadowLiteral).join(', ')}}},`);
      }
    }
    lines.push('};', '');
  }
  lines.push('}  // namespace rujian::ui::tokens', '');
  return lines.join('\n');
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  try {
    const text = generate();
    if (process.argv.includes('--check')) {
      if (!existsSync(OUTPUT_FILE) || readFileSync(OUTPUT_FILE, 'utf8') !== text) throw new Error('tokens.generated.hpp 与生成结果不一致，运行 node tools/gen-tokens.mjs 重新生成');
      console.log('tokens.generated.hpp 与生成结果一致');
    } else {
      writeFileSync(OUTPUT_FILE, text);
      console.log(`已生成 ${OUTPUT_FILE}`);
    }
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exit(1);
  }
}
