/*
 * ------------------------------------------------------------------------
 * 名称：input-model.ts
 * 说明：手柄设置页面的纯逻辑：手柄的 10 个键、物理键与手柄按钮的显示文字、绑定的修改（键盘键在两个玩家之间不重复）、从浏览器读到的手柄列表、
 *       学习绑定时从手柄状态的变化里认出被按下的按钮或轴、错误说明的读取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：不依赖界面与请求，便于单独测试；所有修改都返回新的槽位数组，不改传入的数据。规则见 docs/05-界面.md 第 4.6 节。
 * ------------------------------------------------------------------------
 */

import type { PadGamepadAssignment, PadGamepadBinding, PadSlot } from '../../contracts/input/generated/input';

/** 手柄的键的名称。 */
export type PadButtonName = PadGamepadBinding['button'];

/** 用户可见的文字。 */
const TEXT = {
  failed: '操作失败。',
  unbound: '未绑定',
  numpad: (key: string) => `小键盘 ${key}`,
  leftOf: (name: string) => `左 ${name}`,
  rightOf: (name: string) => `右 ${name}`,
  button: (index: number) => `按钮 ${index}`,
  axis: (index: number, direction: string) => `轴 ${index} ${direction}`,
  negative: '负向',
  positive: '正向',
  nth: (id: string, occurrence: number) => `${id}（第 ${occurrence + 1} 个）`,
  disconnected: (name: string) => `${name}（未连接）`
} as const;

/** 手柄的 10 个键，按显示顺序排列。 */
export const PAD_BUTTONS: readonly { readonly id: PadButtonName; readonly label: string }[] = [
  { id: 'up', label: '上' },
  { id: 'down', label: '下' },
  { id: 'left', label: '左' },
  { id: 'right', label: '右' },
  { id: 'a', label: 'A' },
  { id: 'b', label: 'B' },
  { id: 'turboA', label: '连发 A' },
  { id: 'turboB', label: '连发 B' },
  { id: 'select', label: '选择' },
  { id: 'start', label: '开始' }
];

/** 轴被认为按下的阈值，与核心一致。 */
const AXIS_THRESHOLD = 0.5;

/** 核心接受的按钮与轴的编号上限。 */
const MAX_BUTTON_INDEX = 31;
const MAX_AXIS_INDEX = 15;

/** 方向键与修饰键的显示名。 */
const KEY_NAMES: Readonly<Record<string, string>> = {
  ArrowUp: '↑',
  ArrowDown: '↓',
  ArrowLeft: '←',
  ArrowRight: '→',
  Enter: '回车',
  Space: '空格',
  Backspace: '退格',
  Tab: 'Tab',
  Escape: 'Esc',
  Delete: 'Delete',
  Insert: 'Insert',
  Home: 'Home',
  End: 'End',
  PageUp: 'PageUp',
  PageDown: 'PageDown',
  CapsLock: 'CapsLock',
  Backquote: '`',
  Minus: '-',
  Equal: '=',
  BracketLeft: '[',
  BracketRight: ']',
  Backslash: '\\',
  Semicolon: ';',
  Quote: "'",
  Comma: ',',
  Period: '.',
  Slash: '/'
};

/** 浏览器手柄里本页用到的部分。 */
export interface GamepadLike {
  readonly index: number;
  readonly id: string;
  readonly connected: boolean;
  readonly buttons: ReadonlyArray<{ readonly pressed: boolean }>;
  readonly axes: readonly number[];
}

/** 系统发现的一个手柄：同一个设备标识连着多个时，occurrence 是按编号从小到大排的第几个（从 0 开始）。 */
export interface PadDevice {
  readonly id: string;
  readonly occurrence: number;
}

/** 手柄上被认出的来源（不含它绑给哪个键）。 */
export type PadSource = Pick<PadGamepadBinding, 'kind' | 'index' | 'direction'>;

/** 学习绑定时用的基准：开始学习那一刻手柄上的按钮与轴。 */
export interface PadBaseline {
  readonly buttons: readonly boolean[];
  readonly axes: readonly number[];
}

/**
 * 物理键位的显示文字。
 * @param code KeyboardEvent.code，如 KeyZ、ArrowUp。
 */
