/*
 * ------------------------------------------------------------------------
 * 名称：Shell.test.tsx
 * 说明：外壳的测试：只渲染登记表（页面、活动栏、菜单、快捷键、边栏、底部面板、状态项）、启动页面、边栏随页面更换与置灰、再点活动栏按钮折叠、
 *       全局快捷键与画面优先、右边栏置灰、退出与全屏经窗口接口、消息追加与清空、没有页面时的空工作区、全屏只显示页面、启动时各面板全部关闭且展开状态不保存、底部面板高度的读取与保存、登记错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的 window.ember；页面是测试里写的简单组件，不引用运行页。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, fireEvent, render, screen, within } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { useEffect } from 'react';
import { describe, expect, it, vi } from 'vitest';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { ShellLayout } from '../../contracts/shell/shell-state-service';
import type { WindowState } from '../../contracts/shell/window-control';
import { useShell } from './shell-context';
import type { ShellApi, ShellModule } from './shell-registry';
import { Shell } from './Shell';

const ICON = ['M0 0'] as const;

/** 没有保存过时的布局。 */
const DEFAULT_LAYOUT: ShellLayout = { bottomPanelHeight: null };

/** 假的 window.ember：布局与窗口状态由测试决定，调用都记录下来。 */
function createBridge(layout: Partial<ShellLayout> = {}) {
  let windowListener: ((state: WindowState) => void) | undefined;
  const shell = {
    getState: vi.fn(() => Promise.resolve({ window: null, layout: { ...DEFAULT_LAYOUT, ...layout } })),
    setLayout: vi.fn(),
    windowAction: vi.fn(),
    getWindowState: vi.fn(() => Promise.resolve({ maximized: false, fullScreen: false })),
    onWindowStateChange: (listener: (state: WindowState) => void) => {
      windowListener = listener;
      return () => {
        windowListener = undefined;
      };
    }
  };
  return {
    bridge: { shell } as unknown as EmberBridge,
    shell,
    emitWindowState: (state: WindowState) => act(() => windowListener?.(state))
  };
}

/** 追加一条消息的页面，用来测试消息接口。 */
function MessagePage() {
  const shell = useShell();
  return (
    <button type="button" onClick={() => shell.addMessage({ level: 'error', text: '核心已停止', time: new Date(2026, 10, 14, 9, 5, 7) })}>
      追加消息
    </button>
  );
}

/** 有边栏、启动时打开的甲模块；乙模块没有边栏；丙模块有一个按钮追加消息。 */
const MODULES: readonly ShellModule[] = [
  {
    name: 'a',
    pages: [{ id: 'a.page', title: '甲页', icon: ICON, Component: () => <p>甲页内容</p>, openOnStart: true, sideBar: { Component: () => <p>甲边栏内容</p> } }],
    activityItems: [{ id: 'a.btn', label: '甲', icon: ICON, page: 'a.page' }],
    commands: [{ id: 'a.hello', title: '问好', defaultShortcut: 'Ctrl+H', run: (shell) => shell.addMessage({ level: 'info', text: '你好' }) }],
    menuItems: [{ menu: 'machine', command: 'a.hello' }]
  },
  {
    name: 'b',
    pages: [{ id: 'b.page', title: '乙页', icon: ICON, Component: () => <p>乙页内容</p> }],
    activityItems: [{ id: 'b.btn', label: '乙', icon: ICON, page: 'b.page' }]
  },
  {
    name: 'c',
    pages: [{ id: 'c.messages', title: '消息页', icon: ICON, Component: MessagePage }],
    activityItems: [{ id: 'c.btn', label: '丙', icon: ICON, page: 'c.messages' }]
  }
];

function renderShell(options: { layout?: Partial<ShellLayout>; modules?: readonly ShellModule[] } = {}) {
  const fake = createBridge(options.layout);
  const result = render(
    <UiProvider>
      <Shell bridge={fake.bridge} modules={options.modules ?? MODULES} />
    </UiProvider>
  );
  return { ...fake, ...result };
}

