// ------------------------------------------------------------------------
// 名称：fat12-image.test.mjs
// 说明：测试 FAT12 软盘镜像：大小与引导扇区、文件读回、多簇文件、卷标、8.3 名字校验、空间不足。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { describe, it } from 'node:test';
import { GEOMETRY, buildFat12Image, listFat12, readFat12File, toShortName } from '../src/packagers/fat12-image.mjs';

describe('FAT12 镜像', () => {
  it('镜像大小是 1.44MB，引导扇区带 55 AA 签名与几何参数', () => {
    const image = buildFat12Image([]);
    assert.equal(image.length, 1474560);
    assert.equal(image.readUInt16LE(510), 0xaa55);
    assert.equal(image.readUInt16LE(11), GEOMETRY.bytesPerSector);
    assert.equal(image.readUInt16LE(19), GEOMETRY.totalSectors);
  });

  it('写入的文件能读回，内容逐字节一致', () => {
    const data = Buffer.from('Hello, FAT12!');
    const image = buildFat12Image([{ name: 'hello.txt', data }]);
    const [entry] = listFat12(image);
    assert.equal(entry.name, 'HELLO.TXT');
    assert.equal(entry.size, data.length);
    assert.deepEqual(readFat12File(image, entry), data);
  });

  it('跨多个簇的文件按簇链读回', () => {
    const data = Buffer.alloc(5000);
    data.forEach((_, index) => {
      data[index] = (index * 7) & 0xff;
    });
    const image = buildFat12Image([{ name: 'a.bin', data }, { name: 'b.bin', data: Buffer.from('tail') }]);
    const entries = listFat12(image);
    assert.deepEqual(entries.map((entry) => entry.name), ['A.BIN', 'B.BIN']);
    assert.deepEqual(readFat12File(image, entries[0]), data);
    assert.deepEqual(readFat12File(image, entries[1]), Buffer.from('tail'));
  });

  it('空文件只有目录项，没有数据', () => {
    const image = buildFat12Image([{ name: 'empty.dat', data: Buffer.alloc(0) }]);
    const [entry] = listFat12(image);
    assert.equal(entry.size, 0);
    assert.equal(readFat12File(image, entry).length, 0);
  });

  it('卷标写进引导扇区与根目录，并且不会出现在文件列表里', () => {
    const image = buildFat12Image([{ name: 'a.txt', data: Buffer.from('x') }], { label: 'fcdev' });
    assert.equal(image.toString('ascii', 43, 54), 'FCDEV      ');
    assert.deepEqual(listFat12(image).map((entry) => entry.name), ['A.TXT']);
  });

  it('文件名不符合 8.3 时报错', () => {
    assert.equal(toShortName('hello.exe'), 'HELLO   EXE');
    assert.throws(() => toShortName('toolongname.exe'), (error) => error.code === 'DISK_NAME_INVALID');
    assert.throws(() => toShortName('a.html'), (error) => error.code === 'DISK_NAME_INVALID');
    assert.throws(() => toShortName('a b.exe'), (error) => error.code === 'DISK_NAME_INVALID');
  });

  it('放不下时报错', () => {
    assert.throws(() => buildFat12Image([{ name: 'big.bin', data: Buffer.alloc(1_500_000) }]), (error) => error.code === 'DISK_FULL');
  });
});
