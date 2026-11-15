/*
 * ------------------------------------------------------------------------
 * 名称：shell-registry.ts
 * 说明：外壳的登记项类型与外壳接口：各模块在渲染进程的模块清单中登记显示区、对话框、命令（含默认快捷键）、菜单项（命令或子菜单）、底部面板页签、状态项与常驻组件，
 *       外壳只渲染登记表；对话框、面板与命令经外壳接口打开对话框、展开面板、通知外壳重算命令状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：登记项的标识以“模块名.”开头（对话框、命令、面板、状态项）；外壳自己的登记项以 shell. 开头；规则见 docs/06-构建与模块登记.md 第 3.1 节与 docs/05-界面.md。
 *       显示区只有一个（中间固定显示，没有标签页），其他界面都是对话框；每个底部面板页签在系统栏左侧有一个按钮。
 * ------------------------------------------------------------------------
 */

import type { ComponentType } from 'react';
import type { IconData, NotifyFunction } from '@rujianjs/ui-kit';
import type { DroppedFile } from '../../contracts/shell/dropped-files';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';

/** 显示区、对话框内容组件收到的属性。 */
export interface ShellPageProps {
  /** 预加载脚本暴露的接口。 */
  readonly bridge: EmberBridge;
}

/** 菜单栏的菜单，按显示顺序；没有任何菜单项的菜单不显示。 */
export const SHELL_MENUS = [
  { id: 'file', label: '文件', mnemonic: 'F' },
  { id: 'machine', label: '机器', mnemonic: 'M' },
  { id: 'settings', label: '设置', mnemonic: 'S' },
  { id: 'help', label: '帮助', mnemonic: 'H' }
] as const;

/** 菜单的标识。 */
export type ShellMenuId = (typeof SHELL_MENUS)[number]['id'];

/** 对话框：独立的系统窗口，同一个对话框同时只开一个，再次打开时聚焦已有的。 */
export interface ShellDialogDefinition {
  /** 标识，如 preferences.preferences。 */
  readonly id: string;
  /** 对话框标题。 */
  readonly title: string;
  /** 对话框内容。 */
  readonly Component: ComponentType<ShellPageProps>;
  /** 内容区初始宽度（像素）。 */
  readonly width: number;
  /** 内容区初始高度（像素）；不写时按内容。 */
  readonly height?: number;
  /** 最小宽度与最小高度（像素）。 */
  readonly minWidth?: number;
  readonly minHeight?: number;
}

/** 外壳当前的视图状态，命令用它决定菜单项是否可用、是否勾选。 */
export interface ShellViewState {
  readonly bottomPanelOpen: boolean;
  /** 底部面板里当前的页签；面板关闭时为 null。 */
  readonly activePanelId: string | null;
  readonly fullScreen: boolean;
}

/** 命令：菜单项与快捷键都经命令执行动作。 */
export interface ShellCommandDefinition {
  /** 标识，如 runtime.powerToggle。 */
  readonly id: string;
  /** 菜单里显示的名称；用函数时每次渲染菜单都重新取（如“开机”与“断电”的切换）。 */
  readonly title: string | (() => string);
  /** 默认快捷键，如 Ctrl+Shift+K；全部命令之间不能重复。 */
  readonly defaultShortcut?: string;
  readonly run: (shell: ShellApi) => void;
  /** 当前是否可用，缺省总是可用。 */
  readonly enabled?: (view: ShellViewState) => boolean;
  /** 菜单项是否勾选，缺省不是勾选项。 */
  readonly checked?: (view: ShellViewState) => boolean;
}

/** 菜单项：放在某个菜单里的一条命令，或一个子菜单（二选一）。 */
export interface ShellMenuItemDefinition {
  readonly menu: ShellMenuId;
  /** 命令标识；与 submenu 二选一。 */
  readonly command?: string;
  /** 子菜单；与 command 二选一。 */
  readonly submenu?: ShellSubmenuDefinition;
  readonly mnemonic?: string;
  readonly separatorBefore?: boolean;
}

/** 子菜单里的一项：选中时执行 run，不登记成命令（没有快捷键，项目随数据变化，如最近打开的卡带）。 */
export interface ShellSubmenuEntry {
  /** 在子菜单里唯一的标识。 */
  readonly id: string;
  readonly label: string;
  /** 右侧的提示文字，如“暂未支持”。 */
  readonly hint?: string;
  readonly disabled?: boolean;
  /** 给出时是单选项：选中的一项前面打对勾，选中只执行 run，菜单关闭。 */
  readonly checked?: boolean;
  readonly separatorBefore?: boolean;
  readonly run: (shell: ShellApi) => void;
}

/** 子菜单。 */
export interface ShellSubmenuDefinition {
  /** 标识，以模块名开头，如 runtime.machine；子菜单项的菜单项标识是“子菜单标识.项标识”。 */
  readonly id: string;
  /** 菜单里显示的名称。 */
  readonly title: string;
  /** 当前的子菜单项；每次渲染菜单时调用，没有项时这一项置灰。 */
  readonly entries: () => readonly ShellSubmenuEntry[];
}

