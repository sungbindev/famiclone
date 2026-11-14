/*
 * ------------------------------------------------------------------------
 * 名称：runtime-module.tsx
 * 说明：运行模块在外壳里的登记项：“运行”页（含边栏）、活动栏按钮、命令与默认快捷键、“文件”与“机器”菜单、状态栏项，以及常驻组件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：登记项都绑定同一个控制器（RuntimeController），由 bootstrap/renderer/modules/runtime.ts 创建并传入；规则见 docs/05-界面.md 第 3、4.1、5、6 节与 docs/06 第 3 节。
 *       文件菜单的“开机…”列出经典 FC 与四个整机机型：只有经典 FC 可用（选后等同“打开卡带…”），整机机型置灰并注明“暂未支持”；“最近打开”随最近打开的清单变化，没有记录时置灰。
 *       快捷键：打开卡带 Ctrl+O，上电或断电 F9，暂停或继续 F8，软复位 F7，硬复位 Ctrl+F7。
 * ------------------------------------------------------------------------
 */

import { iconPlayerPlay } from '@rujianjs/ui-kit';
import type { ComponentType } from 'react';
import type { ShellModule, ShellPageProps, ShellSubmenuEntry } from '../shell/shell-registry';
import type { RuntimeController } from './runtime-controller';
import { pauseToggleText, powerToggleText } from './runtime-status';
import { RuntimeHost } from './RuntimeHost';
import { RuntimePage } from './RuntimePage';
import { RuntimeSideBar } from './RuntimeSideBar';
import { CartridgeStatusItem, HostStatusItem, PluginStatusItem, PowerStatusItem, VolumeStatusItem, type RuntimeStatusItemProps } from './RuntimeStatusItems';

/** “运行”页的标识。 */
const RUN_PAGE_ID = 'runtime.run';

/** 命令的标识。 */
export const RUNTIME_COMMANDS = {
  openCartridge: 'runtime.openCartridge',
  ejectCartridge: 'runtime.ejectCartridge',
  powerToggle: 'runtime.powerToggle',
  pauseToggle: 'runtime.pauseToggle',
  softReset: 'runtime.softReset',
  hardReset: 'runtime.hardReset',
  muteToggle: 'runtime.muteToggle'
} as const;

/** 子菜单的标识。 */
const BOOT_SUBMENU_ID = 'runtime.boot';
const RECENT_SUBMENU_ID = 'runtime.recent';

/** 用户可见的文字。 */
const TEXT = {
  run: '运行',
  boot: '开机…',
  openCartridge: '打开卡带…',
  recent: '最近打开',
  ejectCartridge: '关闭卡带',
  softReset: '软复位',
  hardReset: '硬复位',
  mute: '静音',
  unsupported: '暂未支持'
} as const;

/** “开机…”里的机型：经典 FC 可用，整机机型暂未支持。 */
const BOOT_MACHINES: ReadonlyArray<{ readonly id: string; readonly label: string; readonly supported: boolean }> = [
  { id: 'fc', label: '经典 FC', supported: true },
  { id: 'sb97', label: '小霸王 97', supported: false },
  { id: 'sb2000', label: '小霸王 2000', supported: false },
  { id: 'bbk10', label: '步步高 1.0', supported: false },
  { id: 'bbk20', label: '步步高 2.0', supported: false }
];

/** 把只需要控制器的组件绑定成不带属性的组件。 */
function bind(Component: ComponentType<RuntimeStatusItemProps>, controller: RuntimeController): ComponentType {
  return function Bound() {
    return <Component controller={controller} />;
  };
}

/**
 * 创建运行模块的登记项。
 * @param controller 运行页的控制器，边栏、状态栏、菜单与常驻组件共用。
 */
