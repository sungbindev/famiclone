// ------------------------------------------------------------------------
// 名称：disasm-xrefs-detect.test.mjs
// 说明：测试交叉引用与候选识别：调用、跳转、分支、读、写、取地址、地址表引用；填充、地址表、字符串、图形的识别。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { describe, it } from 'node:test';
import {
  buildRegions, buildXrefs, createAddressSpace, createAnalysis, createRegionMap, detectCandidates, entryAtAddress, findXrefs, memoryAccess, parseFlat, setRegion, traceCode,
} from '../src/disasm/index.mjs';

/** 平铺映像并从给定入口追踪 */
function traced(bytes, entries, address = 0x8000) {
  const buffer = Buffer.from(bytes);
  const { layout } = parseFlat(buffer, { address });
  const space = createAddressSpace(layout);
  const analysis = createAnalysis(layout);
  traceCode(analysis, { bytes: buffer, layout }, entries.map((entry) => entryAtAddress(space, entry, { source: 'manual' })));
  return { bytes: buffer, layout, analysis };
}

describe('内存访问分类', () => {
  it('读、写、先读后写，累加器、立即数、隐含、控制转移不访问内存', () => {
    assert.equal(memoryAccess('lda', 'absolute'), 'read');
    assert.equal(memoryAccess('bit', 'zeroPage'), 'read');
    assert.equal(memoryAccess('cmp', 'indirectIndexed'), 'read');
    assert.equal(memoryAccess('sta', 'absoluteX'), 'write');
    assert.equal(memoryAccess('inc', 'zeroPage'), 'readWrite');
    assert.equal(memoryAccess('asl', 'absoluteX'), 'readWrite');
    assert.equal(memoryAccess('asl', 'accumulator'), null);
    assert.equal(memoryAccess('lda', 'immediate'), null);
    assert.equal(memoryAccess('jmp', 'absolute'), null);
    assert.equal(memoryAccess('jsr', 'absolute'), null);
    assert.equal(memoryAccess('bne', 'relative'), null);
    assert.equal(memoryAccess('tax', 'implied'), null);
  });
});

describe('交叉引用', () => {
  const PROGRAM = [
    0xa9, 0x14,       // C000 LDA #$14
    0xa2, 0xc0,       // C002 LDX #$C0
    0x20, 0x14, 0xc0, // C004 JSR $C014
    0xad, 0x02, 0x20, // C007 LDA $2002
    0x9d, 0x00, 0x20, // C00A STA $2000,X
    0xe6, 0x10,       // C00D INC $10
    0xd0, 0xef,       // C00F BNE $C000
    0x4c, 0x14, 0xc0, // C011 JMP $C014
    0x60,             // C014 RTS
  ];

  it('区分调用、跳转、分支、读、写、先读后写、取地址', () => {
    const { bytes, layout, analysis } = traced(PROGRAM, [0xc000], 0xc000);
    const index = buildXrefs({ bytes, layout, analysis });
    assert.deepEqual(findXrefs(index, 0xc014).map((ref) => [ref.kind, ref.address]), [['call', 0xc004], ['jump', 0xc011], ['address', 0xc000]]);
    assert.deepEqual(findXrefs(index, 0x2002).map((ref) => [ref.kind, ref.address, ref.mnemonic]), [['read', 0xc007, 'lda']]);
    assert.deepEqual(findXrefs(index, 0x2000).map((ref) => [ref.kind, ref.indexed]), [['write', true]]);
    assert.deepEqual(findXrefs(index, 0x0010).map((ref) => [ref.kind, ref.indexed]), [['readWrite', false]]);
    assert.deepEqual(findXrefs(index, 0xc000).map((ref) => [ref.kind, ref.address]), [['branch', 0xc00f]]);
    assert.deepEqual(findXrefs(index, 0xc014, { kind: 'jump' }).map((ref) => ref.address), [0xc011]);
    assert.deepEqual(findXrefs(index, 0x1234), []);
    assert.equal(index.count, 7);
  });

  it('引用带被引用地址所在的段，不在任何段里时为 null；取地址标为推测', () => {
    const { bytes, layout, analysis } = traced(PROGRAM, [0xc000], 0xc000);
    const index = buildXrefs({ bytes, layout, analysis });
    const [call, , address] = findXrefs(index, 0xc014);
    assert.equal(call.targetSegmentId, 0);
    assert.equal(call.likely, undefined);
    assert.equal(address.likely, true);
    assert.equal(findXrefs(index, 0x2002)[0].targetSegmentId, null);
  });

  it('立即数加载凑不成段里的地址时不算取地址', () => {
    const { bytes, layout, analysis } = traced([0xa9, 0x01, 0xa2, 0x02, 0x60], [0x8000]);
    assert.equal(buildXrefs({ bytes, layout, analysis }).count, 0);
  });

  it('间接 JMP 算对指针的读，地址表的每一项引用它的目标', () => {
    const { bytes, layout, analysis } = traced([0x6c, 0x05, 0x80, 0xff, 0xff, 0x07, 0x80, 0x60], [0x8000]);
    const index = buildXrefs({ bytes, layout, analysis });
    assert.deepEqual(findXrefs(index, 0x8005).map((ref) => [ref.kind, ref.mnemonic]), [['read', 'jmp']]);
    assert.deepEqual(findXrefs(index, 0x8007).map((ref) => [ref.kind, ref.address]), [['table', 0x8005]]);
  });
});

