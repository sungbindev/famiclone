// ------------------------------------------------------------------------
// 名称：disasm-generate.test.ts
// 说明：测试源码生成与回读：生成的文字、符号与注释的输出位置、写不成文字时退回 .byte；用 ca65 与
//       ld65 回读，全部 151 条指令、随机字节序列、换页布局、fcdev 构建的真实程序都必须与映像逐字节
//       一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：回读需要 cc65 工具链（没有时跳过）；随机用例用固定种子，失败可复现。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'vitest';
import type { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { OPCODE_TABLE } from '../../../src/shared/disassembly/cpu/opcodes';
import { applyEdits, type EditItem } from '../../../src/shared/disassembly/model/edits';
import { buildStatus } from '../../../src/shared/disassembly/verify/status';
import { createAddressSpace } from '../../../src/shared/disassembly/analysis/address-space';
import { createLayout } from '../../../src/shared/disassembly/model/layout';
import { createProject, isVerificationStale, type Project } from '../../../src/shared/disassembly/model/project';
import { ensureAutoSymbols } from '../../../src/shared/disassembly/analysis/auto-symbols';
import { entriesFromVectors, entryAtAddress } from '../../../src/shared/disassembly/analysis/entry-points';
import { generateProject } from '../../../src/shared/disassembly/generate/index';
import { indexByValue, parseSymbolSet } from '../../../src/shared/disassembly/profile/symbol-set';
import { parseFlat } from '../../../src/shared/disassembly/formats/flat';
import { parseInes } from '../../../src/shared/disassembly/formats/ines';
import { recordVerification, verifyRoundtrip } from '../../../src/logic/disassembly/verify/roundtrip';
import { traceProject } from '../../../src/shared/disassembly/analysis/project-trace';
import { buildPath, BUILTIN_TARGETS_DIR, locateTestToolchain, HAS_TOOLCHAIN } from '../../support/disasm-fixtures';
import { createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { ProcessToolRunner } from '../../../src/infra/toolchain/process-tool-runner';

const files = new NodeFileSystem();
const tools = new ProcessToolRunner();

/** 平铺映像的工程：整个文件一个段，从给定地址开始，入口为 entries */
function flatProject(bytes: Uint8Array | number[], { address = 0xc000, entries = [] as number[] } = {}) {
  const buffer = Buffer.from(bytes);
  const parsed = parseFlat(buffer, { address });
  const space = createAddressSpace(parsed.layout);
  const project = createProject({
    imagePath: 'image.bin', bytes: buffer, format: 'flat', formatOptions: { address }, layout: parsed.layout,
    entries: entries.map((entry) => entryAtAddress(space, entry, { source: 'manual' })),
  });
  return { project, bytes: buffer };
}

/** 追踪、自动命名后生成 */
function build(project: Project, bytes: Buffer, symbolSets = new Map<string, string>()) {
  const traced = traceProject(project, bytes).project;
  const constants = [...symbolSets.values()].flatMap((text) => parseSymbolSet(text));
  const named = ensureAutoSymbols(traced, bytes, { externalNames: indexByValue(constants) }).project;
  return { project: named, generated: generateProject(named, bytes, { symbolSets }) };
}

describe('生成的源码（不需要工具链）', () => {
  it('指令用符号名，a: 强制绝对寻址，自动命名按调用、跳转、数据区分，入口叫 sub_', () => {
    const { project, bytes } = flatProject([
      0xa9, 0x01,       // C000 LDA #$01
      0x8d, 0x00, 0x20, // C002 STA $2000
      0xad, 0x10, 0x00, // C005 LDA $0010（绝对寻址，地址小于 $100）
      0x20, 0x10, 0xc0, // C008 JSR $C010
      0xd0, 0xf5,       // C00B BNE $C002
      0x4c, 0x0d, 0xc0, // C00D JMP $C00D
      0x60,             // C010 RTS
    ], { entries: [0xc000] });
    const { generated } = build(project, bytes);
    const source = generated.files.get('src/main.s')!;
    assert.ok(source.includes('.segment "SEG0_0000"\n; 段 0，$C000-$C010\nsub_C000:\n  lda #$01\nloc_C002:\n  sta var_2000\n  lda a:var_0010\n  jsr sub_C010\n  bne loc_C002\nloc_C00D:\n  jmp loc_C00D\n\nsub_C010:\n  rts\n'));
    assert.ok(generated.files.get('include/constants.inc')!.includes('var_2000 = $2000\n'));
    assert.ok(generated.files.get('include/constants.inc')!.includes('var_0010 = $0010\n'));
    assert.match(generated.files.get('include/globals.inc')!, /\.global sub_C000, loc_C002, loc_C00D, sub_C010/);
    assert.deepEqual(generated.warnings, []);
  });

  it('符号集里有同值的常量时用它的名字，不再生成 var_', () => {
    const { project, bytes } = flatProject([0xad, 0x02, 0x20, 0x60], { entries: [0xc000] });
    const symbolSets = new Map([['hw.inc', 'PPUSTATUS = $2002 ; 状态\nPPUCTRL   = $2000\n']]);
    const { generated, project: named } = build({ ...project, symbolSets: [{ name: 'hw.inc', path: 'hw.inc' }] }, bytes, symbolSets);
    assert.ok(generated.files.get('src/main.s')!.includes('  lda PPUSTATUS\n'));
    assert.equal(named.symbols.some((symbol) => symbol.name.startsWith('var_')), false);
    assert.match(generated.files.get('include/constants.inc')!, /\.include "hw\.inc"/);
    assert.equal(generated.files.get('include/hw.inc')!, symbolSets.get('hw.inc'));
  });

  it('符号与注释：说明、块注释、行尾注释写在对应的地址上，写不进去的注释记入警告', () => {
    const { project, bytes } = flatProject([0xa9, 0x01, 0xea, 0x4c, 0x00, 0xc0], { entries: [0xc000] });
    const edited = applyEdits(build(project, bytes).project, [
      { op: 'renameSymbol', from: 'sub_C000', to: 'main' },
      { op: 'updateSymbol', name: 'main', description: '主循环' },
      { op: 'setComment', address: 0xc000, kind: 'block', text: '程序入口\n第二行' },
      { op: 'setComment', address: 0xc000, kind: 'inline', text: '清零' },
      { op: 'setComment', address: 0xc001, kind: 'inline', text: '指令中间' },
    ]).project;
    const generated = generateProject(edited, bytes);
    assert.ok(generated.files.get('src/main.s')!.includes('$C005\n; 程序入口\n; 第二行\n; 主循环\nmain:\n  lda #$01  ; 清零\n  nop\n  jmp main\n'));
    assert.deepEqual(generated.warnings.map((warning) => warning.code), ['comment-inside']);
  });

  it('不能写成文字的指令退回 .byte 并在行尾写出反汇编文字：jmp ($xxFF)', () => {
    const { project, bytes } = flatProject([0x6c, 0xff, 0x12, 0xd0, 0x00, 0x60], { entries: [0xc000] });
    const { generated, project: named } = build(project, bytes);
    assert.ok(generated.files.get('src/main.s')!.includes('  .byte $6C, $FF, $12  ; jmp ($12FF)\n'));
    assert.equal(named.symbols.some((symbol) => symbol.name.startsWith('var_')), false);
  });

  it('数据：长重复写成 .res，字符串用引号，RTS 表写 符号 - 1', () => {
    const image = [0x60, ...Array.from({ length: 20 }, () => 0xff), ...Buffer.from('HI'), 0x22, 0x58, 0x00, 0xff, 0xbf, 0x14, 0xc0];
    const { project, bytes } = flatProject(image, { entries: [0xc000] });
    const edited = applyEdits(project, [
      { op: 'setRegion', address: 0xc001, length: 20, type: 'padding', params: { value: 0xff } },
      { op: 'setRegion', address: 0xc015, length: 5, type: 'string', params: { encoding: 'ascii', terminator: 0 } },
      { op: 'setRegion', address: 0xc01a, length: 4, type: 'words', params: { adjust: 1 } },
    ]).project;
    const source = build(edited, bytes).generated.files.get('src/main.s')!;
    assert.ok(source.includes('sub_C000:\n  rts\n  .res 20, $FF\nloc_C015:\n  .byte "HI", $22, "X", $00\n  .word sub_C000 - 1, loc_C015 - 1\n'));
  });

  it('符号落在指令中间：写 名字 = * + 偏移；目标在指令中间时写 符号加偏移', () => {
    const { project, bytes } = flatProject([0xa9, 0x00, 0x8d, 0x34, 0x12, 0x4c, 0x01, 0xc0], { entries: [0xc000] });
    const named = build(project, bytes).project;
    assert.ok(generateProject(named, bytes).files.get('src/main.s')!.includes('  jmp sub_C000+1\n'));
    const edited = applyEdits(named, [{ op: 'defineSymbol', name: 'smc', address: 0xc001, kind: 'variable' }]).project;
    const source = generateProject(edited, bytes).files.get('src/main.s')!;
    assert.ok(source.includes('sub_C000:\nsmc = * + 1\n  lda #$00\n'));
    assert.ok(source.includes('  jmp smc\n'));
  });

  it('操作数提示：低字节、高字节、字符、二进制；与字节对不上的提示不采用并留下警告', () => {
    const { project, bytes } = flatProject([0xa9, 0x05, 0xa2, 0xc0, 0xa0, 0x41, 0xa9, 0x0a, 0xa9, 0x99, 0x60], { entries: [0xc000] });
    const edited = applyEdits(build(project, bytes).project, [
      { op: 'defineSymbol', name: 'target', address: 0xc005, kind: 'label' },
      { op: 'setHint', address: 0xc000, form: 'low', symbol: 'target' },
      { op: 'setHint', address: 0xc002, form: 'high', symbol: 'target' },
      { op: 'setHint', address: 0xc004, form: 'char' },
      { op: 'setHint', address: 0xc006, form: 'binary' },
      { op: 'setHint', address: 0xc008, form: 'low', symbol: 'target' },
    ]).project;
    const generated = generateProject(edited, bytes);
    const source = generated.files.get('src/main.s')!;
    assert.ok(source.includes('  lda #<target\n  ldx #>target\ntarget = * + 1\n  ldy #\'A\'\n  lda #%00001010\n  lda #$99\n'));
    assert.deepEqual(generated.warnings.map((warning) => warning.code), ['hint-ignored']);
    assert.match(generated.warnings[0]!.message, /\$C008/);
  });

  it('模块：每个模块一个文件，范围在段里按地址切成片，没归入的字节在 main', () => {
    const { project, bytes } = flatProject([0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0x60], { entries: [0xc000] });
    const edited = applyEdits(build(project, bytes).project, [{ op: 'assignModule', name: 'video', description: '显示', address: 0xc002, length: 2 }]).project;
    const generated = generateProject(edited, bytes);
    assert.deepEqual(generated.modules, ['main', 'video']);
    assert.deepEqual(generated.pieces.map((piece) => [piece.name, piece.module, piece.start, piece.end]), [['SEG0_0000', 'main', 0, 2], ['SEG0_0002', 'video', 2, 4], ['SEG0_0004', 'main', 4, 7]]);
    assert.match(generated.files.get('src/video.s')!, /^; 模块：video\n; 显示/);
    const config = generated.files.get('link.cfg')!;
    assert.ok(config.includes('SEG0: file = "seg0.bin", start = $C000, size = $7, fill = yes;'));
    assert.ok(config.indexOf('SEG0_0000') < config.indexOf('SEG0_0002') && config.indexOf('SEG0_0002') < config.indexOf('SEG0_0004'));
  });

  it('映像与工程不符、缺符号集、与符号集重名时报错', () => {
    const { project, bytes } = flatProject([0x60], { entries: [0xc000] });
    assert.throws(() => generateProject(project, Buffer.from([0x60, 0x60])), (error: FcdevError) => error.code === 'DISASM_IMAGE_MISMATCH');
    assert.throws(() => generateProject({ ...project, symbolSets: [{ name: 'a.inc', path: 'a.inc' }] }, bytes), (error: FcdevError) => error.code === 'DISASM_SYMBOL_SET_MISSING');
    const clash = applyEdits(project, [{ op: 'defineSymbol', name: 'REG', address: 0xc000, kind: 'label' }]).project;
    assert.throws(
      () => generateProject({ ...clash, symbolSets: [{ name: 'a.inc', path: 'a.inc' }] }, bytes, { symbolSets: new Map([['a.inc', 'REG = $10\n']]) }),
      (error: FcdevError) => error.code === 'DISASM_SYMBOL_DUPLICATE',
    );
  });
});

/** 固定种子的伪随机数 */
function createRandom(seed: number) {
  let state = seed >>> 0;
  return () => {
    state = (state + 0x6d2b79f5) >>> 0;
    let value = Math.imul(state ^ (state >>> 15), 1 | state);
    value = (value + Math.imul(value ^ (value >>> 7), 61 | value)) ^ value;
    return ((value ^ (value >>> 14)) >>> 0) / 4294967296;
  };
}

describe.skipIf(!HAS_TOOLCHAIN)('回读（需要 cc65）', () => {
  let temp: TempDir;
  beforeEach(() => { temp = createTempDir(); });
  afterEach(() => temp.cleanup());

  /** 追踪、命名、生成并回读 */
  async function roundtrip(project: Project, bytes: Buffer, symbolSets = new Map<string, string>()) {
    const built = build(project, bytes, symbolSets);
    const result = await verifyRoundtrip({ files, tools, project: built.project, bytes, generated: built.generated, projectDir: path.join(temp.dir, 'project'), toolchain: locateTestToolchain() });
    return { ...built, result };
  }

  it('全部 151 条指令各取多个操作数，加上非官方操作码：整段当作代码，回读一致', async () => {
    const image = [];
    for (const entry of OPCODE_TABLE) {
      if (entry === null) continue;
      const operands = entry.size === 1 ? [null] : entry.size === 2 ? [0x00, 0x12, 0x7f, 0x80, 0xff] : [0x0000, 0x0012, 0x0100, 0x1234, 0xc000, 0xc003, 0xc010, 0xffff];
      for (const operand of operands) {
        image.push(entry.opcode);
        if (entry.size >= 2) image.push((operand ?? 0) & 0xff);
        if (entry.size === 3) image.push((operand ?? 0) >> 8);
      }
    }
    OPCODE_TABLE.forEach((entry, opcode) => { if (entry === null) image.push(opcode, 0x55); });
    const { project, bytes } = flatProject(image);
    const prepared = applyEdits(project, [{ op: 'setRegion', address: 0xc000, length: image.length, type: 'code' }]).project;
    const { result, generated } = await roundtrip(prepared, bytes);
    assert.deepEqual(result, { ok: true, firstDifference: null });
    const source = generated.files.get('src/main.s')!;
    assert.ok(source.includes('  lda #$01') === false);
    assert.ok(source.includes('  .byte $6C, $FF'));
    assert.ok(/^ {2}lda /m.test(source));
  });

  it('随机字节序列、随机区域、随机入口与模块：回读都一致', { timeout: 120_000 }, async () => {
    for (let seed = 1; seed <= 25; seed += 1) {
      const random = createRandom(seed);
      const size = 32 + Math.floor(random() * 224);
      const image = Array.from({ length: size }, () => Math.floor(random() * 256));
      if (random() < 0.5) image.fill(image[3] ?? 0, 8, 8 + Math.min(40, size - 8));
      const address = random() < 0.5 ? 0xc000 : 0x8000;
      const entries = Array.from({ length: 1 + Math.floor(random() * 3) }, () => address + Math.floor(random() * size));
      const { project, bytes } = flatProject(image, { address, entries });
      const types = ['code', 'bytes', 'words', 'string', 'padding', 'graphics', 'unknown'];
      const edits = [];
      let position = 0;
      while (position < size) {
        const type = types[Math.floor(random() * types.length)];
        let length = Math.min(size - position, 1 + Math.floor(random() * 40));
        if (type === 'words') length -= length % 2;
        if (length > 0 && type !== 'unknown') edits.push({ op: 'setRegion', address: address + position, length, type, params: type === 'words' ? { adjust: Math.floor(random() * 2) } : undefined });
        position += Math.max(length, 1);
      }
      const split = Math.floor(random() * size);
      if (split > 0 && split < size) edits.push({ op: 'assignModule', name: 'second', address: address + split, length: size - split });
      const { result } = await roundtrip(applyEdits(project, edits as EditItem[]).project, bytes);
      assert.deepEqual(result, { ok: true, firstDifference: null }, `种子 ${seed}`);
    }
  });

  it('换页布局：固定区与两个 bank 各有输出文件，bank 里的转移留在 bank 内，跨 bank 的写成数值', async () => {
    const bytes = Buffer.alloc(48, 0xff);
    bytes.set([0x20, 0x00, 0x80, 0x60], 0x00);
    bytes.set([0x20, 0x04, 0x80, 0x60, 0xa9, 0x01, 0x60], 0x10);
    bytes.set([0x4c, 0x06, 0x80, 0xff, 0xff, 0xff, 0xe8, 0x60], 0x20);
    const layout = createLayout([
      { id: 0, fileOffset: 0x00, length: 0x10, address: 0xc000 },
      { id: 1, fileOffset: 0x10, length: 0x10, address: 0x8000, bank: 0 },
      { id: 2, fileOffset: 0x20, length: 0x10, address: 0x8000, bank: 1 },
    ], { imageSize: 48 });
    const project = createProject({
      imagePath: 'banked.bin', bytes, format: 'flat', layout,
      entries: [{ segmentId: 0, address: 0xc000 }, { segmentId: 1, address: 0x8000 }, { segmentId: 2, address: 0x8000 }].map((entry) => ({ ...entry, source: 'manual', description: '' })),
    });
    const modular = applyEdits(project, [{ op: 'assignModule', name: 'bank1', segmentId: 2, address: 0x8000, length: 0x10 }]).project;
    const { result, generated, project: named } = await roundtrip(modular, bytes);
    assert.deepEqual(result, { ok: true, firstDifference: null });
    const config = generated.files.get('link.cfg')!;
    assert.ok(config.includes('SEG1: file = "seg1.bin", start = $8000, size = $10, fill = yes;'));
    assert.ok(config.includes('SEG2: file = "seg2.bin", start = $8000, size = $10, fill = yes;'));
    assert.ok(generated.files.get('src/main.s')!.includes('  jsr $8000\n'));
    assert.ok(generated.files.get('src/main.s')!.includes('  jsr sub_b0_8004\n'));
    assert.ok(generated.files.get('src/bank1.s')!.includes('  jmp loc_b1_8006\n'));
    assert.deepEqual(named.unresolved.map((item) => item.kind), ['cross-bank']);
  });

  it('生成的文件里改动一个字节，回读指出第一处差异的段、地址与符号', async () => {
    const { project, bytes } = flatProject([0xa9, 0x01, 0x60], { entries: [0xc000] });
    const built = build(project, bytes);
    const tampered = new Map(built.generated.files);
    tampered.set('src/main.s', tampered.get('src/main.s')!.replace('lda #$01', 'lda #$02'));
    const result = await verifyRoundtrip({ files, tools, project: built.project, bytes, generated: { ...built.generated, files: tampered }, projectDir: path.join(temp.dir, 'bad'), toolchain: locateTestToolchain() });
    assert.deepEqual(result, { ok: false, firstDifference: { segmentId: 0, address: 0xc001, offset: 1, expected: 1, actual: 2, symbol: 'sub_C000' } });
  });

  it('回读结果记进工程，工程再改动后算过期；进度报告反映区域、自动符号与回读', async () => {
    const { project, bytes } = flatProject([0xa9, 0x01, 0x20, 0x05, 0xc0, 0x60, 0x6c, 0x00, 0x02], { entries: [0xc000] });
    const { result, project: named } = await roundtrip(project, bytes);
    const recorded = recordVerification(named, result, new Date('2026-11-21T00:00:00Z'));
    let status = buildStatus(recorded, bytes);
    assert.equal(status.verification.state, 'ok');
    assert.equal(status.image.stale, false);
    assert.equal(status.regions.code!.bytes, 6);
    assert.equal(status.symbols.auto, status.symbols.total);
    assert.equal(status.proceduresWithoutComment, 2);
    assert.deepEqual(status.unresolved, { indirectJumps: 0, crossBank: 0 });
    const edited = applyEdits(recorded, [{ op: 'renameSymbol', from: 'sub_C000', to: 'entry' }, { op: 'setComment', address: 0xc000, kind: 'block', text: '入口' }]).project;
    status = buildStatus(edited);
    assert.equal(isVerificationStale(edited), true);
    assert.equal(status.verification.state, 'stale');
    assert.equal(status.symbols.auto, status.symbols.total - 1);
    assert.equal(status.proceduresWithoutComment, 1);
  });

  /** 构建一个项目并返回产物路径 */
  async function buildProject(target: string, fileName: string, source: string): Promise<string> {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target }));
    writeTestFile(temp.dir, fileName, source);
    const { result } = await buildPath({ pathArg: fileName, cwd: temp.dir });
    return result.outputFile;
  }

  describe('fcdev 构建的真实程序', () => {
    it('NES 程序：入口取自中断向量，用 fcnes.inc 的寄存器名，回读一致', async () => {
      const file = readFileSync(await buildProject('nes', 'hello.c', '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "HI"); nes_screen_on(); for (;;) { } }\n'));
      const parsed = parseInes(file);
      const { entries } = entriesFromVectors(createAddressSpace(parsed.layout), parsed.vectors);
      const symbolSets = new Map([['fcnes.inc', readFileSync(path.join(BUILTIN_TARGETS_DIR, 'nes', 'asminc', 'fcnes.inc'), 'utf8')]]);
      const project = {
        ...createProject({ imagePath: 'hello.nes', bytes: file, format: 'ines', layout: parsed.layout, entries, symbolSets: [{ name: 'fcnes.inc', path: 'fcnes.inc' }] }),
      };
      const { result, generated } = await roundtrip(project, file, symbolSets);
      assert.deepEqual(result, { ok: true, firstDifference: null });
      const source = generated.files.get('src/main.s')!;
      assert.match(source, /^reset:$/m);
      assert.match(source, /PPUSTATUS/);
      assert.ok(source.split('\n').filter((line) => /^ {2}[a-z]{3}( |$)/.test(line)).length > 50);
    });

    it('SB2000 程序：平铺在 $8000，入口取自文件头的 JMP，回读一致', async () => {
      const file = readFileSync(await buildProject('sb2000', 'hello.c', '#include <sb2000.h>\nint main(void) { sb_puts("Hi"); return 0; }\n'));
      const { project, bytes } = flatProject(file, { address: 0x8000, entries: [file.readUInt16LE(1)] });
      const { result, generated } = await roundtrip(project, bytes);
      assert.deepEqual(result, { ok: true, firstDifference: null });
      assert.ok(generated.files.get('src/main.s')!.includes('  jsr '));
    });
  });
});
