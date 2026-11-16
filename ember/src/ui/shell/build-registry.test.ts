/*
 * ------------------------------------------------------------------------
 * 名称：build-registry.test.ts
 * 说明：登记表汇总与检查的测试：标识前缀与重复、引用的命令存在、显示区至多一个、文件处理项的扩展名、默认快捷键的写法与保留组合、快捷键冲突；以及外壳自己登记项的完整性。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { buildShellRegistry, checkDefaultShortcut } from './build-registry';
import { SHELL_COMMANDS, SHELL_MODULE } from './shell-module';
import { commandTitle, type ShellModule } from './shell-registry';

const ICON = ['M0 0'] as const;
const Screen = () => null;
const Dialog = () => null;
const HostA = () => null;
const HostB = () => null;
const open = () => undefined;

/** 一个有对话框与命令的测试模块。 */
function moduleOf(name: string, overrides: Partial<ShellModule> = {}): ShellModule {
  return {
    name,
    dialogs: [{ id: `${name}.dialog`, title: '对话框', Component: Dialog, width: 400 }],
    commands: [{ id: `${name}.go`, title: '执行', run: () => undefined }],
    ...overrides
  };
}

describe('外壳自己的登记项', () => {
  it('单独汇总没有错误，只有全屏一个默认快捷键，没有底部面板的快捷键', () => {
    const registry = buildShellRegistry([SHELL_MODULE]);
    expect(Object.fromEntries(registry.shortcuts)).toEqual({ F11: SHELL_COMMANDS.toggleFullScreen });
    expect(registry.bottomPanels).toEqual([]);
  });

  it('菜单项：文件里只有退出，设置里只有窗口始终置顶，没有视图菜单项', () => {
    const registry = buildShellRegistry([SHELL_MODULE]);
    const titles = (menu: string) => registry.menuItems.filter((item) => item.menu === menu).map((item) => registry.commands.get(item.command ?? '')?.title);
    expect(titles('file')).toEqual(['退出']);
    expect(titles('settings')).toEqual(['窗口始终置顶']);
    expect(registry.menuItems.some((item) => item.menu !== 'file' && item.menu !== 'settings')).toBe(false);
  });
});

describe('buildShellRegistry', () => {
  it('外壳自己的菜单项排在各模块之后：“文件”菜单里“退出”在最后、前面有分隔线', () => {
    const registry = buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'file', command: 'a.go' }] })]);
    const file = registry.menuItems.filter((item) => item.menu === 'file');
    expect(file.map((item) => item.command)).toEqual(['a.go', SHELL_COMMANDS.quit]);
    expect(file[1]?.separatorBefore).toBe(true);
  });

  it('菜单项可以是子菜单：标识必须以模块名开头且不重复，不能同时是命令，也不能什么都不是', () => {
    const submenu = { id: 'a.sub', title: '子菜单', entries: () => [] };
    const registry = buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'file', submenu }] })]);
    expect(registry.menuItems.find((item) => item.submenu?.id === 'a.sub')).toBeDefined();
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'file', submenu: { ...submenu, id: 'b.sub' } }] })])).toThrow(/子菜单“b.sub”的标识必须以“a.”开头/);
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'file', submenu }, { menu: 'file', submenu }] })])).toThrow(/子菜单“a.sub”重复登记/);
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'file', command: 'a.go', submenu }] })])).toThrow(/不能同时是命令与子菜单/);
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'file' }] })])).toThrow(/既没有命令也没有子菜单/);
  });

  it('收集各模块的常驻组件，按登记顺序', () => {
    const registry = buildShellRegistry([SHELL_MODULE, moduleOf('a', { Host: HostA }), moduleOf('b'), moduleOf('c', { Host: HostB })]);
    expect(registry.hosts).toEqual([
      { module: 'a', Host: HostA },
      { module: 'c', Host: HostB }
    ]);
  });

  it('命令的名称可以是函数，每次取当前的值', () => {
    let title = '开机';
    const command = { id: 'a.go', title: () => title, run: () => undefined };
    expect(commandTitle(command)).toBe('开机');
    title = '断电';
    expect(commandTitle(command)).toBe('断电');
    expect(commandTitle({ title: '静音' })).toBe('静音');
  });

  it('汇总显示区、对话框、命令、底部面板与状态项，按登记顺序排列', () => {
    const registry = buildShellRegistry([
      SHELL_MODULE,
      moduleOf('a', { screen: Screen, bottomPanels: [{ id: 'a.panel', title: '甲', icon: ICON, Component: () => null }], statusItems: [{ id: 'a.status', align: 'end', Component: () => null }] }),
      moduleOf('b', { statusItems: [{ id: 'b.status', align: 'start', Component: () => null }] })
    ]);
    expect(registry.screen).toBe(Screen);
    expect([...registry.dialogs.keys()]).toEqual(['a.dialog', 'b.dialog']);
    expect(registry.bottomPanels.map((panel) => panel.id)).toEqual(['a.panel']);
    expect(registry.statusItems.map((item) => item.id)).toEqual(['a.status', 'b.status']);
    expect(registry.commands.has('a.go')).toBe(true);
  });

  it('显示区只能有一个模块登记', () => {
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { screen: Screen }), moduleOf('b', { screen: Screen })])).toThrowError(/显示区只能有一个，“a”与“b”都登记了/);
  });

  it('标识不以“模块名.”开头、重复登记、模块名不合格或重复时报错，并一次列出全部问题', () => {
    const bad = moduleOf('a', {
      dialogs: [
        { id: 'x.dialog', title: '对话框', Component: Dialog, width: 400 },
        { id: 'a.dup', title: '对话框', Component: Dialog, width: 400 },
        { id: 'a.dup', title: '对话框', Component: Dialog, width: 400 }
      ]
    });
    expect(() => buildShellRegistry([SHELL_MODULE, bad, moduleOf('a'), moduleOf('Bad_Name')])).toThrowError(/对话框“x\.dialog”的标识必须以“a\.”开头[\s\S]*对话框“a\.dup”重复登记[\s\S]*模块名“a”不合格或重复[\s\S]*模块名“Bad_Name”不合格或重复/);
  });

  it('模块不能登记外壳名称空间里的标识', () => {
    const bad = moduleOf('a', { commands: [{ id: 'shell.quit', title: '冒充', run: () => undefined }] });
    expect(() => buildShellRegistry([SHELL_MODULE, bad])).toThrowError(/命令“shell\.quit”的标识必须以“a\.”开头/);
  });

  it('菜单项只能引用本模块的命令，引用的命令必须存在', () => {
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'file', command: SHELL_COMMANDS.quit }] })])).toThrowError(/只能使用本模块的命令/);
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'file', command: 'a.none' }] })])).toThrowError(/菜单项引用的命令不存在：a\.none/);
  });
});

