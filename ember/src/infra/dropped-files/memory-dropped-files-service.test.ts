/*
 * ------------------------------------------------------------------------
 * 名称：memory-dropped-files-service.test.ts
 * 说明：拖入窗口的文件服务的测试：编号从 1 递增并给出文件名、按编号取回路径、没登记过的编号取不到、只保留最近的若干个。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { MemoryDroppedFilesService } from './memory-dropped-files-service';

describe('MemoryDroppedFilesService', () => {
  it('编号从 1 递增，显示名取文件名，登记多次时编号接着往下走', () => {
    const service = new MemoryDroppedFilesService();
    expect(service.register(['D:\\roms\\idle.nes', 'D:\\disks\\sys.img'])).toEqual([
      { id: 1, name: 'idle.nes' },
      { id: 2, name: 'sys.img' }
    ]);
    expect(service.register(['D:\\tapes\\lesson.m4a'])).toEqual([{ id: 3, name: 'lesson.m4a' }]);
  });

  it('按编号取回登记的路径，没登记过的编号取不到', () => {
    const service = new MemoryDroppedFilesService();
    service.register(['D:\\roms\\idle.nes']);
    expect(service.resolve(1)).toBe('D:\\roms\\idle.nes');
    expect(service.resolve(2)).toBeUndefined();
    expect(service.resolve(0)).toBeUndefined();
  });

  it('只保留最近登记的 64 个，更早的编号失效', () => {
    const service = new MemoryDroppedFilesService();
    for (let index = 0; index < 70; index += 1) service.register([`D:\\roms\\${index}.nes`]);
    expect(service.resolve(6)).toBeUndefined();
    expect(service.resolve(7)).toBe('D:\\roms\\6.nes');
    expect(service.resolve(70)).toBe('D:\\roms\\69.nes');
  });
});
