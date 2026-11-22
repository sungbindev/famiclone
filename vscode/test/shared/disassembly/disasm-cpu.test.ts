// ------------------------------------------------------------------------
// 名称：disasm-cpu.test.ts
// 说明：测试 6502 指令表、解码与汇编文字格式化：表的完整性，解码结果，以及把全部官方指令的多种操作
//       数解码、格式化后用 ca65 重新汇编，字节必须与原来一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：汇编核对需要 cc65 工具链（没有时跳过）；源文件用 .org 进入绝对模式，分支才能写成目标地址。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { readFileSync, writeFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'vitest';
import { ADDRESSING_MODES, OPCODE_TABLE, findOpcode } from '../../../src/shared/disassembly/cpu/opcodes';
import { decode, type DecodedInstruction } from '../../../src/shared/disassembly/cpu/decode';
import { formatInstruction, isAssemblable, type FormatOptions } from '../../../src/shared/disassembly/cpu/format';
import { runTool, locateTestToolchain, HAS_TOOLCHAIN } from '../../support/disasm-fixtures';
import { createTempDir, type TempDir } from '../../support/build-fixtures';

/** 解码并断言成功 */
function decodeOk(bytes: Uint8Array, offset: number, address: number): DecodedInstruction {
  const result = decode(bytes, offset, address);
  assert.ok(result.ok);
  return result;
}

describe('指令表', () => {
  it('有 151 条官方指令，其余 105 个操作码为 null', () => {
    assert.equal(OPCODE_TABLE.length, 256);
    assert.equal(OPCODE_TABLE.filter((entry) => entry !== null).length, 151);
    assert.equal(OPCODE_TABLE.filter((entry) => entry === null).length, 105);
  });

  it('每一项的操作码、长度与寻址方式一致，助记符为小写', () => {
    OPCODE_TABLE.forEach((entry, opcode) => {
      if (entry === null) return;
      assert.equal(entry.opcode, opcode);
      assert.equal(entry.size, ADDRESSING_MODES[entry.mode].size);
      assert.equal(entry.mnemonic, entry.mnemonic.toLowerCase());
    });
  });

  it('流程类别：顺序、分支、调用、跳转、间接跳转、返回、停止', () => {
    assert.deepEqual([0xa9, 0x8d, 0xd0, 0x20, 0x4c, 0x6c, 0x60, 0x40, 0x00].map((opcode) => OPCODE_TABLE[opcode]!.flow), ['next', 'next', 'branch', 'call', 'jump', 'jump-indirect', 'return', 'return', 'stop']);
  });

  it('按助记符与寻址方式查操作码，组合不存在时返回 undefined', () => {
    assert.equal(findOpcode('LDA', 'immediate')!.opcode, 0xa9);
    assert.equal(findOpcode('jmp', 'indirect')!.opcode, 0x6c);
    assert.equal(findOpcode('ldx', 'zeroPageY')!.opcode, 0xb6);
    assert.equal(findOpcode('sta', 'immediate'), undefined);
  });
});

