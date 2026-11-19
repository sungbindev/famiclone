/*
 * ------------------------------------------------------------------------
 * 名称：vite.config.mts
 * 说明：网页版参考页的构建配置：用 Ember 的组件库源码别名，把参考页构建成可用 file:// 打开的静态页面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：不压缩，输出到 build/ember/win32-reference/page；@kit-source/ 指向组件库源码的 src 目录，参考页用它引入没有经组件导入的样式文件。
 * ------------------------------------------------------------------------
 */

import react from '@vitejs/plugin-react';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { defineConfig } from 'vite';
import { DEDUPE, uiKitAlias, uiKitDirectory } from '../../../../ui-kit-source.mts';
import { WIN32_REFERENCE_DIRECTORY } from '../../../../tools/build/build-paths.mjs';

const here = dirname(fileURLToPath(import.meta.url));

export default defineConfig({
  root: join(here, 'page'),
  base: './',
  plugins: [react()],
  resolve: { alias: [...uiKitAlias(), { find: /^@kit-source\//, replacement: `${join(uiKitDirectory(), 'src')}/` }], dedupe: DEDUPE },
  build: { outDir: join(WIN32_REFERENCE_DIRECTORY, 'page'), emptyOutDir: true, minify: false },
  logLevel: 'warn'
});
