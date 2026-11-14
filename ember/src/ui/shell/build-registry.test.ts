/*
 * ------------------------------------------------------------------------
 * 名称：build-registry.test.ts
 * 说明：登记表汇总与检查的测试：标识前缀与重复、引用的命令与页面存在、默认快捷键的写法与保留组合、快捷键冲突、活动栏入口命令、启动页面；以及外壳自己登记项的完整性。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { activityCommandId, buildShellRegistry, checkDefaultShortcut } from './build-registry';
import { SHELL_COMMANDS, SHELL_MODULE } from './shell-module';
import { commandTitle, type ShellModule } from './shell-registry';

const ICON = ['M0 0'] as const;
const Page = () => null;
const HostA = () => null;
const HostB = () => null;

/** 一个有页面、命令、菜单项与活动栏按钮的测试模块。 */
function moduleOf(name: string, overrides: Partial<ShellModule> = {}): ShellModule {
  return {
    name,
    pages: [{ id: `${name}.page`, title: '页面', icon: ICON, Component: Page }],
    commands: [{ id: `${name}.go`, title: '执行', run: () => undefined }],
    ...overrides
  };
}

describe('外壳自己的登记项', () => {
  it('单独汇总没有错误，并带有规定的默认快捷键', () => {
    const registry = buildShellRegistry([SHELL_MODULE]);
    expect(Object.fromEntries(registry.shortcuts)).toEqual({
      'Ctrl+B': SHELL_COMMANDS.toggleSideBar,
      'Ctrl+J': SHELL_COMMANDS.toggleBottomPanel,
      'Ctrl+Alt+B': SHELL_COMMANDS.toggleRightBar,
      F11: SHELL_COMMANDS.toggleFullScreen
    });
    expect(registry.bottomPanels.map((panel) => panel.title)).toEqual(['消息']);
  });

  it('菜单项：文件里只有退出，视图里是边栏、底部面板、右边栏、全屏', () => {
    const registry = buildShellRegistry([SHELL_MODULE]);
    expect(registry.menuItems.filter((item) => item.menu === 'file').map((item) => registry.commands.get(item.command ?? '')?.title)).toEqual(['退出']);
    expect(registry.menuItems.filter((item) => item.menu === 'view').map((item) => registry.commands.get(item.command ?? '')?.title)).toEqual(['边栏', '底部面板', '右边栏', '全屏']);
    expect(registry.menuItems.some((item) => item.menu === 'machine' || item.menu === 'settings' || item.menu === 'help')).toBe(false);
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
    let title = '上电';
    const command = { id: 'a.go', title: () => title, run: () => undefined };
    expect(commandTitle(command)).toBe('上电');
    title = '断电';
    expect(commandTitle(command)).toBe('断电');
    expect(commandTitle({ title: '静音' })).toBe('静音');
  });

  it('汇总各模块的页面、命令、活动栏按钮与状态项，按登记顺序排列', () => {
    const registry = buildShellRegistry([
      SHELL_MODULE,
      moduleOf('a', { activityItems: [{ id: 'a.btn', label: 'A', icon: ICON, page: 'a.page' }], statusItems: [{ id: 'a.status', align: 'end', Component: () => null }] }),
      moduleOf('b', { statusItems: [{ id: 'b.status', align: 'start', Component: () => null }] })
    ]);
    expect([...registry.pages.keys()]).toEqual(['a.page', 'b.page']);
    expect(registry.statusItems.map((item) => item.id)).toEqual(['a.status', 'b.status']);
    expect(registry.commands.has('a.go')).toBe(true);
  });

  it('活动栏按钮在“视图”菜单里生成入口命令，排在视图菜单最前面，命令打开按钮对应的页面', () => {
    const opened: string[] = [];
    const registry = buildShellRegistry([SHELL_MODULE, moduleOf('a', { activityItems: [{ id: 'a.btn', label: '甲', icon: ICON, page: 'a.page' }] })]);
    const first = registry.menuItems.find((item) => item.menu === 'view');
    expect(first?.command).toBe(activityCommandId('a.btn'));
    const command = registry.commands.get(activityCommandId('a.btn'));
    expect(command?.title).toBe('甲');
    command?.run({ openPage: (id: string) => opened.push(id) } as never);
    expect(opened).toEqual(['a.page']);
  });

  it('只收集登记了 openOnStart 的页面作为启动页面', () => {
    const registry = buildShellRegistry([
      SHELL_MODULE,
      moduleOf('a', { pages: [{ id: 'a.page', title: 'A', icon: ICON, Component: Page, openOnStart: true }, { id: 'a.other', title: 'B', icon: ICON, Component: Page }] })
    ]);
    expect(registry.startPages).toEqual(['a.page']);
  });

  it('标识不以“模块名.”开头、重复登记、模块名不合格或重复时报错，并一次列出全部问题', () => {
    const bad = moduleOf('a', {
      pages: [
        { id: 'x.page', title: '页面', icon: ICON, Component: Page },
        { id: 'a.dup', title: '页面', icon: ICON, Component: Page },
        { id: 'a.dup', title: '页面', icon: ICON, Component: Page }
      ]
    });
    expect(() => buildShellRegistry([SHELL_MODULE, bad, moduleOf('a'), moduleOf('Bad_Name')])).toThrowError(/页面“x\.page”的标识必须以“a\.”开头[\s\S]*页面“a\.dup”重复登记[\s\S]*模块名“a”不合格或重复[\s\S]*模块名“Bad_Name”不合格或重复/);
  });

  it('模块不能登记外壳名称空间里的标识', () => {
    const bad = moduleOf('a', { commands: [{ id: 'shell.quit', title: '冒充', run: () => undefined }] });
    expect(() => buildShellRegistry([SHELL_MODULE, bad])).toThrowError(/命令“shell\.quit”的标识必须以“a\.”开头/);
  });

  it('菜单项只能引用本模块的命令，引用的命令必须存在', () => {
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'view', command: SHELL_COMMANDS.quit }] })])).toThrowError(/只能使用本模块的命令/);
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { menuItems: [{ menu: 'view', command: 'a.none' }] })])).toThrowError(/菜单项引用的命令不存在：a\.none/);
  });

  it('活动栏按钮引用的页面必须存在', () => {
    expect(() => buildShellRegistry([SHELL_MODULE, moduleOf('a', { activityItems: [{ id: 'a.btn', label: 'A', icon: ICON, page: 'a.none' }] })])).toThrowError(/引用的页面不存在：a\.none/);
  });
});

