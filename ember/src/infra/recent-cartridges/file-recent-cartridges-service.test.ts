/*
 * ------------------------------------------------------------------------
 * 名称：file-recent-cartridges-service.test.ts
 * 说明：最近打开的卡带服务的测试：启动时读出保存的清单、文件缺失或损坏时得到空清单、加入后写盘、编号在本次运行内固定、按编号取路径、渲染进程只拿到显示名与编号。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用内存里的假存储；损坏回退的“读文件失败或不是 JSON”由 json-file-store 的测试覆盖，这里覆盖“内容不合格”。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it, vi } from 'vitest';
import { FileRecentCartridgesService, type RecentCartridgesStorage } from './file-recent-cartridges-service';

function memoryStorage(initial: unknown): RecentCartridgesStorage & { written: unknown[] } {
  const written: unknown[] = [];
  return { read: () => initial, write: (value) => written.push(value), flush: vi.fn(), written };
}

describe('FileRecentCartridgesService', () => {
  it('读出保存的清单，对界面只给编号与显示名', () => {
    const service = new FileRecentCartridgesService(memoryStorage({ version: 1, items: [{ path: 'D:\\roms\\a.nes' }, { path: 'D:\\roms\\b.nes' }] }));
    expect(service.list()).toEqual([
      { id: 1, name: 'a.nes' },
      { id: 2, name: 'b.nes' }
    ]);
    expect(JSON.stringify(service.list())).not.toContain('roms');
  });

  it('文件不存在、内容损坏或不合格时得到空清单，不抛异常', () => {
    for (const initial of [undefined, null, '损坏', 7, { items: 3 }, { items: [{ path: 'x.txt' }] }]) {
      expect(new FileRecentCartridgesService(memoryStorage(initial)).list()).toEqual([]);
    }
  });

  it('加入后写盘，最近的在前，已有的路径编号不变', () => {
    const storage = memoryStorage({ items: [{ path: 'D:/a.nes' }] });
    const service = new FileRecentCartridgesService(storage);
    service.add('D:/b.nes');
    service.add('D:/a.nes');
    expect(service.list()).toEqual([
      { id: 1, name: 'a.nes' },
      { id: 2, name: 'b.nes' }
    ]);
    expect(storage.written).toHaveLength(2);
    expect(storage.written.at(-1)).toEqual({ version: 1, items: [{ path: 'D:/a.nes' }, { path: 'D:/b.nes' }] });
  });

  it('按编号取路径；移出后编号失效；没有变化时不写盘', () => {
    const storage = memoryStorage({ items: [{ path: 'D:/a.nes' }] });
    const service = new FileRecentCartridgesService(storage);
    expect(service.resolve(1)).toBe('D:/a.nes');
    expect(service.resolve(99)).toBeUndefined();
    service.remove('D:/a.nes');
    expect(service.resolve(1)).toBeUndefined();
    expect(service.list()).toEqual([]);
    service.remove('D:/a.nes');
    service.add('D:/notes.txt');
    expect(storage.written).toHaveLength(1);
  });

  it('flush 交给存储立即写盘', () => {
    const storage = memoryStorage(undefined);
    new FileRecentCartridgesService(storage).flush();
    expect(storage.flush).toHaveBeenCalledTimes(1);
  });
});
