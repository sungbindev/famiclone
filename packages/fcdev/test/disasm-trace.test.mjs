// ------------------------------------------------------------------------
// 名称：disasm-trace.test.mjs
// 说明：测试静态追踪：逐字节的分类、换页窗口、跨 bank 转移、间接跳转与地址表的登记，以及分多次追踪。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：地址表、PHA 加 RTS 表等写法的夹具集中在 FIXTURES；真实程序的用例需要 cc65 工具链。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath } from '../src/build/index.mjs';
import {
  BYTE_KIND, createAddressSpace, createAnalysis, createLayout, entriesFromVectors, entryAtAddress, parseFlat, parseInes, traceCode,
} from '../src/disasm/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** 平铺映像：布局、映像字节与分析结果 */
function flatImage(bytes, address = 0x8000) {
  const buffer = Buffer.from(bytes);
  const { layout } = parseFlat(buffer, { address });
  return { bytes: buffer, layout, space: createAddressSpace(layout), analysis: createAnalysis(layout) };
}

/** 对平铺映像从给定地址追踪 */
function traceFlat(bytes, entryAddresses, address = 0x8000) {
  const image = flatImage(bytes, address);
  const entries = entryAddresses.map((entry) => entryAtAddress(image.space, entry, { source: 'manual' }));
  const summary = traceCode(image.analysis, image, entries);
  return { ...image, summary };
}

/** 地址表、间接跳转等写法的夹具：映像、加载地址、入口 */
const FIXTURES = {
  '顺序、条件分支与子程序调用': { bytes: [0xa9, 0x05, 0xf0, 0x03, 0xea, 0x60, 0xff, 0x20, 0x0b, 0xc0, 0x60, 0xe8, 0x60], base: 0xc000, entries: [0xc000] },
  'JMP 只追目标': { bytes: [0x4c, 0x05, 0x80, 0xff, 0xff, 0x60], base: 0x8000, entries: [0x8000] },
  '间接 JMP、RTI 与 BRK 结束这一路': { bytes: [0x6c, 0x00, 0x02, 0xea, 0x40, 0xea, 0x00, 0xea], base: 0x8000, entries: [0x8000, 0x8004, 0x8006] },
  '跳到映像之外': { bytes: [0x20, 0x00, 0x20, 0x60], base: 0x8000, entries: [0x8000] },
  '字表加间接 JMP': { bytes: [0x0a, 0xaa, 0xbd, 0x10, 0x80, 0x85, 0x19, 0xbd, 0x11, 0x80, 0x85, 0x1a, 0x6c, 0x19, 0x00, 0xff, 0x14, 0x80, 0x15, 0x80, 0x60, 0x60], base: 0x8000, entries: [0x8000] },
  '表里的目标不是官方操作码': { bytes: [0x0a, 0xaa, 0xbd, 0x10, 0x80, 0x85, 0x19, 0xbd, 0x11, 0x80, 0x85, 0x1a, 0x6c, 0x19, 0x00, 0xff, 0x0f, 0x80, 0x0f, 0x80, 0x60, 0x60], base: 0x8000, entries: [0x8000] },
  'PHA、PHA、RTS 的字表': { bytes: [0xbd, 0x0f, 0x80, 0x48, 0xbd, 0x0e, 0x80, 0x48, 0x60, 0xff, 0xff, 0xff, 0xff, 0xff, 0x11, 0x80, 0x12, 0x80, 0x60, 0x60], base: 0x8000, entries: [0x8000] },
  'PHA、PHA、RTS 的分开的两张表': { bytes: [0xbd, 0x12, 0x80, 0x48, 0xbd, 0x0f, 0x80, 0x48, 0x60, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x13, 0x14, 0xff, 0x80, 0x80, 0x60, 0x60], base: 0x8000, entries: [0x8000] },
  'JMP (地址) 指向映像内的字表': { bytes: [0x6c, 0x05, 0x80, 0xff, 0xff, 0x07, 0x80, 0x60], base: 0x8000, entries: [0x8000] },
};

