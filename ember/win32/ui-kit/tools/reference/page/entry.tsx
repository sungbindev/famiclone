/*
 * ------------------------------------------------------------------------
 * 名称：entry.tsx
 * 说明：网页版参考页：用 rujian-ui-kit 的真实组件画出和 gallery 示例页一样的内容，作为原生控件库对照的基准。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：页面参数 theme 取 dark 或 light；示例页的结构与 gallery/main.cpp 的 SamplePage 一一对应，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Button, iconDeviceFloppy, UiProvider } from '@rujianjs/ui-kit';
import '@rujianjs/ui-kit/styles.css';
import { createRoot } from 'react-dom/client';
import './page.css';

const theme = new URLSearchParams(location.search).get('theme') === 'light' ? 'light' : 'dark';
document.documentElement.classList.add(`theme-${theme}`);

createRoot(document.getElementById('root') as HTMLElement).render(
  <UiProvider dialogDisplay="window">
    <div className="reference-row">
      <Button icon={false}>取消</Button>
      <Button icon={false} variant="primary">
        确定
      </Button>
      <Button icon={iconDeviceFloppy}>保存</Button>
      <Button icon={false} disabled>
        禁用
      </Button>
    </div>
  </UiProvider>
);