describe('默认快捷键', () => {
  it('两个命令用同一个默认快捷键时报冲突，包括与外壳自己的命令冲突', () => {
    const clash = moduleOf('a', { commands: [{ id: 'a.go', title: '执行', defaultShortcut: 'Ctrl+B', run: () => undefined }] });
    expect(() => buildShellRegistry([SHELL_MODULE, clash])).toThrowError(/“a\.go”与“shell\.toggleSideBar”的默认快捷键都是 Ctrl\+B/);
    const two = moduleOf('a', {
      commands: [
        { id: 'a.one', title: '一', defaultShortcut: 'F5', run: () => undefined },
        { id: 'a.two', title: '二', defaultShortcut: 'F5', run: () => undefined }
      ]
    });
    expect(() => buildShellRegistry([SHELL_MODULE, two])).toThrowError(/默认快捷键都是 F5/);
  });

  it('不合格的写法与保留组合被拒绝', () => {
    for (const shortcut of ['ctrl+b', 'Shift+Ctrl+B', 'Ctrl+', 'B', 'Shift+B', 'Alt+F4', 'Ctrl+A', 'Alt+F', 'Alt+V', 'F25', 'Ctrl+Ctrl+B']) {
      const module = moduleOf('a', { commands: [{ id: 'a.go', title: '执行', defaultShortcut: shortcut, run: () => undefined }] });
      expect(() => buildShellRegistry([SHELL_MODULE, module]), shortcut).toThrowError(/不可用/);
    }
  });
});

describe('checkDefaultShortcut', () => {
  it('合格的写法返回 null，包括同时带 Ctrl 与 Alt（右边栏的默认快捷键）', () => {
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