/** 每个夹具追踪后每个字节的分类（0 未知、1 操作码、2 操作数、3 地址表）与指令条数 */
const EXPECTED_KINDS = {
  '顺序、条件分支与子程序调用': { kinds: [1, 2, 1, 2, 1, 1, 0, 1, 2, 2, 1, 1, 1], instructions: 8 },
  'JMP 只追目标': { kinds: [1, 2, 2, 0, 0, 1], instructions: 2 },
  '间接 JMP、RTI 与 BRK 结束这一路': { kinds: [1, 2, 2, 0, 1, 0, 1, 0], instructions: 3 },
  '跳到映像之外': { kinds: [1, 2, 2, 1], instructions: 2 },
  '字表加间接 JMP': { kinds: [1, 1, 1, 2, 2, 1, 2, 1, 2, 2, 1, 2, 1, 2, 2, 0, 3, 3, 3, 3, 1, 1], instructions: 9 },
  '表里的目标不是官方操作码': { kinds: [1, 1, 1, 2, 2, 1, 2, 1, 2, 2, 1, 2, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0], instructions: 7 },
  'PHA、PHA、RTS 的字表': { kinds: [1, 2, 2, 1, 1, 2, 2, 1, 1, 0, 0, 0, 0, 0, 3, 3, 3, 3, 1, 1], instructions: 7 },
  'PHA、PHA、RTS 的分开的两张表': { kinds: [1, 2, 2, 1, 1, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1], instructions: 7 },
  'JMP (地址) 指向映像内的字表': { kinds: [1, 2, 2, 0, 0, 3, 3, 1], instructions: 2 },
};