export function keyLabel(code: string): string {
  const known = KEY_NAMES[code];
  if (known !== undefined) return known;
  const letter = /^Key([A-Z])$/.exec(code);
  if (letter?.[1] !== undefined) return letter[1];
  const digit = /^Digit([0-9])$/.exec(code);
  if (digit?.[1] !== undefined) return digit[1];
  const numpad = /^Numpad(.+)$/.exec(code);
  if (numpad?.[1] !== undefined) return TEXT.numpad(numpad[1]);
  const modifier = /^(Shift|Control|Alt|Meta)(Left|Right)$/.exec(code);
  if (modifier?.[1] !== undefined) return modifier[2] === 'Left' ? TEXT.leftOf(modifier[1]) : TEXT.rightOf(modifier[1]);
  return code;
}

/** 一个键在键盘上绑定的物理键位；没有绑定时为 undefined。 */
export function keyOf(slot: PadSlot | undefined, button: PadButtonName): string | undefined {
  return slot?.keys.find((item) => item.button === button)?.code;
}

/** 替换第 slotIndex 个槽位。 */
function withSlot(slots: readonly PadSlot[], slotIndex: number, change: (slot: PadSlot) => PadSlot): PadSlot[] {
  return slots.map((slot, index) => (index === slotIndex ? change(slot) : slot));
}

/**
 * 把一个物理键绑给手柄的一个键：这个物理键原来绑着的键（两个玩家里的任何一个）先解除，这个键原来的物理键被替换。
 * @param slots 当前的槽位。
 * @param slotIndex 玩家（从 0 开始）。
 * @param button 手柄的键。
 * @param code 物理键位。
 */
export function bindKey(slots: readonly PadSlot[], slotIndex: number, button: PadButtonName, code: string): PadSlot[] {
  const freed = slots.map((slot) => ({ ...slot, keys: slot.keys.filter((item) => item.code !== code) }));
  return withSlot(freed, slotIndex, (slot) => ({ ...slot, keys: [...slot.keys.filter((item) => item.button !== button), { button, code }] }));
}

/** 解除手柄的一个键在键盘上的绑定。 */
export function clearKey(slots: readonly PadSlot[], slotIndex: number, button: PadButtonName): PadSlot[] {
  return withSlot(slots, slotIndex, (slot) => ({ ...slot, keys: slot.keys.filter((item) => item.button !== button) }));
}

/** 设备在选项里用的值：占几号与设备标识连在一起。 */
export function deviceKey(device: PadDevice): string {
  return `${device.occurrence}:${device.id}`;
}

/** 由选项值还原设备；格式不对时为 undefined。 */
export function parseDeviceKey(value: string): PadDevice | undefined {
  const match = /^(\d+):(.+)$/s.exec(value);
  if (match?.[1] === undefined || match[2] === undefined) return undefined;
  return { id: match[2], occurrence: Number(match[1]) };
}

/** 设备的显示名：同一个设备标识连着多个时标出第几个。 */
export function deviceLabel(device: PadDevice, connected: boolean): string {
  const name = device.occurrence === 0 ? device.id : TEXT.nth(device.id, device.occurrence);
  return connected ? name : TEXT.disconnected(name);
}

/**
 * 从浏览器的手柄列表得到系统发现的手柄（只含已连接的），按编号从小到大排，并算出各自是同一个设备标识里的第几个。
 * @param pads navigator.getGamepads() 的结果，没有手柄的位置为 null。
 */
export function listDevices(pads: ArrayLike<GamepadLike | null>): PadDevice[] {
  const connected: GamepadLike[] = [];
  for (let position = 0; position < pads.length; position += 1) {
    const pad = pads[position];
    if (pad !== null && pad !== undefined && pad.connected) connected.push(pad);
  }
  connected.sort((left, right) => left.index - right.index);
  const counts = new Map<string, number>();
  return connected.map((pad) => {
    const occurrence = counts.get(pad.id) ?? 0;
    counts.set(pad.id, occurrence + 1);
    return { id: pad.id, occurrence };
  });
}

/** 找出设备对应的手柄：同一个设备标识里按编号排的第 occurrence 个已连接的手柄。 */
export function findPad(pads: ArrayLike<GamepadLike | null>, device: PadDevice): GamepadLike | undefined {
  const matches: GamepadLike[] = [];
  for (let position = 0; position < pads.length; position += 1) {
    const pad = pads[position];
    if (pad !== null && pad !== undefined && pad.connected && pad.id === device.id) matches.push(pad);
  }
  matches.sort((left, right) => left.index - right.index);
  return matches[device.occurrence];
}

/** 两个设备是不是同一个。 */
function sameDevice(left: PadDevice, right: PadDevice): boolean {
  return left.id === right.id && left.occurrence === right.occurrence;
}

