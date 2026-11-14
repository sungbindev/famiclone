/*
 * ------------------------------------------------------------------------
 * 名称：runtime-module.test.tsx
 * 说明：运行模块在外壳里的登记项的测试：命令的可用状态与名称随机器状态变化（菜单置灰、勾选）、默认快捷键与它们的实际效果（F9、F8、F7、Ctrl+F7、Ctrl+O）、焦点在输入控件时不带 Ctrl 的快捷键不触发、
 *       “文件”菜单的“开机…”与“最近打开”子菜单、关闭卡带、消息接入（底部面板的“消息”）、失败时的通知、边栏没打开时状态仍是最新的、登记表无冲突。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：外壳与运行模块都是真实的，核心接口是假的；运行页用一个简单的可聚焦区域代替（真实的画面需要 Worker 与 WebGL）。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, fireEvent, render, screen, within } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { describe, expect, it, vi } from 'vitest';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { ShellLayout } from '../../contracts/shell/shell-state-service';
import { buildShellRegistry } from '../shell/build-registry';
import { SHELL_MODULE } from '../shell/shell-module';
import type { ShellModule } from '../shell/shell-registry';
import { Shell } from '../shell/Shell';
import { RuntimeController } from './runtime-controller';
import { FakeRuntimeBackend, machineStatus, settle } from './runtime-fakes';
import { createRuntimeModule, RUNTIME_COMMANDS } from './runtime-module';

const DEFAULT_LAYOUT: ShellLayout = { sideBarOpen: true, bottomPanelOpen: true, rightBarOpen: false, bottomPanelHeight: null };
const NES = 'D:\\roms\\nestest.nes';

/** 渲染外壳与运行模块；运行页换成一个带输入框的简单页面。 */
async function setup(options: { status?: ReturnType<typeof machineStatus> | undefined; recent?: string[]; layout?: Partial<ShellLayout> } = {}) {
  const fake = new FakeRuntimeBackend();
  if ('status' in options) fake.status = options.status;
  fake.recentPaths = options.recent ?? [];
  const controller = new RuntimeController();
  const module = createRuntimeModule(controller);
  const modules: ShellModule[] = [
    {
      ...module,
      pages: (module.pages ?? []).map((page) => ({
        ...page,
        Component: () => (
          <div role="application" tabIndex={0} aria-label="模拟器画面">
            <input aria-label="测试输入框" />
          </div>
        )
      }))
    }
  ];
  const shell = {
    getState: vi.fn(() => Promise.resolve({ window: null, layout: { ...DEFAULT_LAYOUT, ...options.layout } })),
    setLayout: vi.fn(),
    windowAction: vi.fn(),
    getWindowState: vi.fn(() => Promise.resolve({ maximized: false, fullScreen: false })),
    onWindowStateChange: () => () => undefined
  };
  const bridge = { ...fake.backend, shell } as unknown as EmberBridge;
  const result = render(
    <UiProvider>
      <Shell bridge={bridge} modules={modules} />
    </UiProvider>
  );
  await screen.findByRole('tablist', { name: '已打开的页面' });
  await act(async () => settle());
  return { fake, controller, shell, ...result };
}

/** 打开菜单栏里的菜单。 */
async function openMenu(name: string): Promise<void> {
  await userEvent.click(screen.getByRole('menuitem', { name }));
}

/** 当前打开的菜单里的全部菜单项（顶层弹层）。 */
function popupItems(): HTMLElement[] {
  return Array.from(document.querySelectorAll<HTMLElement>('.ui-menu__popup:not(.ui-menu__popup--sub) [role^="menuitem"]'));
}

/** 按文字找菜单项（文字以 label 开头，后面可以跟快捷键或提示）。 */
function item(label: string): HTMLElement {
  const found = Array.from(document.querySelectorAll<HTMLElement>('.ui-menu__popup [role^="menuitem"]')).find((element) => element.querySelector('.ui-menu__label')?.textContent === label);
  if (found === undefined) throw new Error(`没有菜单项：${label}`);
  return found;
}

