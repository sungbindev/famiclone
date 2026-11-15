/*
 * ------------------------------------------------------------------------
 * 名称：use-file-drop.test.tsx
 * 说明：窗口文件拖放的测试：带文件的拖动被接受（指针显示复制）而别的拖动不处理、拖入的文件登记后按扩展名（不分大小写）交给认领的处理项、没人认领与读不到路径时用通知说明、卸载后不再响应。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：用假的 window.ember；jsdom 没有 DataTransfer，用带 types 与 files 的对象代替。
 * ------------------------------------------------------------------------
 */

import { act, fireEvent, render } from '@testing-library/react';
import { describe, expect, it, vi } from 'vitest';
import type { DroppedFile } from '../../contracts/shell/dropped-files';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { ShellFileHandlerDefinition } from './shell-registry';
import { fileHandlerFor, useFileDrop } from './use-file-drop';

/** 装上拖放监听的空组件。 */
function Probe(props: { bridge: EmberBridge; handlers: readonly ShellFileHandlerDefinition[]; notify: (text: string, level?: string) => void }) {
  useFileDrop(props.bridge, props.handlers, props.notify as never);
  return null;
}

/** 假的 window.ember：登记时按名字给出编号。 */
function fakeBridge(register: (files: readonly File[]) => Promise<readonly DroppedFile[]>) {
  const registerDroppedFiles = vi.fn(register);
  return { bridge: { shell: { registerDroppedFiles } } as unknown as EmberBridge, registerDroppedFiles };
}

const file = (name: string) => ({ name }) as unknown as File;
const transferOf = (types: string[], files: File[] = []) => ({ types, files, dropEffect: 'none' });

/** 两个处理项：卡带与软盘镜像。 */
function handlers() {
  const opened: string[] = [];
  const list: ShellFileHandlerDefinition[] = [
    { id: 'a.cartridge', extensions: ['nes'], open: (item) => opened.push(`cartridge ${item.id}`) },
    { id: 'a.image', extensions: ['img', 'ima'], open: (item) => opened.push(`image ${item.id}`) }
  ];
  return { opened, list };
}

/** 登记结果给出的清单：编号按文件顺序。 */
const numbered = (files: readonly File[]) => Promise.resolve(files.map((item, index) => ({ id: index + 1, name: item.name })));

describe('fileHandlerFor', () => {
  it('按扩展名认领，不分大小写；没有扩展名或没人认领时没有处理项', () => {
    const { list } = handlers();
    expect(fileHandlerFor(list, 'Mario.NES')?.id).toBe('a.cartridge');
    expect(fileHandlerFor(list, 'sys.ima')?.id).toBe('a.image');
    expect(fileHandlerFor(list, 'a.b.img')?.id).toBe('a.image');
    expect(fileHandlerFor(list, 'notes.txt')).toBeUndefined();
    expect(fileHandlerFor(list, 'nes')).toBeUndefined();
    expect(fileHandlerFor(list, 'nes.')).toBeUndefined();
  });
});

describe('useFileDrop', () => {
  it('带文件的拖动被接受：阻止默认行为，指针显示复制；别的拖动不处理', () => {
    const { bridge } = fakeBridge(numbered);
    render(<Probe bridge={bridge} handlers={[]} notify={vi.fn()} />);
    const withFiles = transferOf(['Files']);
    expect(fireEvent.dragOver(window, { dataTransfer: withFiles })).toBe(false);
    expect(withFiles.dropEffect).toBe('copy');
    const text = transferOf(['text/plain']);
    expect(fireEvent.dragOver(window, { dataTransfer: text })).toBe(true);
    expect(text.dropEffect).toBe('none');
  });

  it('拖入的文件登记后按扩展名交给认领的处理项，一次拖入多个文件时逐个处理', async () => {
    const { bridge, registerDroppedFiles } = fakeBridge(numbered);
    const { opened, list } = handlers();
    render(<Probe bridge={bridge} handlers={list} notify={vi.fn()} />);
    const dropped = [file('idle.nes'), file('SYS.IMG')];
    await act(async () => {
      expect(fireEvent.drop(window, { dataTransfer: transferOf(['Files'], dropped) })).toBe(false);
    });
    expect(registerDroppedFiles).toHaveBeenCalledWith(dropped);
    expect(opened).toEqual(['cartridge 1', 'image 2']);
  });

  it('没有处理项认领的文件用通知说明，认领的照常处理', async () => {
    const { bridge } = fakeBridge(numbered);
    const { opened, list } = handlers();
    const notify = vi.fn();
    render(<Probe bridge={bridge} handlers={list} notify={notify} />);
    await act(async () => {
      fireEvent.drop(window, { dataTransfer: transferOf(['Files'], [file('notes.txt'), file('idle.nes')]) });
    });
    expect(notify).toHaveBeenCalledTimes(1);
    expect(notify).toHaveBeenCalledWith('不支持拖入的文件：notes.txt。可以拖入 NES 卡带、软盘镜像或声音文件', 'error');
    expect(opened).toEqual(['cartridge 2']);
  });

  it('读不到路径（登记结果为空或登记失败）时用通知说明', async () => {
    const notify = vi.fn();
    const empty = fakeBridge(() => Promise.resolve([]));
    const first = render(<Probe bridge={empty.bridge} handlers={[]} notify={notify} />);
    await act(async () => {
      fireEvent.drop(window, { dataTransfer: transferOf(['Files'], [file('idle.nes')]) });
    });
    first.unmount();
    const failing = fakeBridge(() => Promise.reject(new Error('失败')));
    render(<Probe bridge={failing.bridge} handlers={[]} notify={notify} />);
    await act(async () => {
      fireEvent.drop(window, { dataTransfer: transferOf(['Files'], [file('idle.nes')]) });
    });
    expect(notify.mock.calls).toEqual([
      ['无法读取拖入的文件，请从文件管理器里拖入磁盘上的文件', 'error'],
      ['无法读取拖入的文件，请从文件管理器里拖入磁盘上的文件', 'error']
    ]);
  });

  it('不带文件的放下不处理、不登记；卸载后不再响应', async () => {
    const { bridge, registerDroppedFiles } = fakeBridge(numbered);
    const { unmount } = render(<Probe bridge={bridge} handlers={[]} notify={vi.fn()} />);
    await act(async () => {
      expect(fireEvent.drop(window, { dataTransfer: transferOf(['text/plain']) })).toBe(true);
    });
    unmount();
    await act(async () => {
      expect(fireEvent.drop(window, { dataTransfer: transferOf(['Files'], [file('idle.nes')]) })).toBe(true);
    });
    expect(registerDroppedFiles).not.toHaveBeenCalled();
  });
});
