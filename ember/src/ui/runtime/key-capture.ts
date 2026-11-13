/*
 * ------------------------------------------------------------------------
 * 名称：key-capture.ts
 * 说明：键盘到手柄按键的采集逻辑：默认按键映射表、按下与松开的状态跟踪（过滤重复按下）、全部松开，以及判断事件是否来自可编辑元素。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：纯逻辑，不监听任何事件，由画面组件把键盘事件交进来；映射表是数据，可以整体替换（首选项里改映射以后再做）。
 *       只在按键掩码变化时才发送，所以重复的 keydown 与重复的 keyup 都不会产生消息。按键位定义见 src/contracts/stream-ports/stream-messages.ts。
 * ------------------------------------------------------------------------
 */

/** 手柄按键的位：位 0 为 A，位 1 为 B，位 2 为选择，位 3 为开始，位 4 到 7 依次为上、下、左、右。 */
export const PAD_BUTTON = {
  a: 0x01,
  b: 0x02,
  select: 0x04,
  start: 0x08,
  up: 0x10,
  down: 0x20,
  left: 0x40,
  right: 0x80
} as const;

/** 一个键对应的手柄口与按键位。 */
export interface KeyBinding {
  /** 手柄口，0 或 1。 */
  readonly port: number;
  /** 按键位，取 PAD_BUTTON 的值。 */
  readonly button: number;
}

/** 键位映射：KeyboardEvent.code（物理键位）到手柄按键。 */
export type KeyMap = Readonly<Record<string, KeyBinding>>;

/** 默认映射：方向键为十字键，Z 为 B，X 为 A，Enter 为开始，右 Shift 为选择；手柄 2 暂不映射。 */
export const DEFAULT_KEY_MAP: KeyMap = {
  ArrowUp: { port: 0, button: PAD_BUTTON.up },
  ArrowDown: { port: 0, button: PAD_BUTTON.down },
  ArrowLeft: { port: 0, button: PAD_BUTTON.left },
  ArrowRight: { port: 0, button: PAD_BUTTON.right },
  KeyZ: { port: 0, button: PAD_BUTTON.b },
  KeyX: { port: 0, button: PAD_BUTTON.a },
  Enter: { port: 0, button: PAD_BUTTON.start },
  ShiftRight: { port: 0, button: PAD_BUTTON.select }
};

/** 手柄口的个数。 */
const PAD_PORT_COUNT = 2;

/** 不接收文字的 input 类型：焦点在这些控件上时不算在编辑文字。 */
const NON_TEXT_INPUT_TYPES: ReadonlySet<string> = new Set(['button', 'checkbox', 'radio', 'range', 'submit', 'reset', 'image', 'file', 'color']);

/** 判断所需的元素信息（HTMLElement 满足）。 */
interface EditableLike {
  readonly tagName?: unknown;
  readonly isContentEditable?: unknown;
  readonly type?: unknown;
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

/** 键盘采集：把按键的按下与松开转成各手柄口的整套按键掩码。 */
export class KeyboardCapture {
  private readonly held = new Set<string>();
  private readonly masks: number[] = Array.from({ length: PAD_PORT_COUNT }, () => 0);

  /**
   * @param send 某个手柄口的按键掩码变化时调用，参数是手柄口与该口当前的整个掩码。
   * @param keyMap 键位映射，默认用 DEFAULT_KEY_MAP。
   */
  constructor(
    private readonly send: (port: number, buttons: number) => void,
    private keyMap: KeyMap = DEFAULT_KEY_MAP
  ) {}

  /** 换用新的键位映射；先松开全部按键，避免旧映射下按住的键留在核心里。 */
  setKeyMap(keyMap: KeyMap): void {
    this.releaseAll();
    this.keyMap = keyMap;
  }

  /**
   * 处理一次按下。
   * @param code 物理键位（KeyboardEvent.code）。
   * @param repeat 是不是按住不放产生的重复事件。
   * @param target 事件目标；可编辑元素上不采集。
   * @returns 这个键是否在映射里并被采集（调用方据此阻止默认行为）；重复按下也返回 true。
   */
  keyDown(code: string, repeat: boolean, target?: unknown): boolean {
    if (isEditableTarget(target) || !this.isMapped(code)) return false;
    if (!repeat && !this.held.has(code)) {
      this.held.add(code);
      this.publish();
    }
    return true;
  }

  /**
   * 处理一次松开。
   * @param code 物理键位。
   * @param target 事件目标；可编辑元素上不采集。
   * @returns 这个键是否在映射里。
   */
  keyUp(code: string, target?: unknown): boolean {
    if (isEditableTarget(target) || !this.isMapped(code)) return false;
    if (this.held.delete(code)) this.publish();
    return true;
  }

  /** 松开全部按键：窗口失焦、页面隐藏、卸载时调用。 */
  releaseAll(): void {
    if (this.held.size === 0) return;
    this.held.clear();
    this.publish();
  }

  /** 这个键是不是映射里的键。 */
  private isMapped(code: string): boolean {
    return Object.hasOwn(this.keyMap, code);
  }

  /** 按住的键重新算出各口的掩码，变化的口发出消息。 */
  private publish(): void {
    const next = this.masks.map(() => 0);
    for (const code of this.held) {
      const binding = this.keyMap[code];
      if (binding !== undefined && binding.port >= 0 && binding.port < PAD_PORT_COUNT) next[binding.port] = (next[binding.port] ?? 0) | binding.button;
    }
    next.forEach((mask, port) => {
      if (mask === this.masks[port]) return;
      this.masks[port] = mask;
      this.send(port, mask);
    });
  }
}
