/*
 * ------------------------------------------------------------------------
 * 名称：key-capture.test.ts
 * 说明：键盘采集的测试：默认映射、按键掩码的合成、重复按下的过滤、全部松开、可编辑元素不采集、映射表替换。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { DEFAULT_KEY_MAP, isEditableTarget, KeyboardCapture, PAD_BUTTON } from './key-capture';

/** 建一个记录发送内容的采集器。 */
function setup(keyMap = DEFAULT_KEY_MAP) {
  const sent: Array<[number, number]> = [];
  const capture = new KeyboardCapture((port, buttons) => sent.push([port, buttons]), keyMap);
  return { capture, sent };
}

describe('默认映射', () => {
  it('方向键为十字键，Z 为 B，X 为 A，Enter 为开始，右 Shift 为选择', () => {
    const expected: Array<[string, number]> = [
      ['ArrowUp', 0x10],
      ['ArrowDown', 0x20],
      ['ArrowLeft', 0x40],
      ['ArrowRight', 0x80],
      ['KeyZ', 0x02],
      ['KeyX', 0x01],
      ['Enter', 0x08],
      ['ShiftRight', 0x04]
    ];
    for (const [code, mask] of expected) {
      const { capture, sent } = setup();
      expect(capture.keyDown(code, false)).toBe(true);
      expect(sent).toEqual([[0, mask]]);
    }
  });

  it('只映射手柄 1，左 Shift 等其他键不采集', () => {
    expect(Object.values(DEFAULT_KEY_MAP).every((binding) => binding.port === 0)).toBe(true);
    const { capture, sent } = setup();
    expect(capture.keyDown('ShiftLeft', false)).toBe(false);
    expect(capture.keyDown('KeyA', false)).toBe(false);
    expect(capture.keyUp('KeyA')).toBe(false);
    expect(sent).toEqual([]);
  });

  it('不把对象原型上的属性名当作键位', () => {
    const { capture, sent } = setup();
    expect(capture.keyDown('constructor', false)).toBe(false);
    expect(capture.keyDown('toString', false)).toBe(false);
    expect(sent).toEqual([]);
  });
});

describe('按下与松开', () => {
  it('同时按下多个键时发送合成的整个掩码，松开一个只去掉它那一位', () => {
    const { capture, sent } = setup();
    capture.keyDown('ArrowRight', false);
    capture.keyDown('KeyX', false);
    capture.keyUp('ArrowRight');
    capture.keyUp('KeyX');
    expect(sent).toEqual([
      [0, PAD_BUTTON.right],
      [0, PAD_BUTTON.right | PAD_BUTTON.a],
      [0, PAD_BUTTON.a],
      [0, 0]
    ]);
  });

  it('重复的 keydown（repeat 为 true）不重复发送，仍算已处理', () => {
    const { capture, sent } = setup();
    capture.keyDown('KeyX', false);
    expect(capture.keyDown('KeyX', true)).toBe(true);
    expect(capture.keyDown('KeyX', true)).toBe(true);
    expect(sent).toEqual([[0, PAD_BUTTON.a]]);
  });

  it('没有 repeat 标记但已经按住的键再次 keydown 也不重复发送', () => {
    const { capture, sent } = setup();
    capture.keyDown('KeyX', false);
    capture.keyDown('KeyX', false);
    expect(sent).toHaveLength(1);
  });

  it('没按住的键的 keyup 不发送', () => {
    const { capture, sent } = setup();
    expect(capture.keyUp('KeyX')).toBe(true);
    expect(sent).toEqual([]);
  });

  it('两个键映射到同一个按键位时，松开一个仍保持按下', () => {
    const { capture, sent } = setup({ KeyX: { port: 0, button: PAD_BUTTON.a }, KeyC: { port: 0, button: PAD_BUTTON.a } });
    capture.keyDown('KeyX', false);
    capture.keyDown('KeyC', false);
    capture.keyUp('KeyX');
    capture.keyUp('KeyC');
    expect(sent).toEqual([
      [0, PAD_BUTTON.a],
      [0, 0]
    ]);
  });

  it('映射到手柄 2 的键发给口 1，口 0 的掩码不变', () => {
    const { capture, sent } = setup({ KeyJ: { port: 1, button: PAD_BUTTON.start } });
    capture.keyDown('KeyJ', false);
    expect(sent).toEqual([[1, PAD_BUTTON.start]]);
  });

  it('映射里口号超出范围的键被忽略，不发送', () => {
    const { capture, sent } = setup({ KeyJ: { port: 5, button: PAD_BUTTON.start } });
    capture.keyDown('KeyJ', false);
    expect(sent).toEqual([]);
  });
});