/**
 * 给一个玩家指定实体手柄，或取消指定（device 为 undefined）：
 * 这个手柄原来在另一个玩家名下的话先从那里取消；换成别的手柄时原来的按钮绑定清空（不同手柄的按钮排布不同），指定的还是同一个手柄时保留。
 */
export function assignDevice(slots: readonly PadSlot[], slotIndex: number, device: PadDevice | undefined): PadSlot[] {
  const released = slots.map((slot, index) => {
    if (index === slotIndex || device === undefined || slot.gamepad === undefined || !sameDevice(slot.gamepad, device)) return slot;
    return { keys: slot.keys };
  });
  return withSlot(released, slotIndex, (slot) => {
    if (device === undefined) return { keys: slot.keys };
    const keep = slot.gamepad !== undefined && sameDevice(slot.gamepad, device) ? slot.gamepad.bindings : [];
    return { keys: slot.keys, gamepad: { id: device.id, occurrence: device.occurrence, bindings: keep } };
  });
}

/** 一个玩家指定的实体手柄上，手柄的一个键的绑定。 */
export function bindingOf(assignment: PadGamepadAssignment | undefined, button: PadButtonName): PadGamepadBinding | undefined {
  return assignment?.bindings.find((item) => item.button === button);
}

/** 实体手柄上一个来源的显示文字：按钮几，或轴几的哪个方向。 */
export function sourceLabel(source: PadSource): string {
  if (source.kind === 'button') return TEXT.button(source.index);
  return TEXT.axis(source.index, source.direction === 'negative' ? TEXT.negative : TEXT.positive);
}

/** 没有绑定时显示的文字。 */
export function unboundText(): string {
  return TEXT.unbound;
}

/** 把实体手柄上的一个来源绑给手柄的一个键；这个键原来的来源被替换，没有指定实体手柄时不改。 */
export function bindGamepad(slots: readonly PadSlot[], slotIndex: number, button: PadButtonName, source: PadSource): PadSlot[] {
  return withSlot(slots, slotIndex, (slot) => {
    if (slot.gamepad === undefined) return slot;
    const binding: PadGamepadBinding = source.kind === 'axis' && source.direction !== undefined ? { button, kind: 'axis', index: source.index, direction: source.direction } : { button, kind: source.kind, index: source.index };
    return { ...slot, gamepad: { ...slot.gamepad, bindings: [...slot.gamepad.bindings.filter((item) => item.button !== button), binding] } };
  });
}

/** 解除手柄的一个键在实体手柄上的绑定。 */
export function clearGamepad(slots: readonly PadSlot[], slotIndex: number, button: PadButtonName): PadSlot[] {
  return withSlot(slots, slotIndex, (slot) => (slot.gamepad === undefined ? slot : { ...slot, gamepad: { ...slot.gamepad, bindings: slot.gamepad.bindings.filter((item) => item.button !== button) } }));
}

/** 取手柄此刻的按钮与轴，作为学习绑定的基准。 */
export function baselineOf(pad: GamepadLike): PadBaseline {
  return { buttons: pad.buttons.map((button) => button.pressed), axes: [...pad.axes] };
}

/**
 * 从手柄状态相对基准的变化里认出被按下的按钮或轴：先看按钮（从没按下变成按下），再看轴（离开基准超过 0.5 且绝对值超过 0.5，方向取符号）。
 * 编号超出核心接受范围的不认。
 * @param baseline 开始学习时的基准。
 * @param pad 手柄现在的状态。
 */
export function detectSource(baseline: PadBaseline, pad: GamepadLike): PadSource | undefined {
  for (let index = 0; index < Math.min(pad.buttons.length, MAX_BUTTON_INDEX + 1); index += 1) {
    if (pad.buttons[index]?.pressed === true && baseline.buttons[index] !== true) return { kind: 'button', index };
  }
  for (let index = 0; index < Math.min(pad.axes.length, MAX_AXIS_INDEX + 1); index += 1) {
    const value = pad.axes[index] ?? 0;
    const origin = baseline.axes[index] ?? 0;
    if (Math.abs(value) > AXIS_THRESHOLD && Math.abs(value - origin) > AXIS_THRESHOLD) return { kind: 'axis', index, direction: value < 0 ? 'negative' : 'positive' };
  }
  return undefined;
}

/**
 * 从请求失败的原因里取出给用户看的说明。
 * @param error 请求失败时抛出的值（核心的错误为 { code, message }）。
 */
export function errorText(error: unknown): string {
  if (typeof error === 'object' && error !== null && 'message' in error && typeof error.message === 'string' && error.message !== '') return error.message;
  return TEXT.failed;
}