/** 标签栏（页面区上方已打开的页面）。 */
const TABS_NAME = { name: '已打开的页面' };

/** 等外壳读完布局、显示出来。 */
async function shown(): Promise<void> {
  await screen.findByRole('tablist', TABS_NAME);
}

/** 边栏区域元素。 */
function sideArea(container: HTMLElement): HTMLElement {
  return container.querySelector('.ui-app-frame__side') as HTMLElement;
}

function bottomArea(container: HTMLElement): HTMLElement {
  return container.querySelector('.ui-app-frame__bottom') as HTMLElement;
}

function rightArea(container: HTMLElement): HTMLElement {
  return container.querySelector('.ui-app-frame__right') as HTMLElement;
}

/** 打开的菜单里的全部菜单项（普通项是 menuitem，可勾选项是 menuitemcheckbox）。 */
function popupItems(): HTMLElement[] {
  return Array.from(document.querySelectorAll<HTMLElement>('.ui-menu__popup [role^="menuitem"]'));
}

/** 按下一个组合键（事件发在 body 上，冒泡到 window）。 */
function press(init: KeyboardEventInit): void {
  fireEvent.keyDown(document.body, init);
}

/** Ctrl+B：切换边栏。 */
const TOGGLE_SIDE_BAR = { code: 'KeyB', ctrlKey: true } as const;
/** Ctrl+J：切换底部面板。 */
const TOGGLE_BOTTOM_PANEL = { code: 'KeyJ', ctrlKey: true } as const;

describe('登记表驱动的界面', () => {
  it('启动时打开登记了 openOnStart 的页面，活动栏、标签与页面内容都来自登记', async () => {
    renderShell();
    await shown();
    expect(screen.getByRole('tab', { name: '甲页' })).toBeTruthy();
    expect(screen.queryByRole('tab', { name: '乙页' })).toBeNull();
    expect(screen.getByText('甲页内容')).toBeTruthy();
    for (const label of ['甲', '乙', '丙']) expect(screen.getByRole('button', { name: label })).toBeTruthy();
  });

  it('菜单栏只显示有登记项的菜单：文件、机器、视图，没有设置与帮助', async () => {
    renderShell();
    await shown();
    expect(screen.getAllByRole('menuitem').map((item) => item.textContent)).toEqual(['文件(F)', '机器(M)', '视图(V)']);
  });

  it('菜单项：视图里先是活动栏按钮的入口，再是边栏、底部面板、右边栏、全屏，并显示快捷键', async () => {
    const user = userEvent.setup();
    renderShell();
    await shown();
    await user.click(screen.getByRole('menuitem', { name: /视图/ }));
    const labels = popupItems().map((item) => item.textContent ?? '');
    expect(labels.filter((text) => /^(甲|乙|丙|边栏|底部面板|右边栏|全屏)/.test(text)).map((text) => text.replace(/(Ctrl\+(Alt\+)?[BJ]|F11)$/, ''))).toEqual(['甲', '乙', '丙', '边栏', '底部面板', '右边栏', '全屏']);
    expect(labels.some((text) => text.endsWith('Ctrl+Alt+B'))).toBe(true);
    expect(labels.some((text) => text.endsWith('F11'))).toBe(true);
  });

  it('右边栏现在不能打开：标题栏按钮置灰，菜单项置灰，快捷键不起作用，右边栏里是“没有盘片”', async () => {
    const user = userEvent.setup();
    const { container, shell } = renderShell();
    await shown();
    expect((screen.getByRole('button', { name: '切换右边栏' }) as HTMLButtonElement).disabled).toBe(true);
    expect(rightArea(container).hidden).toBe(true);
    expect(within(rightArea(container)).getByText('没有盘片')).toBeTruthy();
    press({ code: 'KeyB', ctrlKey: true, altKey: true });
    expect(rightArea(container).hidden).toBe(true);
    expect(shell.setLayout).not.toHaveBeenCalled();
    await user.click(screen.getByRole('menuitem', { name: /视图/ }));
    const item = popupItems().find((entry) => entry.textContent?.startsWith('右边栏')) as HTMLElement;
    expect(item.getAttribute('aria-disabled')).toBe('true');
  });

  it('启动时边栏、底部面板、右边栏全部关闭；打开底部面板后有“消息”页签和“清空”“关闭”', async () => {
    const { container } = renderShell();
    await shown();
    expect(sideArea(container).hidden).toBe(true);
    expect(bottomArea(container).hidden).toBe(true);
    expect(rightArea(container).hidden).toBe(true);
    press(TOGGLE_BOTTOM_PANEL);
    expect(within(bottomArea(container)).getByRole('tab', { name: '消息' })).toBeTruthy();
    expect(within(bottomArea(container)).getByRole('button', { name: '清空' })).toBeTruthy();
    expect(within(bottomArea(container)).getByRole('button', { name: '关闭' })).toBeTruthy();
    expect(within(bottomArea(container)).getByText('暂无消息')).toBeTruthy();
  });

  it('读出保存的底部面板高度，展开时用保存的高度；没有保存过时用默认高度', async () => {
    const saved = renderShell({ layout: { bottomPanelHeight: 220 } });
    await shown();
    expect(bottomArea(saved.container).style.height).toBe('220px');
    saved.unmount();
    const fresh = renderShell();
    await shown();
    expect(bottomArea(fresh.container).style.height).toBe('160px');
  });
});

