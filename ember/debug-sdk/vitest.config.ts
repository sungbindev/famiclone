/*
 * ------------------------------------------------------------------------
 * 名称：vitest.config.ts
 * 说明：调试 SDK 的测试配置：测试放在 test/，在 Node 环境运行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    environment: 'node',
    include: ['test/**/*.test.ts']
  }
});