describe('候选识别', () => {
  /** 代码调用三个子程序，后面依次是字表、字符串与 20 个 $FF 的填充 */
  function dataProgram(tableValues = [0x8010, 0x8012, 0x8014]) {
    const bytes = new Array(0x36).fill(0);
    bytes.splice(0, 10, 0x20, 0x10, 0x80, 0x20, 0x12, 0x80, 0x20, 0x14, 0x80, 0x60);
    for (const start of [0x10, 0x12, 0x14]) bytes.splice(start, 2, 0xea, 0x60);
    tableValues.forEach((value, index) => bytes.splice(0x16 + index * 2, 2, value & 0xff, value >> 8));
    bytes.splice(0x1c, 6, 0x48, 0x45, 0x4c, 0x4c, 0x4f, 0x00);
    bytes.fill(0xff, 0x22, 0x36);
    return traced(bytes, [0x8000]);
  }

  const detect = (image, options) => detectCandidates({ bytes: image.bytes, layout: image.layout, analysis: image.analysis, regions: createRegionMap(image.layout), options });

  it('填充、地址表、字符串各给出候选，附依据与可信度', () => {
    const found = detect(dataProgram());
    assert.deepEqual(found.padding.map((item) => [item.offset, item.length, item.params.value, item.confidence]), [[0x22, 20, 0xff, 'high']]);
    assert.deepEqual(found.tables.map((item) => [item.offset, item.address, item.length, item.params, item.confidence]), [[0x16, 0x8016, 6, { adjust: 0 }, 'high']]);
    assert.match(found.tables[0].evidence, /3 个 16 位值，其中 3 个指向已识别的指令开头/);
    assert.deepEqual(found.strings.map((item) => [item.offset, item.length, item.params, item.confidence]), [[0x1c, 6, { encoding: 'ascii', terminator: 0 }, 'high']]);
    assert.match(found.strings[0].evidence, /5 个字符，以 \$00 结尾：“HELLO”/);
    assert.deepEqual(found.graphics, []);
  });

  it('RTS 跳转表：值加 1 才指向指令开头', () => {
    const found = detect(dataProgram([0x800f, 0x8011, 0x8013]));
    assert.deepEqual(found.tables.map((item) => [item.offset, item.length, item.params]), [[0x16, 6, { adjust: 1 }]]);
    assert.match(found.tables[0].evidence, /疑似 RTS 跳转表/);
  });

  it('指向指令开头的表项不到 3 个时不算地址表', () => {
    const found = detect(dataProgram([0x8010, 0x8012, 0x9999]));
    assert.deepEqual(found.tables, []);
  });

  it('已被追到的字节、已有区域标注的字节都不参与识别', () => {
    const image = dataProgram();
    const regions = createRegionMap(image.layout);
    setRegion(regions, { offset: 0x1c, length: 6, type: 'bytes', source: 'manual' });
    setRegion(regions, { offset: 0x16, length: 6, type: 'words', source: 'manual' });
    const found = detectCandidates({ bytes: image.bytes, layout: image.layout, analysis: image.analysis, regions });
    assert.deepEqual([found.tables, found.strings], [[], []]);
    assert.equal(found.padding.length, 1);
    const withTrace = buildRegions(image.layout, image.analysis);
    assert.equal(detectCandidates({ bytes: image.bytes, layout: image.layout, analysis: image.analysis, regions: withTrace }).tables.length, 1);
  });

  it('字符串：以 $00 结尾、最高位置位结尾、没有结尾标记，太短的不算', () => {
    const bytes = Buffer.from([...Buffer.from('GAME OVER'), 0x01, ...Buffer.from('DON'), 0xc5, 0x01, ...Buffer.from('AB'), 0x00, 0x01, ...Buffer.from('OK!!'), 0x00]);
    const { layout } = parseFlat(bytes, { address: 0x8000 });
    const analysis = createAnalysis(layout);
    const found = detectCandidates({ bytes, layout, analysis, regions: createRegionMap(layout) }).strings;
    assert.deepEqual(found.map((item) => [item.offset, item.length, item.params.terminator, item.confidence]), [[0, 9, null, 'medium'], [10, 4, 'high-bit', 'high'], [19, 5, 0, 'high']]);
    assert.match(found[1].evidence, /最后一个字符的最高位置位：“DONE”/);
    assert.match(found[0].evidence, /没有结尾标记/);
  });

  it('图形：只有给出图形数据端口并且代码把表里的值写进端口时才识别', () => {
    const bytes = new Array(0x20).fill(0);
    bytes.splice(0, 12, 0xa2, 0x00, 0xbd, 0x10, 0x80, 0x8d, 0x07, 0x20, 0xe8, 0xd0, 0xf7, 0x60);
    for (let index = 0; index < 16; index += 1) bytes[0x10 + index] = index + 1;
    const image = traced(bytes, [0x8000]);
    assert.deepEqual(detect(image).graphics, []);
    const found = detect(image, { graphicsPorts: [0x2007] }).graphics;
    assert.deepEqual(found.map((item) => [item.offset, item.address, item.length, item.params, item.confidence]), [[0x10, 0x8010, 16, { format: 'unspecified' }, 'medium']]);
    assert.match(found[0].evidence, /\$2007/);
    assert.deepEqual(detect(image, { graphicsPorts: [0x2006] }).graphics, []);
  });
});
