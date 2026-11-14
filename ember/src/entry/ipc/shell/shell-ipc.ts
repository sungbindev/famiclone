/*
 * ------------------------------------------------------------------------
 * 名称：shell-ipc.ts
 * 说明：外壳能力的 IPC 入口：读取界面状态、保存布局修改、执行窗口动作、读取窗口状态、接收展开的面板占用的尺寸；每条请求都先检查发送方是应用自己的页面，再校验参数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：窗口动作只接受白名单里的动作名；布局修改只接受布局字段且值合格的整条修改，不合格的整条丢弃；面板尺寸同样整条校验；
 *       不可信的发送方读取状态时得到默认值，不泄露保存的内容；窗口状态的变化由组装根推送，见 forwardWindowState。
 * ------------------------------------------------------------------------
 */

import type { IpcMain, WebContents } from 'electron';
import { SHELL_CHANNELS } from '../../../contracts/shell/shell-bridge';
import type { ShellStateService } from '../../../contracts/shell/shell-state-service';
import { isWindowAction, type WindowController, type WindowState } from '../../../contracts/shell/window-control';
import { DEFAULT_SHELL_STATE, parseLayoutPatch } from '../../../shared/shell-state/shell-state';
import { parsePanelExtents } from '../../../shared/shell-state/panel-extents';
import type { SenderCheck } from '../sender-check';

/** 窗口状态在不可信发送方面前的样子。 */
const HIDDEN_WINDOW_STATE: WindowState = { maximized: false, fullScreen: false };

/** 外壳 IPC 的依赖。 */
export interface ShellIpcOptions {
  readonly ipc: IpcMain;
  readonly isTrustedSender: SenderCheck;
  readonly state: ShellStateService;
  readonly window: WindowController;
}

/** 登记外壳能力的 IPC 处理。 */
export function registerShellIpc(options: ShellIpcOptions): void {
  const { ipc, isTrustedSender, state, window } = options;
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
  ipc.handle(SHELL_CHANNELS.setPanelExtents, (event, raw: unknown) => {
    if (!isTrustedSender(event)) return;
    const extents = parsePanelExtents(raw);
    if (extents !== null) window.setPanelExtents(extents);
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