describe('边栏', () => {
  it('切换页面时边栏换成该页面的边栏，没有边栏的页面使边栏自动关闭、折叠按钮置灰', async () => {
    const user = userEvent.setup();
    const { container, shell } = renderShell();
    await shown();
    press(TOGGLE_SIDE_BAR);
    expect(screen.getByText('甲边栏内容')).toBeTruthy();
    const toggle = screen.getByRole('button', { name: '切换边栏' }) as HTMLButtonElement;
    expect(toggle.disabled).toBe(false);

    await user.click(screen.getByRole('button', { name: '乙' }));
    expect(screen.getByRole('tab', { name: '乙页' }).getAttribute('aria-selected')).toBe('true');
    expect(sideArea(container).hidden).toBe(true);
    expect(toggle.disabled).toBe(true);
    // 展开状态不保存。
    expect(shell.setLayout).not.toHaveBeenCalled();

    await user.click(screen.getByRole('tab', { name: '甲页' }));
    expect(sideArea(container).hidden).toBe(false);
    expect(toggle.disabled).toBe(false);
  });

  it('没有边栏的页面上边栏的快捷键不起作用，切回有边栏的页面时边栏仍是原来的状态', async () => {
    const user = userEvent.setup();
    const { container } = renderShell();
    await shown();
    await user.click(screen.getByRole('button', { name: '乙' }));
    press(TOGGLE_SIDE_BAR);
    await user.click(screen.getByRole('tab', { name: '甲页' }));
    expect(sideArea(container).hidden).toBe(true);
  });

  it('再点选中的活动栏按钮展开，再点折叠，不保存', async () => {
    const user = userEvent.setup();
    const { container, shell } = renderShell();
    await shown();
    const button = screen.getByRole('button', { name: '甲' });
    expect(button.getAttribute('aria-current')).toBe('page');
    await user.click(button);
    expect(sideArea(container).hidden).toBe(false);
    await user.click(button);
    expect(sideArea(container).hidden).toBe(true);
    expect(shell.setLayout).not.toHaveBeenCalled();
  });

  it('边栏折叠时点另一个有边栏页面的按钮会展开边栏', async () => {
    const user = userEvent.setup();
    const { container } = renderShell();
    await shown();
    await user.click(screen.getByRole('button', { name: '乙' }));
    await user.click(screen.getByRole('button', { name: '甲' }));
    expect(sideArea(container).hidden).toBe(false);
  });

  it('标题栏的边栏按钮与 Ctrl+B 切换边栏，不保存', async () => {
    const user = userEvent.setup();
    const { container, shell } = renderShell();
    await shown();
    await user.click(screen.getByRole('button', { name: '切换边栏' }));
    expect(sideArea(container).hidden).toBe(false);
    press(TOGGLE_SIDE_BAR);
    expect(sideArea(container).hidden).toBe(true);
    expect(shell.setLayout).not.toHaveBeenCalled();
  });
});