const isDisabled = (element: HTMLElement): boolean => element.getAttribute('aria-disabled') === 'true';

/** 按下一个组合键，目标默认是 body。 */
async function press(init: KeyboardEventInit, target: Element = document.body): Promise<void> {
  fireEvent.keyDown(target, init);
  await act(async () => settle());
}

/** 底部面板里的消息文字。 */
function messageTexts(container: HTMLElement): string[] {
  const panel = container.querySelector('.ui-app-frame__bottom') as HTMLElement;
  return Array.from(panel.querySelectorAll('.ember-messages__text')).map((element) => element.textContent ?? '');
}

describe('登记表', () => {
  it('与外壳自己的登记项合在一起没有冲突，快捷键符合规定', () => {
    const registry = buildShellRegistry([SHELL_MODULE, createRuntimeModule(new RuntimeController())]);
    expect(Object.fromEntries(registry.shortcuts)).toMatchObject({
      'Ctrl+O': RUNTIME_COMMANDS.openCartridge,
      F9: RUNTIME_COMMANDS.powerToggle,
      F8: RUNTIME_COMMANDS.pauseToggle,
      F7: RUNTIME_COMMANDS.softReset,
      'Ctrl+F7': RUNTIME_COMMANDS.hardReset
    });
    expect(new Set(registry.shortcuts.values()).size).toBe(registry.shortcuts.size);
    expect(registry.hosts.map((host) => host.module)).toEqual(['runtime']);
  });
});

describe('“机器”菜单', () => {
  it('没有开机：上电、暂停、软复位、硬复位全部置灰，静音可用', async () => {
    await setup();
    await openMenu('机器(M)');
    expect(popupItems().map((element) => element.querySelector('.ui-menu__label')?.textContent)).toEqual(['上电', '继续', '软复位', '硬复位', '静音']);
    expect(['上电', '继续', '软复位', '硬复位'].map((label) => isDisabled(item(label)))).toEqual([true, true, true, true]);
    expect(isDisabled(item('静音'))).toBe(false);
  });

  it('断电：上电与硬复位可用，暂停（继续）与软复位置灰，快捷键显示在右侧', async () => {
    await setup({ status: machineStatus('off', 'paused', NES) });
    await openMenu('机器(M)');
    expect(['上电', '继续', '软复位', '硬复位'].map((label) => isDisabled(item(label)))).toEqual([false, true, true, false]);
    expect(item('上电').textContent).toContain('F9');
    expect(item('继续').textContent).toContain('F8');
    expect(item('软复位').textContent).toContain('F7');
    expect(item('硬复位').textContent).toContain('Ctrl+F7');
  });

  it('运行中：名称是“断电”“暂停”，全部可用；点击经控制器发请求', async () => {
    const { fake } = await setup({ status: machineStatus('on', 'running', NES) });
    await openMenu('机器(M)');
    expect(['断电', '暂停', '软复位', '硬复位'].map((label) => isDisabled(item(label)))).toEqual([false, false, false, false]);
    await userEvent.click(item('暂停'));
    await act(async () => settle());
    expect(fake.calls).toContain('runtime.pause {}');
    await openMenu('机器(M)');
    expect(item('继续')).toBeTruthy();
  });

  it('静音是勾选项，勾选状态与边栏一致', async () => {
    const { fake } = await setup();
    await openMenu('机器(M)');
    expect(item('静音').getAttribute('aria-checked')).toBe('false');
    await userEvent.click(item('静音'));
    await act(async () => settle());
    expect(fake.muted).toBe(true);
    expect(screen.getByRole('button', { name: '静音' }).getAttribute('aria-pressed')).toBe('true');
    // 勾选项选中后菜单保持打开，勾选状态随之更新。
    expect(item('静音').getAttribute('aria-checked')).toBe('true');
  });

  it('菜单没有“截图”', async () => {
    await setup();
    await openMenu('机器(M)');
    expect(screen.queryByRole('menuitem', { name: /截图/ })).toBeNull();
  });
});

