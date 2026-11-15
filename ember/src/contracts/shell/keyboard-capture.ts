/*
 * ------------------------------------------------------------------------
 * 名称：keyboard-capture.ts
 * 说明：键盘独占的契约：主进程提供的“独占键盘”能力，开启后应用里的窗口收不到任何键盘消息，按键只以物理键位（KeyboardEvent.code 的写法）交给界面。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：当前只有 Windows 有实现；没有实现的平台（或原生模块加载失败）setCapture 返回 false，界面继续用页面内的键盘事件。
 *       开启期间窗口失去焦点、隐藏、最小化或关闭时，实现会自动关闭独占，并通过 onReleased 通知界面。
 * ------------------------------------------------------------------------
 */

/** 一次物理按键：键位用 KeyboardEvent.code 的写法（如 "KeyA"、"Escape"、"ControlLeft"）。 */
export interface NativeKeyEvent {
  readonly code: string;
  readonly down: boolean;
}

/** 键盘独占服务。 */
export interface KeyboardCaptureService {
  /**
   * 开启或关闭键盘独占。
   * @param enabled 是否独占。
   * @returns 调用后独占是否生效；不支持独占或开启失败时为 false。
   */
  setCapture(enabled: boolean): boolean;
  /** 订阅独占期间的按键；返回取消订阅的函数。 */
  onKey(listener: (event: NativeKeyEvent) => void): () => void;
  /** 订阅“独占被自动关闭”（窗口失去焦点、隐藏、最小化或关闭）；返回取消订阅的函数。 */
  onReleased(listener: () => void): () => void;
}