describe('底部面板与快捷键', () => {
  it('Ctrl+J 与标题栏按钮切换底部面板，“关闭”收起，不保存', async () => {
    const user = userEvent.setup();
    const { container, shell } = renderShell();
    await shown();
    press(TOGGLE_BOTTOM_PANEL);
    expect(bottomArea(container).hidden).toBe(false);
    await user.click(screen.getByRole('button', { name: '切换底部面板' }));
    expect(bottomArea(container).hidden).toBe(true);
    await user.click(screen.getByRole('button', { name: '切换底部面板' }));
    expect(bottomArea(container).hidden).toBe(false);
    await user.click(within(bottomArea(container)).getByRole('button', { name: '关闭' }));
    expect(bottomArea(container).hidden).toBe(true);
    expect(shell.setLayout).not.toHaveBeenCalled();
  });

  it('F11 请求窗口切换全屏，菜单“文件→退出”请求退出应用', async () => {
    const user = userEvent.setup();
    const { shell } = renderShell();
    await shown();
    press({ code: 'F11' });
    expect(shell.windowAction).toHaveBeenLastCalledWith('toggleFullScreen');
    await user.click(screen.getByRole('menuitem', { name: /文件/ }));
    await user.click(screen.getByRole('menuitem', { name: '退出' }));
    expect(shell.windowAction).toHaveBeenLastCalledWith('quit');
  });

  it('模块登记的命令可以用默认快捷键和菜单项执行', async () => {
    const user = userEvent.setup();
    renderShell();
    await shown();
    press({ code: 'KeyH', ctrlKey: true });
    press(TOGGLE_BOTTOM_PANEL);
    await user.click(screen.getByRole('menuitem', { name: /机器/ }));
    await user.click(screen.getByRole('menuitem', { name: /问好/ }));
    expect(within(screen.getByRole('tabpanel', { name: '消息' })).getAllByText('你好')).toHaveLength(2);
  });

  it('Alt+助记字母打开对应的菜单；没有登记项的菜单不响应', async () => {
    renderShell();
    await shown();
    press({ code: 'KeyV', altKey: true });
    expect(await screen.findByRole('menu', { name: /视图/ })).toBeTruthy();
    press({ code: 'KeyH', altKey: true });
    expect(screen.queryByRole('menu', { name: /帮助/ })).toBeNull();
  });

  it('画面处理过的按键（defaultPrevented）外壳不再响应；没有处理的组合键照常响应', async () => {
    const { container, shell } = renderShell();
    await shown();
    // 模拟画面区：自己处理按键并阻止默认行为，事件冒泡到 window 时已是 defaultPrevented。
    const screenElement = document.createElement('div');
    screenElement.tabIndex = 0;
    screenElement.addEventListener('keydown', (event) => event.preventDefault());
    document.body.append(screenElement);
    screenElement.focus();
    fireEvent.keyDown(screenElement, { code: 'KeyJ', ctrlKey: true });
    expect(bottomArea(container).hidden).toBe(true);
    expect(shell.setLayout).not.toHaveBeenCalled();
    fireEvent.keyDown(document.body, { code: 'KeyJ', ctrlKey: true });
    expect(bottomArea(container).hidden).toBe(false);
    screenElement.remove();
  });

  it('按住不放产生的重复按键只响应一次', async () => {
    const { container } = renderShell();
    await shown();
    press(TOGGLE_BOTTOM_PANEL);
    press({ ...TOGGLE_BOTTOM_PANEL, repeat: true });
    expect(bottomArea(container).hidden).toBe(false);
  });
});

