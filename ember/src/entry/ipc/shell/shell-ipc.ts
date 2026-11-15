/*
 * ------------------------------------------------------------------------
 * 名称：shell-ipc.ts
 * 说明：外壳能力的 IPC 入口：读取界面状态、保存布局修改、执行窗口动作、读取窗口状态、开启与关闭键盘独占、登记拖入窗口的文件；每条请求都先检查发送方是应用自己的页面，再校验参数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：窗口动作只接受白名单里的动作名；布局修改只接受布局字段且值合格的整条修改，不合格的整条丢弃；
 *       不可信的发送方读取状态时得到默认值，不泄露保存的内容；窗口状态的变化由组装根推送，见 forwardWindowState；
 *       拖入的文件：预加载脚本从拖入的文件对象取得路径后交来，这里只接受数量不超过上限的绝对路径，登记后只把编号与显示名返回给页面。
 * ------------------------------------------------------------------------
 */

import { isAbsolute } from 'node:path';
import type { IpcMain, WebContents } from 'electron';
import type { DroppedFile, DroppedFilesService } from '../../../contracts/shell/dropped-files';
import type { KeyboardCaptureService } from '../../../contracts/shell/keyboard-capture';
import { SHELL_CHANNELS } from '../../../contracts/shell/shell-bridge';
import type { ShellStateService } from '../../../contracts/shell/shell-state-service';
import { isWindowAction, type WindowController, type WindowState } from '../../../contracts/shell/window-control';
import { DEFAULT_SHELL_STATE, parseLayoutPatch } from '../../../shared/shell-state/shell-state';
import type { SenderCheck } from '../sender-check';

/** 窗口状态在不可信发送方面前的样子。 */
const HIDDEN_WINDOW_STATE: WindowState = { maximized: false, fullScreen: false };

/** 一次拖放最多登记的文件个数。 */
const MAX_DROPPED_FILES = 16;
/** Windows 路径的长度上限。 */
const MAX_PATH_LENGTH = 32768;

/** 拖入的文件路径合格（数量不超过上限、都是长度合理的绝对路径）时返回路径清单，否则返回 null。 */
function parseDroppedPaths(raw: unknown): readonly string[] | null {
  if (!Array.isArray(raw) || raw.length > MAX_DROPPED_FILES) return null;
  const paths: string[] = [];
  for (const item of raw as unknown[]) {
    if (typeof item !== 'string' || item.length === 0 || item.length > MAX_PATH_LENGTH || !isAbsolute(item)) return null;
    paths.push(item);
  }
  return paths;
}

/** 外壳 IPC 的依赖。 */
export interface ShellIpcOptions {
  readonly ipc: IpcMain;
  readonly isTrustedSender: SenderCheck;
  readonly state: ShellStateService;
  readonly window: WindowController;
  /** 键盘独占；不支持独占的平台也传入，setCapture 返回 false。 */
  readonly keyboard: KeyboardCaptureService;
  /** 拖入窗口的文件。 */
  readonly droppedFiles: DroppedFilesService;
}

/** 登记外壳能力的 IPC 处理。 */
export function registerShellIpc(options: ShellIpcOptions): void {
  const { ipc, isTrustedSender, state, window, keyboard, droppedFiles } = options;
  ipc.handle(SHELL_CHANNELS.getState, (event) => (isTrustedSender(event) ? state.get() : DEFAULT_SHELL_STATE));
  ipc.handle(SHELL_CHANNELS.getWindowState, (event) => (isTrustedSender(event) ? window.state() : HIDDEN_WINDOW_STATE));
  ipc.on(SHELL_CHANNELS.setLayout, (event, patch: unknown) => {
    if (!isTrustedSender(event)) return;
    const valid = parseLayoutPatch(patch);
    if (valid !== null) state.updateLayout(valid);
  });
  ipc.on(SHELL_CHANNELS.windowAction, (event, action: unknown) => {
    if (!isTrustedSender(event) || !isWindowAction(action)) return;
    window.perform(action);
  });
  // 键盘独占：只接受来自应用自己页面的布尔值；其余一律视为关闭。
  ipc.handle(SHELL_CHANNELS.setKeyboardCapture, (event, enabled: unknown) => {
    if (!isTrustedSender(event)) return false;
    return keyboard.setCapture(enabled === true);
  });
  // 拖入的文件：只登记合格的路径，只把编号与显示名返回给页面。
  ipc.handle(SHELL_CHANNELS.registerDroppedFiles, (event, paths: unknown): readonly DroppedFile[] => {
    if (!isTrustedSender(event)) return [];
    const valid = parseDroppedPaths(paths);
    return valid === null ? [] : droppedFiles.register(valid);
  });
}

/**
 * 把窗口状态的变化推送给页面。
 * @param contents 页面。
 * @param current 取窗口当前的状态。
 * @returns 推送函数，窗口的最大化、还原、进入与退出全屏事件触发时调用。
 */
export function forwardWindowState(contents: WebContents, current: () => WindowState): () => void {
  return () => {
    if (!contents.isDestroyed()) contents.send(SHELL_CHANNELS.windowStateChanged, current());
  };
}

/**
 * 把键盘独占期间的按键与“独占被自动关闭”推送给页面。
 * @param keyboard 键盘独占服务。
 * @param contents 页面。
 * @returns 取消转发的函数。
 */
export function forwardKeyboardToWindow(keyboard: KeyboardCaptureService, contents: WebContents): () => void {
  const stopKeys = keyboard.onKey((event) => {
    if (!contents.isDestroyed()) contents.send(SHELL_CHANNELS.nativeKey, event);
  });
  const stopReleased = keyboard.onReleased(() => {
    if (!contents.isDestroyed()) contents.send(SHELL_CHANNELS.keyboardReleased);
  });
  return () => {
    stopKeys();
    stopReleased();
  };
}
