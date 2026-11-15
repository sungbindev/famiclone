/*
 * ------------------------------------------------------------------------
 * 名称：key-capture.test.ts
 * 说明：键盘采集的测试：按下与松开转成原始事件、不上报的键、重复按下、可编辑元素、全部松开。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import type { KeyInputMessage } from '../../contracts/stream-ports/stream-messages';
import { isEditableTarget, isForwardedKey, KeyboardCapture } from './key-capture';

function setup() {
  const sent: KeyInputMessage[] = [];
  const capture = new KeyboardCapture((message) => sent.push(message));
  return { sent, capture };
}

describe('isForwardedKey', () => {
  it('普通键与修饰键本身都上报', () => {
    for (const code of ['KeyW', 'Space', 'Numpad1', 'Enter', 'ShiftRight', 'ControlLeft', 'ArrowUp']) expect(isForwardedKey(code)).toBe(true);
  });

  it('功能键、Tab、Escape 与空键位不上报', () => {
    for (const code of ['F1', 'F9', 'F24', 'Tab', 'Escape', '']) expect(isForwardedKey(code)).toBe(false);
  });

  it('带 Ctrl、Alt、Meta 的组合不上报', () => {
    expect(isForwardedKey('KeyO', { ctrlKey: true })).toBe(false);
    expect(isForwardedKey('KeyO', { altKey: true })).toBe(false);
    expect(isForwardedKey('KeyO', { metaKey: true })).toBe(false);
    expect(isForwardedKey('KeyO', { ctrlKey: false, altKey: false, metaKey: false })).toBe(true);
  });
});

describe('KeyboardCapture', () => {
  it('没有任何默认键位：任何普通键都原样上报', () => {
    const { sent, capture } = setup();
    expect(capture.keyDown('KeyQ', false)).toBe(true);
    expect(capture.keyDown('Backspace', false)).toBe(true);
    expect(sent).toEqual([
      { type: 'key', code: 'KeyQ', down: true },
      { type: 'key', code: 'Backspace', down: true }
    ]);
  });

  it('松开只上报按着的键', () => {
    const { sent, capture } = setup();
    expect(capture.keyUp('KeyQ')).toBe(false);
    capture.keyDown('KeyQ', false);
    expect(capture.keyUp('KeyQ')).toBe(true);
    expect(capture.keyUp('KeyQ')).toBe(false);
    expect(sent).toEqual([
      { type: 'key', code: 'KeyQ', down: true },
      { type: 'key', code: 'KeyQ', down: false }
    ]);
  });

  it('重复的按下不重复发送，仍算已处理', () => {
    const { sent, capture } = setup();
    capture.keyDown('KeyX', false);
    expect(capture.keyDown('KeyX', true)).toBe(true);
    expect(capture.keyDown('KeyX', true)).toBe(true);
    expect(sent).toHaveLength(1);
  });

  it('没有 repeat 标记但已经按住的键再次按下也不重复发送', () => {
    const { sent, capture } = setup();
    capture.keyDown('KeyX', false);
    capture.keyDown('KeyX', false);
    expect(sent).toHaveLength(1);
  });

  it('不上报的键返回 false 且不发送', () => {
    const { sent, capture } = setup();
    expect(capture.keyDown('F9', false)).toBe(false);
    expect(capture.keyDown('KeyO', false, undefined, { ctrlKey: true })).toBe(false);
    expect(sent).toEqual([]);
  });

  it('已经按着的键在加上 Ctrl 之后松开，仍然上报松开', () => {
    const { sent, capture } = setup();
    capture.keyDown('KeyO', false);
    capture.keyDown('KeyO', true, undefined, { ctrlKey: true });
    capture.keyUp('KeyO');
    expect(sent).toEqual([
      { type: 'key', code: 'KeyO', down: true },
      { type: 'key', code: 'KeyO', down: false }
    ]);
  });

  it('全部松开：按着的键各发一次松开，之后没有按着的键', () => {
    const { sent, capture } = setup();
    capture.keyDown('KeyA', false);
    capture.keyDown('KeyD', false);
    sent.length = 0;
    capture.releaseAll();
    expect(sent).toEqual([
      { type: 'key', code: 'KeyA', down: false },
      { type: 'key', code: 'KeyD', down: false }
    ]);
    sent.length = 0;
    capture.releaseAll();
    expect(sent).toEqual([]);
  });
});

describe('KeyboardCapture.nativeKey（键盘独占交来的物理按键）', () => {
  it('没有保留键：Esc、Tab、功能键、Ctrl、Alt、Win 键都上报，重复的按下不重复发送', () => {
    const { sent, capture } = setup();
    for (const code of ['Escape', 'Tab', 'F9', 'ControlLeft', 'AltRight', 'MetaLeft']) capture.nativeKey(code, true);
    capture.nativeKey('Escape', true);
    expect(sent.map((message) => message.code)).toEqual(['Escape', 'Tab', 'F9', 'ControlLeft', 'AltRight', 'MetaLeft']);
    expect(sent.every((message) => message.down)).toBe(true);
  });

  it('松开只上报之前按下过的键，空键位被忽略，全部松开时按着的键都松开', () => {
    const { sent, capture } = setup();
    capture.nativeKey('KeyA', false);
    capture.nativeKey('', true);
    expect(sent).toEqual([]);
    capture.nativeKey('KeyA', true);
    capture.nativeKey('KeyB', true);
    capture.nativeKey('KeyA', false);
    sent.length = 0;
    capture.releaseAll();
    expect(sent).toEqual([{ type: 'key', code: 'KeyB', down: false }]);
  });
});

describe('isEditableTarget 与采集', () => {
  it('文本输入框、多行文字框、下拉框与可编辑区域算可编辑元素', () => {
    for (const target of [{ tagName: 'INPUT', type: 'text' }, { tagName: 'input' }, { tagName: 'TEXTAREA' }, { tagName: 'SELECT' }, { tagName: 'DIV', isContentEditable: true }]) {
      expect(isEditableTarget(target)).toBe(true);
    }
    for (const target of [{ tagName: 'INPUT', type: 'button' }, { tagName: 'INPUT', type: 'checkbox' }, { tagName: 'DIV' }, null, undefined, 'x']) {
      expect(isEditableTarget(target)).toBe(false);
    }
  });

  it('焦点在可编辑元素上时不采集按下，但已按着的键仍能松开', () => {
    const { sent, capture } = setup();
    expect(capture.keyDown('KeyX', false, { tagName: 'INPUT', type: 'text' })).toBe(false);
    expect(sent).toEqual([]);
    capture.keyDown('KeyX', false);
    expect(capture.keyUp('KeyX')).toBe(true);
    expect(sent).toHaveLength(2);
  });
});
