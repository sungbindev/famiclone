/*
 * ------------------------------------------------------------------------
 * 名称：editable-target.test.ts
 * 说明：isTextEditingTarget 的测试：文本输入框、多行文字框、下拉框、可编辑区域算在编辑文字；按钮、复选框、滑块等不算；非元素的目标不算。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { isSelectTrigger, isTextEditingTarget } from './editable-target';

/** 造一个带指定标签与属性的元素。 */
function element(tag: string, attributes: Record<string, string> = {}): HTMLElement {
  const created = document.createElement(tag);
  for (const [name, value] of Object.entries(attributes)) created.setAttribute(name, value);
  return created;
}

describe('isTextEditingTarget', () => {
  it('文本输入框（含没写 type 与搜索、密码、数字）、多行文字框、下拉框算在编辑文字', () => {
    for (const target of [element('input'), element('input', { type: 'text' }), element('input', { type: 'search' }), element('input', { type: 'password' }), element('input', { type: 'number' }), element('textarea'), element('select')]) {
      expect(isTextEditingTarget(target)).toBe(true);
    }
  });

  it('contenteditable 的元素算在编辑文字', () => {
    const editable = element('div');
    Object.defineProperty(editable, 'isContentEditable', { value: true });
    expect(isTextEditingTarget(editable)).toBe(true);
  });

  it('按钮、复选框、单选框、滑块、文件框等不接收文字的 input 不算', () => {
    for (const type of ['button', 'checkbox', 'radio', 'range', 'submit', 'reset', 'image', 'file', 'color']) expect(isTextEditingTarget(element('input', { type }))).toBe(false);
  });

  it('普通元素、body、window 与空值不算', () => {
    for (const target of [element('div'), element('button'), document.body, window, null, undefined, 'x', 3]) expect(isTextEditingTarget(target)).toBe(false);
  });
});

describe('isSelectTrigger', () => {
  it('role=combobox 的元素及其内部元素算下拉列表的触发器，其他不算', () => {
    const trigger = element('button', { role: 'combobox' });
    const inner = document.createElement('span');
    trigger.append(inner);
    expect(isSelectTrigger(trigger)).toBe(true);
    expect(isSelectTrigger(inner)).toBe(true);
    for (const target of [element('button'), element('div'), window, null, undefined, 'x']) expect(isSelectTrigger(target)).toBe(false);
  });
});
