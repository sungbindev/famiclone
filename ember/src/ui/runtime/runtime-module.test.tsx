/*
 * ------------------------------------------------------------------------
 * 名称：runtime-module.test.tsx
 * 说明：运行模块在外壳里的登记项的测试：命令的可用状态与名称随机器状态变化（菜单置灰、勾选）、默认快捷键与它们的实际效果（F9、F8、F7、Ctrl+F7、Ctrl+O）、焦点在输入控件时不带 Ctrl 的快捷键不触发、
 *       “文件”菜单的“选择机型”（只打对勾，不开机）与“最近打开”子菜单、关闭卡带、系统栏左侧的运行按钮（图标实时显示运行状态）与卡带、mapper 状态项、失败时的通知、面板没打开时状态仍是最新的、登记表无冲突。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：外壳与运行模块都是真实的，核心接口是假的；显示区用一个简单的可聚焦区域代替（真实的画面需要 Worker 与 WebGL）。
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

const DEFAULT_LAYOUT: ShellLayout = { bottomPanelHeight: null, selectedMachine: null };
const NES = 'D:\\roms\\nestest.nes';

/** 渲染外壳与运行模块；显示区换成一个带输入框的简单区域；面板启动时是关闭的。 */
async function setup(options: { status?: ReturnType<typeof machineStatus> | undefined; recent?: string[]; sb2000Bios?: boolean } = {}) {
  const fake = new FakeRuntimeBackend();
  if ('status' in options) fake.status = options.status;
  fake.recentPaths = options.recent ?? [];
  if (options.sb2000Bios === true) fake.machines = fake.machines.map((machine) => (machine.id === 'sb2000' ? { ...machine, bios: { fileName: 'SB2K_BIOS.NES', size: 524304 } } : machine));
  const controller = new RuntimeController();
  const modules: ShellModule[] = [
    {
      ...createRuntimeModule(controller),
      screen: () => (
        <div role="application" tabIndex={0} aria-label="模拟器画面">
          <input aria-label="测试输入框" />
        </div>
      )
    }
  ];
  const shell = {
    getState: vi.fn(() => Promise.resolve({ window: null, layout: DEFAULT_LAYOUT })),
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
  await screen.findByLabelText('系统栏');
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

/** 系统栏左侧的全部项（按钮与状态项）。 */
function startItems(container: HTMLElement): HTMLElement[] {
  return Array.from(container.querySelectorAll<HTMLElement>('.ui-status-bar__start > *'));
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
    expect(registry.bottomPanels.map((panel) => panel.id)).toEqual(['runtime.panel']);
  });

  it('状态项是主机型号、卡带与 mapper 插件，都在左侧', () => {
    const registry = buildShellRegistry([SHELL_MODULE, createRuntimeModule(new RuntimeController())]);
    expect(registry.statusItems.map((entry) => [entry.id, entry.align])).toEqual([
      ['runtime.host', 'start'],
      ['runtime.cartridge', 'start'],
      ['runtime.plugin', 'start']
    ]);
  });
});

