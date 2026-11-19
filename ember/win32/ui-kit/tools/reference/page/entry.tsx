/*
 * ------------------------------------------------------------------------
 * 名称：entry.tsx
 * 说明：网页版参考页入口：按页面参数 page 选一页，用 rujian-ui-kit 的真实组件画出和 gallery 对应示例页一样的内容，作为原生控件库对照的基准。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：页面参数：page（页名，对应 gallery 的 --page）、theme（dark 或 light）；每页的 data-sample 元素就是参与对照的样本。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import '@rujianjs/ui-kit/styles.css';
import { createRoot } from 'react-dom/client';
import './page.css';
import { ButtonPage } from './pages/button-page';
import { ChoicePage } from './pages/choice-page';
import { InputPage } from './pages/input-page';
import { SelectPage } from './pages/select-page';
import { SliderPage } from './pages/slider-page';

const PAGES: Record<string, () => React.JSX.Element> = {
  button: ButtonPage,
  choice: ChoicePage,
  input: InputPage,
  select: SelectPage,
  slider: SliderPage
};

const parameters = new URLSearchParams(location.search);
const Page = PAGES[parameters.get('page') ?? 'button'];
if (!Page) throw new Error(`未知的参考页：${parameters.get('page')}`);
document.documentElement.classList.add(`theme-${parameters.get('theme') === 'light' ? 'light' : 'dark'}`);

createRoot(document.getElementById('root') as HTMLElement).render(
  <UiProvider dialogDisplay="window">
    <Page />
  </UiProvider>
);