describe('解码', () => {
  it('立即数、零页、绝对：操作数与引用的地址', () => {
    assert.deepEqual(decodeOk(Uint8Array.of(0xa9, 0x42), 0, 0xc000), {
      ok: true, opcode: 0xa9, mnemonic: 'lda', mode: 'immediate', size: 2, flow: 'next', operand: 0x42, target: null, reference: null,
    });
    assert.deepEqual(decodeOk(Uint8Array.of(0xa5, 0x10), 0, 0xc000).reference, 0x10);
    const store = decodeOk(Uint8Array.of(0x00, 0x8d, 0x00, 0x20), 1, 0xc001);
    assert.equal(store.mnemonic, 'sta');
    assert.equal(store.operand, 0x2000);
    assert.equal(store.reference, 0x2000);
    assert.equal(store.target, null);
  });

  it('分支目标按符号扩展的偏移从下一条指令算起，在 16 位内回绕', () => {
    assert.equal(decodeOk(Uint8Array.of(0xd0, 0x05), 0, 0xc000).target, 0xc007);
    assert.equal(decodeOk(Uint8Array.of(0xd0, 0xfe), 0, 0xc000).target, 0xc000);
    assert.equal(decodeOk(Uint8Array.of(0xd0, 0x80), 0, 0xc000).target, 0xbf82);
    assert.equal(decodeOk(Uint8Array.of(0xf0, 0x7f), 0, 0xff00).target, 0xff81);
    assert.equal(decodeOk(Uint8Array.of(0x10, 0x10), 0, 0xfff0).target, 0x0002);
    assert.equal(decodeOk(Uint8Array.of(0xd0, 0x05), 0, 0xc000).reference, null);
  });

  it('JMP、JSR 的目标与引用都是操作数；间接 JMP 只有引用，没有目标', () => {
    const jump = decodeOk(Uint8Array.of(0x4c, 0x34, 0x12), 0, 0xc000);
    assert.deepEqual([jump.flow, jump.target, jump.reference], ['jump', 0x1234, 0x1234]);
    const call = decodeOk(Uint8Array.of(0x20, 0x00, 0x80), 0, 0xc000);
    assert.deepEqual([call.flow, call.target, call.reference], ['call', 0x8000, 0x8000]);
    const indirect = decodeOk(Uint8Array.of(0x6c, 0xfc, 0xff), 0, 0xc000);
    assert.deepEqual([indirect.flow, indirect.target, indirect.reference], ['jump-indirect', null, 0xfffc]);
  });

  it('隐含与累加器没有操作数', () => {
    assert.equal(decodeOk(Uint8Array.of(0x60), 0, 0xc000).operand, null);
    assert.equal(decodeOk(Uint8Array.of(0x0a), 0, 0xc000).mode, 'accumulator');
  });

  it('非官方操作码、被截断的指令、起点越界都返回失败原因而不抛错', () => {
    assert.deepEqual(decode(Uint8Array.of(0x02), 0, 0), { ok: false, reason: 'illegal', opcode: 0x02 });
    assert.deepEqual(decode(Uint8Array.of(0x4c, 0x00), 0, 0), { ok: false, reason: 'truncated', opcode: 0x4c, size: 3 });
    assert.deepEqual(decode(Uint8Array.of(0xea), 1, 0), { ok: false, reason: 'outOfRange' });
    assert.deepEqual(decode(Uint8Array.of(0xea), -1, 0), { ok: false, reason: 'outOfRange' });
  });
});

const text = (bytes: number[], address = 0xc000, options?: FormatOptions) => formatInstruction(decodeOk(Uint8Array.from(bytes), 0, address), options);

describe('汇编文字', () => {
  it('各寻址方式的写法', () => {
    assert.equal(text([0xea]), 'nop');
    assert.equal(text([0x0a]), 'asl a');
    assert.equal(text([0xa9, 0x01]), 'lda #$01');
    assert.equal(text([0xa5, 0x12]), 'lda $12');
    assert.equal(text([0xb5, 0x12]), 'lda $12,x');
    assert.equal(text([0xb6, 0x12]), 'ldx $12,y');
    assert.equal(text([0xad, 0x34, 0x12]), 'lda $1234');
    assert.equal(text([0xbd, 0x34, 0x12]), 'lda $1234,x');
    assert.equal(text([0xb9, 0x34, 0x12]), 'lda $1234,y');
    assert.equal(text([0x6c, 0x34, 0x12]), 'jmp ($1234)');
    assert.equal(text([0xa1, 0x12]), 'lda ($12,x)');
    assert.equal(text([0xb1, 0x12]), 'lda ($12),y');
    assert.equal(text([0xd0, 0x05]), 'bne $C007');
  });

  it('绝对寻址的地址小于 $100 时加 a: 前缀，零页与间接不加', () => {
    assert.equal(text([0xad, 0x12, 0x00]), 'lda a:$0012');
    assert.equal(text([0xbd, 0xff, 0x00]), 'lda a:$00FF,x');
    assert.equal(text([0x4c, 0x00, 0x00]), 'jmp a:$0000');
    assert.equal(text([0xad, 0x00, 0x01]), 'lda $0100');
    assert.equal(text([0x6c, 0x12, 0x00]), 'jmp ($0012)');
  });

  it('可以用符号表达式代替数值，a: 前缀规则不变', () => {
    assert.equal(text([0x8d, 0x00, 0x20], 0xc000, { expression: 'PPU_CTRL' }), 'sta PPU_CTRL');
    assert.equal(text([0x85, 0x10], 0xc000, { expression: 'counter' }), 'sta counter');
    assert.equal(text([0xad, 0x12, 0x00], 0xc000, { expression: 'flag' }), 'lda a:flag');
    assert.equal(text([0xb1, 0x10], 0xc000, { expression: 'ptr' }), 'lda (ptr),y');
    assert.equal(text([0xd0, 0x05], 0xc000, { expression: 'loop+2' }), 'bne loop+2');
  });

  it('间接 JMP 的指针低字节是 $FF 时 ca65 不接受，其余指令都可以汇编', () => {
    assert.equal(isAssemblable(decodeOk(Uint8Array.of(0x6c, 0xff, 0x12), 0, 0xc000)), false);
    assert.equal(isAssemblable(decodeOk(Uint8Array.of(0x6c, 0xfe, 0x12), 0, 0xc000)), true);
    assert.equal(isAssemblable(decodeOk(Uint8Array.of(0x4c, 0xff, 0x12), 0, 0xc000)), true);
  });
});