describe('文件处理项', () => {
  it('汇总各模块的文件处理项，按登记顺序', () => {
    const registry = buildShellRegistry([
      SHELL_MODULE,
      moduleOf('a', { fileHandlers: [{ id: 'a.cartridge', extensions: ['nes'], open }] }),
      moduleOf('b', { fileHandlers: [{ id: 'b.image', extensions: ['img', 'ima'], open }] })
    ]);
    expect(registry.fileHandlers.map((item) => item.id)).toEqual(['a.cartridge', 'b.image']);
  });

  it('标识必须以模块名开头且不重复；扩展名必须是小写字母与数字且至少一个', () => {
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { fileHandlers: [{ id: 'b.cartridge', extensions: ['nes'], open }] })])).toThrowError(/文件处理项“b\.cartridge”的标识必须以“a\.”开头/);
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { fileHandlers: [{ id: 'a.one', extensions: ['nes'], open }, { id: 'a.one', extensions: ['img'], open }] })])).toThrowError(
      /文件处理项“a\.one”重复登记/
    );
    for (const extensions of [[], ['.nes'], ['NES'], ['n es'], ['']]) {
      expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { fileHandlers: [{ id: 'a.one', extensions, open }] })]), extensions.join()).toThrowError(/扩展名必须是小写字母与数字/);
    }
  });

  it('两个处理项认领同一个扩展名时报错', () => {
    const modules = [
      SHELL_MODULE,
      moduleOf('a', { fileHandlers: [{ id: 'a.one', extensions: ['nes', 'img'], open }] }),
      moduleOf('b', { fileHandlers: [{ id: 'b.two', extensions: ['img'], open }] })
    ];
    expect(() => buildShellRegistry(modules)).toThrowError(/“b\.two”与“a\.one”都认领了扩展名 img/);
  });
});

describe('默认快捷键', () => {
  it('两个命令用同一个默认快捷键时报冲突，包括与外壳自己的命令冲突', () => {
    const clash = moduleOf('a', { commands: [{ id: 'a.go', title: '执行', defaultShortcut: 'F11', run: () => undefined }] });
    expect(() => buildShellRegistry([SHELL_MODULE, clash])).toThrowError(/“a\.go”与“shell\.toggleFullScreen”的默认快捷键都是 F11/);
    const two = moduleOf('a', {
      commands: [
        { id: 'a.one', title: '一', defaultShortcut: 'F5', run: () => undefined },
        { id: 'a.two', title: '二', defaultShortcut: 'F5', run: () => undefined }
      ]
    });
    expect(() => buildShellRegistry([SHELL_MODULE, two])).toThrowError(/默认快捷键都是 F5/);
  });

  it('不合格的写法与保留组合被拒绝', () => {
    for (const shortcut of ['ctrl+b', 'Shift+Ctrl+B', 'Ctrl+', 'B', 'Shift+B', 'Alt+F4', 'Ctrl+A', 'Alt+F', 'Alt+H', 'F25', 'Ctrl+Ctrl+B']) {
      const module = moduleOf('a', { commands: [{ id: 'a.go', title: '执行', defaultShortcut: shortcut, run: () => undefined }] });
      expect(() => buildShellRegistry([SHELL_MODULE, module]), shortcut).toThrowError(/不可用/);
    }
  });
});

describe('checkDefaultShortcut', () => {
  it('合格的写法返回 null', () => {
    for (const shortcut of ['Ctrl+B', 'Ctrl+Alt+B', 'Ctrl+Shift+S', 'F11', 'Ctrl+F7', 'Alt+Left', 'Ctrl+,', 'Ctrl+Shift+Tab']) {
      expect(checkDefaultShortcut(shortcut), shortcut).toBeNull();
    }
  });

  it('不带 Ctrl 或 Alt 的非功能键、写法错误的组合返回原因', () => {
    expect(checkDefaultShortcut('B')).toMatch(/必须包含 Ctrl 或 Alt/);
    expect(checkDefaultShortcut('Shift+B')).toMatch(/必须包含 Ctrl 或 Alt/);
    expect(checkDefaultShortcut('Shift+F5')).toBeNull();
    expect(checkDefaultShortcut('b')).toMatch(/写法不合格/);
    expect(checkDefaultShortcut('Alt+Ctrl+B')).toMatch(/写法不合格/);
  });
});
