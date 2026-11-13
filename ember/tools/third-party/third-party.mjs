/*
 * ------------------------------------------------------------------------
 * 名称：third-party.mjs
 * 说明：Ember 使用的第三方开源代码清单：组件库与 core/third_party 下各项目的声明、Ember 与组件库源码引入的 npm 包
 *       （及其依赖）、原生模块用到的 npm 包与 Electron；提供界面用的虚拟模块 virtual:ember-third-party，
 *       并写出随安装包分发的许可全文。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：读取、校验与格式化复用组件库 tools/third-party.mjs；声明或许可缺失时直接报错，构建、测试与打包随之失败。
 *       清单见 docs/01-软件架构设计.md 第 10 节。
 * ------------------------------------------------------------------------
 */

import { existsSync, mkdirSync, readdirSync, readFileSync, statSync, writeFileSync } from 'node:fs';
import { builtinModules } from 'node:module';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
/** 界面用的虚拟模块。 */
export const VIRTUAL_MODULE = 'virtual:ember-third-party';
/** 许可全文文件名，打包时复制到 resources 目录。 */
export const NOTICES_FILE = 'THIRD_PARTY_NOTICES.txt';
/** 许可全文的输出目录。 */
export const NOTICES_DIRECTORY = join(ROOT, 'build', 'third-party');
/** 自己的包，不算第三方。 */
const OWN_PACKAGES = new Set(['@rujianjs/ui-kit']);
/** 由安装包单独处理的包。 */
const ELECTRON = 'electron';
/** 原生模块包含的头文件与提供它的 npm 包。 */
const NATIVE_HEADERS = { 'napi.h': 'node-addon-api' };
const ELECTRON_USAGE = '桌面程序框架；其中 Chromium 等组件的许可见程序目录中的 LICENSES.chromium.html';

/** 递归列出文件。 */
function listFiles(directory, accept) {
  if (!existsSync(directory)) return [];
  return readdirSync(directory).flatMap((name) => {
    const path = join(directory, name);
    if (statSync(path).isDirectory()) return name === 'node_modules' ? [] : listFiles(path, accept);
    return accept(path) ? [path] : [];
  });
}

/** 包名（去掉子路径）。 */
function packageName(specifier) {
  const parts = specifier.split('/');
  return specifier.startsWith('@') ? parts.slice(0, 2).join('/') : parts[0];
}

/** 源码中引入的 npm 包：不含只引入类型的语句、相对路径、Node 内置模块与自己的包。 */
function importedPackages(files) {
  const names = new Set();
  for (const file of files) {
    const text = readFileSync(file, 'utf8');
    const specifiers = [
      ...[...text.matchAll(/(?:^|[\n;])\s*(?:import|export)\s+(?!type\s)[^'";]*?\bfrom\s+['"]([^'"]+)['"]/g)].map((match) => match[1]),
      ...[...text.matchAll(/(?:^|[\n;])\s*import\s+['"]([^'"]+)['"]/g)].map((match) => match[1]),
      ...[...text.matchAll(/\bimport\(\s*['"]([^'"]+)['"]\s*\)/g)].map((match) => match[1])
    ];
    for (const specifier of specifiers) {
      if (specifier.startsWith('.') || specifier.startsWith('node:') || specifier.startsWith('virtual:')) continue;
      const name = packageName(specifier);
      if (builtinModules.includes(name) || OWN_PACKAGES.has(name)) continue;
      names.add(name);
    }
  }
  return names;
}

/** 原生模块源码包含的头文件对应的 npm 包。 */
function nativePackages() {
  const names = new Set();
  for (const file of listFiles(join(ROOT, 'core', 'src'), (path) => /\.(cpp|hpp)$/.test(path))) {
    for (const match of readFileSync(file, 'utf8').matchAll(/^\s*#include\s+<([^>]+)>/gm)) {
      const name = NATIVE_HEADERS[match[1]];
      if (name !== undefined) names.add(name);
    }
  }
  return names;
}

/** 程序代码（不含测试、示例与类型声明）。 */
function isProgramSource(path) {
  return /\.(ts|tsx|mts|js|mjs)$/.test(path) && !/\.(test|stories)\.tsx?$/.test(path) && !path.endsWith('.d.ts') && !/[\\/](test|stories)[\\/]/.test(path);
}

/**
 * Ember 的第三方清单（含许可全文），按名称排序；不含不随安装包分发的项。
 * @param {string} uiKitDirectory 组件库源码目录。
 */
export async function collectClient(uiKitDirectory) {
  const tool = await import(pathToFileURL(join(uiKitDirectory, 'tools', 'third-party.mjs')).href);
  const sources = [...listFiles(join(ROOT, 'src'), isProgramSource), ...listFiles(join(uiKitDirectory, 'src'), isProgramSource)];
  const imported = importedPackages(sources);
  imported.delete(ELECTRON);
  const roots = [ROOT, uiKitDirectory];
  const packages = tool.collectPackages([...imported], roots);
  // 原生模块与 Electron 只取包本身：它们的依赖是编译与下载工具，不进安装包。
  const own = (names, usage) =>
    names.map((name) => ({ ...tool.collectPackages([name], roots).find((entry) => entry.name === name), ...(usage ? { usage } : {}) }));
  const entries = [
    ...tool.readDeclarations(join(uiKitDirectory, 'src', 'third_party')),
    ...tool.readDeclarations(join(ROOT, 'core', 'third_party')),
    ...packages,
    ...own([...nativePackages()]),
    ...own([ELECTRON], ELECTRON_USAGE)
  ].filter((entry) => entry.bundled);
  const unique = new Map(entries.map((entry) => [entry.name, entry]));
  return { tool, entries: [...unique.values()].toSorted((a, b) => a.name.localeCompare(b.name, 'en', { sensitivity: 'base' })) };
}

/**
 * 写出许可全文。
 * @param {string} uiKitDirectory 组件库源码目录。
 * @returns {Promise<string>} 文件路径。
 */
export async function writeNotices(uiKitDirectory) {
  const { tool, entries } = await collectClient(uiKitDirectory);
  mkdirSync(NOTICES_DIRECTORY, { recursive: true });
  const file = join(NOTICES_DIRECTORY, NOTICES_FILE);
  writeFileSync(file, tool.formatNotices('Ember 使用的第三方开源代码', entries));
  return file;
}

/**
 * Vite 插件：提供虚拟模块，默认导出不含许可全文的清单。
 * @param {string} uiKitDirectory 组件库源码目录。
 */
export function thirdPartyPlugin(uiKitDirectory) {
  const resolved = `\0${VIRTUAL_MODULE}`;
  return {
    name: 'ember-third-party',
    resolveId(id) {
      return id === VIRTUAL_MODULE ? resolved : undefined;
    },
    async load(id) {
      if (id !== resolved) return undefined;
      const { tool, entries } = await collectClient(uiKitDirectory);
      return `export default ${JSON.stringify(tool.summarize(entries))};`;
    }
  };
}