describe('静态追踪', () => {
  for (const [name, expected] of Object.entries(EXPECTED_KINDS)) {
    it(`${name}：每个字节的分类与指令条数`, () => {
      const { bytes, base, entries } = FIXTURES[name];
      const { analysis, summary } = traceFlat(bytes, entries, base);
      assert.deepEqual([...analysis.kinds], expected.kinds);
      assert.equal(summary.instructions, expected.instructions);
    });
  }

  it('顺序、分支、调用都被追到，不可达的字节保持未知，指令记录有地址与长度', () => {
    const { analysis, summary } = traceFlat(FIXTURES['顺序、条件分支与子程序调用'].bytes, [0xc000], 0xc000);
    assert.deepEqual([...analysis.kinds], [1, 2, 1, 2, 1, 1, 0, 1, 2, 2, 1, 1, 1]);
    assert.deepEqual(summary, { instructions: 8, tables: 0, warnings: 0 });
    assert.deepEqual(analysis.instructions.get(7), { segmentId: 0, address: 0xc007, size: 3 });
    assert.equal(analysis.instructions.size, 8);
  });

  it('非官方操作码、指令超出末尾、重叠：放弃这一路，警告带地址与原因码', () => {
    const illegal = traceFlat([0x02, 0xea], [0x8000]).analysis.warnings;
    assert.deepEqual(illegal.map((item) => [item.code, item.address]), [['illegal', 0x8000]]);
    assert.match(illegal[0].message, /\$8000：操作码 \$02 不是官方/);
    const cut = traceFlat([0x4c, 0x00], [0x8000]).analysis.warnings;
    assert.deepEqual(cut.map((item) => item.code), ['truncated']);
    const overlap = traceFlat([0xa9, 0xea, 0x60], [0x8001, 0x8000]).analysis.warnings;
    assert.deepEqual(overlap.map((item) => [item.code, item.address]), [['overlap', 0x8001]]);
  });

  it('顺序执行到段尾：警告并放弃，不继续', () => {
    const { analysis } = traceFlat([0xea, 0xea], [0x8000]);
    assert.deepEqual(analysis.warnings.map((item) => item.code), ['segment-end']);
    assert.deepEqual([...analysis.kinds], [1, 1]);
  });

  it('指令的操作数越过段尾：警告，不标记', () => {
    const buffer = Buffer.from([0xea, 0x4c, 0x00, 0x80, 0x60, 0x60]);
    const layout = createLayout([{ id: 0, fileOffset: 0, length: 2, address: 0x8000 }, { id: 1, fileOffset: 2, length: 4, address: 0x9000 }], { imageSize: 6 });
    const analysis = createAnalysis(layout);
    traceCode(analysis, { bytes: buffer, layout }, [{ segmentId: 0, address: 0x8000 }]);
    assert.deepEqual(analysis.warnings.map((item) => item.code), ['segment-end']);
    assert.deepEqual([...analysis.kinds], [1, 0, 0, 0, 0, 0]);
  });

  it('相邻的段可以顺序执行下去', () => {
    const buffer = Buffer.from([0xea, 0xea, 0x60, 0x60]);
    const layout = createLayout([{ id: 0, fileOffset: 0, length: 2, address: 0x8000 }, { id: 1, fileOffset: 2, length: 2, address: 0x8002 }], { imageSize: 4 });
    const analysis = createAnalysis(layout);
    const summary = traceCode(analysis, { bytes: buffer, layout }, [{ segmentId: 0, address: 0x8000 }]);
    assert.equal(summary.instructions, 3);
    assert.deepEqual(analysis.warnings, []);
  });

  it('目标不在任何段里：记入 outside，不算警告，同一处只记一次', () => {
    const { analysis } = traceFlat([0x20, 0x00, 0x20, 0x20, 0x00, 0x20, 0x60], [0x8000]);
    assert.deepEqual(analysis.warnings, []);
    assert.deepEqual(analysis.outside, [
      { segmentId: 0, address: 0x8000, target: 0x2000, flow: 'call' },
      { segmentId: 0, address: 0x8003, target: 0x2000, flow: 'call' },
    ]);
  });

  it('间接 JMP 没有找到地址表时登记为未解析，找到表后从未解析里去掉', () => {
    const unresolved = traceFlat([0x6c, 0x00, 0x02, 0xea], [0x8000]).analysis;
    assert.deepEqual(unresolved.unresolved, [{ kind: 'indirect-jump', segmentId: 0, address: 0x8000, offset: 0, pointer: 0x0200 }]);

    const resolved = traceFlat(FIXTURES['JMP (地址) 指向映像内的字表'].bytes, [0x8000]).analysis;
    assert.deepEqual(resolved.unresolved, []);
    assert.equal(resolved.tables.length, 1);
    assert.deepEqual(resolved.tables[0], {
      key: 'w:5:0', kind: 'words', segmentId: 0, offset: 5, address: 0x8005, highOffset: null, adjust: 0, count: 1, targets: [0x8007], jumpOffsets: [0],
    });
    assert.equal(resolved.kinds[5], BYTE_KIND.ADDRESS);
  });

  it('PHA、PHA、RTS 的表：字表的目标加 1，分开的两张表记为 split', () => {
    const word = traceFlat(FIXTURES['PHA、PHA、RTS 的字表'].bytes, [0x8000]).analysis.tables[0];
    assert.deepEqual([word.kind, word.adjust, word.targets], ['words', 1, [0x8012, 0x8013]]);
    const split = traceFlat(FIXTURES['PHA、PHA、RTS 的分开的两张表'].bytes, [0x8000]).analysis.tables[0];
    assert.deepEqual([split.kind, split.offset, split.highOffset, split.count, split.targets], ['split', 0x0f, 0x12, 2, [0x8014, 0x8015]]);
  });

  it('分多次追踪：新增数量只算这一次，已追到的不重复', () => {
    const image = flatImage([0xea, 0x60, 0x00, 0xa9, 0x01, 0x60]);
    const first = traceCode(image.analysis, image, [entryAtAddress(image.space, 0x8000, { source: 'manual' })]);
    const second = traceCode(image.analysis, image, [entryAtAddress(image.space, 0x8000, { source: 'manual' }), entryAtAddress(image.space, 0x8003, { source: 'manual' })]);
    assert.deepEqual(first, { instructions: 2, tables: 0, warnings: 0 });
    assert.deepEqual(second, { instructions: 2, tables: 0, warnings: 0 });
    assert.equal(image.analysis.instructions.size, 4);
  });

  it('向量生成入口：iNES 的三个向量都被追到', () => {
    const prg = Buffer.alloc(0x4000, 0xff);
    prg.set([0xa9, 0x01, 0x60, 0xea, 0x40, 0xea, 0x40], 0);
    prg.writeUInt16LE(0xc003, 0x4000 - 6);
    prg.writeUInt16LE(0xc000, 0x4000 - 4);
    prg.writeUInt16LE(0xc005, 0x4000 - 2);
    const header = Buffer.alloc(16);
    header.write('NES\x1a', 0, 'latin1');
    header[4] = 1;
    const parsed = parseInes(Buffer.concat([header, prg]));
    const space = createAddressSpace(parsed.layout);
    const { entries, skipped } = entriesFromVectors(space, parsed.vectors);
    assert.deepEqual(entries.map((entry) => [entry.address, entry.source, entry.description]), [[0xc000, 'vector', '复位向量'], [0xc003, 'vector', 'NMI 向量'], [0xc005, 'vector', 'IRQ/BRK 向量']]);
    assert.deepEqual(skipped, []);
    const analysis = createAnalysis(parsed.layout);
    const summary = traceCode(analysis, { bytes: Buffer.concat([header, prg]), layout: parsed.layout }, entries);
    assert.equal(summary.instructions, 6);
  });

  it('向量指向没有映射的地址时不生成入口，写明原因', () => {
    const image = flatImage([0xea]);
    const { entries, skipped } = entriesFromVectors(image.space, { nmi: 0x8000, reset: 0x1234, irq: 0x8000 });
    assert.equal(entries.length, 2);
    assert.deepEqual(skipped.map((item) => [item.name, item.address]), [['reset', 0x1234]]);
    assert.deepEqual(entriesFromVectors(image.space, null), { entries: [], skipped: [] });
  });
});

