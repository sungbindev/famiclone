/*
 * ------------------------------------------------------------------------
 * 名称：dialog-window.test.ts
 * 说明：对话框窗口的测试：请求解析（只接受组件库的空白对话框窗口，窗口特性的取值、缺省值与限幅）与关闭后还回焦点的目标。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-04
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { describe, expect, it, vi } from 'vitest';

vi.mock('electron', () => ({ BrowserWindow: {}, screen: {} }));

const { parseDialogWindowRequest, returnFocusTarget } = await import('./dialog-window');

/** 假窗口。 */
const fakeWindow = (destroyed: boolean) => ({ isDestroyed: () => destroyed });

describe('returnFocusTarget', () => {
  it('焦点还给打开对话框的窗口，它已销毁时还给主窗口，都已销毁时不处理', () => {
    const opener = fakeWindow(false);
    const parent = fakeWindow(false);
    expect(returnFocusTarget(opener, parent)).toBe(opener);
    expect(returnFocusTarget(fakeWindow(true), parent)).toBe(parent);
    expect(returnFocusTarget(fakeWindow(true), fakeWindow(true))).toBeUndefined();
  });
});

describe('parseDialogWindowRequest', () => {
  it('解析组件库给出的窗口特性', () => {
    const request = parseDialogWindowRequest({
      url: 'about:blank',
      frameName: 'ui-dialog-3',
      features: 'popup=yes,width=640,height=560,ui-modal=1,ui-closable=1,ui-resizable=1,ui-maximizable=1,ui-min-width=320,ui-min-height=80'
    });
    // 模态窗口不能最大化。
    expect(request).toEqual({ width: 640, height: 560, minWidth: 320, minHeight: 80, modal: true, closable: true, resizable: true, maximizable: false });
  });

  it('缺失的项用缺省值，尺寸限幅，最小尺寸不超过初始尺寸', () => {
    const request = parseDialogWindowRequest({ url: 'about:blank', frameName: 'ui-dialog-1', features: 'width=99999,height=abc,ui-min-width=900,ui-maximizable=1' });
    expect(request).toEqual({ width: 4000, height: 160, minWidth: 900, minHeight: 160, modal: false, closable: true, resizable: true, maximizable: true });
  });

  it('不是空白页或名称不符的窗口一律拒绝', () => {
    expect(parseDialogWindowRequest({ url: 'https://example.com/', frameName: 'ui-dialog-1', features: '' })).toBeUndefined();
    expect(parseDialogWindowRequest({ url: 'about:blank', frameName: 'other', features: '' })).toBeUndefined();
    expect(parseDialogWindowRequest({ url: 'about:blank', frameName: 'ui-dialog-1x', features: '' })).toBeUndefined();
  });
});
