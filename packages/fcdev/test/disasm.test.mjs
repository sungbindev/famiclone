// ------------------------------------------------------------------------
// 名称：disasm.test.mjs
// 说明：测试反汇编前端：映像读取、CDL 与轨迹解析、静态追踪、信息文件生成、回读校验，以及对 fcdev 自己构建的 NES 与 SB2000 程序做完整反汇编。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：完整反汇编的用例需要 cc65 工具链；环境变量 FCDEV_TEST_NES_ROM 指向一个 NROM 的 .nes 文件时，另做一次真实游戏的验证（仓库不含游戏）。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath } from '../src/build/index.mjs';
import { runCli } from '../src/cli/index.mjs';
import { parseAddress } from '../src/cli/commands/disasm-command.mjs';
import {
  analyzeCode, buildRanges, disassemble, findFirstDifference, loadBinary, loadInes, parseCdl, parseSymbols, parseTrace, renderInfo, renderRoundtripConfig,
} from '../src/disasm/index.mjs';
import { OPCODES } from '../src/disasm/opcode-table.mjs';
import { verifyRoundtrip } from '../src/disasm/roundtrip.mjs';
import { BUILTIN_TARGETS_DIR } from '../src/targets/index.mjs';
import { locateToolchain } from '../src/toolchain/index.mjs';
import { createTempDir, findTestToolchain, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** 构造一个 iNES 文件：prgKilobytes 为 16 或 32，向量三个字放在 PRG 末尾 */
function makeInes({ prgKilobytes = 16, mapper = 0, vectors = [0xc000, 0xc000, 0xc000], trainer = false, code = [] } = {}) {
  const prg = Buffer.alloc(prgKilobytes * 1024);
  Buffer.from(code).copy(prg, 0);
  vectors.forEach((value, index) => prg.writeUInt16LE(value, prg.length - 6 + index * 2));
  const header = Buffer.alloc(16);
  header.write('NES\x1a', 0, 'latin1');
  header[4] = prgKilobytes / 16;
  header[5] = 1;
  header[6] = ((mapper & 0x0f) << 4) | (trainer ? 0x04 : 0);
  header[7] = mapper & 0xf0;
  return Buffer.concat([header, trainer ? Buffer.alloc(512, 0xee) : Buffer.alloc(0), prg, Buffer.alloc(8192)]);
}

describe('指令表', () => {
  it('有 151 条官方指令，长度与流程类别正确', () => {
    assert.equal(OPCODES.filter(Boolean).length, 151);
    assert.deepEqual(OPCODES[0xa9], ['lda', 2, 'next']);
    assert.deepEqual(OPCODES[0x8d], ['sta', 3, 'next']);
    assert.deepEqual(OPCODES[0xd0], ['bne', 2, 'branch']);
    assert.deepEqual(OPCODES[0x20], ['jsr', 3, 'call']);
    assert.deepEqual(OPCODES[0x4c], ['jmp', 3, 'jump']);
    assert.deepEqual(OPCODES[0x6c], ['jmp', 3, 'jump-indirect']);
    assert.equal(OPCODES[0x60][2], 'return');
    assert.equal(OPCODES[0x40][2], 'return');
    assert.equal(OPCODES[0x00][2], 'stop');
    assert.equal(OPCODES[0x02], null);
  });
});

describe('读取映像', () => {
  it('NROM-256 的 PRG 映在 $8000，NROM-128 映在 $C000，并读出三个中断向量', () => {
    const big = loadInes(makeInes({ prgKilobytes: 32, vectors: [0x8100, 0x8000, 0x8200] }));
    assert.equal(big.base, 0x8000);
    assert.equal(big.bytes.length, 0x8000);
    assert.deepEqual(big.vectors, { nmi: 0x8100, reset: 0x8000, irq: 0x8200 });
    assert.equal(loadInes(makeInes({ prgKilobytes: 16 })).base, 0xc000);
  });

  it('跳过 trainer，PRG 内容不变', () => {
    const image = loadInes(makeInes({ trainer: true, code: [0xa9, 0x01] }));
    assert.deepEqual([...image.bytes.subarray(0, 2)], [0xa9, 0x01]);
  });

  it('映射器不是 0、文件头不对、文件被截断时报错', () => {
    assert.throws(() => loadInes(makeInes({ mapper: 4 })), (error) => error.code === 'DISASM_MAPPER_UNSUPPORTED' && error.message.includes('4'));
    assert.throws(() => loadInes(Buffer.from('not a nes file at all')), (error) => error.code === 'DISASM_INPUT_INVALID');
    assert.throws(() => loadInes(makeInes().subarray(0, 100)), (error) => error.code === 'DISASM_INPUT_INVALID');
  });

  it('平铺二进制按加载地址处理，空文件与超出 $FFFF 报错', () => {
    const image = loadBinary(Buffer.from([1, 2, 3]), { base: 0x8000 });
    assert.equal(image.base, 0x8000);
    assert.equal(image.vectors, null);
    assert.throws(() => loadBinary(Buffer.alloc(0), { base: 0 }), (error) => error.code === 'DISASM_INPUT_INVALID');
    assert.throws(() => loadBinary(Buffer.alloc(16), { base: 0xfff8 }), (error) => error.code === 'DISASM_INPUT_INVALID');
  });
});

describe('CDL、轨迹与符号文件', () => {
  it('CDL 只保留代码与数据两位', () => {
    const flags = parseCdl(Buffer.from([0x01, 0x02, 0x83, 0x00]), 4);
    assert.deepEqual([...flags], [1, 2, 3, 0]);
  });

  it('Mesen 的 CDLv2 文件头被跳过，文件比 PRG 短时报错', () => {
    const buffer = Buffer.concat([Buffer.from('CDLv2'), Buffer.alloc(4), Buffer.from([1, 0])]);
    assert.deepEqual([...parseCdl(buffer, 2)], [1, 0]);
    assert.throws(() => parseCdl(Buffer.from([1]), 4), (error) => error.code === 'DISASM_CDL_INVALID');
  });

  it('轨迹取出不重复的指令地址，忽略其他行', () => {
    const text = 'T 8000 -000000 78 00 00 A=00\nT 8001 -000001 D8 00 00 A=00\nT 8000 -000000 78 00 00 A=00\nI NMI\nT C0DE -000000 EA 00 00 A=00\n';
    assert.deepEqual(parseTrace(text), [0x8000, 0x8001, 0xc0de]);
  });

  it('符号文件支持等号与空格、$ 与 0x 前缀、注释与空行', () => {
    const text = '; 注释\n\nplayer_x = $0010\nscore $0020 ; 分数\nvram 0x2007\n# 另一种注释\n';
    assert.deepEqual(parseSymbols(text), [{ name: 'player_x', address: 0x10 }, { name: 'score', address: 0x20 }, { name: 'vram', address: 0x2007 }]);
  });

  it('符号文件格式不对时指出行号', () => {
    assert.throws(() => parseSymbols('ok = $10\nbad line here\n'), (error) => error.code === 'DISASM_SYMBOL_INVALID' && error.message.includes('第 2 行'));
  });
});

describe('静态追踪', () => {
  /** 0xC000 起的小程序，布局见各用例 */
  const PROGRAM = Buffer.from([
    0xa9, 0x05,       // C000 LDA #$05
    0xf0, 0x03,       // C002 BEQ $C007
    0xea,             // C004 NOP
    0x60,             // C005 RTS
    0xff,             // C006 非法操作码，没有任何路径到达
    0x20, 0x0b, 0xc0, // C007 JSR $C00B
    0x60,             // C00A RTS
    0xe8,             // C00B INX
    0x60,             // C00C RTS
  ]);

  it('顺序、条件分支与子程序调用都被追到，不可达的字节保持未知', () => {
    const { kinds, instructions, warnings } = analyzeCode({ bytes: PROGRAM, base: 0xc000, entries: [0xc000] });
    assert.deepEqual([...kinds], [1, 2, 1, 2, 1, 1, 0, 1, 2, 2, 1, 1, 1]);
    assert.equal(instructions, 8);
    assert.deepEqual(warnings, []);
  });

  it('JMP 只追目标，之后的字节不当作代码', () => {
    const bytes = Buffer.from([0x4c, 0x05, 0x80, 0xff, 0xff, 0x60]);
    const { kinds } = analyzeCode({ bytes, base: 0x8000, entries: [0x8000] });
    assert.deepEqual([...kinds], [1, 2, 2, 0, 0, 1]);
  });

  it('间接 JMP、RTI 与 BRK 结束这一路', () => {
    const bytes = Buffer.from([0x6c, 0x00, 0x02, 0xea, 0x40, 0xea, 0x00, 0xea]);
    const { kinds } = analyzeCode({ bytes, base: 0x8000, entries: [0x8000, 0x8004, 0x8006] });
    assert.deepEqual([...kinds], [1, 2, 2, 0, 1, 0, 1, 0]);
  });

  it('非法操作码、越界与重叠都放弃这一路并给出警告', () => {
    const illegal = analyzeCode({ bytes: Buffer.from([0x02, 0xea]), base: 0x8000, entries: [0x8000] });
    assert.match(illegal.warnings[0], /\$8000：操作码 \$02 不是官方/);
    const cut = analyzeCode({ bytes: Buffer.from([0x4c, 0x00]), base: 0x8000, entries: [0x8000] });
    assert.match(cut.warnings[0], /超出映像末尾/);
    const overlap = analyzeCode({ bytes: Buffer.from([0xa9, 0xea, 0x60]), base: 0x8000, entries: [0x8001, 0x8000] });
    assert.match(overlap.warnings[0], /\$8001：与已识别的指令重叠/);
  });

  it('跳到映像之外的目标被忽略', () => {
    const { kinds, warnings } = analyzeCode({ bytes: Buffer.from([0x20, 0x00, 0x20, 0x60]), base: 0x8000, entries: [0x8000] });
    assert.deepEqual([...kinds], [1, 2, 2, 1]);
    assert.deepEqual(warnings, []);
  });

  it('CDL 里每段连续的代码字节，起点也被当作入口', () => {
    const bytes = Buffer.from([0xea, 0x60, 0x00, 0xa9, 0x01, 0x60]);
    const cdl = Uint8Array.from([0, 0, 0, 1, 1, 1]);
    const { kinds } = analyzeCode({ bytes, base: 0x8000, entries: [], cdl });
    assert.deepEqual([...kinds], [0, 0, 0, 1, 2, 1]);
  });
});

describe('信息文件', () => {
  it('把映像划分成覆盖全部字节的代码、数据与向量表段', () => {
    const kinds = Uint8Array.from([1, 2, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0]);
    const ranges = buildRanges({ kinds, base: 0xfff4, hasVectors: true });
    assert.deepEqual(ranges, [
      { start: 0xfff4, end: 0xfff6, type: 'Code' },
      { start: 0xfff7, end: 0xfff9, type: 'ByteTable' },
      { start: 0xfffa, end: 0xffff, type: 'AddrTable' },
    ]);
  });

  it('没有向量表时整个映像都是代码或数据段，相邻的同类段合并', () => {
    const ranges = buildRanges({ kinds: Uint8Array.from([1, 2, 2, 0, 0, 1]), base: 0x8000, hasVectors: false });
    assert.deepEqual(ranges.map((range) => [range.start, range.end, range.type]), [[0x8000, 0x8002, 'Code'], [0x8003, 0x8004, 'ByteTable'], [0x8005, 0x8005, 'Code']]);
  });

  it('信息文件包含全局设置、符号、符号定义文件与各段，同一地址的符号只留第一个', () => {
    const text = renderInfo({
      inputName: 'a.bin',
      outputName: 'a.s',
      base: 0x8000,
      ranges: [{ start: 0x8000, end: 0x80ff, type: 'Code' }],
      labels: [{ name: 'main', address: 0x8000 }, { name: 'reset', address: 0x8000 }, { name: 'var', address: 0x10 }],
      includes: ['C:\\x\\sym.inc'],
    });
    assert.match(text, /GLOBAL \{ INPUTNAME "a\.bin"; OUTPUTNAME "a\.s"; STARTADDR \$8000; CPU "6502"; PAGELENGTH 0; \};/);
    assert.match(text, /ASMINC \{ FILE "C:\/x\/sym\.inc"; \};/);
    assert.match(text, /LABEL \{ NAME "main"; ADDR \$8000; \};/);
    assert.doesNotMatch(text, /NAME "reset"/);
    assert.match(text, /LABEL \{ NAME "var"; ADDR \$0010; \};/);
    assert.match(text, /RANGE \{ START \$8000; END \$80FF; TYPE Code; \};/);
  });

  it('符号名不合法时报错', () => {
    assert.throws(() => renderInfo({ inputName: 'a', outputName: 'b', base: 0, ranges: [], labels: [{ name: '1bad', address: 1 }] }), (error) => error.code === 'DISASM_SYMBOL_INVALID');
  });
});

describe('回读校验辅助', () => {
  it('找出第一个差异，长度不同时在较短的末尾', () => {
    assert.equal(findFirstDifference(Buffer.from([1, 2, 3]), Buffer.from([1, 2, 3])), -1);
    assert.equal(findFirstDifference(Buffer.from([1, 2, 3]), Buffer.from([1, 9, 3])), 1);
    assert.equal(findFirstDifference(Buffer.from([1, 2, 3]), Buffer.from([1, 2])), 2);
  });

  it('链接配置的内存区从映像起始地址开始，大小等于映像', () => {
    assert.match(renderRoundtripConfig(0xc000, 0x4000), /start = \$C000, size = \$4000/);
  });

  it('地址参数接受 $、0x 前缀与裸十六进制，其他写法报错', () => {
    assert.equal(parseAddress('$8000', '--base'), 0x8000);
    assert.equal(parseAddress('0xC000', '--base'), 0xc000);
    assert.equal(parseAddress('ff', '--entry'), 0xff);
    assert.throws(() => parseAddress('12345', '--base'), /--base/);
    assert.throws(() => parseAddress('zz', '--entry'), /--entry/);
  });
});

describe('完整反汇编（需要 cc65）', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  /** 构建一个 fcdev 项目并返回产物路径 */
  async function buildProject(target, fileName, source) {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target }));
    writeTestFile(temp.dir, fileName, source);
    const { result } = await buildPath({ pathArg: fileName, cwd: temp.dir });
    return result.outputFile;
  }

  /** 运行反汇编 */
  function run(options) {
    return disassemble({ outDir: path.join(temp.dir, 'out'), toolchain: locateToolchain(), ...options });
  }

  const NES_HELLO = '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "HI"); nes_screen_on(); for (;;) { } }\n';

  it('fcdev 构建的 NES 程序：入口取自中断向量，回读与原映像逐字节一致，源码里有 reset 标签与传入的寄存器符号', async () => {
    const rom = await buildProject('nes', 'hello.c', NES_HELLO);
    const includes = [path.join(BUILTIN_TARGETS_DIR, 'nes', 'asminc', 'fcnes.inc')];
    const report = await run({ inputFile: rom, includes });
    assert.equal(report.verified, true);
    assert.ok(report.instructions > 50, `指令数 ${report.instructions}`);
    assert.ok(report.codeBytes < report.size);
    const source = readFileSync(report.sourceFile, 'utf8');
    assert.match(source, /^reset:/m);
    assert.match(source, /PPUSTATUS/);
  });

  it('符号文件里的名字出现在源码里', async () => {
    const rom = await buildProject('nes', 'hello.c', NES_HELLO);
    const symbols = writeTestFile(temp.dir, 'symbols.txt', 'zero_page_var = $0070\n');
    const report = await run({ inputFile: rom, symbolsFile: symbols });
    assert.match(readFileSync(report.sourceFile, 'utf8'), /zero_page_var/);
  });

  it('fcdev 构建的 SB2000 程序：平铺二进制，加载地址 $8000，入口取自文件头的 JMP', async () => {
    const exe = await buildProject('sb2000', 'hello.c', '#include <sb2000.h>\nint main(void) { sb_puts("Hi"); return 0; }\n');
    const image = readFileSync(exe);
    const entry = image.readUInt16LE(1);
    const report = await run({ inputFile: exe, format: 'binary', base: 0x8000, entries: [entry] });
    assert.equal(report.verified, true);
    assert.ok(report.instructions > 20);
    assert.ok(readFileSync(report.sourceFile, 'utf8').includes('jsr'));
  });

  it('只给 CDL 或只给轨迹也能做（没有向量表的平铺二进制）', async () => {
    const exe = await buildProject('sb2000', 'hello.c', '#include <sb2000.h>\nint main(void) { return 0; }\n');
    const image = readFileSync(exe);
    const entry = image.readUInt16LE(1);
    const cdl = Buffer.alloc(image.length);
    cdl.fill(0x01, entry - 0x8000);
    const cdlFile = path.join(temp.dir, 'sb.cdl');
    writeFileSync(cdlFile, cdl);
    const byCdl = await run({ inputFile: exe, format: 'binary', base: 0x8000, cdlFile });
    assert.equal(byCdl.verified, true);
    assert.ok(byCdl.instructions > 5);

    const traceFile = path.join(temp.dir, 'sb.trace');
    writeFileSync(traceFile, `T ${entry.toString(16).toUpperCase()} -000000 A9 00 00 A=00 X=00 Y=00 S=FF P=22 F=0\n`);
    const byTrace = await run({ inputFile: exe, format: 'binary', base: 0x8000, traceFile, outDir: path.join(temp.dir, 'out2') });
    assert.equal(byTrace.verified, true);
    assert.ok(byTrace.instructions > 5);
  });

  it('缺少加载地址、缺少入口、映射器不支持时报错', async () => {
    const exe = await buildProject('sb2000', 'hello.c', '#include <sb2000.h>\nint main(void) { return 0; }\n');
    await assert.rejects(run({ inputFile: exe, format: 'binary', entries: [0x8000] }), (error) => error.code === 'ARGUMENT_MISSING' && error.hint.includes('--base'));
    await assert.rejects(run({ inputFile: exe, format: 'binary', base: 0x8000 }), (error) => error.code === 'ARGUMENT_MISSING' && error.hint.includes('--entry'));
    const mapper4 = path.join(temp.dir, 'm4.nes');
    writeFileSync(mapper4, makeInes({ mapper: 4 }));
    await assert.rejects(run({ inputFile: mapper4 }), (error) => error.code === 'DISASM_MAPPER_UNSUPPORTED');
  });

  it('回读不一致时能被识别并指出第一个差异', async () => {
    const source = writeTestFile(temp.dir, 'wrong.s', '.byte $01, $02, $03\n');
    const result = await verifyRoundtrip({ toolchain: locateToolchain(), sourceFile: source, expected: Buffer.from([1, 2, 4]), base: 0x8000, workDir: temp.dir, name: 'wrong' });
    assert.deepEqual(result, { ok: false, firstDifference: 2 });
  });

  it('命令行：disasm 成功时输出校验结果；参数不对时报错', async () => {
    const rom = await buildProject('nes', 'hello.c', NES_HELLO);
    const stdout = [];
    const stderr = [];
    const context = { cwd: temp.dir, env: process.env, stdout: { write: (text) => stdout.push(text) }, stderr: { write: (text) => stderr.push(text) } };
    assert.equal(await runCli(['disasm', rom, '--out', 'dis'], context), 0);
    assert.match(stdout.join(''), /回读校验通过/);
    assert.ok(existsSync(path.join(temp.dir, 'dis', 'hello.s')));
    assert.equal(await runCli(['disasm'], context), 1);
    assert.match(stderr.join(''), /需要一个输入文件/);
    stderr.length = 0;
    assert.equal(await runCli(['disasm', rom, '--base', 'xyz'], context), 1);
    assert.match(stderr.join(''), /--base/);
  });
});

const externalRom = process.env.FCDEV_TEST_NES_ROM;

describe('真实 NES 游戏（FCDEV_TEST_NES_ROM）', { skip: externalRom && existsSync(externalRom) && findTestToolchain() ? false : '没有设置 FCDEV_TEST_NES_ROM，或没有 cc65' }, () => {
  it('回读与原映像逐字节一致', async () => {
    const temp = createTempDir();
    try {
      const report = await disassemble({ inputFile: externalRom, outDir: temp.dir, toolchain: locateToolchain() });
      assert.equal(report.verified, true);
      assert.ok(report.instructions > 100);
    } finally {
      temp.cleanup();
    }
  });
});
