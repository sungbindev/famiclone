/*
 * ------------------------------------------------------------------------
 * 名称：scan-code-table.ts
 * 说明：Windows 原始输入的扫描码（第 1 套扫描码）到 KeyboardEvent.code 键位名的对照。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：键位名与学习机键盘模块使用的名字一致（物理键位，与键盘布局无关）；
 *       数字键盘区关闭 NumLock 时系统会额外发出“假 Shift”（E0 2A、E0 36），它们不是真实按键，对照表里不收录；
 *       Pause 键由 E1 前缀的序列表示，不收录，其余没有收录的扫描码返回 undefined 由调用方忽略。
 * ------------------------------------------------------------------------
 */

/** 无前缀的扫描码。 */
const PLAIN_CODES: Readonly<Record<number, string>> = {
  0x01: 'Escape',
  0x02: 'Digit1',
  0x03: 'Digit2',
  0x04: 'Digit3',
  0x05: 'Digit4',
  0x06: 'Digit5',
  0x07: 'Digit6',
  0x08: 'Digit7',
  0x09: 'Digit8',
  0x0a: 'Digit9',
  0x0b: 'Digit0',
  0x0c: 'Minus',
  0x0d: 'Equal',
  0x0e: 'Backspace',
  0x0f: 'Tab',
  0x10: 'KeyQ',
  0x11: 'KeyW',
  0x12: 'KeyE',
  0x13: 'KeyR',
  0x14: 'KeyT',
  0x15: 'KeyY',
  0x16: 'KeyU',
  0x17: 'KeyI',
  0x18: 'KeyO',
  0x19: 'KeyP',
  0x1a: 'BracketLeft',
  0x1b: 'BracketRight',
  0x1c: 'Enter',
  0x1d: 'ControlLeft',
  0x1e: 'KeyA',
  0x1f: 'KeyS',
  0x20: 'KeyD',
  0x21: 'KeyF',
  0x22: 'KeyG',
  0x23: 'KeyH',
  0x24: 'KeyJ',
  0x25: 'KeyK',
  0x26: 'KeyL',
  0x27: 'Semicolon',
  0x28: 'Quote',
  0x29: 'Backquote',
  0x2a: 'ShiftLeft',
  0x2b: 'Backslash',
  0x2c: 'KeyZ',
  0x2d: 'KeyX',
  0x2e: 'KeyC',
  0x2f: 'KeyV',
  0x30: 'KeyB',
  0x31: 'KeyN',
  0x32: 'KeyM',
  0x33: 'Comma',
  0x34: 'Period',
  0x35: 'Slash',
  0x36: 'ShiftRight',
  0x37: 'NumpadMultiply',
  0x38: 'AltLeft',
  0x39: 'Space',
  0x3a: 'CapsLock',
  0x3b: 'F1',
  0x3c: 'F2',
  0x3d: 'F3',
  0x3e: 'F4',
  0x3f: 'F5',
  0x40: 'F6',
  0x41: 'F7',
  0x42: 'F8',
  0x43: 'F9',
  0x44: 'F10',
  0x45: 'NumLock',
  0x46: 'ScrollLock',
  0x47: 'Numpad7',
  0x48: 'Numpad8',
  0x49: 'Numpad9',
  0x4a: 'NumpadSubtract',
  0x4b: 'Numpad4',
  0x4c: 'Numpad5',
  0x4d: 'Numpad6',
  0x4e: 'NumpadAdd',
  0x4f: 'Numpad1',
  0x50: 'Numpad2',
  0x51: 'Numpad3',
  0x52: 'Numpad0',
  0x53: 'NumpadDecimal',
  0x56: 'IntlBackslash',
  0x57: 'F11',
  0x58: 'F12'
};

/** E0 前缀的扫描码（扩展键）。 */
const EXTENDED_CODES: Readonly<Record<number, string>> = {
  0x1c: 'NumpadEnter',
  0x1d: 'ControlRight',
  0x35: 'NumpadDivide',
  0x37: 'PrintScreen',
  0x38: 'AltRight',
  0x47: 'Home',
  0x48: 'ArrowUp',
  0x49: 'PageUp',
  0x4b: 'ArrowLeft',
  0x4d: 'ArrowRight',
  0x4f: 'End',
  0x50: 'ArrowDown',
  0x51: 'PageDown',
  0x52: 'Insert',
  0x53: 'Delete',
  0x5b: 'MetaLeft',
  0x5c: 'MetaRight',
  0x5d: 'ContextMenu'
};

/**
 * 扫描码转键位名。
 * @param make 扫描码。
 * @param extended 是否带 E0 前缀。
 * @param prefixE1 是否带 E1 前缀（Pause 键）；带 E1 的一律忽略。
 * @returns 键位名；假 Shift、Pause 与未收录的扫描码返回 undefined。
 */
export function scanCodeToKeyCode(make: number, extended: boolean, prefixE1: boolean): string | undefined {
  if (prefixE1) return undefined;
  return extended ? EXTENDED_CODES[make] : PLAIN_CODES[make];
}
