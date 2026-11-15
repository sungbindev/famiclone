/*
 * ------------------------------------------------------------------------
 * 名称：native-keyboard-capture.ts
 * 说明：键盘独占的 Windows 实现：加载原生模块 ember_keyboard.node，开启后把原始输入的扫描码转成键位名交给订阅者；窗口失去焦点、隐藏、最小化或关闭时自动关闭独占。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：独占生效期间，应用里所有窗口（含对话框）都收不到键盘消息，所以只在主窗口处于前台、界面要求时才开启；
 *       不是 Windows、文件不存在或加载失败时 setCapture 返回 false，界面继续使用页面内的键盘事件；
 *       原生模块的 enable、disable 必须在主进程的 JavaScript 线程调用，本类的方法也只在主进程调用。
 * ------------------------------------------------------------------------
 */

import { scanCodeToKeyCode } from './scan-code-table';

/** 原生模块交来的一次按键。 */
export interface RawKeyEvent {
  readonly make: number;
  readonly e0: boolean;
  readonly e1: boolean;
  readonly up: boolean;
}

/** 原生模块（ember_keyboard.node）的接口。 */
export interface NativeKeyboardModule {
  /** 开启独占，返回 0 表示成功，否则为 Windows 错误码。 */
  enable(windowHandle: Buffer, callback: (event: RawKeyEvent) => void): number;
  /** 关闭独占。 */
  disable(): void;
}

/** 独占需要用到的窗口能力（BrowserWindow 的子集）。 */
export interface CaptureWindow {
  getNativeWindowHandle(): Buffer;
  isDestroyed(): boolean;
  on(event: CaptureWindowEvent, listener: () => void): unknown;
  removeListener(event: CaptureWindowEvent, listener: () => void): unknown;
}

/** 会让独占自动关闭的窗口事件。 */
export type CaptureWindowEvent = 'blur' | 'hide' | 'minimize' | 'closed';

/** 会让独占自动关闭的窗口事件，全部要订阅。 */
const RELEASE_EVENTS: readonly CaptureWindowEvent[] = ['blur', 'hide', 'minimize', 'closed'];

/** 创建键盘独占的依赖。 */
export interface NativeKeyboardCaptureOptions {
  /** 加载原生模块；不支持或加载失败时返回 undefined，失败原因交给 onError。 */
  readonly load: () => NativeKeyboardModule | undefined;
  /** 取主窗口；没有窗口时返回 undefined。 */
  readonly getWindow: () => CaptureWindow | undefined;
  /** 开启失败（Windows 错误码）时的记录。 */
  readonly onError: (message: string) => void;
}

/** 独占一次按键的订阅者。 */
type KeyListener = (event: { readonly code: string; readonly down: boolean }) => void;

/** 键盘独占的 Windows 实现。 */
export class NativeKeyboardCapture {
  private readonly options: NativeKeyboardCaptureOptions;
  private native: NativeKeyboardModule | undefined;
  private loaded = false;
  private active: { readonly window: CaptureWindow; readonly release: () => void } | undefined;
  private readonly keyListeners = new Set<KeyListener>();
  private readonly releasedListeners = new Set<() => void>();

  constructor(options: NativeKeyboardCaptureOptions) {
    this.options = options;
  }

  /** 开启或关闭独占，返回调用后独占是否生效。 */
  setCapture(enabled: boolean): boolean {
    if (!enabled) {
      this.stop();
      return false;
    }
    if (this.active !== undefined) return true;
    const native = this.nativeModule();
    const window = this.options.getWindow();
    if (native === undefined || window === undefined || window.isDestroyed()) return false;

    const error = native.enable(window.getNativeWindowHandle(), (event) => this.handleKey(event));
    if (error !== 0) {
      this.options.onError(`开启键盘独占失败，Windows 错误码 ${error}`);
      return false;
    }
    const onWindowEvent = (): void => {
      this.stop();
      for (const listener of this.releasedListeners) listener();
    };
    for (const event of RELEASE_EVENTS) window.on(event, onWindowEvent);
    this.active = {
      window,
      release: () => {
        for (const event of RELEASE_EVENTS) window.removeListener(event, onWindowEvent);
      }
    };
    return true;
  }

  /** 订阅独占期间的按键。 */
  onKey(listener: KeyListener): () => void {
    this.keyListeners.add(listener);
    return () => {
      this.keyListeners.delete(listener);
    };
  }

  /** 订阅独占被自动关闭。 */
  onReleased(listener: () => void): () => void {
    this.releasedListeners.add(listener);
    return () => {
      this.releasedListeners.delete(listener);
    };
  }

  /** 关闭独占（退出应用前调用）。 */
  dispose(): void {
    this.stop();
  }

  /** 只加载一次原生模块。 */
  private nativeModule(): NativeKeyboardModule | undefined {
    if (!this.loaded) {
      this.native = this.options.load();
      this.loaded = true;
    }
    return this.native;
  }

  /** 关闭独占并摘掉窗口事件的订阅；没有开启时什么也不做。 */
  private stop(): void {
    const active = this.active;
    if (active === undefined) return;
    this.active = undefined;
    active.release();
    this.native?.disable();
  }

  /** 把一次原始按键转成键位名，交给订阅者。 */
  private handleKey(event: RawKeyEvent): void {
    const code = scanCodeToKeyCode(event.make, event.e0, event.e1);
    if (code === undefined) return;
    for (const listener of this.keyListeners) listener({ code, down: !event.up });
  }
}

/**
 * 加载原生模块。
 * @param path 原生模块文件的路径。
 * @param platform 当前平台；只有 win32 才加载。
 * @param onError 加载失败的记录。
 * @returns 原生模块；不支持或失败时为 undefined。
 */
export function loadNativeKeyboard(path: string, platform: string, onError: (message: string) => void): NativeKeyboardModule | undefined {
  if (platform !== 'win32') return undefined;
  try {
    const holder = { exports: {} as unknown } as NodeJS.Module;
    process.dlopen(holder, path);
    return holder.exports as NativeKeyboardModule;
  } catch (error) {
    onError(`加载键盘独占模块失败：${error instanceof Error ? error.message : String(error)}`);
    return undefined;
  }
}