describe.skipIf(!HAS_TOOLCHAIN)('与 ca65 逐条核对', () => {
  let temp: TempDir;
  beforeEach(() => { temp = createTempDir(); });
  afterEach(() => temp.cleanup());

  /** 各种长度的操作数样本：边界值与常用值 */
  const OPERANDS: Record<number, number[]> = { 1: [0x00, 0x01, 0x12, 0x7f, 0x80, 0xff], 2: [0x0000, 0x0012, 0x00ff, 0x0100, 0x1234, 0x8000, 0xffff] };

  it('151 条指令各取多个操作数：解码、格式化、汇编后字节与原来一致', async () => {
    const base = 0xc000;
    const expected = [];
    const lines = ['.setcpu "6502"', '.segment "CODE"', `.org $${base.toString(16)}`];
    const origins = [];
    for (const entry of OPCODE_TABLE) {
      if (entry === null) continue;
      const samples = entry.size === 1 ? [null] : OPERANDS[entry.size - 1]!;
      for (const operand of samples) {
        const bytes = [entry.opcode];
        if (entry.size >= 2) bytes.push((operand ?? 0) & 0xff);
        if (entry.size === 3) bytes.push((operand ?? 0) >> 8);
        const address = base + expected.length;
        const decoded = decodeOk(Uint8Array.from(bytes), 0, address);
        assert.equal(decoded.ok, true);
        if (!isAssemblable(decoded)) continue;
        lines.push(formatInstruction(decoded));
        origins.push({ address, text: lines.at(-1) });
        expected.push(...bytes);
      }
    }
    const toolchain = locateTestToolchain();
    const source = path.join(temp.dir, 'all.s');
    const config = path.join(temp.dir, 'all.cfg');
    writeFileSync(source, `${lines.join('\n')}\n`);
    writeFileSync(config, `MEMORY { ROM: file = %O, start = $${base.toString(16)}, size = $${expected.length.toString(16)}; }\nSEGMENTS { CODE: load = ROM, type = ro; }\n`);
    const assembled = await runTool(toolchain, 'ca65', [source, '-o', path.join(temp.dir, 'all.o')]);
    assert.equal(assembled.code, 0, `${assembled.stdout}${assembled.stderr}`);
    const linked = await runTool(toolchain, 'ld65', ['-C', config, '-o', path.join(temp.dir, 'all.bin'), path.join(temp.dir, 'all.o')]);
    assert.equal(linked.code, 0, `${linked.stdout}${linked.stderr}`);
    const actual = readFileSync(path.join(temp.dir, 'all.bin'));

    assert.equal(actual.length, expected.length);
    const mismatches = [];
    let offset = 0;
    for (const { address, text: originText } of origins) {
      const size = OPCODE_TABLE[expected[offset]!]!.size;
      for (let index = 0; index < size; index += 1) {
        if (actual[offset + index] !== expected[offset + index]) {
          mismatches.push(`$${address.toString(16)} ${originText}：应为 ${expected.slice(offset, offset + size).map((value) => value.toString(16)).join(' ')}，实际 ${[...actual.subarray(offset, offset + size)].map((value) => value.toString(16)).join(' ')}`);
          break;
        }
      }
      offset += size;
    }
    assert.deepEqual(mismatches, []);
  });

  it('ca65 拒绝 jmp ($xxFF)，接受 jmp ($xxFE)', async () => {
    const toolchain = locateTestToolchain();
    const assemble = async (name: string, operand: string) => {
      const source = path.join(temp.dir, `${name}.s`);
      writeFileSync(source, `.setcpu "6502"\n.segment "CODE"\n.org $C000\njmp ($12${operand})\n`);
      return runTool(toolchain, 'ca65', [source, '-o', path.join(temp.dir, `${name}.o`)]);
    };
    assert.notEqual((await assemble('rejected', 'FF')).code, 0);
    assert.equal((await assemble('accepted', 'FE')).code, 0);
  });
});
