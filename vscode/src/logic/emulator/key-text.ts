// ------------------------------------------------------------------------
// 名称：key-text.ts
// 说明：把一段文字转成物理键位的按键序列，并依次输入到模拟器；
//       字符与键位的对应按美式键盘布局，与机型无关。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-26
// 备注：键位取 KeyboardEvent.code，由 Ember 里各机型的键盘模块换成各自的键码或扫描码；大写字母与
//       需要 Shift 的符号靠先按下 ShiftLeft 实现，机器是否区分大小写由机型的键盘决定。
// ------------------------------------------------------------------------

import type { DebugSession } from '../../contracts/debug-client/debug-session';
import { invalidParam } from './params';
import { tapKey } from './machine-session';

/** 一次按键：键位，以及按下时是否要同时按住 Shift。 */
export interface KeyStroke {
  readonly code: string;
  readonly shift: boolean;
}

/** 用来按住的 Shift 键位。 */
const SHIFT_CODE = 'ShiftLeft';

/** 不用 Shift 的符号字符对应的键位。 */
const PLAIN_SYMBOLS: Readonly<Record<string, string>> = {
  ' ': 'Space', '\n': 'Enter', '\t': 'Tab', '-': 'Minus', '=': 'Equal', '[': 'BracketLeft', ']': 'BracketRight', '\\': 'Backslash',
  ';': 'Semicolon', "'": 'Quote', ',': 'Comma', '.': 'Period', '/': 'Slash', '`': 'Backquote'
};

/** 要 Shift 的符号字符对应的键位。 */
const SHIFTED_SYMBOLS: Readonly<Record<string, string>> = {
  '!': 'Digit1', '@': 'Digit2', '#': 'Digit3', $: 'Digit4', '%': 'Digit5', '^': 'Digit6', '&': 'Digit7', '*': 'Digit8', '(': 'Digit9', ')': 'Digit0',
  _: 'Minus', '+': 'Equal', '{': 'BracketLeft', '}': 'BracketRight', '|': 'Backslash', ':': 'Semicolon', '"': 'Quote', '<': 'Comma', '>': 'Period',
  '?': 'Slash', '~': 'Backquote'
};

/**
 * 一个字符对应的按键。
 * @param char 一个字符。
 * @returns 按键；没有对应的键位时为 undefined。
 */
function keyStrokeOf(char: string): KeyStroke | undefined {
  if (/^[a-z]$/.test(char)) return { code: `Key${char.toUpperCase()}`, shift: false };
  if (/^[A-Z]$/.test(char)) return { code: `Key${char}`, shift: true };
  if (/^\d$/.test(char)) return { code: `Digit${char}`, shift: false };
  const plain = PLAIN_SYMBOLS[char];
  if (plain !== undefined) return { code: plain, shift: false };
  const shifted = SHIFTED_SYMBOLS[char];
  return shifted === undefined ? undefined : { code: shifted, shift: true };
}

/**
 * 把文字转成按键序列；回车符 \r 忽略，换行 \n 是 Enter。
 * @param text 要输入的文字。
 * @returns 按键序列。
 * @throws FcdevError 有无法输入的字符（不在美式键盘上的字符），参数错误。
 */
export function keyStrokesOf(text: string): KeyStroke[] {
  const strokes: KeyStroke[] = [];
  for (const char of text) {
    if (char === '\r') continue;
    const stroke = keyStrokeOf(char);
    if (stroke === undefined) invalidParam(`无法输入字符“${char}”`, '只支持美式键盘上的字母、数字、空格、标点、换行与制表符；其他按键用 code 单独输入');
    strokes.push(stroke);
  }
  return strokes;
}

/**
 * 依次输入一段文字：每个字符点一次键，需要 Shift 的先按下 Shift、点完再松开。
 * @param client 已连接的调试会话。
 * @param text 要输入的文字。
 * @param downFrames 每个键按下后运行的帧数。
 * @param upFrames 每个键松开后运行的帧数。
 * @returns 输入的按键数。
 * @throws FcdevError 有无法输入的字符；此时一个键都没有按。
 */
export async function typeText(client: DebugSession, text: string, downFrames: number, upFrames: number): Promise<number> {
  const strokes = keyStrokesOf(text);
  for (const { code, shift } of strokes) {
    if (!shift) {
      await tapKey(client, code, downFrames, upFrames);
      continue;
    }
    await client.request('input.key', { code: SHIFT_CODE, down: true });
    try {
      await tapKey(client, code, downFrames, upFrames);
    } finally {
      await client.request('input.key', { code: SHIFT_CODE, down: false });
    }
  }
  return strokes.length;
}