describe('换页窗口', () => {
  /** 固定区 $C000 在文件 0，bank 0 与 bank 1 都映在 $8000，各占 16 字节 */
  function bankedImage() {
    const bytes = Buffer.alloc(48, 0xff);
    bytes.set([0x20, 0x00, 0x80, 0x60], 0x00); // 固定区：JSR $8000、RTS
    bytes.set([0x20, 0x04, 0x80, 0x60, 0xa9, 0x01, 0x60], 0x10); // bank 0：JSR $8004、RTS、LDA #1、RTS
    bytes.set([0x4c, 0x06, 0x80, 0xff, 0xff, 0xff, 0xe8, 0x60], 0x20); // bank 1：JMP $8006、INX、RTS
    const layout = createLayout([
      { id: 0, fileOffset: 0x00, length: 0x10, address: 0xc000 },
      { id: 1, fileOffset: 0x10, length: 0x10, address: 0x8000, bank: 0 },
      { id: 2, fileOffset: 0x20, length: 0x10, address: 0x8000, bank: 1 },
    ], { imageSize: 48 });
    return { bytes, layout, space: createAddressSpace(layout), analysis: createAnalysis(layout) };
  }

  it('同一个窗口里的转移留在当前 bank，两个 bank 的字节互不混淆', () => {
    const image = bankedImage();
    traceCode(image.analysis, image, [{ segmentId: 1, address: 0x8000 }, { segmentId: 2, address: 0x8000 }]);
    assert.deepEqual([...image.analysis.kinds.subarray(0x10, 0x17)], [1, 2, 2, 1, 1, 2, 1]);
    assert.deepEqual([...image.analysis.kinds.subarray(0x20, 0x28)], [1, 2, 2, 0, 0, 0, 1, 1]);
    assert.deepEqual(image.analysis.warnings, []);
  });

  it('从固定区转移到换页窗口：无法确定 bank，登记为跨 bank 转移，不追下去', () => {
    const image = bankedImage();
    traceCode(image.analysis, image, [{ segmentId: 0, address: 0xc000 }]);
    assert.deepEqual(image.analysis.unresolved, [{ kind: 'cross-bank', segmentId: 0, address: 0xc000, offset: 0, target: 0x8000, candidates: [1, 2] }]);
    assert.deepEqual([...image.analysis.kinds.subarray(0x10, 0x20)], new Array(16).fill(0));
    assert.deepEqual([...image.analysis.kinds.subarray(0, 4)], [1, 2, 2, 1]);
  });

  it('入口地址在多个 bank 里都有映射时必须给段号', () => {
    const { space } = bankedImage();
    assert.throws(() => entryAtAddress(space, 0x8000, { source: 'manual' }), (error) => error.code === 'DISASM_ENTRY_AMBIGUOUS' && error.hint.includes('段号'));
    assert.equal(entryAtAddress(space, 0x8000, { source: 'manual', segmentId: 2 }).segmentId, 2);
    assert.equal(entryAtAddress(space, 0xc000, { source: 'manual' }).segmentId, 0);
    assert.throws(() => entryAtAddress(space, 0x4000, { source: 'manual' }), (error) => error.code === 'DISASM_ENTRY_INVALID');
    assert.throws(() => entryAtAddress(space, 0x8000, { source: 'manual', segmentId: 9 }), (error) => error.code === 'DISASM_ENTRY_INVALID');
  });

  it('地址空间解析：来源在窗口内的 bank 里就留在那个 bank', () => {
    const { space } = bankedImage();
    assert.equal(space.resolve(0x8005, 2).segment.id, 2);
    assert.equal(space.resolve(0x8005, 1).segment.id, 1);
    assert.equal(space.resolve(0x8005, 0).status, 'ambiguous');
    assert.equal(space.resolve(0x8005).status, 'ambiguous');
    assert.equal(space.resolve(0x0000, 0).status, 'unmapped');
  });
});

