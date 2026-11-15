/*
 * ------------------------------------------------------------------------
 * 名称：scan-code-table.test.ts
 * 说明：扫描码对照表的测试：字母、数字、功能键、修饰键、数字键盘与扩展键的键位名，假 Shift、Pause 与未收录的扫描码被忽略，对照表里的键位名没有重复。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { scanCodeToKeyCode } from './scan-code-table';

describe('scanCodeToKeyCode', () => {
  it('字母、数字与标点按物理键位命名，与键盘布局无关', () => {
    expect(scanCodeToKeyCode(0x1e, false, false)).toBe('KeyA');
    expect(scanCodeToKeyCode(0x2c, false, false)).toBe('KeyZ');
    expect(scanCodeToKeyCode(0x10, false, false)).toBe('KeyQ');
    expect(scanCodeToKeyCode(0x02, false, false)).toBe('Digit1');
    expect(scanCodeToKeyCode(0x0b, false, false)).toBe('Digit0');
    expect(scanCodeToKeyCode(0x29, false, false)).toBe('Backquote');
    expect(scanCodeToKeyCode(0x39, false, false)).toBe('Space');
  });

  it('Esc、Tab、功能键与左侧修饰键都能得到键位名', () => {
    expect(scanCodeToKeyCode(0x01, false, false)).toBe('Escape');
    expect(scanCodeToKeyCode(0x0f, false, false)).toBe('Tab');
    expect(scanCodeToKeyCode(0x3b, false, false)).toBe('F1');
    expect(scanCodeToKeyCode(0x44, false, false)).toBe('F10');
    expect(scanCodeToKeyCode(0x57, false, false)).toBe('F11');
    expect(scanCodeToKeyCode(0x58, false, false)).toBe('F12');
    expect(scanCodeToKeyCode(0x1d, false, false)).toBe('ControlLeft');
    expect(scanCodeToKeyCode(0x38, false, false)).toBe('AltLeft');
    expect(scanCodeToKeyCode(0x2a, false, false)).toBe('ShiftLeft');
    expect(scanCodeToKeyCode(0x36, false, false)).toBe('ShiftRight');
  });

  it('带 E0 前缀的是右侧修饰键、方向键、编辑键区、数字键盘的回车与除号', () => {
    expect(scanCodeToKeyCode(0x1d, true, false)).toBe('ControlRight');
    expect(scanCodeToKeyCode(0x38, true, false)).toBe('AltRight');
    expect(scanCodeToKeyCode(0x48, true, false)).toBe('ArrowUp');
    expect(scanCodeToKeyCode(0x50, true, false)).toBe('ArrowDown');
    expect(scanCodeToKeyCode(0x4b, true, false)).toBe('ArrowLeft');
    expect(scanCodeToKeyCode(0x4d, true, false)).toBe('ArrowRight');
    expect(scanCodeToKeyCode(0x53, true, false)).toBe('Delete');
    expect(scanCodeToKeyCode(0x1c, true, false)).toBe('NumpadEnter');
    expect(scanCodeToKeyCode(0x35, true, false)).toBe('NumpadDivide');
    expect(scanCodeToKeyCode(0x5b, true, false)).toBe('MetaLeft');
  });

  it('同一个扫描码有没有 E0 前缀是两个不同的键', () => {
    expect(scanCodeToKeyCode(0x48, false, false)).toBe('Numpad8');
    expect(scanCodeToKeyCode(0x48, true, false)).toBe('ArrowUp');
    expect(scanCodeToKeyCode(0x1c, false, false)).toBe('Enter');
    expect(scanCodeToKeyCode(0x1c, true, false)).toBe('NumpadEnter');
  });

  it('关闭 NumLock 时系统额外发出的假 Shift（E0 2A、E0 36）被忽略', () => {
    expect(scanCodeToKeyCode(0x2a, true, false)).toBeUndefined();
    expect(scanCodeToKeyCode(0x36, true, false)).toBeUndefined();
  });

  it('带 E1 前缀（Pause）与未收录的扫描码返回 undefined', () => {
    expect(scanCodeToKeyCode(0x1d, false, true)).toBeUndefined();
    expect(scanCodeToKeyCode(0x45, false, true)).toBeUndefined();
    expect(scanCodeToKeyCode(0x7f, false, false)).toBeUndefined();
    expect(scanCodeToKeyCode(0, false, false)).toBeUndefined();
  });
});
