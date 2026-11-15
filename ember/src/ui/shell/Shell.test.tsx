/*
 * ------------------------------------------------------------------------
 * 名称：Shell.test.tsx
 * 说明：外壳的测试：显示区与菜单来自登记表（没有视图菜单、活动栏、标签栏、边栏、右边栏）、系统栏（大尺寸）里的面板按钮只有图标并按“展开、切换、收起”工作、按钮按下态随面板收起恢复、
 *       面板没有页签条且收起时内容卸载、指示灯、全局快捷键与画面优先、退出与全屏经窗口接口、对话框同时只开一个、全屏只显示显示区、底部面板高度的读取与保存、命令状态刷新、子菜单、常驻组件、登记错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的 window.ember；显示区与面板是测试里写的简单组件，不引用运行模块。
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
const DEFAULT_LAYOUT: ShellLayout = { bottomPanelHeight: null, selectedMachine: null, autoCollapsePanels: [] };

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

/** 模块自己的状态：命令的名称与可用状态读它，变化后经 refreshCommands 通知外壳。 */
function createDemo() {
  const demo = {
    on: false,
    enabled: true,
    lit: false,
    entries: [{ id: 'x', label: '甲项' }] as Array<{ id: string; label: string; hint?: string; disabled?: boolean; checked?: boolean }>,
    ran: [] as string[],
    api: undefined as ShellApi | undefined,
    hostMounts: 0,
    panelMounts: 0
  };
  const module: ShellModule = {
    name: 'd',
    screen: () => <input aria-label="输入框" />,
    commands: [{ id: 'd.toggle', title: () => (demo.on ? '关闭' : '打开'), defaultShortcut: 'F5', enabled: () => demo.enabled, run: () => demo.ran.push('toggle') }],
    menuItems: [
      { menu: 'machine', command: 'd.toggle' },
      { menu: 'machine', submenu: { id: 'd.sub', title: '子菜单', entries: () => demo.entries.map((entry) => ({ ...entry, run: () => demo.ran.push(entry.id) })) } }
    ],
    bottomPanels: [
      {
        id: 'd.device',
        title: '设备',
        icon: ICON,
        Component: function DevicePanel() {
          useEffect(() => {
            demo.panelMounts += 1;
          }, []);
          return <p>设备内容</p>;
        },
        lamp: () => ({ lit: demo.lit, label: '读写灯' })
      }
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

/** 对话框内容：点按钮经外壳接口弹出通知。 */
function NotifyingDialog() {
  const { notify } = useShell();
  return (
    <button type="button" onClick={() => notify('来自对话框的通知', 'success')}>
      发通知
    </button>
  );
}

/** 显示区、两个面板、一个对话框与两条命令的模块。 */
const MODULES: readonly ShellModule[] = [
  {
    name: 'a',
    screen: () => <p>显示区内容</p>,
    commands: [
      { id: 'a.hello', title: '问好', defaultShortcut: 'Ctrl+H', run: () => undefined },
      { id: 'a.open', title: '打开设置', defaultShortcut: 'F6', run: (shell) => shell.openDialog('a.settings') }
    ],
    menuItems: [{ menu: 'machine', command: 'a.hello' }],
    dialogs: [{ id: 'a.settings', title: '设置', Component: () => <p>设置内容</p>, width: 320 }],
    bottomPanels: [
      { id: 'a.run', title: '运行', icon: ICON, Component: () => <p>运行内容</p> },
      { id: 'a.device', title: '设备', icon: ICON, Component: () => <p>设备内容</p> }
    ]
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

/** 等外壳读完布局、显示出来。 */
async function shown(): Promise<void> {
  await screen.findByLabelText('系统栏');
}

function bottomArea(container: HTMLElement): HTMLElement {
  return container.querySelector('.ui-app-frame__bottom') as HTMLElement;
}

/** 打开的菜单里的全部菜单项（普通项是 menuitem，可勾选项是 menuitemcheckbox）。 */
function popupItems(): HTMLElement[] {
  return Array.from(document.querySelectorAll<HTMLElement>('.ui-menu__popup [role^="menuitem"]'));
}

/** 按下一个组合键（事件发在 body 上，冒泡到 window）。 */
function press(init: KeyboardEventInit): void {
  fireEvent.keyDown(document.body, init);
}

describe('登记表驱动的界面', () => {
  it('显示区、菜单都来自登记；没有活动栏、标签栏、边栏、右边栏与标题栏上的面板按钮', async () => {
    const { container } = renderShell();
    await shown();
    expect(screen.getByText('显示区内容')).toBeTruthy();
    expect(screen.queryByLabelText('活动栏')).toBeNull();
    expect(screen.queryByRole('tablist')).toBeNull();
    expect(container.querySelector('.ui-app-frame__side, .ui-app-frame__right')).toBeNull();
    for (const name of ['切换边栏', '切换底部面板', '切换右边栏']) expect(screen.queryByRole('button', { name })).toBeNull();
  });

  it('菜单栏只显示有登记项的菜单：文件、机器，没有视图、设置与帮助', async () => {
    renderShell();
    await shown();
    expect(screen.getAllByRole('menuitem').map((item) => item.textContent)).toEqual(['文件(F)', '机器(M)']);
  });

  it('读出保存的底部面板高度，展开时用保存的高度；没有保存过时用默认高度', async () => {
    const saved = renderShell({ layout: { bottomPanelHeight: 220 } });
    await shown();
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    expect(bottomArea(saved.container).style.height).toBe('220px');
    saved.unmount();
    const fresh = renderShell();
    await shown();
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    expect(bottomArea(fresh.container).style.height).toBe('130px');
  });
});

describe('系统栏里的面板按钮', () => {
  it('每个面板一个按钮，在系统栏左侧，只有图标：没有文字，名称在可访问名称与悬停提示里；右侧只有通知按钮', async () => {
    renderShell();
    await shown();
    const bar = screen.getByLabelText('系统栏');
    const start = bar.querySelector('.ui-status-bar__start') as HTMLElement;
    const buttons = within(start).getAllByRole('button');
    expect(buttons.map((button) => button.getAttribute('aria-label'))).toEqual(['运行', '设备']);
    for (const button of buttons) {
      expect(button.textContent).toBe('');
      expect(button.getAttribute('title')).toBe(button.getAttribute('aria-label'));
      expect(button.querySelector('svg')).not.toBeNull();
    }
    const end = bar.querySelector('.ui-status-bar__end') as HTMLElement;
    expect(within(end).getAllByRole('button').map((button) => button.getAttribute('aria-label'))).toEqual(['通知']);
  });

  it('面板按钮的外观可以随模块状态变化：图标、颜色与悬停提示取自 look，状态变化后调用 refreshCommands 更新', async () => {
    const state = { running: true };
    let api: ShellApi | undefined;
    const module: ShellModule = {
      name: 'a',
      Host: function Host() {
        const shell = useShell();
        useEffect(() => {
          api = shell;
        }, [shell]);
        return null;
      },
      bottomPanels: [
        {
          id: 'a.run',
          title: '运行',
          icon: ICON,
          look: () => (state.running ? { icon: ['M1 1'], tone: 'success', hint: '运行：运行中' } : { icon: ['M2 2'], hint: '运行：已暂停' }),
          Component: () => null
        }
      ]
    };
    renderShell({ modules: [module] });
    await shown();
    const button = screen.getByRole('button', { name: '运行' });
    expect(button.getAttribute('title')).toBe('运行：运行中');
    expect(button.getAttribute('data-tone')).toBe('success');
    expect(button.querySelector('path')?.getAttribute('d')).toBe('M1 1');
    state.running = false;
    act(() => api?.refreshCommands());
    expect(button.getAttribute('title')).toBe('运行：已暂停');
    expect(button.getAttribute('data-tone')).toBe('normal');
    expect(button.querySelector('path')?.getAttribute('d')).toBe('M2 2');
  });

  it('启动时面板关闭；点按钮展开并显示该面板的内容，没有页签条', async () => {
    const { container } = renderShell();
    await shown();
    expect(bottomArea(container)).toBeFalsy();
    expect(screen.queryByText('运行内容')).toBeNull();
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    expect(bottomArea(container).hidden).toBe(false);
    expect(within(screen.getByRole('region', { name: '运行' })).getByText('运行内容')).toBeTruthy();
    expect(screen.queryByRole('tab')).toBeNull();
    expect(screen.queryByRole('tablist')).toBeNull();
  });

  it('面板开着时：点同一个按钮收起，点另一个按钮切换内容；收起后内容卸载', async () => {
    const { container } = renderShell();
    await shown();
    const run = screen.getByRole('button', { name: '运行' });
    const device = screen.getByRole('button', { name: '设备' });
    await userEvent.click(run);
    await userEvent.click(device);
    expect(screen.getByText('设备内容')).toBeTruthy();
    expect(screen.queryByText('运行内容')).toBeNull();
    await userEvent.click(device);
    expect(screen.queryByText('设备内容')).toBeNull();
    expect(bottomArea(container)).toBeFalsy();
  });

  it('按钮的按下态只在面板开着并停在该面板时出现，收起后恢复常态；没有快捷键，也不保存', async () => {
    const { shell } = renderShell();
    await shown();
    const run = screen.getByRole('button', { name: '运行' });
    const device = screen.getByRole('button', { name: '设备' });
    expect(run.getAttribute('aria-pressed')).toBe('false');
    await userEvent.click(run);
    expect(run.getAttribute('aria-pressed')).toBe('true');
    expect(device.getAttribute('aria-pressed')).toBe('false');
    await userEvent.click(device);
    expect(run.getAttribute('aria-pressed')).toBe('false');
    expect(device.getAttribute('aria-pressed')).toBe('true');
    await userEvent.click(device);
    expect(device.getAttribute('aria-pressed')).toBe('false');
    press({ code: 'KeyJ', ctrlKey: true });
    expect(screen.queryByRole('region')).toBeNull();
    expect(shell.setLayout).not.toHaveBeenCalled();
  });

  it('面板按钮的指示灯：取值函数返回状态就显示亮灭，模块调用 refreshCommands 后更新', async () => {
    const { demo, module } = createDemo();
    renderShell({ modules: [module] });
    await shown();
    expect(screen.getByRole('img', { name: '读写灯' }).getAttribute('data-lit')).toBe('false');
    demo.lit = true;
    act(() => demo.api?.refreshCommands());
    expect(screen.getByRole('img', { name: '读写灯' }).getAttribute('data-lit')).toBe('true');
  });

  it('面板内容每次展开重新挂载，收起后不再挂载', async () => {
    const { demo, module } = createDemo();
    renderShell({ modules: [module] });
    await shown();
    expect(demo.panelMounts).toBe(0);
    await userEvent.click(screen.getByRole('button', { name: '设备' }));
    await userEvent.click(screen.getByRole('button', { name: '设备' }));
    await userEvent.click(screen.getByRole('button', { name: '设备' }));
    expect(demo.panelMounts).toBe(2);
  });

  it('系统栏：左侧先是面板按钮，再是模块登记的左侧状态项；右侧是模块登记的右侧状态项，最后是通知按钮', async () => {
    const modules: readonly ShellModule[] = [
      {
        name: 'a',
        statusItems: [
          { id: 'a.left', align: 'start', Component: () => <span>左项</span> },
          { id: 'a.right', align: 'end', Component: () => <span>右项</span> }
        ],
        bottomPanels: [{ id: 'a.run', title: '运行', icon: ICON, Component: () => null }]
      }
    ];
    renderShell({ modules });
    await shown();
    const bar = screen.getByLabelText('系统栏');
    const start = bar.querySelector('.ui-status-bar__start') as HTMLElement;
    expect(start.children[0]?.getAttribute('aria-label')).toBe('运行');
    expect(start.children[1]?.textContent).toBe('左项');
    const end = bar.querySelector('.ui-status-bar__end') as HTMLElement;
    expect(end.children[0]?.textContent).toBe('右项');
    expect(end.lastElementChild?.getAttribute('aria-label')).toBe('通知');
  });

  it('系统栏用大尺寸', async () => {
    renderShell();
    await shown();
    expect(document.documentElement.classList.contains('ui-status-bar-large')).toBe(true);
  });
});

/** 一个登记了“自动收缩”的面板（内容里有“完成一次操作”按钮）和一个没登记的面板。 */
const AUTO_MODULES: readonly ShellModule[] = [
  {
    name: 'a',
    screen: () => <p>显示区内容</p>,
    bottomPanels: [
      {
        id: 'a.run',
        title: '运行',
        icon: ICON,
        autoCollapse: true,
        Component: ({ finishAction }) => (
          <button type="button" onClick={finishAction}>
            完成一次操作
          </button>
        )
      },
      { id: 'a.plain', title: '普通', icon: ICON, Component: () => <p>普通内容</p> }
    ]
  }
];

describe('面板的自动收缩开关', () => {
  it('登记了自动收缩的面板，标题栏有只有图标的开关，在关闭按钮左边；没有登记的面板没有开关', async () => {
    renderShell({ modules: AUTO_MODULES });
    await shown();
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    const header = screen.getByRole('region', { name: '运行' }).querySelector('.ember-shell-panel__header') as HTMLElement;
    const buttons = within(header).getAllByRole('button');
    expect(buttons.map((button) => button.getAttribute('aria-label'))).toEqual(['自动收缩', '关闭']);
    expect(buttons[0]?.textContent).toBe('');
    expect(buttons[0]?.getAttribute('aria-pressed')).toBe('false');
    await userEvent.click(screen.getByRole('button', { name: '普通' }));
    expect(screen.queryByRole('button', { name: '自动收缩' })).toBeNull();
  });

  it('开关关着：面板里完成操作后面板保持展开', async () => {
    renderShell({ modules: AUTO_MODULES });
    await shown();
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    await userEvent.click(screen.getByRole('button', { name: '完成一次操作' }));
    expect(screen.getByRole('region', { name: '运行' })).toBeTruthy();
  });

  it('开关开着：面板里完成操作后面板收起，系统栏按钮恢复常态；再展开开关仍然开着', async () => {
    renderShell({ modules: AUTO_MODULES });
    await shown();
    const run = screen.getByRole('button', { name: '运行' });
    await userEvent.click(run);
    await userEvent.click(screen.getByRole('button', { name: '自动收缩' }));
    expect(screen.getByRole('button', { name: '自动收缩' }).getAttribute('aria-pressed')).toBe('true');
    await userEvent.click(screen.getByRole('button', { name: '完成一次操作' }));
    expect(screen.queryByRole('region')).toBeNull();
    expect(run.getAttribute('aria-pressed')).toBe('false');
    await userEvent.click(run);
    expect(screen.getByRole('button', { name: '自动收缩' }).getAttribute('aria-pressed')).toBe('true');
  });

  it('开关按面板各自保存：改动时把开着的面板清单交给 setLayout，再关掉时清单为空', async () => {
    const { shell } = renderShell({ modules: AUTO_MODULES });
    await shown();
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    await userEvent.click(screen.getByRole('button', { name: '自动收缩' }));
    expect(shell.setLayout).toHaveBeenLastCalledWith({ autoCollapsePanels: ['a.run'] });
    await userEvent.click(screen.getByRole('button', { name: '自动收缩' }));
    expect(shell.setLayout).toHaveBeenLastCalledWith({ autoCollapsePanels: [] });
  });

  it('读出保存的清单：启动后该面板的开关就是开着的', async () => {
    renderShell({ modules: AUTO_MODULES, layout: { autoCollapsePanels: ['a.run'] } });
    await shown();
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    expect(screen.getByRole('button', { name: '自动收缩' }).getAttribute('aria-pressed')).toBe('true');
    await userEvent.click(screen.getByRole('button', { name: '完成一次操作' }));
    expect(screen.queryByRole('region')).toBeNull();
  });

  it('面板收起后已经切到别的面板时，迟到的“完成操作”不会收起别的面板', async () => {
    let finish: (() => void) | undefined;
    const modules: readonly ShellModule[] = [
      {
        name: 'a',
        screen: () => <p>显示区内容</p>,
        bottomPanels: [
          {
            id: 'a.run',
            title: '运行',
            icon: ICON,
            autoCollapse: true,
            Component: function Run({ finishAction }) {
              useEffect(() => {
                finish = finishAction;
              });
              return <p>运行内容</p>;
            }
          },
          { id: 'a.plain', title: '普通', icon: ICON, Component: () => <p>普通内容</p> }
        ]
      }
    ];
    renderShell({ modules, layout: { autoCollapsePanels: ['a.run'] } });
    await shown();
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    const staleFinish = finish;
    await userEvent.click(screen.getByRole('button', { name: '普通' }));
    act(() => staleFinish?.());
    expect(screen.getByText('普通内容')).toBeTruthy();
  });
});

describe('快捷键与菜单', () => {
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

  it('模块登记的命令可以用菜单项执行，菜单项显示默认快捷键', async () => {
    const user = userEvent.setup();
    renderShell();
    await shown();
    await user.click(screen.getByRole('menuitem', { name: /机器/ }));
    const item = popupItems().find((entry) => entry.textContent?.startsWith('问好'));
    expect(item?.textContent).toContain('Ctrl+H');
  });

  it('Alt+助记字母打开对应的菜单；没有登记项的菜单不响应', async () => {
    renderShell();
    await shown();
    press({ code: 'KeyM', altKey: true });
    expect(await screen.findByRole('menu', { name: /机器/ })).toBeTruthy();
    press({ code: 'KeyH', altKey: true });
    expect(screen.queryByRole('menu', { name: /帮助/ })).toBeNull();
  });

  it('显示区处理过的按键（defaultPrevented）外壳不再响应；没有处理的组合键照常响应', async () => {
    renderShell();
    await shown();
    const screenElement = document.createElement('div');
    screenElement.tabIndex = 0;
    screenElement.addEventListener('keydown', (event) => event.preventDefault());
    document.body.append(screenElement);
    screenElement.focus();
    fireEvent.keyDown(screenElement, { code: 'F6' });
    expect(screen.queryByText('设置内容')).toBeNull();
    fireEvent.keyDown(document.body, { code: 'F6' });
    expect(await screen.findByText('设置内容')).toBeTruthy();
    screenElement.remove();
  });
});

describe('对话框', () => {
  it('命令打开对话框，标题与内容来自登记；同一个对话框再次打开时不会重复出现', async () => {
    renderShell();
    await shown();
    press({ code: 'F6' });
    expect(await screen.findByText('设置内容')).toBeTruthy();
    press({ code: 'F6' });
    expect(screen.getAllByText('设置内容')).toHaveLength(1);
  });

  it('对话框内容渲染在组件库的对话框层（取不到 useNotify），经外壳接口的 notify 仍能弹出通知', async () => {
    renderShell({
      modules: [
        {
          name: 'b',
          screen: () => <p>显示区内容</p>,
          commands: [{ id: 'b.open', title: '打开', defaultShortcut: 'F6', run: (shell) => shell.openDialog('b.dialog') }],
          dialogs: [{ id: 'b.dialog', title: '通知', Component: NotifyingDialog, width: 320 }]
        }
      ]
    });
    await shown();
    press({ code: 'F6' });
    fireEvent.click(await screen.findByRole('button', { name: '发通知' }));
    expect(await screen.findByText('来自对话框的通知')).toBeTruthy();
  });
});

describe('全屏', () => {
  it('全屏时只显示显示区：标题栏、系统栏与面板都隐藏，退出全屏后恢复，显示区不重新挂载', async () => {
    const { emitWindowState } = renderShell();
    await shown();
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    const content = screen.getByText('显示区内容');
    emitWindowState({ maximized: false, fullScreen: true });
    expect(screen.queryByRole('banner')).toBeNull();
    expect(screen.queryByLabelText('系统栏')).toBeNull();
    expect(screen.getByText('显示区内容')).toBe(content);
    emitWindowState({ maximized: false, fullScreen: false });
    expect(screen.getByRole('banner')).toBeTruthy();
    expect(screen.getByLabelText('系统栏')).toBeTruthy();
    expect(screen.getByText('显示区内容')).toBe(content);
  });
});

describe('命令状态刷新、子菜单与常驻组件', () => {
  it('模块状态变化后调用 refreshCommands，菜单里命令的名称与置灰随之更新', async () => {
    const { demo, module } = createDemo();
    renderShell({ modules: [module] });
    await shown();
    await userEvent.click(screen.getByRole('menuitem', { name: '机器(M)' }));
    expect(popupItems()[0]?.textContent).toContain('打开');
    expect(popupItems()[0]?.getAttribute('aria-disabled')).toBeNull();
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

  it('子菜单里带 checked 的项是单选项：选中的一项前面有对勾，选中只执行、不改别的', async () => {
    const { demo, module } = createDemo();
    demo.entries = [
      { id: 'x', label: '甲项', checked: true },
      { id: 'y', label: '乙项' }
    ];
    renderShell({ modules: [module] });
    await shown();
    await userEvent.click(screen.getByRole('menuitem', { name: '机器(M)' }));
    await userEvent.hover(screen.getByRole('menuitem', { name: '子菜单' }));
    const submenu = screen.getByRole('menu', { name: '子菜单' });
    expect(within(submenu).getByRole('menuitemradio', { name: '甲项' }).getAttribute('aria-checked')).toBe('true');
    expect(within(submenu).getByRole('menuitem', { name: '乙项' })).toBeTruthy();
    await userEvent.click(within(submenu).getByRole('menuitem', { name: '乙项' }));
    expect(demo.ran).toEqual(['y']);
  });

  it('常驻组件在外壳启动后挂载，全屏时仍在，不重复挂载', async () => {
    const { demo, module } = createDemo();
    const { emitWindowState } = renderShell({ modules: [module] });
    await shown();
    expect(demo.hostMounts).toBeGreaterThan(0);
    const mounts = demo.hostMounts;
    emitWindowState({ maximized: false, fullScreen: true });
    expect(demo.hostMounts).toBe(mounts);
    expect(demo.api).toBeDefined();
  });
});

describe('登记有误', () => {
  it('快捷键冲突时界面只显示错误说明，并在控制台报告全部问题', async () => {
    const error = vi.spyOn(console, 'error').mockImplementation(() => undefined);
    const clash: ShellModule = { name: 'a', commands: [{ id: 'a.go', title: '执行', defaultShortcut: 'F11', run: () => undefined }] };
    renderShell({ modules: [clash] });
    expect(await screen.findByText(/外壳登记有误/)).toBeTruthy();
    expect(screen.queryByLabelText('系统栏')).toBeNull();
    const reported = error.mock.calls[0]?.[0] as Error;
    expect(reported.message).toMatch(/默认快捷键都是 F11/);
    error.mockRestore();
  });
});
