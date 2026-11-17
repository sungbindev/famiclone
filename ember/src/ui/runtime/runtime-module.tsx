/*
 * ------------------------------------------------------------------------
 * 名称：runtime-module.tsx
 * 说明：运行模块在外壳里的登记项：显示区（画面）、底部面板的“运行”内容（状态栏左侧的运行按钮，图标实时显示运行状态）、命令与默认快捷键、“文件”与“机器”菜单、状态栏项（卡带、mapper 插件），以及常驻组件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：登记项都绑定同一个控制器（RuntimeController），由 bootstrap/renderer/modules/runtime.ts 创建并传入；规则见 docs/05-界面.md 第 3、4.1、5、6 节与 docs/06 第 3 节。
 *       文件菜单的“选择机型”列出四个整机机型，已选的一项前面打对勾，点一项就记住它并直接开机，下次启动沿用，之后点“开机”直接开这个机型；没有经典 FC（经典 FC 按卡带开机，打开卡带即选中）；
 *       整机机型程序里有实现且已添加 BIOS 才可选，没有 BIOS 的置灰并注明“未添加 BIOS，请在设置里添加”，暂未支持的置灰并注明“暂未支持”；
 *       “最近打开”随最近打开的清单变化，没有记录时置灰。
 *       快捷键：打开卡带 Ctrl+O，开机或断电 F9，暂停或继续 F8，软复位 F7，硬复位 Ctrl+F7。
 * ------------------------------------------------------------------------
 */

import { iconPlayerPause, iconPlayerPlay } from '@rujianjs/ui-kit';
import type { ComponentType } from 'react';
import type { ShellModule, ShellPageProps, ShellPanelDefinition, ShellSubmenuEntry } from '../shell/shell-registry';
import { machineChoices } from './machine-choices';
import type { RuntimeController } from './runtime-controller';
import { machineState, pauseToggleText, powerToggleText, stateText } from './runtime-status';
import { RuntimeHost } from './RuntimeHost';
import { RuntimePage } from './RuntimePage';
import { RuntimePanel } from './RuntimePanel';
import { CartridgeStatusItem, HostStatusItem, PluginStatusItem, type RuntimeStatusItemProps } from './RuntimeStatusItems';

/** 底部面板“运行”页签的标识。 */
const RUN_PANEL_ID = 'runtime.panel';

/** 命令的标识。 */
export const RUNTIME_COMMANDS = {
  openCartridge: 'runtime.openCartridge',
  ejectCartridge: 'runtime.ejectCartridge',
  powerToggle: 'runtime.powerToggle',
  pauseToggle: 'runtime.pauseToggle',
  softReset: 'runtime.softReset',
  hardReset: 'runtime.hardReset',
  muteToggle: 'runtime.muteToggle',
  familyKeyboardToggle: 'runtime.familyKeyboardToggle'
} as const;

/** 子菜单的标识。 */
const MACHINE_SUBMENU_ID = 'runtime.machine';
const RECENT_SUBMENU_ID = 'runtime.recent';

/** 可以拖进窗口的卡带文件扩展名。 */
const CARTRIDGE_EXTENSIONS = ['nes'];

/** 用户可见的文字。 */
const TEXT = {
  run: '运行',
  selectMachine: '选择机型',
  openCartridge: '打开卡带',
  recent: '最近打开',
  ejectCartridge: '关闭卡带',
  softReset: '软复位',
  hardReset: '硬复位',
  mute: '静音',
  familyKeyboard: '接入 Family BASIC 键盘'
} as const;

/** 把只需要控制器的组件绑定成不带属性的组件。 */
function bind(Component: ComponentType<RuntimeStatusItemProps>, controller: RuntimeController): ComponentType {
  return function Bound() {
    return <Component controller={controller} />;
  };
}

/**
 * 创建运行模块的登记项。
 * @param controller 运行模块的控制器，面板、状态栏、菜单与常驻组件共用。
 */
