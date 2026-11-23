// ------------------------------------------------------------------------
// 名称：fat12-overlay.test.ts
// 说明：测试在已有 FAT12 镜像上添加文件：保留原有文件、读出新文件、同名替换、空间与根目录不足、
//       底盘不合法，以及不改传入的底盘。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-23
// 备注：底盘用测试里的函数按步步高系统盘的几何参数（每簇 4 扇区、208 个根目录项）合成。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { ENTRY_SIZE, END_OF_CHAIN, getFatEntry, setFatEntry, toShortName } from '../../../src/shared/packaging/fat12-image';
import { overlayFat12Image } from '../../../src/shared/packaging/fat12-overlay';

/** 合成底盘的几何参数。 */
interface BaseOptions {
  readonly sectorsPerCluster?: number;
  readonly rootEntries?: number;
  readonly totalSectors?: number;
  readonly files?: ReadonlyArray<{ readonly name: string; readonly data: Buffer }>;
}

/** 合成底盘：固定 512 字节扇区、每份 FAT 3 个扇区，原有文件从簇 2 起连续存放。 */
function makeBase({ sectorsPerCluster = 4, rootEntries = 208, totalSectors = 2880, files = [] }: BaseOptions = {}): Buffer {
  const image = Buffer.alloc(totalSectors * 512);
  image.writeUInt16LE(512, 11);
  image.writeUInt8(sectorsPerCluster, 13);
  image.writeUInt16LE(1, 14);
  image.writeUInt8(2, 16);
  image.writeUInt16LE(rootEntries, 17);
  image.writeUInt16LE(totalSectors, 19);
  image.writeUInt8(0xf0, 21);
  image.writeUInt16LE(3, 22);
  const fat = Buffer.alloc(3 * 512);
  setFatEntry(fat, 0, 0xff0);
  setFatEntry(fat, 1, 0xfff);
  const rootStart = (1 + 2 * 3) * 512;
  const dataStart = rootStart + Math.ceil((rootEntries * ENTRY_SIZE) / 512) * 512;
  const clusterBytes = sectorsPerCluster * 512;
  let next = 2;
  files.forEach((file, index) => {
    const need = Math.ceil(file.data.length / clusterBytes);
    const entryAt = rootStart + index * ENTRY_SIZE;
    image.write(toShortName(file.name), entryAt, 'ascii');
    image.writeUInt8(0x20, entryAt + 11);
    image.writeUInt16LE(next, entryAt + 26);
    image.writeUInt32LE(file.data.length, entryAt + 28);
    for (let part = 0; part < need; part += 1) {
      setFatEntry(fat, next + part, part === need - 1 ? END_OF_CHAIN : next + part + 1);
      file.data.copy(image, dataStart + (next + part - 2) * clusterBytes, part * clusterBytes, (part + 1) * clusterBytes);
    }
    next += need;
  });
  fat.copy(image, 512);
  fat.copy(image, 512 + 3 * 512);
  return image;
}

/** 按引导扇区的参数列出根目录里的文件名与内容。 */
function readFiles(image: Buffer): Map<string, Buffer> {
  const sectorsPerCluster = image.readUInt8(13);
  const rootEntries = image.readUInt16LE(17);
  const fat = image.subarray(512, 512 + 3 * 512);
  const rootStart = (1 + 2 * 3) * 512;
  const dataStart = rootStart + Math.ceil((rootEntries * ENTRY_SIZE) / 512) * 512;
  const clusterBytes = sectorsPerCluster * 512;
  const files = new Map<string, Buffer>();
  for (let index = 0; index < rootEntries; index += 1) {
    const at = rootStart + index * ENTRY_SIZE;
    const first = image.readUInt8(at);
    if (first === 0) break;
    if (first === 0xe5) continue;
    const chunks: Buffer[] = [];
    for (let cluster = image.readUInt16LE(at + 26); cluster >= 2 && cluster < 0xff8; cluster = getFatEntry(fat, cluster)) {
      chunks.push(image.subarray(dataStart + (cluster - 2) * clusterBytes, dataStart + (cluster - 1) * clusterBytes));
    }
    files.set(`${image.toString('ascii', at, at + 8).trimEnd()}.${image.toString('ascii', at + 8, at + 11).trimEnd()}`, Buffer.concat(chunks).subarray(0, image.readUInt32LE(at + 28)));
  }
  return files;
}

/** 数 FAT 里的空闲簇。 */
function freeClusters(image: Buffer): number {
  const fat = image.subarray(512, 512 + 3 * 512);
  let count = 0;
  for (let cluster = 2; cluster < 715 + 2; cluster += 1) if (getFatEntry(fat, cluster) === 0) count += 1;
  return count;
}