describe('“文件”菜单', () => {
  it('依次是开机…、打开卡带…、最近打开、关闭卡带、退出；没有卡带时关闭卡带置灰，没有记录时最近打开置灰', async () => {
    await setup();
    await openMenu('文件(F)');
    expect(popupItems().map((element) => element.querySelector('.ui-menu__label')?.textContent)).toEqual(['开机…', '打开卡带…', '最近打开', '关闭卡带', '退出']);
    expect(item('打开卡带…').textContent).toContain('Ctrl+O');
    expect(isDisabled(item('关闭卡带'))).toBe(true);
    expect(isDisabled(item('最近打开'))).toBe(true);
    expect(isDisabled(item('开机…'))).toBe(false);
    expect(isDisabled(item('退出'))).toBe(false);
  });

  it('开机…的子菜单：经典 FC 可用，四个整机机型置灰并注明“暂未支持”', async () => {
    await setup();
    await openMenu('文件(F)');
    await userEvent.hover(item('开机…'));
    const submenu = screen.getByRole('menu', { name: '开机…' });
    const entries = Array.from(submenu.querySelectorAll<HTMLElement>('[role^="menuitem"]'));
    expect(entries.map((element) => element.querySelector('.ui-menu__label')?.textContent)).toEqual(['经典 FC', '小霸王 97', '小霸王 2000', '步步高 1.0', '步步高 2.0']);
    expect(entries.map(isDisabled)).toEqual([false, true, true, true, true]);
    expect(entries.slice(1).every((element) => element.textContent?.includes('暂未支持'))).toBe(true);
  });

  it('开机…→经典 FC 等同打开卡带：弹出文件选择（cartridges.open）', async () => {
    const { fake } = await setup();
    fake.pickedPath = NES;
    await openMenu('文件(F)');
    await userEvent.hover(item('开机…'));
    await userEvent.click(within(screen.getByRole('menu', { name: '开机…' })).getByRole('menuitem', { name: '经典 FC' }));
    await act(async () => settle());
    expect(fake.calls).toContain('cartridges.open {}');
    expect(screen.getAllByText('运行中').length).toBeGreaterThan(0);
  });

  it('选整机机型不会发请求', async () => {
    const { fake } = await setup();
    await openMenu('文件(F)');
    await userEvent.hover(item('开机…'));
    await userEvent.click(within(screen.getByRole('menu', { name: '开机…' })).getByRole('menuitem', { name: /小霸王 2000/ }));
    await act(async () => settle());
    expect(fake.calls.filter((call) => call.startsWith('cartridges.open'))).toEqual([]);
  });

  it('最近打开的子菜单列出卡带名，选中即重新打开', async () => {
    const { fake } = await setup({ recent: ['D:\\roms\\a.nes', 'D:\\roms\\b.nes'] });
    await openMenu('文件(F)');
    expect(isDisabled(item('最近打开'))).toBe(false);
    await userEvent.hover(item('最近打开'));
    const submenu = screen.getByRole('menu', { name: '最近打开' });
    expect(within(submenu).getAllByRole('menuitem').map((element) => element.textContent)).toEqual(['a.nes', 'b.nes']);
    await userEvent.click(within(submenu).getByRole('menuitem', { name: 'b.nes' }));
    await act(async () => settle());
    expect(fake.calls).toContain('cartridges.openRecent {"id":2}');
    expect(screen.queryByRole('menu')).toBeNull();
  });

  it('关闭卡带：有卡带时可用，点击后取出卡带，状态变为未开机', async () => {
    const { fake } = await setup({ status: machineStatus('on', 'running', NES) });
    await openMenu('文件(F)');
    expect(isDisabled(item('关闭卡带'))).toBe(false);
    await userEvent.click(item('关闭卡带'));
    await act(async () => settle());
    expect(fake.calls).toContain('cartridges.eject {}');
    await openMenu('文件(F)');
    expect(isDisabled(item('关闭卡带'))).toBe(true);
  });

  it('退出经窗口接口', async () => {
    const { shell } = await setup();
    await openMenu('文件(F)');
    await userEvent.click(item('退出'));
    expect(shell.windowAction).toHaveBeenCalledWith('quit');
  });
});

