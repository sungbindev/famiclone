/*
 * ------------------------------------------------------------------------
 * 名称：file-last-directories-service.test.ts
 * 说明：记住上次目录服务的测试：没有保存过与内容损坏时为空、读出保存的目录并跳过不合格的项、修改后写入存储、相同的目录不重复写、flush 转给存储，并用真实的文件存储走一遍“写盘后重启读回”。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { describe, expect, it, vi } from 'vitest';
import { JsonFileStore } from '../../platform/shell-state/json-file-store';
import { FileLastDirectoriesService } from './file-last-directories-service';

/** 假存储：读出的内容由测试决定，写入记录在 writes 里。 */
function createStorage(saved: unknown) {
  const writes: unknown[] = [];
  return { read: () => saved, write: (value: unknown) => writes.push(value), flush: vi.fn(), writes };
}

describe('FileLastDirectoriesService', () => {
  it('没有保存过或内容损坏时什么都没记过', () => {
    expect(new FileLastDirectoriesService(createStorage(undefined)).get('floppy')).toBeUndefined();
    expect(new FileLastDirectoriesService(createStorage('坏的')).get('floppy')).toBeUndefined();
    expect(new FileLastDirectoriesService(createStorage({ version: 2, directories: { floppy: 'D:/a' } })).get('floppy')).toBeUndefined();
  });

  it('读出保存的目录，不合格的项跳过', () => {
    const service = new FileLastDirectoriesService(
      createStorage({ version: 1, directories: { floppy: 'D:/images', 'bad purpose': 'D:/x', tape: '', cartridge: 5, bios: 'D:/bios' } })
    );
    expect(service.get('floppy')).toBe('D:/images');
    expect(service.get('bios')).toBe('D:/bios');
    expect(service.get('tape')).toBeUndefined();
    expect(service.get('cartridge')).toBeUndefined();
  });

  it('记下目录后把整份记录交给存储，相同的目录不重复写', () => {
    const storage = createStorage(undefined);
    const service = new FileLastDirectoriesService(storage);
    service.set('floppy', 'D:/images');
    service.set('floppy', 'D:/images');
    service.set('tape', 'D:/tapes');
    expect(service.get('floppy')).toBe('D:/images');
    expect(storage.writes).toEqual([
      { version: 1, directories: { floppy: 'D:/images' } },
      { version: 1, directories: { floppy: 'D:/images', tape: 'D:/tapes' } }
    ]);
  });

  it('用途或目录不合格时不记', () => {
    const storage = createStorage(undefined);
    const service = new FileLastDirectoriesService(storage);
    service.set('', 'D:/a');
    service.set('1floppy', 'D:/a');
    service.set('floppy', '');
    expect(storage.writes).toEqual([]);
  });

  it('flush 转给存储', () => {
    const storage = createStorage(undefined);
    new FileLastDirectoriesService(storage).flush();
    expect(storage.flush).toHaveBeenCalledTimes(1);
  });

  it('用真实的文件存储：写盘后重新创建服务能读回', () => {
    const directory = mkdtempSync(join(tmpdir(), 'ember-last-directories-'));
    try {
      const path = join(directory, 'config', 'last-directories.json');
      const first = new FileLastDirectoriesService(new JsonFileStore({ path }));
      first.set('floppy', 'D:/images');
      first.flush();
      expect(new FileLastDirectoriesService(new JsonFileStore({ path })).get('floppy')).toBe('D:/images');
    } finally {
      rmSync(directory, { recursive: true, force: true });
    }
  });
});
