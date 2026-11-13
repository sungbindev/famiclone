/*
 * ------------------------------------------------------------------------
 * 名称：open-file-dialog.test.ts
 * 说明：文件选择窗口的测试：选项传给系统窗口、有父窗口时模态弹出、取消返回 undefined、只取第一个文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的 electron 模块。
 * ------------------------------------------------------------------------
 */

import { beforeEach, describe, expect, it, vi } from 'vitest';

const electron = vi.hoisted(() => ({ showOpenDialog: vi.fn() }));

vi.mock('electron', () => ({ dialog: { showOpenDialog: electron.showOpenDialog } }));

const { createOpenFileDialog } = await import('./open-file-dialog');

const OPTIONS = { title: '打开卡带', filters: [{ name: 'NES 卡带', extensions: ['nes'] }] };

describe('createOpenFileDialog', () => {
  beforeEach(() => electron.showOpenDialog.mockReset());

  it('没有父窗口时单独弹出，只选一个文件并按选项过滤', async () => {
    electron.showOpenDialog.mockResolvedValue({ canceled: false, filePaths: ['D:/roms/idle.nes'] });
    await expect(createOpenFileDialog(() => undefined).pickFile(OPTIONS)).resolves.toBe('D:/roms/idle.nes');
    expect(electron.showOpenDialog).toHaveBeenCalledTimes(1);
    expect(electron.showOpenDialog).toHaveBeenCalledWith({ title: '打开卡带', properties: ['openFile'], filters: [{ name: 'NES 卡带', extensions: ['nes'] }] });
  });

  it('有父窗口时以它为父窗口', async () => {
    electron.showOpenDialog.mockResolvedValue({ canceled: false, filePaths: ['a.nes'] });
    const parent = { name: '主窗口' };
    await createOpenFileDialog(() => parent as never).pickFile(OPTIONS);
    expect(electron.showOpenDialog).toHaveBeenCalledWith(parent, expect.objectContaining({ properties: ['openFile'] }));
  });

  it('用户取消时返回 undefined', async () => {
    electron.showOpenDialog.mockResolvedValue({ canceled: true, filePaths: [] });
    await expect(createOpenFileDialog(() => undefined).pickFile(OPTIONS)).resolves.toBeUndefined();
  });

  it('没有选中任何文件时返回 undefined', async () => {
    electron.showOpenDialog.mockResolvedValue({ canceled: false, filePaths: [] });
    await expect(createOpenFileDialog(() => undefined).pickFile(OPTIONS)).resolves.toBeUndefined();
  });
});
