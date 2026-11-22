// ------------------------------------------------------------------------
// 名称：fat12-image.test.ts
// 说明：测试 FAT12 软盘镜像：大小与引导扇区、文件读回、多簇文件、卷标、8.3 名字校验、空间不足。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：无
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { GEOMETRY, buildFat12Image, listFat12, readFat12File, toShortName } from '../../../src/shared/packaging/fat12-image';

describe('FAT12 镜像', () => {
  it('镜像大小是 1.44MB，引导扇区带 55 AA 签名与几何参数', () => {
    const image = buildFat12Image([]);
    expect(image.length).toBe(1474560);
    expect(image.readUInt16LE(510)).toBe(0xaa55);
    expect(image.readUInt16LE(11)).toBe(GEOMETRY.bytesPerSector);
    expect(image.readUInt16LE(19)).toBe(GEOMETRY.totalSectors);
  });

  it('写入的文件能读回，内容逐字节一致', () => {
    const data = Buffer.from('Hello, FAT12!');
    const image = buildFat12Image([{ name: 'hello.txt', data }]);
    const [entry] = listFat12(image);
    expect(entry?.name).toBe('HELLO.TXT');
    expect(entry?.size).toBe(data.length);
    expect(readFat12File(image, entry!)).toEqual(data);
  });

  it('跨多个簇的文件按簇链读回', () => {
    const data = Buffer.alloc(5000);
    data.forEach((_, index) => {
      data[index] = (index * 7) & 0xff;
    });
    const image = buildFat12Image([
      { name: 'a.bin', data },
      { name: 'b.bin', data: Buffer.from('tail') }
    ]);
    const entries = listFat12(image);
    expect(entries.map((entry) => entry.name)).toEqual(['A.BIN', 'B.BIN']);
    expect(readFat12File(image, entries[0]!)).toEqual(data);
    expect(readFat12File(image, entries[1]!)).toEqual(Buffer.from('tail'));
  });

  it('空文件只有目录项，没有数据', () => {
    const image = buildFat12Image([{ name: 'empty.dat', data: Buffer.alloc(0) }]);
    const [entry] = listFat12(image);
    expect(entry?.size).toBe(0);
    expect(readFat12File(image, entry!).length).toBe(0);
  });

  it('卷标写进引导扇区与根目录，并且不会出现在文件列表里', () => {
    const image = buildFat12Image([{ name: 'a.txt', data: Buffer.from('x') }], { label: 'fcdev' });
    expect(image.toString('ascii', 43, 54)).toBe('FCDEV      ');
    expect(listFat12(image).map((entry) => entry.name)).toEqual(['A.TXT']);
  });

  it('文件名不符合 8.3 时报错', () => {
    expect(toShortName('hello.exe')).toBe('HELLO   EXE');
    for (const name of ['toolongname.exe', 'a.html', 'a b.exe']) {
      expect(() => toShortName(name)).toThrow(expect.objectContaining({ code: 'DISK_NAME_INVALID' }));
    }
  });

  it('放不下时报错', () => {
    expect(() => buildFat12Image([{ name: 'big.bin', data: Buffer.alloc(1_500_000) }])).toThrow(expect.objectContaining({ code: 'DISK_FULL' }));
  });
});