describe('消息', () => {
  it('其他模块经外壳接口追加的消息显示时间、级别与文字，清空后回到提示', async () => {
    const user = userEvent.setup();
    const { container } = renderShell();
    await shown();
    press(TOGGLE_BOTTOM_PANEL);
    await user.click(screen.getByRole('button', { name: '丙' }));
    await user.click(screen.getByRole('button', { name: '追加消息' }));
    const panel = bottomArea(container);
    expect(within(panel).getByText('核心已停止')).toBeTruthy();
    expect(within(panel).getByText('09:05:07')).toBeTruthy();
    expect(within(panel).getByText('错误')).toBeTruthy();
    expect(within(panel).queryByText('暂无消息')).toBeNull();
    await user.click(within(panel).getByRole('button', { name: '清空' }));
    expect(within(panel).getByText('暂无消息')).toBeTruthy();
    expect((within(panel).getByRole('button', { name: '清空' }) as HTMLButtonElement).disabled).toBe(true);
  });
});

describe('没有页面时与全屏', () => {
  it('关闭最后一个标签后页面区显示淡色标志和一句提示，没有标签栏', async () => {
    const user = userEvent.setup();
    const { container } = renderShell();
    await shown();
    await user.click(screen.getByRole('button', { name: '关闭：甲页' }));
    expect(screen.getByText(/没有打开的页面/)).toBeTruthy();
    expect(container.querySelector('.ui-empty-workspace__logo svg')).not.toBeNull();
    expect(screen.queryByRole('tablist', TABS_NAME)).toBeNull();
    // 没有页面时没有边栏，折叠按钮置灰。
    expect((screen.getByRole('button', { name: '切换边栏' }) as HTMLButtonElement).disabled).toBe(true);
    // 再点活动栏按钮重新打开页面。
    await user.click(screen.getByRole('button', { name: '甲' }));
    expect(screen.getByRole('tab', { name: '甲页' })).toBeTruthy();
  });

  it('登记为不可关闭的页面（closable: false）标签上没有关闭按钮，其他页面仍有', async () => {
    const user = userEvent.setup();
    const fixed: ShellModule = {
      name: 'e',
      pages: [
        { id: 'e.fixed', title: '固定页', icon: ICON, Component: () => <p>固定页内容</p>, openOnStart: true, closable: false },
        { id: 'e.other', title: '普通页', icon: ICON, Component: () => <p>普通页内容</p> }
      ],
      activityItems: [{ id: 'e.btn', label: '戊', icon: ICON, page: 'e.other' }]
    };
    renderShell({ modules: [fixed] });
    await shown();
    expect(screen.getByRole('tab', { name: '固定页' })).toBeTruthy();
    expect(screen.queryByRole('button', { name: '关闭：固定页' })).toBeNull();
    await user.click(screen.getByRole('button', { name: '戊' }));
    expect(screen.getByRole('button', { name: '关闭：普通页' })).toBeTruthy();
    await user.click(screen.getByRole('button', { name: '关闭：普通页' }));
    expect(screen.queryByRole('tab', { name: '普通页' })).toBeNull();
    expect(screen.getByRole('tab', { name: '固定页' })).toBeTruthy();
  });

  it('单例页面重复打开只聚焦已有标签', async () => {
    const user = userEvent.setup();
    renderShell();
    await shown();
    await user.click(screen.getByRole('button', { name: '乙' }));
    await user.click(screen.getByRole('button', { name: '甲' }));
    await user.click(screen.getByRole('button', { name: '乙' }));
    expect(within(screen.getByRole('tablist', TABS_NAME)).getAllByRole('tab')).toHaveLength(2);
  });

  it('全屏时只显示页面：标题栏、活动栏、标签栏、状态栏与各面板都隐藏，退出全屏后恢复，页面不重新挂载', async () => {
    const { container, emitWindowState } = renderShell();
    await shown();
    press(TOGGLE_SIDE_BAR);
    press(TOGGLE_BOTTOM_PANEL);
    const page = screen.getByText('甲页内容');
    emitWindowState({ maximized: false, fullScreen: true });
    expect(screen.queryByRole('banner')).toBeNull();
    expect(screen.queryByLabelText('活动栏')).toBeNull();
    expect(screen.queryByRole('tablist', TABS_NAME)).toBeNull();
    expect(screen.queryByLabelText('状态栏')).toBeNull();
    expect(sideArea(container).hidden).toBe(true);
    expect(bottomArea(container).hidden).toBe(true);
    expect(screen.getByText('甲页内容')).toBe(page);
    emitWindowState({ maximized: false, fullScreen: false });
    expect(screen.getByRole('banner')).toBeTruthy();
    expect(screen.getByRole('tablist', TABS_NAME)).toBeTruthy();
    expect(sideArea(container).hidden).toBe(false);
    expect(bottomArea(container).hidden).toBe(false);
    expect(screen.getByText('甲页内容')).toBe(page);
  });

  it('状态栏：模块登记的状态项分别显示在左侧与右侧，右侧最后是通知按钮', async () => {
    const modules: readonly ShellModule[] = [
      {
        name: 'a',
        statusItems: [
          { id: 'a.left', align: 'start', Component: () => <span>左项</span> },
          { id: 'a.right', align: 'end', Component: () => <span>右项</span> }
        ]
      }
    ];
    renderShell({ modules });
    await screen.findByLabelText('状态栏');
    const status = screen.getByLabelText('状态栏');
    expect(status.querySelector('.ui-status-bar__start')?.textContent).toBe('左项');
    const end = status.querySelector('.ui-status-bar__end') as HTMLElement;
    expect(end.textContent).toBe('右项');
    expect(end.lastElementChild?.getAttribute('aria-label')).toBe('通知');
  });
});