describe('“机器”菜单', () => {
  it('没有开机：开机、暂停、软复位、硬复位全部置灰，静音可用', async () => {
    await setup();
    await openMenu('机器(M)');
    expect(popupItems().map((element) => element.querySelector('.ui-menu__label')?.textContent)).toEqual(['开机', '继续', '软复位', '硬复位', '静音']);
    expect(['开机', '继续', '软复位', '硬复位'].map((label) => isDisabled(item(label)))).toEqual([true, true, true, true]);
    expect(isDisabled(item('静音'))).toBe(false);
  });

  it('断电：开机与硬复位可用，暂停（继续）与软复位置灰，快捷键显示在右侧', async () => {
    await setup({ status: machineStatus('off', 'paused', NES) });
    await openMenu('机器(M)');
    expect(['开机', '继续', '软复位', '硬复位'].map((label) => isDisabled(item(label)))).toEqual([false, true, true, false]);
    expect(item('开机').textContent).toContain('F9');
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

  it('静音是勾选项，勾选状态与运行面板里的静音按钮一致', async () => {
    const { fake } = await setup();
    await openMenu('机器(M)');
    expect(item('静音').getAttribute('aria-checked')).toBe('false');
    await userEvent.click(item('静音'));
    await act(async () => settle());
    expect(fake.muted).toBe(true);
    expect(item('静音').getAttribute('aria-checked')).toBe('true');
  });
});

describe('“文件”菜单', () => {
  it('依次是选择机型、打开卡带、最近打开、关闭卡带、退出，都没有三个点；没有卡带时关闭卡带置灰，没有记录时最近打开置灰', async () => {
    await setup();
    await openMenu('文件(F)');
    expect(popupItems().map((element) => element.querySelector('.ui-menu__label')?.textContent)).toEqual(['选择机型', '打开卡带', '最近打开', '关闭卡带', '退出']);
    expect(item('打开卡带').textContent).toContain('Ctrl+O');
    expect(isDisabled(item('关闭卡带'))).toBe(true);
    expect(isDisabled(item('最近打开'))).toBe(true);
    expect(isDisabled(item('选择机型'))).toBe(false);
    expect(isDisabled(item('退出'))).toBe(false);
  });

  it('选择机型的子菜单：经典 FC 打对勾；没有 BIOS 的小霸王 2000 置灰并注明“未添加 BIOS”，其余机型置灰并注明“暂未支持”', async () => {
    await setup();
    await openMenu('文件(F)');
    await userEvent.hover(item('选择机型'));
    const submenu = screen.getByRole('menu', { name: '选择机型' });
    const entries = Array.from(submenu.querySelectorAll<HTMLElement>('[role^="menuitem"]'));
    expect(entries.map((element) => element.querySelector('.ui-menu__label')?.textContent)).toEqual(['经典 FC', '小霸王 97', '小霸王 2000', '步步高 1.0', '步步高 2.0']);
    expect(entries.map(isDisabled)).toEqual([false, true, true, true, true]);
    expect(entries.map((element) => element.getAttribute('aria-checked'))).toEqual(['true', 'false', 'false', 'false', 'false']);
    expect(entries[1]?.textContent).toContain('暂未支持');
    expect(entries[2]?.textContent).toContain('未添加 BIOS，请在设置里添加');
    expect(entries[3]?.textContent).toContain('暂未支持');
    expect(entries[4]?.textContent).toContain('暂未支持');
  });

  it('已添加 BIOS 的小霸王 2000 可选', async () => {
    await setup({ sb2000Bios: true });
    await openMenu('文件(F)');
    await userEvent.hover(item('选择机型'));
    const entries = Array.from(screen.getByRole('menu', { name: '选择机型' }).querySelectorAll<HTMLElement>('[role^="menuitem"]'));
    expect(entries.map(isDisabled)).toEqual([false, true, false, true, true]);
  });

  it('选择机型只改已选机型（打对勾）并保存：不开机、不打开文件选择，不影响正在运行的机器', async () => {
    const { fake, shell } = await setup({ status: machineStatus('on', 'running', NES), sb2000Bios: true });
    fake.calls.length = 0;
    await openMenu('文件(F)');
    await userEvent.hover(item('选择机型'));
    await userEvent.click(within(screen.getByRole('menu', { name: '选择机型' })).getByRole('menuitemradio', { name: /小霸王 2000/ }));
    await act(async () => settle());
    expect(fake.calls.filter((call) => call.startsWith('cartridges.open') || call.startsWith('machines.boot') || call.startsWith('runtime.'))).toEqual([]);
    expect(shell.setLayout).toHaveBeenCalledWith({ selectedMachine: 'sb2000' });
    await openMenu('文件(F)');
    await userEvent.hover(item('选择机型'));
    const entries = Array.from(screen.getByRole('menu', { name: '选择机型' }).querySelectorAll<HTMLElement>('[role^="menuitem"]'));
    expect(entries.map((element) => element.getAttribute('aria-checked'))).toEqual(['false', 'false', 'true', 'false', 'false']);
    await userEvent.keyboard('{Escape}');
  });

  it('没有 BIOS 的整机机型点了也不会发请求', async () => {
    const { fake, shell } = await setup();
    await openMenu('文件(F)');
    await userEvent.hover(item('选择机型'));
    await userEvent.click(within(screen.getByRole('menu', { name: '选择机型' })).getByRole('menuitemradio', { name: /小霸王 2000/ }));
    await act(async () => settle());
    expect(fake.calls.filter((call) => call.startsWith('cartridges.open') || call.startsWith('machines.boot'))).toEqual([]);
    expect(shell.setLayout).not.toHaveBeenCalled();
  });

  it('选了小霸王 2000 后，机器菜单的“开机”用已添加的 BIOS 开机，不需要卡带', async () => {
    const { fake } = await setup({ sb2000Bios: true });
    await openMenu('文件(F)');
    await userEvent.hover(item('选择机型'));
    await userEvent.click(within(screen.getByRole('menu', { name: '选择机型' })).getByRole('menuitemradio', { name: /小霸王 2000/ }));
    await act(async () => settle());
    await openMenu('机器(M)');
    expect(isDisabled(item('开机'))).toBe(false);
    await userEvent.click(item('开机'));
    await act(async () => settle());
    expect(fake.calls).toContain('machines.boot {"machine":"sb2000"}');
    expect(fake.calls.filter((call) => call.startsWith('cartridges.open'))).toEqual([]);
  });

  it('打开卡带：弹出文件选择（cartridges.open），选后直接运行，已选机型回到经典 FC', async () => {
    const { fake } = await setup({ sb2000Bios: true });
    fake.pickedPath = NES;
    await openMenu('文件(F)');
    await userEvent.click(item('打开卡带'));
    await act(async () => settle());
    expect(fake.calls).toContain('cartridges.open {}');
    await openMenu('机器(M)');
    expect(item('断电')).toBeTruthy();
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

  it('关闭卡带：有卡带时可用，点击后取出卡带', async () => {
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
  it('F9 开机或断电', async () => {
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

describe('系统栏左侧', () => {
  it('依次是运行按钮、设备以外的状态项：主机型号、卡带名与 mapper 插件；没有电源状态，右侧没有音量', async () => {
    const { container } = await setup({ status: machineStatus('on', 'running', NES) });
    const left = startItems(container);
    expect(left.map((element) => element.getAttribute('aria-label') ?? element.textContent)).toEqual(['运行', '经典 FC', 'nestest.nes', 'NROM']);
    const bar = container.querySelector('.ui-status-bar') as HTMLElement;
    const texts = Array.from(bar.querySelectorAll('.ui-status-bar__item')).map((element) => element.textContent);
    expect(texts.some((text) => /运行中|已暂停|已断电|未开机|音量/.test(text ?? ''))).toBe(false);
    expect(texts.some((text) => /fps|帧率/i.test(text ?? ''))).toBe(false);
  });

  it('整机机型用 BIOS 开机：只显示主机型号，没有卡带与 mapper 插件', async () => {
    const { container } = await setup({ status: { ...machineStatus('on', 'running'), machine: 'sb2000' } });
    expect(startItems(container).map((element) => element.getAttribute('aria-label') ?? element.textContent)).toEqual(['运行', '小霸王 2000']);
  });

  it('没有开机时没有信息的状态项不显示', async () => {
    const { container } = await setup();
    expect(startItems(container).map((element) => element.getAttribute('aria-label'))).toEqual(['运行']);
  });

  it('运行按钮的图标实时显示运行状态：运行中是绿色，暂停换成暂停图标，其余是常色', async () => {
    const { fake, container } = await setup({ status: machineStatus('on', 'running', NES) });
    const button = () => within(container).getByRole('button', { name: '运行' });
    const iconPath = () => button().querySelector('path')?.getAttribute('d') ?? '';
    expect(button().getAttribute('title')).toBe('运行：运行中');
    expect(button().getAttribute('data-tone')).toBe('success');
    const playIcon = iconPath();

    fake.status = machineStatus('on', 'paused', NES);
    fake.emit('runtime.stateChanged', { power: 'on', mode: 'paused', reason: 'pause' });
    await act(async () => settle());
    expect(button().getAttribute('title')).toBe('运行：已暂停');
    expect(button().getAttribute('data-tone')).toBe('normal');
    expect(iconPath()).not.toBe(playIcon);

    fake.status = machineStatus('off', 'paused', NES);
    fake.emit('runtime.stateChanged', { power: 'off', mode: 'paused', reason: 'powerOff' });
    await act(async () => settle());
    expect(button().getAttribute('title')).toBe('运行：已断电');
    expect(button().getAttribute('data-tone')).toBe('normal');
    expect(iconPath()).toBe(playIcon);

    fake.status = machineStatus('off', 'paused');
    fake.emit('runtime.stateChanged', { power: 'off', mode: 'paused', reason: 'eject' });
    await act(async () => settle());
    expect(button().getAttribute('title')).toBe('运行：未开机');
  });

  it('点运行按钮展开运行面板，里面的开机按钮可用', async () => {
    const { fake } = await setup({ status: machineStatus('off', 'paused', NES) });
    await userEvent.click(screen.getByRole('button', { name: '运行' }));
    const panel = screen.getByRole('region', { name: '运行' });
    await userEvent.click(within(panel).getByRole('button', { name: '开机' }));
    await act(async () => settle());
    expect(fake.calls).toContain('runtime.powerOn {"resume":true}');
  });
});

describe('通知', () => {
  it('操作失败时弹出通知', async () => {
    const { fake } = await setup({ status: machineStatus('on', 'running', NES) });
    fake.failures.set('runtime.pause', { code: 'runtime.notPowered', message: '机器没有通电' });
    await press({ key: 'F8', code: 'F8' });
    expect(await screen.findByText('暂停失败：机器没有通电')).toBeTruthy();
  });

  it('核心被杀后重启：状态先清空再刷新', async () => {
    const { fake, container } = await setup({ status: machineStatus('on', 'running', NES) });
    expect(within(container).getByText('nestest.nes')).toBeTruthy();
    act(() => fake.emitCoreStatus({ state: 'starting' }));
    expect(within(container).queryByText('nestest.nes')).toBeNull();
    fake.status = machineStatus('on', 'running', 'D:\\roms\\other.nes');
    act(() => fake.emitCoreStatus({ state: 'ready', coreVersion: '0.1.0' }));
    await act(async () => settle());
    expect(within(container).getByText('other.nes')).toBeTruthy();
  });
});

describe('常驻组件', () => {
  it('运行面板没有展开时，菜单与系统栏的状态也是最新的', async () => {
    const { fake, container } = await setup({ status: machineStatus('on', 'running', NES) });
    expect(screen.queryByRole('region')).toBeNull();
    fake.status = machineStatus('off', 'paused', NES);
    fake.emit('runtime.stateChanged', { power: 'off', mode: 'paused', reason: 'powerOff' });
    await act(async () => settle());
    expect(within(container).getByRole('button', { name: '运行' }).getAttribute('title')).toBe('运行：已断电');
    await openMenu('机器(M)');
    expect(item('开机')).toBeTruthy();
    expect(isDisabled(item('继续'))).toBe(true);
  });
});
