// ------------------------------------------------------------------------
// 名称：sb97-rom.test.mjs
// 说明：SB97 BIOS 文件的读取、校验与地址视图的测试。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：用到真实 BIOS（ember/bios/sb97_bios.bin）的用例在文件不存在时跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { test } from 'node:test';
import { bankView, fixedView, windowView } from '../tools/sb97-memory.mjs';
import { DEFAULT_ROM_PATH, PAGE_COUNT, PAGE_SIZE, PRG_CRC32, crc32, loadRom, pageBytes } from '../tools/sb97-rom.mjs';

const hasRom = existsSync(DEFAULT_ROM_PATH);

test('crc32 与标准测试向量一致', () => {
  assert.equal(crc32(new TextEncoder().encode('123456789')), 0xcbf43926);
});

test('pageBytes 检查页号范围', () => {
  assert.throws(() => pageBytes({ prg: new Uint8Array(0) }, 64), RangeError);
  assert.throws(() => pageBytes({ prg: new Uint8Array(0) }, -1), RangeError);
});

test('真实 BIOS：程序区 1MB、64 页、校验和是 SB97 的 BIOS', { skip: !hasRom }, () => {
  const rom = loadRom();
  assert.equal(rom.prg.length, PAGE_COUNT * PAGE_SIZE);
  assert.equal(rom.crc, PRG_CRC32);
});

test('真实 BIOS：改动一个字节后校验和不对，被拒绝', { skip: !hasRom }, () => {
  const directory = mkdtempSync(join(tmpdir(), 'sb97-'));
  try {
    const bytes = new Uint8Array(readFileSync(DEFAULT_ROM_PATH));
    bytes[100] ^= 0xff;
    const path = join(directory, 'changed.bin');
    writeFileSync(path, bytes);
    assert.throws(() => loadRom(path), /CRC32/);
  } finally {
    rmSync(directory, { recursive: true, force: true });
  }
});

test('16K 窗口视图：$8000 是窗口页，$C000 固定是第 0 页', { skip: !hasRom }, () => {
  const rom = loadRom();
  const view = windowView(rom, 2);
  assert.equal(view.read(0x8000), pageBytes(rom, 2)[0]);
  assert.equal(view.read(0xc000), pageBytes(rom, 0)[0]);
  assert.deepEqual(view.origin(0xbfff), { page: 2, offset: 0x3fff });
  assert.deepEqual(view.origin(0xc010), { page: 0, offset: 0x10 });
  assert.equal(view.has(0x7fff), false);
});

test('32K 视图：$8000-$FFFF 是第 2n、2n+1 页；只有固定页的视图低半窗口没有映射', { skip: !hasRom }, () => {
  const rom = loadRom();
  const bank = bankView(rom, 2);
  assert.equal(bank.read(0x8000), pageBytes(rom, 4)[0]);
  assert.equal(bank.read(0xc000), pageBytes(rom, 5)[0]);
  const fixed = fixedView(rom);
  assert.equal(fixed.has(0x8000), false);
  assert.equal(fixed.has(0xc000), true);
  assert.throws(() => bankView(rom, 32), RangeError);
});