describe('快捷键', () => {
  it('F9 上电或断电', async () => {
    const { fake } = await setup({ status: machineStatus('off', 'paused', NES) });
    await press({ key: 'F9', code: 'F9' });
    expect(fake.calls).toContain('runtime.powerOn {"resume":true}');
    await press({ key: 'F9', code: 'F9' });
    expect(fake.calls).toContain('runtime.powerOff {}');
  });

  it('F8 暂停或继续，F7 软复位，Ctrl+F7 硬复位', async () => {
    const { fake } = await setup({ status: machineStatus('on', 'running', NES) });
    await press({ key: 'F8', code: 'F8' });
    expect(fake.calls).toContain('runtime.pause {}');
    await press({ key: 'F8', code: 'F8' });
    expect(fake.calls).toContain('runtime.resume {}');
    await press({ key: 'F7', code: 'F7' });
    expect(fake.calls).toContain('runtime.reset {"kind":"soft"}');
    await press({ key: 'F7', code: 'F7', ctrlKey: true });
    expect(fake.calls).toContain('runtime.reset {"kind":"hard"}');
  });

  it('Ctrl+O 打开卡带', async () => {
    const { fake } = await setup();
    await press({ key: 'o', code: 'KeyO', ctrlKey: true });
    expect(fake.calls).toContain('cartridges.open {}');
  });

  it('命令置灰时快捷键不起作用：没有开机时 F9、F8、F7 都不发请求，断电时 F8 与 F7 不发请求', async () => {
    const { fake } = await setup();
    for (const key of ['F9', 'F8', 'F7']) await press({ key, code: key });
    expect(fake.calls.filter((call) => call.startsWith('runtime.') && !call.startsWith('runtime.status'))).toEqual([]);

    const off = await setup({ status: machineStatus('off', 'paused', NES) });
    await press({ key: 'F8', code: 'F8' });
    await press({ key: 'F7', code: 'F7' });
    expect(off.fake.calls.filter((call) => /runtime\.(pause|resume|reset)/.test(call))).toEqual([]);
  });

  it('焦点在输入控件里时，不带 Ctrl 或 Alt 的快捷键（F9）不触发，带 Ctrl 的（Ctrl+O）仍然触发', async () => {
    const { fake } = await setup({ status: machineStatus('off', 'paused', NES) });
    const input = screen.getByLabelText('测试输入框');
    await press({ key: 'F9', code: 'F9' }, input);
    expect(fake.calls.filter((call) => call.startsWith('runtime.powerOn'))).toEqual([]);
    await press({ key: 'o', code: 'KeyO', ctrlKey: true }, input);
    expect(fake.calls).toContain('cartridges.open {}');
    await press({ key: 'F9', code: 'F9' }, screen.getByRole('application'));
    expect(fake.calls).toContain('runtime.powerOn {"resume":true}');
  });

  it('画面已经处理的按键（defaultPrevented）外壳不再响应；F 键与 Ctrl 组合在画面获得焦点时照常生效', async () => {
    const { fake } = await setup({ status: machineStatus('on', 'running', NES) });
    const screenArea = screen.getByRole('application');
    screenArea.addEventListener('keydown', (event) => {
      if (event.code === 'F8') event.preventDefault();
    });
    await press({ key: 'F8', code: 'F8' }, screenArea);
    expect(fake.calls.filter((call) => call.startsWith('runtime.pause'))).toEqual([]);
    await press({ key: 'F7', code: 'F7' }, screenArea);
    expect(fake.calls).toContain('runtime.reset {"kind":"soft"}');
  });
});

