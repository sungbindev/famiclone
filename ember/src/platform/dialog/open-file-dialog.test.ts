/*
 * ------------------------------------------------------------------------
 * 名称：open-file-dialog.test.ts
 * 说明：文件选择窗口的测试：选项传给系统窗口、有父窗口时模态弹出、取消返回 undefined、只取第一个文件，以及按用途记住上次的目录（打开、选文件夹、保存）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的 electron 模块。
 * ------------------------------------------------------------------------
 */

import { join } from 'node:path';
import { beforeEach, describe, expect, it, vi } from 'vitest';

const electron = vi.hoisted(() => ({ showOpenDialog: vi.fn(), showSaveDialog: vi.fn() }));

vi.mock('electron', () => ({ dialog: { showOpenDialog: electron.showOpenDialog, showSaveDialog: electron.showSaveDialog } }));

const { createOpenFileDialog } = await import('./open-file-dialog');

const OPTIONS = { title: '打开卡带', purpose: 'cartridge', filters: [{ name: 'NES 卡带', extensions: ['nes'] }] };
const DIRECTORY = { title: '选择文件夹', purpose: 'folder' };

describe('createOpenFileDialog', () => {
  beforeEach(() => {
    electron.showOpenDialog.mockReset();
    electron.showSaveDialog.mockReset();
  });

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

  it('选择文件夹：系统窗口只显示文件夹，返回用户选定的路径，取消时返回 undefined', async () => {
    electron.showOpenDialog.mockResolvedValue({ canceled: false, filePaths: ['D:/prints'] });
    await expect(createOpenFileDialog(() => undefined).pickDirectory(DIRECTORY)).resolves.toBe('D:/prints');
    expect(electron.showOpenDialog).toHaveBeenCalledWith({ title: '选择文件夹', properties: ['openDirectory', 'createDirectory'] });
    electron.showOpenDialog.mockResolvedValue({ canceled: true, filePaths: [] });
    await expect(createOpenFileDialog(() => undefined).pickDirectory(DIRECTORY)).resolves.toBeUndefined();
  });

  it('保存文件：系统的保存窗口按选项过滤，有父窗口时以它为父窗口，取消时返回 undefined', async () => {
    const options = { title: '保存录音', purpose: 'tapeRecording', filters: [{ name: 'WAV', extensions: ['wav'] }] };
    electron.showSaveDialog.mockResolvedValue({ canceled: false, filePath: 'D:/tapes/out.wav' });
    await expect(createOpenFileDialog(() => undefined).pickSaveFile(options)).resolves.toBe('D:/tapes/out.wav');
    expect(electron.showSaveDialog).toHaveBeenCalledWith({ title: '保存录音', filters: [{ name: 'WAV', extensions: ['wav'] }] });
    const parent = { name: '主窗口' };
    await createOpenFileDialog(() => parent as never).pickSaveFile(options);
    expect(electron.showSaveDialog).toHaveBeenLastCalledWith(parent, expect.objectContaining({ title: '保存录音' }));
    electron.showSaveDialog.mockResolvedValue({ canceled: true, filePath: '' });
    await expect(createOpenFileDialog(() => undefined).pickSaveFile(options)).resolves.toBeUndefined();
  });
});

/** 内存里的目录记忆。 */
function createMemory(initial: Record<string, string> = {}) {
  const saved = new Map(Object.entries(initial));
  return { get: (purpose: string) => saved.get(purpose), set: vi.fn((purpose: string, directory: string) => saved.set(purpose, directory)), saved };
}

describe('createOpenFileDialog 记住上次的目录', () => {
  beforeEach(() => {
    electron.showOpenDialog.mockReset();
    electron.showSaveDialog.mockReset();
  });

  it('选文件：没记过时不带起始位置，选定后记下文件所在的目录，下次从那里打开', async () => {
    const memory = createMemory();
    const dialogs = createOpenFileDialog(() => undefined, memory);
    electron.showOpenDialog.mockResolvedValue({ canceled: false, filePaths: [join('D:', 'roms', 'idle.nes')] });
    await dialogs.pickFile(OPTIONS);
    expect(electron.showOpenDialog).toHaveBeenLastCalledWith({ title: '打开卡带', properties: ['openFile'], filters: [{ name: 'NES 卡带', extensions: ['nes'] }] });
    expect(memory.saved.get('cartridge')).toBe(join('D:', 'roms'));
    await dialogs.pickFile(OPTIONS);
    expect(electron.showOpenDialog).toHaveBeenLastCalledWith(expect.objectContaining({ defaultPath: join('D:', 'roms') }));
  });

  it('不同用途各记各的，互不影响', async () => {
    const memory = createMemory({ cartridge: 'D:/roms', floppy: 'D:/disks' });
    electron.showOpenDialog.mockResolvedValue({ canceled: true, filePaths: [] });
    await createOpenFileDialog(() => undefined, memory).pickFile({ ...OPTIONS, purpose: 'floppy' });
    expect(electron.showOpenDialog).toHaveBeenLastCalledWith(expect.objectContaining({ defaultPath: 'D:/disks' }));
  });

  it('用户取消时不改记住的目录', async () => {
    const memory = createMemory({ cartridge: 'D:/roms' });
    electron.showOpenDialog.mockResolvedValue({ canceled: true, filePaths: [] });
    await createOpenFileDialog(() => undefined, memory).pickFile(OPTIONS);
    expect(memory.set).not.toHaveBeenCalled();
  });

  it('选文件夹：记下选定的文件夹自己，下次从它打开', async () => {
    const memory = createMemory();
    const dialogs = createOpenFileDialog(() => undefined, memory);
    electron.showOpenDialog.mockResolvedValue({ canceled: false, filePaths: ['D:/prints'] });
    await dialogs.pickDirectory(DIRECTORY);
    expect(memory.saved.get('folder')).toBe('D:/prints');
    await dialogs.pickDirectory(DIRECTORY);
    expect(electron.showOpenDialog).toHaveBeenLastCalledWith(expect.objectContaining({ defaultPath: 'D:/prints', properties: ['openDirectory', 'createDirectory'] }));
  });

  it('保存文件：记下保存位置所在的目录，下次从那里打开', async () => {
    const memory = createMemory();
    const dialogs = createOpenFileDialog(() => undefined, memory);
    const options = { title: '保存录音', purpose: 'tapeRecording', filters: [{ name: 'WAV', extensions: ['wav'] }] };
    electron.showSaveDialog.mockResolvedValue({ canceled: false, filePath: join('D:', 'tapes', 'out.wav') });
    await dialogs.pickSaveFile(options);
    expect(memory.saved.get('tapeRecording')).toBe(join('D:', 'tapes'));
    await dialogs.pickSaveFile(options);
    expect(electron.showSaveDialog).toHaveBeenLastCalledWith(expect.objectContaining({ defaultPath: join('D:', 'tapes') }));
  });
});
