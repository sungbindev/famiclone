/*
 * ------------------------------------------------------------------------
 * 名称：ui-kit-source.mts
 * 说明：组件库源码的位置：开发、打包与测试都直接引用本机组件库的源码，并只保留一份 React。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：默认是 ../../rujian-ui-kit（与 famiclone 仓库并列的组件库仓库），可用环境变量 EMBER_UI_KIT_SOURCE 修改；找不到时直接报错。
 *       TypeScript 的路径映射写在 tsconfig.json，只认默认位置。
 * ------------------------------------------------------------------------
 */

import { existsSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import type { Alias } from 'vite';

const ROOT = dirname(fileURLToPath(import.meta.url));
const DEFAULT_SOURCE = '../../rujian-ui-kit';

/**
 * 组件库源码的入口文件。
 * @returns 入口文件的绝对路径。
 */
export function uiKitEntry(): string {
  return resolve(uiKitDirectory(), 'src', 'index.ts');
}

/** 组件库源码目录；找不到入口文件时报错。 */
export function uiKitDirectory(): string {
  const directory = resolve(ROOT, process.env.EMBER_UI_KIT_SOURCE ?? DEFAULT_SOURCE);
  const entry = resolve(directory, 'src', 'index.ts');
  if (!existsSync(entry)) {
    throw new Error(`找不到组件库源码：${entry}。请把 rujian-ui-kit 拉取到 ${directory}，或用环境变量 EMBER_UI_KIT_SOURCE 指定它的目录。`);
  }
  return directory;
}

/**
 * Vite 的别名：@rujianjs/ui-kit 指向组件库源码入口，@rujianjs/ui-kit/styles.css 指向源码的样式入口。
 * @returns 别名配置。
 */
export function uiKitAlias(): Alias[] {
  return [
    { find: /^@rujianjs\/ui-kit$/, replacement: uiKitEntry() },
    { find: /^@rujianjs\/ui-kit\/styles\.css$/, replacement: resolve(uiKitDirectory(), 'src', 'styles', 'index.css') }
  ];
}

/** 只保留一份的依赖：组件库源码与 Ember 共用同一份 React。 */
export const DEDUPE = ['react', 'react-dom'];