describe('fcdev 构建的真实程序（需要 cc65）', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
  let temp;
  beforeEach(() => { temp = createTempDir(); });
  afterEach(() => temp.cleanup());

  /** 构建一个 fcdev 项目并返回产物路径 */
  async function buildProject(target, fileName, source) {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target }));
    writeTestFile(temp.dir, fileName, source);
    const { result } = await buildPath({ pathArg: fileName, cwd: temp.dir });
    return result.outputFile;
  }

  it('NES 程序：入口取自向量，指令条数与操作码字节数一致', async () => {
    const file = readFileSync(await buildProject('nes', 'hello.c', '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "HI"); nes_screen_on(); for (;;) { } }\n'));
    const parsed = parseInes(file);
    const { entries, skipped } = entriesFromVectors(createAddressSpace(parsed.layout), parsed.vectors);
    assert.deepEqual(skipped, []);
    const analysis = createAnalysis(parsed.layout);
    const summary = traceCode(analysis, { bytes: file, layout: parsed.layout }, entries);

    const segment = parsed.layout.segments[0];
    const kinds = analysis.kinds.subarray(segment.fileOffset, segment.fileOffset + segment.length);
    assert.ok(kinds.some((kind) => kind === BYTE_KIND.OPCODE));
    assert.equal(kinds.filter((kind) => kind === BYTE_KIND.OPCODE).length, summary.instructions);
    assert.ok(summary.instructions > 50);
  });

  it('SB2000 程序：平铺在 $8000，入口取自文件头的 JMP', async () => {
    const file = readFileSync(await buildProject('sb2000', 'hello.c', '#include <sb2000.h>\nint main(void) { sb_puts("Hi"); return 0; }\n'));
    const entry = file.readUInt16LE(1);
    const { layout } = parseFlat(file, { address: 0x8000 });
    const analysis = createAnalysis(layout);
    const summary = traceCode(analysis, { bytes: file, layout }, [entryAtAddress(createAddressSpace(layout), entry, { source: 'manual' })]);
    assert.equal([...analysis.kinds].filter((kind) => kind === BYTE_KIND.OPCODE).length, summary.instructions);
    assert.ok(summary.instructions > 20);
  });
});