describe('全部松开', () => {
  it('窗口失焦等情况下一次松开全部按住的键', () => {
    const { capture, sent } = setup();
    capture.keyDown('ArrowLeft', false);
    capture.keyDown('KeyZ', false);
    sent.length = 0;
    capture.releaseAll();
    expect(sent).toEqual([[0, 0]]);
  });

  it('松开后再按同一个键会重新发送', () => {
    const { capture, sent } = setup();
    capture.keyDown('KeyX', false);
    capture.releaseAll();
    capture.keyDown('KeyX', false);
    expect(sent).toEqual([
      [0, PAD_BUTTON.a],
      [0, 0],
      [0, PAD_BUTTON.a]
    ]);
  });

  it('没有按住任何键时不发送，重复调用也不发送', () => {
    const { capture, sent } = setup();
    capture.releaseAll();
    capture.keyDown('KeyX', false);
    capture.releaseAll();
    capture.releaseAll();
    expect(sent).toEqual([
      [0, PAD_BUTTON.a],
      [0, 0]
    ]);
  });

  it('两个手柄口都有按键时各自发送', () => {
    const { capture, sent } = setup({ KeyX: { port: 0, button: PAD_BUTTON.a }, KeyJ: { port: 1, button: PAD_BUTTON.a } });
    capture.keyDown('KeyX', false);
    capture.keyDown('KeyJ', false);
    sent.length = 0;
    capture.releaseAll();
    expect(sent).toEqual([
      [0, 0],
      [1, 0]
    ]);
  });
});

describe('可编辑元素不采集', () => {
  it('在文本框、多行文字框、下拉框、可编辑区域里按键不采集也不拦截', () => {
    const input = document.createElement('input');
    const textarea = document.createElement('textarea');
    const select = document.createElement('select');
    const editable = { tagName: 'DIV', isContentEditable: true };
    for (const target of [input, textarea, select, editable]) {
      const { capture, sent } = setup();
      expect(capture.keyDown('KeyX', false, target)).toBe(false);
      expect(capture.keyUp('KeyX', target)).toBe(false);
      expect(sent).toEqual([]);
    }
  });

  it('在按钮、复选框、普通元素上照常采集', () => {
    const checkbox = document.createElement('input');
    checkbox.type = 'checkbox';
    for (const target of [document.createElement('button'), checkbox, document.createElement('div'), undefined, null]) {
      const { capture, sent } = setup();
      expect(capture.keyDown('KeyX', false, target)).toBe(true);
      expect(sent).toEqual([[0, PAD_BUTTON.a]]);
    }
  });

  it('isEditableTarget 区分各种元素', () => {
    const text = document.createElement('input');
    text.type = 'text';
    const range = document.createElement('input');
    range.type = 'range';
    expect(isEditableTarget(text)).toBe(true);
    expect(isEditableTarget(range)).toBe(false);
    expect(isEditableTarget(document.createElement('button'))).toBe(false);
    expect(isEditableTarget(null)).toBe(false);
    expect(isEditableTarget('input')).toBe(false);
  });

  it('按住的键在焦点移入输入框后，松开事件被忽略，需要靠失焦时的全部松开', () => {
    const { capture, sent } = setup();
    capture.keyDown('KeyX', false);
    capture.keyUp('KeyX', document.createElement('input'));
    expect(sent).toEqual([[0, PAD_BUTTON.a]]);
    capture.releaseAll();
    expect(sent[sent.length - 1]).toEqual([0, 0]);
  });
});

describe('替换映射表', () => {
  it('换映射时先松开旧映射下按住的键，之后按新映射采集', () => {
    const { capture, sent } = setup();
    capture.keyDown('KeyX', false);
    capture.setKeyMap({ KeyK: { port: 0, button: PAD_BUTTON.b } });
    expect(sent).toEqual([
      [0, PAD_BUTTON.a],
      [0, 0]
    ]);
    expect(capture.keyDown('KeyX', false)).toBe(false);
    expect(capture.keyDown('KeyK', false)).toBe(true);
    expect(sent[sent.length - 1]).toEqual([0, PAD_BUTTON.b]);
  });
});
