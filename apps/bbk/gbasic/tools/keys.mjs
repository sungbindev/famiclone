// ------------------------------------------------------------------------
// 名称：keys.mjs
// 说明：给调试版模拟器输入按键：按 DirectInput 扫描码按下、松开，并把文字转成按键序列。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：步步高键盘默认开着大写锁定，字母不按 Shift 就是大写；符号按美式键盘的位置换算。
// ------------------------------------------------------------------------

/** 命名按键的扫描码 */
export const KEY = {
  escape: 0x01, backspace: 0x0e, tab: 0x0f, enter: 0x1c, ctrl: 0x1d, shift: 0x2a, space: 0x39,
  f1: 0x3b, f2: 0x3c, f3: 0x3d, f4: 0x3e, f5: 0x3f, f6: 0x40, f7: 0x41, f8: 0x42, f9: 0x43, f10: 0x44,
  home: 0xc7, up: 0xc8, pageUp: 0xc9, left: 0xcb, right: 0xcd, end: 0xcf, down: 0xd0, pageDown: 0xd1, insert: 0xd2, delete: 0xd3,
};

/** 不用 Shift 的字符 */
const PLAIN = { ' ': 0x39, '\n': 0x1c, '-': 0x0c, '=': 0x0d, ',': 0x33, '.': 0x34, '/': 0x35, ';': 0x27, "'": 0x28, '\\': 0x2b };

/** 要按住 Shift 的字符 */
const SHIFTED = { '"': 0x28, '+': 0x0d, '*': 0x09, '(': 0x0a, ')': 0x0b, '?': 0x35, '<': 0x33, '>': 0x34, ':': 0x27, '_': 0x0c, '!': 0x02, '$': 0x05, '%': 0x06, '&': 0x08 };

'qwertyuiop'.split('').forEach((c, i) => { PLAIN[c] = 0x10 + i; });
'asdfghjkl'.split('').forEach((c, i) => { PLAIN[c] = 0x1e + i; });
'zxcvbnm'.split('').forEach((c, i) => { PLAIN[c] = 0x2c + i; });
'1234567890'.split('').forEach((c, i) => { PLAIN[c] = 0x02 + i; });

/** 每次按下与松开之间运行的帧数 */
const HOLD_FRAMES = 4;

/**
 * 按一下某个键，可同时按住 Shift 或 Ctrl。
 * @param {{send: (command: string) => Promise<unknown>}} session 调试会话
 * @param {number} code 扫描码
 * @param {{shift?: boolean, ctrl?: boolean}} [modifiers] 同时按住的修饰键
 * @returns {Promise<void>}
 */
export async function press(session, code, { shift = false, ctrl = false } = {}) {
  const hex = (value) => value.toString(16);
  if (shift) await session.send(`key ${hex(KEY.shift)} 1`);
  if (ctrl) await session.send(`key ${hex(KEY.ctrl)} 1`);
  await session.send(`key ${hex(code)} 1`);
  await session.send(`run ${HOLD_FRAMES}`);
  await session.send(`key ${hex(code)} 0`);
  if (ctrl) await session.send(`key ${hex(KEY.ctrl)} 0`);
  if (shift) await session.send(`key ${hex(KEY.shift)} 0`);
  await session.send(`run ${HOLD_FRAMES}`);
}

/**
 * 把文字转成按键输入，\n 是回车；字母一律输入大写。
 * @param {{send: (command: string) => Promise<unknown>}} session 调试会话
 * @param {string} text 文字
 * @returns {Promise<void>}
 * @throws {Error} 有不支持的字符
 */
export async function typeText(session, text) {
  for (const raw of text) {
    const lower = raw.toLowerCase();
    if (PLAIN[lower] !== undefined && /[a-z]/.test(lower)) await press(session, PLAIN[lower]);
    else if (PLAIN[raw] !== undefined) await press(session, PLAIN[raw]);
    else if (SHIFTED[raw] !== undefined) await press(session, SHIFTED[raw], { shift: true });
    else throw new Error(`不支持的字符：${raw}`);
  }
}
