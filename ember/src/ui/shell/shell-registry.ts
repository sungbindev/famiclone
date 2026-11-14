/*
 * ------------------------------------------------------------------------
 * 名称：shell-registry.ts
 * 说明：外壳的登记项类型与外壳接口：各模块在渲染进程的模块清单中登记页面（可声明边栏）、命令（含默认快捷键）、菜单项（命令或子菜单）、活动栏按钮、底部面板页签、状态项与常驻组件，
 *       外壳只渲染登记表；页面、面板与命令经外壳接口打开页面、切换区域、追加消息、通知外壳重算命令状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：登记项的标识以“模块名.”开头（页面、命令、活动栏按钮、面板、状态项）；外壳自己的登记项以 shell. 开头；规则见 docs/06-构建与模块登记.md 第 3.1 节与 docs/05-界面.md。
 * ------------------------------------------------------------------------
 */

import type { ComponentType } from 'react';
import type { IconData } from '@rujianjs/ui-kit';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';

/** 页面与边栏组件收到的属性。 */
export interface ShellPageProps {
  /** 预加载脚本暴露的接口。 */
  readonly bridge: EmberBridge;
}

/** 菜单栏的菜单，按显示顺序；没有任何菜单项的菜单不显示。 */
export const SHELL_MENUS = [
  { id: 'file', label: '文件', mnemonic: 'F' },
  { id: 'machine', label: '机器', mnemonic: 'M' },
  { id: 'view', label: '视图', mnemonic: 'V' },
  { id: 'settings', label: '设置', mnemonic: 'S' },
  { id: 'help', label: '帮助', mnemonic: 'H' }
] as const;

/** 菜单的标识。 */
export type ShellMenuId = (typeof SHELL_MENUS)[number]['id'];

/** 页面声明的边栏：页面显示时外壳把它放进边栏，切换页面时边栏随之更换。 */
export interface ShellSideBarDefinition {
  /** 边栏标题，缺省用页面标题。 */
  readonly title?: string;
  /** 边栏内容；每个页面一份，页面被关闭前一直保持挂载。 */
  readonly Component: ComponentType<ShellPageProps>;
}

/** 页面。 */
export interface ShellPageDefinition {
  /** 标识，如 runtime.run。 */
  readonly id: string;
  /** 标签上的标题。 */
  readonly title: string;
  readonly icon: IconData;
  /** 页面内容。 */
  readonly Component: ComponentType<ShellPageProps>;
  /** 单例页面再次打开时聚焦已有标签；非单例页面每次打开一个新标签。默认 true。 */
  readonly singleton?: boolean;
  /** 启动时自动打开。 */
  readonly openOnStart?: boolean;
  /** 标签上是否有关闭按钮，也决定能否被“关闭当前标签”关掉；默认 true。 */
  readonly closable?: boolean;
  /** 页面的边栏；没有时边栏自动关闭，折叠按钮置灰。 */
  readonly sideBar?: ShellSideBarDefinition;
}

/** 外壳当前的视图状态，命令用它决定菜单项是否可用、是否勾选。 */
export interface ShellViewState {
  /** 边栏是否显示出来。 */
  readonly sideBarOpen: boolean;
  /** 当前页面有没有边栏。 */
  readonly hasSideBar: boolean;
  readonly bottomPanelOpen: boolean;
  readonly rightBarOpen: boolean;
  /** 右边栏现在能不能打开。 */
  readonly canOpenRightBar: boolean;
  readonly fullScreen: boolean;
}

/** 命令：菜单项、快捷键与空工作区的入口都经命令执行动作。 */
export interface ShellCommandDefinition {
  /** 标识，如 runtime.powerToggle。 */
  readonly id: string;
  /** 菜单里显示的名称；用函数时每次渲染菜单都重新取（如“上电”与“断电”的切换）。 */
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
  readonly separatorBefore?: boolean;
  readonly run: (shell: ShellApi) => void;
}

