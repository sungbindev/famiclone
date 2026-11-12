/*
 * ------------------------------------------------------------------------
 * 名称：vitest.config.mts
 * 说明：测试配置：测试放在 test/ 下，目录层次与 src 一致，全部在 Node 环境运行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-10
 * 备注：不加载 vscode 模块；入口层的测试用 vi.mock 提供替身。
 * ------------------------------------------------------------------------
 */

import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    environment: 'node',
    globals: true,
    include: ['test/**/*.test.ts']
  }
});
