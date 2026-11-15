/*
 * ------------------------------------------------------------------------
 * 名称：key-capture.ts
 * 说明：键盘采集：判断哪些按键要送给核心、跟踪按着的键、把按下与松开转成键盘事件消息，以及判断事件是否来自可编辑元素。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：纯逻辑，不监听任何事件，由画面组件把键盘事件交进来。这里没有键位映射，也没有默认键位：键位由卡带的 mapper 脚本决定，
 *       界面只上报“哪个物理键（KeyboardEvent.code）按下或松开了”。页面内的键盘事件（没有键盘独占时）里，F1 到 F24、Tab、Escape 与带 Ctrl、Alt、Meta 的组合键留给应用自己的快捷键与焦点移动，不上报；
 *       键盘独占开启时（Windows），主进程交来的物理按键经 nativeKey 全部上报，不做这些保留。
 * ------------------------------------------------------------------------
 */

import type { KeyInputMessage } from '../../contracts/stream-ports/stream-messages';

/** 不上报的键：Tab 与 Escape 留给焦点移动与关闭操作。 */
const RESERVED_KEYS: ReadonlySet<string> = new Set(['Tab', 'Escape']);

/** 功能键 F1 到 F24。 */
const FUNCTION_KEY = /^F\d{1,2}$/;

/** 不接收文字的 input 类型：焦点在这些控件上时不算在编辑文字。 */
const NON_TEXT_INPUT_TYPES: ReadonlySet<string> = new Set(['button', 'checkbox', 'radio', 'range', 'submit', 'reset', 'image', 'file', 'color']);

/** 判断所需的元素信息（HTMLElement 满足）。 */
interface EditableLike {
  readonly tagName?: unknown;
  readonly isContentEditable?: unknown;
  readonly type?: unknown;
}

/** 按下事件里的修饰键状态。 */
export interface KeyModifiers {
  readonly ctrlKey?: boolean;
  readonly altKey?: boolean;
  readonly metaKey?: boolean;
}

/**
 * 事件的目标是不是可编辑元素：文本输入框、多行文字框、下拉框与 contenteditable 的元素。
 * @param target 事件目标。
 */
export function isEditableTarget(target: unknown): boolean {
  if (typeof target !== 'object' || target === null) return false;
  const element = target as EditableLike;
  if (element.isContentEditable === true) return true;
  const tag = typeof element.tagName === 'string' ? element.tagName.toUpperCase() : '';
  if (tag === 'TEXTAREA' || tag === 'SELECT') return true;
  if (tag === 'INPUT') return !(typeof element.type === 'string' && NON_TEXT_INPUT_TYPES.has(element.type.toLowerCase()));
  return false;
}

/**
 * 这个按下的键要不要上报给核心。
 * @param code 物理键位（KeyboardEvent.code）。
 * @param modifiers 按下时的修饰键状态。
 */
export function isForwardedKey(code: string, modifiers: KeyModifiers = {}): boolean {
  if (code === '' || RESERVED_KEYS.has(code) || FUNCTION_KEY.test(code)) return false;
  return modifiers.ctrlKey !== true && modifiers.altKey !== true && modifiers.metaKey !== true;
}

/** 键盘采集：把按键的按下与松开转成键盘事件消息，按着的键在窗口失焦等时统一松开。 */
export class KeyboardCapture {
  private readonly held = new Set<string>();

  /** @param send 一个键按下或松开时调用。 */
  constructor(private readonly send: (message: KeyInputMessage) => void) {}

  /**
   * 处理一次按下。
   * @param code 物理键位（KeyboardEvent.code）。
   * @param repeat 是不是按住不放产生的重复事件。
   * @param target 事件目标；可编辑元素上不采集。
   * @param modifiers 修饰键状态。
   * @returns 这个键是否上报（调用方据此阻止默认行为）；重复按下也返回 true，但不重复发送。
   */
  keyDown(code: string, repeat: boolean, target?: unknown, modifiers?: KeyModifiers): boolean {
    if (isEditableTarget(target) || !isForwardedKey(code, modifiers)) return false;
    if (!repeat && !this.held.has(code)) {
      this.held.add(code);
      this.send({ type: 'key', code, down: true });
    }
    return true;
  }

  /**
   * 处理一次松开：只有之前上报过按下的键才上报松开，不管现在焦点在哪里。
   * @param code 物理键位。
   * @returns 这个键是否按着。
   */
  keyUp(code: string): boolean {
    if (!this.held.delete(code)) return false;
    this.send({ type: 'key', code, down: false });
    return true;
  }

  /**
   * 处理键盘独占交来的一次物理按键：按住不放的重复按下忽略，其余全部上报，没有保留键，也不看焦点在哪里。
   * @param code 物理键位。
   * @param down 按下为 true，松开为 false。
   */
  nativeKey(code: string, down: boolean): void {
    if (code === '') return;
    if (!down) {
      this.keyUp(code);
      return;
    }
    if (this.held.has(code)) return;
    this.held.add(code);
    this.send({ type: 'key', code, down: true });
  }

  /** 松开全部按着的键：窗口失焦、页面隐藏、卸载时调用。 */
  releaseAll(): void {
    for (const code of this.held) this.send({ type: 'key', code, down: false });
    this.held.clear();
  }
}
