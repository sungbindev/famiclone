// ------------------------------------------------------------------------
// 名称：gasm.test.mjs
// 说明：GABBG 汇编器的单元测试，以及对原厂源码的汇编集成测试。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：运行 npm test；集成测试读取 reference/docs/BBGDOS源代码 里的原厂文件。
// ------------------------------------------------------------------------

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, writeFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { Gasm, GasmError } from '../tools/gasm.mjs';
import { BIOS_SOURCE, INCLUDE_DIR, resolveSource } from '../tools/config.mjs';

/**
 * 汇编一段源码并取出字节。
 * @param {string} source 源码
 * @param {Record<string, string>} [extraFiles] 同目录的其他文件
 * @returns {{bytes: number[], result: object}} 按地址排序的字节与汇编结果
 */
function build(source, extraFiles = {}) {
  const dir = mkdtempSync(join(tmpdir(), 'gasm-'));
  try {
    for (const [name, text] of Object.entries(extraFiles)) writeFileSync(join(dir, name), text, 'latin1');
    writeFileSync(join(dir, 'main.asm'), source, 'latin1');
    const result = new Gasm().assemble(join(dir, 'main.asm'));
    const bytes = [...result.memory.entries()].sort((a, b) => a[0] - b[0]).map(([, byte]) => byte);
    return { bytes, result };
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}

test('立即数的 < 取高字节、> 取低字节', () => {
  const { bytes } = build(' org $8000\n lda #<$1234\n lda #>$1234\n');
  assert.deepEqual(bytes, [0xa9, 0x12, 0xa9, 0x34]);
});

test('开头的 < 作用于整个表达式', () => {
  const { bytes } = build(' org $8000\n lda #<$1234-$1000+$5800\n');
  assert.deepEqual(bytes, [0xa9, 0x58 + 0x02]);
});

test('表达式没有优先级，从左到右计算', () => {
  const { bytes } = build(' org $8000\n lda #2+3*4\n lda #2+(3*4)\n');
  assert.deepEqual(bytes, [0xa9, 20, 0xa9, 14]);
});

test('AND、OR、XOR 为运算符，大小写无关', () => {
  const { bytes } = build(' ORG $8000\n LDA #$F0 and $3C\n lda #$01 Or $02\n lda #$ff xor $0f\n');
  assert.deepEqual(bytes, [0xa9, 0x30, 0xa9, 0x03, 0xa9, 0xf0]);
});

test('双字符的字符常量按国标码取值', () => {
  const { bytes } = build(" org $8000\n ldx #<'\xb5\xc4'\n ldy #>'\xb5\xc4'\n");
  assert.deepEqual(bytes, [0xa2, 0xb5, 0xa0, 0xc4]);
});

test('前向引用的操作数按绝对寻址，已知的小值按零页', () => {
  const { bytes } = build(' org $8000\nzp equ $20\n lda later\n lda zp\nlater equ $10\n');
  assert.deepEqual(bytes, [0xad, 0x10, 0x00, 0xa5, 0x20]);
});

test('分支偏移与前向引用', () => {
  const { bytes } = build(' org $8000\n bne next\n nop\nnext: rts\n');
  assert.deepEqual(bytes, [0xd0, 0x01, 0xea, 0x60]);
});

test('标号后可直接跟指令，名字与助记符同名时跟 equ 仍是名字', () => {
  const { bytes, result } = build(' org $8000\nloop:dex\n bne loop\nCommand equ $00\n lda Command\n');
  assert.deepEqual(bytes, [0xca, 0xd0, 0xfd, 0xa5, 0x00]);
  assert.equal(result.symbols.get('command'), 0);
});

test('符号不区分大小写', () => {
  const { bytes } = build(' org $8000\nFoo equ $1234\n jmp FOO\n jmp foo\n');
  assert.deepEqual(bytes, [0x4c, 0x34, 0x12, 0x4c, 0x34, 0x12]);
});

test('db 的字符串、字节与 dw、ds', () => {
  const { bytes } = build(' org $8000\n db "AB",$0d,1+2\n dw $1234,here\nhere ds 2\n');
  assert.deepEqual(bytes, [0x41, 0x42, 0x0d, 3, 0x34, 0x12, 0x08, 0x80, 0, 0]);
});

test('@ 表示当前地址', () => {
  const { result } = build(' org $8000\nstart db 1,2,3\nlen equ @-start\n');
  assert.equal(result.symbols.get('len'), 3);
});

test('org 之后空隙不产生字节，位置按 org 计', () => {
  const { result } = build(' org $8000\n nop\n org $8010\nlab nop\n');
  assert.equal(result.symbols.get('lab'), 0x8010);
  assert.equal(result.memory.has(0x8005), false);
});

test('orgrun 只改运行地址，字节仍顺序存放；orgreal 恢复', () => {
  const { bytes, result } = build(' org $8000\n nop\n orgrun $6100\nrun jmp run\n orgreal\nafter nop\n');
  assert.deepEqual(bytes, [0xea, 0x4c, 0x00, 0x61, 0xea]);
  assert.equal(result.symbols.get('run'), 0x6100);
  assert.equal(result.symbols.get('after'), 0x8004);
});

test('comment 块与分号注释被跳过', () => {
  const { bytes } = build(' org $8000\n nop ; 注释\n comment %\n lda #1\n lda #2\n %\n rts\n');
  assert.deepEqual(bytes, [0xea, 0x60]);
});

test('include 在源文件目录与附加目录里查找；缺失时登记', () => {
  const found = build(' org $8000\n include defs.asm\n lda #val\n', { 'defs.asm': 'val equ 7\n' });
  assert.deepEqual(found.bytes, [0xa9, 7]);
  const missing = build(' org $8000\n include none.asm\n nop\n');
  assert.deepEqual(missing.result.missingIncludes.map((item) => item.name), ['none.asm']);
});

test('未定义符号与不认识的宏被登记而不是报错', () => {
  const { result } = build(' org $8000\n lda #missing\n somemacro 1,2\n');
  assert.equal(result.undefinedSymbols.has('missing'), true);
  assert.equal(result.unknownOps.has('SOMEMACRO'), true);
});

test('分支超出范围报错并带行号', () => {
  const source = ` org $8000\n bne far\n${' nop\n'.repeat(200)}far nop\n`;
  assert.throws(() => build(source), (error) => error instanceof GasmError && error.line === 2);
});

test('原厂三个源文件汇编后没有未定义符号、宏与重复定义', () => {
  for (const name of ['Bios.asm', 'Command.asm', 'Smcdos.asm']) {
    const result = new Gasm({ includeDirs: [INCLUDE_DIR] }).assemble(resolveSource(name));
    assert.equal(result.missingIncludes.length, 0, `${name} 缺少包含文件`);
    assert.equal(result.undefinedSymbols.size, 0, `${name} 有未定义符号：${[...result.undefinedSymbols.keys()].join(',')}`);
    assert.equal(result.unknownOps.size, 0, `${name} 有不认识的宏`);
    assert.equal(result.duplicates.size, 0, `${name} 有重复定义`);
  }
});

test('Bios.asm 的入口跳转表与原厂 ROM 一致', () => {
  const result = new Gasm({ includeDirs: [INCLUDE_DIR] }).assemble(BIOS_SOURCE);
  const head = [...Array(9).keys()].map((i) => result.memory.get(0xc000 + i));
  assert.deepEqual(head, [0x4c, 0x67, 0x58, 0x4c, 0x8e, 0x58, 0x4c, 0x92, 0x58]);
});
