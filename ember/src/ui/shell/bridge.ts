/*
 * ------------------------------------------------------------------------
 * 名称：bridge.ts
 * 说明：取预加载脚本暴露的 window.ember；界面只经它与主进程通信。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import type { EmberBridge } from '../../contracts/shell/shell-bridge';

declare global {
  interface Window {
    /** 预加载脚本暴露的接口。 */
    readonly ember: EmberBridge;
  }
}

/** 预加载脚本暴露的接口。 */
export function getBridge(): EmberBridge {
  return window.ember;
}
