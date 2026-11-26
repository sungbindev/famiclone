// ------------------------------------------------------------------------
// 名称：machine-session.test.ts
// 说明：测试整机在 DOS 里运行程序的输入：没有流程的机型报错且不发任何请求，
//       CEC-I 开机后输入 BRUN 与程序名，以及字符到键位的转换（字母、数字、空格、标点与 Shift）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-26
// 备注：用假的调试会话，只看发出的请求，不启动 Ember。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { keyStrokesOf, typeText, uppercaseByDefault } from '../../../src/logic/emulator/key-text';
import { enterProgram } from '../../../src/logic/emulator/machine-session';
import { createFakeClient } from '../../support/emulator-fixtures';

/** 假会话：按键与运行都成功。 */
function createClient() {
  return createFakeClient({ 'input.key': {}, 'runtime.run': { stopped: 'completed', frames: 1, cycles: 1 } });
}

/** 发出的按键请求，写成“键位+”（按下）与“键位-”（松开）。 */
function keysOf(client: ReturnType<typeof createClient>): string[] {
  return client.calls.filter((call) => call.method === 'input.key').map((call) => `${(call.params as { code: string }).code}${(call.params as { down: boolean }).down ? '+' : '-'}`);
}

describe('在 DOS 里运行程序', () => {
  it('没有进入 DOS 流程的机型报 DOS_MACHINE_UNSUPPORTED，不发任何请求', async () => {
    for (const machine of ['pec9588', 'bbg98', 'fc']) {
      const client = createClient();
      await expect(enterProgram(client, 'DEMO', machine)).rejects.toMatchObject({ code: 'DOS_MACHINE_UNSUPPORTED', hint: expect.stringContaining('emu_input') });
      expect(client.calls).toEqual([]);
    }
  });

  it('CEC-I：先运行 900 帧等 DOS 3.3 启动完，再输入 BRUN、空格、程序名与回车，不按回车跳过日期', async () => {
    const client = createClient();
    await enterProgram(client, 'Demo1', 'cec1');
    expect(client.calls[0]).toEqual({ method: 'runtime.run', params: { frames: 900, wait: true } });
    const codes = keysOf(client).filter((_, index) => index % 2 === 0).map((item) => item.slice(0, -1));
    expect(codes).toEqual(['KeyB', 'KeyR', 'KeyU', 'KeyN', 'Space', 'KeyD', 'KeyE', 'KeyM', 'KeyO', 'Digit1', 'Enter']);
  });

  it('程序名里有字母与数字以外的字符时报 DOS_KEY_UNSUPPORTED', async () => {
    await expect(enterProgram(createClient(), 'a-b', 'cec1')).rejects.toMatchObject({ code: 'DOS_KEY_UNSUPPORTED' });
  });
});

describe('文字转按键', () => {
  it('字母、数字、空格与换行；大写字母与要 Shift 的符号带 shift', () => {
    expect(keyStrokesOf('aB 1\n')).toEqual([
      { code: 'KeyA', shift: false },
      { code: 'KeyB', shift: true },
      { code: 'Space', shift: false },
      { code: 'Digit1', shift: false },
      { code: 'Enter', shift: false }
    ]);
    expect(keyStrokesOf('!_:"')).toEqual([
      { code: 'Digit1', shift: true },
      { code: 'Minus', shift: true },
      { code: 'Semicolon', shift: true },
      { code: 'Quote', shift: true }
    ]);
    expect(keyStrokesOf('a.b,c/d\t')).toEqual([
      { code: 'KeyA', shift: false },
      { code: 'Period', shift: false },
      { code: 'KeyB', shift: false },
      { code: 'Comma', shift: false },
      { code: 'KeyC', shift: false },
      { code: 'Slash', shift: false },
      { code: 'KeyD', shift: false },
      { code: 'Tab', shift: false }
    ]);
  });

  it('回车符 \\r 被忽略，不在美式键盘上的字符报错', () => {
    expect(keyStrokesOf('a\r\nb').map((stroke) => stroke.code)).toEqual(['KeyA', 'Enter', 'KeyB']);
    expect(() => keyStrokesOf('中')).toThrow(expect.objectContaining({ code: 'EMULATOR_PARAM_INVALID', message: expect.stringContaining('中') }));
  });

  it('输入文字：每个字符点一次键，需要 Shift 的先按下 Shift、点完再松开，每个键运行指定的帧数', async () => {
    const client = createClient();
    expect(await typeText(client, 'A!', 3, 2)).toBe(2);
    expect(keysOf(client)).toEqual(['ShiftLeft+', 'KeyA+', 'KeyA-', 'ShiftLeft-', 'ShiftLeft+', 'Digit1+', 'Digit1-', 'ShiftLeft-']);
    expect(client.calls.filter((call) => call.method === 'runtime.run').map((call) => (call.params as { frames: number }).frames)).toEqual([3, 2, 3, 2]);
  });

  it('默认大写的机型：金字塔与 CEC-I 大写不按 Shift，小写才按 Shift，符号不变', () => {
    expect(uppercaseByDefault('cec1')).toBe(true);
    expect(uppercaseByDefault('pec9588')).toBe(true);
    expect(uppercaseByDefault('pec586')).toBe(true);
    expect(uppercaseByDefault('sb2000')).toBe(false);
    expect(uppercaseByDefault(undefined)).toBe(false);
    expect(keyStrokesOf('CATALOG', true).every((stroke) => !stroke.shift)).toBe(true);
    expect(keyStrokesOf('aB!', true)).toEqual([
      { code: 'KeyA', shift: true },
      { code: 'KeyB', shift: false },
      { code: 'Digit1', shift: true }
    ]);
  });

  it('默认大写的机型输入文字：小写字母按 Shift，大写直接点键', async () => {
    const client = createClient();
    expect(await typeText(client, 'Ab', 3, 2, true)).toBe(2);
    expect(keysOf(client)).toEqual(['KeyA+', 'KeyA-', 'ShiftLeft+', 'KeyB+', 'KeyB-', 'ShiftLeft-']);
  });

  it('有不支持的字符时一个键都不按', async () => {
    const client = createClient();
    await expect(typeText(client, 'ab中', 8, 8)).rejects.toMatchObject({ code: 'EMULATOR_PARAM_INVALID' });
    expect(client.calls).toEqual([]);
  });
});
