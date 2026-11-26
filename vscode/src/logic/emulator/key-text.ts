// ------------------------------------------------------------------------
// 名称：key-text.ts
// 说明：把一段文字转成物理键位的按键序列，并依次输入到模拟器；字符与键位的对应按美式键盘布局，
//       字母的大小写按机型的键盘默认状态处理。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-26
// 备注：键位取 KeyboardEvent.code，由 Ember 里各机型的键盘模块换成各自的键码或扫描码；需要 Shift
//       的符号靠先按下 ShiftLeft 实现；多数机型的键盘不按 Shift 出小写、按 Shift 出大写，金字塔
//       与中华学习机 CEC-I 的键盘上电默认大写、按 Shift 才出小写，字母的 Shift 要反过来。
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

/** 键盘上电默认大写、按 Shift 才出小写的机型：金字塔两个型号与中华学习机 CEC-I。 */
const UPPERCASE_DEFAULT_MACHINES: ReadonlySet<string> = new Set(['pec9588', 'pec586', 'cec1']);

/**
 * 机型的键盘是不是上电默认大写。
 * @param machine 机型标识（runtime.status 的 machine）；没有机型时为 undefined。
 * @returns 默认大写时为 true。
 */
export function isUppercaseDefault(machine: string | undefined): boolean {
  return machine !== undefined && UPPERCASE_DEFAULT_MACHINES.has(machine);
}

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
 * @param uppercaseDefault 键盘是否上电默认大写：是则大写字母不按 Shift、小写字母按 Shift。
 * @returns 按键；没有对应的键位时为 undefined。
 */
function keyStrokeOf(char: string, uppercaseDefault: boolean): KeyStroke | undefined {
  if (/^[a-z]$/.test(char)) return { code: `Key${char.toUpperCase()}`, shift: uppercaseDefault };
  if (/^[A-Z]$/.test(char)) return { code: `Key${char}`, shift: !uppercaseDefault };
  if (/^\d$/.test(char)) return { code: `Digit${char}`, shift: false };
  const plain = PLAIN_SYMBOLS[char];
  if (plain !== undefined) return { code: plain, shift: false };
  const shifted = SHIFTED_SYMBOLS[char];
  return shifted === undefined ? undefined : { code: shifted, shift: true };
}

/**
 * 把文字转成按键序列；回车符 \r 忽略，换行 \n 是 Enter。
 * @param text 要输入的文字。
 * @param uppercaseDefault 键盘是否上电默认大写，默认否。
 * @returns 按键序列。
 * @throws FcdevError 有无法输入的字符（不在美式键盘上的字符），参数错误。
 */
export function keyStrokesOf(text: string, uppercaseDefault = false): KeyStroke[] {
  const strokes: KeyStroke[] = [];
  for (const char of text) {
    if (char === '\r') continue;
    const stroke = keyStrokeOf(char, uppercaseDefault);
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
 * @param uppercaseDefault 键盘是否上电默认大写，默认否。
 * @returns 输入的按键数。
 * @throws FcdevError 有无法输入的字符；此时一个键都没有按。
 */
export async function typeText(client: DebugSession, text: string, downFrames: number, upFrames: number, uppercaseDefault = false): Promise<number> {
  const strokes = keyStrokesOf(text, uppercaseDefault);
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