/** 模块自己的状态：命令的名称与可用状态读它，变化后经 refreshCommands 通知外壳。 */
function createDemo() {
  const demo = { on: false, enabled: true, entries: [{ id: 'x', label: '甲项' }] as Array<{ id: string; label: string; hint?: string; disabled?: boolean }>, ran: [] as string[], api: undefined as ShellApi | undefined, hostMounts: 0 };
  const module: ShellModule = {
    name: 'd',
    pages: [{ id: 'd.page', title: '丁页', icon: ICON, Component: () => <input aria-label="输入框" />, openOnStart: true }],
    commands: [{ id: 'd.toggle', title: () => (demo.on ? '关闭' : '打开'), defaultShortcut: 'F5', enabled: () => demo.enabled, run: () => demo.ran.push('toggle') }],
    menuItems: [
      { menu: 'machine', command: 'd.toggle' },
      { menu: 'machine', submenu: { id: 'd.sub', title: '子菜单', entries: () => demo.entries.map((entry) => ({ ...entry, run: () => demo.ran.push(entry.id) })) } }
    ],
    Host: function DemoHost() {
      const shell = useShell();
      useEffect(() => {
        demo.api = shell;
        demo.hostMounts += 1;
      }, [shell]);
      return null;
    }
  };
  return { demo, module };
}