export function createRuntimeModule(controller: RuntimeController): ShellModule {
  /** “开机…”的子菜单项。 */
  const bootEntries = (): readonly ShellSubmenuEntry[] =>
    BOOT_MACHINES.map((machine, index) => ({
      id: machine.id,
      label: machine.label,
      ...(machine.supported ? {} : { hint: TEXT.unsupported, disabled: true }),
      ...(index === 1 ? { separatorBefore: true } : {}),
      run: () => {
        if (machine.supported) controller.openCartridge();
      }
    }));
  /** “最近打开”的子菜单项：最近打开的卡带，选中即重新打开。 */
  const recentEntries = (): readonly ShellSubmenuEntry[] =>
    controller.snapshot().recent.map((item) => ({ id: String(item.id), label: item.name, run: () => controller.openRecent(item.id) }));

  return {
    name: 'runtime',
    pages: [
      {
        id: RUN_PAGE_ID,
        title: TEXT.run,
        icon: iconPlayerPlay,
        Component: (props: ShellPageProps) => <RuntimePage {...props} controller={controller} />,
        openOnStart: true,
        sideBar: { Component: () => <RuntimeSideBar controller={controller} /> }
      }
    ],
    activityItems: [{ id: RUN_PAGE_ID, label: TEXT.run, icon: iconPlayerPlay, page: RUN_PAGE_ID }],
    commands: [
      { id: RUNTIME_COMMANDS.openCartridge, title: TEXT.openCartridge, defaultShortcut: 'Ctrl+O', run: () => controller.openCartridge() },
      { id: RUNTIME_COMMANDS.ejectCartridge, title: TEXT.ejectCartridge, run: () => controller.ejectCartridge(), enabled: () => controller.availability().eject },
      {
        id: RUNTIME_COMMANDS.powerToggle,
        title: () => powerToggleText(controller.snapshot().status),
        defaultShortcut: 'F9',
        run: () => controller.togglePower(),
        enabled: () => controller.availability().powerToggle
      },
      {
        id: RUNTIME_COMMANDS.pauseToggle,
        title: () => pauseToggleText(controller.snapshot().status),
        defaultShortcut: 'F8',
        run: () => controller.togglePause(),
        enabled: () => controller.availability().pauseToggle
      },
      { id: RUNTIME_COMMANDS.softReset, title: TEXT.softReset, defaultShortcut: 'F7', run: () => controller.softReset(), enabled: () => controller.availability().softReset },
      { id: RUNTIME_COMMANDS.hardReset, title: TEXT.hardReset, defaultShortcut: 'Ctrl+F7', run: () => controller.hardReset(), enabled: () => controller.availability().hardReset },
      { id: RUNTIME_COMMANDS.muteToggle, title: TEXT.mute, run: () => controller.toggleMute(), checked: () => controller.snapshot().muted }
    ],
    menuItems: [
      { menu: 'file', submenu: { id: BOOT_SUBMENU_ID, title: TEXT.boot, entries: bootEntries } },
      { menu: 'file', command: RUNTIME_COMMANDS.openCartridge },
      { menu: 'file', submenu: { id: RECENT_SUBMENU_ID, title: TEXT.recent, entries: recentEntries } },
      { menu: 'file', command: RUNTIME_COMMANDS.ejectCartridge },
      { menu: 'machine', command: RUNTIME_COMMANDS.powerToggle },
      { menu: 'machine', command: RUNTIME_COMMANDS.pauseToggle },
      { menu: 'machine', command: RUNTIME_COMMANDS.softReset, separatorBefore: true },
      { menu: 'machine', command: RUNTIME_COMMANDS.hardReset },
      { menu: 'machine', command: RUNTIME_COMMANDS.muteToggle, separatorBefore: true }
    ],
    statusItems: [
      { id: 'runtime.power', align: 'start', Component: bind(PowerStatusItem, controller) },
      { id: 'runtime.host', align: 'start', Component: bind(HostStatusItem, controller) },
      { id: 'runtime.cartridge', align: 'start', Component: bind(CartridgeStatusItem, controller) },
      { id: 'runtime.plugin', align: 'start', Component: bind(PluginStatusItem, controller) },
      { id: 'runtime.volume', align: 'end', Component: bind(VolumeStatusItem, controller) }
    ],
    Host: (props: ShellPageProps) => <RuntimeHost {...props} controller={controller} />
  };
}
