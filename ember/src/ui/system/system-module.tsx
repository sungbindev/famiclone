/*
 * ------------------------------------------------------------------------
 * 名称：system-module.tsx
 * 说明：系统模块在外壳里的登记项：“帮助”菜单的“帮助文档”“打开日志目录”“关于 Ember”、“关于 Ember”对话框，以及取得预加载接口的常驻组件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：命令只拿到外壳接口，拿不到预加载脚本的接口，所以由常驻组件把接口交给模块；对话框是独立的系统窗口，同一时间只开一个；规则见 docs/05-界面.md 第 3、5、8 节。
 * ------------------------------------------------------------------------
 */

import { useEffect } from 'react';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { ShellModule, ShellPageProps } from '../shell/shell-registry';
import { AboutDialog, type ThirdPartyNotice } from './AboutDialog';

/** “关于 Ember”对话框的标识。 */
const DIALOG_ID = 'system.about';

/** 命令的标识。 */
export const SYSTEM_COMMANDS = { openHelp: 'system.openHelp', openLogDirectory: 'system.openLogDirectory', openAbout: 'system.openAbout' } as const;

/** 对话框内容区的初始尺寸与最小尺寸（像素）。 */
const DIALOG_SIZE = { width: 520, height: 560, minWidth: 400, minHeight: 320 } as const;

/** 用户可见的文字。 */
const TEXT = {
  about: '关于 Ember',
  openHelp: '帮助文档',
  helpFailed: '无法打开帮助文档',
  openLogDirectory: '打开日志目录',
  failed: '无法打开日志目录'
} as const;

/** 错误里给用户看的说明；错误没有说明时用 fallback。 */
function errorText(error: unknown, fallback: string): string {
  if (typeof error === 'object' && error !== null && 'message' in error && typeof error.message === 'string' && error.message !== '') return error.message;
  return fallback;
}

/**
 * 创建系统模块的登记项。
 * @param notices 第三方依赖清单，由构建脚本生成。
 * @param buildTime 编译时间，由构建脚本生成。
 */
export function createSystemModule(notices: readonly ThirdPartyNotice[], buildTime: string): ShellModule {
  /** 常驻组件交给模块的预加载接口；没有挂载时为空。 */
  const holder: { bridge?: EmberBridge } = {};
  return {
    name: 'system',
    dialogs: [{ id: DIALOG_ID, title: TEXT.about, Component: (props: ShellPageProps) => <AboutDialog {...props} notices={notices} buildTime={buildTime} />, ...DIALOG_SIZE }],
    commands: [
      {
        id: SYSTEM_COMMANDS.openHelp,
        title: TEXT.openHelp,
        run: (shell) => {
          holder.bridge?.main.system.openHelp({}).catch((error: unknown) => shell.notify(errorText(error, TEXT.helpFailed), 'error'));
        }
      },
      {
        id: SYSTEM_COMMANDS.openLogDirectory,
        title: TEXT.openLogDirectory,
        run: (shell) => {
          holder.bridge?.main.system.openLogDirectory({}).catch((error: unknown) => shell.notify(errorText(error, TEXT.failed), 'error'));
        }
      },
      { id: SYSTEM_COMMANDS.openAbout, title: TEXT.about, run: (shell) => shell.openDialog(DIALOG_ID) }
    ],
    menuItems: [
      { menu: 'help', command: SYSTEM_COMMANDS.openHelp },
      { menu: 'help', command: SYSTEM_COMMANDS.openLogDirectory },
      { menu: 'help', command: SYSTEM_COMMANDS.openAbout, separatorBefore: true }
    ],
    Host: function SystemHost({ bridge }: ShellPageProps) {
      useEffect(() => {
        holder.bridge = bridge;
        return () => {
          if (holder.bridge === bridge) delete holder.bridge;
        };
      }, [bridge]);
      return null;
    }
  };
}
