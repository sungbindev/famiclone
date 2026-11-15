/*
 * ------------------------------------------------------------------------
 * 名称：vitest.config.mts
 * 说明：TypeScript 部分的测试配置：测试与被测文件同目录或放在 test/，界面测试使用 jsdom。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：组件库同样直接引用本机源码；C++ 部分的测试由 npm run core:test 运行；插件 SDK 与官方插件的测试在插件 SDK 仓库运行。
 * ------------------------------------------------------------------------
 */

import react from '@vitejs/plugin-react';
import { defineConfig } from 'vitest/config';
import { buildInfoPlugin } from './tools/build/build-info.mjs';
import { thirdPartyPlugin } from './tools/third-party/third-party.mjs';
import { DEDUPE, uiKitAlias, uiKitDirectory } from './ui-kit-source.mts';

export default defineConfig({
  plugins: [react(), thirdPartyPlugin(uiKitDirectory()), buildInfoPlugin()],
  resolve: {
    alias: uiKitAlias(),
    dedupe: DEDUPE
  },
  test: {
    environment: 'jsdom',
    globals: true,
    setupFiles: ['test/support/setup.ts'],
    include: ['src/**/*.test.{ts,tsx}', 'test/**/*.test.{ts,tsx}'],
    css: false
  }
});
