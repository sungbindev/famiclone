// ------------------------------------------------------------------------
// 名称：fat12-add.test.mjs
// 说明：fat12-add.mjs 的单元测试：按步步高 DOS 盘的布局（每簇 4 扇区、根目录 208 项）追加文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：运行 npm test。
// ------------------------------------------------------------------------

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { addFat12Files } from '../tools/fat12-add.mjs';
import { DOS_FLOPPY } from '../tools/config.mjs';

/** 做一张空的 1.44MB 软盘：每簇 4 扇区、2 份 FAT 各 3 扇区、根目录 208 项 */
function emptyDisk() {
  const image = Buffer.alloc(1474560);
  image.writeUInt16LE(512, 11);
  image[13] = 4;
  image.writeUInt16LE(1, 14);
  image[16] = 2;
  image.writeUInt16LE(208, 17);
  image.writeUInt16LE(2880, 19);
  image.writeUInt16LE(3, 22);
  for (const fat of [512, 512 + 3 * 512]) image.set([0xf0, 0xff, 0xff], fat);
  return image;
}

/** 按目录项读回文件内容 */
function readBack(image, name) {
  const rootStart = (1 + 2 * 3) * 512;
  const dataStart = rootStart + 13 * 512;
  for (let index = 0; index < 208; index += 1) {
    const entry = rootStart + index * 32;
    if (image.subarray(entry, entry + 11).toString('latin1') !== name) continue;
    const size = image.readUInt32LE(entry + 28);
    const chunks = [];
    let cluster = image.readUInt16LE(entry + 26);
    while (cluster >= 2 && cluster < 0xff8) {
      const at = dataStart + (cluster - 2) * 2048;
      chunks.push(image.subarray(at, at + 2048));
      const fatAt = 512 + ((cluster * 3) >> 1);
      const value = image.readUInt16LE(fatAt);
      cluster = cluster & 1 ? value >> 4 : value & 0xfff;
    }
    return Buffer.concat(chunks).subarray(0, size);
  }
  return null;
}

test('追加的文件能按簇链读回，两份 FAT 一致', () => {
  const small = Buffer.from('hello');
  const large = Buffer.alloc(5000, 0x5a);
  const image = addFat12Files(emptyDisk(), [{ name: 'a.txt', data: small }, { name: 'b.bin', data: large }]);
  assert.deepEqual(readBack(image, 'A       TXT'), small);
  assert.deepEqual(readBack(image, 'B       BIN'), large);
  assert.ok(image.subarray(512, 512 + 1536).equals(image.subarray(512 + 1536, 512 + 3072)));
});

test('不改动原镜像对象，也不动引导扇区', () => {
  const source = emptyDisk();
  source[0] = 0xeb;
  const copy = Buffer.from(source);
  const image = addFat12Files(source, [{ name: 'x.y', data: Buffer.from('1') }]);
  assert.ok(source.equals(copy));
  assert.equal(image[0], 0xeb);
});

test('文件名必须是 8.3', () => {
  assert.throws(() => addFat12Files(emptyDisk(), [{ name: 'toolongname.txt', data: Buffer.from('1') }]), /8\.3/);
});

test('步步高 DOS 启动盘追加文件后原有文件仍在', () => {
  const dos = readFileSync(DOS_FLOPPY);
  const image = addFat12Files(dos, [{ name: 'new.dat', data: Buffer.from('data') }]);
  assert.deepEqual(readBack(image, 'NEW     DAT'), Buffer.from('data'));
  assert.ok(image.subarray(0, 512).equals(dos.subarray(0, 512)));
  const original = readBack(dos, 'BBGDOS  SYS');
  assert.ok(original && original.length > 0);
  assert.deepEqual(readBack(image, 'BBGDOS  SYS'), original);
});