export function createRuntimeModule(controller: RuntimeController): ShellModule {
  /** “选择机型”的子菜单项：已选的打对勾，点一项记住并开机；程序里有实现且已添加 BIOS 才可选，否则置灰并在右侧注明原因。 */
  const machineEntries = (): readonly ShellSubmenuEntry[] => {
    const { machines, selectedMachine } = controller.snapshot();
    return machineChoices(machines).map((machine) => {
      return {
        id: machine.id,
        label: machine.label,
        checked: machine.id === selectedMachine,
        ...(machine.hint === undefined ? {} : { hint: machine.hint, disabled: true }),
        run: () => controller.selectAndBootMachine(machine.id)
      };
    });
  };
  /** “最近打开”的子菜单项：最近打开的卡带，选中即重新打开。 */
  const recentEntries = (): readonly ShellSubmenuEntry[] =>
    controller.snapshot().recent.map((item) => ({ id: String(item.id), label: item.name, run: () => controller.openRecent(item.id) }));

  return {
    name: 'runtime',
    screen: (props: ShellPageProps) => <RuntimePage {...props} controller={controller} />,
    bottomPanels: [
      {
        id: RUN_PANEL_ID,
        title: TEXT.run,
        icon: iconPlayerPlay,
        // 按钮图标实时显示运行状态：运行中是绿色三角形，暂停是暂停图标，其余（未开机、已断电）是常色三角形。
        look: (): ReturnType<NonNullable<ShellPanelDefinition['look']>> => {
          const state = machineState(controller.snapshot().status);
          const text = stateText(controller.snapshot().status);
          const hint = text === undefined ? TEXT.run : `${TEXT.run}：${text}`;
          if (state === 'running') return { icon: iconPlayerPlay, tone: 'success', hint };
          if (state === 'paused') return { icon: iconPlayerPause, hint };
          return { icon: iconPlayerPlay, hint };
        },
        autoCollapse: true,
        Component: () => <RuntimePanel controller={controller} />
      }
    ],
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
      { id: RUNTIME_COMMANDS.muteToggle, title: TEXT.mute, run: () => controller.toggleMute(), checked: () => controller.snapshot().muted },
      { id: RUNTIME_COMMANDS.familyKeyboardToggle, title: TEXT.familyKeyboard, run: () => controller.toggleFamilyKeyboard(), checked: () => controller.snapshot().familyKeyboard }
    ],
    menuItems: [
      { menu: 'file', submenu: { id: MACHINE_SUBMENU_ID, title: TEXT.selectMachine, entries: machineEntries } },
      { menu: 'file', command: RUNTIME_COMMANDS.openCartridge },
      { menu: 'file', submenu: { id: RECENT_SUBMENU_ID, title: TEXT.recent, entries: recentEntries } },
      { menu: 'file', command: RUNTIME_COMMANDS.ejectCartridge },
      { menu: 'machine', command: RUNTIME_COMMANDS.powerToggle },
      { menu: 'machine', command: RUNTIME_COMMANDS.pauseToggle },
      { menu: 'machine', command: RUNTIME_COMMANDS.softReset, separatorBefore: true },
      { menu: 'machine', command: RUNTIME_COMMANDS.hardReset },
      { menu: 'machine', command: RUNTIME_COMMANDS.muteToggle, separatorBefore: true },
      { menu: 'machine', command: RUNTIME_COMMANDS.familyKeyboardToggle }
    ],
    statusItems: [
      { id: 'runtime.host', align: 'start', Component: bind(HostStatusItem, controller) },
      { id: 'runtime.cartridge', align: 'start', Component: bind(CartridgeStatusItem, controller) },
      { id: 'runtime.plugin', align: 'start', Component: bind(PluginStatusItem, controller) }
    ],
    fileHandlers: [{ id: 'runtime.cartridge', extensions: CARTRIDGE_EXTENSIONS, open: (file) => controller.openDroppedCartridge(file.id) }],
    Host: (props: ShellPageProps) => <RuntimeHost {...props} controller={controller} />
  };
}
