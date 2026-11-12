// ------------------------------------------------------------------------
// 名称：bbk-bios.test.mjs
// 说明：SB2000 版步步高 BIOS 的测试：移植规则命中次数、与提交的源码一致、汇编结果的大小与布局、bbk.bios 的文件格式。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：运行 npm test；读取 reference/ 下的原厂源码与 ROM。bbk.bios 在模拟器里能否启动到 A:\> 要用 npm run verify 另行验证。
// ------------------------------------------------------------------------

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { Gasm } from '../tools/gasm.mjs';
import { portBios } from '../tools/port-bios.mjs';
import { BBK_ROOT, BIOS_SOURCE, INCLUDE_DIR, INES_HEADER_SIZE, ORIGINAL_SOURCE_DIR, ROM_BIOS10 } from '../tools/config.mjs';

/** 汇编提交进仓库的 SB2000 版 BIOS */
function assembleBios() {
  return new Gasm({ includeDirs: [INCLUDE_DIR] }).assemble(BIOS_SOURCE);
}

test('移植规则全部命中预期次数', () => {
  const { report } = portBios(readFileSync(join(ORIGINAL_SOURCE_DIR, 'Bios.asm'), 'latin1'));
  const bad = report.filter((item) => item.count !== item.expected).map((item) => `${item.label} ${item.count}/${item.expected}`);
  assert.deepEqual(bad, []);
});

test('src/bios/Bios.asm 与移植规则的生成结果一致', () => {
  const { text } = portBios(readFileSync(join(ORIGINAL_SOURCE_DIR, 'Bios.asm'), 'latin1'));
  assert.ok(readFileSync(BIOS_SOURCE).equals(Buffer.from(text, 'latin1')), 'Bios.asm 被手改过：同步修改 tools/port-bios.mjs 的规则，或确认后用 --force 重新生成');
});

test('源码里不再直接访问 NES PPU 数据口与步步高的 $FF00 区寄存器', () => {
  const source = readFileSync(BIOS_SOURCE, 'latin1');
  for (const pattern of [/sta\s+\$2007/, /lda\s+\$2007/, /[sx]t[axy]\s+\$2006/, /sta\s+\$2008/, /sta\s+VideoCtrlPort/, /sta\s+DRAMPagePort(?![A-Za-z])/]) {
    assert.doesNotMatch(source, pattern);
  }
});

test('汇编后没有未定义符号，代码在 $C000-$FFFF 之内', () => {
  const result = assembleBios();
  assert.equal(result.undefinedSymbols.size, 0);
  assert.deepEqual(result.missingIncludes, []);
  assert.equal(result.unknownOps.size, 0);
  const addresses = [...result.memory.keys()];
  assert.ok(Math.min(...addresses) >= 0xc000 && Math.max(...addresses) <= 0xffff);
});

test('临时部分（复制到 $5800 的那段）在 $5C00 之前结束，HAL 变量在 ERAM 保留区里', () => {
  const { symbols } = assembleBios();
  const ramEnd = symbols.get('moveramending') - 0xc000 + 0x5800;
  assert.ok(ramEnd <= 0x5c00, `临时部分结束于 $${ramEnd.toString(16)}`);
  assert.ok(symbols.get('halvars') >= 0x6010 && symbols.get('halvarsend') <= 0x6100);
});

test('向量表：NMI、复位、IRQ', () => {
  const { memory, symbols } = assembleBios();
  const word = (address) => memory.get(address) | (memory.get(address + 1) << 8);
  assert.equal(word(0xfffa), symbols.get('nmientry') - 0xc000 + 0x5800);
  assert.equal(word(0xfffc), symbols.get('begin'));
  assert.equal(word(0xfffe), symbols.get('brkintentry') - 0xc000 + 0x5800);
});

test('bbk.bios：文件头、字库页取自原版 ROM、最后一页是汇编结果', () => {
  const dir = mkdtempSync(join(tmpdir(), 'bbk-bios-'));
  try {
    const output = join(dir, 'bbk.bios');
    execFileSync(process.execPath, [join(BBK_ROOT, 'tools', 'build-bios.mjs'), output], { stdio: 'pipe' });
    const image = readFileSync(output);
    const result = assembleBios();
    assert.equal(image.length, 16 + 8 * 0x4000);
    assert.equal(image.subarray(0, 4).toString('latin1'), 'BBKB');
    assert.equal(image[4], 1);
    assert.equal(image[5], 8);
    assert.equal(image.readUInt16LE(6), result.symbols.get('begin'));
    assert.ok(image.subarray(8, 16).every((byte) => byte === 0));
    const rom = readFileSync(ROM_BIOS10);
    assert.ok(image.subarray(16, 16 + 7 * 0x4000).equals(rom.subarray(INES_HEADER_SIZE, INES_HEADER_SIZE + 7 * 0x4000)));
    const code = image.subarray(16 + 7 * 0x4000);
    for (const [address, byte] of result.memory) assert.equal(code[address - 0xc000], byte);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});
