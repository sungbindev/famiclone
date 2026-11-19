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
import { AppFramePage } from './pages/app-frame-page';
import { ChoicePage } from './pages/choice-page';
import { DialogPage } from './pages/dialog-page';
import { FactsPage } from './pages/facts-page';
import { FeedbackPage } from './pages/feedback-page';
import { FieldPage } from './pages/field-page';
import { InputPage } from './pages/input-page';
import { ListPage } from './pages/list-page';
import { NotificationCenterPage, NotificationPage } from './pages/notification-page';
import { MenuPage } from './pages/menu-page';
import { PanelPage } from './pages/panel-page';
import { RowListPage } from './pages/row-list-page';
import { ScrollPage } from './pages/scroll-page';
import { SelectPage } from './pages/select-page';
import { SettingRowPage } from './pages/setting-row-page';
import { SliderPage } from './pages/slider-page';
import { SplitPage } from './pages/split-page';
import { TabsPage } from './pages/tabs-page';
import { TablePage } from './pages/table-page';
import { TooltipPage } from './pages/tooltip-page';
import { StatusBarPage } from './pages/status-bar-page';
import { TitleBarPage } from './pages/title-bar-page';
import { ToolbarPage } from './pages/toolbar-page';
import { TreePage } from './pages/tree-page';

const PAGES: Record<string, () => React.JSX.Element> = {
  'app-frame': AppFramePage,
  button: ButtonPage,
  choice: ChoicePage,
  dialog: DialogPage,
  facts: FactsPage,
  'facts-narrow': FactsPage,
  feedback: FeedbackPage,
  field: FieldPage,
  input: InputPage,
  list: ListPage,
  menu: MenuPage,
  notification: NotificationPage,
  'notification-center': NotificationCenterPage,
  panel: PanelPage,
  'row-list': RowListPage,
  scroll: ScrollPage,
  select: SelectPage,
  'setting-row': SettingRowPage,
  'setting-row-narrow': SettingRowPage,
  slider: SliderPage,
  split: SplitPage,
  tabs: TabsPage,
  table: TablePage,
  tooltip: TooltipPage,
  'status-bar': StatusBarPage,
  'status-bar-large': StatusBarPage,
  'title-bar': TitleBarPage,
  toolbar: ToolbarPage,
  'toolbar-narrow': ToolbarPage,
  tree: TreePage
};

const parameters = new URLSearchParams(location.search);
const Page = PAGES[parameters.get('page') ?? 'button'];
if (!Page) throw new Error(`未知的参考页：${parameters.get('page')}`);
document.documentElement.classList.add(`theme-${parameters.get('theme') === 'light' ? 'light' : 'dark'}`);
// 大尺寸状态栏是根元素上的类（AppFrame 的 statusBarSize=large）
if (parameters.get('page') === 'status-bar-large') document.documentElement.classList.add('ui-status-bar-large');

createRoot(document.getElementById('root') as HTMLElement).render(
  <UiProvider dialogDisplay="window">
    <Page />
  </UiProvider>
);
