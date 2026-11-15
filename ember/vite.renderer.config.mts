/*
 * ------------------------------------------------------------------------
 * 名称：vite.renderer.config.mts
 * 说明：界面（渲染进程）的 Vite 构建配置：React、组件库源码别名、第三方开源代码清单的虚拟模块，开发模式下给内容安全策略追加热更新所需的放行项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：正式包使用 index.html 里严格的策略；开发服务器需要 ws 连接和内联样式（Vite 把样式注入为 style 标签），还要允许访问项目目录之外的组件库。
 * ------------------------------------------------------------------------
 */

import react from '@vitejs/plugin-react';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { defineConfig, searchForWorkspaceRoot } from 'vite';
import { buildInfoPlugin } from './tools/build/build-info.mjs';
import { thirdPartyPlugin } from './tools/third-party/third-party.mjs';
import { DEDUPE, uiKitAlias, uiKitDirectory } from './ui-kit-source.mts';

/** 开发模式追加的放行项：热更新的 WebSocket 连接。 */
const DEV_CONNECT_SOURCE = 'ws://localhost:*';
/** 开发模式追加的放行项：Vite 注入的内联样式。 */
const DEV_STYLE_SOURCE = "'unsafe-inline'";

export default defineConfig({
  plugins: [
    react(),
    thirdPartyPlugin(uiKitDirectory()),
    buildInfoPlugin(),
    {
      name: 'ember-dev-csp',
      apply: 'serve',
      transformIndexHtml(html: string): string {
        return html.replace("style-src 'self'", `style-src 'self' ${DEV_STYLE_SOURCE}`).replace("connect-src 'self'", `connect-src 'self' ${DEV_CONNECT_SOURCE}`);
      }
    }
  ],
  resolve: {
    alias: uiKitAlias(),
    dedupe: DEDUPE
  },
  // 组件库在项目目录之外，开发服务器要允许访问它（标题栏的 Quicksand 字体文件在组件库里）。
  server: { fs: { allow: [searchForWorkspaceRoot(dirname(fileURLToPath(import.meta.url))), uiKitDirectory()] } },
  // 渲染 Worker 用 ES 模块格式，与 new Worker(..., { type: 'module' }) 一致。
  worker: { format: 'es' }
});