describe('命令状态刷新、子菜单与常驻组件', () => {
  it('模块状态变化后调用 refreshCommands，菜单里命令的名称与置灰随之更新', async () => {
    const { demo, module } = createDemo();
    renderShell({ modules: [module] });
    await shown();
    await userEvent.click(screen.getByRole('menuitem', { name: '机器(M)' }));
    expect(popupItems()[0]?.textContent).toContain('打开');
    expect(popupItems()[0]?.getAttribute('aria-disabled')).toBeNull();
    // 菜单打开时模块状态变了：外壳重新计算后菜单项跟着变。
    demo.on = true;
    demo.enabled = false;
    act(() => demo.api?.refreshCommands());
    expect(popupItems()[0]?.textContent).toContain('关闭');
    expect(popupItems()[0]?.getAttribute('aria-disabled')).toBe('true');
  });

  it('命令置灰时快捷键不起作用，可用时起作用', async () => {
    const { demo, module } = createDemo();
    renderShell({ modules: [module] });
    await shown();
    demo.enabled = false;
    press({ key: 'F5', code: 'F5' });
    expect(demo.ran).toEqual([]);
    demo.enabled = true;
    press({ key: 'F5', code: 'F5' });
    expect(demo.ran).toEqual(['toggle']);
  });

  it('不带 Ctrl 或 Alt 的快捷键在输入框里不触发，在页面其他位置触发', async () => {
    const { demo, module } = createDemo();
    renderShell({ modules: [module] });
    await shown();
    fireEvent.keyDown(screen.getByLabelText('输入框'), { key: 'F5', code: 'F5' });
    expect(demo.ran).toEqual([]);
    fireEvent.keyDown(document.body, { key: 'F5', code: 'F5' });
    expect(demo.ran).toEqual(['toggle']);
  });

  it('子菜单：菜单项带展开箭头，子菜单项来自模块当前的数据，选中后执行并关闭菜单，置灰的项带提示文字', async () => {
    const { demo, module } = createDemo();
    demo.entries = [
      { id: 'x', label: '甲项' },
      { id: 'y', label: '乙项', hint: '暂未支持', disabled: true }
    ];
    renderShell({ modules: [module] });
    await shown();
    await userEvent.click(screen.getByRole('menuitem', { name: '机器(M)' }));
    const parent = screen.getByRole('menuitem', { name: '子菜单' });
    expect(parent.getAttribute('aria-haspopup')).toBe('menu');
    await userEvent.hover(parent);
    const submenu = screen.getByRole('menu', { name: '子菜单' });
    expect(within(submenu).getByRole('menuitem', { name: /乙项/ }).textContent).toContain('暂未支持');
    expect(within(submenu).getByRole('menuitem', { name: /乙项/ }).getAttribute('aria-disabled')).toBe('true');
    await userEvent.click(within(submenu).getByRole('menuitem', { name: '甲项' }));
    expect(demo.ran).toEqual(['x']);
    expect(screen.queryByRole('menu')).toBeNull();
  });

  it('子菜单没有项时这一项置灰，不能展开；数据变化后再打开能看到新项', async () => {
    const { demo, module } = createDemo();
    demo.entries = [];
    renderShell({ modules: [module] });
    await shown();
    await userEvent.click(screen.getByRole('menuitem', { name: '机器(M)' }));
    expect(screen.getByRole('menuitem', { name: '子菜单' }).getAttribute('aria-disabled')).toBe('true');
    await userEvent.hover(screen.getByRole('menuitem', { name: '子菜单' }));
    expect(screen.queryByRole('menu', { name: '子菜单' })).toBeNull();
    await userEvent.keyboard('{Escape}');
    demo.entries = [{ id: 'z', label: '新项' }];
    await userEvent.click(screen.getByRole('menuitem', { name: '机器(M)' }));
    await userEvent.hover(screen.getByRole('menuitem', { name: '子菜单' }));
    expect(within(screen.getByRole('menu', { name: '子菜单' })).getByRole('menuitem', { name: '新项' })).toBeTruthy();
  });

  it('常驻组件在外壳启动后挂载，页面关闭、全屏时仍在，不重复挂载', async () => {
    const { demo, module } = createDemo();
    const { emitWindowState } = renderShell({ modules: [module] });
    await shown();
    expect(demo.hostMounts).toBeGreaterThan(0);
    const mounts = demo.hostMounts;
    emitWindowState({ maximized: false, fullScreen: true });
    await userEvent.keyboard('{F11}');
    expect(demo.hostMounts).toBe(mounts);
    expect(demo.api).toBeDefined();
  });
});

describe('登记有误', () => {
  it('快捷键冲突时界面只显示错误说明，并在控制台报告全部问题', async () => {
    const error = vi.spyOn(console, 'error').mockImplementation(() => undefined);
    const clash: ShellModule = { name: 'a', commands: [{ id: 'a.go', title: '执行', defaultShortcut: 'Ctrl+B', run: () => undefined }] };
    renderShell({ modules: [clash] });
    expect(await screen.findByText(/外壳登记有误/)).toBeTruthy();
    expect(screen.queryByRole('tablist', TABS_NAME)).toBeNull();
    const reported = error.mock.calls[0]?.[0] as Error;
    expect(reported.message).toMatch(/默认快捷键都是 Ctrl\+B/);
    error.mockRestore();
  });
});
