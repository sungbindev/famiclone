// ------------------------------------------------------------------------
// 名称：disasm-pipeline.test.ts
// 说明：测试一次完成的反汇编：对扩展自己构建的 NES 与 SB2000 程序生成反汇编工程并回读，符号文件、
//       CDL 与指令轨迹提供入口，输入不对时的错误。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：需要扩展自带的 cc65 工具链（没有时跳过）；环境变量 FCDEV_TEST_NES_ROM 指向一个 NROM 的 .nes
//       文件时，另做一次真实游戏的验证（仓库不含游戏）。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'vitest';
import type { DisassembleInput } from '../../../src/contracts/disassembly/disasm-operations';
import type { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { readProject, readProjectImage } from '../../../src/logic/disassembly/workspace/project-store';
import { createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';
import { BUILTIN_TARGETS_DIR, DISASM, HAS_TOOLCHAIN, buildPath } from '../../support/disasm-fixtures';

/** 构造一个 iNES 文件：prgKilobytes 为 16 或 32，向量三个字放在 PRG 末尾 */
function makeInes({ prgKilobytes = 16, mapper = 0, vectors = [0xc000, 0xc000, 0xc000], code = [] as number[] } = {}) {
  const prg = Buffer.alloc(prgKilobytes * 1024);
  Buffer.from(code).copy(prg, 0);
  vectors.forEach((value, index) => prg.writeUInt16LE(value, prg.length - 6 + index * 2));
  const header = Buffer.alloc(16);
  header.write('NES\x1a', 0, 'latin1');
  header[4] = prgKilobytes / 16;
  header[5] = 1;
  header[6] = (mapper & 0x0f) << 4;
  header[7] = mapper & 0xf0;
  return Buffer.concat([header, prg, Buffer.alloc(8192)]);
}

/** 一行指令轨迹：给定地址处执行 LDA #$00 */
function traceLine(address: number): string {
  return `${address.toString(16).toUpperCase().padStart(4, '0')}  A9 00     LDA #$00                        A:00 X:00 Y:00 P:22 SP:FF CYC:7\n`;
}

describe.skipIf(!HAS_TOOLCHAIN)('一次完成的反汇编（需要 cc65）', () => {
  const files = new NodeFileSystem();
  let temp: TempDir;

  beforeEach(() => { temp = createTempDir(); });
  afterEach(() => temp.cleanup());

  /** 构建一个项目并返回产物路径 */
  async function buildProject(target: string, fileName: string, source: string): Promise<string> {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target }));
    writeTestFile(temp.dir, fileName, source);
    const { result } = await buildPath({ pathArg: fileName, cwd: temp.dir });
    return result.outputFile;
  }

  /** 运行反汇编 */
  function run(options: Omit<DisassembleInput, 'outDir'> & { outDir?: string }) {
    return DISASM.disassemble({ outDir: path.join(temp.dir, 'out'), ...options });
  }

  const NES_HELLO = '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "HI"); nes_screen_on(); for (;;) { } }\n';
  const NES_REGISTERS = path.join(BUILTIN_TARGETS_DIR, 'nes', 'asminc', 'fcnes.inc');

  it('NES 程序：入口取自中断向量，回读一致，源码里有 reset 标签与寄存器符号，工程文件可读回', async () => {
    const rom = await buildProject('nes', 'hello.c', NES_HELLO);
    const report = await run({ inputFile: rom, symbolSetFiles: [NES_REGISTERS] });
    assert.equal(report.verified, true);
    assert.ok(report.instructions > 50, `指令数 ${report.instructions}`);
    assert.ok(report.codeBytes < report.mappedBytes);
    assert.equal(report.status.verification.state, 'ok');
    const source = readFileSync(path.join(report.projectDir, 'src', 'main.s'), 'utf8');
    assert.match(source, /^reset:/m);
    assert.match(source, /PPUSTATUS/);
    for (const file of ['disasm.json', 'link.cfg', 'include/globals.inc', 'include/fcnes.inc']) assert.ok(existsSync(path.join(report.projectDir, file)), file);

    const project = readProject(files, report.projectDir);
    assert.equal(project.format.name, 'ines');
    assert.equal(project.verification?.ok, true);
    assert.deepEqual(readProjectImage(files, report.projectDir, project), readFileSync(rom));
  });

  it('符号文件里的名字出现在源码里，同一地址有多个名字时只用第一个', async () => {
    const rom = await buildProject('nes', 'hello.c', NES_HELLO);
    const symbols = writeTestFile(temp.dir, 'symbols.txt', 'zero_page_var = $0070\nsecond_name = $0070\n');
    const report = await run({ inputFile: rom, symbolsFile: symbols });
    const source = readFileSync(path.join(report.projectDir, 'src', 'main.s'), 'utf8');
    assert.match(source, /zero_page_var/);
    assert.doesNotMatch(source, /second_name/);
  });

  it('SB2000 程序：平铺在 $8000，入口取自文件头的 JMP，回读一致', async () => {
    const exe = await buildProject('sb2000', 'hello.c', '#include <sb2000.h>\nint main(void) { sb_puts("Hi"); return 0; }\n');
    const entry = readFileSync(exe).readUInt16LE(1);
    const report = await run({ inputFile: exe, format: 'flat', base: 0x8000, entries: [entry] });
    assert.equal(report.verified, true);
    assert.ok(report.instructions > 20);
    assert.ok(readFileSync(path.join(report.projectDir, 'src', 'main.s'), 'utf8').includes('jsr'));
  });

  it('SB2000 程序按 sbdos 格式：不用给加载地址与入口，文件头的 JMP 就是入口，回读一致', async () => {
    const exe = await buildProject('sb2000', 'hello.c', '#include <sb2000.h>\nint main(void) { sb_puts("Hi"); return 0; }\n');
    const report = await run({ inputFile: exe, format: 'sbdos' });
    assert.equal(report.verified, true);
    assert.match(report.description, /SB-DOS 程序/);
    assert.deepEqual(readProject(files, report.projectDir).entries.map((item) => [item.address, item.source]), [[0x8000, 'vector']]);
  });

  it('只给 CDL 或只给指令轨迹也能做（没有向量表的平铺二进制），轨迹里不在映像里的地址被忽略并给出警告', async () => {
    const exe = await buildProject('sb2000', 'hello.c', '#include <sb2000.h>\nint main(void) { return 0; }\n');
    const image = readFileSync(exe);
    const entry = image.readUInt16LE(1);
    const cdl = Buffer.alloc(image.length);
    cdl.fill(0x01, entry - 0x8000);
    const cdlFile = path.join(temp.dir, 'sb.cdl');
    writeFileSync(cdlFile, cdl);
    const byCdl = await run({ inputFile: exe, base: 0x8000, cdlFile });
    assert.equal(byCdl.verified, true);
    assert.ok(byCdl.instructions > 5);

    const traceFile = writeTestFile(temp.dir, 'sb.trace', `${traceLine(entry)}${traceLine(0x1234)}`);
    const byTrace = await run({ inputFile: exe, base: 0x8000, traceFile, outDir: path.join(temp.dir, 'out2') });
    assert.equal(byTrace.verified, true);
    assert.ok(byTrace.instructions > 5);
    assert.ok(byTrace.warnings.some((warning) => warning.includes('指令轨迹') && warning.includes('$1234')));
    assert.deepEqual(readProject(files, byTrace.projectDir).entries.map((item) => item.source), ['trace']);
  });

  it('缺少加载地址、缺少入口、映射器不支持、格式名不对时报错', async () => {
    const exe = await buildProject('sb2000', 'hello.c', '#include <sb2000.h>\nint main(void) { return 0; }\n');
    await assert.rejects(run({ inputFile: exe, format: 'flat', entries: [0x8000] }), (error: FcdevError) => error.code === 'ARGUMENT_MISSING' && error.hint.includes('加载地址'));
    await assert.rejects(run({ inputFile: exe, base: 0x8000 }), (error: FcdevError) => error.code === 'ARGUMENT_MISSING' && error.hint.includes('入口'));
    await assert.rejects(run({ inputFile: exe, base: 0x8000, entries: [0x1234] }), (error: FcdevError) => error.code === 'DISASM_ENTRY_INVALID');
    const mapper5 = path.join(temp.dir, 'm5.nes');
    writeFileSync(mapper5, makeInes({ mapper: 5 }));
    await assert.rejects(run({ inputFile: mapper5 }), (error: FcdevError) => error.code === 'DISASM_MAPPER_UNSUPPORTED');
    await assert.rejects(run({ inputFile: exe, format: 'binary', base: 0x8000, entries: [0x8000] }), (error: FcdevError) => error.code === 'DISASM_FORMAT_UNKNOWN');
  });
});

const externalRom = process.env.FCDEV_TEST_NES_ROM;

describe.skipIf(externalRom === undefined || !existsSync(externalRom) || !HAS_TOOLCHAIN)('真实 NES 游戏（FCDEV_TEST_NES_ROM）', () => {
  it('回读与原映像逐字节一致', async () => {
    const temp = createTempDir();
    try {
      const report = await DISASM.disassemble({ inputFile: externalRom as string, outDir: temp.dir });
      assert.equal(report.verified, true);
      assert.ok(report.instructions > 100);
    } finally {
      temp.cleanup();
    }
  });
});
