// ------------------------------------------------------------------------
// 名称：build-nes.test.mjs
// 说明：用 Bios.asm 构建 ROM 映像的测试：大小、文件头、前 7 页、最后一页与复位向量。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：不启动模拟器；启动到命令提示符的验证用 npm run boot。
// ------------------------------------------------------------------------

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { Gasm } from '../tools/gasm.mjs';
import { INCLUDE_DIR, ORIGINAL_SOURCE_DIR, ROM_BIOS10 } from '../tools/config.mjs';

const BANK_SIZE = 0x4000;
const script = fileURLToPath(new URL('../tools/build-nes.mjs', import.meta.url));

test('构建出的 ROM 与原版同大小、同文件头、前 7 页相同，复位向量指向 Begin', () => {
  const dir = mkdtempSync(join(tmpdir(), 'bbk-nes-'));
  try {
    const output = join(dir, 'out.nes');
    execFileSync(process.execPath, [script, output], { stdio: 'pipe' });
    const built = readFileSync(output);
    const original = readFileSync(ROM_BIOS10);
    assert.equal(built.length, original.length);
    assert.deepEqual(built.subarray(0, 16), original.subarray(0, 16));
    assert.deepEqual(built.subarray(16, built.length - BANK_SIZE), original.subarray(16, original.length - BANK_SIZE));

    const lastBank = built.subarray(built.length - BANK_SIZE);
    const symbols = new Gasm({ includeDirs: [INCLUDE_DIR] }).assemble(join(ORIGINAL_SOURCE_DIR, 'Bios.asm')).symbols;
    assert.equal(lastBank.readUInt16LE(0x3ffc), symbols.get('begin'));
    assert.deepEqual([...lastBank.subarray(0, 3)], [0x4c, 0x67, 0x58]);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});