describe('消息接入与通知', () => {
  it('上电、暂停、继续、软复位、硬复位、断电、卡带弹出都写进底部面板的“消息”', async () => {
    const { container } = await setup({ status: machineStatus('off', 'paused', NES) });
    for (const [key, ctrl] of [['F9', false], ['F8', false], ['F8', false], ['F7', false], ['F7', true], ['F9', false]] as const) await press({ key, code: key, ctrlKey: ctrl });
    await openMenu('文件(F)');
    await userEvent.click(item('关闭卡带'));
    await act(async () => settle());
    expect(messageTexts(container)).toEqual(['上电', '已暂停', '继续运行', '软复位', '硬复位', '断电', '已关闭卡带 nestest.nes']);
  });

  it('装入卡带成功写一条带卡带名、mapper 号与匹配插件的消息', async () => {
    const { fake, container } = await setup();
    fake.pickedPath = NES;
    await press({ key: 'o', code: 'KeyO', ctrlKey: true });
    expect(messageTexts(container)).toEqual(['已装入卡带 nestest.nes：mapper 0，匹配插件 NROM 1.0.0']);
  });

  it('操作失败时弹出通知，同时写一条错误消息', async () => {
    const { fake, container } = await setup({ status: machineStatus('on', 'running', NES) });
    fake.failures.set('runtime.pause', { code: 'runtime.notPowered', message: '机器没有通电' });
    await press({ key: 'F8', code: 'F8' });
    expect(messageTexts(container)).toEqual(['暂停失败：机器没有通电']);
    expect((await screen.findAllByText('暂停失败：机器没有通电')).length).toBeGreaterThan(1);
  });

  it('核心被杀后重启：状态先清空再刷新，写“核心已重启”', async () => {
    const { fake, container } = await setup({ status: machineStatus('on', 'running', NES) });
    expect(screen.getAllByText('运行中').length).toBeGreaterThan(0);
    act(() => fake.emitCoreStatus({ state: 'starting' }));
    expect(screen.queryAllByText('运行中')).toHaveLength(0);
    fake.status = machineStatus('off', 'paused');
    act(() => fake.emitCoreStatus({ state: 'ready', coreVersion: '0.1.0' }));
    await act(async () => settle());
    expect(screen.getAllByText('未开机').length).toBeGreaterThan(0);
    expect(messageTexts(container)).toEqual(['核心进程已重启，机器状态已重新读取']);
  });
});

describe('常驻组件', () => {
  it('边栏折叠着、页面没有显示边栏时，菜单与状态栏的状态也是最新的', async () => {
    const { fake } = await setup({ status: machineStatus('on', 'running', NES), layout: { sideBarOpen: false } });
    expect(screen.getAllByText('运行中').length).toBeGreaterThan(0);
    fake.status = machineStatus('off', 'paused', NES);
    fake.emit('runtime.stateChanged', { power: 'off', mode: 'paused', reason: 'powerOff' });
    await act(async () => settle());
    expect(screen.getAllByText('已断电').length).toBeGreaterThan(0);
    await openMenu('机器(M)');
    expect(item('上电')).toBeTruthy();
    expect(isDisabled(item('继续'))).toBe(true);
  });

  it('状态栏：左侧电源状态、主机型号、卡带名、mapper 插件，右侧音量；没有帧率项', async () => {
    const { container } = await setup({ status: machineStatus('on', 'running', NES) });
    const bar = container.querySelector('.ui-status-bar') as HTMLElement;
    const texts = Array.from(bar.querySelectorAll('.ui-status-bar__item')).map((element) => element.textContent);
    expect(texts).toEqual(expect.arrayContaining(['运行中', '经典 FC', 'nestest.nes', 'NROM', '音量 100%']));
    expect(texts.some((text) => /fps|帧率/i.test(text ?? ''))).toBe(false);
  });
});
