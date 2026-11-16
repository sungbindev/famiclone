/*
 * ------------------------------------------------------------------------
 * 名称：build-paths.mjs
 * 说明：Ember 构建产物的目录：全部放在仓库根目录的 build/ember 下，不进入 ember/ 目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：打包输出（Ember-win32-x64、make）由 forge.config.mts 的 outDir 指到同一个目录；Vite 的 .vite 目录由 Forge 固定在 ember/ 下，不在这里；帮助文档的 VitePress 配置（help/.vitepress/config.mts）直接引用这里的 HELP_BUILD_DIRECTORY。
 *       主进程（src/bootstrap/main/main.ts）、forge.config.mts、core/CMakeLists.txt、build.ps1 与 package-mac.sh 各自写了同样的路径，改这里时一并修改。
 * ------------------------------------------------------------------------
 */

import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

/** 构建产物的根目录：<仓库根>/build/ember。 */
export const BUILD_ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..', '..', 'build', 'ember');
/** 原生模块的构建目录，模块在其下的 Release 里。 */
export const CORE_BUILD_DIRECTORY = join(BUILD_ROOT, 'core');
/** C++ 测试的构建目录。 */
export const CORE_TEST_BUILD_DIRECTORY = join(BUILD_ROOT, 'core-test');
/** 打好的预装插件包目录。 */
export const PLUGINS_BUILD_DIRECTORY = join(BUILD_ROOT, 'plugins');
/** 第三方开源代码许可全文的输出目录。 */
export const THIRD_PARTY_BUILD_DIRECTORY = join(BUILD_ROOT, 'third-party');
/** 帮助文档站点（help/ 由 VitePress 构建）的输出目录。 */
export const HELP_BUILD_DIRECTORY = join(BUILD_ROOT, 'help');