describe('在底盘镜像上添加文件', () => {
  const system = Buffer.alloc(5000, 0x53);

  it('保留原有文件，新文件内容可读，跨多个簇', () => {
    const base = makeBase({ files: [{ name: 'bbgdos.sys', data: system }] });
    const program = Buffer.alloc(9000, 0x41);
    const files = readFiles(overlayFat12Image(base, [{ name: 'hello.cmd', data: program }]));
    expect([...files.keys()]).toEqual(['BBGDOS.SYS', 'HELLO.CMD']);
    expect(files.get('BBGDOS.SYS')).toEqual(system);
    expect(files.get('HELLO.CMD')).toEqual(program);
  });

  it('不改传入的底盘，两份 FAT 一致', () => {
    const base = makeBase({ files: [{ name: 'bbgdos.sys', data: system }] });
    const copy = Buffer.from(base);
    const image = overlayFat12Image(base, [{ name: 'a.cmd', data: Buffer.from('x') }]);
    expect(base).toEqual(copy);
    expect(image.subarray(512, 512 + 3 * 512)).toEqual(image.subarray(512 + 3 * 512, 512 + 6 * 512));
  });

  it('同名文件被替换，旧簇被释放', () => {
    const base = makeBase({ files: [{ name: 'hello.cmd', data: Buffer.alloc(9000, 1) }] });
    const before = freeClusters(base);
    const image = overlayFat12Image(base, [{ name: 'HELLO.CMD', data: Buffer.alloc(100, 2) }]);
    const files = readFiles(image);
    expect([...files.keys()]).toEqual(['HELLO.CMD']);
    expect(files.get('HELLO.CMD')).toEqual(Buffer.alloc(100, 2));
    expect(freeClusters(image)).toBe(before + 5 - 1);
  });

  it('空文件只占目录项，不占簇', () => {
    const base = makeBase();
    const image = overlayFat12Image(base, [{ name: 'empty.dat', data: new Uint8Array(0) }]);
    expect(readFiles(image).get('EMPTY.DAT')).toEqual(Buffer.alloc(0));
    expect(freeClusters(image)).toBe(freeClusters(base));
  });

  it('空闲簇不连续时分散存放，内容不变', () => {
    const base = makeBase({ files: ['a.bin', 'b.bin', 'c.bin'].map((name) => ({ name, data: Buffer.alloc(2048, 3) })) });
    const withHole = overlayFat12Image(base, [{ name: 'b.bin', data: new Uint8Array(0) }]);
    const data = Buffer.alloc(4000, 7);
    const files = readFiles(overlayFat12Image(withHole, [{ name: 'd.bin', data }]));
    expect(files.get('D.BIN')).toEqual(data);
    expect(files.get('C.BIN')).toEqual(Buffer.alloc(2048, 3));
  });

  it('剩余空间不足时报 DISK_FULL 并提示换底盘', () => {
    const base = makeBase({ files: [{ name: 'big.bin', data: Buffer.alloc(714 * 2048) }] });
    expect(() => overlayFat12Image(base, [{ name: 'a.cmd', data: Buffer.alloc(4097) }])).toThrow(expect.objectContaining({ code: 'DISK_FULL', hint: expect.stringContaining('底盘') }));
  });

  it('根目录已满时报 DISK_FULL', () => {
    const base = makeBase({ rootEntries: 16, files: Array.from({ length: 16 }, (_, index) => ({ name: `f${index}.bin`, data: Buffer.from('x') })) });
    expect(() => overlayFat12Image(base, [{ name: 'a.cmd', data: Buffer.from('x') }])).toThrow(expect.objectContaining({ code: 'DISK_FULL' }));
  });

  it('底盘不是可用的 FAT12 镜像时报 DISK_BASE_INVALID', () => {
    expect(() => overlayFat12Image(Buffer.alloc(10), [])).toThrow(expect.objectContaining({ code: 'DISK_BASE_INVALID' }));
    expect(() => overlayFat12Image(Buffer.alloc(2880 * 512), [])).toThrow(expect.objectContaining({ code: 'DISK_BASE_INVALID' }));
    const truncated = makeBase().subarray(0, 1000 * 512);
    expect(() => overlayFat12Image(truncated, [])).toThrow(expect.objectContaining({ code: 'DISK_BASE_INVALID' }));
    const fat16 = makeBase({ sectorsPerCluster: 1, totalSectors: 20000 });
    expect(() => overlayFat12Image(fat16, [])).toThrow(expect.objectContaining({ code: 'DISK_BASE_INVALID' }));
  });

  it('文件名不符合 8.3 时报 DISK_NAME_INVALID', () => {
    expect(() => overlayFat12Image(makeBase(), [{ name: 'too-long-name.cmd', data: Buffer.from('x') }])).toThrow(expect.objectContaining({ code: 'DISK_NAME_INVALID' }));
  });
});
