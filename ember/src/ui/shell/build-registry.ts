/*
 * ------------------------------------------------------------------------
 * 名称：build-registry.ts
 * 说明：汇总外壳自己与各模块的登记项并检查：模块名合格且不重复、标识（含子菜单）以“模块名.”开头且不重复、菜单项是命令或子菜单二选一、引用的命令与页面存在、
 *       默认快捷键写法合格且不与保留组合、其他命令冲突；为活动栏按钮生成“视图”菜单里的入口命令；收集启动时要打开的页面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：登记有误属于程序错误，直接抛出异常并列出全部问题；快捷键允许 Ctrl 与 Alt 同时使用（右边栏的 Ctrl+Alt+B 是文档规定的默认值），
 *       快捷键写法与 @rujianjs/ui-kit 的 shortcutFromEvent 一致：修饰键按 Ctrl、Shift、Alt 排列。
 * ------------------------------------------------------------------------
 */

import { RESERVED_SHORTCUTS } from '@rujianjs/ui-kit';
import type { ComponentType } from 'react';
import {
  SHELL_MENUS,
  type ShellActivityDefinition,
  type ShellCommandDefinition,
  type ShellMenuItemDefinition,
  type ShellModule,
  type ShellPageDefinition,
  type ShellPageProps,
  type ShellPanelDefinition,
  type ShellStatusItemDefinition
} from './shell-registry';

