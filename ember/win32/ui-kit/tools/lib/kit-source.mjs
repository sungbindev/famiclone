/*
 * ------------------------------------------------------------------------
 * 名称：kit-source.mjs
 * 说明：读取 rujian-ui-kit 的源文件：令牌、主题、基础样式，以及已登记的组件目录里的样式文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：组件库位置默认是 famiclone 仓库旁边的 rujian-ui-kit，环境变量 EMBER_UI_KIT_SOURCE 可以指定别处；登记的组件目录见 tools/kit-components.json。
 * ------------------------------------------------------------------------
 */

import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseRules } from './css.mjs';

const here = dirname(fileURLToPath(import.meta.url));
/** 控件库根目录（ui-kit）。 */
export const KIT_ROOT = resolve(here, '..', '..');
/** 工具目录。 */
export const TOOLS_ROOT = resolve(here, '..');

/**
 * 找到 rujian-ui-kit 的源码目录（其下有 src/styles）。
 * @returns {string} 组件库根目录。
 */
export function findWebKit() {
  const root = process.env.EMBER_UI_KIT_SOURCE ?? resolve(KIT_ROOT, '..', '..', '..', '..', 'rujian-ui-kit');
  if (!existsSync(join(root, 'src', 'styles'))) throw new Error(`没有找到组件库 ${root}，可以用环境变量 EMBER_UI_KIT_SOURCE 指定`);
  return root;
}

/**
 * 读取已登记的组件清单。
 * @returns {{ components: string[], extraStyles: string[] }} 清单。
 */
export function readManifest() {
  return JSON.parse(readFileSync(join(TOOLS_ROOT, 'kit-components.json'), 'utf8'));
}

/**
 * 把一个样式文件里选择器符合条件的规则的自定义属性收集到 into。
 * @param {string} file 样式文件。
 * @param {Map<string, string>} into 收集结果。
 * @param {(selector: string) => boolean} selectorTest 选择器条件。
 */
function collectVariables(file, into, selectorTest) {
  for (const rule of parseRules(readFileSync(file, 'utf8'))) {
    if (rule.media !== '' || !selectorTest(rule.selector)) continue;
    for (const [name, value] of rule.declarations) if (name.startsWith('--')) into.set(name, value);
  }
}

/**
 * 读取令牌、基础样式与全部主题里的自定义属性。
 * @param {string} webKit 组件库根目录。
 * @returns {{ base: Map<string, string>, themes: Map<string, Map<string, string>> }} 公共属性与各主题自己的属性。
 */
export function readStyleVariables(webKit) {
  const base = new Map();
  const themes = new Map();
  const styles = join(webKit, 'src', 'styles');
  collectVariables(join(styles, 'tokens', 'ui-tokens.css'), base, (selector) => selector === ':root');
  collectVariables(join(styles, 'base', 'ui-base.css'), base, (selector) => selector === ':root');
  for (const file of readdirSync(join(styles, 'themes'))) {
    const name = file.replace(/\.css$/, '');
    const theme = new Map();
    collectVariables(join(styles, 'themes', file), theme, (selector) => selector === `:root.theme-${name}`);
    themes.set(name, theme);
  }
  return { base, themes };
}

/**
 * 读取已登记的组件目录与额外样式文件的文本。
 * @param {string} webKit 组件库根目录。
 * @returns {{ file: string, text: string }[]} 样式文件。
 */
export function readComponentStyles(webKit) {
  const manifest = readManifest();
  const files = [];
  for (const component of manifest.components) {
    const directory = join(webKit, 'src', 'components', component);
    for (const name of readdirSync(directory)) if (name.endsWith('.css')) files.push(join(directory, name));
  }
  for (const extra of manifest.extraStyles) files.push(join(webKit, 'src', extra));
  return files.map((file) => ({ file, text: readFileSync(file, 'utf8') }));
}

/**
 * 找出样式文本里引用的全部自定义属性名。
 * @param {string} text 样式文本。
 * @returns {Set<string>} 属性名。
 */
export function referencedVariables(text) {
  return new Set([...text.matchAll(/var\(\s*(--[a-zA-Z0-9-]+)/g)].map((match) => match[1]));
}