/** 底部面板：同时在系统栏左侧有一个只有图标的按钮，点击展开或收起面板并切到本面板的内容。 */
export interface ShellPanelDefinition {
  /** 标识，如 runtime.panel。 */
  readonly id: string;
  /** 面板名称，也是状态栏按钮的悬停提示与可访问名称。 */
  readonly title: string;
  /** 状态栏按钮上的图标。 */
  readonly icon: IconData;
  /** 按钮当前的外观：图标、颜色与悬停提示随模块状态变化（如运行按钮显示运行或暂停）；每次渲染状态栏时调用，不给时用 icon 与 title；状态变化后模块调用 refreshCommands 让外壳重新渲染。 */
  readonly look?: () => { readonly icon: IconData; readonly tone?: 'normal' | 'success' | 'warning' | 'error'; readonly hint?: string };
  /** 面板内容；面板收起时卸载，展开时重新挂载。 */
  readonly Component: ComponentType;
  /** 为 true 时面板标题栏有“自动收缩”开关（开关状态由外壳保存），开关开着时点面板之外的地方就收起面板。 */
  readonly autoCollapse?: boolean;
  /** 状态栏按钮上的指示灯（如软驱读写灯）：每次渲染状态栏时调用，返回 undefined 表示不显示；inIcon 给出时灯画在图标里（灯心在图标内的位置，百分比）；状态变化后模块调用 refreshCommands 让外壳重新渲染。 */
  readonly lamp?: () => { readonly lit: boolean; readonly label: string; readonly inIcon?: { readonly x: number; readonly y: number } } | undefined;
}

/** 状态栏项：放在系统栏左侧或右侧，组件自己用组件库的 StatusBarItem 渲染；左侧的状态项排在面板按钮之后，右侧最后一项是通知按钮。 */
export interface ShellStatusItemDefinition {
  /** 标识，如 runtime.power。 */
  readonly id: string;
  readonly align: 'start' | 'end';
  readonly Component: ComponentType;
}

/** 拖入窗口的文件的处理：按扩展名认领文件，打开的办法由模块自己决定（经主进程请求的 droppedId）。 */
export interface ShellFileHandlerDefinition {
  /** 标识，如 runtime.cartridge。 */
  readonly id: string;
  /** 认领的扩展名，小写、不带点；全部处理项之间不能重复。 */
  readonly extensions: readonly string[];
  /** 打开拖入的文件；失败由模块自己用通知提示。 */
  readonly open: (file: DroppedFile) => void;
}

/** 一个模块在外壳中的登记项。 */
export interface ShellModule {
  /** 模块名，与 schema 与目录名一致。 */
  readonly name: string;
  /** 显示区的内容：整个应用只能有一个模块登记。 */
  readonly screen?: ComponentType<ShellPageProps>;
  readonly dialogs?: readonly ShellDialogDefinition[];
  readonly commands?: readonly ShellCommandDefinition[];
  readonly menuItems?: readonly ShellMenuItemDefinition[];
  readonly bottomPanels?: readonly ShellPanelDefinition[];
  readonly statusItems?: readonly ShellStatusItemDefinition[];
  /** 拖入窗口的文件的处理项：外壳接住窗口上的文件拖放，按扩展名交给认领它的处理项。 */
  readonly fileHandlers?: readonly ShellFileHandlerDefinition[];
  /** 常驻组件：外壳启动后一直挂载（全屏时也在），没有界面，用来订阅事件、维护模块自己的状态；模块状态变化影响命令的可用状态时调用 refreshCommands。 */
  readonly Host?: ComponentType<ShellPageProps>;
}

/** 外壳接口：显示区、面板与命令经它调用外壳。 */
export interface ShellApi {
  /** 打开对话框；已打开时聚焦它。 */
  openDialog(dialogId: string): void;
  /** 点击面板按钮：面板关着就展开并切到该页签，开着而且就是该页签就收起，开着但在别的页签就切过去。 */
  togglePanel(panelId: string): void;
  /** 进入或退出全屏。 */
  toggleFullScreen(): void;
  /** 退出应用。 */
  quit(): void;
  /** 执行命令；命令不存在或当前不可用时什么都不做。 */
  runCommand(commandId: string): void;
  /** 通知外壳重新计算菜单里命令的名称、勾选与可用状态，并重画状态栏（模块自己的状态变化后调用）。 */
  refreshCommands(): void;
  /** 弹出通知；对话框内容渲染在组件库的对话框层，取不到组件库的 useNotify，要发通知就用它。 */
  notify: NotifyFunction;
}

/**
 * 命令当前显示的名称。
 * @param command 命令。
 */
export function commandTitle(command: Pick<ShellCommandDefinition, 'title'>): string {
  return typeof command.title === 'function' ? command.title() : command.title;
}