/** 子菜单。 */
export interface ShellSubmenuDefinition {
  /** 标识，以模块名开头，如 runtime.boot；子菜单项的菜单项标识是“子菜单标识.项标识”。 */
  readonly id: string;
  /** 菜单里显示的名称。 */
  readonly title: string;
  /** 当前的子菜单项；每次渲染菜单时调用，没有项时这一项置灰。 */
  readonly entries: () => readonly ShellSubmenuEntry[];
}

/** 活动栏按钮：打开一个页面。 */
export interface ShellActivityDefinition {
  /** 标识，如 runtime.run。 */
  readonly id: string;
  /** 名称，作为悬停提示与可访问名称，也是“视图”菜单里的入口名。 */
  readonly label: string;
  readonly icon: IconData;
  /** 点击时打开的页面标识。 */
  readonly page: string;
  readonly separatorBefore?: boolean;
}

/** 底部面板的页签。 */
export interface ShellPanelDefinition {
  /** 标识，如 shell.messages。 */
  readonly id: string;
  readonly title: string;
  /** 页签标题行右侧的操作（如“清空”），只在选中本页签时显示。 */
  readonly Actions?: ComponentType;
  readonly Component: ComponentType;
}

/** 状态栏项：放在状态栏左侧或右侧，组件自己用组件库的 StatusBarItem 渲染。 */
export interface ShellStatusItemDefinition {
  /** 标识，如 runtime.power。 */
  readonly id: string;
  readonly align: 'start' | 'end';
  readonly Component: ComponentType;
}

/** 一个模块在外壳中的登记项。 */
export interface ShellModule {
  /** 模块名，与 schema 与目录名一致。 */
  readonly name: string;
  readonly pages?: readonly ShellPageDefinition[];
  readonly commands?: readonly ShellCommandDefinition[];
  readonly menuItems?: readonly ShellMenuItemDefinition[];
  readonly activityItems?: readonly ShellActivityDefinition[];
  readonly bottomPanels?: readonly ShellPanelDefinition[];
  readonly statusItems?: readonly ShellStatusItemDefinition[];
  /** 常驻组件：外壳启动后一直挂载（全屏时也在），没有界面，用来订阅事件、维护模块自己的状态；模块状态变化影响命令的可用状态时调用 refreshCommands。 */
  readonly Host?: ComponentType<ShellPageProps>;
}

/** 消息的级别。 */
export type ShellMessageLevel = 'info' | 'warning' | 'error';

/** 追加到底部面板“消息”页签的一条消息。 */
export interface ShellMessageInput {
  readonly level: ShellMessageLevel;
  readonly text: string;
  /** 发生的时间，缺省为现在。 */
  readonly time?: Date;
}

/** 外壳接口：页面、面板与命令经它调用外壳。 */
export interface ShellApi {
  /** 打开页面；单例页面已打开时聚焦它。 */
  openPage(pageId: string): void;
  /** 关闭当前标签。 */
  closeActiveTab(): void;
  /** 展开或收起边栏；当前页面没有边栏时什么都不做。 */
  toggleSideBar(): void;
  /** 展开或收起底部面板。 */
  toggleBottomPanel(): void;
  /** 展开或收起右边栏；右边栏现在不能打开时什么都不做。 */
  toggleRightBar(): void;
  /** 进入或退出全屏。 */
  toggleFullScreen(): void;
  /** 退出应用。 */
  quit(): void;
  /** 展开底部面板并切到指定页签。 */
  showBottomPanel(panelId: string): void;
  /** 往底部面板的“消息”页签追加一条消息。 */
  addMessage(message: ShellMessageInput): void;
  /** 执行命令；命令不存在或当前不可用时什么都不做。 */
  runCommand(commandId: string): void;
  /** 通知外壳重新计算菜单里命令的名称、勾选与可用状态（模块自己的状态变化后调用）。 */
  refreshCommands(): void;
}

/**
 * 命令当前显示的名称。
 * @param command 命令。
 */
export function commandTitle(command: Pick<ShellCommandDefinition, 'title'>): string {
  return typeof command.title === 'function' ? command.title() : command.title;
}
