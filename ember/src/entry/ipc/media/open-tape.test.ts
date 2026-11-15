/*
 * ------------------------------------------------------------------------
 * 名称：open-tape.test.ts
 * 说明：media.openTape 与 media.openTapeRecording 的测试：文件选择显示声音文件与所有文件、保存窗口只显示 WAV，选定的路径交给核心的 media.tapeLoad 与 media.tapeRecord，取消时返回明确的结果而不是错误，核心的错误码原样返回，参数不合格时在入口被拒绝；media.openTape 给出 droppedId 时不弹窗口，用拖入窗口的声音文件，编号无效或扩展名不是声音时返回 media.droppedNotFound。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的文件选择窗口与假的核心网关；经主进程请求入口调用，覆盖从请求名到核心的整条转接。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import type { CoreGateway } from '../../../contracts/core-gateway/core-gateway';
import { CoreRequestError } from '../../../contracts/core-gateway/core-messages';
import { MEDIA_SCHEMA } from '../../../contracts/media/generated/media';
import type { DroppedFilesService } from '../../../contracts/shell/dropped-files';
import type { FileDialog } from '../../../contracts/shell/file-dialog';
import { handleMainRequest } from '../core/main-request-ipc';
import { createOpenTapeHandler, createOpenTapeRecordingHandler } from './open-tape';

const STATUS = { connected: true, loaded: true, fileName: 'lesson.m4a', mode: 'stopped', position: 0, duration: 1689.5, stereo: true };

/** 拖入窗口的文件：1 是声音文件，2 不是。 */
const DROPPED: DroppedFilesService = {
  register: () => [],
  resolve: (id) => (id === 1 ? 'D:/dropped/lesson.M4A' : id === 2 ? 'D:/dropped/notes.txt' : undefined)
};

function setup(dialog: Partial<FileDialog>, request: CoreGateway['request'] = vi.fn().mockResolvedValue({ status: STATUS })) {
  const gateway: CoreGateway = { request, onEvent: () => () => undefined, onStatusChange: () => () => undefined, status: () => ({ state: 'starting' }) };
  const schemas = [MEDIA_SCHEMA];
  const core = { gateway, modules: [MEDIA_SCHEMA], schemas };
  const fileDialog: FileDialog = { pickFile: vi.fn(), pickDirectory: vi.fn(), pickSaveFile: vi.fn(), ...dialog };
  const handlers = { 'media.openTape': createOpenTapeHandler({ dialog: fileDialog, dropped: DROPPED, core }), 'media.openTapeRecording': createOpenTapeRecordingHandler({ dialog: fileDialog, core }) };
  return { request, call: (name: 'media.openTape' | 'media.openTapeRecording', params: unknown = {}) => handleMainRequest({ handlers, modules: [MEDIA_SCHEMA], schemas }, name, params) };
}

describe('media.openTape', () => {
  it('文件选择显示声音文件与所有文件，选定的路径交给核心的 media.tapeLoad', async () => {
    const pickFile = vi.fn().mockResolvedValue('D:/tapes/lesson.m4a');
    const { request, call } = setup({ pickFile });
    await expect(call('media.openTape')).resolves.toEqual({ ok: true, result: { canceled: false, status: STATUS } });
    expect(pickFile).toHaveBeenCalledWith({
      title: '载入磁带',
      filters: [
        { name: '声音文件', extensions: ['m4a', 'mp3', 'wav', 'aac', 'flac', 'ogg', 'opus', 'mp4', 'mka'] },
        { name: '所有文件', extensions: ['*'] }
      ]
    });
    expect(request).toHaveBeenCalledWith('media.tapeLoad', { path: 'D:/tapes/lesson.m4a' });
  });

  it('用户取消选择时返回 canceled，不是错误，也不调用核心', async () => {
    const { request, call } = setup({ pickFile: vi.fn().mockResolvedValue(undefined) });
    await expect(call('media.openTape')).resolves.toEqual({ ok: true, result: { canceled: true } });
    expect(request).not.toHaveBeenCalled();
  });

  it('核心拒绝文件时返回核心的错误码与说明', async () => {
    const failing = vi.fn().mockRejectedValue(new CoreRequestError({ code: 'media.tapeUnsupported', message: '文件里没有声音' }));
    const { call } = setup({ pickFile: vi.fn().mockResolvedValue('D:/tapes/x.txt') }, failing);
    await expect(call('media.openTape')).resolves.toEqual({ ok: false, error: { code: 'media.tapeUnsupported', message: '文件里没有声音' } });
  });

  it('带了多余的参数时在入口被拒绝，不弹出文件选择窗口', async () => {
    const pickFile = vi.fn();
    const { request, call } = setup({ pickFile });
    await expect(call('media.openTape', { path: 'D:/x.wav' })).resolves.toMatchObject({ ok: false, error: { code: 'core.invalidParams' } });
    expect(pickFile).not.toHaveBeenCalled();
    expect(request).not.toHaveBeenCalled();
  });

  it('给出 droppedId 时不弹窗口，用拖入窗口的声音文件交给 media.tapeLoad', async () => {
    const pickFile = vi.fn();
    const { request, call } = setup({ pickFile });
    await expect(call('media.openTape', { droppedId: 1 })).resolves.toEqual({ ok: true, result: { canceled: false, status: STATUS } });
    expect(pickFile).not.toHaveBeenCalled();
    expect(request).toHaveBeenCalledWith('media.tapeLoad', { path: 'D:/dropped/lesson.M4A' });
  });

  it('拖入的文件不是声音文件或编号没有登记过时返回 media.droppedNotFound，不调用核心', async () => {
    const { request, call } = setup({ pickFile: vi.fn() });
    for (const droppedId of [2, 99]) {
      await expect(call('media.openTape', { droppedId })).resolves.toMatchObject({ ok: false, error: { code: 'media.droppedNotFound' } });
    }
    expect(request).not.toHaveBeenCalled();
  });
});

describe('media.openTapeRecording', () => {
  it('保存窗口只显示 WAV，选定的路径交给核心的 media.tapeRecord', async () => {
    const pickSaveFile = vi.fn().mockResolvedValue('D:/tapes/out.wav');
    const { request, call } = setup({ pickSaveFile });
    await expect(call('media.openTapeRecording')).resolves.toEqual({ ok: true, result: { canceled: false, status: STATUS } });
    expect(pickSaveFile).toHaveBeenCalledWith({ title: '保存录音', filters: [{ name: 'WAV 声音文件', extensions: ['wav'] }] });
    expect(request).toHaveBeenCalledWith('media.tapeRecord', { path: 'D:/tapes/out.wav' });
  });

  it('用户取消选择时返回 canceled，不调用核心', async () => {
    const { request, call } = setup({ pickSaveFile: vi.fn().mockResolvedValue(undefined) });
    await expect(call('media.openTapeRecording')).resolves.toEqual({ ok: true, result: { canceled: true } });
    expect(request).not.toHaveBeenCalled();
  });

  it('核心无法创建文件时返回核心的错误码', async () => {
    const failing = vi.fn().mockRejectedValue(new CoreRequestError({ code: 'media.saveFailed', message: '无法创建录音文件' }));
    const { call } = setup({ pickSaveFile: vi.fn().mockResolvedValue('D:/readonly/out.wav') }, failing);
    await expect(call('media.openTapeRecording')).resolves.toEqual({ ok: false, error: { code: 'media.saveFailed', message: '无法创建录音文件' } });
  });
});
