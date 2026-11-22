// ------------------------------------------------------------------------
// 名称：disasm-regions.test.ts
// 说明：测试区域模型、入口模型，以及由追踪结果生成区域表。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { describe, it } from 'vitest';
import type { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { REGION_TYPES, createRegionMap, listRegions, regionAt, regionStats, setRegion } from '../../../src/shared/disassembly/model/regions';
import { buildRegions } from '../../../src/shared/disassembly/analysis/build-regions';
import { createAnalysis, traceCode } from '../../../src/shared/disassembly/analysis/tracer';
import { createEntry } from '../../../src/shared/disassembly/model/entries';
import { createLayout } from '../../../src/shared/disassembly/model/layout';
import { entryAtAddress } from '../../../src/shared/disassembly/analysis/entry-points';
import { createAddressSpace } from '../../../src/shared/disassembly/analysis/address-space';
import { parseFlat } from '../../../src/shared/disassembly/formats/flat';

/** 两个段：段 0 在文件 0 到 0x40，段 1 在文件 0x40 到 0x80 */
function twoSegments() {
  return createLayout([
    { id: 0, fileOffset: 0x00, length: 0x40, address: 0x8000, bank: 0 },
    { id: 1, fileOffset: 0x40, length: 0x40, address: 0x8000, bank: 1 },
  ], { imageSize: 0x80 });
}

const manual = (offset: number, length: number, type: string, params?: Record<string, unknown>) => ({ offset, length, type, params, source: 'manual' });

describe('区域模型', () => {
  it('设置区域：按偏移排序，查询与列出时没有标注的部分是 unknown', () => {
    const map = createRegionMap(twoSegments());
    setRegion(map, manual(0x10, 8, 'bytes', { perLine: 4 }));
    setRegion(map, manual(0x00, 4, 'code'));
    assert.deepEqual(map.regions.map((region) => [region.offset, region.length, region.type]), [[0, 4, 'code'], [0x10, 8, 'bytes']]);
    assert.equal(regionAt(map, 0x12)!.type, 'bytes');
    assert.equal(regionAt(map, 0x08), null);
    assert.deepEqual(listRegions(map, 0, 0x20).map((region) => [region.offset, region.length, region.type]), [[0, 4, 'code'], [4, 12, 'unknown'], [0x10, 8, 'bytes'], [0x18, 8, 'unknown']]);
    assert.deepEqual(listRegions(map, 0x12, 4).map((region) => [region.offset, region.length, region.type]), [[0x12, 4, 'bytes']]);
  });

  it('新区域盖住旧区域：被拆开或裁掉，两头保留', () => {
    const map = createRegionMap(twoSegments());
    setRegion(map, manual(0x00, 0x20, 'bytes'));
    setRegion(map, manual(0x08, 4, 'string', { encoding: 'ascii', terminator: 0 }));
    assert.deepEqual(map.regions.map((region) => [region.offset, region.length, region.type]), [[0, 8, 'bytes'], [8, 4, 'string'], [12, 0x14, 'bytes']]);
    setRegion(map, manual(0x04, 0x10, 'padding', { value: 0 }));
    assert.deepEqual(map.regions.map((region) => [region.offset, region.length, region.type]), [[0, 4, 'bytes'], [4, 0x10, 'padding'], [0x14, 0x0c, 'bytes']]);
  });

  it('类型 unknown 等于清除', () => {
    const map = createRegionMap(twoSegments());
    setRegion(map, manual(0x00, 0x10, 'bytes'));
    assert.equal(setRegion(map, manual(0x04, 4, 'unknown')), null);
    assert.deepEqual(map.regions.map((region) => [region.offset, region.length]), [[0, 4], [8, 8]]);
  });

  it('地址表：长度必须是偶数，只能在表项边界上被切开', () => {
    const map = createRegionMap(twoSegments());
    assert.throws(() => setRegion(map, manual(0, 3, 'words')), (error: FcdevError) => error.code === 'DISASM_REGION_INVALID');
    setRegion(map, manual(0, 8, 'words', { adjust: 1 }));
    assert.throws(() => setRegion(map, manual(3, 1, 'bytes')), (error: FcdevError) => error.code === 'DISASM_REGION_CONFLICT');
    assert.throws(() => setRegion(map, manual(0, 3, 'bytes')), (error: FcdevError) => error.code === 'DISASM_REGION_CONFLICT');
    assert.equal(map.regions.length, 1);
    setRegion(map, manual(2, 2, 'bytes'));
    assert.deepEqual(map.regions.map((region) => [region.offset, region.length, region.type]), [[0, 2, 'words'], [2, 2, 'bytes'], [4, 4, 'words']]);
  });

  it('区域必须完整地落在一个段里，不能落在段外', () => {
    const map = createRegionMap(twoSegments());
    assert.throws(() => setRegion(map, manual(0x3c, 8, 'bytes')), (error: FcdevError) => error.code === 'DISASM_REGION_INVALID' && error.message.includes('某一个段'));
    assert.throws(() => setRegion(map, manual(0x7c, 8, 'bytes')), (error: FcdevError) => error.code === 'DISASM_REGION_INVALID');
    assert.throws(() => setRegion(map, manual(-1, 2, 'bytes')), (error: FcdevError) => error.code === 'DISASM_REGION_INVALID');
    assert.throws(() => setRegion(map, manual(0, 0, 'bytes')), (error: FcdevError) => error.code === 'DISASM_REGION_INVALID');
  });

  it('类型、参数与来源的检查', () => {
    const map = createRegionMap(twoSegments());
    assert.throws(() => setRegion(map, manual(0, 4, 'sprite')), /未知的区域类型/);
    assert.throws(() => setRegion(map, manual(0, 4, 'bytes', { width: 4 })), /没有参数 width/);
    assert.throws(() => setRegion(map, manual(0, 4, 'bytes', { perLine: 0 })), /取值不合法/);
    assert.throws(() => setRegion(map, manual(0, 4, 'string', { terminator: 1 })), /取值不合法/);
    assert.throws(() => setRegion(map, manual(0, 4, 'padding', { value: 256 })), /取值不合法/);
    assert.throws(() => setRegion(map, { offset: 0, length: 4, type: 'bytes', source: '' }), /来源/);
    assert.deepEqual(setRegion(map, manual(0, 4, 'string', { encoding: 'ascii', terminator: 'high-bit' }))!.params, { encoding: 'ascii', terminator: 'high-bit' });
    assert.deepEqual(REGION_TYPES, ['code', 'bytes', 'words', 'string', 'graphics', 'padding', 'unknown']);
  });

  it('统计各类型的字节数，没有标注的字节算 unknown', () => {
    const map = createRegionMap(twoSegments());
    setRegion(map, manual(0, 0x10, 'code'));
    setRegion(map, manual(0x40, 0x20, 'bytes'));
    assert.deepEqual(regionStats(map), { code: 0x10, bytes: 0x20, words: 0, string: 0, graphics: 0, padding: 0, unknown: 0x80 - 0x30 });
  });
});

describe('入口模型', () => {
  const layout = twoSegments();

  it('创建入口：检查段、地址与来源', () => {
    const entry = createEntry(layout, { segmentId: 1, address: 0x8000, source: 'manual', description: '主循环' });
    assert.deepEqual(entry, { segmentId: 1, address: 0x8000, source: 'manual', description: '主循环' });
    assert.equal(Object.isFrozen(entry), true);
    assert.equal(createEntry(layout, { segmentId: 0, address: 0x8001, source: 'trace' }).description, '');
    assert.throws(() => createEntry(layout, { segmentId: 5, address: 0x8000, source: 'manual' }), (error: FcdevError) => error.code === 'DISASM_ENTRY_INVALID');
    assert.throws(() => createEntry(layout, { segmentId: 0, address: 0x9000, source: 'manual' }), (error: FcdevError) => error.code === 'DISASM_ENTRY_INVALID');
    assert.throws(() => createEntry(layout, { segmentId: 0, address: 0x8000, source: 'guess' }), (error: FcdevError) => error.code === 'DISASM_ENTRY_INVALID' && error.hint.includes('vector'));
  });
});

describe('由追踪结果生成区域', () => {
  it('连续的指令字节是 code 区域，字表是 words，分开的两张表是 bytes，没追到的字节不标注', () => {
    // 与追踪测试里“字表加间接 JMP”相同的夹具
    const buffer = Buffer.from([0x0a, 0xaa, 0xbd, 0x10, 0x80, 0x85, 0x19, 0xbd, 0x11, 0x80, 0x85, 0x1a, 0x6c, 0x19, 0x00, 0xff, 0x14, 0x80, 0x15, 0x80, 0x60, 0x60]);
    const { layout } = parseFlat(buffer, { address: 0x8000 });
    const analysis = createAnalysis(layout);
    traceCode(analysis, { bytes: buffer, layout }, [entryAtAddress(createAddressSpace(layout), 0x8000, { source: 'manual' })]);
    const map = buildRegions(layout, analysis);
    assert.deepEqual(map.regions.map((region) => [region.offset, region.length, region.type, region.source]), [
      [0, 15, 'code', 'trace'], [16, 4, 'words', 'table'], [20, 2, 'code', 'trace'],
    ]);
    assert.deepEqual(map.regions[1]!.params, { adjust: 0 });
  });

  it('低、高字节分开的两张表生成两个 bytes 区域，写明各自的角色', () => {
    const buffer = Buffer.from([0xbd, 0x12, 0x80, 0x48, 0xbd, 0x0f, 0x80, 0x48, 0x60, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x13, 0x14, 0xff, 0x80, 0x80, 0x60, 0x60]);
    const { layout } = parseFlat(buffer, { address: 0x8000 });
    const analysis = createAnalysis(layout);
    traceCode(analysis, { bytes: buffer, layout }, [entryAtAddress(createAddressSpace(layout), 0x8000, { source: 'manual' })]);
    const regions = buildRegions(layout, analysis).regions.filter((region) => region.type === 'bytes');
    assert.deepEqual(regions.map((region) => [region.offset, region.length, region.params.role]), [[15, 2, 'split-address-low'], [18, 2, 'split-address-high']]);
  });

  it('多个段：区域不跨段，每段各自成块', () => {
    const buffer = Buffer.from([0xea, 0x60, 0xea, 0x60]);
    const layout = createLayout([{ id: 0, fileOffset: 0, length: 2, address: 0x8000, bank: 0 }, { id: 1, fileOffset: 2, length: 2, address: 0x8000, bank: 1 }], { imageSize: 4 });
    const analysis = createAnalysis(layout);
    traceCode(analysis, { bytes: buffer, layout }, [{ segmentId: 0, address: 0x8000 }, { segmentId: 1, address: 0x8000 }]);
    assert.deepEqual(buildRegions(layout, analysis).regions.map((region) => [region.offset, region.length]), [[0, 2], [2, 2]]);
  });
});