/** 外壳自己的登记项使用的名称。 */
export const SHELL_NAMESPACE = 'shell';
/** 活动栏按钮在“视图”菜单中的入口命令的标识前缀。 */
const ACTIVITY_COMMAND_PREFIX = `${SHELL_NAMESPACE}.showActivity.`;
/** 模块名格式。 */
const MODULE_NAME_PATTERN = /^[a-z][a-z0-9]*(-[a-z0-9]+)*$/;
/** 快捷键的写法：修饰键按 Ctrl、Shift、Alt 排列，再接一个按键。 */
const SHORTCUT_PATTERN = /^(Ctrl\+)?(Shift\+)?(Alt\+)?([A-Z0-9]|F([1-9]|1[0-9]|2[0-4])|Space|Enter|Tab|Backspace|Delete|Insert|Home|End|PageUp|PageDown|Up|Down|Left|Right|[`\-=[\]\\;',./])$/;
/** 不带 Ctrl 或 Alt 也可以用的功能键。 */
const FUNCTION_KEY_PATTERN = /^F([1-9]|1[0-9]|2[0-4])$/;
/** 菜单的 Alt+助记字母，快捷键不能占用。 */
const MENU_MNEMONIC_SHORTCUTS = SHELL_MENUS.map((menu) => `Alt+${menu.mnemonic}`);

/** 汇总后的登记表。 */
export interface ShellRegistry {
  readonly pages: ReadonlyMap<string, ShellPageDefinition>;
  readonly commands: ReadonlyMap<string, ShellCommandDefinition>;
  /** 全部菜单项：活动栏按钮生成的入口排在“视图”菜单的最前面。 */
  readonly menuItems: readonly ShellMenuItemDefinition[];
  readonly activityItems: readonly ShellActivityDefinition[];
  readonly bottomPanels: readonly ShellPanelDefinition[];
  readonly statusItems: readonly ShellStatusItemDefinition[];
  /** 默认快捷键到命令标识。 */
  readonly shortcuts: ReadonlyMap<string, string>;
  /** 常驻组件，按登记顺序。 */
  readonly hosts: readonly { readonly module: string; readonly Host: ComponentType<ShellPageProps> }[];
  /** 启动时要打开的页面标识，按登记顺序。 */
  readonly startPages: readonly string[];
}

/** 活动栏按钮对应的入口命令的标识。 */
export function activityCommandId(activityId: string): string {
  return `${ACTIVITY_COMMAND_PREFIX}${activityId}`;
}

/** 检查默认快捷键；可用时返回 null，不可用时返回原因。 */
export function checkDefaultShortcut(shortcut: string): string | null {
  if (!SHORTCUT_PATTERN.test(shortcut)) return '写法不合格，应写成 Ctrl+Shift+K 这样的形式（修饰键按 Ctrl、Shift、Alt 排列）';
  if (RESERVED_SHORTCUTS.includes(shortcut) || MENU_MNEMONIC_SHORTCUTS.includes(shortcut)) return '留给系统、文本编辑或菜单，不能使用';
  const parts = shortcut.split('+');
  if (!parts.includes('Ctrl') && !parts.includes('Alt') && !FUNCTION_KEY_PATTERN.test(parts[parts.length - 1] ?? '')) return '必须包含 Ctrl 或 Alt（F1 至 F24 除外）';
  return null;
}

/**
 * 汇总并检查登记表。
 * @param modules 全部登记项，外壳自己的排在最前面（名称为 shell），其余是渲染进程模块清单里的各模块。
 * @returns 登记表。
 * @throws Error 登记有误时，说明里列出全部问题。
 */
export function buildShellRegistry(modules: readonly ShellModule[]): ShellRegistry {
  const errors: string[] = [];
  const claimed = new Map<string, Set<string>>();
  /** 登记标识：必须以“模块名.”开头且同一种登记项里不重复。 */
  const claim = (kind: string, id: string, owner: string): boolean => {
    if (!id.startsWith(`${owner}.`) || id.length <= owner.length + 1) {
      errors.push(`${kind}“${id}”的标识必须以“${owner}.”开头`);
      return false;
    }
    const ids = claimed.get(kind) ?? new Set<string>();
    claimed.set(kind, ids);
    if (ids.has(id)) {
      errors.push(`${kind}“${id}”重复登记`);
      return false;
    }
    ids.add(id);
    return true;
  };

  const pages = new Map<string, ShellPageDefinition>();
  const commands = new Map<string, ShellCommandDefinition>();
  const menuItems: ShellMenuItemDefinition[] = [];
  const activityItems: ShellActivityDefinition[] = [];
  const bottomPanels: ShellPanelDefinition[] = [];
  const statusItems: ShellStatusItemDefinition[] = [];
  const startPages: string[] = [];
  const hosts: Array<{ module: string; Host: ComponentType<ShellPageProps> }> = [];
  /** 外壳自己的菜单项：排在各模块的菜单项之后（如“文件”菜单的“退出”在最后）。 */
  const shellMenuItems: ShellMenuItemDefinition[] = [];

  const names = new Set<string>();
  for (const module of modules) {
    const owner = module.name;
    if (!MODULE_NAME_PATTERN.test(owner) || names.has(owner)) {
      errors.push(`模块名“${owner}”不合格或重复`);
      continue;
    }
    names.add(owner);
    for (const page of module.pages ?? []) {
      if (!claim('页面', page.id, owner)) continue;
      pages.set(page.id, page);
      if (page.openOnStart === true) startPages.push(page.id);
    }
    for (const command of module.commands ?? []) if (claim('命令', command.id, owner)) commands.set(command.id, command);
    for (const item of module.activityItems ?? []) if (claim('活动栏按钮', item.id, owner)) activityItems.push(item);
    for (const item of module.bottomPanels ?? []) if (claim('底部面板页签', item.id, owner)) bottomPanels.push(item);
    for (const item of module.statusItems ?? []) if (claim('状态项', item.id, owner)) statusItems.push(item);
    const menuTarget = owner === SHELL_NAMESPACE ? shellMenuItems : menuItems;
    for (const item of module.menuItems ?? []) {
      if (item.command !== undefined && item.submenu !== undefined) {
        errors.push(`模块“${owner}”的菜单项不能同时是命令与子菜单：${item.command}`);
      } else if (item.submenu !== undefined) {
        if (claim('子菜单', item.submenu.id, owner)) menuTarget.push(item);
      } else if (item.command === undefined) {
        errors.push(`模块“${owner}”有菜单项既没有命令也没有子菜单`);
      } else if (item.command.startsWith(`${owner}.`)) {
        menuTarget.push(item);
      } else {
        errors.push(`模块“${owner}”的菜单项只能使用本模块的命令：${item.command}`);
      }
    }
    if (module.Host !== undefined) hosts.push({ module: owner, Host: module.Host });
  }

  // 活动栏按钮在“视图”菜单里的入口，排在“视图”菜单的最前面；外壳自己的菜单项排在各模块之后。
  menuItems.push(...shellMenuItems);
  const activityMenuItems: ShellMenuItemDefinition[] = [];
  for (const item of activityItems) {
    if ((item.page === undefined) === (item.sideBar === undefined)) errors.push(`活动栏按钮“${item.id}”必须且只能有页面或边栏中的一个`);
    else if (item.page !== undefined && !pages.has(item.page)) errors.push(`活动栏按钮“${item.id}”引用的页面不存在：${item.page}`);
    const id = activityCommandId(item.id);
    const page = item.page;
    commands.set(id, { id, title: item.label, run: (shell) => (page === undefined ? shell.showActivity(item.id) : shell.openPage(page)) });
    activityMenuItems.push({ menu: 'view', command: id });
  }
  menuItems.unshift(...activityMenuItems);

  for (const item of menuItems) if (item.command !== undefined && !commands.has(item.command)) errors.push(`菜单项引用的命令不存在：${item.command}`);

  const shortcuts = new Map<string, string>();
  for (const command of commands.values()) {
    if (command.defaultShortcut === undefined) continue;
    const reason = checkDefaultShortcut(command.defaultShortcut);
    if (reason !== null) errors.push(`命令“${command.id}”的默认快捷键 ${command.defaultShortcut} 不可用：${reason}`);
    const other = shortcuts.get(command.defaultShortcut);
    if (other !== undefined) errors.push(`命令“${command.id}”与“${other}”的默认快捷键都是 ${command.defaultShortcut}`);
    else shortcuts.set(command.defaultShortcut, command.id);
  }

  if (errors.length > 0) throw new Error(`外壳登记有误：\n${errors.join('\n')}`);
  return { pages, commands, menuItems, activityItems, bottomPanels, statusItems, shortcuts, hosts, startPages };
}
