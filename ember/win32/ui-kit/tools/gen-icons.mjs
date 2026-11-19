/*
 * ------------------------------------------------------------------------
 * 名称：gen-icons.mjs
 * 说明：图标生成器：从 rujian-ui-kit 的图标数据里取出界面与已登记组件用到的图标，生成公共的图标编号 icon-id.hpp 与路径数据 icon-data.generated.cpp。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用到的图标 = 已登记组件目录的源码里出现的 icon<名称> 标识符中在图标数据里存在的那些，加上 tools/kit-components.json 的 extraIcons（应用自己用到的图标，需要新图标时在这里加）；
 *       图标数据的路径是 24×24 画布内的 SVG 路径，字符串是描边路径，对象还可带 fill、stroke、opacity。
 *       用法：node tools/gen-icons.mjs 写文件，node tools/gen-icons.mjs --check 只检查已提交的文件与生成结果一致。
 * ------------------------------------------------------------------------
 */

import { existsSync, readdirSync, readFileSync, statSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import vm from 'node:vm';
import { findWebKit, KIT_ROOT, readManifest } from './lib/kit-source.mjs';

/** 公共图标编号头文件。 */
export const ID_FILE = join(KIT_ROOT, 'include', 'rujian', 'ui', 'icon-id.hpp');
/** 路径数据文件。 */
export const DATA_FILE = join(KIT_ROOT, 'src', 'generated', 'icon-data.generated.cpp');
/** 按文字匹配图标的规则表文件。 */
export const RULES_FILE = join(KIT_ROOT, 'src', 'generated', 'icon-rules.generated.cpp');

/**
 * 递归列出目录里的源码文件（.ts 与 .tsx，排除示例与测试）。
 * @param {string} directory 目录。
 * @returns {string[]} 文件路径。
 */
function listSources(directory) {
  const files = [];
  for (const name of readdirSync(directory)) {
    const path = join(directory, name);
    if (statSync(path).isDirectory()) files.push(...listSources(path));
    else if (/\.(ts|tsx)$/.test(name) && !/\.(test|stories)\./.test(name)) files.push(path);
  }
  return files;
}

/**
 * 读取全部图标数据。
 * @param {string} file icon-data.ts 的路径。
 * @returns {Map<string, (string | object)[]>} 图标标识符到路径数组。
 */
function readIconData(file) {
  const icons = new Map();
  for (const line of readFileSync(file, 'utf8').split('\n')) {
    const match = /^export const (icon\w+) = (\[.*\]);\s*$/.exec(line);
    if (match) icons.set(match[1], vm.runInNewContext(match[2], Object.create(null), { timeout: 1000 }));
  }
  return icons;
}

/**
 * 找出用到的图标标识符。
 * @param {string} webKit 组件库根目录。
 * @param {Set<string>} known 图标数据里存在的标识符。
 * @returns {string[]} 排序后的标识符。
 */
function findUsedIcons(webKit, known) {
  const manifest = readManifest();
  const directories = manifest.components.map((component) => join(webKit, 'src', 'components', component));
  const used = new Set();
  for (const name of manifest.extraIcons) {
    if (!known.has(name)) throw new Error(`kit-components.json 的 extraIcons 里的 ${name} 不在图标数据里`);
    used.add(name);
  }
  for (const directory of directories) {
    for (const file of listSources(directory)) {
      for (const match of readFileSync(file, 'utf8').matchAll(/\bicon[A-Z][A-Za-z0-9]*\b/g)) if (known.has(match[0])) used.add(match[0]);
    }
  }
  return [...used].toSorted();
}

/**
 * 图标标识符转成枚举名：去掉 icon 前缀。
 * @param {string} name 标识符。
 * @returns {string} 枚举名。
 */
function enumName(name) {
  return name.slice('icon'.length);
}

/**
 * 生成文件头注释。
 * @param {string} name 文件名。
 * @param {string} description 说明。
 * @returns {string} 文件头。
 */
function header(name, description) {
  return `/*
 * ------------------------------------------------------------------------
 * 名称：${name}
 * 说明：${description}
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：由 tools/gen-icons.mjs 生成，不要手改；图标图形来自 Tabler Icons（MIT，见 third_party/tabler-icons）。
 * ------------------------------------------------------------------------
 */
`;
}

/**
 * 把文字写成 C++ 字符串字面量。
 * @param {string} text 文字。
 * @returns {string} 字面量。
 */
function quote(text) {
  return `"${text.replaceAll('\\', '\\\\').replaceAll('"', '\\"')}"`;
}

/**
 * 读取 icon-rules.ts 里的“按文字匹配图标”规则。
 * @param {string} webKit 组件库根目录。
 * @param {Set<string>} known 图标数据里存在的标识符。
 * @returns {{ pattern: string, icon: string, atEnd: boolean }[]} 规则，按书写顺序。
 */
function readLabelRules(webKit, known) {
  const text = readFileSync(join(webKit, 'src', 'components', 'icons', 'icon-rules.ts'), 'utf8');
  const declaration = text.indexOf('LABEL_ICON_RULES');
  const start = text.indexOf('[', text.indexOf('=', declaration));
  const end = text.indexOf('\n];', start);
  if (declaration === -1 || start === -1 || end === -1) throw new Error('没有找到 icon-rules.ts 里的 LABEL_ICON_RULES');
  const rules = [];
  for (const raw of text.slice(start + 1, end).split('\n')) {
    const line = raw.trim();
    if (line === '' || line.startsWith('//')) continue;
    const match = /^\[\/(.+)\/,\s*(icon\w+)(?:,\s*(END))?\],?$/.exec(line);
    if (!match) throw new Error(`不认识的图标规则写法：${line}`);
    const [, pattern, icon, end_] = match;
    if (!known.has(icon)) throw new Error(`图标规则里的 ${icon} 不在图标数据里`);
    // C++ 按字节匹配 UTF-8，字符类与量词会把多字节汉字拆开，只允许分组、选择与首尾锚点
    if (/[[\]?*+{}.\\]/.test(pattern)) throw new Error(`图标规则 /${pattern}/ 用了不支持的写法（只支持分组、选择和 ^ $ 锚点）`);
    rules.push({ pattern, icon, atEnd: end_ === 'END' });
  }
  return rules;
}

/**
 * 生成三个文件的文本。
 * @returns {{ id: string, data: string, rules: string, count: number }} 文本与图标个数。
 */
export function generate() {
  const webKit = findWebKit();
  const icons = readIconData(join(webKit, 'src', 'third_party', 'tabler-icons', 'icon-data.ts'));
  const used = findUsedIcons(webKit, new Set(icons.keys()));

  const id = [
    header('icon-id.hpp', '图标编号：界面与已登记组件用到的全部图标，名称与组件库里的 icon<名称> 对应。'),
    '#pragma once',
    '',
    '#include <cstdint>',
    '',
    'namespace rujian::ui {',
    '',
    '/** 图标编号。 */',
    'enum class IconId : std::uint16_t {',
    ...used.map((name) => `  ${enumName(name)},`),
    '  Count',
    '};',
    '',
    '}  // namespace rujian::ui',
    ''
  ].join('\n');

  const data = [
    header('icon-data.generated.cpp', '图标的路径数据：每个图标是若干条 SVG 路径，按图标编号的顺序排列。'),
    '#include "base/icon-data.hpp"',
    '',
    'namespace rujian::ui {',
    '',
    'namespace {',
    ''
  ];
  for (const name of used) {
    const paths = icons.get(name).map((path) => (typeof path === 'string' ? { d: path } : path));
    data.push(`constexpr IconPath ${enumName(name)}Paths[] = {`);
    for (const path of paths) {
      const unknown = Object.keys(path).filter((key) => !['d', 'fill', 'stroke', 'opacity'].includes(key));
      if (unknown.length > 0) throw new Error(`${name} 的路径带有不支持的属性：${unknown.join('、')}`);
      data.push(`    {${quote(path.d)}, ${quote(path.fill ?? '')}, ${quote(path.stroke ?? '')}, ${path.opacity ?? 1}},`);
    }
    data.push('};', '');
  }
  data.push('constexpr IconData kIcons[] = {');
  for (const name of used) data.push(`    {${enumName(name)}Paths, sizeof(${enumName(name)}Paths) / sizeof(IconPath)},`);
  data.push('};', '', '}  // namespace', '', 'const IconData& iconData(IconId id) { return kIcons[static_cast<std::size_t>(id)]; }', '', '}  // namespace rujian::ui', '');

  const rules = [
    header('icon-rules.generated.cpp', '按文字含义匹配图标的规则表，顺序与组件库 icon-rules.ts 里的内置规则相同，先匹配先得。'),
    '#include "base/icon-rules.hpp"',
    '',
    'namespace rujian::ui {',
    '',
    'const std::vector<LabelIconRule>& labelIconRules() {',
    '  static const std::vector<LabelIconRule> rules = {',
    ...readLabelRules(webKit, new Set(icons.keys())).map((rule) => `      {${quote(rule.pattern)}, IconId::${enumName(rule.icon)}, ${rule.atEnd}},`),
    '  };',
    '  return rules;',
    '}',
    '',
    '}  // namespace rujian::ui',
    ''
  ];
  return { id, data: data.join('\n'), rules: rules.join('\n'), count: used.length };
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  try {
    const { id, data, rules, count } = generate();
    if (process.argv.includes('--check')) {
      const files = [[ID_FILE, id], [DATA_FILE, data], [RULES_FILE, rules]];
      if (files.some(([file, text]) => !existsSync(file) || readFileSync(file, 'utf8') !== text)) throw new Error('图标文件与生成结果不一致，运行 node tools/gen-icons.mjs 重新生成');
      console.log(`图标文件与生成结果一致（${count} 个图标）`);
    } else {
      writeFileSync(ID_FILE, id);
      writeFileSync(DATA_FILE, data);
      writeFileSync(RULES_FILE, rules);
      console.log(`已生成 ${count} 个图标`);
    }
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exit(1);
  }
}
