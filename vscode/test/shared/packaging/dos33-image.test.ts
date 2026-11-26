// ------------------------------------------------------------------------
// 名称：dos33-image.test.ts
// 说明：测试在 DOS 3.3 镜像上添加文件：程序是二进制（B）、其他是文本（T），
//       读出内容一致，占用的扇区与空闲位图一致，同名替换会释放旧扇区，
//       大文件用多个轨道扇区列表，目录与空间不足，底盘不合法，以及不改
//       传入的底盘。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-26
// 备注：底盘用 test/support/dos33-fixtures.ts 合成。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { listDos33, overlayDos33Image, readDos33File } from '../../../src/shared/packaging/dos33-image';
import { countFreeSectors, createBlankDos33Image } from '../../support/dos33-fixtures';

/** 合成一个二进制程序：4 字节文件头（装入地址、长度）加内容。 */
function program(address: number, body: Buffer): Buffer {
  const header = Buffer.alloc(4);
  header.writeUInt16LE(address, 0);
  header.writeUInt16LE(body.length, 2);
  return Buffer.concat([header, body]);
}

describe('DOS 3.3 镜像：添加文件', () => {
  it('程序是二进制文件，其他文件是文本文件，名字转成大写，读出的内容一致', () => {
    const base = createBlankDos33Image();
    const hello = program(0x6000, Buffer.from(Array.from({ length: 600 }, (_, index) => index & 0xff)));
    const image = overlayDos33Image(base, [
      { name: 'Hello', data: hello, program: true },
      { name: 'readme.txt', data: Buffer.from('hello text') }
    ]);
    expect(listDos33(image)).toEqual([
      { name: 'HELLO', type: 4, sectors: 4 },
      { name: 'README.TXT', type: 0, sectors: 2 }
    ]);
    expect(readDos33File(image, 'HELLO')).toEqual(hello);
    expect(readDos33File(image, 'README.TXT')?.subarray(0, 10).toString()).toBe('hello text');
    expect(readDos33File(image, 'NONE')).toBeUndefined();
  });

  it('占用的扇区在空闲位图里标成占用，不放在磁道 0 与磁道 17，数量等于目录里的扇区数', () => {
    const base = createBlankDos33Image();
    const image = overlayDos33Image(base, [{ name: 'BIG', data: program(0x6000, Buffer.alloc(3000, 0x11)), program: true }]);
    const [entry] = listDos33(image);
    expect(entry?.sectors).toBe(Math.ceil(3004 / 256) + 1);
    expect(countFreeSectors(base) - countFreeSectors(image)).toBe(entry?.sectors);
    // 目录扇区所在的磁道 17 不被取走：位图仍然是全 0
    expect(image.readUInt32BE(17 * 4096 + 0x38 + 17 * 4)).toBe(0);
  });

  it('文件超过 122 个数据扇区时用多个轨道扇区列表，读出的内容一致', () => {
    const body = Buffer.from(Array.from({ length: 40_000 }, (_, index) => (index * 7) & 0xff));
    const image = overlayDos33Image(createBlankDos33Image(), [{ name: 'LARGE', data: program(0x0800, body), program: true }]);
    const [entry] = listDos33(image);
    expect(entry?.sectors).toBe(Math.ceil(40_004 / 256) + 2);
    expect(readDos33File(image, 'LARGE')).toEqual(program(0x0800, body));
  });

  it('同名文件被替换，旧文件占用的扇区全部释放', () => {
    const base = createBlankDos33Image();
    const first = overlayDos33Image(base, [{ name: 'HELLO', data: program(0x6000, Buffer.alloc(2000, 1)), program: true }]);
    const second = overlayDos33Image(first, [{ name: 'hello', data: program(0x6000, Buffer.alloc(100, 2)), program: true }]);
    expect(listDos33(second)).toEqual([{ name: 'HELLO', type: 4, sectors: 2 }]);
    expect(countFreeSectors(base) - countFreeSectors(second)).toBe(2);
    expect(readDos33File(second, 'HELLO')).toEqual(program(0x6000, Buffer.alloc(100, 2)));
  });

  it('不修改传入的底盘，底盘里原有的其他文件保留', () => {
    const base = createBlankDos33Image();
    const withText = overlayDos33Image(base, [{ name: 'KEEP', data: Buffer.from('keep me') }]);
    const snapshot = Buffer.from(withText);
    const image = overlayDos33Image(withText, [{ name: 'HELLO', data: program(0x6000, Buffer.alloc(10)), program: true }]);
    expect(withText).toEqual(snapshot);
    expect(listDos33(image).map((entry) => entry.name)).toEqual(['KEEP', 'HELLO']);
    expect(readDos33File(image, 'KEEP')?.subarray(0, 7).toString()).toBe('keep me');
  });

  it('同名的锁定文件（如系统盘的启动程序）不替换，报 DISK_FILE_LOCKED，镜像不变', () => {
    const base = overlayDos33Image(createBlankDos33Image(), [{ name: 'HELLO', data: Buffer.from('boot') }]);
    // 第一个目录项的类型字节：目录扇区 15 的偏移 11 + 2，位 7 是锁定
    const typeAt = 17 * 4096 + 15 * 256 + 11 + 2;
    base.writeUInt8(base.readUInt8(typeAt) | 0x80, typeAt);
    const snapshot = Buffer.from(base);
    expect(() => overlayDos33Image(base, [{ name: 'hello', data: program(0x6000, Buffer.alloc(4)), program: true }])).toThrow(expect.objectContaining({ code: 'DISK_FILE_LOCKED' }));
    expect(base).toEqual(snapshot);
  });

  it('空间不足、目录已满时报 DISK_FULL', () => {
    const tracks = Array.from({ length: 35 }, (_, track) => track).filter((track) => track !== 20);
    const small = createBlankDos33Image({ usedTracks: tracks });
    expect(() => overlayDos33Image(small, [{ name: 'BIG', data: Buffer.alloc(6000), program: true }])).toThrow(expect.objectContaining({ code: 'DISK_FULL' }));
    let image = createBlankDos33Image();
    for (let index = 0; index < 105; index += 1) image = overlayDos33Image(image, [{ name: `F${index}`, data: Buffer.alloc(1) }]);
    expect(() => overlayDos33Image(image, [{ name: 'ONE-MORE', data: Buffer.alloc(1) }])).toThrow(expect.objectContaining({ code: 'DISK_FULL' }));
  });

  it('文件名不合规时报 DISK_NAME_INVALID', () => {
    const base = createBlankDos33Image();
    for (const name of ['', 'A,B', ' LEADING', 'X'.repeat(31), '中文']) {
      expect(() => overlayDos33Image(base, [{ name, data: Buffer.alloc(1) }])).toThrow(expect.objectContaining({ code: 'DISK_NAME_INVALID' }));
    }
  });

  it('底盘不是 DOS 3.3 镜像时报 DISK_BASE_INVALID', () => {
    expect(() => overlayDos33Image(Buffer.alloc(1000), [])).toThrow(expect.objectContaining({ code: 'DISK_BASE_INVALID' }));
    const broken = createBlankDos33Image();
    broken.writeUInt8(0, 17 * 4096 + 0x35);
    expect(() => overlayDos33Image(broken, [])).toThrow(expect.objectContaining({ code: 'DISK_BASE_INVALID' }));
    const loop = createBlankDos33Image();
    loop.writeUInt8(17, 17 * 4096 + 1 * 256 + 1);
    loop.writeUInt8(15, 17 * 4096 + 1 * 256 + 2);
    expect(() => overlayDos33Image(loop, [{ name: 'A', data: Buffer.alloc(1) }])).toThrow(expect.objectContaining({ code: 'DISK_BASE_INVALID' }));
  });
});
